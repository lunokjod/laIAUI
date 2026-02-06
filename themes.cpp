#include "themes.hpp"
#include <algorithm>
#include <cmath>

ThemeManager::ThemeManager() {
    // Constructor
}

void ThemeManager::applyTheme(const Theme& theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    style.Colors[ImGuiCol_Text] = theme.textColor;
    style.Colors[ImGuiCol_TextDisabled] = theme.textDisabledColor;
    style.Colors[ImGuiCol_WindowBg] = theme.windowBgColor;
    style.Colors[ImGuiCol_ChildBg] = theme.childBgColor;
    style.Colors[ImGuiCol_PopupBg] = theme.popupBgColor;
    style.Colors[ImGuiCol_Border] = theme.borderColor;
    style.Colors[ImGuiCol_BorderShadow] = theme.borderShadowColor;
    style.Colors[ImGuiCol_FrameBg] = theme.frameBgColor;
    style.Colors[ImGuiCol_FrameBgHovered] = theme.frameBgHoveredColor;
    style.Colors[ImGuiCol_FrameBgActive] = theme.frameBgActiveColor;
    style.Colors[ImGuiCol_TitleBg] = theme.titleBgColor;
    style.Colors[ImGuiCol_TitleBgActive] = theme.titleBgActiveColor;
    style.Colors[ImGuiCol_TitleBgCollapsed] = theme.titleBgCollapsedColor;
    style.Colors[ImGuiCol_MenuBarBg] = theme.menuBarBgColor;
    style.Colors[ImGuiCol_ScrollbarBg] = theme.scrollbarBgColor;
    style.Colors[ImGuiCol_ScrollbarGrab] = theme.scrollbarGrabColor;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = theme.scrollbarGrabHoveredColor;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = theme.scrollbarGrabActiveColor;
    style.Colors[ImGuiCol_CheckMark] = theme.checkMarkColor;
    style.Colors[ImGuiCol_SliderGrab] = theme.sliderGrabColor;
    style.Colors[ImGuiCol_SliderGrabActive] = theme.sliderGrabActiveColor;
    style.Colors[ImGuiCol_Button] = theme.buttonColor;
    style.Colors[ImGuiCol_ButtonHovered] = theme.buttonHoveredColor;
    style.Colors[ImGuiCol_ButtonActive] = theme.buttonActiveColor;
    style.Colors[ImGuiCol_Header] = theme.headerColor;
    style.Colors[ImGuiCol_HeaderHovered] = theme.headerHoveredColor;
    style.Colors[ImGuiCol_HeaderActive] = theme.headerActiveColor;
    style.Colors[ImGuiCol_Separator] = theme.separatorColor;
    style.Colors[ImGuiCol_SeparatorHovered] = theme.separatorHoveredColor;
    style.Colors[ImGuiCol_SeparatorActive] = theme.separatorActiveColor;
    style.Colors[ImGuiCol_ResizeGrip] = theme.resizeGripColor;
    style.Colors[ImGuiCol_ResizeGripHovered] = theme.resizeGripHoveredColor;
    style.Colors[ImGuiCol_ResizeGripActive] = theme.resizeGripActiveColor;
    style.Colors[ImGuiCol_Tab] = theme.tabColor;
    style.Colors[ImGuiCol_TabHovered] = theme.tabHoveredColor;
    style.Colors[ImGuiCol_TabActive] = theme.tabActiveColor;
    style.Colors[ImGuiCol_TabUnfocused] = theme.tabUnfocusedColor;
    style.Colors[ImGuiCol_TabUnfocusedActive] = theme.tabUnfocusedActiveColor;
    style.Colors[ImGuiCol_PlotLines] = theme.plotLinesColor;
    style.Colors[ImGuiCol_PlotLinesHovered] = theme.plotLinesHoveredColor;
    style.Colors[ImGuiCol_PlotHistogram] = theme.plotHistogramColor;
    style.Colors[ImGuiCol_PlotHistogramHovered] = theme.plotHistogramHoveredColor;
    style.Colors[ImGuiCol_TableHeaderBg] = theme.tableHeaderBgColor;
    style.Colors[ImGuiCol_TableBorderStrong] = theme.tableBorderStrongColor;
    style.Colors[ImGuiCol_TableBorderLight] = theme.tableBorderLightColor;
    style.Colors[ImGuiCol_TableRowBg] = theme.tableRowBgColor;
    style.Colors[ImGuiCol_TableRowBgAlt] = theme.tableRowBgAltColor;
    style.Colors[ImGuiCol_TextSelectedBg] = theme.textSelectedBgColor;
    style.Colors[ImGuiCol_DragDropTarget] = theme.dragDropTargetColor;
    style.Colors[ImGuiCol_NavHighlight] = theme.navHighlightColor;
    style.Colors[ImGuiCol_NavWindowingHighlight] = theme.navWindowingHighlightColor;
    style.Colors[ImGuiCol_NavWindowingDimBg] = theme.navWindowingDimBgColor;
    style.Colors[ImGuiCol_ModalWindowDimBg] = theme.modalWindowDimBgColor;
}

