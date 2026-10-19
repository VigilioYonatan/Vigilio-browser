#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Zonas funcionales y estructurales de la ventana principal.
enum class LayoutZone : uint8_t {
    TitleBar,
    TabStrip,
    ToolBar,
    NotificationBanner,
    Sidebar,
    Content,
    StatusBar
};

/// @brief Densidades visuales de escritorio soportadas en la aplicación.
enum class LayoutDensity : uint8_t { Compact, Default, Senior };

/// @brief Estados operativos y de visibilidad de la barra lateral.
enum class SidebarState : uint8_t { Hidden, Collapsed, Expanded };

/// @brief Posición de anclaje de la barra lateral en la ventana.
enum class SidebarDockPosition : uint8_t { Left, Right };

/// @brief Modalidades de decoración de ventana (chrome).
enum class WindowChromeMode : uint8_t { Standard, Integrated, Fullscreen };

/// @brief Clasificación fuertemente tipada de anomalías y fallos de layout.
enum class MainWindowLayoutError : uint8_t {
    None,
    DimensionsBelowMinimum,
    InvalidSidebarWidth,
    ConflictingZoneAnchors,
    ZeroContentAreaBudget,
    NegativeMarginOrPadding,
    InvalidDensityConfiguration,
    LayoutPersistenceCorrupted
};

/// @brief Márgenes de contención rectangulares.
struct LayoutMargins {
    int top{0};
    int right{0};
    int bottom{0};
    int left{0};

    constexpr bool operator==(const LayoutMargins& other) const = default;
};

/// @brief Geometría rectangular cartesiana con métodos de contención, intersección y área.
struct LayoutRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    [[nodiscard]] constexpr bool intersects(const LayoutRect& other) const noexcept {
        if (width <= 0 || height <= 0 || other.width <= 0 || other.height <= 0) {
            return false;
        }
        return !(x + width <= other.x || other.x + other.width <= x || y + height <= other.y ||
                 other.y + other.height <= y);
    }

    [[nodiscard]] constexpr int area() const noexcept {
        return (width > 0 && height > 0) ? (width * height) : 0;
    }

    constexpr bool operator==(const LayoutRect& other) const = default;
};

/// @brief Dimensiones y límites dimensionales mínimos de la ventana principal.
struct WindowDimensions {
    int width{1200};
    int height{780};
    int minWidth{640};
    int minHeight{480};

    constexpr bool operator==(const WindowDimensions& other) const = default;
};

/// @brief Presupuesto dimensional asignado a una zona de la ventana.
struct ZoneBudget {
    LayoutZone zone{LayoutZone::Content};
    bool visible{true};
    int preferredHeight{0};
    int minHeight{0};
    int maxHeight{0};
    int preferredWidth{0};
    int minWidth{0};
    int maxWidth{0};

    constexpr bool operator==(const ZoneBudget& other) const = default;
};

/// @brief Parámetros de configuración de la barra lateral.
struct SidebarConfig {
    SidebarState state{SidebarState::Collapsed};
    SidebarDockPosition position{SidebarDockPosition::Left};
    int width{48};
    int collapsedWidth{48};
    int defaultExpandedWidth{240};
    int minExpandedWidth{200};
    int maxExpandedWidth{360};

    constexpr bool operator==(const SidebarConfig& other) const = default;
};

/// @brief Plan estructural declarativo e inmutable para la ventana principal.
struct MainWindowLayoutPlan {
    WindowDimensions window{};
    LayoutDensity density{LayoutDensity::Default};
    WindowChromeMode chromeMode{WindowChromeMode::Standard};
    SidebarConfig sidebar{};
    std::vector<ZoneBudget> zones{};
    bool showTitleBar{false};
    bool showTabStrip{true};
    bool showToolBar{true};
    bool showBanner{false};
    bool showSidebar{true};
    bool showStatusBar{true};

    bool operator==(const MainWindowLayoutPlan& other) const = default;
};

/// @brief Asignación espacial concreta calculada para una zona.
struct ZoneGeometry {
    LayoutZone zone{LayoutZone::Content};
    LayoutRect rect{};
    bool visible{true};

    constexpr bool operator==(const ZoneGeometry& other) const = default;
};

/// @brief Reporte de evaluación geométrica y cálculo de layout de ventana.
struct MainWindowLayoutReport {
    bool valid{true};
    MainWindowLayoutError error{MainWindowLayoutError::None};
    std::vector<ZoneGeometry> allocatedZones{};
    LayoutRect contentArea{};
    std::string message{};
    std::string remediationHint{};

    bool operator==(const MainWindowLayoutReport& other) const = default;
};

} // namespace vigilio
