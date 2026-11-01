/* $Id$ */
/** @file
 * Shared Clipboard - private file snapshot tests without a system pasteboard.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#include <VBox/GuestHost/ClipboardFileCache.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/test.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/** @page pg_tstClipboardFileCache Clipboard file snapshot regression tests
 * Exercises the real local transfer provider against private temporary files.
 * Malformed metadata, limits and cancellation must not publish partial paths.
 */

static bool g_fCancel;
static uint32_t g_cChecks;
static uint32_t g_cCancelAfter;
static DECLCALLBACK(bool) tstCanceled(void *)
{
    ++g_cChecks;
    return g_fCancel || (g_cCancelAfter && g_cChecks >= g_cCancelAfter);
}

/** A provider that stops before the advertised end of the file. */
static DECLCALLBACK(int) tstShortRead(PSHCLTXPROVIDERCTX, SHCLOBJHANDLE, void *, uint32_t, uint32_t, uint32_t *pcbRead)
{
    *pcbRead = 0;
    return VINF_SUCCESS;
}

/** Counts unpublished snapshots left after an error. */
static uint32_t tstCacheDirectories(const char *pszCache)
{
    uint32_t cDirectories = 0;
    DIR *pDir = opendir(pszCache);
    if (pDir)
    {
        struct dirent *pEntry;
        while ((pEntry = readdir(pDir)) != NULL)
            if (!strncmp(pEntry->d_name, "copy-", 5))
                ++cDirectories;
        closedir(pDir);
    }
    return cDirectories;
}

static PSHCLTRANSFER tstTransfer(const char *pszSource)
{
    PSHCLTRANSFER pTransfer = NULL;
    SHCLTXPROVIDER Provider;
    RT_ZERO(Provider);
    ShClTransferProviderLocalQueryInterface(&Provider);
    RTTESTI_CHECK_RC(ShClTransferCreate(SHCLTRANSFERDIR_HOST_TO_GUEST, SHCLSOURCE_LOCAL, NULL, &pTransfer), VINF_SUCCESS);
    if (!pTransfer)
        return NULL;
    RTTESTI_CHECK_RC(ShClTransferSetProvider(pTransfer, &Provider), VINF_SUCCESS);
    RTTESTI_CHECK_RC(ShClTransferRootsSetFromPath(pTransfer, pszSource), VINF_SUCCESS);
    RTTESTI_CHECK_RC(ShClTransferInit(pTransfer), VINF_SUCCESS);
    return pTransfer;
}

static void tstWrite(const char *pszPath)
{
    RTFILE hFile = NIL_RTFILE;
    RTTESTI_CHECK_RC(RTFileOpen(&hFile, pszPath, RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE), VINF_SUCCESS);
    if (hFile != NIL_RTFILE)
    {
        uint8_t abData[_64K + 17];
        for (size_t i = 0; i < sizeof(abData); ++i)
            abData[i] = (uint8_t)(i % 251);
        RTTESTI_CHECK_RC(RTFileWrite(hFile, abData, sizeof(abData), NULL), VINF_SUCCESS);
        RTFileClose(hFile);
    }
}

