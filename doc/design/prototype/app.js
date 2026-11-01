/* $Id$ */
/* Elseport - local interface simulation. No network, VM or clipboard access. */
/* SPDX-License-Identifier: GPL-3.0-only */
"use strict";

const elseportCopy = {
    zh: {
        review: "交互模拟 · 不连接真实虚拟电脑", caption: "画面、文件与进度均为示例。请勿输入真实密码。",
        screen: "预览画面", contrast: "高对比度", theme: "切换深色外观", themeLight: "切换浅色外观", language: "切换语言",
        welcome: "首次打开", library: "虚拟电脑", install: "创建电脑", download: "下载系统", verify: "校验文件",
        installing: "安装系统", ready: "安装完成", error: "下载中断", storage: "空间不足", settings: "设置",
        snapshots: "恢复点", running: "运行窗口", paused: "已暂停", saving: "保存进度", fullscreen: "全屏预览",
        seamless: "专注视图", about: "关于 Elseport", shutdown: "关闭电脑", local: "保存在这台 Mac 上",
        workspace: "工作空间", preferences: "偏好设置", newComputer: "创建虚拟电脑", back: "返回",
        welcomeTitle: "多一台电脑，\n多一些可能。", welcomeSub: "在 Mac 上为新的想法留一片空间。从安装系统到开始使用，让每一步都清楚。",
        welcomeOne: "几步完成安装", welcomeTwo: "自己的空间，自己的文件", welcomeThree: "随时保存，下次接着来",
        getStarted: "创建第一台虚拟电脑", useImage: "也可以使用已有的系统镜像", computersTitle: "你的虚拟电脑",
        computersSub: "一个安静的地方，继续你的工作。", workName: "Ubuntu · 工作", testName: "Ubuntu · 测试",
        savedState: "已保存", stoppedState: "已关闭", runningState: "正在运行", resume: "继续使用", start: "启动电脑",
        configure: "设置", more: "更多操作", memory: "内存", processor: "处理器", storageLabel: "磁盘", cores: "核",
        savedTime: "上次保存：今天 10:42", libraryHelp: "新增电脑的入口始终在这里。已有的电脑和文件不会受到影响。",
        today: "今日计划", noteText: "留点空间，做有意思的事。", documents: "文档", osDesktop: "桌面预览",
        installTitle: "从一个新空间开始", installSub: "选好系统，其余步骤由 Elseport 帮你完成。", recommended: "推荐系统",
        ubuntuSub: "Ubuntu 26.04.1 · 适用于这台 Mac", autoDownload: "自动下载并校验官方安装文件",
        computerName: "电脑名称", accountName: "使用者名称", password: "电脑密码", passwordHelp: "用于登录这台虚拟电脑。原型中仅填写示例。",
        passwordPlaceholder: "至少 8 个字符", suggested: "已为你选好", diskDetail: "按需使用空间", settingsLater: "这些设置都可以在安装后调整。",
        advanced: "更多选项", advancedHelp: "使用已有系统镜像、调整位置或分配资源。", useOwnIso: "使用自己的系统镜像",
        isoHint: "本原型不会读取文件。", createInstall: "创建并安装", formNote: "文件保存在本机。安装需要联网。",
        downloadTitle: "正在准备你的新电脑", downloadSub: "你可以继续使用 Mac。下载进度会自动保留。", downloadStep: "下载系统",
        verifyTitle: "确认安装文件完整", verifySub: "正在比对文件校验值，确认下载过程中没有损坏。", verifyStep: "校验文件",
        installingTitle: "正在安装 Ubuntu", installingSub: "系统设置正在自动完成。安装好后，这里会告诉你。", installingStep: "安装系统",
        prepareStep: "准备", downloadShort: "下载", verifyShort: "校验", installShort: "安装", fetched: "已下载", pauseDownload: "暂停下载",
        pausedDownload: "下载已暂停", continueDownload: "继续下载", closeHint: "关闭这个窗口，下次仍可从这里继续。",
        installingHint: "安装过程中请保持 Mac 开机。", cancelInstall: "取消安装", readyTitle: "你的新电脑，准备好了。",
        readySub: "Ubuntu 已安装完成。打开它，开始你的第一件事。", openComputer: "打开电脑", readyInfo: "已为这台电脑创建独立磁盘。",
        errorTitle: "下载停了一下", errorSub: "暂时无法连接下载服务器。已经下载的部分仍然保留。", retry: "继续下载",
        errorAdviceTitle: "检查网络后再试", errorAdvice: "确认 Mac 可以访问网络。如果当前网络限制下载，可以稍后继续。",
        storageTitle: "给新电脑留一点空间", storageSub: "完成安装前，还需要更多可用磁盘空间。你现有的文件没有改动。",
        freeSpace: "当前可用空间", neededSpace: "建议预留空间", storageAdvice: "可以在 Mac 上移走暂时不用的文件，再返回继续。",
        checkAgain: "重新检查", chooseLocation: "更换保存位置", commonSettings: "常用", sharing: "共享", display: "显示",
        settingsSub: "Ubuntu · 工作已关闭，可以安全调整配置。", resources: "电脑资源", memoryHelp: "为虚拟电脑分配的内存。",
        cpuHelp: "留一些处理器资源给 Mac 上的其他应用。", clipboard: "共享剪贴板", clipboardHelp: "开启后，两边的应用都可以读取你复制的文字和图片。",
        shareFolder: "共享文件夹", folderHelp: "仅共享你主动选择的文件夹。默认关闭。", displaySize: "显示大小",
        displayHelp: "选择看起来舒服的大小，无需记住像素或缩放比例。", displayComfort: "舒适 · 跟随屏幕", displaySharp: "清晰 · 原始像素",
        saveChanges: "保存设置", changesSaved: "设置已保存到原型", advancedSettings: "高级设置", advancedSettingsHelp: "启动顺序、网络和存储控制器会在实际产品中提供。",
        restoreTitle: "给尝试留一条回来的路", restoreSub: "恢复点保留当时的电脑状态。重要改动前，可以先存一份。",
        newRestore: "创建恢复点", restoreNotice: "恢复到一个确定的起点", restoreNoticeSub: "恢复会替换当前状态。请先保存现在想留下的文件。",
        currentState: "当前状态", currentSub: "你正在使用的工作空间", freshInstall: "系统安装完成", freshInstallSub: "今天 09:20 · 开始使用前",
        beforeUpdate: "更新前", beforeUpdateSub: "昨天 16:35 · 手动创建", restoreAction: "恢复", newRestoreName: "新的恢复点",
        backComputers: "返回虚拟电脑", pause: "暂停", continueRun: "继续", saveClose: "保存并关闭", fitWindow: "适合窗口",
        originalPixels: "原始像素", focusView: "专注视图", fullView: "全屏预览", normalView: "返回窗口", connected: "电脑正在运行",
        isolatedSharing: "共享功能已关闭", activeSharing: "共享功能已开启", savingTitle: "保存好，下次接着来。", savingSub: "正在记录当前状态，完成后会关闭虚拟电脑。",
        savingStep: "保存当前状态", savingHint: "请暂时不要退出 Elseport。", pauseHint: "虚拟电脑已暂停，点击继续即可恢复。",
        stopTitle: "这次先到这里？", stopSub: "保存当前状态，下次打开就能接着使用。也可以让系统正常关机。",
        cancel: "取消", normalShutdown: "正常关机", forceShutdown: "强制关闭…", forceTitle: "要强制关闭吗？",
        forceSub: "没有保存的内容可能丢失。仅在系统没有响应时使用。", forceConfirm: "强制关闭", restoreConfirmTitle: "恢复到这个恢复点？",
        restoreConfirmSub: "当前状态会被替换。还没保存的文件改动可能丢失。", restoreConfirm: "恢复此状态",
        prototypeVersion: "界面设计预览 01", tagline: "一台电脑，更多可能。", source: "Elseport 是基于 VirtualBox 开源代码的独立项目，与 Oracle 无关联。",
        copyright: "上游代码 © 2004–2026 Oracle 和/或其关联公司及其他贡献者。",
        license: "本项目采用 GNU 通用公共许可证第三版。你可以按许可证复制、修改和再分发本软件。",
        warranty: "在适用法律允许的范围内，本软件不提供担保。", viewLicense: "查看许可证", thirdParty: "第三方声明",
        fontNotice: "字标使用 Manrope 字体，按 SIL Open Font License 1.1 许可。界面图形为本项目自行绘制。",
        trademark: "Oracle 和 VirtualBox 是 Oracle 和/或其关联公司的商标。Ubuntu 是 Canonical 的商标。",
        demoOnly: "当前仅为设计模拟，未连接任何虚拟电脑。", restoreCreated: "已在原型中创建恢复点", closed: "电脑已关闭",
        locationDemo: "保存位置选择仅作原型展示", stopInstall: "取消这次安装？", stopInstallSub: "下载文件会保留，下次无需从头开始。",
        stopInstallConfirm: "取消安装", pausedOverlay: "已暂停", saveFoot: "状态保存在本机", fieldError: "请填写名称、使用者名称和至少 8 个字符的示例密码。"
    },
    en: {
        review: "Interactive preview · No real computers connected", caption: "Screens, files and progress are examples. Do not enter a real password.",
        screen: "Preview screen", contrast: "High contrast", theme: "Switch to dark appearance", themeLight: "Switch to light appearance", language: "Switch language",
        welcome: "First launch", library: "Computers", install: "Create computer", download: "Download", verify: "Verify files",
        installing: "Install system", ready: "Ready", error: "Download interrupted", storage: "Storage needed", settings: "Settings",
        snapshots: "Restore points", running: "Running window", paused: "Paused", saving: "Saving state", fullscreen: "Full-screen preview",
        seamless: "Focus view", about: "About Elseport", shutdown: "Close computer", local: "Stored on this Mac",
        workspace: "WORKSPACE", preferences: "PREFERENCES", newComputer: "New computer", back: "Back",
        welcomeTitle: "Another computer.\nMore possibilities.", welcomeSub: "Make room for a new idea on your Mac. From installing a system to getting started, every step is clear.",
        welcomeOne: "Set up in a few simple steps", welcomeTwo: "Your own space for your own files", welcomeThree: "Save now. Pick up where you left off.",
        getStarted: "Create your first computer", useImage: "Or use a system image you already have", computersTitle: "Your computers",
        computersSub: "A quiet place to pick up your work.", workName: "Ubuntu · Work", testName: "Ubuntu · Testing",
        savedState: "Saved", stoppedState: "Off", runningState: "Running", resume: "Pick up where you left off", start: "Start computer",
        configure: "Settings", more: "More options", memory: "Memory", processor: "Processors", storageLabel: "Disk", cores: "cores",
        savedTime: "Last saved today at 10:42", libraryHelp: "There is always room for another computer. Your existing computers and files stay as they are.",
        today: "A little space for today", noteText: "Make something worth making.", documents: "Documents", osDesktop: "Desktop preview",
        installTitle: "Start with a fresh space", installSub: "Choose a system. Elseport will take care of the setup.", recommended: "RECOMMENDED SYSTEM",
        ubuntuSub: "Ubuntu 26.04.1 · Made for this Mac", autoDownload: "Downloads and verifies the official system image",
        computerName: "Computer name", accountName: "Username", password: "Computer password", passwordHelp: "For signing in to this computer. Use an example in this preview.",
        passwordPlaceholder: "At least 8 characters", suggested: "Ready-to-use settings", diskDetail: "Uses space as needed", settingsLater: "You can change these settings after installation.",
        advanced: "More options", advancedHelp: "Use an existing image, choose a location or adjust resources.", useOwnIso: "Use your own system image",
        isoHint: "This preview does not read files.", createInstall: "Create and install", formNote: "Files stay on your Mac. Setup needs an internet connection.",
        downloadTitle: "Getting your new computer ready", downloadSub: "You can keep using your Mac. Download progress is saved automatically.", downloadStep: "Downloading the system",
        verifyTitle: "Making sure every file is intact", verifySub: "Checking the downloaded file against its checksum before installation.", verifyStep: "Verifying the download",
        installingTitle: "Installing Ubuntu", installingSub: "Your system is being set up automatically. We will let you know when it is ready.", installingStep: "Installing the system",
        prepareStep: "Prepare", downloadShort: "Download", verifyShort: "Verify", installShort: "Install", fetched: "Downloaded", pauseDownload: "Pause download",
        pausedDownload: "Download paused", continueDownload: "Continue download", closeHint: "Close this window and continue from here next time.",
        installingHint: "Keep your Mac on during installation.", cancelInstall: "Cancel installation", readyTitle: "Your new computer is ready.",
        readySub: "Ubuntu is installed. Open it and make a start.", openComputer: "Open computer", readyInfo: "This computer has its own separate disk.",
        errorTitle: "The download needs a moment", errorSub: "We cannot reach the download server right now. Everything downloaded so far is still here.", retry: "Continue download",
        errorAdviceTitle: "Check your connection and try again", errorAdvice: "Make sure your Mac can get online. If this network limits downloads, you can continue later.",
        storageTitle: "Make a little room", storageSub: "Your new computer needs more free disk space to finish installing. Your existing files have not changed.",
        freeSpace: "Space available", neededSpace: "Suggested free space", storageAdvice: "Move files you do not need right now, then come back to continue.",
        checkAgain: "Check again", chooseLocation: "Choose another location", commonSettings: "General", sharing: "Sharing", display: "Display",
        settingsSub: "Ubuntu · Work is off, so it is safe to change these settings.", resources: "COMPUTER RESOURCES", memoryHelp: "Memory set aside for this computer.",
        cpuHelp: "Leave some processing power for the other apps on your Mac.", clipboard: "Share clipboard", clipboardHelp: "Apps on both computers can read the text and images you copy.",
        shareFolder: "Share a folder", folderHelp: "Only folders you choose are shared. Off by default.", displaySize: "Display size",
        displayHelp: "Choose what feels comfortable. No need to think in pixels or percentages.", displayComfort: "Comfortable · Follow display", displaySharp: "Sharp · Original pixels",
        saveChanges: "Save settings", changesSaved: "Settings saved in the preview", advancedSettings: "Advanced settings", advancedSettingsHelp: "Boot order, networking and storage controllers will be available in the product.",
        restoreTitle: "A way back, when you need one", restoreSub: "Restore points keep a moment in your computer's history. Save one before a big change.",
        newRestore: "New restore point", restoreNotice: "Return to a known starting point", restoreNoticeSub: "Restoring replaces the current state. Save any files you want to keep first.",
        currentState: "Current state", currentSub: "The workspace you are using now", freshInstall: "Fresh installation", freshInstallSub: "Today, 09:20 · Before getting started",
        beforeUpdate: "Before the update", beforeUpdateSub: "Yesterday, 16:35 · Created manually", restoreAction: "Restore", newRestoreName: "New restore point",
        backComputers: "Back to computers", pause: "Pause", continueRun: "Continue", saveClose: "Save and close", fitWindow: "Fit to window",
        originalPixels: "Original pixels", focusView: "Focus view", fullView: "Full-screen preview", normalView: "Back to window", connected: "Computer is running",
        isolatedSharing: "Sharing is off", activeSharing: "Sharing is on", savingTitle: "Saved for your next good idea.", savingSub: "Keeping the current state before closing your computer.",
        savingStep: "Saving the current state", savingHint: "Keep Elseport open for a moment.", pauseHint: "This computer is paused. Continue whenever you are ready.",
        stopTitle: "Call it a day?", stopSub: "Save the current state and pick up right here next time. You can also shut down the system normally.",
        cancel: "Cancel", normalShutdown: "Shut down normally", forceShutdown: "Force off…", forceTitle: "Force this computer off?",
        forceSub: "Unsaved work may be lost. Only do this if the system is not responding.", forceConfirm: "Force off", restoreConfirmTitle: "Return to this restore point?",
        restoreConfirmSub: "This will replace the current state. Unsaved file changes may be lost.", restoreConfirm: "Restore this state",
        prototypeVersion: "Interface preview 01", tagline: "One computer. More possibilities.", source: "Elseport is an independent project built from the open-source code of VirtualBox. It is not affiliated with Oracle.",
        copyright: "Upstream code © 2004–2026 Oracle and/or its affiliates and other contributors.",
        license: "This project is licensed under the GNU General Public License, version 3. You may copy, modify and redistribute this software under that license.",
        warranty: "To the extent permitted by applicable law, this software comes without warranty.", viewLicense: "View license", thirdParty: "Third-party notices",
        fontNotice: "The wordmark uses Manrope under the SIL Open Font License 1.1. Interface graphics are drawn for this project.",
        trademark: "Oracle and VirtualBox are trademarks of Oracle and/or its affiliates. Ubuntu is a trademark of Canonical.",
        demoOnly: "This is a design simulation. No real computer is connected.", restoreCreated: "Restore point created in the preview", closed: "Computer is off",
        locationDemo: "Location selection is simulated in this preview", stopInstall: "Cancel this installation?", stopInstallSub: "Downloaded files will be kept, so you do not have to start again.",
        stopInstallConfirm: "Cancel installation", pausedOverlay: "Paused", saveFoot: "State is stored on this Mac", fieldError: "Enter a name, username and an example password with at least 8 characters."
    }
};

