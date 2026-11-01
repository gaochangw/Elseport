/* $Id$ */
/** @file
 * Main Shared Clipboard - macOS pasteboard format conversion testcase.
 *
 * This uses a private pasteboard, so it does not touch the clipboard of the user.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
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
#include <Carbon/Carbon.h>
#include <ImageIO/ImageIO.h>

#include <iprt/errcore.h>
#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>

#include <iprt/formats/bmp.h>
#include <iprt/mem.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/test.h>
#include <iprt/utf16.h>

#include "darwin-pasteboard.h"

RT_GCC_NO_WARN_DEPRECATED_BEGIN /* The pasteboard API is deprecated since 12.0 */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest;

/** The test image, top row first, as opaque RGB. */
static const uint8_t g_abTestPixels[2][3][3] =
{
    { { 0xff, 0x00, 0x00 }, { 0x00, 0xff, 0x00 }, { 0x00, 0x00, 0xff } },
    { { 0xff, 0xff, 0x00 }, { 0x00, 0xff, 0xff }, { 0x10, 0x20, 0x30 } },
};


/**
 * Encodes the test image in the given format.
 */
static CFDataRef tstEncodeImage(CFStringRef hType)
{
    uint8_t abRgba[2 * 3 * 4];
    for (unsigned y = 0; y < 2; y++)
        for (unsigned x = 0; x < 3; x++)
        {
            uint8_t * const pb = &abRgba[(y * 3 + x) * 4];
            memcpy(pb, g_abTestPixels[y][x], 3);
            pb[3] = 0xff;
        }

    CGColorSpaceRef   hColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef      hContext    = CGBitmapContextCreate(abRgba, 3, 2, 8, 3 * 4, hColorSpace,
                                                          kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGImageRef        hImage      = CGBitmapContextCreateImage(hContext);
    CFMutableDataRef  hData       = CFDataCreateMutable(kCFAllocatorDefault, 0);
    CGImageDestinationRef hDst    = CGImageDestinationCreateWithData(hData, hType, 1, NULL);
    CGImageDestinationAddImage(hDst, hImage, NULL);
    RTTESTI_CHECK(CGImageDestinationFinalize(hDst));
    CFRelease(hDst);
    CGImageRelease(hImage);
    CGContextRelease(hContext);
    CGColorSpaceRelease(hColorSpace);
    return hData;
}


/**
 * Replaces the content of the pasteboard with a single flavor.
 */
static void tstPutFlavor(PasteboardRef hPasteboard, CFStringRef hFlavor, CFDataRef hData)
{
    RTTESTI_CHECK(PasteboardClear(hPasteboard) == noErr);
    PasteboardSynchronize(hPasteboard);
    RTTESTI_CHECK(PasteboardPutItemFlavor(hPasteboard, (PasteboardItemID)1, hFlavor, hData, kPasteboardFlavorNoFlags) == noErr);
}


/**
 * Reads a format, going through the buffer-too-small step the service goes through as well.
 *
 * @returns Heap buffer with the data, NULL on failure.
 */
static void *tstRead(PasteboardRef hPasteboard, uint32_t fFormat, uint32_t *pcb, int *prc)
{
    uint8_t  abTooSmall[1];
    uint32_t cbActual = 0;
    int vrc = readFromPasteboard(hPasteboard, fFormat, abTooSmall, sizeof(abTooSmall), &cbActual);
    if (prc)
        *prc = vrc;
    if (RT_FAILURE(vrc) || cbActual <= sizeof(abTooSmall))
        return NULL;

    void *pv = RTMemAllocZ(cbActual);
    uint32_t cbActual2 = 0;
    vrc = readFromPasteboard(hPasteboard, fFormat, pv, cbActual, &cbActual2);
    RTTESTI_CHECK_RC(vrc, VINF_SUCCESS);
    RTTESTI_CHECK(cbActual2 == cbActual);
    *pcb = cbActual2;
    return pv;
}


/**
 * Tests that an image in the given flavor is offered and comes out as the expected DIB.
 */
static void tstImageFlavor(PasteboardRef hPasteboard, const char *pszName, CFStringRef hFlavor)
{
    RTTestSubF(g_hTest, "%s image", pszName);

    CFDataRef hData = tstEncodeImage(hFlavor);
    tstPutFlavor(hPasteboard, hFlavor, hData);
    CFRelease(hData);

    uint32_t fFormats = 0;
    bool     fChanged = false;
    RTTESTI_CHECK_RC(queryNewPasteboardFormats(hPasteboard, 0, NULL, true /*fForce*/, &fFormats, &fChanged), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(fFormats & VBOX_SHCL_FMT_BITMAP, ("fFormats=%#x\n", fFormats));

    uint32_t cbDib = 0;
    uint8_t * const pbDib = (uint8_t *)tstRead(hPasteboard, VBOX_SHCL_FMT_BITMAP, &cbDib, NULL);
    RTTESTI_CHECK_RETV(pbDib);
    RTTESTI_CHECK_RETV(cbDib == sizeof(BMPWIN3XINFOHDR) + 2 * 3 * 4);

    BMPWIN3XINFOHDR const *pHdr = (BMPWIN3XINFOHDR const *)pbDib;
    RTTESTI_CHECK(pHdr->cbSize == sizeof(*pHdr));
    RTTESTI_CHECK(pHdr->uWidth == 3);
    RTTESTI_CHECK(pHdr->uHeight == 2);
    RTTESTI_CHECK(pHdr->cPlanes == 1);
    RTTESTI_CHECK(pHdr->cBits == 32);
    RTTESTI_CHECK(pHdr->enmCompression == BMP_COMPRESSION_TYPE_NONE);

    /* Rows are bottom-up and the pixels BGRX.  Allow for rounding in the color management. */
    uint8_t const *pbPixels = (uint8_t const *)(pHdr + 1);
    for (unsigned y = 0; y < 2; y++)
        for (unsigned x = 0; x < 3; x++)
        {
            uint8_t const * const pbGot      = &pbPixels[(y * 3 + x) * 4];
            uint8_t const * const pabExpect = g_abTestPixels[1 - y][x];
            for (unsigned i = 0; i < 3; i++)
                RTTESTI_CHECK_MSG(RT_ABS((int)pbGot[2 - i] - (int)pabExpect[i]) <= 3,
                                  ("row %u pixel %u channel %u: got %#x, expected %#x\n", y, x, i, pbGot[2 - i], pabExpect[i]));
        }

    /* This is what X11 guests use on the DIB and it only accepts the plain header. */
    void  *pvBmp = NULL;
    size_t cbBmp = 0;
    RTTESTI_CHECK_RC(ShClHlpDibToBmp(pbDib, cbDib, &pvBmp, &cbBmp), VINF_SUCCESS);
    RTMemFree(pvBmp);

    RTMemFree(pbDib);
}


/**
 * Tests that damaged image data fails cleanly.
 */
static void tstDamagedImage(PasteboardRef hPasteboard)
{
    RTTestSub(g_hTest, "Damaged image");

    static const uint8_t s_abGarbage[] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a, 1, 2, 3, 4, 5, 6, 7, 8 };
    CFDataRef hData = CFDataCreate(kCFAllocatorDefault, s_abGarbage, sizeof(s_abGarbage));
    tstPutFlavor(hPasteboard, kUTTypePNG, hData);
    CFRelease(hData);

    int      vrc = VINF_SUCCESS;
    uint32_t cb  = 0;
    void *pv = tstRead(hPasteboard, VBOX_SHCL_FMT_BITMAP, &cb, &vrc);
    RTTESTI_CHECK(pv == NULL);
    RTTESTI_CHECK_MSG(RT_FAILURE(vrc), ("vrc=%Rrc\n", vrc));
    RTMemFree(pv);
}


/**
 * Tests that a BMP flavor still is passed on as is.
 */
static void tstBmpFlavor(PasteboardRef hPasteboard)
{
    RTTestSub(g_hTest, "BMP image");

    uint8_t abDib[sizeof(BMPWIN3XINFOHDR) + 4];
    RT_ZERO(abDib);
    BMPWIN3XINFOHDR *pHdr = (BMPWIN3XINFOHDR *)abDib;
    pHdr->cbSize      = sizeof(*pHdr);
    pHdr->uWidth      = 1;
    pHdr->uHeight     = 1;
    pHdr->cPlanes     = 1;
    pHdr->cBits       = 32;
    pHdr->cbSizeImage = 4;
    abDib[sizeof(*pHdr)]     = 0x11;
    abDib[sizeof(*pHdr) + 1] = 0x22;
    abDib[sizeof(*pHdr) + 2] = 0x33;

    void  *pvBmp = NULL;
    size_t cbBmp = 0;
    RTTESTI_CHECK_RC_RETV(ShClHlpDibToBmp(abDib, sizeof(abDib), &pvBmp, &cbBmp), VINF_SUCCESS);
    CFDataRef hData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)pvBmp, (CFIndex)cbBmp);
    tstPutFlavor(hPasteboard, kUTTypeBMP, hData);
    CFRelease(hData);
    RTMemFree(pvBmp);

    uint32_t cb = 0;
    void *pv = tstRead(hPasteboard, VBOX_SHCL_FMT_BITMAP, &cb, NULL);
    RTTESTI_CHECK_RETV(pv);
    RTTESTI_CHECK(cb == sizeof(abDib));
    RTTESTI_CHECK(cb == sizeof(abDib) && memcmp(pv, abDib, sizeof(abDib)) == 0);
    RTMemFree(pv);
}


