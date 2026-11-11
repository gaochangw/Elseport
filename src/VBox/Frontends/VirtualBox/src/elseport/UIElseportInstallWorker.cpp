/* $Id$ */
/** @file
 * Elseport - cancellable Linux setup through the existing Main API.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

/** @page pg_elseport_install Native Linux setup
 * Images are read from the official HTTPS endpoint and verified against its
 * checksum list. Only app-owned partial downloads may be replaced. Machine
 * creation uses a separate COM session and does not run on the GUI thread.
 */
#include "UIElseportInstallWorker.h"
#include "UIGlobalSession.h"
#include "CConsole.h"
#include "CClipboardSettings.h"
#include "CGraphicsAdapter.h"
#include "CMachine.h"
#include "CMedium.h"
#include "CProgress.h"
#include "CSession.h"
#include "CStorageController.h"
#include "CUnattended.h"
#include "CVirtualBox.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QTimeZone>
#include <VBox/com/VirtualBox.h>
#include <iprt/err.h>
#include <VBox/log.h>
#include <iprt/param.h>
#include <iprt/string.h>

#define ELSEPORT_UBUNTU_URL "https://cdimage.ubuntu.com/releases/26.04/release/"
#define QUICKINSTALL_DOWNLOAD_ATTEMPTS 3

UIElseportInstallWorker::UIElseportInstallWorker(const UIElseportInstallOptions &options, QObject *pParent)
    : QThread(pParent), m_options(options), m_strImagePath(options.strImage), m_fCancelled(0), m_hHttp(NIL_RTHTTP),
      m_cbResumedFrom(0), m_cbStored(0), m_msLastReport(0), m_cbLastReport(0), m_cbPerSecond(0), m_uHttpStatus(0),
      m_fWriteFailed(false), m_fTextRequest(false)
{
}

QString UIElseportInstallWorker::text(const char *pszEnglish, const char *pszChinese) const
{
    return QString::fromUtf8(m_options.fChinese ? pszChinese : pszEnglish);
}

void UIElseportInstallWorker::cancel()
{
    m_fCancelled.storeRelaxed(1);
    QMutexLocker locker(&m_mutexHttp);
    if (m_hHttp != NIL_RTHTTP)
        RTHttpAbort(m_hHttp);
}

void UIElseportInstallWorker::run()
{
    if (!QDir().mkpath(m_options.strCache))
    {
        m_strError = text("Could not create the download folder.", "无法创建下载目录。");
        return;
    }
    if (m_strImagePath.isEmpty() && !getUbuntuImage())
        return;
    if (!QFileInfo(m_strImagePath).isFile() || isCancelled())
        return;
    if (FAILED(COMBase::InitializeCOM(false)))
    {
        m_strError = text("Could not connect to the virtual machine service.", "无法连接虚拟电脑服务。");
        return;
    }
    createMachine();
    m_options.strPassword.fill(QChar(0));
    m_options.strPassword.clear();
    COMBase::CleanupCOM();
}

