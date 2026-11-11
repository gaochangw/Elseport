/* $Id$ */
/** @file
 * Elseport - completion acknowledgement and owned installation media cleanup.
 */
/* SPDX-License-Identifier: GPL-3.0-only */
#include "UIElseportIntegration.h"
#include "UIGlobalSession.h"
#include "CMachine.h"
#include "CMedium.h"
#include "CMediumAttachment.h"
#include "CSession.h"
#include "CVirtualBox.h"
#include "CVirtualBoxClient.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

/** @page pg_elseport_integration Completed installation cleanup
 * The guest's first-boot service acknowledges its own machine ID. Only then
 * may the frontend eject auxiliary media and remove matching files in that
 * machine's directory. User-selected ISOs and unrelated files are untouched.
 */
bool uiElseportCleanupInstallation(CMachine &machine)
{
    if (machine.isNull() || machine.GetExtraData("Elseport/InstallPending") != "true")
        return true;
    if (machine.GetGuestPropertyValue("/Elseport/InstallComplete") != machine.GetId().toString(QUuid::WithoutBraces))
        return false;
    const KMachineState state = machine.GetState();
    if (state != KMachineState_Running && state != KMachineState_Paused && state != KMachineState_PoweredOff)
        return false;
    CSession session = gpGlobalSession->virtualBoxClient().GetSession();
    machine.LockMachine(session, state == KMachineState_PoweredOff ? KLockType_Write : KLockType_Shared);
    if (!machine.isOk())
        return false;
    CMachine live = session.GetMachine();
    const QString root = QFileInfo(live.GetSettingsFilePath()).absolutePath();
    const QString prefix = "Unattended-" + live.GetId().toString(QUuid::WithoutBraces) + "-";
    bool clean = true;
    for (CMediumAttachment attachment : live.GetMediumAttachments())
    {
        CMedium medium = attachment.GetMedium();
        if (attachment.GetType() == KDeviceType_DVD && !medium.isNull())
        {
            const QFileInfo info(medium.GetLocation());
            if (info.fileName().startsWith(prefix) && info.absolutePath() == root)
            {
                live.MountMedium(attachment.GetController(), attachment.GetPort(), attachment.GetDevice(), CMedium(), true);
                clean = live.isOk() && clean;
            }
        }
    }
    /* Commit the ejected drive before closing its medium: the settings backup
     * can otherwise retain a read lock on the old installation image. */
    if (clean)
    {
        live.SaveSettings();
        clean = live.isOk();
    }
    if (clean)
        for (CMedium medium : gpGlobalSession->virtualBox().GetDVDImages())
        {
            const QFileInfo info(medium.GetLocation());
            if (info.fileName().startsWith(prefix) && info.absolutePath() == root && medium.GetMachineIds().isEmpty())
            {
                medium.Close();
                clean = medium.isOk() && clean;
            }
        }
    if (clean)
    {
        const QDir dir(root);
        for (const QString &file : dir.entryList(QStringList() << prefix + "*", QDir::Files | QDir::NoSymLinks))
        {
            const QFileInfo info(dir.filePath(file));
            /* The generator reserves this exact UUID prefix, including the
             * extensionless Ubuntu user-data file which contains credentials. */
            if (info.isFile() && !info.isSymLink())
                clean = QFile::remove(info.absoluteFilePath()) && clean;
        }
        if (clean)
        {
            live.SetExtraData("Elseport/InstallPending", QString());
            live.SetExtraData("Elseport/InstallMedia", QString());
            live.SaveSettings();
            clean = live.isOk();
        }
    }
    session.UnlockMachine();
    return clean;
}
