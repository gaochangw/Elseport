/* $Id$ */
/** @file
 * Elseport - real machine state and native desktop controls.
 */
/* SPDX-License-Identifier: GPL-3.0-only */

/** @page pg_elseport_manager Elseport native manager
 * The presentation uses existing Main interfaces. Sharing is opt-in, destructive
 * restore/power-off actions require confirmation, and every long operation has
 * an actual progress object or a cancellable media worker. No HTML is embedded.
 */
#include "UIElseportPane.h"
#include "UIElseportIntegration.h"
#include "UIElseportInstallWorker.h"
#include "UICommon.h"
#include "UIGlobalSession.h"
#include "UIExtraDataManager.h"
#include "UILocalMachineStuff.h"
#include "UITranslationEventListener.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIErrorString.h"
#include "CClipboardSettings.h"
#include "CConsole.h"
#include "CDisplay.h"
#include "CGraphicsAdapter.h"
#include "CHost.h"
#include "CMachine.h"
#include "CMediumAttachment.h"
#include "CProgress.h"
#include "CSession.h"
#include "CSharedFolder.h"
#include "CSnapshot.h"
#include "CSystemProperties.h"
#include "CVirtualBox.h"
#include "CVirtualBoxClient.h"
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

static bool epChinese()
{
    const QString lang = gEDataManager->languageId();
    return lang.isEmpty() ? QLocale::system().language() == QLocale::Chinese : lang.startsWith("zh");
}
static QString epText(const char *en, const char *zh)
{
    return QString::fromUtf8(epChinese() ? zh : en);
}
static QLabel *epLabel(const QString &text, const char *name, QWidget *parent = 0)
{
    QLabel *label = new QLabel(text, parent);
    label->setObjectName(QString::fromLatin1(name));
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    return label;
}
static QPushButton *epButton(const QString &text, const char *name, bool primary = false)
{
    QPushButton *button = new QPushButton(text);
    button->setObjectName(QString::fromLatin1(name));
    button->setProperty("epPrimary", primary);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(38);
    return button;
}
static QString epState(KMachineState state)
{
    switch (state)
    {
    case KMachineState_Running:
        return epText("Running", "正在运行");
    case KMachineState_Paused:
        return epText("Paused", "已暂停");
    case KMachineState_Saved:
        return epText("Saved", "已保存");
    case KMachineState_PoweredOff:
        return epText("Powered off", "已关闭");
    case KMachineState_Aborted:
        return epText("Stopped unexpectedly", "意外停止");
    case KMachineState_Starting:
        return epText("Starting…", "正在启动…");
    case KMachineState_Stopping:
        return epText("Shutting down…", "正在关闭…");
    case KMachineState_Saving:
        return epText("Saving…", "正在保存…");
    case KMachineState_Restoring:
        return epText("Restoring…", "正在恢复…");
    default:
        return epText("Working…", "正在处理…");
    }
}
static bool epOffline(KMachineState state)
{
    return state == KMachineState_PoweredOff || state == KMachineState_Aborted;
}
static bool epLinux(const QString &type)
{
    return type.contains("Linux", Qt::CaseInsensitive) || type.startsWith("Ubuntu") || type.startsWith("Debian") ||
           type.startsWith("Fedora") || type.startsWith("RedHat") || type.startsWith("Oracle") || type.startsWith("OpenSUSE") ||
           type.startsWith("ArchLinux") || type.startsWith("Mandriva") || type.startsWith("Gentoo");
}

struct UIElseportData
{
    UIElseportPane *q;
    QStackedWidget *pages;
    QListWidget *machines;
    QLabel *heading, *subheading, *computerName, *state, *preview, *resources, *empty;
    QPushButton *start, *pause, *save, *shutdown, *more, *newButton;
    QPushButton *navComputers, *navSnapshots, *navSettings, *navAbout;
    QWidget *details, *settingsForm;
    QTreeWidget *snapshots;
    QPushButton *snapshotNew, *snapshotRestore;
    QDoubleSpinBox *memory, *newMemory;
    QSpinBox *cpus, *newCpus, *newDisk;
    QComboBox *clipboard, *scale, *language;
    QCheckBox *files, *share, *writable, *automatic;
    QLineEdit *folder, *newName, *newUser, *newPassword, *image;
    QLabel *settingsStatus, *createStatus, *taskLabel;
    QPushButton *create, *cancel, *settingsSave;
    QProgressBar *progressBar;
    QTimer *timer;
    QUuid selected;
    CProgress progress;
    CSession session;
    UIElseportInstallWorker *worker;
    QString operation;
    bool loading;
    bool busy;
    bool closeWhenIdle;
    explicit UIElseportData(UIElseportPane *pane) : q(pane), worker(0), loading(false), busy(false), closeWhenIdle(false) {}
    void build();
    void refresh(bool loadSettings = false);
    void select();
    CMachine machine() const;
    void showPage(int page);
    void loadSettings();
    void saveSettings();
    void startMachine();
    void control(const QString &action);
    void beginProgress(const QString &caption, const CProgress &value, const CSession &lock);
    void tickProgress();
    void loadSnapshots();
    void takeSnapshot();
    void restoreSnapshot();
    void createMachine();
    void error(const QString &message, const QString &detail = QString());
    void updatePreview(CMachine &machine);
    void checkInstall(CMachine &machine);
    void validateForm();
};

void UIElseportData::error(const QString &message, const QString &detail)
{
    QMessageBox box(QMessageBox::Warning, "Elseport", message, QMessageBox::Ok, q);
    box.setTextFormat(Qt::PlainText);
    if (!detail.isEmpty())
        box.setDetailedText(detail);
    box.exec();
}

