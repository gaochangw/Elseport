/* $Id$ */
/** @file
 * Elseport - isolated macOS development bundle launcher.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef ELSEPORT_PROFILE_NAME
# define ELSEPORT_PROFILE_NAME "Elseport-Development"
#endif
#ifndef ELSEPORT_IPC_NAME
# define ELSEPORT_IPC_NAME "elseport-development"
#endif

static int elseportSetDefault(const char *pszName, const char *pszValue)
{
    const char *pszCurrent = getenv(pszName);
    if (pszCurrent && *pszCurrent)
        return 0;
    return setenv(pszName, pszValue, 1);
}

int main(int argc, char **argv)
{
    char szPath[PATH_MAX];
    char szResolved[PATH_MAX];
    char szValue[PATH_MAX];
    uint32_t cbPath = sizeof(szPath);
    (void)argc;
    if (_NSGetExecutablePath(szPath, &cbPath) || !realpath(szPath, szResolved))
        return 1;
    char *pszSlash = strrchr(szResolved, '/');
    if (!pszSlash)
        return 1;
    *pszSlash = '\0';
    if (setenv("VBOX_APP_HOME", szResolved, 1))
        return 1;
    int cch = snprintf(szValue, sizeof(szValue), "%s/../PlugIns", szResolved);
    if (cch < 0 || (size_t)cch >= sizeof(szValue) || setenv("QT_PLUGIN_PATH", szValue, 1))
        return 1;
    const char *pszHome = getenv("HOME");
    if (!pszHome || *pszHome != '/')
        return 1;
    cch = snprintf(szValue, sizeof(szValue), "%s/Library/Application Support/" ELSEPORT_PROFILE_NAME, pszHome);
    if (   cch < 0 || (size_t)cch >= sizeof(szValue)
        || elseportSetDefault("VBOX_USER_HOME", szValue)
        || elseportSetDefault("VBOX_IPC_SOCKETID", ELSEPORT_IPC_NAME))
        return 1;
    cch = snprintf(szPath, sizeof(szPath), "%s/VirtualBox", szResolved);
    if (cch < 0 || (size_t)cch >= sizeof(szPath))
        return 1;
    argv[0] = szPath;
    execv(szPath, argv);
    perror("Elseport development launcher");
    return 1;
}