/**
 * Tests text with non-ASCII characters and several lines.
 */
static void tstText(PasteboardRef hPasteboard)
{
    RTTestSub(g_hTest, "Chinese multi-line text");

    static const char s_szText[] = "\xE4\xBD\xA0\xE5\xA5\xBD\xEF\xBC\x8C\xE4\xB8\x96\xE7\x95\x8C\nline 2\nline 3"; /* "Ni hao, shi jie" in Chinese. */
    CFDataRef hData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)s_szText, sizeof(s_szText) - 1);
    tstPutFlavor(hPasteboard, kUTTypeUTF8PlainText, hData);
    CFRelease(hData);

    uint32_t fFormats = 0;
    bool     fChanged = false;
    RTTESTI_CHECK_RC(queryNewPasteboardFormats(hPasteboard, 0, NULL, true /*fForce*/, &fFormats, &fChanged), VINF_SUCCESS);
    RTTESTI_CHECK_MSG(fFormats & VBOX_SHCL_FMT_UNICODETEXT, ("fFormats=%#x\n", fFormats));

    uint32_t cb = 0;
    PCRTUTF16 pwsz = (PCRTUTF16)tstRead(hPasteboard, VBOX_SHCL_FMT_UNICODETEXT, &cb, NULL);
    RTTESTI_CHECK_RETV(pwsz);

    /* Guests expect CRLF line endings. */
    char *pszGot = NULL;
    RTTESTI_CHECK_RC_RETV(RTUtf16ToUtf8Ex(pwsz, cb / sizeof(RTUTF16), &pszGot, 0, NULL), VINF_SUCCESS);
    static const char s_szExpect[] = "\xE4\xBD\xA0\xE5\xA5\xBD\xEF\xBC\x8C\xE4\xB8\x96\xE7\x95\x8C\r\nline 2\r\nline 3";
    RTTESTI_CHECK_MSG(strcmp(pszGot, s_szExpect) == 0, ("got: %s\n", pszGot));
    RTStrFree(pszGot);
    RTMemFree((void *)pwsz);
}