ThemeManager::Theme ThemeManager::generateThemeFromBaseColor(const std::string& name, const ImVec4& baseColor) {
    Theme theme;
    theme.name = name;
    theme.baseColor = baseColor;
    
    // Determinar si el tema ha de ser clar o fosc basant-nos en la brillantor del color base
    float baseBrightness = (baseColor.x + baseColor.y + baseColor.z) / 3.0f;
    bool isLightTheme = (baseBrightness > 0.6f);
    
    if (isLightTheme) {
        // Per a temes clars
        theme.backgroundColor = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        theme.textColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        theme.textDisabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        theme.windowBgColor = theme.backgroundColor;
        theme.childBgColor = lightenColor(theme.windowBgColor, 0.03f);
        theme.popupBgColor = theme.windowBgColor;
        theme.borderColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.borderShadowColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        
        theme.frameBgColor = darkenColor(theme.windowBgColor, 0.05f);
        theme.frameBgHoveredColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.frameBgActiveColor = darkenColor(theme.windowBgColor, 0.15f);
        
        theme.titleBgColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.titleBgActiveColor = baseColor;
        theme.titleBgCollapsedColor = theme.titleBgColor;
        theme.menuBarBgColor = darkenColor(theme.titleBgColor, 0.05f);
        
        theme.scrollbarBgColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.scrollbarGrabColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.scrollbarGrabHoveredColor = darkenColor(theme.windowBgColor, 0.3f);
        theme.scrollbarGrabActiveColor = darkenColor(theme.windowBgColor, 0.4f);
        
        theme.checkMarkColor = darkenColor(baseColor, 0.3f);
        theme.sliderGrabColor = darkenColor(baseColor, 0.2f);
        theme.sliderGrabActiveColor = darkenColor(baseColor, 0.3f);
        
        theme.buttonColor = lightenColor(baseColor, 0.3f);
        theme.buttonHoveredColor = lightenColor(theme.buttonColor, 0.1f);
        theme.buttonActiveColor = lightenColor(theme.buttonColor, 0.2f);
        
        theme.headerColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.headerHoveredColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.headerActiveColor = darkenColor(theme.windowBgColor, 0.3f);
        
        theme.separatorColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.separatorHoveredColor = darkenColor(baseColor, 0.3f);
        theme.separatorActiveColor = baseColor;
        
        theme.resizeGripColor = theme.buttonColor;
        theme.resizeGripHoveredColor = theme.buttonHoveredColor;
        theme.resizeGripActiveColor = theme.buttonActiveColor;
        
        theme.tabColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.tabHoveredColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.tabActiveColor = darkenColor(theme.windowBgColor, 0.3f);
        theme.tabUnfocusedColor = lightenColor(theme.tabColor, 0.1f);
        theme.tabUnfocusedActiveColor = lightenColor(theme.tabActiveColor, 0.1f);
        
        theme.plotLinesColor = darkenColor(baseColor, 0.3f);
        theme.plotLinesHoveredColor = baseColor;
        theme.plotHistogramColor = darkenColor(baseColor, 0.2f);
        theme.plotHistogramHoveredColor = darkenColor(baseColor, 0.1f);
        
        theme.tableHeaderBgColor = darkenColor(theme.windowBgColor, 0.05f);
        theme.tableBorderStrongColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.tableBorderLightColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.tableRowBgColor = theme.windowBgColor;
        theme.tableRowBgAltColor = lightenColor(theme.windowBgColor, 0.02f);
        
        theme.textSelectedBgColor = ImVec4(baseColor.x, baseColor.y, baseColor.z, 0.35f);
        theme.dragDropTargetColor = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
        theme.navHighlightColor = baseColor;
        theme.navWindowingHighlightColor = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
        theme.navWindowingDimBgColor = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
        theme.modalWindowDimBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
        
    } else {
        // Per a temes foscos (codi original)
        theme.backgroundColor = generateBackgroundColor(baseColor);
        theme.textColor = generateTextColor(theme.backgroundColor);
        theme.textDisabledColor = darkenColor(theme.textColor, 0.5f);
        theme.windowBgColor = darkenColor(theme.backgroundColor, 0.1f);
        theme.childBgColor = darkenColor(theme.windowBgColor, 0.05f);
        theme.popupBgColor = theme.windowBgColor;
        theme.borderColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.borderShadowColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        
        theme.frameBgColor = generateFrameBgColor(baseColor);
        theme.frameBgHoveredColor = lightenColor(theme.frameBgColor, 0.1f);
        theme.frameBgActiveColor = lightenColor(theme.frameBgColor, 0.2f);
        
        theme.titleBgColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.titleBgActiveColor = baseColor;
        theme.titleBgCollapsedColor = theme.titleBgColor;
        theme.menuBarBgColor = darkenColor(theme.titleBgColor, 0.05f);
        
        theme.scrollbarBgColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.scrollbarGrabColor = lightenColor(theme.scrollbarBgColor, 0.2f);
        theme.scrollbarGrabHoveredColor = lightenColor(theme.scrollbarGrabColor, 0.1f);
        theme.scrollbarGrabActiveColor = lightenColor(theme.scrollbarGrabColor, 0.2f);
        
        theme.checkMarkColor = lightenColor(baseColor, 0.5f);
        theme.sliderGrabColor = theme.buttonColor;
        theme.sliderGrabActiveColor = theme.buttonActiveColor;
        
        theme.buttonColor = generateButtonColor(baseColor);
        theme.buttonHoveredColor = lightenColor(theme.buttonColor, 0.1f);
        theme.buttonActiveColor = lightenColor(theme.buttonColor, 0.2f);
        
        theme.headerColor = generateHeaderColor(baseColor);
        theme.headerHoveredColor = lightenColor(theme.headerColor, 0.1f);
        theme.headerActiveColor = lightenColor(theme.headerColor, 0.2f);
        
        theme.separatorColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.separatorHoveredColor = lightenColor(baseColor, 0.3f);
        theme.separatorActiveColor = baseColor;
        
        theme.resizeGripColor = theme.buttonColor;
        theme.resizeGripHoveredColor = theme.buttonHoveredColor;
        theme.resizeGripActiveColor = theme.buttonActiveColor;
        
        theme.tabColor = generateTabColor(baseColor);
        theme.tabHoveredColor = lightenColor(theme.tabColor, 0.1f);
        theme.tabActiveColor = lightenColor(theme.tabColor, 0.2f);
        theme.tabUnfocusedColor = darkenColor(theme.tabColor, 0.3f);
        theme.tabUnfocusedActiveColor = darkenColor(theme.tabActiveColor, 0.3f);
        
        theme.plotLinesColor = lightenColor(baseColor, 0.4f);
        theme.plotLinesHoveredColor = baseColor;
        theme.plotHistogramColor = lightenColor(baseColor, 0.3f);
        theme.plotHistogramHoveredColor = lightenColor(baseColor, 0.5f);
        
        theme.tableHeaderBgColor = darkenColor(theme.windowBgColor, 0.05f);
        theme.tableBorderStrongColor = darkenColor(theme.windowBgColor, 0.2f);
        theme.tableBorderLightColor = darkenColor(theme.windowBgColor, 0.1f);
        theme.tableRowBgColor = theme.windowBgColor;
        theme.tableRowBgAltColor = lightenColor(theme.windowBgColor, 0.05f);
        
        theme.textSelectedBgColor = ImVec4(baseColor.x, baseColor.y, baseColor.z, 0.35f);
        theme.dragDropTargetColor = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
        theme.navHighlightColor = baseColor;
        theme.navWindowingHighlightColor = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
        theme.navWindowingDimBgColor = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
        theme.modalWindowDimBgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
    }
    
    // Colors especials per a la nostra aplicació (comuns per a tots els temes)
    theme.userMessageColor = generateUserMessageColor(baseColor);
    theme.aiMessageColor = generateAiMessageColor(baseColor);
    theme.commandOutputColor = generateCommandOutputColor(baseColor);
    theme.commandErrorColor = generateCommandErrorColor(baseColor);
    theme.systemMessageColor = generateSystemMessageColor(baseColor);
    theme.onlineStatusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    theme.offlineStatusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    theme.thinkingStatusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    return theme;
}

