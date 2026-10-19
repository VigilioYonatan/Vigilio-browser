#pragma once

#include "core/MainWindowLayoutTypes.h"

namespace vigilio {

/// @brief Constantes canónicas, factorías de planes de layout y verificación de invariantes.
class MainWindowLayoutConfiguration {
  public:
    // Dimensiones canónicas estándar de ventana
    static constexpr int kDefaultWindowWidth = 1200;
    static constexpr int kDefaultWindowHeight = 780;
    static constexpr int kMinWindowWidthDefault = 640;
    static constexpr int kMinWindowHeightDefault = 480;

    // Dimensiones mínimas para «Interfaz más fácil de usar» (Senior)
    static constexpr int kMinWindowWidthSenior = 720;
    static constexpr int kMinWindowHeightSenior = 520;
    static constexpr int kDefaultWindowWidthSenior = 1280;
    static constexpr int kDefaultWindowHeightSenior = 840;

    // Dimensiones mínimas para densidad compacta
    static constexpr int kMinWindowWidthCompact = 500;
    static constexpr int kMinWindowHeightCompact = 380;
    static constexpr int kDefaultWindowWidthCompact = 1024;
    static constexpr int kDefaultWindowHeightCompact = 640;

    // Barra lateral (Sidebar)
    static constexpr int kCanonicalSidebarCollapsedWidth = 48;
    static constexpr int kCanonicalSidebarDefaultExpandedWidth = 240;
    static constexpr int kCanonicalSidebarMinExpandedWidth = 200;
    static constexpr int kCanonicalSidebarMaxExpandedWidth = 360;

    // Alturas canónicas de barras de navegación (ToolBar)
    static constexpr int kCanonicalToolBarHeightDefault = 42;
    static constexpr int kCanonicalToolBarHeightSenior = 56;
    static constexpr int kCanonicalToolBarHeightCompact = 36;

    // Alturas canónicas de tiras de pestañas (TabStrip)
    static constexpr int kCanonicalTabStripHeightDefault = 38;
    static constexpr int kCanonicalTabStripHeightSenior = 48;
    static constexpr int kCanonicalTabStripHeightCompact = 32;

    // Alturas canónicas de avisos (NotificationBanner)
    static constexpr int kCanonicalBannerHeightDefault = 44;
    static constexpr int kCanonicalBannerHeightSenior = 56;
    static constexpr int kCanonicalBannerHeightCompact = 36;

    // Alturas canónicas de barra de estado (StatusBar)
    static constexpr int kCanonicalStatusBarHeightDefault = 28;
    static constexpr int kCanonicalStatusBarHeightSenior = 34;
    static constexpr int kCanonicalStatusBarHeightCompact = 24;

    // Altura canónica de barra de título nativa/personalizada (TitleBar)
    static constexpr int kCanonicalTitleBarHeight = 32;

    // Presupuestos mínimos del área central de contenido
    static constexpr int kMinContentWidth = 400;
    static constexpr int kMinContentHeight = 250;

    // Accesibilidad: tap targets y controles mínimos para «Interfaz más fácil de usar»
    static constexpr int kSeniorMinTapTargetSize = 48;

    /// @brief Genera el plan canónico de escritorio estándar.
    [[nodiscard]] static MainWindowLayoutPlan canonicalDefaultLayout();

    /// @brief Genera el plan canónico para la «Interfaz más fácil de usar» (Modo Senior).
    [[nodiscard]] static MainWindowLayoutPlan canonicalSeniorLayout();

    /// @brief Genera el plan canónico para pantallas de alta densidad (Compacto).
    [[nodiscard]] static MainWindowLayoutPlan canonicalCompactLayout();

    /// @brief Verifica de manera booleana el cumplimiento de todas las invariantes geométricas del plan.
    [[nodiscard]] static bool verifyInvariants(const MainWindowLayoutPlan& plan) noexcept;
};

} // namespace vigilio
