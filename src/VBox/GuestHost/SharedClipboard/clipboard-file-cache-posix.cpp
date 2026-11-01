/* $Id$ */
/** @file
 * Shared Clipboard - receive files into a private POSIX snapshot.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#include <VBox/GuestHost/ClipboardFileCache.h>

#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

/** @page pg_main_clipboard_file_cache Clipboard file snapshots
 * A snapshot is private until every advertised file has been received. Names
 * are single path components; descriptor-relative, no-follow operations keep
 * guest-controlled names below the newly created directory. Published snapshots
 * remain available after the VM stops. Unpublished snapshots are removed.
 */

/** Maximum entries in one offer, including directories. */
#define SHCL_FILE_CACHE_MAX_ENTRIES  UINT32_C(16384)
/** Maximum nesting of a guest directory tree. */
#define SHCL_FILE_CACHE_MAX_DEPTH    UINT32_C(64)
/** Aggregate cache quota, including a per-entry metadata allowance. */
#define SHCL_FILE_CACHE_QUOTA        (UINT64_C(8) * _1G)
/** Leave space for the host even when another writer is active. */
#define SHCL_FILE_CACHE_RESERVE      (UINT64_C(256) * _1M)

/** Per-snapshot accounting and cancellation. */
struct SHCLFILECACHECOPY
{
    PSHCLTRANSFER         pTransfer;
    PFNSHCLFILECACHECANCEL pfnCanceled;
    void                  *pvUser;
    uint64_t               cbLeft;
    uint64_t               cbQuotaLeft;
    uint32_t               cEntries;
};

/** Checks a name before using it in a descriptor-relative operation. */
static bool shClFileCacheNameIsValid(const char *pszName, size_t cbName)
{
    if (!pszName || !cbName)
        return false;
    size_t const cch = RTStrNLen(pszName, cbName);
    if (!cch || cch == cbName || cch > 255 || !strcmp(pszName, ".") || !strcmp(pszName, ".."))
        return false;
    if (RT_FAILURE(RTStrValidateEncodingEx(pszName, cch, RTSTR_VALIDATE_ENCODING_EXACT_LENGTH)))
        return false;
    for (size_t i = 0; i < cch; ++i)
        if ((uint8_t)pszName[i] < 32 || pszName[i] == 127 || strchr("/\\:", pszName[i]))
            return false;
    return true;
}