ThemeManager::Theme ThemeManager::generateThemeFromBaseColor(const std::string& name, float r, float g, float b) {
    return generateThemeFromBaseColor(name, rgbToImVec4(r, g, b));
}

ThemeManager::Theme ThemeManager::getDefaultDarkTheme() {
    return generateThemeFromBaseColor("Dark", 0.2f, 0.6f, 1.0f);
}

ThemeManager::Theme ThemeManager::getDefaultLightTheme() {
    Theme theme = generateThemeFromBaseColor("Light", 0.8f, 0.85f, 0.9f);
    
    // Ajustar colors per a tema clar
    theme.textColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    theme.textDisabledColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    theme.windowBgColor = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    theme.childBgColor = ImVec4(0.98f, 0.98f, 0.98f, 1.0f);
    theme.popupBgColor = theme.windowBgColor;
    
    // Actualitzar colors derivats del windowBgColor
    theme.titleBgColor = darkenColor(theme.windowBgColor, 0.1f);
    theme.titleBgActiveColor = theme.baseColor;
    theme.titleBgCollapsedColor = theme.titleBgColor;
    theme.menuBarBgColor = darkenColor(theme.titleBgColor, 0.05f);
    
    theme.scrollbarBgColor = darkenColor(theme.windowBgColor, 0.1f);
    theme.scrollbarGrabColor = lightenColor(theme.scrollbarBgColor, 0.2f);
    theme.scrollbarGrabHoveredColor = lightenColor(theme.scrollbarGrabColor, 0.1f);
    theme.scrollbarGrabActiveColor = lightenColor(theme.scrollbarGrabColor, 0.2f);
    
    theme.frameBgColor = lightenColor(theme.windowBgColor, 0.1f);
    theme.frameBgHoveredColor = lightenColor(theme.frameBgColor, 0.1f);
    theme.frameBgActiveColor = lightenColor(theme.frameBgColor, 0.2f);
    
    theme.borderColor = darkenColor(theme.windowBgColor, 0.2f);
    theme.separatorColor = darkenColor(theme.windowBgColor, 0.2f);
    
    theme.tableHeaderBgColor = darkenColor(theme.windowBgColor, 0.05f);
    theme.tableBorderStrongColor = darkenColor(theme.windowBgColor, 0.2f);
    theme.tableBorderLightColor = darkenColor(theme.windowBgColor, 0.1f);
    theme.tableRowBgColor = theme.windowBgColor;
    theme.tableRowBgAltColor = lightenColor(theme.windowBgColor, 0.05f);
    
    // Ajustar colors de botons per a tema clar
    theme.buttonColor = lightenColor(theme.baseColor, 0.3f);
    theme.buttonHoveredColor = lightenColor(theme.buttonColor, 0.1f);
    theme.buttonActiveColor = lightenColor(theme.buttonColor, 0.2f);
    
    // Ajustar colors de tabs per a tema clar
    theme.tabColor = darkenColor(theme.windowBgColor, 0.1f);
    theme.tabHoveredColor = lightenColor(theme.tabColor, 0.1f);
    theme.tabActiveColor = lightenColor(theme.tabColor, 0.2f);
    theme.tabUnfocusedColor = darkenColor(theme.tabColor, 0.3f);
    theme.tabUnfocusedActiveColor = darkenColor(theme.tabActiveColor, 0.3f);
    
    // Ajustar colors de header per a tema clar
    theme.headerColor = darkenColor(theme.windowBgColor, 0.1f);
    theme.headerHoveredColor = lightenColor(theme.headerColor, 0.1f);
    theme.headerActiveColor = lightenColor(theme.headerColor, 0.2f);
    
    return theme;
}

