/* $Id: VBoxAboutDlg.cpp 113629 2026-03-27 14:38:08Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - VBoxAboutDlg class implementation.
 */

/*
 * Copyright (C) 2006-2026 Oracle and/or its affiliates.
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

/* Qt includes: */
#include <QGuiApplication>
#include <QDir>
#include <QUrl>
#include "UIExtraDataManager.h"
#include <QLabel>
#include <QLocale>
#include <QPainter>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIIconPool.h"
#include "UITranslationEventListener.h"
#include "UIVersion.h"
#include "VBoxAboutDlg.h"

/* Other VBox includes: */
#include <iprt/path.h> /* RTPathExecDir */
#include <VBox/version.h> /* VBOX_VENDOR */


VBoxAboutDlg::VBoxAboutDlg(QWidget *pParent /* = 0 */, const QString &strVersion /* = QString() */)
#ifdef VBOX_WS_MAC
    // No need for About dialog parent on macOS.
    // First of all, non of other native apps (Safari, App Store, iTunes) centers About dialog according the app itself, they do
    // it according to screen instead, we should do it as well.  Besides that since About dialog is not modal, it will be in
    // conflict with modal dialogs if there will be a parent passed, because the dialog will not have own event-loop in that case.
    : QDialog(0)
    , m_pPseudoParent(pParent)
#else
    // On other hosts we will keep the current behavior for now.
    // First of all it's quite difficult to find native (Metro UI) Windows app which have About dialog at all.  But non-native
    // cross-platform apps (Qt Creator, VLC) centers About dialog according the app exactly.
    : QDialog(pParent)
    , m_pPseudoParent(0)
#endif
    , m_fPolished(false)
    , m_strVersion(strVersion)
    , m_pMainLayout(0)
    , m_pLabel(0)
{
    prepare();
}

void VBoxAboutDlg::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QDialog::showEvent(pEvent);

    /* Polish once: */
    if (!m_fPolished)
    {
        m_fPolished = true;
        setFixedSize(m_size);

        QRect geo = geometry();
#ifdef VBOX_WS_MAC
        /* Center according to parent screen: */
        geo.moveCenter(gpDesktop->screenGeometry(m_pPseudoParent).center());
#else
        /* Center according to parent widget: */
        if (parentWidget())
            geo.moveCenter(parentWidget()->geometry().center());
        /* Center according to parent screen: */
        else
            geo.moveCenter(gpDesktop->screenGeometry(m_pPseudoParent).center());
#endif
        setGeometry(geo);
    }
}

void VBoxAboutDlg::paintEvent(QPaintEvent *)
{
    /* Draw About-VirtualBox background image: */
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_pixmap);
}

void VBoxAboutDlg::sltRetranslateUI()
{
#ifdef VBOX_WITH_ELSEPORT_UI
    /* The temporary notification dialog is constructed before extra-data initialization. */
    const QString strLanguage = gEDataManager ? gEDataManager->languageId() : QString();
    const bool chinese = strLanguage.isEmpty()
                       ? QLocale::system().language() == QLocale::Chinese
                       : strLanguage.startsWith("zh");
    setWindowTitle(chinese ? QString::fromUtf8("关于 Elseport") : QString("About Elseport"));
    if (m_pLabel)
    {
        m_pLabel->setText(chinese
            ? QString::fromUtf8("<h2>Elseport 0.1.0 Preview</h2><p>面向 Mac 的 Linux 工作空间。</p>"
                                "<p>基于 VirtualBox 开源代码的独立项目，不受 Oracle 认可或赞助。"
                                "相关版权归 Oracle 与各贡献者所有。</p><p>受覆盖的代码使用 GNU GPL 第三版，"
                                "不提供担保。第三方组件保留各自许可证。</p>"
                                "<p><a href='https://github.com/gaochangw/Elseport'>源码与构建说明</a> · "
                                "<a href='https://www.gnu.org/licenses/gpl-3.0.html'>GNU GPL v3</a></p>")
            : QString("<h2>Elseport 0.1.0 Preview</h2><p>A Linux workspace for your Mac.</p>"
                      "<p>An independent project built from the open-source code of VirtualBox. "
                      "Not endorsed or sponsored by Oracle. Respective copyrights remain with Oracle and contributors.</p>"
                      "<p>Covered code is available under GNU GPL version 3, without warranty. "
                      "Third-party components retain their own licenses.</p>"
                      "<p><a href='https://github.com/gaochangw/Elseport'>Source and build instructions</a> · "
                      "<a href='https://www.gnu.org/licenses/gpl-3.0.html'>GNU GPL v3</a></p>"));
    }
    return;
#endif
    setWindowTitle(tr("VirtualBox - About"));

    if (m_pLabel)
    {
        const QString strAboutText = tr("VirtualBox Graphical User Interface");
#ifdef VBOX_BLEEDING_EDGE
        const QString strVersionText = "EXPERIMENTAL build %1 - " + QString(VBOX_BLEEDING_EDGE);
#else
        const QString strVersionText = tr("Version %1");
#endif
#ifdef VBOX_OSE
        m_strAboutText = strAboutText + " " + strVersionText.arg(m_strVersion) + "\n"
                       + QString("%1 2004-" VBOX_C_YEAR " " VBOX_VENDOR).arg(QChar(0xa9));
#else
        m_strAboutText = strAboutText + "\n" + strVersionText.arg(m_strVersion);
#endif
        QString strPlatformName = QGuiApplication::platformName();
        if (strPlatformName.isEmpty())
            m_strAboutText = m_strAboutText + QString(" (Qt%1)").arg(qVersion());
        else
            m_strAboutText = m_strAboutText + QString(" (Qt%1 on %2)").arg(qVersion()).arg(strPlatformName);
        m_strAboutText = m_strAboutText + "\n" + QString("Copyright %1 %2 %3.")
                                                         .arg(QChar(0xa9)).arg(VBOX_C_YEAR).arg(VBOX_VENDOR);
        m_pLabel->setText(m_strAboutText);
    }
}

