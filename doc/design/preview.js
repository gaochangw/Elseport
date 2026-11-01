/* $Id$ */
/* Elseport - language-specific light interface preview. */
/* SPDX-License-Identifier: GPL-3.0-only */
"use strict";

const previewLanguage = new URLSearchParams(location.search).get("lang") === "zh" ? "zh" : "en";
const previewCopy = {
    en: {
        eyebrow: "Brand and interface · Design preview 01",
        title: "One computer. More possibilities.",
        intro: "A new workspace, from setup to picking up where you left off.",
        appearance: "Light appearance",
        open: "Explore the interactive preview ↗",
        try: "Try the interface →",
        brand: "View the brand design",
        note: "Design simulation. No real virtual computers are connected.",
        spec: "Design specification",
        implementation: "Implementation notes",
        copy: "Interface copy",
        validation: "Validation notes",
        image: "Elseport in English, light appearance",
        icon: "Elseport app icon",
        pageTitle: "Elseport · Interface preview"
    },
    zh: {
        eyebrow: "品牌与界面 · 设计预览 01",
        title: "一台电脑，更多可能。",
        intro: "一处新的工作空间。从安装到继续使用，让下一步始终清楚。",
        appearance: "浅色外观",
        open: "查看交互原型 ↗",
        try: "体验界面原型 →",
        brand: "查看品牌设计",
        note: "这是设计模拟，不连接真实虚拟电脑。",
        spec: "设计规范",
        implementation: "实现方向",
        copy: "中英文文案",
        validation: "检查记录",
        image: "Elseport 中文浅色界面设计预览",
        icon: "Elseport 应用图标",
        pageTitle: "Elseport · 界面预览"
    }
};

const currentPreviewCopy = previewCopy[previewLanguage];
document.documentElement.lang = previewLanguage === "zh" ? "zh-CN" : "en";
document.title = currentPreviewCopy.pageTitle;
document.querySelectorAll("[data-copy]").forEach(element => {
    element.textContent = currentPreviewCopy[element.dataset.copy];
});
document.getElementById("screens").setAttribute("aria-label", currentPreviewCopy.image);
document.getElementById("screen-image").src = `previews/readme-light-${previewLanguage}.png`;
document.getElementById("screen-image").alt = currentPreviewCopy.image;
document.getElementById("app-icon").alt = currentPreviewCopy.icon;
for (const id of ["screen-link", "prototype-link"])
    document.getElementById(id).href = `prototype/?lang=${previewLanguage}&theme=light`;

const alternateLanguage = previewLanguage === "zh" ? "en" : "zh";
const languageLink = document.getElementById("language-link");
languageLink.href = `?lang=${alternateLanguage}`;
languageLink.lang = alternateLanguage === "zh" ? "zh-CN" : "en";
languageLink.textContent = alternateLanguage === "zh" ? "简体中文" : "English";
