#pragma once

#include "core/MainWindowLayoutTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Estado de fijación de una pestaña en la tira.
enum class TabPinState : uint8_t { Unpinned, Pinned };

/// @brief Estado de reproducción sonora y multimedia de la pestaña.
enum class TabMediaState : uint8_t { None, AudioPlaying, AudioMuted };

/// @brief Estado de ahorro de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo»).
enum class TabDiscardState : uint8_t { Active, Discarded };

/// @brief Estado de carga y conectividad de la página de la pestaña.
enum class TabLoadingState : uint8_t { Idle, Loading, Failed };

/// @brief Estados interactivos de elemento para puntero y teclado.
enum class TabInteractiveState : uint8_t { Normal, Hovered, Selected, Pressed, Dragging };

/// @brief Clasificación exhaustiva de zonas de impacto espacial (hit-testing desacoplado).
enum class TabStripHitTestZone : uint8_t {
    Nowhere,
    EmptySpace,
    TabBody,
    TabFavicon,
    TabTitle,
    TabCloseButton,
    TabAudioIndicator,
    NewTabButton,
    ScrollLeftButton,
    ScrollRightButton,
    OverflowMenuButton
};

/// @brief Modalidades de manejo ante desbordamiento horizontal por exceso de pestañas.
enum class TabStripOverflowMode : uint8_t { None, Scrollable, DropdownMenu };

/// @brief Clasificación fuertemente tipada de anomalías y fallos de tira de pestañas.
enum class TabStripDensityError : uint8_t {
    None,
    WidthBelowMinimum,
    HeightBelowMinimum,
    SeniorModeMinimumHeightViolation,
    SeniorModeTapTargetViolation,
    InvalidTabDimensions,
    InvalidTabCount,
    NegativeMarginOrSpacing,
    AvailableWidthExhausted
};

/// @brief Dimensiones canónicas y parámetros dimensionales de la tira de pestañas.
struct TabStripDimensions {
    int height{36};
    int minStripWidth{400};
    int preferredTabWidth{200};
    int minTabWidth{120};
    int maxTabWidth{240};
    int pinnedTabWidth{36};
    int newTabButtonWidth{28};
    int newTabButtonHeight{28};
    int faviconSize{16};
    int closeButtonSize{16};
    int audioIndicatorSize{16};
    int tabSpacing{1};
    int closeButtonThresholdWidth{90};
    int paddingLeft{8};
    int paddingRight{8};

    constexpr bool operator==(const TabStripDimensions& other) const = default;
};

/// @brief Especificación individual de entrada para una pestaña.
struct TabSpec {
    uint64_t id{0};
    std::string title{};
    std::string url{};
    TabPinState pinState{TabPinState::Unpinned};
    bool isSelected{false};
    TabMediaState mediaState{TabMediaState::None};
    TabDiscardState discardState{TabDiscardState::Active};
    TabLoadingState loadingState{TabLoadingState::Idle};
    TabInteractiveState interactiveState{TabInteractiveState::Normal};

    bool operator==(const TabSpec& other) const = default;
};

/// @brief Distribución geométrica calculada e inmutable de una pestaña individual.
struct TabItemLayout {
    uint64_t id{0};
    LayoutRect rect{};
    LayoutRect faviconRect{};
    LayoutRect titleRect{};
    LayoutRect closeButtonRect{};
    LayoutRect audioIndicatorRect{};
    bool showFavicon{true};
    bool showTitle{true};
    bool showCloseButton{true};
    bool showAudioIndicator{false};
    bool isPinned{false};
    bool isSelected{false};
    std::string accessibleLabel{};
    std::string tooltip{};

    bool operator==(const TabItemLayout& other) const = default;
};

/// @brief Parámetros declarativos de configuración para el cálculo de la tira de pestañas.
struct TabStripDensityConfig {
    LayoutDensity density{LayoutDensity::Default};
    bool showNewTabButton{true};
    TabStripOverflowMode overflowMode{TabStripOverflowMode::Scrollable};
    int scrollOffset{0};

    bool operator==(const TabStripDensityConfig& other) const = default;
};

/// @brief Plan estructural calculado e inmutable para la tira de pestañas.
struct TabStripDensityPlan {
    int availableWidth{1000};
    int height{36};
    LayoutDensity density{LayoutDensity::Default};
    LayoutRect containerRect{};
    LayoutRect tabsBoundsRect{};
    LayoutRect newTabButtonRect{};
    LayoutRect scrollLeftButtonRect{};
    LayoutRect scrollRightButtonRect{};
    LayoutRect overflowMenuButtonRect{};
    std::vector<TabItemLayout> tabs{};
    int effectiveTabWidth{200};
    int pinnedTabsCount{0};
    int regularTabsCount{0};
    bool isOverflowing{false};
    TabStripOverflowMode overflowMode{TabStripOverflowMode::None};
    int totalConsumedWidth{0};

    [[nodiscard]] constexpr int totalWidth() const noexcept {
        return availableWidth;
    }

    [[nodiscard]] constexpr int totalHeight() const noexcept {
        return height;
    }

    bool operator==(const TabStripDensityPlan& other) const = default;
};

/// @brief Reporte inmutable de diagnóstico y evaluación geométrica de la tira de pestañas.
struct TabStripDensityReport {
    bool valid{true};
    TabStripDensityError error{TabStripDensityError::None};
    TabStripDensityPlan plan{};
    std::string message{};
    std::string remediationHint{};

    bool operator==(const TabStripDensityReport& other) const = default;
};

} // namespace vigilio