ThemeManager::Theme ThemeManager::getBlueTheme() {
    return generateThemeFromBaseColor("Blue", 0.2f, 0.6f, 1.0f);
}

ThemeManager::Theme ThemeManager::getGreenTheme() {
    return generateThemeFromBaseColor("Green", 0.2f, 0.8f, 0.4f);
}

ThemeManager::Theme ThemeManager::getPurpleTheme() {
    return generateThemeFromBaseColor("Purple", 0.7f, 0.3f, 0.9f);
}

ThemeManager::Theme ThemeManager::getOrangeTheme() {
    return generateThemeFromBaseColor("Orange", 1.0f, 0.5f, 0.2f);
}

ThemeManager::Theme ThemeManager::getRedTheme() {
    return generateThemeFromBaseColor("Red", 1.0f, 0.3f, 0.3f);
}

std::vector<ThemeManager::Theme> ThemeManager::getPredefinedThemes() {
    std::vector<Theme> themes;
    themes.push_back(getDefaultDarkTheme());
    themes.push_back(getDefaultLightTheme());
    themes.push_back(getBlueTheme());
    themes.push_back(getGreenTheme());
    themes.push_back(getPurpleTheme());
    themes.push_back(getOrangeTheme());
    themes.push_back(getRedTheme());
    return themes;
}