const elseportPaths = {
    computer: '<rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8M12 17v4"/>',
    play: '<path d="m9 5 11 7-11 7Z"/>', plus: '<path d="M12 5v14M5 12h14"/>',
    pause: '<path d="M8 5v14M16 5v14"/>', settings: '<path d="M4 7h16M4 17h16"/><circle cx="9" cy="7" r="2.5" fill="var(--surface)"/><circle cx="15" cy="17" r="2.5" fill="var(--surface)"/>',
    check: '<path d="m5 12 4 4L19 6"/>', back: '<path d="m14 5-7 7 7 7"/>',
    folder: '<path d="M3 7V5a1 1 0 0 1 1-1h5l2 3h9a1 1 0 0 1 1 1v11a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1Z"/>',
    file: '<path d="M6 3h8l4 4v14H6Z M14 3v5h4M9 12h6M9 16h6"/>',
    sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2M5 5l1.5 1.5M17.5 17.5 19 19M5 19l1.5-1.5M17.5 6.5 19 5"/>',
    moon: '<path d="M20 15.5A8.5 8.5 0 0 1 8.5 4 8.5 8.5 0 1 0 20 15.5Z"/>',
    globe: '<circle cx="12" cy="12" r="9"/><ellipse cx="12" cy="12" rx="4" ry="9"/><path d="M3 12h18"/>',
    arrow: '<path d="M4 12h15m-6-6 6 6-6 6"/>',
    spark: '<path d="m12 3 2.5 6.5L21 12l-6.5 2.5L12 21l-2.5-6.5L3 12l6.5-2.5Z"/>',
    shield: '<path d="m12 3 8 3v6c0 5-8 9-8 9s-8-4-8-9V6Z"/><path d="m8 12 3 3 5-6"/>',
    download: '<path d="M12 3v12m-5-5 5 5 5-5M4 16v5h16v-5"/>',
    restore: '<path d="M4 8a9 9 0 1 1-1 7M4 3v5h5M12 7v5l3 2"/>',
    power: '<path d="M12 2v10M6 5a9 9 0 1 0 12 0"/>',
    expand: '<path d="M3 9V3h6M15 3h6v6M21 15v6h-6M9 21H3v-6"/>',
    focus: '<rect x="3" y="5" width="18" height="14" rx="3"/><path d="M3 10h18"/>',
    help: '<circle cx="12" cy="12" r="9"/><path d="M9.2 9a3 3 0 1 1 4 2.8c-1 .4-1.2 1-1.2 2.2M12 17h.01"/>',
    close: '<path d="m6 6 12 12M18 6 6 18"/>',
    dots: '<circle cx="5" cy="12" r="1"/><circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/>',
    disk: '<path d="M5 3h12l3 3v15H4V3Z M8 3v6h8V3M8 21v-8h8v8"/>',
    lock: '<rect x="5" y="10" width="14" height="11" rx="2"/><path d="M8 10V7a4 4 0 0 1 8 0v3"/>',
    timer: '<circle cx="12" cy="13" r="8"/><path d="M9 2h6M12 5V2M12 9v5l3 2"/>',
    alert: '<path d="m12 3 10 18H2Z M12 9v5M12 17h.01"/>',
    network: '<path d="M4 9a12 12 0 0 1 16 0M7 12a8 8 0 0 1 10 0M10 15a3 3 0 0 1 4 0M12 18h.01"/>'
};
const elseportViews = ['welcome', 'library', 'install', 'download', 'verify', 'installing', 'ready', 'error', 'storage',
    'settings', 'snapshots', 'running', 'paused', 'saving', 'fullscreen', 'seamless', 'shutdown', 'about'];