/**
 * Tests telling our own content from what another application put there afterwards.
 */
static void tstOwnership(PasteboardRef hPasteboard, CFStringRef hStrName)
{
    RTTestSub(g_hTest, "Pasteboard ownership");

    /* The backend uses random IDs, so the top bit is as likely set as not. */
    uint64_t const idOwnership = UINT64_C(0x87c33528096c2f0f);
    void          *hStrFlavor  = NULL;
    RTTESTI_CHECK_RC(clearPasteboard(hPasteboard, &hStrFlavor), VINF_SUCCESS);
    RTTESTI_CHECK(!isPasteboardOwner(hPasteboard, idOwnership, hStrFlavor));

    RTTESTI_CHECK_RC_RETV(takePasteboardOwnership(hPasteboard, idOwnership, "org.virtualbox.testcase.ownership", "0x1", &hStrFlavor),
                          VINF_SUCCESS);
    RTTESTI_CHECK(isPasteboardOwner(hPasteboard, idOwnership, hStrFlavor));

    /* Writing the data stays on the item carrying the ownership flavor. */
    static const RTUTF16 s_wszText[] = { 'o', 'u', 'r', 's', '\0' };
    RTTESTI_CHECK_RC(writeToPasteboard(hPasteboard, idOwnership, s_wszText, sizeof(s_wszText), VBOX_SHCL_FMT_UNICODETEXT),
                     VINF_SUCCESS);
    RTTESTI_CHECK(isPasteboardOwner(hPasteboard, idOwnership, hStrFlavor));

    /* Another application copies something, going through a handle of its own. */
    PasteboardRef hOther = NULL;
    RTTESTI_CHECK_RETV(PasteboardCreate(hStrName, &hOther) == noErr);
    static const char s_szOther[] = "theirs";
    CFDataRef hData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)s_szOther, sizeof(s_szOther) - 1);
    tstPutFlavor(hOther, kUTTypeUTF8PlainText, hData);
    CFRelease(hData);
    CFRelease(hOther);
    RTTESTI_CHECK(!isPasteboardOwner(hPasteboard, idOwnership, hStrFlavor));

    RTTESTI_CHECK_RC(clearPasteboard(hPasteboard, &hStrFlavor), VINF_SUCCESS);
}