int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstClipboardFileCache", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);
    char szRoot[RTPATH_MAX];
    RTTESTI_CHECK_RC_RET(RTPathTemp(szRoot, sizeof(szRoot)), VINF_SUCCESS, 1);
    RTTESTI_CHECK_RC_RET(RTPathAppend(szRoot, sizeof(szRoot), "tstClipboardFileCache-XXXXXX"), VINF_SUCCESS, 1);
    RTTESTI_CHECK_RC_RET(RTDirCreateTemp(szRoot, 0700), VINF_SUCCESS, 1);
    char szReal[RTPATH_MAX];
    RTTESTI_CHECK_RC_RET(RTPathReal(szRoot, szReal, sizeof(szReal)), VINF_SUCCESS, 1);
    RTStrCopy(szRoot, sizeof(szRoot), szReal);
    char szSource[RTPATH_MAX];
    char szCache[RTPATH_MAX];
    char szFile[RTPATH_MAX];
    RTPathJoin(szSource, sizeof(szSource), szRoot, "source");
    RTPathJoin(szCache, sizeof(szCache), szRoot, "cache");
    RTTESTI_CHECK_RC(RTDirCreate(szSource, 0700, 0), VINF_SUCCESS);
    RTPathJoin(szFile, sizeof(szFile), szSource, "hello 世界.bin");
    tstWrite(szFile);

    RTTestSub(hTest, "Exact file contents and private permissions");
    PSHCLTRANSFER pTransfer = tstTransfer(szFile);
    char *pszDirectory = NULL;
    char *pszRoots = NULL;
    size_t cbRoots = 0;
    int vrc = shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL, &pszDirectory, &pszRoots, &cbRoots);
    RTTESTI_CHECK_RC(vrc, VINF_SUCCESS);
    if (RT_SUCCESS(vrc))
    {
        RTTESTI_CHECK(cbRoots == strlen(pszRoots) + 1);
        char szReceived[RTPATH_MAX];
        RTPathJoin(szReceived, sizeof(szReceived), pszDirectory, "hello 世界.bin");
        void *pvData = NULL;
        size_t cbData = 0;
        RTTESTI_CHECK_RC(RTFileReadAll(szReceived, &pvData, &cbData), VINF_SUCCESS);
        RTTESTI_CHECK(cbData == _64K + 17);
        if (pvData)
            for (size_t i = 0; i < cbData; ++i)
                RTTESTI_CHECK(((uint8_t *)pvData)[i] == (uint8_t)(i % 251));
        RTFileReadAllFree(pvData, cbData);
        struct stat Info;
        RTTESTI_CHECK(stat(szReceived, &Info) == 0 && (Info.st_mode & 0777) == 0600);
        RTTESTI_CHECK(stat(pszDirectory, &Info) == 0 && (Info.st_mode & 0777) == 0700);
        RTTESTI_CHECK_RC(shClFileCacheDiscard(pszDirectory), VINF_SUCCESS);
    }
    RTStrFree(pszDirectory); RTStrFree(pszRoots);
    ShClTransferDestroy(pTransfer);

    RTTestSub(hTest, "Limit and cancellation return no paths");
    pTransfer = tstTransfer(szFile);
    pszDirectory = NULL; pszRoots = NULL;
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, 1, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_TOO_MUCH_DATA);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    g_fCancel = true;
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_CANCELLED);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    g_fCancel = false;
    ShClTransferDestroy(pTransfer);

    RTTestSub(hTest, "Short read and cancellation discard partial data");
    pTransfer = tstTransfer(szFile);
    pTransfer->ProviderIface.pfnObjRead = tstShortRead;
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_EOF);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    ShClTransferDestroy(pTransfer);
    pTransfer = tstTransfer(szFile);
    g_cChecks = 0; g_cCancelAfter = 3;
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_CANCELLED);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    g_cCancelAfter = 0;
    ShClTransferDestroy(pTransfer);
    RTTESTI_CHECK(tstCacheDirectories(szCache) == 0);

    RTTestSub(hTest, "Nested directories and empty files");
    char szNested[RTPATH_MAX];
    RTPathJoin(szNested, sizeof(szNested), szSource, "nested");
    RTTESTI_CHECK_RC(RTDirCreate(szNested, 0700, 0), VINF_SUCCESS);
    char szEmpty[RTPATH_MAX];
    RTPathJoin(szEmpty, sizeof(szEmpty), szNested, "empty");
    RTFILE hEmpty = NIL_RTFILE;
    RTTESTI_CHECK_RC(RTFileOpen(&hEmpty, szEmpty, RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE), VINF_SUCCESS);
    RTFileClose(hEmpty);
    pTransfer = tstTransfer(szSource);
    vrc = shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                               &pszDirectory, &pszRoots, &cbRoots);
    RTTESTI_CHECK_RC(vrc, VINF_SUCCESS);
    if (RT_SUCCESS(vrc))
    {
        char szReceived[RTPATH_MAX];
        RTPathJoin(szReceived, sizeof(szReceived), pszDirectory, "source/nested/empty");
        struct stat Info;
        RTTESTI_CHECK(stat(szReceived, &Info) == 0 && Info.st_size == 0);
        RTTESTI_CHECK_RC(shClFileCacheDiscard(pszDirectory), VINF_SUCCESS);
    }
    RTStrFree(pszDirectory); RTStrFree(pszRoots);
    pszDirectory = NULL; pszRoots = NULL;
    ShClTransferDestroy(pTransfer);

    RTTestSub(hTest, "Reject traversal and special file metadata");
    pTransfer = tstTransfer(szFile);
    PSHCLLISTENTRY pEntry = (PSHCLLISTENTRY)ShClTransferRootsEntryGet(pTransfer, 0);
    RTTESTI_CHECK(pEntry != NULL);
    if (pEntry)
    {
        RTStrFree(pEntry->pszName);
        pEntry->pszName = RTStrDup("../escape");
        pEntry->cbName = 10;
        vrc = shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                   &pszDirectory, &pszRoots, &cbRoots);
        RTTESTI_CHECK(RT_FAILURE(vrc) && !pszDirectory && !pszRoots);
    }
    ShClTransferDestroy(pTransfer);
    pTransfer = tstTransfer(szFile);
    pEntry = (PSHCLLISTENTRY)ShClTransferRootsEntryGet(pTransfer, 0);
    ((PSHCLFSOBJINFO)pEntry->pvInfo)->Attr.fMode = RTFS_TYPE_SYMLINK | 0777;
    vrc = shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                               &pszDirectory, &pszRoots, &cbRoots);
    RTTESTI_CHECK(RT_FAILURE(vrc) && !pszDirectory && !pszRoots);
    ShClTransferDestroy(pTransfer);
    RTTESTI_CHECK(tstCacheDirectories(szCache) == 0);

    RTTestSub(hTest, "Reject unsafe cache roots and lock permissions");
    pTransfer = tstTransfer(szFile);
    char szAlias[RTPATH_MAX];
    RTPathJoin(szAlias, sizeof(szAlias), szRoot, "cache-link");
    RTTESTI_CHECK(symlink(szCache, szAlias) == 0);
    vrc = shClFileCacheReceive(pTransfer, szAlias, _1M, tstCanceled, NULL,
                               &pszDirectory, &pszRoots, &cbRoots);
    RTTESTI_CHECK(RT_FAILURE(vrc) && !pszDirectory && !pszRoots);
    RTTESTI_CHECK(unlink(szAlias) == 0);
    char szLock[RTPATH_MAX];
    RTPathJoin(szLock, sizeof(szLock), szCache, ".lock");
    RTTESTI_CHECK(chmod(szLock, 0644) == 0);
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_ACCESS_DENIED);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    RTTESTI_CHECK(chmod(szLock, 0600) == 0);

    RTTestSub(hTest, "Aggregate quota includes existing snapshots");
    char szQuota[RTPATH_MAX];
    RTPathJoin(szQuota, sizeof(szQuota), szCache, "quota-test");
    RTFILE hQuota = NIL_RTFILE;
    RTTESTI_CHECK_RC(RTFileOpen(&hQuota, szQuota, RTFILE_O_CREATE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RTFileSetSize(hQuota, UINT64_C(9) * _1G), VINF_SUCCESS);
    RTFileClose(hQuota);
    RTTESTI_CHECK_RC(shClFileCacheReceive(pTransfer, szCache, _1M, tstCanceled, NULL,
                                         &pszDirectory, &pszRoots, &cbRoots), VERR_DISK_FULL);
    RTTESTI_CHECK(!pszDirectory && !pszRoots && !cbRoots);
    RTTESTI_CHECK_RC(RTFileDelete(szQuota), VINF_SUCCESS);
    ShClTransferDestroy(pTransfer);
    RTDirRemoveRecursive(szRoot, 0);
    return RTTestSummaryAndDestroy(hTest);
}
