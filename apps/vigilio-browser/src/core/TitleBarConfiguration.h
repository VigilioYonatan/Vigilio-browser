#pragma once

#include "core/TitleBarTypes.h"

namespace vigilio {

/// @brief Constantes canónicas, factorías de planes de barra de título y verificación de invariantes.
class TitleBarConfiguration {
  public:
    // Alturas canónicas de barra de título
    static constexpr int kDefaultTitleBarHeight = 32;
    static constexpr int kSeniorTitleBarHeight = 48;
    static constexpr int kCompactTitleBarHeight = 28;

    // Dimensiones de botones estándar
    static constexpr int kDefaultButtonWidth = 46;
    static constexpr int kDefaultButtonHeight = 32;

    // Dimensiones de botones Senior («Interfaz más fácil de usar»)
    static constexpr int kSeniorButtonWidth = 48;
    static constexpr int kSeniorButtonHeight = 48;

    // Dimensiones de botones Compact
    static constexpr int kCompactButtonWidth = 36;
    static constexpr int kCompactButtonHeight = 28;

    // Tamaños de icono de ventana
    static constexpr int kDefaultIconSize = 16;
    static constexpr int kSeniorIconSize = 24;
    static constexpr int kCompactIconSize = 16;

    // Áreas mínimas de arrastre (draggable space)
    static constexpr int kMinDraggableAreaWidthDefault = 120;
    static constexpr int kMinDraggableAreaWidthSenior = 160;
    static constexpr int kMinDraggableAreaWidthCompact = 100;

    // Anchuras mínimas de ventana requeridas para barra de título integrada
    static constexpr int kMinWindowWidthDefault = 400;
    static constexpr int kMinWindowWidthSenior = 500;
    static constexpr int kMinWindowWidthCompact = 360;

    // Separación entre botones
    static constexpr int kDefaultButtonSpacing = 1;
    static constexpr int kSeniorButtonSpacing = 4;
    static constexpr int kCompactButtonSpacing = 1;

    // Márgenes de icono
    static constexpr int kDefaultIconMargin = 10;
    static constexpr int kSeniorIconMargin = 12;
    static constexpr int kCompactIconMargin = 8;

    /// @brief Genera el plan canónico de barra de título estándar.
    [[nodiscard]] static TitleBarLayoutPlan canonicalDefaultTitleBar(int windowWidth = 1200);

    /// @brief Genera el plan canónico de barra de título para la «Interfaz más fácil de usar» (Senior).
    [[nodiscard]] static TitleBarLayoutPlan canonicalSeniorTitleBar(int windowWidth = 1280);

    /// @brief Genera el plan canónico de barra de título para pantallas de alta densidad (Compacto).
    [[nodiscard]] static TitleBarLayoutPlan canonicalCompactTitleBar(int windowWidth = 1024);

    /// @brief Verifica el cumplimiento estricto de todas las invariantes geométricas del plan.
    [[nodiscard]] static bool verifyInvariants(const TitleBarLayoutPlan& plan) noexcept;
};

} // namespace vigilio
