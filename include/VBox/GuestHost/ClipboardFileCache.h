/* $Id$ */
/** @file
 * Shared Clipboard - bounded POSIX file snapshots.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef VBOX_INCLUDED_GuestHost_ClipboardFileCache_h
#define VBOX_INCLUDED_GuestHost_ClipboardFileCache_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/GuestHost/SharedClipboard-transfers.h>
#include <iprt/cdefs.h>

/** Cancellation predicate. It must not perform guest I/O. */
typedef DECLCALLBACKTYPE(bool, FNSHCLFILECACHECANCEL, (void *pvUser));
typedef FNSHCLFILECACHECANCEL *PFNSHCLFILECACHECANCEL;

/** Creates a complete private snapshot. No partially received paths are returned.
 * @returns VBox status code.
 * @param pTransfer         Initialized transfer whose root list has been read.
 * @param pszCacheRoot      Trusted user-owned cache directory, created if absent.
 * @param cbLimit           Maximum bytes in this snapshot.
 * @param pfnCanceled       Optional cancellation predicate.
 * @param pvUser            Argument for the predicate.
 * @param ppszDirectory     Allocated snapshot path. Free with RTStrFree.
 * @param ppszRoots         Allocated CRLF-separated absolute paths, including NUL.
 * @param pcbRoots          Size of the root list, including NUL.
 */
DECLHIDDEN(int) shClFileCacheReceive(PSHCLTRANSFER pTransfer, const char *pszCacheRoot, uint64_t cbLimit,
                                    PFNSHCLFILECACHECANCEL pfnCanceled, void *pvUser,
                                    char **ppszDirectory, char **ppszRoots, size_t *pcbRoots);

/** Removes an unpublished snapshot created by shClFileCacheReceive.
 * @returns VBox status code.
 * @param pszDirectory Private snapshot directory returned by the receiver.
 */
DECLHIDDEN(int) shClFileCacheDiscard(const char *pszDirectory);

#endif /* !VBOX_INCLUDED_GuestHost_ClipboardFileCache_h */
