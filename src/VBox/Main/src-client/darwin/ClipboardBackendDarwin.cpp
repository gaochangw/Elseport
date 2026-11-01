/* $Id: ClipboardBackendDarwin.cpp 115131 2026-08-25 17:30:42Z andreas.loeffler@oracle.com $ */
/** @file
 * Shared Clipboard Service - Mac OS X host.
 */

/*
 * Copyright (C) 2008-2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <VBox/HostServices/VBoxClipboardSvc.h>
#include "GuestShClBackend.h"
#include "../GuestShClBackendPrivate.h"
#include "GuestShClConn.h"

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/critsect.h>
#include <iprt/env.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/rand.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include "darwin-pasteboard.h"
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
# include <VBox/GuestHost/ClipboardFileCache.h>
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Global clipboard context information */
typedef struct SHCLCONTEXT
{
    /** We have a separate thread to poll for new clipboard content. */
    RTTHREAD                hThread;
    /** Termination indicator.   */
    bool volatile           fTerminate;
    /** The reference to the current pasteboard */
    PasteboardRef           hPasteboard;
    /** Main connection to the Shared Clipboard service. */
    GuestShClConn          *pConn;
    /** Whether @a pConn may be used by the pasteboard poller. */
    bool                    fClientReady;
    /** Number of polls left for looking again at a pasteboard which had nothing
     * usable on it after it changed, see SHCL_DARWIN_RECHECK_POLLS. */
    uint32_t                cRecheckPolls;
    /** Ownership probes can consume Carbon's modified bit; force the next poll in that case. */
    bool volatile           fForceSync;
    /** Random 64-bit number embedded into szGuestOwnershipFlavor. */
    uint64_t                idGuestOwnership;
    /** Ownership flavor CFStringRef returned by takePasteboardOwnership().
     * This is the same a szGuestOwnershipFlavor only in core foundation terms. */
    void                   *hStrOwnershipFlavor;
    /** The guest ownership flavor (type) string. */
    char                    szGuestOwnershipFlavor[64];
    /** Serialize access to the current pasteboard. */
    RTCRITSECT              CritSectPasteboard;
    /** Serialize the client pointer and its readiness state. */
    RTCRITSECT              CritSect;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /** Worker which receives complete file snapshots outside pasteboard callbacks. */
    RTTHREAD                hFileThread;
    /** Wakes the file worker for a new offer or shutdown. */
    RTSEMEVENT              hFileEvent;
    /** Invalidate previous work whenever either endpoint replaces the clipboard. */
    uint64_t                uFileGeneration;
    /** Current guest formats; only the worker consumes file offers. */
    SHCLFORMATS             fFileFormats;
    /** Stop before the connection or pasteboard is released. */
    bool                    fFileStopping;
#endif
} SHCLCONTEXT;


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** How many polls to look again at a changed pasteboard with nothing usable on
 * it, about three seconds. */
#define SHCL_DARWIN_RECHECK_POLLS   15


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Only one client is supported. There seems to be no need for more clients. */
static SHCLCONTEXT g_ctx;

/** Ownership checks must not hide a concurrent host copy from the poller. */
static bool shClDarwinIsPasteboardOwner(PSHCLCONTEXT pCtx, uint64_t idOwnership)
{
    bool const fOwner = isPasteboardOwner(pCtx->hPasteboard, idOwnership, pCtx->hStrOwnershipFlavor);
    if (!fOwner)
        ASMAtomicWriteBool(&pCtx->fForceSync, true);
    return fOwner;
}


#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
static void shClBackendDarwinTransferGetCallbacks(PSHCLCONTEXT pCtx, PSHCLTRANSFERCALLBACKS pCallbacks);

/** Identity of one background file offer. */
struct SHCLDARWINFILEWORK
{
    PSHCLCONTEXT pCtx;
    uint64_t uGeneration;
    uint64_t idOwnership;
};

/** Cancellation includes user replacements of the host clipboard. */
static DECLCALLBACK(bool) shClDarwinFileCanceled(void *pvUser)
{
    SHCLDARWINFILEWORK *pWork = (SHCLDARWINFILEWORK *)pvUser;
    PSHCLCONTEXT pCtx = pWork->pCtx;
    RTCritSectEnter(&pCtx->CritSect);
    bool fCanceled = pCtx->fFileStopping || !pCtx->fClientReady
                   || pCtx->uFileGeneration != pWork->uGeneration;
    if (!fCanceled)
    {
        RTCritSectEnter(&pCtx->CritSectPasteboard);
        fCanceled = pCtx->idGuestOwnership != pWork->idOwnership
                 || !shClDarwinIsPasteboardOwner(pCtx, pWork->idOwnership);
        RTCritSectLeave(&pCtx->CritSectPasteboard);
    }
    RTCritSectLeave(&pCtx->CritSect);
    return fCanceled;
}