void ThemeManager::applyRoundedStyle(ImGuiStyle& style, float rounding) {
    style.FrameRounding = rounding;
    style.FrameBorderSize = 1.0f;
    style.WindowRounding = rounding * 1.5f;
    style.PopupRounding = rounding;
    style.ScrollbarRounding = rounding;
    style.GrabRounding = rounding;
    style.TabRounding = rounding;
    style.ChildRounding = rounding;
}

ImVec4 ThemeManager::darkenColor(const ImVec4& color, float factor) {
    return ImVec4(
        std::max(0.0f, color.x * (1.0f - factor)),
        std::max(0.0f, color.y * (1.0f - factor)),
        std::max(0.0f, color.z * (1.0f - factor)),
        color.w
    );
}

ImVec4 ThemeManager::lightenColor(const ImVec4& color, float factor) {
    return ImVec4(
        std::min(1.0f, color.x + (1.0f - color.x) * factor),
        std::min(1.0f, color.y + (1.0f - color.y) * factor),
        std::min(1.0f, color.z + (1.0f - color.z) * factor),
        color.w
    );
}

ImVec4 ThemeManager::adjustSaturation(const ImVec4& color, float factor) {
    // Convertir a HSV, ajustar saturació, tornar a RGB
    float r = color.x, g = color.y, b = color.z;
    float max = std::max(std::max(r, g), b);
    float min = std::min(std::min(r, g), b);
    float delta = max - min;
    
    if (delta < 0.001f) return color; // Color gris
    
    float s = delta / max;
    s *= factor;
    s = std::min(std::max(s, 0.0f), 1.0f);
    
    // Tornar a RGB (simplificat)
    if (max == r) {
        g = (g - min) * s / delta + min;
        b = (b - min) * s / delta + min;
    } else if (max == g) {
        r = (r - min) * s / delta + min;
        b = (b - min) * s / delta + min;
    } else {
        r = (r - min) * s / delta + min;
        g = (g - min) * s / delta + min;
    }
    
    return ImVec4(r, g, b, color.w);
}