const elseportQuery = new URLSearchParams(location.search);
const elseportState = {
    view: elseportViews.includes(elseportQuery.get('view')) ? elseportQuery.get('view') : 'library',
    locale: elseportQuery.get('lang') === 'en' ? 'en' : 'zh',
    theme: elseportQuery.get('theme') === 'dark' ? 'dark' : 'light',
    contrast: elseportQuery.get('contrast') === 'high',
    modal: '', tab: 'commonSettings', memory: 6, cpus: 4, clipboard: false, folder: false,
    selected: 0, power: 'saved', pausedDownload: false, progress: 54, restoreCount: 0, customName: ''
};
let elseportTimer = null;

function elseportText(key) { return elseportCopy[elseportState.locale][key] || key; }
function elseportEscape(value) {
    return String(value).replace(/[&<>"']/g, character => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[character]));
}
function elseportIcon(name) { return `<svg class="icon" viewBox="0 0 24 24" aria-hidden="true">${elseportPaths[name] || elseportPaths.computer}</svg>`; }
function elseportButton(label, action, style = '', icon = '') {
    return `<button type="button" class="button ${style}" data-action="${action}">${icon ? elseportIcon(icon) : ''}${elseportText(label)}</button>`;
}
function elseportIconButton(label, action, icon) {
    return `<button type="button" class="icon-button" data-action="${action}" aria-label="${elseportText(label)}" title="${elseportText(label)}">${elseportIcon(icon)}</button>`;
}
function elseportComputerName() { return elseportState.customName || elseportText(elseportState.selected ? 'testName' : 'workName'); }
function elseportBrand() { return `<div class="brand-lockup"><img src="assets/mark.svg" alt=""><span>Elseport</span></div>`; }
function elseportTitlebar() {
    return `<div class="titlebar"><div class="traffic" aria-hidden="true"><i></i><i></i><i></i></div><span class="window-title">Elseport</span><div class="title-end">${elseportIcon('lock')}</div></div>`;
}
function elseportNav() {
    const active = elseportState.view === 'snapshots' ? 'snapshots' : elseportState.view === 'settings' ? 'settings' : elseportState.view === 'about' ? 'about' : 'library';
    const item = (key, icon, count = '') => `<button class="nav-item ${key === active ? 'active' : ''}" data-action="view:${key}" ${key === active ? 'aria-current="page"' : ''}>${elseportIcon(icon)}<span>${elseportText(key)}</span>${count ? `<span class="nav-count">${count}</span>` : ''}</button>`;
    return `<aside class="sidebar">${elseportBrand()}<div class="nav-label">${elseportText('workspace')}</div>${item('library', 'computer', '2')}${item('snapshots', 'restore')}<div class="nav-label">${elseportText('preferences')}</div>${item('settings', 'settings')}<div class="sidebar-bottom">${item('about', 'help')}<div class="sidebar-foot"><i class="dot"></i>${elseportText('local')}</div></div></aside>`;
}
function elseportHead(title, sub = '', action = '') {
    return `<header class="page-head"><div><h1 tabindex="-1">${elseportText(title)}</h1>${sub ? `<p class="subheading">${elseportText(sub)}</p>` : ''}</div>${action}</header>`;
}
function elseportDesktop(paused = false) {
    return `<div class="desktop-scene" role="img" aria-label="${elseportText('osDesktop')}"><div class="scene-top"><span>Ubuntu</span><span>${elseportIcon('network')} ${elseportIcon('power')}</span></div><div class="scene-note"><div class="scene-note-bar"><span>${elseportText('documents')}</span><span>— &nbsp; ◻ &nbsp; ×</span></div><h3>${elseportText('today')}</h3><p>${elseportText('noteText')}</p><div class="note-line"></div><div class="note-line short"></div><div class="note-line"></div></div><div class="scene-dock"><i>${elseportIcon('folder')}</i><i>${elseportIcon('globe')}</i><i>${elseportIcon('file')}</i></div>${paused ? `<div class="scene-overlay"><span>${elseportIcon('pause')}${elseportText('pausedOverlay')}</span></div>` : ''}</div>`;
}
function elseportWelcome() {
    return `<section class="welcome"><div class="welcome-art" aria-hidden="true"><div class="portal-orbit outer"></div><div class="portal-orbit"></div><img class="hero-app-icon" src="assets/app-icon.svg" alt=""><div class="mini-bubble one">${elseportIcon('computer')}</div><div class="mini-bubble two">${elseportIcon('spark')}</div></div><div class="welcome-copy">${elseportBrand()}<h1 tabindex="-1">${elseportText('welcomeTitle').split('\n').join('<br>')}</h1><p class="subheading">${elseportText('welcomeSub')}</p><div class="welcome-list"><div>${elseportIcon('download')}${elseportText('welcomeOne')}</div><div>${elseportIcon('shield')}${elseportText('welcomeTwo')}</div><div>${elseportIcon('restore')}${elseportText('welcomeThree')}</div></div>${elseportButton('getStarted', 'view:install', 'primary', 'plus')}<button type="button" class="button quiet welcome-secondary" data-action="own-image">${elseportText('useImage')}</button></div></section>`;
}
function elseportLibrary() {
    const state = elseportState.selected ? 'stoppedState' : elseportState.power === 'off' ? 'stoppedState' : 'savedState';
    const tile = (name, index) => `<button class="computer-tile ${elseportState.selected === index ? 'selected' : ''}" data-action="computer:${index}" aria-pressed="${elseportState.selected === index}"><div class="workspace-icon ${index ? 'secondary' : ''}">${elseportIcon('computer')}</div><div><strong>${elseportText(name)}</strong><span>Ubuntu 26.04.1</span></div></button>`;
    return `${elseportHead('computersTitle', 'computersSub', elseportButton('newComputer', 'view:install', '', 'plus'))}<div class="library-layout"><div class="computer-list">${tile('workName', 0)}${tile('testName', 1)}</div><div><article class="details-card">${elseportDesktop()}<div class="details-main"><div class="details-title"><div><h2>${elseportEscape(elseportComputerName())}</h2><span class="tag ${state === 'savedState' ? 'saved' : ''}"><i class="dot"></i>${elseportText(state)}</span></div>${elseportIconButton('more', 'shutdown', 'dots')}</div><div class="stats"><div><div class="stat-label">${elseportText('memory')}</div><div class="stat-value">${elseportState.memory} GB</div></div><div><div class="stat-label">${elseportText('processor')}</div><div class="stat-value">${elseportState.cpus} ${elseportText('cores')}</div></div><div><div class="stat-label">${elseportText('storageLabel')}</div><div class="stat-value">64 GB</div></div></div><div class="details-actions">${elseportButton(state === 'savedState' ? 'resume' : 'start', 'view:running', 'primary', 'play')}${elseportButton('configure', 'view:settings', '', 'settings')}</div></div></article><p class="last-saved">${elseportIcon('restore')}${elseportText('savedTime')}</p></div></div><div class="help-strip">${elseportIcon('plus')}${elseportText('libraryHelp')}</div>`;
}
function elseportInstall() {
    return `${elseportHead('installTitle', 'installSub')}<form id="install-form"><div class="form-layout"><div><div class="eyebrow">${elseportText('recommended')}</div><div class="os-choice"><div class="workspace-icon">${elseportIcon('computer')}</div><div><h3>Ubuntu</h3><p>${elseportText('ubuntuSub')}</p><p>${elseportText('autoDownload')}</p></div><span class="choice-tick">${elseportIcon('check')}</span></div><div class="field"><label for="computer-name">${elseportText('computerName')}</label><input id="computer-name" class="text-input" value="${elseportEscape(elseportComputerName())}" maxlength="60" required></div><div class="form-row"><div class="field"><label for="account-name">${elseportText('accountName')}</label><input id="account-name" class="text-input" value="demo" autocomplete="off" pattern="[a-z]([a-z0-9_]|-)*" required></div><div class="field"><label for="account-password">${elseportText('password')}</label><input id="account-password" type="password" class="text-input" value="example-only" autocomplete="off" minlength="8" placeholder="${elseportText('passwordPlaceholder')}" required></div></div><p class="field-help">${elseportText('passwordHelp')}</p><details id="install-options" style="margin-top:20px"><summary>${elseportText('advanced')}</summary><div class="advanced-body"><p>${elseportText('advancedHelp')}</p>${elseportButton('useOwnIso', 'own-image', 'quiet', 'folder')}<p class="small muted">${elseportText('isoHint')}</p></div></details></div><aside class="setup-summary"><h3>${elseportText('suggested')}</h3><div class="summary-row"><span>${elseportText('memory')}</span><strong>6 GB</strong></div><div class="summary-row"><span>${elseportText('processor')}</span><strong>4 ${elseportText('cores')}</strong></div><div class="summary-row"><span>${elseportText('storageLabel')}</span><strong>64 GB</strong></div><p class="small muted">${elseportText('diskDetail')}</p><p class="summary-note">${elseportText('settingsLater')}</p></aside></div><div class="form-footer"><p class="small muted">${elseportText('formNote')}</p><button class="button primary" type="submit">${elseportText('createInstall')}${elseportIcon('arrow')}</button></div></form>`;
}
function elseportSteps(active) {
    return `<div class="steps">${['prepareStep', 'downloadShort', 'verifyShort', 'installShort'].map((key, i) => `<div class="step ${i < active ? 'done' : i === active ? 'current' : ''}"><span class="step-index">${i < active ? elseportIcon('check') : i + 1}</span>${elseportText(key)}</div>`).join('')}</div>`;
}
function elseportProgress() {
    const view = elseportState.view;
    const save = view === 'saving';
    const active = view === 'download' ? 1 : view === 'verify' ? 2 : 3;
    const title = save ? 'savingTitle' : view + 'Title';
    const sub = save ? 'savingSub' : view + 'Sub';
    const progress = view === 'download' ? elseportState.progress : view === 'verify' ? 73 : save ? 68 : 46;
    const label = save ? 'savingStep' : elseportState.pausedDownload && view === 'download' ? 'pausedDownload' : view + 'Step';
    return `<section class="progress-page"><div class="progress-symbol">${elseportIcon(save ? 'disk' : view === 'download' ? 'download' : view === 'verify' ? 'shield' : 'computer')}</div><h1 tabindex="-1">${elseportText(title)}</h1><p class="subheading">${elseportText(sub)}</p><div class="progress-card"><div class="progress-label"><span>${elseportText(label)}</span><span>${progress}%</span></div><div class="progress-track" role="progressbar" aria-label="${elseportText(label)}" aria-valuenow="${progress}" aria-valuemin="0" aria-valuemax="100"><span style="width:${progress}%"></span></div><div class="progress-detail"><span>${view === 'download' ? `${elseportText('fetched')} ${(3.87 * progress / 100).toFixed(2)} / 3.87 GB` : elseportText(save ? 'saveFoot' : 'ubuntuSub')}</span><span>${save ? '' : 'Ubuntu'}</span></div>${save ? '' : elseportSteps(active)}</div>${save ? '' : `<div class="progress-actions">${view === 'download' ? elseportButton(elseportState.pausedDownload ? 'continueDownload' : 'pauseDownload', 'toggle-download', '', elseportState.pausedDownload ? 'play' : 'pause') : ''}${elseportButton('cancelInstall', 'cancel-install', 'quiet')}</div>`}<p class="progress-hint">${elseportText(save ? 'savingHint' : view === 'installing' ? 'installingHint' : 'closeHint')}</p></section>`;
}
function elseportReady() {
    return `<section class="progress-page success-page"><div class="progress-symbol">${elseportIcon('check')}</div><h1 tabindex="-1">${elseportText('readyTitle')}</h1><p class="subheading">${elseportText('readySub')}</p><div class="details-card" style="max-width:470px;margin:0 auto 27px">${elseportDesktop()}</div>${elseportButton('openComputer', 'view:running', 'primary', 'play')}<p class="progress-hint">${elseportText('readyInfo')}</p></section>`;
}
function elseportError() {
    const storage = elseportState.view === 'storage';
    return `<section class="progress-page failure"><div class="progress-symbol">${elseportIcon(storage ? 'disk' : 'network')}</div><h1 tabindex="-1">${elseportText(storage ? 'storageTitle' : 'errorTitle')}</h1><p class="subheading">${elseportText(storage ? 'storageSub' : 'errorSub')}</p>${storage ? `<div class="progress-card"><div class="summary-row"><span>${elseportText('freeSpace')}</span><strong>18 GB</strong></div><div class="summary-row"><span>${elseportText('neededSpace')}</span><strong>64 GB</strong></div></div><p class="progress-hint">${elseportText('storageAdvice')}</p>` : `<div class="notice">${elseportIcon('network')}<div><h3>${elseportText('errorAdviceTitle')}</h3><p>${elseportText('errorAdvice')}</p></div></div>`}<div class="progress-actions">${elseportButton(storage ? 'checkAgain' : 'retry', 'retry', 'primary', 'restore')}${elseportButton(storage ? 'chooseLocation' : 'backComputers', storage ? 'location' : 'view:library', 'quiet')}</div></section>`;
}
function elseportToggle(id, checked, label) {
    return `<label class="switch"><input id="${id}" type="checkbox" role="switch" aria-label="${elseportText(label)}" ${checked ? 'checked' : ''}></label>`;
}
function elseportSettings() {
    const text = elseportText;
    const row = (title, sub, control) => `<div class="setting-row"><div><div class="setting-name">${text(title)}</div><p class="setting-description">${text(sub)}</p></div>${control}</div>`;
    let content;
    if (elseportState.tab === 'sharing') {
        content = `<div class="settings-section"><div class="section-heading">${text('sharing')}</div>${row('clipboard', 'clipboardHelp', elseportToggle('share-clipboard', elseportState.clipboard, 'clipboard'))}${row('shareFolder', 'folderHelp', elseportToggle('share-folder', elseportState.folder, 'shareFolder'))}</div><div class="notice">${elseportIcon('shield')}<p>${text('folderHelp')}</p></div>`;
    } else if (elseportState.tab === 'display') {
        content = `<div class="settings-section"><div class="section-heading">${text('display')}</div>${row('displaySize', 'displayHelp', `<select class="display-select" aria-label="${text('displaySize')}"><option>${text('displayComfort')}</option><option>${text('displaySharp')}</option></select>`)}</div><div class="details-card">${elseportDesktop()}</div>`;
    } else {
        content = `<div class="settings-section"><div class="section-heading">${text('resources')}</div>${row('memory', 'memoryHelp', `<div class="slider-group"><div class="slider-label"><span>2 GB</span><strong class="setting-value" id="memory-value">${elseportState.memory} GB</strong><span>12 GB</span></div><input id="memory-range" type="range" min="2" max="12" value="${elseportState.memory}" aria-label="${text('memory')}" aria-valuetext="${elseportState.memory} GB"></div>`)}${row('processor', 'cpuHelp', `<div class="slider-group"><div class="slider-label"><span>1</span><strong class="setting-value" id="cpu-value">${elseportState.cpus} ${text('cores')}</strong><span>8</span></div><input id="cpu-range" type="range" min="1" max="8" value="${elseportState.cpus}" aria-label="${text('processor')}" aria-valuetext="${elseportState.cpus} ${text('cores')}"></div>`)}</div><details><summary>${text('advancedSettings')}</summary><div class="advanced-body">${text('advancedSettingsHelp')}</div></details>`;
    }
    return `${elseportHead('settings', 'settingsSub')}<div class="settings"><div class="tabs" role="tablist" aria-label="${text('settings')}">${['commonSettings', 'sharing', 'display'].map(key => `<button role="tab" aria-selected="${elseportState.tab === key}" class="${elseportState.tab === key ? 'selected' : ''}" data-action="tab:${key}">${text(key)}</button>`).join('')}</div><div role="tabpanel">${content}</div><div class="form-footer"><span></span>${elseportButton('saveChanges', 'save-settings', 'primary')}</div></div>`;
}
function elseportSnapshots() {
    const entry = (title, desc, current = false) => `<div class="timeline-entry"><i class="timeline-dot"></i><div class="restore-card"><div class="restore-copy"><h3>${elseportText(title)}</h3><p>${elseportText(desc)}</p></div>${current ? `<span class="tag saved">${elseportText('savedState')}</span>` : elseportButton('restoreAction', 'restore', '', 'restore')}</div></div>`;
    return `${elseportHead('restoreTitle', 'restoreSub', elseportButton('newRestore', 'new-restore', 'primary', 'plus'))}<div class="restore-intro">${elseportIcon('restore')}<div><h3>${elseportText('restoreNotice')}</h3><p>${elseportText('restoreNoticeSub')}</p></div></div><div class="timeline">${entry('currentState', 'currentSub', true)}${elseportState.restoreCount ? entry('newRestoreName', 'savedTime') : ''}${entry('beforeUpdate', 'beforeUpdateSub')}${entry('freshInstall', 'freshInstallSub')}</div>`;
}
function elseportRuntime() {
    const paused = elseportState.view === 'paused';
    const mode = elseportState.view === 'fullscreen' ? 'fullscreen' : elseportState.view === 'seamless' ? 'seamless' : '';
    return `<section class="runtime ${mode}"><div class="runtime-bar">${elseportIconButton('backComputers', 'view:library', 'back')}<span class="runtime-title">${elseportEscape(elseportComputerName())}</span>${elseportButton(mode ? 'normalView' : 'fitWindow', mode ? 'view:running' : 'resize-demo', 'quiet')}${elseportIconButton(paused ? 'continueRun' : 'pause', paused ? 'view:running' : 'view:paused', paused ? 'play' : 'pause')}${elseportIconButton('fullView', 'view:fullscreen', 'expand')}${elseportIconButton('focusView', 'view:seamless', 'focus')}${elseportButton('saveClose', 'shutdown', '', 'power')}</div>${elseportDesktop(paused)}<div class="runtime-foot"><span><i class="dot" style="color:var(--green)"></i>${elseportText(paused ? 'pauseHint' : 'connected')}</span><span>${elseportIcon('shield')}${elseportText(elseportState.clipboard || elseportState.folder ? 'activeSharing' : 'isolatedSharing')}</span></div></section>`;
}
function elseportAbout() {
    return `<section class="about"><img src="assets/app-icon.svg" alt="Elseport"><h1 tabindex="-1">Elseport</h1><p class="about-version">${elseportText('prototypeVersion')}</p><p class="tagline">${elseportText('tagline')}</p><div class="legal-copy"><p>${elseportText('source')}</p><p>${elseportText('copyright')}</p><p>${elseportText('license')}</p><p>${elseportText('warranty')}</p><p class="small">${elseportText('trademark')}</p></div><div class="about-links"><a class="button" href="assets/GPL-3.0.txt" target="_blank" rel="noopener">${elseportText('viewLicense')}</a>${elseportButton('thirdParty', 'third-party', 'quiet')}</div></section>`;
}
function elseportModal() {
    const kind = elseportState.modal || (elseportState.view === 'shutdown' ? 'shutdown' : '');
    if (!kind) { return ''; }
    const titles = {shutdown: 'stopTitle', force: 'forceTitle', restore: 'restoreConfirmTitle', cancelInstall: 'stopInstall', thirdParty: 'thirdParty'};
    const subs = {shutdown: 'stopSub', force: 'forceSub', restore: 'restoreConfirmSub', cancelInstall: 'stopInstallSub', thirdParty: 'fontNotice'};
    let actions = elseportButton('cancel', 'dismiss', 'quiet');
    if (kind === 'shutdown') { actions += elseportButton('saveClose', 'save-close', 'primary', 'disk'); }
    if (kind === 'force') { actions += elseportButton('forceConfirm', 'power-off', 'danger'); }
    if (kind === 'restore') { actions += elseportButton('restoreConfirm', 'confirm-restore', 'primary', 'restore'); }
    if (kind === 'cancelInstall') { actions += elseportButton('stopInstallConfirm', 'confirm-cancel', 'primary'); }
    return `<div class="overlay"><section class="dialog" role="dialog" aria-modal="true" aria-labelledby="dialog-title" aria-describedby="dialog-description"><div class="dialog-mark">${elseportIcon(kind === 'shutdown' ? 'power' : kind === 'force' ? 'alert' : 'restore')}</div><h2 id="dialog-title">${elseportText(titles[kind])}</h2><p id="dialog-description">${elseportText(subs[kind])}</p><div class="dialog-actions">${actions}</div>${kind === 'shutdown' ? `<div class="dialog-extra">${elseportButton('normalShutdown', 'power-off', 'quiet')}${elseportButton('forceShutdown', 'force', 'quiet')}</div>` : ''}</section></div>`;
}

function elseportRender(focus = false) {
    const state = elseportState;
    const text = elseportText;
    document.documentElement.dataset.theme = state.theme;
    document.documentElement.dataset.contrast = state.contrast ? 'high' : 'normal';
    document.documentElement.lang = state.locale === 'zh' ? 'zh-CN' : 'en';
    document.title = `Elseport · ${text(state.view)}`;
    document.getElementById('review-note').textContent = text('review');
    document.getElementById('review-caption').textContent = text('caption');
    document.getElementById('review-language').textContent = state.locale === 'zh' ? 'EN' : '中文';
    document.getElementById('review-language').setAttribute('aria-label', text('language'));
    document.getElementById('review-theme').innerHTML = elseportIcon(state.theme === 'light' ? 'moon' : 'sun');
    document.getElementById('review-theme').setAttribute('aria-label', text(state.theme === 'light' ? 'theme' : 'themeLight'));
    document.getElementById('review-contrast').textContent = text('contrast');
    document.getElementById('review-contrast').setAttribute('aria-pressed', String(state.contrast));
    const screen = document.getElementById('review-screen');
    screen.setAttribute('aria-label', text('screen'));
    screen.innerHTML = elseportViews.map(key => `<option value="${key}" ${key === state.view ? 'selected' : ''}>${text(key)}</option>`).join('');
    let body;
    if (state.view === 'welcome') { body = elseportWelcome(); }
    else if (['running', 'paused', 'fullscreen', 'seamless', 'shutdown'].includes(state.view)) { body = elseportRuntime(); }
    else {
        const content = state.view === 'library' ? elseportLibrary() : state.view === 'install' ? elseportInstall()
            : ['download', 'verify', 'installing', 'saving'].includes(state.view) ? elseportProgress()
            : state.view === 'ready' ? elseportReady() : ['error', 'storage'].includes(state.view) ? elseportError()
            : state.view === 'settings' ? elseportSettings() : state.view === 'snapshots' ? elseportSnapshots() : elseportAbout();
        body = `${elseportNav()}<section class="main">${content}</section>`;
    }
    document.getElementById('prototype').innerHTML = `${elseportTitlebar()}<div class="app-body">${body}</div>${elseportModal()}`;
    if (state.modal || state.view === 'shutdown') {
        document.querySelector('.dialog .button.primary, .dialog .button')?.focus();
    } else if (focus) { document.querySelector('#prototype h1')?.focus(); }
}
function elseportGo(view) {
    clearTimeout(elseportTimer);
    elseportState.view = view;
    elseportState.modal = '';
    elseportRender(true);
    document.getElementById('announcer').textContent = elseportText(view);
}
function elseportToast(key) {
    document.querySelector('.toast')?.remove();
    const toast = document.createElement('div');
    toast.className = 'toast'; toast.setAttribute('role', 'status'); toast.textContent = elseportText(key);
    document.getElementById('prototype').append(toast);
    setTimeout(() => toast.remove(), 3000);
}
function elseportInstallDemo() {
    if (elseportQuery.get('static') === '1' || elseportState.pausedDownload) { return; }
    elseportTimer = setTimeout(() => {
        if (elseportState.view === 'download') {
            elseportState.progress = Math.min(100, elseportState.progress + 11);
            if (elseportState.progress === 100) { elseportState.view = 'verify'; }
        } else if (elseportState.view === 'verify') { elseportState.view = 'installing'; }
        else if (elseportState.view === 'installing') { elseportGo('ready'); return; }
        else { return; }
        elseportRender(); elseportInstallDemo();
    }, 1400);
}

document.addEventListener('click', event => {
    const button = event.target.closest('[data-action]');
    if (!button) { return; }
    const action = button.dataset.action;
    if (action.startsWith('view:')) { elseportGo(action.slice(5)); return; }
    if (action.startsWith('computer:')) { elseportState.selected = Number(action.slice(9)); elseportRender(); return; }
    if (action.startsWith('tab:')) { elseportState.tab = action.slice(4); elseportRender(); document.querySelector('.tabs .selected')?.focus(); return; }
    if (action === 'save-settings') { elseportToast('changesSaved'); return; }
    if (action === 'new-restore') { elseportState.restoreCount++; elseportRender(); elseportToast('restoreCreated'); return; }
    if (action === 'own-image') { elseportGo('install'); document.getElementById('install-options').open = true; elseportToast('isoHint'); return; }
    if (action === 'location' || action === 'resize-demo') { elseportToast(action === 'location' ? 'locationDemo' : 'demoOnly'); return; }
    if (action === 'toggle-download') {
        elseportState.pausedDownload = !elseportState.pausedDownload;
        clearTimeout(elseportTimer); elseportRender(); elseportInstallDemo(); return;
    }
    if (action === 'retry') { elseportState.pausedDownload = false; elseportGo('download'); elseportInstallDemo(); return; }
    if (action === 'dismiss') {
        elseportState.modal = ''; if (elseportState.view === 'shutdown') { elseportState.view = 'running'; }
        elseportRender(); return;
    }
    if (action === 'save-close') {
        elseportGo('saving');
        if (elseportQuery.get('static') !== '1') { elseportTimer = setTimeout(() => { elseportState.power = 'saved'; elseportGo('library'); }, 2500); }
        return;
    }
    if (action === 'power-off') { elseportState.power = 'off'; elseportGo('library'); elseportToast('closed'); return; }
    if (action === 'confirm-restore') { elseportGo('library'); elseportToast('demoOnly'); return; }
    if (action === 'confirm-cancel') { elseportGo('library'); return; }
    const modals = {shutdown: 'shutdown', force: 'force', restore: 'restore', 'cancel-install': 'cancelInstall', 'third-party': 'thirdParty'};
    if (modals[action]) { elseportState.modal = modals[action]; elseportRender(); }
});
document.addEventListener('submit', event => {
    if (event.target.id !== 'install-form') { return; }
    event.preventDefault();
    // Only the display name is retained. No password is copied to state or storage.
    elseportState.customName = document.getElementById('computer-name').value.trim();
    elseportState.progress = 12; elseportState.pausedDownload = false;
    elseportGo('download'); elseportInstallDemo();
});
document.addEventListener('input', event => {
    if (event.target.id === 'memory-range') {
        elseportState.memory = Number(event.target.value);
        document.getElementById('memory-value').textContent = `${elseportState.memory} GB`;
        event.target.setAttribute('aria-valuetext', `${elseportState.memory} GB`);
    }
    if (event.target.id === 'cpu-range') {
        elseportState.cpus = Number(event.target.value);
        document.getElementById('cpu-value').textContent = `${elseportState.cpus} ${elseportText('cores')}`;
        event.target.setAttribute('aria-valuetext', `${elseportState.cpus} ${elseportText('cores')}`);
    }
});
document.addEventListener('change', event => {
    if (event.target.id === 'share-clipboard') { elseportState.clipboard = event.target.checked; }
    if (event.target.id === 'share-folder') { elseportState.folder = event.target.checked; }
});
document.getElementById('review-screen').addEventListener('change', event => elseportGo(event.target.value));
document.getElementById('review-language').addEventListener('click', () => { elseportState.locale = elseportState.locale === 'zh' ? 'en' : 'zh'; elseportRender(); });
document.getElementById('review-theme').addEventListener('click', () => { elseportState.theme = elseportState.theme === 'light' ? 'dark' : 'light'; elseportRender(); });
document.getElementById('review-contrast').addEventListener('click', () => { elseportState.contrast = !elseportState.contrast; elseportRender(); });
document.addEventListener('keydown', event => {
    const dialog = document.querySelector('.dialog');
    if (event.key === 'Escape' && dialog) { elseportState.modal = ''; if (elseportState.view === 'shutdown') { elseportState.view = 'running'; } elseportRender(); return; }
    if (event.key === 'Tab' && dialog) {
        const controls = Array.from(dialog.querySelectorAll('button, a[href]'));
        const first = controls[0], last = controls[controls.length - 1];
        if (event.shiftKey && document.activeElement === first) { event.preventDefault(); last.focus(); }
        if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first.focus(); }
    }
    if (event.key === 'ArrowRight' || event.key === 'ArrowLeft') {
        const tab = document.activeElement;
        if (tab?.getAttribute('role') !== 'tab') { return; }
        const tabs = Array.from(document.querySelectorAll('[role="tab"]'));
        const index = (tabs.indexOf(tab) + (event.key === 'ArrowRight' ? 1 : tabs.length - 1)) % tabs.length;
        event.preventDefault(); tabs[index].click();
    }
});

window.elseportPrototype = {
    views: elseportViews,
    state: elseportState,
    setView: elseportGo,
    render: elseportRender,
    setLocale: value => { elseportState.locale = value; elseportRender(); },
    setTheme: value => { elseportState.theme = value; elseportRender(); }
};
elseportRender();
