#ifndef THEMES_HPP
#define THEMES_HPP

#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <functional>

class ThemeManager {
public:
    // Constructor
    ThemeManager();
    
    // Estructura per a un tema
    struct Theme {
        std::string name;
        ImVec4 baseColor;
        ImVec4 backgroundColor;
        ImVec4 textColor;
        ImVec4 textDisabledColor;
        ImVec4 windowBgColor;
        ImVec4 childBgColor;
        ImVec4 popupBgColor;
        ImVec4 borderColor;
        ImVec4 borderShadowColor;
        ImVec4 frameBgColor;
        ImVec4 frameBgHoveredColor;
        ImVec4 frameBgActiveColor;
        ImVec4 titleBgColor;
        ImVec4 titleBgActiveColor;
        ImVec4 titleBgCollapsedColor;
        ImVec4 menuBarBgColor;
        ImVec4 scrollbarBgColor;
        ImVec4 scrollbarGrabColor;
        ImVec4 scrollbarGrabHoveredColor;
        ImVec4 scrollbarGrabActiveColor;
        ImVec4 checkMarkColor;
        ImVec4 sliderGrabColor;
        ImVec4 sliderGrabActiveColor;
        ImVec4 buttonColor;
        ImVec4 buttonHoveredColor;
        ImVec4 buttonActiveColor;
        ImVec4 headerColor;
        ImVec4 headerHoveredColor;
        ImVec4 headerActiveColor;
        ImVec4 separatorColor;
        ImVec4 separatorHoveredColor;
        ImVec4 separatorActiveColor;
        ImVec4 resizeGripColor;
        ImVec4 resizeGripHoveredColor;
        ImVec4 resizeGripActiveColor;
        ImVec4 tabColor;
        ImVec4 tabHoveredColor;
        ImVec4 tabActiveColor;
        ImVec4 tabUnfocusedColor;
        ImVec4 tabUnfocusedActiveColor;
        ImVec4 plotLinesColor;
        ImVec4 plotLinesHoveredColor;
        ImVec4 plotHistogramColor;
        ImVec4 plotHistogramHoveredColor;
        ImVec4 tableHeaderBgColor;
        ImVec4 tableBorderStrongColor;
        ImVec4 tableBorderLightColor;
        ImVec4 tableRowBgColor;
        ImVec4 tableRowBgAltColor;
        ImVec4 textSelectedBgColor;
        ImVec4 dragDropTargetColor;
        ImVec4 navHighlightColor;
        ImVec4 navWindowingHighlightColor;
        ImVec4 navWindowingDimBgColor;
        ImVec4 modalWindowDimBgColor;
        
        // Colors especials per a la nostra aplicació
        ImVec4 userMessageColor;
        ImVec4 aiMessageColor;
        ImVec4 commandOutputColor;
        ImVec4 commandErrorColor;
        ImVec4 systemMessageColor;
        ImVec4 onlineStatusColor;
        ImVec4 offlineStatusColor;
        ImVec4 thinkingStatusColor;
    };
    
    // Mètodes per gestionar temes
    void applyTheme(const Theme& theme);
    Theme generateThemeFromBaseColor(const std::string& name, const ImVec4& baseColor);
    Theme generateThemeFromBaseColor(const std::string& name, float r, float g, float b);
    
    // Tema per defecte (Dark) - CANVIAT: no estàtic
    Theme getDefaultDarkTheme();
    
    // Tema per defecte (Light) - CANVIAT: no estàtic
    Theme getDefaultLightTheme();
    
    // Tema basat en un color específic
    Theme getBlueTheme();
    Theme getGreenTheme();
    Theme getPurpleTheme();
    Theme getOrangeTheme();
    Theme getRedTheme();
    
    // Llista de temes predefinits
    std::vector<Theme> getPredefinedThemes();
    
    // Aplicar estils d'arrodoniment
    static void applyRoundedStyle(ImGuiStyle& style, float rounding = 8.0f);
    
    // Utilitats per a colors
    static ImVec4 darkenColor(const ImVec4& color, float factor);
    static ImVec4 lightenColor(const ImVec4& color, float factor);
    static ImVec4 adjustSaturation(const ImVec4& color, float factor);
    static ImVec4 blendColors(const ImVec4& color1, const ImVec4& color2, float t);
    static ImVec4 rgbToImVec4(float r, float g, float b, float a = 1.0f);
    
private:
    // Generar colors derivats
    ImVec4 generateBackgroundColor(const ImVec4& baseColor);
    ImVec4 generateTextColor(const ImVec4& backgroundColor);
    ImVec4 generateFrameBgColor(const ImVec4& baseColor);
    ImVec4 generateButtonColor(const ImVec4& baseColor);
    ImVec4 generateHeaderColor(const ImVec4& baseColor);
    ImVec4 generateTabColor(const ImVec4& baseColor);
    
    // Colors especials per a la nostra aplicació
    ImVec4 generateUserMessageColor(const ImVec4& baseColor);
    ImVec4 generateAiMessageColor(const ImVec4& baseColor);
    ImVec4 generateCommandOutputColor(const ImVec4& baseColor);
    ImVec4 generateCommandErrorColor(const ImVec4& baseColor);
    ImVec4 generateSystemMessageColor(const ImVec4& baseColor);
};

#endif // THEMES_HPP