/** Prepares and publishes file URLs only after a complete, current transfer. */
static int shClDarwinFileReceive(PSHCLCONTEXT pCtx, SHCLDARWINFILEWORK *pWork)
{
    GuestShClConn *pConn = pCtx->pConn; /* Disconnect joins this worker before clearing pConn. */
    void *pvOffer = NULL;
    uint32_t cbOffer = 0;
    int vrc = pConn->readDataFromGuest(VBOX_SHCL_FMT_URI_LIST, &pvOffer, &cbOffer);
    RTMemFree(pvOffer);
    /* Windows announces file descriptors without a raw URI-list payload. The
     * transfer provider, rather than this optional data reply, supplies roots. */
    if (vrc == VERR_SHCLPB_NO_DATA || vrc == VERR_NOT_SUPPORTED)
        vrc = VINF_SUCCESS;
    if (RT_SUCCESS(vrc) && shClDarwinFileCanceled(pWork))
        vrc = VERR_CANCELLED;
    PSHCLTRANSFER pTransfer = NULL;
    SHCLTRANSFERID idTransfer = NIL_SHCLTRANSFERID;
    if (RT_SUCCESS(vrc))
    {
        SHCLTRANSFERCALLBACKS Callbacks;
        shClBackendDarwinTransferGetCallbacks(pCtx, &Callbacks);
        vrc = pConn->transferCreate(SHCLTRANSFERDIR_GUEST_TO_HOST, SHCLSOURCE_REMOTE, &Callbacks,
                                   NIL_SHCLTRANSFERID, &pTransfer);
    }
    if (RT_SUCCESS(vrc))
    {
        idTransfer = ShClTransferGetID(pTransfer);
        vrc = pConn->transferInit(pTransfer);
    }
    if (RT_SUCCESS(vrc))
        vrc = ShClTransferWaitForStatus(pTransfer, SHCL_TIMEOUT_DEFAULT_MS, SHCLTRANSFERSTATUS_INITIALIZED);
    if (RT_SUCCESS(vrc) && shClDarwinFileCanceled(pWork))
        vrc = VERR_CANCELLED;
    if (RT_SUCCESS(vrc))
        vrc = ShClTransferRootListRead(pTransfer);
    char szCache[RTPATH_MAX];
    if (RT_SUCCESS(vrc))
    {
        const char *pszHome = RTEnvGet("VBOX_USER_HOME");
        vrc = pszHome && *pszHome ? RTStrCopy(szCache, sizeof(szCache), pszHome)
                                 : RTPathTemp(szCache, sizeof(szCache));
        if (RT_SUCCESS(vrc))
            vrc = RTPathAppend(szCache, sizeof(szCache), "VBoxClipboardCache");
    }
    char *pszDirectory = NULL;
    char *pszRoots = NULL;
    size_t cbRoots = 0;
    if (RT_SUCCESS(vrc))
        vrc = shClFileCacheReceive(pTransfer, szCache, _1G, shClDarwinFileCanceled, pWork,
                                   &pszDirectory, &pszRoots, &cbRoots);
    bool fPublished = false;
    if (RT_SUCCESS(vrc))
    {
        RTCritSectEnter(&pCtx->CritSect);
        if (pCtx->fFileStopping || pCtx->uFileGeneration != pWork->uGeneration)
            vrc = VERR_CANCELLED;
        else
        {
            RTCritSectEnter(&pCtx->CritSectPasteboard);
            vrc = writeFileURLsToPasteboard(pCtx->hPasteboard, pWork->idOwnership,
                                            pCtx->hStrOwnershipFlavor, pszRoots, cbRoots);
            if (vrc == VERR_CANCELLED)
                ASMAtomicWriteBool(&pCtx->fForceSync, true);
            fPublished = RT_SUCCESS(vrc);
            RTCritSectLeave(&pCtx->CritSectPasteboard);
        }
        RTCritSectLeave(&pCtx->CritSect);
    }
    if (pszDirectory && !fPublished)
        shClFileCacheDiscard(pszDirectory);
    RTStrFree(pszDirectory);
    RTStrFree(pszRoots);
    if (RT_FAILURE(vrc))
    {
        RTCritSectEnter(&pCtx->CritSect);
        RTCritSectEnter(&pCtx->CritSectPasteboard);
        if (   pCtx->uFileGeneration == pWork->uGeneration
            && pCtx->idGuestOwnership == pWork->idOwnership
            && shClDarwinIsPasteboardOwner(pCtx, pWork->idOwnership))
            clearPasteboard(pCtx->hPasteboard, &pCtx->hStrOwnershipFlavor);
        RTCritSectLeave(&pCtx->CritSectPasteboard);
        RTCritSectLeave(&pCtx->CritSect);
    }
    if (pTransfer)
    {
        if (!ShClTransferStatusIsTerminal(ShClTransferGetStatus(pTransfer)))
        {
            if (RT_SUCCESS(vrc))
                ShClTransferComplete(pTransfer);
            else if (vrc == VERR_CANCELLED)
                ShClTransferCancel(pTransfer);
            else
                ShClTransferError(pTransfer, vrc);
            pConn->transferReportStatus(pTransfer, ShClTransferGetStatus(pTransfer), vrc);
        }
        ShClTransferRelease(pTransfer);
        pConn->transferDestroyById(idTransfer);
    }
    if (RT_FAILURE(vrc) && vrc != VERR_CANCELLED)
        LogRelMax(16, ("Shared Clipboard: Receiving a macOS file snapshot failed with %Rrc\n", vrc));
    return vrc;
}