bool UIElseportInstallWorker::getUbuntuImage()
{
    emit sigStep(0);
    QString strList;
    if (RT_FAILURE(fetchText(ELSEPORT_UBUNTU_URL "SHA256SUMS", strList)))
    {
        if (!isCancelled())
            m_strError =
                text("Could not reach ubuntu.com. Check your connection and retry.", "无法连接 ubuntu.com，请检查网络后重试。");
        return false;
    }
    const QRegularExpression re("^([0-9a-fA-F]{64}) [ *](ubuntu-26\\.04[.0-9]*-desktop-arm64\\.iso)\\s*$",
                                QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = re.globalMatch(strList);
    QString strName, strHash;
    while (matches.hasNext())
    {
        const QRegularExpressionMatch match = matches.next();
        strHash = match.captured(1).toLower();
        strName = match.captured(2);
    }
    if (strName.isEmpty())
    {
        m_strError = text("No compatible Ubuntu Arm image is listed.", "未找到适用的 Ubuntu Arm 安装镜像。");
        return false;
    }
    const QString strTarget = QDir(m_options.strCache).filePath(strName);
    if (QFileInfo::exists(strTarget))
    {
        emit sigStep(1);
        QString strActual;
        if (!calculateSha256(strTarget, strActual))
            return false;
        if (strActual == strHash)
        {
            m_strImagePath = strTarget;
            return true;
        }
        /* This exact path belongs to the app's cache, never to a selected ISO. */
        if (!QFile::remove(strTarget))
        {
            m_strError = text("Could not replace the damaged cached image.", "无法替换损坏的缓存镜像。");
            return false;
        }
    }
    emit sigStep(0);
    if (!download(ELSEPORT_UBUNTU_URL + strName, strTarget, strHash))
        return false;
    m_strImagePath = strTarget;
    return true;
}

bool UIElseportInstallWorker::createMachine()
{
    emit sigStep(2);
    /* The process owns one VirtualBoxClient. Use its existing backend and
     * create an independent lock session for this worker. */
    CVirtualBox vbox = gpGlobalSession->virtualBox();
    if (vbox.isNull())
    {
        m_strError = text("The virtual machine service is not available.", "虚拟电脑服务暂不可用。");
        return false;
    }
    CUnattended unattended = vbox.CreateUnattendedInstaller();
    if (unattended.isNull())
    {
        m_strError = text("Automatic installation is not available.", "自动安装服务不可用。");
        return false;
    }
    unattended.SetIsoPath(m_strImagePath);
    unattended.DetectIsoOS();
    const QString strType = unattended.GetDetectedOSTypeId();
    const bool fLinux = strType.contains("Linux", Qt::CaseInsensitive) || strType.startsWith("Ubuntu") ||
                        strType.startsWith("Debian") || strType.startsWith("Fedora") || strType.startsWith("Oracle") ||
                        strType.startsWith("RedHat") || strType.startsWith("OpenSUSE") || strType.startsWith("Gentoo");
    if (!unattended.isOk() || !strType.contains("arm64") || !fLinux)
    {
        m_strError =
            text("Choose a Linux Arm64 installation image for this release.", "请选择适用于本版本的 Linux Arm64 安装镜像。");
        return false;
    }
    if (m_options.fAutomatic && !strType.startsWith("Ubuntu", Qt::CaseInsensitive))
    {
        m_strError = text("Automatic setup currently supports Ubuntu. Use manual setup for another Linux image.",
                          "自动安装目前支持 Ubuntu，其他 Linux 镜像请使用手动安装。");
        return false;
    }
    if (m_options.fAutomatic && !QFileInfo::exists(m_options.strAdditions))
    {
        m_strError = text("The bundled Linux integration image is missing. Reinstall Elseport and retry.",
                          "缺少内置的 Linux 增强功能镜像，请重新安装 Elseport 后重试。");
        return false;
    }
    QString strSettings = vbox.ComposeMachineFilename(m_options.strName, QString(), QString(), QString());
    CMachine machine = vbox.CreateMachine(strSettings, m_options.strName, KPlatformArchitecture_ARM, QVector<QString>(),
                                          strType, QString(), QString(), QString(), QString());
    if (!vbox.isOk() || machine.isNull())
    {
        m_strError = text("Could not create this computer. Check its name and destination folder.",
                          "无法创建电脑，请检查名称和存储位置。");
        return false;
    }
    machine.ApplyDefaults(QString());
    machine.SetMemorySize(m_options.cMemoryMB);
    machine.SetCPUCount(m_options.cCpus);
    machine.GetGraphicsAdapter().SetVRAMSize(128);
    machine.GetGraphicsAdapter().SetFeature(KGraphicsFeature_Acceleration3D, false);
    machine.GetClipboard().SetMode(KClipboardMode_Disabled);
    machine.GetClipboard().SetFileTransfersEnabled(false);
    machine.SetExtraData("Elseport/Created", "true");
    if (m_options.fAutomatic)
        machine.SetExtraData("Elseport/InstallPending", "true");
    machine.SaveSettings();
    if (!machine.isOk() || isCancelled())
    {
        m_strError = text("Could not apply the computer settings.", "无法保存电脑配置。");
        return false;
    }
    vbox.RegisterMachine(machine);
    if (!vbox.isOk())
    {
        m_strError = text("Could not register this computer.", "无法登记这台电脑。");
        return false;
    }
    const QUuid uCreatedMachine = machine.GetId();
    CMedium disk;
    bool fSuccess = false;
    do
    {
        if (isCancelled())
            break;
        const QString strDisk = QDir(QFileInfo(strSettings).absolutePath()).filePath(m_options.strName + ".vdi");
        disk = vbox.CreateMedium("VDI", strDisk, KAccessMode_ReadWrite, KDeviceType_HardDisk);
        if (disk.isNull() || !vbox.isOk())
            break;
        CProgress progress =
            disk.CreateBaseStorage((qulonglong)m_options.cDiskGB * _1G, QVector<KMediumVariant>() << KMediumVariant_Standard);
        if (disk.isOk() && !progress.isNull())
            progress.WaitForCompletion(-1);
        if (!disk.isOk() || progress.isNull() || progress.GetResultCode() != 0)
            break;
        CSession session;
        session.createInstance(CLSID_Session);
        machine.LockMachine(session, KLockType_Write);
        if (!machine.isOk())
            break;
        CMachine locked = session.GetMachine();
        const QVector<CStorageController> controllers = locked.GetStorageControllers();
        QString strController;
        for (const CStorageController &controller : controllers)
            if (controller.GetBus() == KStorageBus_SATA || controller.GetBus() == KStorageBus_VirtioSCSI)
            {
                strController = controller.GetName();
                break;
            }
        if (strController.isEmpty())
        {
            locked.AddStorageController("SATA", KStorageBus_SATA);
            strController = "SATA";
        }
        locked.AttachDevice(strController, 0, 0, KDeviceType_HardDisk, disk);
        if (!m_options.fAutomatic)
        {
            CMedium image = vbox.OpenMedium(m_strImagePath, KDeviceType_DVD, KAccessMode_ReadOnly, false);
            locked.AttachDevice(strController, 1, 0, KDeviceType_DVD, image);
        }
        locked.SaveSettings();
        const bool fAttached = locked.isOk();
        session.UnlockMachine();
        if (!fAttached || isCancelled())
            break;
        if (m_options.fAutomatic)
        {
            emit sigStep(3);
            unattended.SetMachine(machine);
            unattended.SetUser(m_options.strUser);
            unattended.SetUserPassword(m_options.strPassword);
            unattended.SetFullUserName(m_options.strUser);
            QString strHost = m_options.strName.toLower();
            strHost.replace(QRegularExpression("[^a-z0-9]+"), "-");
            strHost.remove(QRegularExpression("^-+|-+$"));
            unattended.SetHostname((strHost.isEmpty() ? QString("elseport") : strHost) + ".local");
            unattended.SetTimeZone(QString::fromUtf8(QTimeZone::systemTimeZoneId()));
            unattended.SetLocale(m_options.fChinese ? "zh_CN" : "en_US");
            unattended.SetInstallGuestAdditions(true);
            unattended.SetAdditionsIsoPath(m_options.strAdditions);
            /* A first-boot service confirms that the installed system, rather
             * than the live installer, has started its integration driver. */
            const QString token = uCreatedMachine.toString(QUuid::WithoutBraces);
            const QString unit = "[Unit]\nDescription=Elseport installation completion\nAfter=vboxadd-service.service\n"
                                 "[Service]\nType=oneshot\nRemainAfterExit=yes\n"
                                 "ExecStart=/usr/bin/VBoxControl guestproperty set /Elseport/InstallComplete " +
                                 token + "\nRestart=on-failure\nRestartSec=3\n[Install]\nWantedBy=multi-user.target\n";
            const QString command = "sh -c 'printf \"%b\" \"" + QString(unit).replace("\n", "\\n") +
                                    "\" > /etc/systemd/system/elseport-setup-complete.service; "
                                    "systemctl enable elseport-setup-complete.service'";
            unattended.SetPostInstallCommand(command);
            unattended.Prepare();
            if (unattended.isOk())
                unattended.ConstructMedia();
            if (unattended.isOk())
                unattended.ReconfigureVM();
            if (!unattended.isOk())
                break;
            const QDir folder(QFileInfo(strSettings).absolutePath());
            const QString strPattern = "Unattended-" + token + "-*";
            for (const QString &name : folder.entryList(QStringList() << strPattern, QDir::Files | QDir::NoSymLinks))
                QFile::setPermissions(folder.filePath(name), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
        if (isCancelled())
            break;
        m_uMachine = machine.GetId();
        fSuccess = true;
    } while (false);
    if (!fSuccess)
    {
        m_strError = text("Setup could not be prepared. The incomplete computer will be removed; your selected image is kept.",
                          "无法准备安装。将移除未完成的电脑，保留你选择的镜像。");
        const QVector<CMedium> media = machine.Unregister(KCleanupMode_DetachAllReturnHardDisksOnly);
        if (machine.isOk())
        {
            CProgress cleanup = machine.DeleteConfig(media);
            if (!cleanup.isNull())
                cleanup.WaitForCompletion(-1);
        }
        /* Only files generated for this just-created machine can be removed. */
        const QDir dir(QFileInfo(strSettings).absolutePath());
        const QString prefix = "Unattended-" + uCreatedMachine.toString(QUuid::WithoutBraces) + "-";
        for (const QString &name : dir.entryList(QStringList() << prefix + "*", QDir::Files | QDir::NoSymLinks))
            QFile::remove(dir.filePath(name));
    }
    return fSuccess;
}

int UIElseportInstallWorker::createHttp()
{
    RTHTTP hHttp = NIL_RTHTTP;
    int rc = RTHttpCreate(&hHttp);
    if (RT_FAILURE(rc))
        return rc;
    RTHttpUseSystemProxySettings(hHttp);
    RTHttpSetFollowRedirects(hHttp, 8);
    rc = RTHttpUseTemporaryCaFile(hHttp, NULL);
    if (RT_FAILURE(rc))
    {
        RTHttpDestroy(hHttp);
        return rc;
    }

    QMutexLocker locker(&m_mutexHttp);
    m_hHttp = hHttp;
    return VINF_SUCCESS;
}

void UIElseportInstallWorker::destroyHttp()
{
    QMutexLocker locker(&m_mutexHttp);
    if (m_hHttp != NIL_RTHTTP)
        RTHttpDestroy(m_hHttp);
    m_hHttp = NIL_RTHTTP;
}

int UIElseportInstallWorker::fetchText(const QString &strUrl, QString &strText)
{
    int rc = createHttp();
    if (RT_FAILURE(rc))
        return rc;
    m_fTextRequest = true;
    m_textResponse.clear();
    uint32_t status = 0;
    rc = RTHttpSetDownloadCallback(m_hHttp, RTHTTPDOWNLOAD_F_ANY_STATUS, handleData, this);
    if (RT_SUCCESS(rc))
        rc = RTHttpPerform(m_hHttp, strUrl.toUtf8().constData(), RTHTTPMETHOD_GET, NULL, 0, &status, NULL, NULL, NULL, NULL);
    if (RT_SUCCESS(rc))
    {
        if (status == 200)
            strText = QString::fromUtf8(m_textResponse);
        else
            rc = VERR_NET_IO_ERROR;
    }
    destroyHttp();
    m_textResponse.clear();
    m_fTextRequest = false;
    return rc;
}

/* static */
DECLCALLBACK(int)
UIElseportInstallWorker::handleData(RTHTTP hHttp, void const *pvBuf, size_t cbBuf, uint32_t uHttpStatus, uint64_t offContent,
                                    uint64_t cbContent, void *pvUser)
{
    RT_NOREF(hHttp);
    return static_cast<UIElseportInstallWorker *>(pvUser)->storeData(pvBuf, cbBuf, uHttpStatus, offContent, cbContent);
}

int UIElseportInstallWorker::storeData(void const *pvBuf, size_t cbBuf, uint32_t uHttpStatus, uint64_t offContent,
                                       uint64_t cbContent)
{
    m_uHttpStatus = uHttpStatus;
    if (isCancelled())
        return VERR_CANCELLED;
    /* Error documents aren't what we are after: */
    if (uHttpStatus != 200 && uHttpStatus != 206)
        return VINF_SUCCESS;
    if (m_fTextRequest)
    {
        if (cbBuf > _1M || (size_t)m_textResponse.size() > _1M - cbBuf)
            return VERR_TOO_MUCH_DATA;
        m_textResponse.append(static_cast<const char *>(pvBuf), (qsizetype)cbBuf);
        return VINF_SUCCESS;
    }
    if ((uint64_t)m_cbStored + cbBuf > UINT64_C(16) * _1G)
        return VERR_TOO_MUCH_DATA;

    /* A server not doing ranges sends everything again: */
    if (uHttpStatus == 200 && offContent == 0 && m_cbResumedFrom != 0)
    {
        m_filePartial.resize(0);
        m_filePartial.seek(0);
        m_cbResumedFrom = 0;
        m_cbStored = 0;
    }

    if (m_filePartial.write((const char *)pvBuf, (qint64)cbBuf) != (qint64)cbBuf)
    {
        m_fWriteFailed = true;
        return VERR_WRITE_ERROR;
    }
    m_cbStored += (qint64)cbBuf;

    /* A few reports per second are plenty: */
    const qint64 msNow = QDateTime::currentMSecsSinceEpoch();
    if (msNow - m_msLastReport >= 500)
    {
        const QStorageInfo storage(m_options.strCache);
        if (storage.isValid() && storage.bytesAvailable() >= 0 && storage.bytesAvailable() < 256 * (qint64)_1M)
        {
            m_fWriteFailed = true;
            return VERR_DISK_FULL;
        }
        if (m_msLastReport != 0)
            m_cbPerSecond = (m_cbStored - m_cbLastReport) * 1000 / (msNow - m_msLastReport);
        m_msLastReport = msNow;
        m_cbLastReport = m_cbStored;
        const qint64 cbTotal = cbContent == UINT64_MAX ? 0 : m_cbResumedFrom + (qint64)cbContent;
        emit sigProgress(m_cbStored, cbTotal, m_cbPerSecond);
    }
    return VINF_SUCCESS;
}

bool UIElseportInstallWorker::download(const QString &strUrl, const QString &strTarget, const QString &strSha256)
{
    const QString strPartial = strTarget + ".part";
    for (int iAttempt = 0; iAttempt < QUICKINSTALL_DOWNLOAD_ATTEMPTS; ++iAttempt)
    {
        m_filePartial.setFileName(strPartial);
        if (!m_filePartial.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            m_strError = text("Could not write to %1: %2", "无法写入 %1：%2").arg(strPartial, m_filePartial.errorString());
            return false;
        }
        m_cbResumedFrom = m_filePartial.size();
        m_cbStored = m_cbResumedFrom;
        m_msLastReport = 0;
        m_cbLastReport = m_cbResumedFrom;
        m_uHttpStatus = 0;
        m_fWriteFailed = false;

        int rc = createHttp();
        if (RT_SUCCESS(rc) && m_cbResumedFrom != 0)
            rc = RTHttpAddHeader(m_hHttp, "Range", QString("bytes=%1-").arg(m_cbResumedFrom).toUtf8().constData(), RTSTR_MAX,
                                 RTHTTPADDHDR_F_BACK);
        if (RT_SUCCESS(rc))
            rc = RTHttpSetDownloadCallback(m_hHttp, RTHTTPDOWNLOAD_F_ANY_STATUS, handleData, this);
        uint32_t uStatus = 0;
        if (RT_SUCCESS(rc))
            rc = RTHttpPerform(m_hHttp, strUrl.toUtf8().constData(), RTHTTPMETHOD_GET, NULL, 0, &uStatus, NULL, NULL, NULL,
                               NULL);
        destroyHttp();
        m_filePartial.close();

        if (isCancelled())
            return false;
        if (m_fWriteFailed)
        {
            m_strError = text("Could not write to %1. Is the disk full?", "无法写入 %1，请检查可用磁盘空间。").arg(strPartial);
            return false;
        }

        /* Everything was there already, the server has nothing left to send: */
        const bool fRangeDone = m_uHttpStatus == 416 || uStatus == 416;
        if (RT_SUCCESS(rc) && (uStatus == 200 || uStatus == 206 || fRangeDone))
        {
            emit sigStep(1);
            QString strActual;
            if (!calculateSha256(strPartial, strActual))
                return false;
            if (strActual == strSha256)
            {
                if (QFile::rename(strPartial, strTarget))
                    return true;
                m_strError = text("Could not move the download to %1.", "无法将下载文件保存到 %1。").arg(strTarget);
                return false;
            }

            /* Whatever went wrong can't be resumed from, it takes a fresh start: */
            QFile::remove(strPartial);
            m_strError =
                text("The downloaded file is damaged, its checksum doesn't match. Try again.", "下载文件校验不匹配，请重试。");
            return false;
        }

        /* Refusals won't change by asking again: */
        if (uStatus >= 400 && uStatus < 500)
        {
            m_strError = text("The server refused the download (HTTP %1).", "服务器拒绝了下载请求（HTTP %1）。").arg(uStatus);
            return false;
        }

        /* A broken connection, carry on from where it stopped after a moment: */
        LogRel(("GUI: UIElseportInstallWorker: Download attempt %d stopped at %lld bytes: rc=%Rrc status=%u\n", iAttempt + 1,
                m_cbStored, rc, uStatus));
        for (int i = 0; i < 50 && !isCancelled(); ++i)
            msleep(100);
        if (isCancelled())
            return false;
    }

    m_strError = text("The download keeps stopping. Check the connection and retry; downloaded data is kept.",
                      "下载多次中断，请检查网络后重试，已下载的数据会保留。");
    return false;
}

bool UIElseportInstallWorker::calculateSha256(const QString &strPath, QString &strSha256)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        m_strError = text("Could not read %1: %2", "无法读取 %1：%2").arg(strPath, file.errorString());
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const qint64 cbTotal = file.size();
    qint64 cbDone = 0;
    qint64 msLastReport = 0;
    QByteArray buffer(4 * _1M, Qt::Uninitialized);
    while (!isCancelled())
    {
        const qint64 cbRead = file.read(buffer.data(), buffer.size());
        if (cbRead < 0)
        {
            m_strError = text("Could not read %1: %2", "无法读取 %1：%2").arg(strPath, file.errorString());
            return false;
        }
        if (cbRead == 0)
        {
            strSha256 = QString::fromLatin1(hash.result().toHex());
            return true;
        }
        hash.addData(QByteArrayView(buffer.constData(), cbRead));
        cbDone += cbRead;
        const qint64 msNow = QDateTime::currentMSecsSinceEpoch();
        if (msNow - msLastReport >= 500)
        {
            msLastReport = msNow;
            emit sigProgress(cbDone, cbTotal, 0);
        }
    }
    return false;
}
