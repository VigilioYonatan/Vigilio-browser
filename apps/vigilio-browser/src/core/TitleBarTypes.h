#pragma once

#include "core/MainWindowLayoutTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Modalidades operativas de la barra de título de la ventana.
enum class TitleBarMode : uint8_t { NativeSystem, CustomIntegrated, Suppressed };

/// @brief Posición de anclaje de la caja de controles de ventana.
enum class TitleBarControlsPlacement : uint8_t { Right, Left };

/// @brief Tipos de botones de control de ventana.
enum class TitleBarButtonType : uint8_t { Minimize, MaximizeRestore, Close, SeniorModeToggle };

/// @brief Estados interactivos de un botón de control.
enum class TitleBarButtonState : uint8_t { Normal, Hovered, Pressed, Disabled };

/// @brief Clasificación exhaustiva de zonas de impacto para non-client hit testing.
enum class TitleBarHitTestZone : uint8_t {
    Nowhere,
    ClientArea,
    DraggableSpace,
    WindowIcon,
    CaptionText,
    MinimizeButton,
    MaximizeButton,
    CloseButton,
    SeniorModeToggle
};

/// @brief Clasificación fuertemente tipada de anomalías y fallos de barra de título.
enum class TitleBarError : uint8_t {
    None,
    DimensionsBelowMinimum,
    ControlsOverflowWidth,
    DraggableAreaInsufficient,
    ConflictingControlPlacement,
    SeniorModeMinimumHeightViolation,
    UnsupportedWindowChromeMode
};

/// @brief Dimensiones canónicas y límites para la barra de título.
struct TitleBarDimensions {
    int height{32};
    int minWidth{400};
    int minDraggableWidth{120};
    int iconSize{16};
    int buttonWidth{46};
    int buttonHeight{32};

    constexpr bool operator==(const TitleBarDimensions& other) const = default;
};

/// @brief Especificación individual de un botón de control de la barra.
struct TitleBarControlSpec {
    TitleBarButtonType type{TitleBarButtonType::Close};
    LayoutRect rect{};
    TitleBarButtonState state{TitleBarButtonState::Normal};
    bool visible{true};
    std::string accessibleLabel{};
    std::string tooltip{};

    bool operator==(const TitleBarControlSpec& other) const = default;
};

/// @brief Estado de la ventana reflejado en la barra de título.
struct TitleBarState {
    bool isMaximized{false};
    bool isActive{true};
    bool isFullscreen{false};
    bool seniorModeActive{false};

    constexpr bool operator==(const TitleBarState& other) const = default;
};

/// @brief Parámetros declarativos de configuración para el cálculo del layout de la barra.
struct TitleBarConfig {
    TitleBarMode mode{TitleBarMode::CustomIntegrated};
    TitleBarControlsPlacement placement{TitleBarControlsPlacement::Right};
    std::string title{"Vigilio Browser"};
    bool showIcon{true};
    bool showSeniorToggle{false};
    TitleBarState state{};
    LayoutDensity density{LayoutDensity::Default};

    bool operator==(const TitleBarConfig& other) const = default;
};

/// @brief Plan estructural calculado e inmutable para la barra de título.
struct TitleBarLayoutPlan {
    int windowWidth{1200};
    int height{32};
    LayoutDensity density{LayoutDensity::Default};
    TitleBarMode mode{TitleBarMode::CustomIntegrated};
    TitleBarControlsPlacement placement{TitleBarControlsPlacement::Right};
    LayoutRect iconRect{};
    LayoutRect titleRect{};
    LayoutRect draggableRect{};
    LayoutRect controlsBoxRect{};
    std::vector<TitleBarControlSpec> controls{};
    TitleBarState state{};

    [[nodiscard]] constexpr int totalWidth() const noexcept {
        return windowWidth;
    }

    [[nodiscard]] constexpr int totalHeight() const noexcept {
        return height;
    }

    bool operator==(const TitleBarLayoutPlan& other) const = default;
};

/// @brief Reporte inmutable de diagnóstico y evaluación geométrica de la barra de título.
struct TitleBarReport {
    bool valid{true};
    TitleBarError error{TitleBarError::None};
    TitleBarLayoutPlan plan{};
    std::string message{};
    std::string remediationHint{};

    bool operator==(const TitleBarReport& other) const = default;
};

} // namespace vigilio
