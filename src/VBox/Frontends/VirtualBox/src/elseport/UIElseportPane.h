/* $Id$ */
/** @file
 * Elseport - native Linux workspace manager.
 */
/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FEQT_INCLUDED_SRC_elseport_UIElseportPane_h
#define FEQT_INCLUDED_SRC_elseport_UIElseportPane_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
#pragma once
#endif
#include <iprt/cdefs.h>
#include <QWidget>
#include <QUuid>

struct UIElseportData;
/** Native, state-backed implementation of the Elseport manager design. */
class UIElseportPane : public QWidget
{
    Q_OBJECT;

  public:
    /** Creates the native manager inside the existing application shell. */
    explicit UIElseportPane(QWidget *pParent = 0);
    ~UIElseportPane() RT_OVERRIDE;
    /** Opens the create-computer form. */
    void createComputer(const QString &strImage = QString());
    /** Shows the product, license and source information. */
    void showAbout();
    /** Refreshes real backend state. */
    void refresh();
    /** Checks outstanding work before the application closes. */
    bool canClose();
  signals:
    /** Opens the existing advanced settings for a specific machine. */
    void sigAdvancedSettings(const QUuid &uMachine);
    /** Asks the application shell to refresh localized menu labels. */
    void sigLanguageChanged();

  private:
    UIElseportData *m_pData;
};
#endif /* !FEQT_INCLUDED_SRC_elseport_UIElseportPane_h */