/** Serial worker; clipboard callbacks never wait on guest file I/O. */
static DECLCALLBACK(int) shClDarwinFileThread(RTTHREAD hThread, void *pvUser)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pvUser;
    RTThreadUserSignal(hThread);
    uint64_t uProcessed = 0;
    for (;;)
    {
        int vrc = RTSemEventWait(pCtx->hFileEvent, RT_INDEFINITE_WAIT);
        if (RT_FAILURE(vrc))
            return vrc;
        for (;;)
        {
            RTCritSectEnter(&pCtx->CritSect);
            if (pCtx->fFileStopping)
            {
                RTCritSectLeave(&pCtx->CritSect);
                return VINF_SUCCESS;
            }
            SHCLDARWINFILEWORK Work = { pCtx, pCtx->uFileGeneration, 0 };
            bool const fFiles = RT_BOOL(pCtx->fFileFormats & VBOX_SHCL_FMT_URI_LIST);
            RTCritSectEnter(&pCtx->CritSectPasteboard);
            Work.idOwnership = pCtx->idGuestOwnership;
            RTCritSectLeave(&pCtx->CritSectPasteboard);
            RTCritSectLeave(&pCtx->CritSect);
            if (uProcessed == Work.uGeneration)
                break;
            uProcessed = Work.uGeneration;
            if (fFiles)
                shClDarwinFileReceive(pCtx, &Work);
        }
    }
}

/** Stops the worker before releasing the connection it calls. */
static void shClDarwinFileStop(PSHCLCONTEXT pCtx)
{
    RTCritSectEnter(&pCtx->CritSect);
    pCtx->fFileStopping = true;
    pCtx->fClientReady = false;
    pCtx->fFileFormats = VBOX_SHCL_FMT_NONE;
    ++pCtx->uFileGeneration;
    RTCritSectLeave(&pCtx->CritSect);
    if (pCtx->hFileThread != NIL_RTTHREAD)
    {
        int vrc = RTSemEventSignal(pCtx->hFileEvent);
        AssertRC(vrc);
        vrc = RTThreadWait(pCtx->hFileThread, RT_INDEFINITE_WAIT, NULL);
        AssertFatalRC(vrc);
        RTCritSectEnter(&pCtx->CritSect);
        pCtx->hFileThread = NIL_RTTHREAD;
        RTCritSectLeave(&pCtx->CritSect);
    }
    if (pCtx->hFileEvent != NIL_RTSEMEVENT)
    {
        RTSEMEVENT const hEvent = pCtx->hFileEvent;
        RTCritSectEnter(&pCtx->CritSect);
        pCtx->hFileEvent = NIL_RTSEMEVENT;
        RTCritSectLeave(&pCtx->CritSect);
        RTSemEventDestroy(hEvent);
    }
}