ImVec4 ThemeManager::blendColors(const ImVec4& color1, const ImVec4& color2, float t) {
    t = std::min(std::max(t, 0.0f), 1.0f);
    return ImVec4(
        color1.x * (1.0f - t) + color2.x * t,
        color1.y * (1.0f - t) + color2.y * t,
        color1.z * (1.0f - t) + color2.z * t,
        color1.w * (1.0f - t) + color2.w * t
    );
}

ImVec4 ThemeManager::rgbToImVec4(float r, float g, float b, float a) {
    return ImVec4(r, g, b, a);
}

ImVec4 ThemeManager::generateBackgroundColor(const ImVec4& baseColor) {
    // Fons fosc per a temes amb colors brillants
    float brightness = (baseColor.x + baseColor.y + baseColor.z) / 3.0f;
    if (brightness > 0.5f) {
        return ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    } else {
        return darkenColor(baseColor, 0.7f);
    }
}

ImVec4 ThemeManager::generateTextColor(const ImVec4& backgroundColor) {
    // Text clar sobre fons fosc, text fosc sobre fons clar
    float brightness = (backgroundColor.x + backgroundColor.y + backgroundColor.z) / 3.0f;
    if (brightness < 0.5f) {
        return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    } else {
        return ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    }
}

ImVec4 ThemeManager::generateFrameBgColor(const ImVec4& baseColor) {
    return darkenColor(baseColor, 0.6f);
}

ImVec4 ThemeManager::generateButtonColor(const ImVec4& baseColor) {
    return adjustSaturation(baseColor, 0.8f);
}

ImVec4 ThemeManager::generateHeaderColor(const ImVec4& baseColor) {
    return darkenColor(baseColor, 0.4f);
}

ImVec4 ThemeManager::generateTabColor(const ImVec4& baseColor) {
    return darkenColor(baseColor, 0.5f);
}

ImVec4 ThemeManager::generateUserMessageColor(const ImVec4& baseColor) {
    // Blau cian basat en el color base
    return ImVec4(
        baseColor.x * 0.3f,
        baseColor.y * 0.8f + 0.2f,
        baseColor.z * 0.9f + 0.1f,
        1.0f
    );
}

ImVec4 ThemeManager::generateAiMessageColor(const ImVec4& baseColor) {
    // Groc verdós basat en el color base
    return ImVec4(
        baseColor.x * 0.8f + 0.2f,
        baseColor.y * 0.9f + 0.1f,
        baseColor.z * 0.3f,
        1.0f
    );
}

ImVec4 ThemeManager::generateCommandOutputColor(const ImVec4& baseColor) {
    // Gris clar
    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
}

ImVec4 ThemeManager::generateCommandErrorColor(const ImVec4& baseColor) {
    // Vermell basat en el color base
    return ImVec4(
        baseColor.x * 0.5f + 0.5f,
        baseColor.y * 0.3f,
        baseColor.z * 0.3f,
        1.0f
    );
}

ImVec4 ThemeManager::generateSystemMessageColor(const ImVec4& baseColor) {
    // Verd basat en el color base
    return ImVec4(
        baseColor.x * 0.3f,
        baseColor.y * 0.8f + 0.2f,
        baseColor.z * 0.3f,
        1.0f
    );
}