void UIElseportData::build()
{
    CVirtualBox vbox = gpGlobalSession->virtualBox();
    if (vbox.GetExtraData("Elseport/NativeInitialized") != "true")
    {
        CSystemProperties properties = vbox.GetSystemProperties();
        if (QFileInfo(properties.GetDefaultMachineFolder()).fileName() == "VirtualBox VMs")
            properties.SetDefaultMachineFolder(QDir(vbox.GetHomeFolder()).filePath("Computers"));
        vbox.SetExtraData("Elseport/NativeInitialized", "true");
    }
    q->setObjectName("ElseportManager");
    q->setStyleSheet(QStringLiteral(
        "#ElseportManager{background:#ffffff;color:#242337;font-size:13px;}"
        "#ElseportSidebar{background:#f5f4f9;border-right:1px solid #e5e3ed;}"
        "QLabel{color:#242337;background:transparent;}"
        "QLabel#EpTitle{font-size:28px;font-weight:650;}QLabel#EpName{font-size:20px;font-weight:600;}"
        "QLabel#EpMuted{color:#777387;font-size:12px;}QLabel#EpSection{color:#777387;font-size:11px;}"
        "QLabel#EpState{color:#6252cf;font-size:12px;}"
        "QPushButton{background:#f8f7fb;color:#37334d;border:1px solid #e5e3ed;border-radius:8px;padding:8px 15px;}"
        "QPushButton:hover{background:#efecf8;}QPushButton:disabled{color:#aaa7b7;background:#faf9fc;}"
        "QPushButton[epPrimary=\"true\"]{background:#6252cf;color:white;border-color:#6252cf;font-weight:600;}"
        "QPushButton[epPrimary=\"true\"]:hover{background:#5545bf;}QPushButton[epPrimary=\"true\"]:disabled{background:#b6aee3;"
        "border-color:#b6aee3;}"
        "QPushButton[epNav=\"true\"]{text-align:left;background:transparent;border:0;padding:10px 14px;}"
        "QPushButton[epNav=\"true\"]:checked{background:#edeafa;color:#6252cf;font-weight:600;}"
        "QListWidget{background:white;border:0;outline:0;}QListWidget::item{padding:14px 10px;border-radius:10px;margin:2px 0;}"
        "QListWidget::item:selected{background:#f3f1fa;color:#242337;border:1px solid #ded9ed;}"
        "QFrame#EpCard{background:white;border:1px solid #e5e3ed;border-radius:14px;}"
        "QLineEdit,QSpinBox,QDoubleSpinBox,QComboBox{color:#242337;background:white;border:1px solid "
        "#ddd9e9;border-radius:7px;padding:7px;min-height:22px;}"
        "QTreeWidget{background:white;border:1px solid #e5e3ed;border-radius:8px;padding:8px;}"
        "QTabWidget::pane{border:1px solid #e5e3ed;border-radius:8px;}QTabBar::tab{padding:10px "
        "20px;background:#f8f7fb;color:#6b6a7d;}QTabBar::tab:selected{color:#6252cf;background:#edeafa;}"
        "QProgressBar{background:#eeecf5;border:0;border-radius:5px;text-align:center;min-height:12px;}QProgressBar::chunk{"
        "background:#6252cf;border-radius:5px;}"
        "QCheckBox{spacing:10px;color:#37334d;}QScrollArea{border:0;background:white;}"));
    QHBoxLayout *root = new QHBoxLayout(q);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    QWidget *side = new QWidget;
    side->setObjectName("ElseportSidebar");
    side->setFixedWidth(200);
    QVBoxLayout *sl = new QVBoxLayout(side);
    sl->setContentsMargins(16, 28, 16, 18);
    sl->setSpacing(10);
    QHBoxLayout *brand = new QHBoxLayout;
    QLabel *mark = new QLabel;
    mark->setPixmap(QIcon(":/elseport/mark.svg").pixmap(QSize(27, 27), q->devicePixelRatioF()));
    QLabel *name = epLabel("Elseport", "EpName");
    brand->addWidget(mark);
    brand->addWidget(name);
    brand->addStretch();
    sl->addLayout(brand);
    sl->addSpacing(27);
    sl->addWidget(epLabel(epText("WORKSPACE", "工作空间"), "EpSection"));
    navComputers = epButton(epText("Computers", "虚拟电脑"), "ElseportComputers");
    navSnapshots = epButton(epText("Restore points", "恢复点"), "ElseportSnapshots");
    navSettings = epButton(epText("Settings", "设置"), "ElseportSettings");
    navAbout = epButton(epText("About Elseport", "关于 Elseport"), "ElseportAbout");
    const QList<QPushButton *> nav = {navComputers, navSnapshots, navSettings, navAbout};
    for (QPushButton *button : nav)
    {
        button->setProperty("epNav", true);
        button->setCheckable(true);
        sl->addWidget(button);
    }
    sl->addStretch();
    language = new QComboBox;
    language->setObjectName("ElseportLanguage");
    language->addItem("English", "C");
    language->addItem(QString::fromUtf8("简体中文"), "zh_CN");
    language->setCurrentIndex(epChinese() ? 1 : 0);
    sl->addWidget(language);
    sl->addWidget(epLabel(epText("Stored on this Mac", "保存在这台 Mac 上"), "EpMuted"));
    root->addWidget(side);
    QWidget *body = new QWidget;
    QVBoxLayout *bl = new QVBoxLayout(body);
    bl->setContentsMargins(32, 28, 32, 20);
    bl->setSpacing(20);
    QHBoxLayout *top = new QHBoxLayout;
    QVBoxLayout *titles = new QVBoxLayout;
    heading = epLabel(epText("Your computers", "你的虚拟电脑"), "EpTitle");
    subheading = epLabel(epText("A quiet place to pick up your work.", "一个安静的地方，继续你的工作。"), "EpMuted");
    titles->addWidget(heading);
    titles->addWidget(subheading);
    top->addLayout(titles);
    top->addStretch();
    newButton = epButton(epText("+  New computer", "+  创建虚拟电脑"), "ElseportNewComputer");
    top->addWidget(newButton);
    bl->addLayout(top);
    pages = new QStackedWidget;
    pages->setObjectName("ElseportPages");
    bl->addWidget(pages, 1);
    QWidget *library = new QWidget;
    QHBoxLayout *ll = new QHBoxLayout(library);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(22);
    machines = new QListWidget;
    machines->setObjectName("ElseportMachineList");
    machines->setFixedWidth(202);
    ll->addWidget(machines);
    details = new QWidget;
    QVBoxLayout *dl = new QVBoxLayout(details);
    dl->setContentsMargins(0, 0, 0, 0);
    dl->setSpacing(14);
    QFrame *card = new QFrame;
    card->setObjectName("EpCard");
    QVBoxLayout *cl = new QVBoxLayout(card);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(14);
    preview = new QLabel;
    preview->setObjectName("ElseportPreview");
    preview->setAlignment(Qt::AlignCenter);
    preview->setMinimumHeight(215);
    preview->setStyleSheet("background:#f3f1fa;border-radius:10px;color:#8a82a8;");
    cl->addWidget(preview, 1);
    computerName = epLabel(QString(), "EpName");
    state = epLabel(QString(), "EpState");
    resources = epLabel(QString(), "EpMuted");
    cl->addWidget(computerName);
    cl->addWidget(state);
    cl->addWidget(resources);
    QHBoxLayout *actions = new QHBoxLayout;
    start = epButton(QString(), "ElseportStart", true);
    pause = epButton(QString(), "ElseportPause");
    save = epButton(epText("Save and close", "保存并关闭"), "ElseportSave");
    shutdown = epButton(epText("Shut down", "关机"), "ElseportShutdown");
    more = epButton("…", "ElseportMore");
    more->setAccessibleName(epText("More options", "更多操作"));
    actions->addWidget(start);
    actions->addWidget(pause);
    actions->addWidget(more);
    actions->addStretch();
    cl->addLayout(actions);
    QHBoxLayout *stops = new QHBoxLayout;
    stops->addWidget(save);
    stops->addWidget(shutdown);
    stops->addStretch();
    cl->addLayout(stops);
    dl->addWidget(card, 1);
    dl->addWidget(epLabel(epText("Shared clipboard and folders are off until you enable them in Settings.",
                                 "共享剪贴板和文件夹默认关闭，可在设置中启用。"),
                          "EpMuted"));
    ll->addWidget(details, 1);
    empty = epLabel(epText("A fresh workspace for your next idea.\nCreate your first Linux computer to get started.",
                           "为新想法留一片空间。\n创建第一台 Linux 虚拟电脑，开始使用。"),
                    "EpName");
    empty->setAlignment(Qt::AlignCenter);
    ll->addWidget(empty, 1);
    pages->addWidget(library);
    QWidget *restore = new QWidget;
    QVBoxLayout *rl = new QVBoxLayout(restore);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->addWidget(epLabel(epText("Save a moment before a big change. Restoring replaces the current state.",
                                 "在重要改动前保存一个状态。恢复会替换当前状态。"),
                          "EpMuted"));
    snapshots = new QTreeWidget;
    snapshots->setObjectName("ElseportSnapshotList");
    snapshots->setHeaderHidden(true);
    rl->addWidget(snapshots, 1);
    QHBoxLayout *sr = new QHBoxLayout;
    snapshotNew = epButton(epText("New restore point", "创建恢复点"), "ElseportSnapshotNew", true);
    snapshotRestore = epButton(epText("Restore selected", "恢复所选状态"), "ElseportSnapshotRestore");
    sr->addWidget(snapshotNew);
    sr->addWidget(snapshotRestore);
    sr->addStretch();
    rl->addLayout(sr);
    pages->addWidget(restore);
    QWidget *settings = new QWidget;
    QVBoxLayout *sv = new QVBoxLayout(settings);
    sv->setContentsMargins(0, 0, 0, 0);
    settingsStatus = epLabel(QString(), "EpMuted");
    sv->addWidget(settingsStatus);
    settingsForm = new QWidget;
    QFormLayout *sf = new QFormLayout(settingsForm);
    sf->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    sf->setSpacing(18);
    memory = new QDoubleSpinBox;
    memory->setObjectName("ElseportMemory");
    memory->setDecimals(2);
    memory->setSuffix(" GB");
    memory->setRange(.125, 128);
    memory->setSingleStep(1);
    cpus = new QSpinBox;
    cpus->setObjectName("ElseportCPUs");
    cpus->setRange(1, 64);
    clipboard = new QComboBox;
    clipboard->setObjectName("ElseportClipboard");
    clipboard->addItems({epText("Disabled", "关闭"), epText("Mac to computer", "Mac → 虚拟电脑"),
                         epText("Computer to Mac", "虚拟电脑 → Mac"), epText("Both directions", "双向")});
    files = new QCheckBox(epText("Allow file transfers", "允许文件传输"));
    files->setObjectName("ElseportClipboardFiles");
    files->setToolTip(
        epText("File clipboard requires an X11 desktop in this preview.", "本预览版的文件剪贴板需要客体使用 X11 桌面。"));
    share = new QCheckBox(epText("Share one selected folder", "共享一个指定文件夹"));
    share->setObjectName("ElseportShareFolder");
    folder = new QLineEdit;
    folder->setObjectName("ElseportFolderPath");
    QPushButton *chooseFolder = epButton(epText("Choose…", "选择…"), "ElseportChooseFolder");
    QHBoxLayout *fr = new QHBoxLayout;
    fr->addWidget(folder);
    fr->addWidget(chooseFolder);
    writable = new QCheckBox(epText("Allow changes to files in this folder", "允许修改共享目录中的文件"));
    scale = new QComboBox;
    scale->setObjectName("ElseportDisplayScale");
    scale->addItems({epText("Automatic · follow display", "自动 · 跟随显示器"),
                     epText("Original pixels · 100%", "原始像素 · 100%"), "200%"});
    sf->addRow(epText("Memory", "内存"), memory);
    sf->addRow(epText("Processors", "处理器"), cpus);
    sf->addRow(epText("Clipboard", "剪贴板"), clipboard);
    sf->addRow(QString(), files);
    sf->addRow(epText("Shared folder", "共享目录"), share);
    sf->addRow(QString(), fr);
    sf->addRow(QString(), writable);
    sf->addRow(epText("Display size", "显示大小"), scale);
    sv->addWidget(settingsForm);
    sv->addWidget(epLabel(epText("Sharing lets the guest read selected host data. Enable it only for computers you trust. This "
                                 "preview uses software graphics.",
                                 "共享允许客体读取选定的宿主数据，仅对可信的电脑启用。本预览版使用软件图形。"),
                          "EpMuted"));
    sv->addStretch();
    QHBoxLayout *sb = new QHBoxLayout;
    settingsSave = epButton(epText("Save settings", "保存设置"), "ElseportSettingsSave", true);
    QPushButton *advanced = epButton(epText("Advanced settings…", "高级设置…"), "ElseportAdvancedSettings");
    sb->addWidget(settingsSave);
    sb->addWidget(advanced);
    sb->addStretch();
    sv->addLayout(sb);
    pages->addWidget(settings);
    QWidget *about = new QWidget;
    QVBoxLayout *al = new QVBoxLayout(about);
    al->setContentsMargins(20, 20, 20, 20);
    al->addWidget(epLabel("Elseport 0.1.0 Preview", "EpTitle"));
    al->addWidget(
        epLabel(epText("A native workspace for Linux on your Mac.", "在 Mac 上运行 Linux 的原生工作空间。"), "EpName"));
    al->addWidget(epLabel(epText("Independent software built from the open-source code of VirtualBox. Not affiliated with, "
                                 "endorsed by or sponsored by Oracle.\n\nCovered code is available under GNU GPL version 3. "
                                 "This software comes without warranty. Third-party components retain their own licenses.",
                                 "基于 VirtualBox 开源代码的独立项目，与 Oracle 没有关联、认可或赞助关系。\n\n受覆盖的代码使用 "
                                 "GNU GPL 第三版许可证。本软件不提供担保，第三方组件保留各自许可证。"),
                          "EpMuted"));
    QPushButton *source = epButton(epText("Source code and help", "源码与帮助"), "ElseportSource");
    QPushButton *licenses = epButton(epText("Licenses", "许可证"), "ElseportLicenses");
    al->addWidget(source, 0, Qt::AlignLeft);
    al->addWidget(licenses, 0, Qt::AlignLeft);
    al->addStretch();
    pages->addWidget(about);
    QWidget *form = new QWidget;
    QVBoxLayout *fl = new QVBoxLayout(form);
    fl->setContentsMargins(5, 0, 5, 0);
    fl->addWidget(epLabel(epText("Download Ubuntu from its official site, or choose a Linux Arm64 image you already have.",
                                 "从官方网站下载 Ubuntu，也可以选择已有的 Linux Arm64 镜像。"),
                          "EpMuted"));
    QFormLayout *fields = new QFormLayout;
    fields->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    fields->setSpacing(13);
    newName = new QLineEdit("Ubuntu");
    newName->setObjectName("ElseportCreateName");
    newUser = new QLineEdit("user");
    newUser->setObjectName("ElseportCreateUser");
    newPassword = new QLineEdit;
    newPassword->setObjectName("ElseportCreatePassword");
    newPassword->setEchoMode(QLineEdit::Password);
    newPassword->setPlaceholderText(epText("At least 8 characters", "至少 8 个字符"));
    image = new QLineEdit;
    image->setObjectName("ElseportISO");
    image->setPlaceholderText(epText("Leave empty to download Ubuntu", "留空以下载 Ubuntu"));
    QPushButton *chooseISO = epButton(epText("Choose image…", "选择镜像…"), "ElseportChooseISO");
    QHBoxLayout *isoRow = new QHBoxLayout;
    isoRow->addWidget(image);
    isoRow->addWidget(chooseISO);
    automatic = new QCheckBox(epText("Install Ubuntu automatically", "自动安装 Ubuntu"));
    automatic->setObjectName("ElseportAutomaticInstall");
    automatic->setChecked(true);
    newMemory = new QDoubleSpinBox;
    newMemory->setObjectName("ElseportCreateMemory");
    newMemory->setRange(2, 128);
    newMemory->setValue(6);
    newMemory->setSuffix(" GB");
    newCpus = new QSpinBox;
    newCpus->setObjectName("ElseportCreateCPUs");
    newCpus->setRange(1, 64);
    newCpus->setValue(4);
    newDisk = new QSpinBox;
    newDisk->setObjectName("ElseportCreateDisk");
    newDisk->setRange(25, 2048);
    newDisk->setValue(64);
    newDisk->setSuffix(" GB");
    fields->addRow(epText("Computer name", "电脑名称"), newName);
    fields->addRow(epText("Linux image", "Linux 镜像"), isoRow);
    fields->addRow(QString(), automatic);
    fields->addRow(epText("Username", "使用者名称"), newUser);
    fields->addRow(epText("Computer password", "电脑密码"), newPassword);
    fields->addRow(epText("Memory", "内存"), newMemory);
    fields->addRow(epText("Processors", "处理器"), newCpus);
    fields->addRow(epText("Disk limit", "磁盘上限"), newDisk);
    fl->addLayout(fields);
    createStatus = epLabel(QString(), "EpMuted");
    fl->addWidget(createStatus);
    fl->addStretch();
    create = epButton(epText("Create and install", "创建并安装"), "ElseportCreate", true);
    fl->addWidget(create, 0, Qt::AlignRight);
    pages->addWidget(form);
    QWidget *task = new QWidget;
    QVBoxLayout *tl = new QVBoxLayout(task);
    tl->setContentsMargins(40, 60, 40, 40);
    taskLabel = epLabel(QString(), "EpName");
    taskLabel->setAlignment(Qt::AlignCenter);
    progressBar = new QProgressBar;
    progressBar->setObjectName("ElseportProgress");
    cancel = epButton(epText("Cancel", "取消"), "ElseportCancel");
    tl->addStretch();
    tl->addWidget(taskLabel);
    tl->addWidget(progressBar);
    tl->addWidget(cancel, 0, Qt::AlignCenter);
    tl->addStretch();
    pages->addWidget(task);
    root->addWidget(body, 1);
    QObject::connect(newButton, &QPushButton::clicked, q, [this] { q->createComputer(); });
    QObject::connect(navComputers, &QPushButton::clicked, q, [this] { showPage(0); });
    QObject::connect(navSnapshots, &QPushButton::clicked, q,
                     [this]
                     {
                         showPage(1);
                         loadSnapshots();
                     });
    QObject::connect(navSettings, &QPushButton::clicked, q,
                     [this]
                     {
                         showPage(2);
                         loadSettings();
                     });
    QObject::connect(navAbout, &QPushButton::clicked, q, [this] { showPage(3); });
    QObject::connect(machines, &QListWidget::currentRowChanged, q, [this] { select(); });
    QObject::connect(start, &QPushButton::clicked, q, [this] { startMachine(); });
    QObject::connect(pause, &QPushButton::clicked, q, [this] { control("pause"); });
    QObject::connect(save, &QPushButton::clicked, q, [this] { control("save"); });
    QObject::connect(shutdown, &QPushButton::clicked, q, [this] { control("shutdown"); });
    QObject::connect(
        more, &QPushButton::clicked, q,
        [this]
        {
            QMenu menu(q);
            QAction *config = menu.addAction(epText("Settings", "设置"));
            QAction *showFolder = menu.addAction(epText("Show files", "显示文件"));
            menu.addSeparator();
            QAction *power = menu.addAction(epText("Force power off…", "强制关闭…"));
            CMachine current = machine();
            power->setEnabled(!current.isNull() &&
                              (current.GetState() == KMachineState_Running || current.GetState() == KMachineState_Paused));
            QAction *picked = menu.exec(more->mapToGlobal(QPoint(0, more->height())));
            if (picked == config)
            {
                showPage(2);
                loadSettings();
            }
            else if (picked == showFolder)
            {
                CMachine m = machine();
                if (!m.isNull())
                    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m.GetSettingsFilePath()).absolutePath()));
            }
            else if (picked == power)
                control("poweroff");
        });
    QObject::connect(settingsSave, &QPushButton::clicked, q, [this] { saveSettings(); });
    QObject::connect(clipboard, qOverload<int>(&QComboBox::currentIndexChanged), q,
                     [this](int index)
                     {
                         files->setEnabled(index != 0);
                         if (index == 0)
                             files->setChecked(false);
                     });
    files->setEnabled(clipboard->currentIndex() != 0);
    QObject::connect(advanced, &QPushButton::clicked, q,
                     [this]
                     {
                         if (!selected.isNull())
                             emit q->sigAdvancedSettings(selected);
                     });
    QObject::connect(chooseFolder, &QPushButton::clicked, q,
                     [this]
                     {
                         const QString p = QFileDialog::getExistingDirectory(
                             q, epText("Choose a folder to share", "选择共享目录"), folder->text());
                         if (!p.isEmpty())
                             folder->setText(p);
                     });
    QObject::connect(source, &QPushButton::clicked, q,
                     [] { QDesktopServices::openUrl(QUrl("https://github.com/gaochangw/Elseport")); });
    QObject::connect(licenses, &QPushButton::clicked, q,
                     []
                     {
                         const QString local =
                             QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../Resources/Licenses/NOTICE");
                         QDesktopServices::openUrl(QFileInfo::exists(local)
                                                       ? QUrl::fromLocalFile(local)
                                                       : QUrl("https://github.com/gaochangw/Elseport/blob/main/NOTICE"));
                     });
    QObject::connect(snapshotNew, &QPushButton::clicked, q, [this] { takeSnapshot(); });
    QObject::connect(snapshotRestore, &QPushButton::clicked, q, [this] { restoreSnapshot(); });
    QObject::connect(create, &QPushButton::clicked, q, [this] { createMachine(); });
    QObject::connect(cancel, &QPushButton::clicked, q,
                     [this]
                     {
                         if (worker)
                         {
                             worker->cancel();
                             cancel->setEnabled(false);
                             taskLabel->setText(epText("Stopping safely…", "正在安全停止…"));
                         }
                         else if (!progress.isNull() && progress.GetCancelable())
                             progress.Cancel();
                     });
    QObject::connect(chooseISO, &QPushButton::clicked, q,
                     [this]
                     {
                         const QString p = QFileDialog::getOpenFileName(
                             q, epText("Choose a Linux Arm64 image", "选择 Linux Arm64 镜像"), image->text(), "ISO (*.iso)");
                         if (!p.isEmpty())
                         {
                             image->setText(p);
                             automatic->setChecked(false);
                         }
                     });
    for (QLineEdit *edit : {newName, newUser, newPassword, image})
        QObject::connect(edit, &QLineEdit::textChanged, q, [this] { validateForm(); });
    QObject::connect(automatic, &QCheckBox::toggled, q, [this] { validateForm(); });
    QObject::connect(language, qOverload<int>(&QComboBox::currentIndexChanged), q,
                     [this](int i)
                     {
                         if (loading)
                             return;
                         if (gEDataManager->setLanguageId(language->itemData(i).toString(), q))
                         {
                             QMessageBox::information(q, "Elseport",
                                                      epText("Language saved. Reopen Elseport to update all controls.",
                                                             "语言已保存。重新打开 Elseport 后所有控件将切换语言。"));
                             emit q->sigLanguageChanged();
                         }
                     });
    timer = new QTimer(q);
    QObject::connect(timer, &QTimer::timeout, q,
                     [this]
                     {
                         tickProgress();
                         refresh();
                     });
    timer->start(2000);
    showPage(0);
    refresh(true);
}