#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
/** Verifies file URL conversion without accessing the user's pasteboard. */
static void tstFileURLs(PasteboardRef hPasteboard)
{
    RTTestSub(g_hTest, "File URL roots");
    static struct
    {
        const char *pszURL;
        const char *pszExpected;
    } const s_aCases[] =
    {
        { "file:///tmp/one%20file.txt", "file:///tmp/one%20file.txt\r\n" },
        { "file://localhost/tmp/plain.txt", "file:///tmp/plain.txt\r\n" },
        { "https://example.invalid/file.txt", NULL },
        { "file://remote.invalid/tmp/file.txt", NULL },
        { "file:///tmp/file.txt?query", NULL },
        { "file:///tmp/file.txt#fragment", NULL },
        { "file:///tmp/a%00b", NULL },
        { "file:relative.txt", NULL },
    };
    for (size_t i = 0; i < RT_ELEMENTS(s_aCases); i++)
    {
        CFDataRef hData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)s_aCases[i].pszURL,
                                      strlen(s_aCases[i].pszURL));
        RTTESTI_CHECK_RETV(hData != NULL);
        tstPutFlavor(hPasteboard, kUTTypeFileURL, hData);
        CFRelease(hData);

        char *pszRoots = NULL;
        size_t cbRoots = 0;
        int const vrc = readFileURLsFromPasteboard(hPasteboard, &pszRoots, &cbRoots);
        if (s_aCases[i].pszExpected)
        {
            RTTESTI_CHECK_RC(vrc, VINF_SUCCESS);
            RTTESTI_CHECK(pszRoots != NULL);
            if (pszRoots)
            {
                RTTESTI_CHECK_MSG(!strcmp(pszRoots, s_aCases[i].pszExpected),
                                 ("URL %s produced %s\n", s_aCases[i].pszURL, pszRoots));
                RTTESTI_CHECK(cbRoots == strlen(pszRoots) + 1);
            }
        }
        else
        {
            RTTESTI_CHECK(RT_FAILURE(vrc));
            RTTESTI_CHECK(pszRoots == NULL);
            RTTESTI_CHECK(cbRoots == 0);
        }
        RTStrFree(pszRoots);
    }

    /* The complete offer must fail if any included file URL is invalid. */
    RTTESTI_CHECK(PasteboardClear(hPasteboard) == noErr);
    PasteboardSynchronize(hPasteboard);
    for (uintptr_t iItem = 1; iItem <= 2; iItem++)
    {
        const char *pszURL = iItem == 1 ? "file:///tmp/valid.txt" : "file://remote.invalid/tmp/invalid.txt";
        CFDataRef hData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)pszURL, strlen(pszURL));
        RTTESTI_CHECK_RETV(hData != NULL);
        RTTESTI_CHECK(PasteboardPutItemFlavor(hPasteboard, (PasteboardItemID)iItem, kUTTypeFileURL,
                                            hData, kPasteboardFlavorNoFlags) == noErr);
        CFRelease(hData);
    }
    char *pszRoots = NULL;
    size_t cbRoots = 0;
    RTTESTI_CHECK(RT_FAILURE(readFileURLsFromPasteboard(hPasteboard, &pszRoots, &cbRoots)));
    RTTESTI_CHECK(pszRoots == NULL);
    RTTESTI_CHECK(cbRoots == 0);
    RTStrFree(pszRoots);
}
#endif