/** Removes only children of an already opened, private snapshot directory. */
static int shClFileCacheRemoveChildren(int fdDirectory)
{
    DIR *pDir = fdopendir(dup(fdDirectory));
    if (!pDir)
        return RTErrConvertFromErrno(errno);
    int vrc = VINF_SUCCESS;
    struct dirent *pEntry;
    while ((pEntry = readdir(pDir)) != NULL)
    {
        if (!strcmp(pEntry->d_name, ".") || !strcmp(pEntry->d_name, ".."))
            continue;
        struct stat Info;
        if (fstatat(fdDirectory, pEntry->d_name, &Info, AT_SYMLINK_NOFOLLOW) != 0)
        {
            vrc = RTErrConvertFromErrno(errno);
            break;
        }
        if (S_ISDIR(Info.st_mode))
        {
            int fdChild = openat(fdDirectory, pEntry->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (fdChild < 0)
            {
                vrc = RTErrConvertFromErrno(errno);
                break;
            }
            vrc = shClFileCacheRemoveChildren(fdChild);
            close(fdChild);
            if (RT_FAILURE(vrc))
                break;
            if (unlinkat(fdDirectory, pEntry->d_name, AT_REMOVEDIR) != 0)
                vrc = RTErrConvertFromErrno(errno);
        }
        else if (unlinkat(fdDirectory, pEntry->d_name, 0) != 0)
            vrc = RTErrConvertFromErrno(errno);
        if (RT_FAILURE(vrc))
            break;
    }
    closedir(pDir);
    return vrc;
}

DECLHIDDEN(int) shClFileCacheDiscard(const char *pszDirectory)
{
    AssertPtrReturn(pszDirectory, VERR_INVALID_POINTER);
    const char *pszName = RTPathFilename(pszDirectory);
    AssertReturn(pszName && !strncmp(pszName, "copy-", 5), VERR_INVALID_NAME);
    int fd = open(pszDirectory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return RTErrConvertFromErrno(errno);
    struct stat Info;
    int vrc = fstat(fd, &Info) == 0 ? VINF_SUCCESS : RTErrConvertFromErrno(errno);
    if (RT_SUCCESS(vrc) && (Info.st_uid != geteuid() || (Info.st_mode & 077)))
        vrc = VERR_ACCESS_DENIED;
    if (RT_SUCCESS(vrc))
        vrc = shClFileCacheRemoveChildren(fd);
    close(fd);
    if (RT_SUCCESS(vrc) && rmdir(pszDirectory) != 0)
        vrc = RTErrConvertFromErrno(errno);
    return vrc;
}

/** Counts existing cache data without following links. */
static int shClFileCacheUsage(int fdDirectory, uint32_t cDepth, uint64_t *pcbUsed, uint32_t *pcEntries)
{
    if (cDepth > SHCL_FILE_CACHE_MAX_DEPTH || *pcEntries > SHCL_FILE_CACHE_MAX_ENTRIES * 8)
        return VERR_TOO_MUCH_DATA;
    DIR *pDir = fdopendir(dup(fdDirectory));
    if (!pDir)
        return RTErrConvertFromErrno(errno);
    int vrc = VINF_SUCCESS;
    struct dirent *pEntry;
    while ((pEntry = readdir(pDir)) != NULL)
    {
        if (!strcmp(pEntry->d_name, ".") || !strcmp(pEntry->d_name, ".."))
            continue;
        struct stat Info;
        if (fstatat(fdDirectory, pEntry->d_name, &Info, AT_SYMLINK_NOFOLLOW) != 0)
        {
            vrc = RTErrConvertFromErrno(errno);
            break;
        }
        if (++*pcEntries > SHCL_FILE_CACHE_MAX_ENTRIES * 8)
        {
            vrc = VERR_TOO_MUCH_DATA;
            break;
        }
        uint64_t const cb = S_ISREG(Info.st_mode) && Info.st_size > 0 ? (uint64_t)Info.st_size : 0;
        if (   *pcbUsed > SHCL_FILE_CACHE_QUOTA - 4096
            || cb > SHCL_FILE_CACHE_QUOTA - 4096 - *pcbUsed)
        {
            vrc = VERR_DISK_FULL;
            break;
        }
        *pcbUsed += cb + 4096;
        if (S_ISDIR(Info.st_mode))
        {
            int fdChild = openat(fdDirectory, pEntry->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (fdChild < 0)
            {
                vrc = RTErrConvertFromErrno(errno);
                break;
            }
            vrc = shClFileCacheUsage(fdChild, cDepth + 1, pcbUsed, pcEntries);
            close(fdChild);
        }
        if (RT_FAILURE(vrc))
            break;
    }
    closedir(pDir);
    return vrc;
}

/** Receives one validated file or directory. */
static int shClFileCacheCopy(SHCLFILECACHECOPY *pCopy, const SHCLLISTENTRY *pEntry,
                            const char *pszRemote, int fdParent, uint32_t cDepth)
{
    if (pCopy->pfnCanceled && pCopy->pfnCanceled(pCopy->pvUser))
        return VERR_CANCELLED;
    if (cDepth >= SHCL_FILE_CACHE_MAX_DEPTH || ++pCopy->cEntries > SHCL_FILE_CACHE_MAX_ENTRIES)
        return VERR_TOO_MUCH_DATA;
    if (pCopy->cbQuotaLeft < 4096)
        return VERR_DISK_FULL;
    pCopy->cbQuotaLeft -= 4096;
    if (   !ShClTransferListEntryIsValid((PSHCLLISTENTRY)pEntry)
        || !shClFileCacheNameIsValid(pEntry->pszName, pEntry->cbName)
        || pEntry->fInfo != VBOX_SHCL_INFO_F_FSOBJINFO || pEntry->cbInfo != sizeof(SHCLFSOBJINFO))
        return VERR_INVALID_PARAMETER;
    SHCLFSOBJINFO const Info = *(PCSHCLFSOBJINFO)pEntry->pvInfo;
    const char *pszName = pEntry->pszName;
    if (RTFS_IS_DIRECTORY(Info.Attr.fMode))
    {
        if (mkdirat(fdParent, pszName, 0700) != 0)
            return RTErrConvertFromErrno(errno);
        int fdChild = openat(fdParent, pszName, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (fdChild < 0)
            return RTErrConvertFromErrno(errno);
        SHCLLISTOPENPARMS Parms;
        int vrc = ShClTransferListOpenParmsInit(&Parms);
        SHCLLISTHANDLE hList = NIL_SHCLLISTHANDLE;
        if (RT_SUCCESS(vrc))
        {
            vrc = RTStrCopy(Parms.pszPath, Parms.cbPath, pszRemote);
            if (RT_SUCCESS(vrc))
                vrc = ShClTransferListOpen(pCopy->pTransfer, &Parms, &hList);
            ShClTransferListOpenParmsDestroy(&Parms);
        }
        SHCLLISTHDR Header;
        RT_ZERO(Header);
        if (RT_SUCCESS(vrc))
            vrc = ShClTransferListGetHeader(pCopy->pTransfer, hList, &Header);
        if (RT_SUCCESS(vrc) && (!ShClTransferListHdrIsValid(&Header)
                               || Header.cEntries > SHCL_FILE_CACHE_MAX_ENTRIES - pCopy->cEntries))
            vrc = VERR_TOO_MUCH_DATA;
        /* The transfer protocol supplies an entry count. Reading past it is
         * not an EOF probe: some guests treat that request as a transfer error. */
        for (uint64_t i = 0; RT_SUCCESS(vrc) && i < Header.cEntries; ++i)
        {
            SHCLLISTENTRY Child;
            vrc = ShClTransferListEntryInit(&Child);
            if (RT_FAILURE(vrc))
                break;
            vrc = ShClTransferListRead(pCopy->pTransfer, hList, &Child);
            if (RT_SUCCESS(vrc))
            {
                char *pszPath = NULL;
                if (!shClFileCacheNameIsValid(Child.pszName, Child.cbName))
                    vrc = VERR_INVALID_NAME;
                else if (RTStrAPrintf(&pszPath, "%s/%s", pszRemote, Child.pszName) < 0)
                    vrc = VERR_NO_MEMORY;
                else
                    vrc = shClFileCacheCopy(pCopy, &Child, pszPath, fdChild, cDepth + 1);
                RTStrFree(pszPath);
            }
            ShClTransferListEntryDestroy(&Child);
        }
        if (hList != NIL_SHCLLISTHANDLE)
        {
            int const vrcClose = ShClTransferListClose(pCopy->pTransfer, hList);
            if (RT_SUCCESS(vrc))
                vrc = vrcClose;
        }
        close(fdChild);
        return vrc;
    }
    if (   !RTFS_IS_FILE(Info.Attr.fMode) || Info.cbObject < 0
        || (uint64_t)Info.cbObject > pCopy->cbLeft || (uint64_t)Info.cbObject > pCopy->cbQuotaLeft)
        return VERR_TOO_MUCH_DATA;

    SHCLOBJOPENCREATEPARMS Parms;
    int vrc = ShClTransferObjOpenParmsInit(&Parms);
    SHCLOBJHANDLE hObj = NIL_SHCLOBJHANDLE;
    if (RT_SUCCESS(vrc))
    {
        Parms.fCreate = SHCL_OBJ_CF_ACCESS_READ | SHCL_OBJ_CF_ACCESS_DENYWRITE;
        vrc = RTStrCopy(Parms.pszPath, Parms.cbPath, pszRemote);
        if (RT_SUCCESS(vrc))
            vrc = ShClTransferObjOpen(pCopy->pTransfer, &Parms, &hObj);
        ShClTransferObjOpenParmsDestroy(&Parms);
    }
    int fd = -1;
    if (RT_SUCCESS(vrc))
    {
        fd = openat(fdParent, pszName, O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0)
            vrc = RTErrConvertFromErrno(errno);
    }
    uint64_t cbLeft = (uint64_t)Info.cbObject;
    uint8_t abBuffer[_64K];
    while (RT_SUCCESS(vrc) && cbLeft)
    {
        if (pCopy->pfnCanceled && pCopy->pfnCanceled(pCopy->pvUser))
        {
            vrc = VERR_CANCELLED;
            break;
        }
        struct statvfs FsInfo;
        if (fstatvfs(fd, &FsInfo) != 0)
            vrc = RTErrConvertFromErrno(errno);
        else if ((uint64_t)FsInfo.f_bavail * FsInfo.f_frsize < SHCL_FILE_CACHE_RESERVE + sizeof(abBuffer))
            vrc = VERR_DISK_FULL;
        if (RT_FAILURE(vrc))
            break;
        uint32_t cbRead = 0;
        uint32_t const cbRequested = (uint32_t)RT_MIN(cbLeft, sizeof(abBuffer));
        vrc = ShClTransferObjRead(pCopy->pTransfer, hObj, abBuffer, cbRequested, 0, &cbRead);
        if (RT_SUCCESS(vrc) && (!cbRead || cbRead > cbRequested))
            vrc = VERR_EOF;
        size_t off = 0;
        while (RT_SUCCESS(vrc) && off < cbRead)
        {
            ssize_t const cbWritten = write(fd, abBuffer + off, cbRead - off);
            if (cbWritten < 0 && errno == EINTR)
                continue;
            if (cbWritten <= 0)
                vrc = cbWritten < 0 ? RTErrConvertFromErrno(errno) : VERR_WRITE_ERROR;
            else
                off += (size_t)cbWritten;
        }
        if (RT_SUCCESS(vrc))
        {
            cbLeft -= cbRead;
            pCopy->cbLeft -= cbRead;
            pCopy->cbQuotaLeft -= cbRead;
        }
    }
    if (fd >= 0)
    {
        if (RT_SUCCESS(vrc) && fsync(fd) != 0)
            vrc = RTErrConvertFromErrno(errno);
        if (close(fd) != 0 && RT_SUCCESS(vrc))
            vrc = RTErrConvertFromErrno(errno);
    }
    if (hObj != NIL_SHCLOBJHANDLE)
    {
        int const vrcClose = ShClTransferObjClose(pCopy->pTransfer, hObj);
        if (RT_SUCCESS(vrc))
            vrc = vrcClose;
    }
    return vrc;
}

DECLHIDDEN(int) shClFileCacheReceive(PSHCLTRANSFER pTransfer, const char *pszCacheRoot, uint64_t cbLimit,
                                    PFNSHCLFILECACHECANCEL pfnCanceled, void *pvUser,
                                    char **ppszDirectory, char **ppszRoots, size_t *pcbRoots)
{
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);
    AssertPtrReturn(pszCacheRoot, VERR_INVALID_POINTER);
    AssertPtrReturn(ppszDirectory, VERR_INVALID_POINTER);
    AssertPtrReturn(ppszRoots, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbRoots, VERR_INVALID_POINTER);
    *ppszDirectory = NULL;
    *ppszRoots = NULL;
    *pcbRoots = 0;
    uint64_t const cRoots = ShClTransferRootsCount(pTransfer);
    if (!cRoots || cRoots > SHCL_FILE_CACHE_MAX_ENTRIES)
        return VERR_TOO_MUCH_DATA;
    if (mkdir(pszCacheRoot, 0700) != 0 && errno != EEXIST)
        return RTErrConvertFromErrno(errno);
    int fdRoot = open(pszCacheRoot, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fdRoot < 0)
        return RTErrConvertFromErrno(errno);
    struct stat RootInfo;
    int vrc = fstat(fdRoot, &RootInfo) == 0 ? VINF_SUCCESS : RTErrConvertFromErrno(errno);
    if (RT_SUCCESS(vrc) && (RootInfo.st_uid != geteuid() || (RootInfo.st_mode & 077)))
        vrc = VERR_ACCESS_DENIED;
    int fdLock = -1;
    if (RT_SUCCESS(vrc))
    {
        fdLock = openat(fdRoot, ".lock", O_CREAT | O_RDWR | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fdLock < 0)
            vrc = RTErrConvertFromErrno(errno);
        else
        {
            struct stat LockInfo;
            if (fstat(fdLock, &LockInfo) != 0)
                vrc = RTErrConvertFromErrno(errno);
            else if (!S_ISREG(LockInfo.st_mode) || LockInfo.st_uid != geteuid() || (LockInfo.st_mode & 077))
                vrc = VERR_ACCESS_DENIED;
        }
    }
    while (RT_SUCCESS(vrc) && flock(fdLock, LOCK_EX | LOCK_NB) != 0)
    {
        if (errno != EWOULDBLOCK && errno != EINTR)
            vrc = RTErrConvertFromErrno(errno);
        else if (pfnCanceled && pfnCanceled(pvUser))
            vrc = VERR_CANCELLED;
        else
            RTThreadSleep(20);
    }
    uint64_t cbUsed = 0;
    uint32_t cExisting = 0;
    if (RT_SUCCESS(vrc))
        vrc = shClFileCacheUsage(fdRoot, 0, &cbUsed, &cExisting);
    if (RT_SUCCESS(vrc) && cbUsed >= SHCL_FILE_CACHE_QUOTA)
        vrc = VERR_DISK_FULL;

    char szName[64];
    char szDirectory[RTPATH_MAX];
    int fdCopy = -1;
    bool fCreated = false;
    if (RT_SUCCESS(vrc))
    {
        for (uint32_t i = 0; i < 32; ++i)
        {
            RTStrPrintf(szName, sizeof(szName), "copy-%016RX64-%016RX64", RTRandU64(), RTRandU64());
            if (mkdirat(fdRoot, szName, 0700) == 0)
            {
                vrc = VINF_SUCCESS;
                fCreated = true;
                break;
            }
            vrc = RTErrConvertFromErrno(errno);
            if (errno != EEXIST || i == 31)
                break;
        }
        if (RT_SUCCESS(vrc))
        {
            vrc = RTPathJoin(szDirectory, sizeof(szDirectory), pszCacheRoot, szName);
            fdCopy = openat(fdRoot, szName, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (fdCopy < 0)
                vrc = RTErrConvertFromErrno(errno);
        }
    }
    SHCLFILECACHECOPY Copy = { pTransfer, pfnCanceled, pvUser, cbLimit,
                              cbUsed < SHCL_FILE_CACHE_QUOTA ? SHCL_FILE_CACHE_QUOTA - cbUsed : 0, 0 };
    char *pszRoots = NULL;
    for (uint64_t i = 0; RT_SUCCESS(vrc) && i < cRoots; ++i)
    {
        PCSHCLLISTENTRY pEntry = ShClTransferRootsEntryGet(pTransfer, i);
        if (!pEntry)
            vrc = VERR_INVALID_PARAMETER;
        else
            vrc = shClFileCacheCopy(&Copy, pEntry, pEntry->pszName, fdCopy, 0);
        if (RT_SUCCESS(vrc))
        {
            char *pszNew = NULL;
            if (RTStrAPrintf(&pszNew, "%s%s/%s\r\n", pszRoots ? pszRoots : "", szDirectory, pEntry->pszName) < 0)
                vrc = VERR_NO_MEMORY;
            RTStrFree(pszRoots);
            pszRoots = pszNew;
        }
    }
    if (RT_SUCCESS(vrc))
    {
        *ppszDirectory = RTStrDup(szDirectory);
        if (!*ppszDirectory)
            vrc = VERR_NO_MEMORY;
        else
        {
            *ppszRoots = pszRoots;
            *pcbRoots = strlen(pszRoots) + 1;
            pszRoots = NULL;
        }
    }
    RTStrFree(pszRoots);
    if (fdCopy >= 0)
    {
        if (RT_FAILURE(vrc))
        {
            shClFileCacheRemoveChildren(fdCopy);
            unlinkat(fdRoot, szName, AT_REMOVEDIR);
        }
        close(fdCopy);
    }
    else if (fCreated)
        unlinkat(fdRoot, szName, AT_REMOVEDIR);
    if (fdLock >= 0)
        close(fdLock);
    close(fdRoot);
    return vrc;
}