CMachine UIElseportData::machine() const
{
    return selected.isNull() ? CMachine() : gpGlobalSession->virtualBox().FindMachine(selected.toString());
}

void UIElseportData::showPage(int page)
{
    if (busy && page != 5)
        return;
    pages->setCurrentIndex(page);
    const QStringList titles = {epText("Your computers", "你的虚拟电脑"),
                                epText("Restore points", "恢复点"),
                                epText("Computer settings", "电脑设置"),
                                epText("About Elseport", "关于 Elseport"),
                                epText("Start with a fresh space", "从一个新空间开始"),
                                epText("Getting your computer ready", "正在准备你的电脑")};
    heading->setText(titles.value(page));
    newButton->setVisible(page < 4);
    navComputers->setChecked(page == 0 || page == 4 || page == 5);
    navSnapshots->setChecked(page == 1);
    navSettings->setChecked(page == 2);
    navAbout->setChecked(page == 3);
    subheading->setText(page == 0
                            ? epText("A quiet place to pick up your work.", "一个安静的地方，继续你的工作。")
                            : epText("Linux workspace · Files stay on your Mac", "Linux 工作空间 · 文件保存在这台 Mac 上"));
}

void UIElseportData::refresh(bool reload)
{
    if (busy)
        return;
    const CVirtualBox vbox = gpGlobalSession->virtualBox();
    const QVector<CMachine> all = vbox.GetMachines();
    QStringList ids;
    QVector<CMachine> linuxMachines;
    for (CMachine m : all)
        if (m.GetAccessible() && epLinux(m.GetOSTypeId()))
        {
            ids << m.GetId().toString();
            linuxMachines << m;
        }
    bool rebuild = machines->count() != ids.size();
    for (int i = 0; !rebuild && i < ids.size(); ++i)
        rebuild = machines->item(i)->data(Qt::UserRole).toString() != ids[i];
    if (rebuild)
    {
        const QSignalBlocker blocker(machines);
        machines->clear();
        for (int i = 0; i < linuxMachines.size(); ++i)
        {
            QListWidgetItem *item = new QListWidgetItem(machines);
            item->setData(Qt::UserRole, ids[i]);
            item->setIcon(QIcon(":/elseport/mark.svg"));
        }
        int row = ids.indexOf(selected.toString());
        machines->setCurrentRow(row >= 0 ? row : (ids.isEmpty() ? -1 : 0));
        selected = machines->currentItem() ? QUuid(machines->currentItem()->data(Qt::UserRole).toString()) : QUuid();
    }
    for (int i = 0; i < linuxMachines.size(); ++i)
        machines->item(i)->setText(linuxMachines[i].GetName() + "\n" + epState(linuxMachines[i].GetState()));
    const bool exists = !selected.isNull();
    details->setVisible(exists);
    empty->setVisible(!exists);
    machines->setVisible(exists);
    snapshotNew->setEnabled(exists);
    snapshotRestore->setEnabled(false);
    CMachine m = machine();
    if (!m.isNull())
    {
        const KMachineState current = m.GetState();
        const bool installing = m.GetExtraData("Elseport/InstallPending") == "true";
        snapshotNew->setEnabled(!installing);
        snapshotRestore->setEnabled(!installing && (epOffline(current) || current == KMachineState_Saved) &&
                                    snapshots->topLevelItemCount() > 0);
        snapshotNew->setToolTip(installing ? epText("Available after Ubuntu setup finishes.", "Ubuntu 安装完成后可用。")
                                           : QString());
        computerName->setText(m.GetName());
        state->setText(epState(current));
        qulonglong cbDisk = 0;
        for (CMediumAttachment attachment : m.GetMediumAttachments())
            if (attachment.GetType() == KDeviceType_HardDisk && !attachment.GetMedium().isNull())
                cbDisk += attachment.GetMedium().GetLogicalSize();
        resources->setText(
            epText("Memory: %1 GB   ·   Processors: %2   ·   Disk: %3 GB", "内存：%1 GB   ·   处理器：%2   ·   磁盘：%3 GB")
                .arg(QString::number(m.GetMemorySize() / 1024.0, 'g', 3))
                .arg(m.GetCPUCount())
                .arg(cbDisk / (1024 * 1024 * 1024)));
        start->setText(current == KMachineState_Saved ? epText("Pick up where you left off", "继续使用")
                       : (current == KMachineState_Running || current == KMachineState_Paused)
                           ? epText("Open window", "打开窗口")
                           : epText("Start computer", "启动电脑"));
        start->setEnabled(epOffline(current) || current == KMachineState_Saved || current == KMachineState_Running ||
                          current == KMachineState_Paused);
        pause->setText(current == KMachineState_Paused ? epText("Resume", "继续") : epText("Pause", "暂停"));
        pause->setEnabled(current == KMachineState_Running || current == KMachineState_Paused);
        save->setEnabled(current == KMachineState_Running || current == KMachineState_Paused);
        shutdown->setEnabled(current == KMachineState_Running);
        updatePreview(m);
        checkInstall(m);
        if (reload)
            loadSettings();
    }
}