/** @copydoc SHCLTXPROVIDERIFACE::pfnRootListRead */
static DECLCALLBACK(int) shClSvcDarwinTransferIfaceHGRootListRead(PSHCLTXPROVIDERCTX pProviderCtx)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pProviderCtx->pvUser;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    char  *pszRoots = NULL;
    size_t cbRoots = 0;
    int vrc = RTCritSectEnter(&pCtx->CritSectPasteboard);
    if (RT_SUCCESS(vrc))
    {
        vrc = readFileURLsFromPasteboard(pCtx->hPasteboard, &pszRoots, &cbRoots);

        int const vrc2 = RTCritSectLeave(&pCtx->CritSectPasteboard);
        AssertRC(vrc2);
        if (RT_SUCCESS(vrc))
            vrc = vrc2;
    }

    if (RT_SUCCESS(vrc))
        vrc = ShClTransferRootsSetFromStringList(pProviderCtx->pTransfer, pszRoots, cbRoots);
    RTStrFree(pszRoots);
    return vrc;
}


/** @copydoc SHCLTRANSFERCALLBACKS::pfnOnCreated */
static DECLCALLBACK(void) shClSvcDarwinTransferOnCreatedCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pCbCtx->pvUser;
    AssertPtrReturnVoid(pCtx);

    PSHCLTRANSFER pTransfer = pCbCtx->pTransfer;
    AssertPtrReturnVoid(pTransfer);

    if (   ShClTransferGetDir(pTransfer) == SHCLTRANSFERDIR_HOST_TO_GUEST
        && ShClTransferGetSource(pTransfer) == SHCLSOURCE_LOCAL)
    {
        SHCLTXPROVIDER Provider;
        RT_ZERO(Provider);
        ShClTransferProviderLocalQueryInterface(&Provider);
        Provider.Interface.pfnRootListRead = shClSvcDarwinTransferIfaceHGRootListRead;
        Provider.enmSource = SHCLSOURCE_LOCAL;
        Provider.pvUser    = pCtx;
        Provider.cbUser    = sizeof(*pCtx);

        int const vrc = ShClTransferSetProvider(pTransfer, &Provider);
        AssertRC(vrc);
    }
    else if (ShClTransferGetDir(pTransfer) == SHCLTRANSFERDIR_GUEST_TO_HOST)
    {
        SHCLTXPROVIDER Provider;
        RT_ZERO(Provider);
        int vrc = pCtx->pConn->transferProviderInitGuest(&Provider);
        if (RT_SUCCESS(vrc))
            vrc = ShClTransferSetProvider(pTransfer, &Provider);
        AssertRC(vrc);
    }
}