void VBoxAboutDlg::prepare()
{
    /* If we have some parent - delete dialog on close: */
    if (m_pPseudoParent || parentWidget())
        setAttribute(Qt::WA_DeleteOnClose);
    /* Do not count that window as important for application,
     * it will NOT be taken into account when other top-level windows will be closed: */
    setAttribute(Qt::WA_QuitOnClose, false);

    /* Make sure the dialog is deleted on pseudo-parent destruction: */
    if (m_pPseudoParent)
        connect(m_pPseudoParent, &QObject::destroyed, this, &VBoxAboutDlg::close);

#ifdef VBOX_WITH_ELSEPORT_UI
    m_size = QSize(560, 430);
    m_pixmap = QPixmap(m_size);
    m_pixmap.fill(Qt::white);
    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->setContentsMargins(28, 24, 28, 20);
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/elseport/app-icon.png").pixmap(72, 72));
    m_pMainLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    m_pLabel = new QLabel(this);
    m_pLabel->setWordWrap(true);
    m_pLabel->setOpenExternalLinks(true);
    m_pLabel->setStyleSheet("color:#242337;background:white;font-size:13px;");
    m_pMainLayout->addWidget(m_pLabel, 1);
    prepareCloseButton();
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &VBoxAboutDlg::sltRetranslateUI);
    sltRetranslateUI();
    return;
#endif

    /* Choose default image: */
    QString strPath(":/about.png");

    /* Branding: Use a custom about splash picture if set: */
    const QString strSplash = UIVersionInfo::brandingGetKey("UI/AboutSplash");
    if (UIVersionInfo::brandingIsActive() && !strSplash.isEmpty())
    {
        char szExecPath[1024];
        RTPathExecDir(szExecPath, 1024);
        QString strTmpPath = QString("%1/%2").arg(szExecPath).arg(strSplash);
        if (QFile::exists(strTmpPath))
            strPath = strTmpPath;
    }

    /* Load image: */
    const QIcon icon = UIIconPool::iconSet(strPath);
    m_size = QSize(640, 480);
    QWidget *pParent = m_pPseudoParent ? m_pPseudoParent : parentWidget();
    const qreal fDevicePixelRatio = gpDesktop->devicePixelRatio(pParent);
    m_pixmap = icon.pixmap(m_size, fDevicePixelRatio);

    /* Prepare main-layout: */
    prepareMainLayout();

    /* Translate: */
    sltRetranslateUI();
    if (UITranslationEventListener::instance())
        connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &VBoxAboutDlg::sltRetranslateUI);
}

void VBoxAboutDlg::prepareMainLayout()
{
    /* Create main-layout: */
    m_pMainLayout = new QVBoxLayout(this);
    if (m_pMainLayout)
    {
        /* Prepare stuff: */
        prepareLabel();
        prepareCloseButton();
    }
}

void VBoxAboutDlg::prepareLabel()
{
    /* Create label for version text: */
    m_pLabel = new QLabel(this);
    if (m_pLabel)
    {
        /* Prepare label for version text: */
        QPalette palette;
        /* Branding: Set a different text color (because splash also could be white),
         * otherwise use white as default color: */
        const QString strColor = UIVersionInfo::brandingGetKey("UI/AboutTextColor");
        if (!strColor.isEmpty())
            palette.setColor(QPalette::WindowText, QColor(strColor).name());
        else
            palette.setColor(QPalette::WindowText, Qt::white);
        m_pLabel->setPalette(palette);
        m_pLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_pLabel->setFont(font());

        /* Add label to the main-layout: */
        if (m_pMainLayout)
        {
            m_pMainLayout->addWidget(m_pLabel);
            m_pMainLayout->setAlignment(m_pLabel, Qt::AlignRight | Qt::AlignBottom);
        }
    }
}

void VBoxAboutDlg::prepareCloseButton()
{
    /* Create button-box: */
    QIDialogButtonBox *pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    if (pButtonBox)
    {
        /* Prepare close-button: */
        connect(pButtonBox, &QDialogButtonBox::rejected, this, &VBoxAboutDlg::close);

        /* Add button-box to the main-layout: */
        if (m_pMainLayout)
            m_pMainLayout->addWidget(pButtonBox);
    }
}