void UIElseportData::select()
{
    if (loading)
        return;
    selected = machines->currentItem() ? QUuid(machines->currentItem()->data(Qt::UserRole).toString()) : QUuid();
    preview->clear();
    refresh(true);
    if (pages->currentIndex() == 1)
        loadSnapshots();
}

void UIElseportData::updatePreview(CMachine &m)
{
    QImage image;
    if (m.GetState() == KMachineState_Saved)
    {
        ULONG w = 0, h = 0;
        const QVector<BYTE> bytes = m.ReadSavedScreenshotToArray(0, KBitmapFormat_PNG, w, h);
        if (!bytes.isEmpty())
            image.loadFromData(reinterpret_cast<const uchar *>(bytes.constData()), bytes.size(), "PNG");
    }
    else if (m.GetState() == KMachineState_Running || m.GetState() == KMachineState_Paused)
    {
        CSession lock = gpGlobalSession->virtualBoxClient().GetSession();
        CMachine previewMachine = m;
        previewMachine.LockMachine(lock, KLockType_Shared);
        if (previewMachine.isOk())
        {
            CConsole console = lock.GetConsole();
            if (!console.isNull())
            {
                CDisplay display = console.GetDisplay();
                if (!display.isNull())
                {
                    const QVector<BYTE> bytes = display.TakeScreenShotToArray(0, 1280, 800, KBitmapFormat_PNG);
                    if (!bytes.isEmpty())
                        image.loadFromData(reinterpret_cast<const uchar *>(bytes.constData()), bytes.size(), "PNG");
                }
            }
            lock.UnlockMachine();
        }
    }
    if (!image.isNull())
    {
        const qreal ratio = q->devicePixelRatioF();
        QPixmap pixels =
            QPixmap::fromImage(image).scaled(preview->size() * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pixels.setDevicePixelRatio(ratio);
        preview->setPixmap(pixels);
    }
    else if (m.GetState() != KMachineState_Running && m.GetState() != KMachineState_Paused)
    {
        preview->setPixmap(QPixmap());
        preview->setText(epText("Your Linux workspace\nStart this computer to see its desktop.",
                                "你的 Linux 工作空间\n启动电脑后，这里会显示真实桌面。"));
    }
}

void UIElseportData::checkInstall(CMachine &m)
{
    if (m.GetExtraData("Elseport/InstallPending") != "true")
        return;
    const QString version = m.GetGuestPropertyValue("/VirtualBox/GuestAdd/Version");
    const QString ready = m.GetGuestPropertyValue("/Elseport/InstallComplete");
    if (version.isEmpty() || ready != m.GetId().toString(QUuid::WithoutBraces))
    {
        state->setText(epText("Ubuntu installation in progress · open its window to follow progress",
                              "Ubuntu 正在安装 · 打开窗口可查看实际进度"));
        return;
    }
    uiElseportCleanupInstallation(m);
}

void UIElseportData::startMachine()
{
    CMachine m = machine();
    if (m.isNull())
        return;
    if (m.GetState() == KMachineState_Running || m.GetState() == KMachineState_Paused)
        UILocalMachineStuff::switchToMachine(m);
    else
    {
        CSession lock = gpGlobalSession->virtualBoxClient().GetSession();
        CProgress started = m.LaunchVMProcess(lock, "gui", QVector<QString>());
        if (!m.isOk())
        {
            error(epText("Could not start this computer.", "无法启动这台电脑。"), UIErrorString::formatErrorInfo(m));
            return;
        }
        beginProgress(epText("Starting this computer…", "正在启动电脑…"), started, lock);
    }
}

void UIElseportData::beginProgress(const QString &caption, const CProgress &value, const CSession &lock)
{
    if (value.isNull())
    {
        if (!lock.isNull())
        {
            CSession release = lock;
            release.UnlockMachine();
        }
        error(epText("The operation could not start.", "无法开始操作。"));
        return;
    }
    progress = value;
    session = lock;
    operation = caption;
    busy = true;
    showPage(5);
    taskLabel->setText(caption);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    cancel->setEnabled(value.GetCancelable());
    navComputers->setEnabled(false);
    navSnapshots->setEnabled(false);
    navSettings->setEnabled(false);
    newButton->setEnabled(false);
}
void UIElseportData::tickProgress()
{
    if (progress.isNull())
        return;
    progressBar->setValue(progress.GetPercent());
    if (!progress.GetCompleted())
        return;
    const LONG result = progress.GetResultCode();
    const QString errorDetail = result == 0 ? QString() : UIErrorString::formatErrorInfo(progress);
    progress = CProgress();
    if (!session.isNull())
    {
        session.UnlockMachine();
        session = CSession();
    }
    busy = false;
    navComputers->setEnabled(true);
    navSnapshots->setEnabled(true);
    navSettings->setEnabled(true);
    newButton->setEnabled(true);
    showPage(0);
    refresh(true);
    if (closeWhenIdle)
        QTimer::singleShot(0, q->window(), &QWidget::close);
    else if (result != 0)
        error(
            epText("The operation did not finish. Your existing computer remains available.", "操作未完成，现有电脑仍然保留。"),
            errorDetail);
}
void UIElseportData::control(const QString &action)
{
    CMachine m = machine();
    if (m.isNull() || busy)
        return;
    if (action == "poweroff" &&
        QMessageBox::warning(q, "Elseport",
                             epText("Force power off? Unsaved work inside this computer will be lost.",
                                    "强制关闭？这台电脑中尚未保存的工作会丢失。"),
                             QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    CSession lock = UILocalMachineStuff::openExistingSession(m.GetId(), q);
    if (lock.isNull())
        return;
    CConsole console = lock.GetConsole();
    if (action == "pause")
    {
        if (m.GetState() == KMachineState_Paused)
            console.Resume();
        else
            console.Pause();
        if (!console.isOk())
            error(epText("Could not change the running state.", "无法改变运行状态。"), UIErrorString::formatErrorInfo(console));
        lock.UnlockMachine();
        refresh();
    }
    else if (action == "shutdown")
    {
        console.PowerButton();
        if (!console.isOk())
            error(epText("Could not request shutdown.", "无法请求正常关机。"), UIErrorString::formatErrorInfo(console));
        lock.UnlockMachine();
        refresh();
    }
    else if (action == "save")
    {
        if (m.GetState() == KMachineState_Running)
            console.Pause();
        if (!console.isOk())
        {
            lock.UnlockMachine();
            error(epText("Could not pause this computer for saving.", "无法暂停电脑以保存状态。"));
            return;
        }
        beginProgress(epText("Saving this computer…", "正在保存电脑…"), lock.GetMachine().SaveState(), lock);
    }
    else if (action == "poweroff")
        beginProgress(epText("Powering off…", "正在关闭…"), console.PowerDown(), lock);
    else
        lock.UnlockMachine();
}

void UIElseportData::loadSettings()
{
    CMachine m = machine();
    const bool valid = !m.isNull();
    settingsForm->setEnabled(valid && epOffline(m.GetState()));
    settingsSave->setEnabled(valid && epOffline(m.GetState()));
    settingsStatus->setText(!valid ? epText("Create or select a Linux computer first.", "请先创建或选择一台 Linux 电脑。")
                            : epOffline(m.GetState()) ? m.GetName()
                                                      : epText("Shut down this computer before changing its configuration.",
                                                               "更改配置前，请先正常关闭这台电脑。"));
    if (!valid)
        return;
    CHost host = gpGlobalSession->virtualBox().GetHost();
    memory->setMaximum(qMax(1.0, host.GetMemorySize() / 1024.0 - 2.0));
    cpus->setMaximum(qMax((ULONG)1, host.GetProcessorOnlineCount()));
    memory->setValue(m.GetMemorySize() / 1024.0);
    cpus->setValue(m.GetCPUCount());
    const KClipboardMode mode = m.GetClipboard().GetMode();
    clipboard->setCurrentIndex(mode == KClipboardMode_HostToGuest     ? 1
                               : mode == KClipboardMode_GuestToHost   ? 2
                               : mode == KClipboardMode_Bidirectional ? 3
                                                                      : 0);
    files->setChecked(m.GetClipboard().GetFileTransfersEnabled());
    const QString value = m.GetExtraData("GUI/ScaleFactor");
    scale->setCurrentIndex(value.isEmpty() ? 0 : value.startsWith("2") ? 2 : 1);
    share->setChecked(false);
    folder->clear();
    writable->setChecked(false);
    for (CSharedFolder f : m.GetSharedFolders())
        if (f.GetName() == "Shared")
        {
            share->setChecked(true);
            folder->setText(f.GetHostPath());
            writable->setChecked(f.GetWritable());
        }
}
void UIElseportData::saveSettings()
{
    CMachine m = machine();
    if (m.isNull() || !epOffline(m.GetState()))
        return;
    if (share->isChecked() && !QFileInfo(folder->text()).isDir())
    {
        error(epText("Choose an existing folder to share.", "请选择实际存在的共享目录。"));
        return;
    }
    CSession lock = UILocalMachineStuff::openSession(selected, KLockType_Write, q);
    if (lock.isNull())
        return;
    CMachine live = lock.GetMachine();
    bool ok = true;
    QString detail;
    const auto check = [&](const auto &object)
    {
        if (!object.isOk())
        {
            if (detail.isEmpty())
                detail = UIErrorString::formatErrorInfo(object);
            ok = false;
        }
    };
    live.SetMemorySize((ULONG)qRound(memory->value() * 1024));
    check(live);
    if (ok)
    {
        live.SetCPUCount(cpus->value());
        check(live);
    }
    CClipboardSettings clip = live.GetClipboard();
    check(live);
    const KClipboardMode modes[] = {KClipboardMode_Disabled, KClipboardMode_HostToGuest, KClipboardMode_GuestToHost,
                                    KClipboardMode_Bidirectional};
    if (ok)
    {
        clip.SetMode(modes[clipboard->currentIndex()]);
        check(clip);
    }
    if (ok)
    {
        clip.SetFileTransfersEnabled(files->isChecked());
        check(clip);
    }
    if (ok)
    {
        live.SetExtraData("GUI/ScaleFactor", scale->currentIndex() == 0 ? QString() : scale->currentIndex() == 1 ? "1" : "2");
        check(live);
    }
    if (ok)
    {
        for (CSharedFolder f : live.GetSharedFolders())
            if (f.GetName() == "Shared")
            {
                live.RemoveSharedFolder("Shared");
                check(live);
                break;
            }
    }
    if (ok && share->isChecked())
    {
        live.CreateSharedFolder("Shared", QFileInfo(folder->text()).canonicalFilePath(), writable->isChecked(), true,
                                QString());
        check(live);
    }
    if (ok)
    {
        live.SaveSettings();
        check(live);
    }
    if (!ok)
        live.DiscardSettings();
    lock.UnlockMachine();
    if (ok)
        settingsStatus->setText(epText("Settings saved.", "设置已保存。"));
    else
        error(epText("Could not save these settings.", "无法保存这些设置。"), detail);
    refresh();
}

static void epAddSnapshots(QTreeWidget *tree, QTreeWidgetItem *parent, const CSnapshot &snapshot)
{
    if (snapshot.isNull())
        return;
    QTreeWidgetItem *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
    item->setText(0, snapshot.GetName());
    item->setData(0, Qt::UserRole, snapshot.GetId().toString());
    item->setToolTip(0, QLocale().toString(QDateTime::fromMSecsSinceEpoch(snapshot.GetTimeStamp()), QLocale::ShortFormat));
    for (CSnapshot child : snapshot.GetChildren())
        epAddSnapshots(tree, item, child);
}
void UIElseportData::loadSnapshots()
{
    snapshots->clear();
    CMachine m = machine();
    if (m.isNull())
        return;
    subheading->setText(m.GetName());
    if (m.GetSnapshotCount())
        epAddSnapshots(snapshots, 0, m.FindSnapshot(QString()));
    snapshots->expandAll();
    snapshotRestore->setEnabled(m.GetExtraData("Elseport/InstallPending") != "true" &&
                                (epOffline(m.GetState()) || m.GetState() == KMachineState_Saved) &&
                                snapshots->topLevelItemCount() > 0);
}
void UIElseportData::takeSnapshot()
{
    CMachine m = machine();
    if (m.isNull() || busy || m.GetExtraData("Elseport/InstallPending") == "true")
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(q, epText("New restore point", "创建恢复点"), epText("Name", "名称"),
                                               QLineEdit::Normal, epText("Before a change", "改动之前"), &ok)
                             .trimmed();
    if (!ok || name.isEmpty())
        return;
    const bool running = m.GetState() == KMachineState_Running || m.GetState() == KMachineState_Paused;
    CSession lock = running ? UILocalMachineStuff::openExistingSession(selected, q)
                            : UILocalMachineStuff::openSession(selected, KLockType_Write, q);
    if (lock.isNull())
        return;
    QUuid id;
    beginProgress(epText("Creating a restore point…", "正在创建恢复点…"),
                  lock.GetMachine().TakeSnapshot(name, QString(), true, id), lock);
}
void UIElseportData::restoreSnapshot()
{
    CMachine m = machine();
    QTreeWidgetItem *item = snapshots->currentItem();
    if (m.isNull() || !item || (!epOffline(m.GetState()) && m.GetState() != KMachineState_Saved) || busy)
        return;
    QMessageBox confirmation(QMessageBox::Warning, epText("Restore this point?", "恢复到这个状态？"),
                             epText("Restore %1 to “%2”? Changes after this point will be replaced. "
                                    "Save any files you want to keep first.",
                                    "将 %1 恢复到“%2”？此后的改动将被替换，请先保存需要保留的文件。")
                                 .arg(m.GetName(), item->text(0)),
                             QMessageBox::Yes | QMessageBox::Cancel, q);
    confirmation.setTextFormat(Qt::PlainText);
    confirmation.setDefaultButton(QMessageBox::Cancel);
    if (confirmation.exec() != QMessageBox::Yes)
        return;
    CSnapshot snapshot = m.FindSnapshot(item->data(0, Qt::UserRole).toString());
    CSession lock = UILocalMachineStuff::openSession(selected, KLockType_Write, q);
    if (lock.isNull())
        return;
    CMachine live = lock.GetMachine();
    if (live.GetState() == KMachineState_Saved)
        live.DiscardSavedState(true);
    if (!live.isOk())
    {
        lock.UnlockMachine();
        error(epText("Could not discard the saved state before restoring.", "无法在恢复前移除已保存的运行状态。"));
        return;
    }
    beginProgress(epText("Restoring this computer…", "正在恢复电脑…"), live.RestoreSnapshot(snapshot), lock);
}

void UIElseportData::validateForm()
{
    const QString name = newName->text().trimmed();
    QString problem;
    if (name.isEmpty() || name == "." || name == ".." || name.size() > 80 ||
        name.contains(QRegularExpression("[\\x00-\\x1f/\\\\:]")))
        problem = epText("Enter a name without path separators.", "请输入不含路径分隔符的名称。");
    else if (!image->text().isEmpty() && !QFileInfo(image->text()).isFile())
        problem = epText("Choose an existing Linux ISO file.", "请选择实际存在的 Linux ISO 文件。");
    else if (automatic->isChecked() && !QRegularExpression("^[a-z_][a-z0-9_-]{0,30}$").match(newUser->text()).hasMatch())
        problem = epText("Use lowercase letters and digits for the username.", "使用者名称请使用小写字母和数字。");
    else if (automatic->isChecked() && newPassword->text().size() < 8)
        problem = epText("Use a password of at least 8 characters.", "密码至少需要 8 个字符。");
    else if (!automatic->isChecked() && image->text().isEmpty())
        problem = epText("Choose an image for manual installation.", "手动安装需要选择镜像文件。");
    newUser->setEnabled(automatic->isChecked());
    newPassword->setEnabled(automatic->isChecked());
    createStatus->setText(problem);
    create->setEnabled(problem.isEmpty());
}
void UIElseportData::createMachine()
{
    validateForm();
    if (!create->isEnabled() || busy)
        return;
    CVirtualBox vbox = gpGlobalSession->virtualBox();
    const QString root = vbox.GetHomeFolder();
    const QStorageInfo diskSpace(root);
    const qint64 required = automatic->isChecked() ? Q_INT64_C(12) * 1024 * 1024 * 1024 : Q_INT64_C(1) * 1024 * 1024 * 1024;
    if (diskSpace.isValid() && diskSpace.bytesAvailable() >= 0 && diskSpace.bytesAvailable() < required)
    {
        error(epText("There is not enough free space to prepare this computer. Free some space and try again.",
                     "没有足够的可用空间来准备这台电脑，请先腾出空间再重试。"));
        return;
    }
    const QString additions =
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../Resources/ElseportLinuxAdditions.iso");
    UIElseportInstallOptions options;
    options.strName = newName->text().trimmed();
    options.strUser = newUser->text();
    options.strPassword = newPassword->text();
    options.strImage = image->text().trimmed();
    options.strCache = QDir(root).filePath("Cache");
    options.strAdditions = additions;
    options.cMemoryMB = qRound(newMemory->value() * 1024);
    options.cCpus = newCpus->value();
    options.cDiskGB = newDisk->value();
    options.fAutomatic = automatic->isChecked();
    options.fChinese = epChinese();
    newPassword->clear();
    worker = new UIElseportInstallWorker(options, q);
    busy = true;
    showPage(5);
    progressBar->setRange(0, 0);
    cancel->setEnabled(true);
    taskLabel->setText(epText("Preparing installation…", "正在准备安装…"));
    navComputers->setEnabled(false);
    navSnapshots->setEnabled(false);
    navSettings->setEnabled(false);
    newButton->setEnabled(false);
    QObject::connect(worker, &UIElseportInstallWorker::sigStep, q,
                     [this](int step)
                     {
                         const QStringList stages = {epText("Downloading Ubuntu…", "正在下载 Ubuntu…"),
                                                     epText("Verifying the image…", "正在校验镜像…"),
                                                     epText("Creating your computer…", "正在创建电脑…"),
                                                     epText("Preparing automatic installation…", "正在准备自动安装…")};
                         operation = stages.value(step);
                         taskLabel->setText(operation);
                         progressBar->setRange(0, 0);
                     });
    QObject::connect(worker, &UIElseportInstallWorker::sigProgress, q,
                     [this](qint64 done, qint64 total, qint64)
                     {
                         if (total > 0)
                         {
                             progressBar->setRange(0, 100);
                             progressBar->setValue((int)(done * 100 / total));
                         }
                         taskLabel->setText(operation + QString("\n%1 / %2 GB")
                                                            .arg(done / (1024.0 * 1024 * 1024), 0, 'f', 2)
                                                            .arg(total / (1024.0 * 1024 * 1024), 0, 'f', 2));
                     });
    QObject::connect(worker, &QThread::finished, q,
                     [this]
                     {
                         UIElseportInstallWorker *finished = worker;
                         worker = 0;
                         busy = false;
                         navComputers->setEnabled(true);
                         navSnapshots->setEnabled(true);
                         navSettings->setEnabled(true);
                         newButton->setEnabled(true);
                         const QUuid id = finished->machineId();
                         const bool cancelled = finished->isCancelled();
                         const QString message = finished->error();
                         finished->deleteLater();
                         if (!id.isNull())
                         {
                             selected = id;
                             showPage(0);
                             refresh(true);
                             if (!cancelled && !closeWhenIdle)
                                 startMachine();
                         }
                         else
                         {
                             showPage(4);
                             validateForm();
                             if (!cancelled && !closeWhenIdle)
                                 error(message.isEmpty() ? epText("Could not prepare this computer.", "无法准备这台电脑。")
                                                         : message);
                         }
                         if (closeWhenIdle)
                             QTimer::singleShot(0, q->window(), &QWidget::close);
                     });
    worker->start();
}

UIElseportPane::UIElseportPane(QWidget *parent) : QWidget(parent), m_pData(new UIElseportData(this))
{
    m_pData->build();
}
UIElseportPane::~UIElseportPane()
{
    if (m_pData->worker)
    {
        m_pData->worker->cancel();
        m_pData->worker->wait();
    }
    delete m_pData;
}
void UIElseportPane::createComputer(const QString &strImage)
{
    if (m_pData->busy)
        return;
    const CHost host = gpGlobalSession->virtualBox().GetHost();
    const uint32_t memoryMB = host.GetMemorySize();
    const uint32_t cpus = host.GetProcessorOnlineCoreCount();
    m_pData->newMemory->setMaximum(qMax(2.0, memoryMB / 1024.0 - 2.0));
    m_pData->newCpus->setMaximum(qMax((uint32_t)1, cpus));
    if (!strImage.isEmpty())
    {
        m_pData->image->setText(strImage);
        m_pData->automatic->setChecked(false);
    }
    m_pData->showPage(4);
    m_pData->validateForm();
}
void UIElseportPane::showAbout()
{
    m_pData->showPage(3);
}
void UIElseportPane::refresh()
{
    m_pData->refresh();
}
bool UIElseportPane::canClose()
{
    if (!m_pData->busy)
        return true;
    if (!m_pData->worker && (m_pData->progress.isNull() || !m_pData->progress.GetCancelable()))
    {
        QMessageBox::information(this, "Elseport",
                                 epText("Please wait for the current operation to finish.", "请等待当前操作完成。"));
        return false;
    }
    if (QMessageBox::question(this, "Elseport",
                              epText("Work is still in progress. Cancel it before closing?", "操作仍在进行，取消后再关闭？"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        m_pData->closeWhenIdle = true;
        if (m_pData->worker)
            m_pData->worker->cancel();
        else if (!m_pData->progress.isNull() && m_pData->progress.GetCancelable())
            m_pData->progress.Cancel();
    }
    return false;
}