/** @copydoc SHCLTRANSFERCALLBACKS::pfnOnInitialize */
static DECLCALLBACK(int) shClSvcDarwinTransferOnInitializeCallback(PSHCLTRANSFERCALLBACKCTX pCbCtx)
{
    PSHCLTRANSFER pTransfer = pCbCtx->pTransfer;
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    if (   ShClTransferGetDir(pTransfer) == SHCLTRANSFERDIR_HOST_TO_GUEST
        && ShClTransferGetSource(pTransfer) == SHCLSOURCE_LOCAL)
        return ShClTransferRootListRead(pTransfer);
    if (ShClTransferGetDir(pTransfer) == SHCLTRANSFERDIR_GUEST_TO_HOST)
        return VINF_SUCCESS;
    return VERR_NOT_SUPPORTED;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */


/**
 * Checks if something is present on the clipboard and calls shclSvcReportMsg.
 *
 * @returns IPRT status code (ignored).
 * @param   pCtx    The context.
 * @param   fForce  Whether to report the current pasteboard content even if
 *                  its change was already observed.
 *
 */
static int vboxClipboardChanged(PSHCLCONTEXT pCtx, bool fForce)
{
    int      vrc      = VINF_SUCCESS;
    uint32_t fFormats = 0;

    RTCritSectEnter(&pCtx->CritSect);

    if (   pCtx->pConn
        && pCtx->fClientReady)
    {
        /*
         * An application replacing the pasteboard content clears it first and adds the new
         * items afterwards, and only clearing it makes the pasteboard count as modified.
         * So, having looked right in between and found nothing usable, we have to keep
         * looking for a little while or we would miss the new content.
         */
        bool const fRecheck = pCtx->cRecheckPolls > 0;
        if (fRecheck)
            pCtx->cRecheckPolls--;

        /* Retrieve the formats currently in the clipboard and supported by VBox. */
        bool fChanged = false;
        vrc = RTCritSectEnter(&pCtx->CritSectPasteboard);
        if (RT_SUCCESS(vrc))
        {
            bool const fResync = ASMAtomicXchgBool(&pCtx->fForceSync, false);
            vrc = queryNewPasteboardFormats(pCtx->hPasteboard, pCtx->idGuestOwnership, pCtx->hStrOwnershipFlavor,
                                            fForce || fRecheck || fResync, &fFormats, &fChanged);

            int const vrc2 = RTCritSectLeave(&pCtx->CritSectPasteboard);
            AssertRC(vrc2);
            if (RT_SUCCESS(vrc))
                vrc = vrc2;
        }
        if (RT_SUCCESS(vrc) && fChanged)
        {
            if (fFormats != VBOX_SHCL_FMT_NONE)
                pCtx->cRecheckPolls = 0;
            else if (fRecheck)
                fChanged = false; /* Reported already. */
            else
                pCtx->cRecheckPolls = SHCL_DARWIN_RECHECK_POLLS;
        }
        if (RT_SUCCESS(vrc) && fChanged)
        {
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
            if (pCtx->fFileFormats & VBOX_SHCL_FMT_URI_LIST)
            {
                pCtx->fFileFormats = VBOX_SHCL_FMT_NONE;
                ++pCtx->uFileGeneration;
                if (pCtx->hFileEvent != NIL_RTSEMEVENT)
                    RTSemEventSignal(pCtx->hFileEvent);
            }
#endif
            vrc = pCtx->pConn->reportLocalFormats(fFormats);
        }
    }

    RTCritSectLeave(&pCtx->CritSect);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * @callback_method_impl{FNRTTHREAD, The poller thread.
 *
 * This thread will check for the arrival of new data on the clipboard.}
 */
static DECLCALLBACK(int) vboxClipboardThread(RTTHREAD ThreadSelf, void *pvUser)
{
    PSHCLCONTEXT pCtx = (PSHCLCONTEXT)pvUser;
    AssertPtr(pCtx);
    LogFlowFuncEnter();
    int vrc;

    while (!ASMAtomicReadBool(&pCtx->fTerminate))
    {
        vboxClipboardChanged(pCtx, false /* fForce */);

        /* Sleep for 200 msecs before next poll */
        vrc = RTThreadUserWait(ThreadSelf, 200);
        if (RT_SUCCESS(vrc))
            break;
    }

    LogFlowFuncLeaveRC(VINF_SUCCESS);
    return VINF_SUCCESS;
}


/**
 * Initializes the process-wide macOS clipboard backend.
 *
 * @returns VBox status code.
 */
static int shClBackendDarwinInit(void)
{
    g_ctx.fTerminate = false;
    g_ctx.fClientReady = false;
    g_ctx.fForceSync = false;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    g_ctx.hFileThread = NIL_RTTHREAD;
    g_ctx.hFileEvent = NIL_RTSEMEVENT;
    g_ctx.fFileStopping = true;
#endif

    int vrc;

    vrc = RTCritSectInit(&g_ctx.CritSect);
    AssertRCReturn(vrc, vrc);

    vrc = RTCritSectInit(&g_ctx.CritSectPasteboard);
    if (RT_FAILURE(vrc))
    {
        RTCritSectDelete(&g_ctx.CritSect);
        return vrc;
    }

    vrc = initPasteboard(&g_ctx.hPasteboard);
    if (RT_FAILURE(vrc))
    {
        RTCritSectDelete(&g_ctx.CritSectPasteboard);
        RTCritSectDelete(&g_ctx.CritSect);
        return vrc;
    }

    vrc = RTThreadCreate(&g_ctx.hThread, vboxClipboardThread, &g_ctx, 0,
                         RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "SHCLIP");
    if (RT_FAILURE(vrc))
    {
        g_ctx.hThread = NIL_RTTHREAD;
        destroyPasteboard(&g_ctx.hPasteboard);
        RTCritSectDelete(&g_ctx.CritSectPasteboard);
        RTCritSectDelete(&g_ctx.CritSect);
    }

    return vrc;
}

/**
 * Destroys the process-wide macOS clipboard backend.
 */
static void shClBackendDarwinDestroy(void)
{
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    shClDarwinFileStop(&g_ctx);
#endif
    /*
     * Signal the termination of the polling thread and wait for it to respond.
     */
    ASMAtomicWriteBool(&g_ctx.fTerminate, true);
    if (g_ctx.hThread != NIL_RTTHREAD)
    {
        int vrc = RTThreadUserSignal(g_ctx.hThread);
        if (RT_FAILURE(vrc))
            LogRelMax(16, ("Shared Clipboard: Waking the Darwin clipboard poller during shutdown failed with %Rrc;"
                           " waiting for its polling interval to expire\n", vrc));
        vrc = RTThreadWait(g_ctx.hThread, RT_INDEFINITE_WAIT, NULL);
        AssertFatalMsgRC(vrc, ("Reaping the Darwin clipboard poller failed with %Rrc\n", vrc));
        g_ctx.hThread = NIL_RTTHREAD;
    }

    /*
     * Destroy the hPasteboard and uninitialize the global context record.
     */
    destroyPasteboard(&g_ctx.hPasteboard);
    g_ctx.pConn = NULL;
    g_ctx.fClientReady = false;

    if (RTCritSectIsInitialized(&g_ctx.CritSectPasteboard))
        RTCritSectDelete(&g_ctx.CritSectPasteboard);
    if (RTCritSectIsInitialized(&g_ctx.CritSect))
        RTCritSectDelete(&g_ctx.CritSect);
}

/**
 * Connects a Main service connection to the macOS clipboard backend.
 *
 * @returns VBox status code.
 * @param   pConn               Main service connection to associate.
 * @param   ppCtx               Where to return the backend context.
 */
static int shClBackendDarwinConnect(GuestShClConn *pConn, PSHCLCONTEXT *ppCtx)
{
    AssertPtrReturn(pConn, VERR_INVALID_POINTER);
    AssertPtrReturn(ppCtx,   VERR_INVALID_POINTER);
    *ppCtx = NULL;

    RTCritSectEnter(&g_ctx.CritSect);

    int vrc;
    if (!g_ctx.pConn)
    {
        g_ctx.cRecheckPolls = 0;
        g_ctx.pConn = pConn;
        g_ctx.fClientReady = false;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        g_ctx.fFileStopping = false;
        g_ctx.fFileFormats = VBOX_SHCL_FMT_NONE;
        g_ctx.uFileGeneration = 0;
#endif
        *ppCtx = &g_ctx;
        vrc = VINF_SUCCESS;
    }
    else
        vrc = VERR_NOT_SUPPORTED; /* One client only. */

    RTCritSectLeave(&g_ctx.CritSect);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    if (RT_SUCCESS(vrc))
    {
        vrc = RTSemEventCreate(&g_ctx.hFileEvent);
        if (RT_SUCCESS(vrc))
            vrc = RTThreadCreate(&g_ctx.hFileThread, shClDarwinFileThread, &g_ctx, 0,
                                 RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "ShClMacFiles");
        if (RT_FAILURE(vrc))
        {
            shClDarwinFileStop(&g_ctx);
            RTCritSectEnter(&g_ctx.CritSect);
            g_ctx.pConn = NULL;
            *ppCtx = NULL;
            RTCritSectLeave(&g_ctx.CritSect);
        }
    }
#endif
    return vrc;
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
/**
 * Returns the macOS callbacks for a new transfer.
 *
 * @param   pCtx                Connected backend context.
 * @param   pCallbacks          Where to return the callback table.
 */
static void shClBackendDarwinTransferGetCallbacks(PSHCLCONTEXT pCtx, PSHCLTRANSFERCALLBACKS pCallbacks)
{
    AssertPtrReturnVoid(pCallbacks);
    RT_ZERO(*pCallbacks);
    AssertPtrReturnVoid(pCtx);
    AssertPtrReturnVoid(pCtx->pConn);

    pCallbacks->pvUser          = pCtx;
    pCallbacks->cbUser          = sizeof(*pCtx);
    pCallbacks->pfnOnCreated    = shClSvcDarwinTransferOnCreatedCallback;
    pCallbacks->pfnOnInitialize = shClSvcDarwinTransferOnInitializeCallback;
}
#endif

/**
 * Synchronizes macOS clipboard state with a connected guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Connected backend context.
 */
static int shClBackendDarwinSync(PSHCLCONTEXT pCtx)
{
    AssertPtrReturn(pCtx,          VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx->pConn, VERR_INVALID_POINTER);

    /* GuestShClConn publishes the connection while shClBackendDarwinConnect runs, but
     * do not expose it to the poller until the initial service sync. */
    RTCritSectEnter(&g_ctx.CritSect);
    int vrc = VINF_SUCCESS;
    if (pCtx->pConn)
    {
        pCtx->fClientReady = true;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        if (pCtx->hFileEvent != NIL_RTSEMEVENT)
            RTSemEventSignal(pCtx->hFileEvent);
#endif
    }
    else
        vrc = VERR_NOT_SUPPORTED;
    RTCritSectLeave(&g_ctx.CritSect);

    /* Sync the host clipboard content with the client. */
    if (RT_SUCCESS(vrc))
        vrc = vboxClipboardChanged(pCtx, true /* fForce */);
    return vrc;
}

/**
 * Disconnects a Main service connection from the macOS clipboard backend.
 *
 * @returns VBox status code.
 * @param   pCtx                Backend context to disconnect.
 */
static int shClBackendDarwinDisconnect(PSHCLCONTEXT pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx->pConn, VERR_INVALID_POINTER);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    GuestShClConn * const pConn = pCtx->pConn;
    shClDarwinFileStop(pCtx);
#endif

    RTCritSectEnter(&g_ctx.CritSect);

    if (pCtx->pConn)
    {
        pCtx->fClientReady = false;
        pCtx->pConn = NULL;
    }

    RTCritSectLeave(&g_ctx.CritSect);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    /* Transfer callback tables retain pCtx as their user argument. */
    pConn->transferDestroyAll();
#endif

    return VINF_SUCCESS;
}

/**
 * Reports guest clipboard formats to the macOS pasteboard.
 *
 * @returns VBox status code.
 * @param   pCtx                Connected backend context.
 * @param   fFormats            Guest formats, VBOX_SHCL_FMT_XXX.
 */
static int shClBackendDarwinReportFormats(PSHCLCONTEXT pCtx, SHCLFORMATS fFormats)
{
    AssertPtrReturn(pCtx,          VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx->pConn, VERR_INVALID_POINTER);

    LogFlowFunc(("fFormats=%02X\n", fFormats));

    RTCritSectEnter(&pCtx->CritSect);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    ++pCtx->uFileGeneration;
    pCtx->fFileFormats = fFormats;
    if (pCtx->hFileEvent != NIL_RTSEMEVENT)
        RTSemEventSignal(pCtx->hFileEvent);
#endif

    if (fFormats == VBOX_SHCL_FMT_NONE)
    {
        /* Only take back what is ours.  The GUI gets here when a VM session closes, and by
           then the pasteboard may hold what the user copied in another application. */
        int vrcClear = VINF_SUCCESS;
        RTCritSectEnter(&pCtx->CritSectPasteboard);
        if (shClDarwinIsPasteboardOwner(pCtx, pCtx->idGuestOwnership))
            vrcClear = clearPasteboard(pCtx->hPasteboard, &pCtx->hStrOwnershipFlavor);
        RTCritSectLeave(&pCtx->CritSectPasteboard);
        RTCritSectLeave(&pCtx->CritSect);
        return vrcClear;
    }

    RTCritSectEnter(&pCtx->CritSectPasteboard);

    /*
     * Generate a unique flavor string for this format announcement.
     */
    uint64_t idFlavor = RTRandU64();
    pCtx->idGuestOwnership = idFlavor;
    RTStrPrintf(pCtx->szGuestOwnershipFlavor, sizeof(pCtx->szGuestOwnershipFlavor),
                "org.virtualbox.sharedclipboard.%RTproc.%RX64", RTProcSelf(), idFlavor);

    /*
     * Empty the pasteboard and put our ownership indicator flavor there
     * with the stringified formats as value.
     */
    char szValue[32];
    RTStrPrintf(szValue, sizeof(szValue), "%#x", fFormats);

    int const vrc = takePasteboardOwnership(pCtx->hPasteboard, pCtx->idGuestOwnership,
                                           pCtx->szGuestOwnershipFlavor, szValue, &pCtx->hStrOwnershipFlavor);

    RTCritSectLeave(&pCtx->CritSectPasteboard);
    RTCritSectLeave(&pCtx->CritSect);
    if (RT_FAILURE(vrc))
        return vrc;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    if (fFormats & VBOX_SHCL_FMT_URI_LIST)
        return VINF_SUCCESS; /* The worker receives files and publishes concrete local URLs. */
#endif

    /*
     * Now, request the data from the guest.
     */
    return pCtx->pConn->readDataFromGuestAsync(fFormats, NULL /* ppEvent */);
}

/**
 * Reads clipboard data from the macOS pasteboard.
 *
 * @returns VBox status code.
 * @param   pCtx                Connected backend context.
 * @param   fFormat             Clipboard format to read.
 * @param   pvData              Destination buffer.
 * @param   cbData              Destination buffer size in bytes.
 * @param   pcbActual           Where to return the actual or required byte count.
 */
static int shClBackendDarwinReadData(PSHCLCONTEXT pCtx, SHCLFORMAT fFormat, void *pvData, uint32_t cbData, uint32_t *pcbActual)
{
    AssertPtrReturn(pCtx,      VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx->pConn, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,    VERR_INVALID_POINTER);
    AssertPtrReturn(pcbActual, VERR_INVALID_POINTER);

    RTCritSectEnter(&pCtx->CritSectPasteboard);

    /* Default to no data available. */
    *pcbActual = 0;

    int vrc = readFromPasteboard(pCtx->hPasteboard, fFormat, pvData, cbData, pcbActual);
    if (vrc == VERR_NOT_FOUND)
        LogRel2(("Shared Clipboard: Host-to-guest macOS clipboard data in format %#x is no longer available\n", fFormat));
    else if (RT_FAILURE(vrc))
        LogRelMax(16, ("Shared Clipboard: Reading up to %RU32 bytes of host-to-guest macOS clipboard data in format %#x (actual or required %RU32 bytes) failed with %Rrc\n",
                       cbData, fFormat, *pcbActual, vrc));

    RTCritSectLeave(&pCtx->CritSectPasteboard);

    return vrc;
}

/**
 * Writes guest clipboard data to the macOS pasteboard.
 *
 * @returns VBox status code.
 * @param   pCtx                Connected backend context.
 * @param   fFormat             Clipboard format to write.
 * @param   pvData              Data buffer.
 * @param   cbData              Data size in bytes.
 */
static int shClBackendDarwinWriteData(PSHCLCONTEXT pCtx, SHCLFORMAT fFormat, void *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx->pConn, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    if (fFormat == VBOX_SHCL_FMT_URI_LIST)
        return VINF_SUCCESS; /* The matching request event carries this offer to the file worker. */
#endif
    RTCritSectEnter(&pCtx->CritSect);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    if (pCtx->fFileFormats & VBOX_SHCL_FMT_URI_LIST)
    {
        RTCritSectLeave(&pCtx->CritSect);
        return VERR_CANCELLED;
    }
#endif
    RTCritSectEnter(&pCtx->CritSectPasteboard);

    int vrc = shClDarwinIsPasteboardOwner(pCtx, pCtx->idGuestOwnership)
            ? writeToPasteboard(pCtx->hPasteboard, pCtx->idGuestOwnership, pvData, cbData, fFormat)
            : VERR_CANCELLED;

    RTCritSectLeave(&pCtx->CritSectPasteboard);
    RTCritSectLeave(&pCtx->CritSect);

    if (RT_FAILURE(vrc))
        LogRelMax(16, ("Shared Clipboard: Writing %RU32 bytes of guest-to-host clipboard data in format %#x to the macOS pasteboard failed with %Rrc\n",
                       cbData, fFormat, vrc));

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
/**
 * Handles transfer status replies from the guest.
 *
 * @returns VBox status code.
 * @param   pCtx                Shared Clipboard backend context.
 * @param   pTransfer           Shared Clipboard transfer.
 * @param   enmSource           Transfer source which issued the reply.
 * @param   enmStatus           Transfer status.
 * @param   rcStatus            Transfer status code.
 */
static int shClBackendDarwinTransferHandleStatusReply(PSHCLCONTEXT pCtx, PSHCLTRANSFER pTransfer, SHCLSOURCE enmSource,
                                         SHCLTRANSFERSTATUS enmStatus, int rcStatus)
{
    RT_NOREF(pCtx, pTransfer, enmSource, enmStatus, rcStatus);

    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS */


/** Native macOS Shared Clipboard backend operations. */
static SHCLBACKENDOPS const s_ShClBackendDarwinOps =
{
    shClBackendDarwinInit,
    shClBackendDarwinDestroy,
    NULL,
    shClBackendDarwinConnect,
    shClBackendDarwinDisconnect,
    shClBackendDarwinReportFormats,
    shClBackendDarwinReadData,
    shClBackendDarwinWriteData,
    shClBackendDarwinSync,
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
    shClBackendDarwinTransferGetCallbacks,
    shClBackendDarwinTransferHandleStatusReply,
#endif
};


/**
 * Returns the native macOS Shared Clipboard backend operations.
 *
 * @returns Immutable macOS backend operation table.
 */
PCSHCLBACKENDOPS ShClBackendGetOps(void)
{
    return &s_ShClBackendDarwinOps;
}
