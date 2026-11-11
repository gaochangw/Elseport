/* $Id$ */
/** @file
 * Elseport - prepare Linux installation media and create a real virtual machine.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef FEQT_INCLUDED_SRC_elseport_UIElseportInstallWorker_h
#define FEQT_INCLUDED_SRC_elseport_UIElseportInstallWorker_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
#pragma once
#endif

#include <QAtomicInt>
#include <QFile>
#include <QMutex>
#include <QThread>
#include <QUuid>
#include <iprt/http.h>

/** Validated values copied from the installation form. */
struct UIElseportInstallOptions
{
    /** User-visible computer name. */
    QString strName;
    /** Account for automatic Ubuntu setup. Never logged or persisted by the UI. */
    QString strUser;
    /** Password consumed by the unattended installer. */
    QString strPassword;
    /** Empty to download Ubuntu, otherwise a user-selected Linux image. */
    QString strImage;
    /** Private cache for resumable downloads. */
    QString strCache;
    /** Bundled Linux Guest Additions image. */
    QString strAdditions;
    /** Memory in MiB. */
    uint32_t cMemoryMB;
    /** Virtual processors. */
    uint32_t cCpus;
    /** Maximum virtual disk capacity in GiB. */
    uint32_t cDiskGB;
    /** Perform automatic Ubuntu installation instead of booting a manual installer. */
    bool fAutomatic;
    /** Language for preparation errors. */
    bool fChinese;
};

/** Performs slow media work away from the GUI thread; owns its COM session. */
class UIElseportInstallWorker : public QThread
{
    Q_OBJECT;

  public:
    /** Creates a worker with copied, validated form values. */
    UIElseportInstallWorker(const UIElseportInstallOptions &options, QObject *pParent);
    /** Requests cancellation, keeping a partial download for a later retry. */
    void cancel();
    /** Whether cancellation was requested. */
    bool isCancelled() const
    {
        return m_fCancelled.loadRelaxed() != 0;
    }
    /** Created VM, if preparation completed. */
    QUuid machineId() const
    {
        return m_uMachine;
    }
    /** Human-readable failure without account credentials. */
    QString error() const
    {
        return m_strError;
    }
  signals:
    /** Preparation stage: download, verify, create, configure. */
    void sigStep(int iStep);
    /** Download or verification progress. */
    void sigProgress(qint64 cbDone, qint64 cbTotal, qint64 cbPerSecond);

  protected:
    void run() RT_OVERRIDE;

  private:
    /** Resolves the official Ubuntu image and verifies its bytes. */
    bool getUbuntuImage();
    /** Creates the VM and its installation configuration. */
    bool createMachine();
    /** Reads a small HTTPS text resource. */
    int fetchText(const QString &strUrl, QString &strText);
    /** Downloads and verifies an image using a resumable partial file. */
    bool download(const QString &strUrl, const QString &strTarget, const QString &strSha256);
    /** Computes a hash without blocking the GUI. */
    bool calculateSha256(const QString &strPath, QString &strSha256);
    /** Initializes the HTTP client. */
    int createHttp();
    /** Releases the HTTP client. */
    void destroyHttp();
    /** HTTP body callback. */
    static DECLCALLBACK(int) handleData(RTHTTP, const void *, size_t, uint32_t, uint64_t, uint64_t, void *);
    /** Writes received bytes and emits bounded progress updates. */
    int storeData(const void *, size_t, uint32_t, uint64_t, uint64_t);
    /** Localized preparation text. */
    QString text(const char *pszEnglish, const char *pszChinese) const;
    /** Form values owned by this worker. */
    UIElseportInstallOptions m_options;
    /** Resulting machine identifier. */
    QUuid m_uMachine;
    /** Current media path. */
    QString m_strImagePath;
    /** Last preparation error. */
    QString m_strError;
    /** Cross-thread cancellation. */
    QAtomicInt m_fCancelled;
    /** Serializes HTTP cancellation and destruction. */
    QMutex m_mutexHttp;
    /** HTTP request owned by this worker. */
    RTHTTP m_hHttp;
    /** Partial download file. */
    QFile m_filePartial;
    /** Download counters and throttled progress timestamps. */
    qint64 m_cbResumedFrom, m_cbStored, m_msLastReport, m_cbLastReport, m_cbPerSecond;
    /** Last response status. */
    uint32_t m_uHttpStatus;
    /** A local write failed. */
    bool m_fWriteFailed;
    /** Bounded response storage while fetching the checksum manifest. */
    bool m_fTextRequest;
    QByteArray m_textResponse;
};
#endif /* !FEQT_INCLUDED_SRC_elseport_UIElseportInstallWorker_h */