#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
/** Publishing a delayed file offer must not replace newer host content. */
static void tstFilePublication(PasteboardRef hPasteboard)
{
    RTTestSub(g_hTest, "File publication and superseded ownership");
    char szPath[RTPATH_MAX];
    RTTESTI_CHECK_RC_RETV(RTPathTemp(szPath, sizeof(szPath)), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTPathAppend(szPath, sizeof(szPath), "tstFilePublish-XXXXXX"), VINF_SUCCESS);
    RTTESTI_CHECK_RC_RETV(RTFileCreateTemp(szPath, 0600), VINF_SUCCESS);
    char *pszRoots = NULL;
    RTStrAPrintf(&pszRoots, "%s\r\n", szPath);
    RTTESTI_CHECK_RETV(pszRoots != NULL);
    void *hFlavor = NULL;
    uint64_t const idOwner = 81273;
    RTTESTI_CHECK_RC(takePasteboardOwnership(hPasteboard, idOwner, "org.virtualbox.tests.file-owner", "files", &hFlavor),
                      VINF_SUCCESS);
    RTTESTI_CHECK_RC(writeFileURLsToPasteboard(hPasteboard, idOwner, hFlavor, pszRoots, strlen(pszRoots) + 1),
                      VINF_SUCCESS);
    char *pszRead = NULL;
    size_t cbRead = 0;
    RTTESTI_CHECK_RC(readFileURLsFromPasteboard(hPasteboard, &pszRead, &cbRead), VINF_SUCCESS);
    RTTESTI_CHECK(pszRead != NULL && strstr(pszRead, RTPathFilename(szPath)) != NULL);
    RTTESTI_CHECK(isPasteboardOwner(hPasteboard, idOwner, hFlavor));
    RTStrFree(pszRead);
    CFDataRef hText = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)"newer", 5);
    tstPutFlavor(hPasteboard, kUTTypeUTF8PlainText, hText);
    CFRelease(hText);
    RTTESTI_CHECK_RC(writeFileURLsToPasteboard(hPasteboard, idOwner, hFlavor, pszRoots, strlen(pszRoots) + 1),
                      VERR_CANCELLED);
    CFDataRef hRead = NULL;
    RTTESTI_CHECK(PasteboardCopyItemFlavorData(hPasteboard, (PasteboardItemID)1, kUTTypeUTF8PlainText, &hRead) == noErr);
    if (hRead)
    {
        RTTESTI_CHECK(CFDataGetLength(hRead) == 5 && memcmp(CFDataGetBytePtr(hRead), "newer", 5) == 0);
        CFRelease(hRead);
    }
    CFRelease((CFStringRef)hFlavor);
    RTStrFree(pszRoots);
    RTFileDelete(szPath);
}
#endif

int main()
{
    RTEXITCODE rcExit = RTTestInitAndCreate("tstClipboardPasteboard", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(g_hTest);

    PasteboardRef hPasteboard = NULL;
    CFStringRef const hStrName = CFSTR("org.virtualbox.testcase.tstClipboardPasteboard");
    OSStatus orc = PasteboardCreate(hStrName, &hPasteboard);
    if (orc == noErr)
    {
        tstImageFlavor(hPasteboard, "PNG", kUTTypePNG);
        tstImageFlavor(hPasteboard, "TIFF", kUTTypeTIFF);
        tstDamagedImage(hPasteboard);
        tstBmpFlavor(hPasteboard);
        tstText(hPasteboard);
        tstOwnership(hPasteboard, hStrName);
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        tstFileURLs(hPasteboard);
        tstFilePublication(hPasteboard);
#endif

        PasteboardClear(hPasteboard);
        CFRelease(hPasteboard);
    }
    else
        RTTestSkipped(g_hTest, "PasteboardCreate failed: %d", (int)orc);

    return RTTestSummaryAndDestroy(g_hTest);
}

RT_GCC_NO_WARN_DEPRECATED_END
