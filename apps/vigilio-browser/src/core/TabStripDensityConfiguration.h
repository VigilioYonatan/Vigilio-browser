#pragma once

#include "core/MainWindowLayoutTypes.h"
#include "core/TabStripDensityTypes.h"

namespace vigilio {

/// @brief Constantes canónicas, factorías de planes de tira de pestañas y verificación de invariantes.
class TabStripDensityConfiguration {
  public:
    // Alturas canónicas de tira de pestañas
    static constexpr int kDefaultStripHeight = 36;
    static constexpr int kSeniorStripHeight = 48;
    static constexpr int kCompactStripHeight = 28;

    // Ancho mínimo de tira de pestañas
    static constexpr int kDefaultMinStripWidth = 400;
    static constexpr int kSeniorMinStripWidth = 500;
    static constexpr int kCompactMinStripWidth = 360;

    // Anchos de pestaña regular: Default
    static constexpr int kDefaultPreferredTabWidth = 200;
    static constexpr int kDefaultMinTabWidth = 120;
    static constexpr int kDefaultMaxTabWidth = 240;

    // Anchos de pestaña regular: Senior («Interfaz más fácil de usar»)
    static constexpr int kSeniorPreferredTabWidth = 220;
    static constexpr int kSeniorMinTabWidth = 140;
    static constexpr int kSeniorMaxTabWidth = 280;

    // Anchos de pestaña regular: Compact
    static constexpr int kCompactPreferredTabWidth = 160;
    static constexpr int kCompactMinTabWidth = 80;
    static constexpr int kCompactMaxTabWidth = 200;

    // Anchos de pestaña fijada (pinned)
    static constexpr int kDefaultPinnedTabWidth = 36;
    static constexpr int kSeniorPinnedTabWidth = 48;
    static constexpr int kCompactPinnedTabWidth = 28;

    // Dimensiones botón nueva pestaña
    static constexpr int kDefaultNewTabButtonSize = 28;
    static constexpr int kSeniorNewTabButtonSize = 48;
    static constexpr int kCompactNewTabButtonSize = 24;

    // Tamaños de icono (favicon)
    static constexpr int kDefaultFaviconSize = 16;
    static constexpr int kSeniorFaviconSize = 24;
    static constexpr int kCompactFaviconSize = 16;

    // Tamaños de botón de cierre
    static constexpr int kDefaultCloseButtonSize = 16;
    static constexpr int kSeniorCloseButtonSize = 24;
    static constexpr int kCompactCloseButtonSize = 14;

    // Tamaños de indicador de audio
    static constexpr int kDefaultAudioIndicatorSize = 16;
    static constexpr int kSeniorAudioIndicatorSize = 24;
    static constexpr int kCompactAudioIndicatorSize = 14;

    // Umbrales de compresión para ocultar botón de cierre en pestañas inactivas
    static constexpr int kDefaultCloseThreshold = 90;
    static constexpr int kSeniorCloseThreshold = 140;
    static constexpr int kCompactCloseThreshold = 90;

    // Espaciado entre pestañas
    static constexpr int kDefaultTabSpacing = 1;
    static constexpr int kSeniorTabSpacing = 4;
    static constexpr int kCompactTabSpacing = 1;

    // Márgenes/padding de contenedor
    static constexpr int kDefaultPaddingLeft = 8;
    static constexpr int kDefaultPaddingRight = 8;
    static constexpr int kSeniorPaddingLeft = 12;
    static constexpr int kSeniorPaddingRight = 12;
    static constexpr int kCompactPaddingLeft = 6;
    static constexpr int kCompactPaddingRight = 6;

    /// @brief Devuelve las dimensiones canónicas asociadas a una densidad determinada.
    [[nodiscard]] static TabStripDimensions dimensionsForDensity(LayoutDensity density) noexcept;

    /// @brief Genera el plan canónico de tira de pestañas estándar.
    [[nodiscard]] static TabStripDensityPlan canonicalDefaultDensity(int availableWidth = 1000);

    /// @brief Genera el plan canónico de tira de pestañas para la «Interfaz más fácil de usar» (Senior).
    [[nodiscard]] static TabStripDensityPlan canonicalSeniorDensity(int availableWidth = 1000);

    /// @brief Genera el plan canónico de tira de pestañas para pantallas de alta densidad (Compacto).
    [[nodiscard]] static TabStripDensityPlan canonicalCompactDensity(int availableWidth = 800);

    /// @brief Verifica el cumplimiento estricto de todas las invariantes geométricas del plan.
    [[nodiscard]] static bool verifyInvariants(const TabStripDensityPlan& plan) noexcept;
};

} // namespace vigilio
