#include "core/MainWindowLayoutValidator.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vigilio {

MainWindowLayoutError MainWindowLayoutValidator::validateDimensions(const WindowDimensions& dimensions,
                                                                    LayoutDensity density) noexcept {
    if (dimensions.width < 0 || dimensions.height < 0 || dimensions.minWidth < 0 ||
        dimensions.minHeight < 0) {
        return MainWindowLayoutError::NegativeMarginOrPadding;
    }

    int thresholdW = MainWindowLayoutConfiguration::kMinWindowWidthDefault;
    int thresholdH = MainWindowLayoutConfiguration::kMinWindowHeightDefault;

    switch (density) {
    case LayoutDensity::Default:
        thresholdW = MainWindowLayoutConfiguration::kMinWindowWidthDefault;
        thresholdH = MainWindowLayoutConfiguration::kMinWindowHeightDefault;
        break;
    case LayoutDensity::Senior:
        thresholdW = MainWindowLayoutConfiguration::kMinWindowWidthSenior;
        thresholdH = MainWindowLayoutConfiguration::kMinWindowHeightSenior;
        break;
    case LayoutDensity::Compact:
        thresholdW = MainWindowLayoutConfiguration::kMinWindowWidthCompact;
        thresholdH = MainWindowLayoutConfiguration::kMinWindowHeightCompact;
        break;
    }

    if (dimensions.width < thresholdW || dimensions.height < thresholdH) {
        return MainWindowLayoutError::DimensionsBelowMinimum;
    }
    if (dimensions.minWidth < thresholdW || dimensions.minHeight < thresholdH) {
        return MainWindowLayoutError::DimensionsBelowMinimum;
    }
    if (dimensions.width < dimensions.minWidth || dimensions.height < dimensions.minHeight) {
        return MainWindowLayoutError::DimensionsBelowMinimum;
    }

    return MainWindowLayoutError::None;
}

MainWindowLayoutError MainWindowLayoutValidator::validateSidebar(const SidebarConfig& sidebar) noexcept {
    if (sidebar.width < 0 || sidebar.collapsedWidth < 0 || sidebar.minExpandedWidth < 0 ||
        sidebar.maxExpandedWidth < 0) {
        return MainWindowLayoutError::NegativeMarginOrPadding;
    }

    if (sidebar.collapsedWidth != MainWindowLayoutConfiguration::kCanonicalSidebarCollapsedWidth) {
        return MainWindowLayoutError::InvalidSidebarWidth;
    }
    if (sidebar.minExpandedWidth < MainWindowLayoutConfiguration::kCanonicalSidebarMinExpandedWidth ||
        sidebar.maxExpandedWidth > MainWindowLayoutConfiguration::kCanonicalSidebarMaxExpandedWidth ||
        sidebar.minExpandedWidth > sidebar.maxExpandedWidth) {
        return MainWindowLayoutError::InvalidSidebarWidth;
    }

    if (sidebar.state == SidebarState::Collapsed) {
        if (sidebar.width != sidebar.collapsedWidth) {
            return MainWindowLayoutError::InvalidSidebarWidth;
        }
    } else if (sidebar.state == SidebarState::Expanded) {
        if (sidebar.width < sidebar.minExpandedWidth || sidebar.width > sidebar.maxExpandedWidth) {
            return MainWindowLayoutError::InvalidSidebarWidth;
        }
    }

    return MainWindowLayoutError::None;
}

MainWindowLayoutError
MainWindowLayoutValidator::validatePlan(const MainWindowLayoutPlan& plan) noexcept {
    const auto dimErr = validateDimensions(plan.window, plan.density);
    if (dimErr != MainWindowLayoutError::None) {
        return dimErr;
    }

    const auto sideErr = validateSidebar(plan.sidebar);
    if (sideErr != MainWindowLayoutError::None) {
        return sideErr;
    }

    if (plan.density == LayoutDensity::Senior) {
        for (const auto& zone : plan.zones) {
            if (zone.zone == LayoutZone::ToolBar && zone.visible) {
                if (zone.preferredHeight <
                        MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior ||
                    zone.minHeight < MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior) {
                    return MainWindowLayoutError::InvalidDensityConfiguration;
                }
            }
            if (zone.zone == LayoutZone::TabStrip && zone.visible) {
                if (zone.preferredHeight <
                        MainWindowLayoutConfiguration::kCanonicalTabStripHeightSenior ||
                    zone.minHeight < MainWindowLayoutConfiguration::kCanonicalTabStripHeightSenior) {
                    return MainWindowLayoutError::InvalidDensityConfiguration;
                }
            }
        }
    }

    if (!MainWindowLayoutConfiguration::verifyInvariants(plan)) {
        int sidebarWidth = 0;
        if (plan.showSidebar && plan.sidebar.state != SidebarState::Hidden) {
            sidebarWidth = (plan.sidebar.state == SidebarState::Collapsed) ? plan.sidebar.collapsedWidth
                                                                           : plan.sidebar.width;
        }
        int contentWidth = plan.window.width - sidebarWidth;
        if (contentWidth < MainWindowLayoutConfiguration::kMinContentWidth) {
            return MainWindowLayoutError::ZeroContentAreaBudget;
        }

        int topBars = 0;
        if (plan.showTitleBar) {
            topBars += MainWindowLayoutConfiguration::kCanonicalTitleBarHeight;
        }
        if (plan.showTabStrip) {
            topBars += (plan.density == LayoutDensity::Senior)
                           ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightSenior
                       : (plan.density == LayoutDensity::Compact)
                           ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightCompact
                           : MainWindowLayoutConfiguration::kCanonicalTabStripHeightDefault;
        }
        if (plan.showToolBar) {
            topBars += (plan.density == LayoutDensity::Senior)
                           ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior
                       : (plan.density == LayoutDensity::Compact)
                           ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightCompact
                           : MainWindowLayoutConfiguration::kCanonicalToolBarHeightDefault;
        }
        if (plan.showBanner) {
            topBars += (plan.density == LayoutDensity::Senior)
                           ? MainWindowLayoutConfiguration::kCanonicalBannerHeightSenior
                       : (plan.density == LayoutDensity::Compact)
                           ? MainWindowLayoutConfiguration::kCanonicalBannerHeightCompact
                           : MainWindowLayoutConfiguration::kCanonicalBannerHeightDefault;
        }
        int bottomBars = 0;
        if (plan.showStatusBar) {
            bottomBars += (plan.density == LayoutDensity::Senior)
                              ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightSenior
                          : (plan.density == LayoutDensity::Compact)
                              ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightCompact
                              : MainWindowLayoutConfiguration::kCanonicalStatusBarHeightDefault;
        }

        if (plan.window.height - topBars - bottomBars <
            MainWindowLayoutConfiguration::kMinContentHeight) {
            return MainWindowLayoutError::ZeroContentAreaBudget;
        }

        return MainWindowLayoutError::InvalidDensityConfiguration;
    }

    return MainWindowLayoutError::None;
}

MainWindowLayoutReport
MainWindowLayoutValidator::calculateLayoutAreas(int windowWidth, int windowHeight,
                                                const MainWindowLayoutPlan& plan) {
    MainWindowLayoutReport report{};

    if (windowWidth <= 0 || windowHeight <= 0) {
        report.valid = false;
        report.error = (windowWidth < 0 || windowHeight < 0)
                           ? MainWindowLayoutError::NegativeMarginOrPadding
                           : MainWindowLayoutError::DimensionsBelowMinimum;
        report.message = layoutErrorToString(report.error);
        report.remediationHint = generateRemediationHint(
            report.error, "Las dimensiones de la ventana deben ser mayores que cero.");
        return report;
    }

    WindowDimensions dims = plan.window;
    dims.width = windowWidth;
    dims.height = windowHeight;

    const auto dimErr = validateDimensions(dims, plan.density);
    if (dimErr != MainWindowLayoutError::None) {
        report.valid = false;
        report.error = dimErr;
        report.message = layoutErrorToString(dimErr);
        report.remediationHint = generateRemediationHint(dimErr);
        return report;
    }

    const auto sideErr = validateSidebar(plan.sidebar);
    if (sideErr != MainWindowLayoutError::None) {
        report.valid = false;
        report.error = sideErr;
        report.message = layoutErrorToString(sideErr);
        report.remediationHint = generateRemediationHint(sideErr);
        return report;
    }

    auto getZoneHeight = [&](LayoutZone zone) -> int {
        for (const auto& zb : plan.zones) {
            if (zb.zone == zone && zb.preferredHeight > 0) {
                return zb.preferredHeight;
            }
        }
        switch (zone) {
        case LayoutZone::TitleBar:
            return MainWindowLayoutConfiguration::kCanonicalTitleBarHeight;
        case LayoutZone::TabStrip:
            return (plan.density == LayoutDensity::Senior)
                       ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightSenior
                   : (plan.density == LayoutDensity::Compact)
                       ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightCompact
                       : MainWindowLayoutConfiguration::kCanonicalTabStripHeightDefault;
        case LayoutZone::ToolBar:
            return (plan.density == LayoutDensity::Senior)
                       ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior
                   : (plan.density == LayoutDensity::Compact)
                       ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightCompact
                       : MainWindowLayoutConfiguration::kCanonicalToolBarHeightDefault;
        case LayoutZone::NotificationBanner:
            return (plan.density == LayoutDensity::Senior)
                       ? MainWindowLayoutConfiguration::kCanonicalBannerHeightSenior
                   : (plan.density == LayoutDensity::Compact)
                       ? MainWindowLayoutConfiguration::kCanonicalBannerHeightCompact
                       : MainWindowLayoutConfiguration::kCanonicalBannerHeightDefault;
        case LayoutZone::StatusBar:
            return (plan.density == LayoutDensity::Senior)
                       ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightSenior
                   : (plan.density == LayoutDensity::Compact)
                       ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightCompact
                       : MainWindowLayoutConfiguration::kCanonicalStatusBarHeightDefault;
        default:
            return 0;
        }
    };

    int yTop = 0;
    std::vector<ZoneGeometry> allocated;

    if (plan.showTitleBar) {
        const int h = getZoneHeight(LayoutZone::TitleBar);
        allocated.push_back({LayoutZone::TitleBar, {0, yTop, windowWidth, h}, true});
        yTop += h;
    }
    if (plan.showTabStrip) {
        const int h = getZoneHeight(LayoutZone::TabStrip);
        allocated.push_back({LayoutZone::TabStrip, {0, yTop, windowWidth, h}, true});
        yTop += h;
    }
    if (plan.showToolBar) {
        const int h = getZoneHeight(LayoutZone::ToolBar);
        allocated.push_back({LayoutZone::ToolBar, {0, yTop, windowWidth, h}, true});
        yTop += h;
    }
    if (plan.showBanner) {
        const int h = getZoneHeight(LayoutZone::NotificationBanner);
        allocated.push_back({LayoutZone::NotificationBanner, {0, yTop, windowWidth, h}, true});
        yTop += h;
    }

    int hStatus = 0;
    if (plan.showStatusBar) {
        hStatus = getZoneHeight(LayoutZone::StatusBar);
    }
    const int yBottom = windowHeight - hStatus;
    const int hCenter = yBottom - yTop;

    if (hCenter < MainWindowLayoutConfiguration::kMinContentHeight) {
        report.valid = false;
        report.error = MainWindowLayoutError::ZeroContentAreaBudget;
        report.message = layoutErrorToString(report.error);
        report.remediationHint = generateRemediationHint(
            report.error, "La altura vertical para el contenido es inferior a 250 píxeles.");
        return report;
    }

    int wSidebar = 0;
    if (plan.showSidebar && plan.sidebar.state != SidebarState::Hidden) {
        wSidebar = (plan.sidebar.state == SidebarState::Collapsed) ? plan.sidebar.collapsedWidth
                                                                   : plan.sidebar.width;
    }
    const int wContent = windowWidth - wSidebar;

    if (wContent < MainWindowLayoutConfiguration::kMinContentWidth) {
        report.valid = false;
        report.error = MainWindowLayoutError::ZeroContentAreaBudget;
        report.message = layoutErrorToString(report.error);
        report.remediationHint = generateRemediationHint(
            report.error, "El ancho disponible para el contenido es inferior a 400 píxeles.");
        return report;
    }

    LayoutRect sidebarRect{};
    LayoutRect contentRect{};
    if (plan.sidebar.position == SidebarDockPosition::Left) {
        sidebarRect = {0, yTop, wSidebar, hCenter};
        contentRect = {wSidebar, yTop, wContent, hCenter};
    } else {
        contentRect = {0, yTop, wContent, hCenter};
        sidebarRect = {wContent, yTop, wSidebar, hCenter};
    }

    if (plan.showSidebar && plan.sidebar.state != SidebarState::Hidden) {
        allocated.push_back({LayoutZone::Sidebar, sidebarRect, true});
    }
    allocated.push_back({LayoutZone::Content, contentRect, true});

    if (plan.showStatusBar) {
        allocated.push_back({LayoutZone::StatusBar, {0, yBottom, windowWidth, hStatus}, true});
    }

    for (size_t i = 0; i < allocated.size(); ++i) {
        for (size_t j = i + 1; j < allocated.size(); ++j) {
            if (allocated[i].rect.intersects(allocated[j].rect)) {
                report.valid = false;
                report.error = MainWindowLayoutError::ConflictingZoneAnchors;
                report.message = layoutErrorToString(report.error);
                report.remediationHint =
                    generateRemediationHint(report.error, "Se detectó solapamiento entre zonas.");
                return report;
            }
        }
    }

    report.valid = true;
    report.error = MainWindowLayoutError::None;
    report.allocatedZones = std::move(allocated);
    report.contentArea = contentRect;
    report.message = layoutErrorToString(MainWindowLayoutError::None);
    report.remediationHint.clear();
    return report;
}

std::string MainWindowLayoutValidator::layoutErrorToString(MainWindowLayoutError error) {
    switch (error) {
    case MainWindowLayoutError::None:
        return "Disposición correcta sin anomalías";
    case MainWindowLayoutError::DimensionsBelowMinimum:
        return "Dimensiones de ventana por debajo de los límites mínimos requeridos";
    case MainWindowLayoutError::InvalidSidebarWidth:
        return "Ancho de la barra lateral fuera del rango permitido";
    case MainWindowLayoutError::ConflictingZoneAnchors:
        return "Conflicto o solapamiento en la distribución espacial de zonas";
    case MainWindowLayoutError::ZeroContentAreaBudget:
        return "Espacio insuficiente reservado para el área central de contenido";
    case MainWindowLayoutError::NegativeMarginOrPadding:
        return "Márgenes o dimensiones geométricas negativas no permitidas";
    case MainWindowLayoutError::InvalidDensityConfiguration:
        return "Configuración de densidad visual incompatible con los parámetros de la ventana";
    case MainWindowLayoutError::LayoutPersistenceCorrupted:
        return "Configuración persistida de la ventana dañada o ilegible";
    }
    return "Error de disposición de ventana desconocido";
}

std::string MainWindowLayoutValidator::layoutZoneToString(LayoutZone zone) {
    switch (zone) {
    case LayoutZone::TitleBar:
        return "Barra de título";
    case LayoutZone::TabStrip:
        return "Tira de pestañas";
    case LayoutZone::ToolBar:
        return "Barra de navegación";
    case LayoutZone::NotificationBanner:
        return "Aviso de notificación";
    case LayoutZone::Sidebar:
        return "Barra lateral";
    case LayoutZone::Content:
        return "Área de contenido";
    case LayoutZone::StatusBar:
        return "Barra de estado";
    }
    return "Zona no identificada";
}

std::string MainWindowLayoutValidator::layoutDensityToString(LayoutDensity density) {
    switch (density) {
    case LayoutDensity::Default:
        return "Densidad estándar";
    case LayoutDensity::Senior:
        return "Interfaz más fácil de usar";
    case LayoutDensity::Compact:
        return "Densidad compacta";
    }
    return "Densidad no especificada";
}

std::string MainWindowLayoutValidator::sidebarStateToString(SidebarState state) {
    switch (state) {
    case SidebarState::Hidden:
        return "Oculta";
    case SidebarState::Collapsed:
        return "Plegada";
    case SidebarState::Expanded:
        return "Expandida";
    }
    return "Estado de barra lateral desconocido";
}

std::string MainWindowLayoutValidator::sidebarDockPositionToString(SidebarDockPosition position) {
    switch (position) {
    case SidebarDockPosition::Left:
        return "Izquierda";
    case SidebarDockPosition::Right:
        return "Derecha";
    }
    return "Posición no identificada";
}

std::string MainWindowLayoutValidator::chromeModeToString(WindowChromeMode mode) {
    switch (mode) {
    case WindowChromeMode::Standard:
        return "Estándar";
    case WindowChromeMode::Integrated:
        return "Integrado";
    case WindowChromeMode::Fullscreen:
        return "Pantalla completa";
    }
    return "Modalidad desconocida";
}

std::string MainWindowLayoutValidator::generateRemediationHint(MainWindowLayoutError error,
                                                               std::string_view details) {
    std::string hint;
    switch (error) {
    case MainWindowLayoutError::None:
        hint = "No se requiere ninguna acción correctiva.";
        break;
    case MainWindowLayoutError::DimensionsBelowMinimum:
        hint = "Aumente el tamaño de la ventana para cumplir los límites mínimos de visualización.";
        break;
    case MainWindowLayoutError::InvalidSidebarWidth:
        hint = "Ajuste el ancho de la barra lateral al rango permitido entre 200 y 360 píxeles o "
               "manténgala plegada en 48 píxeles.";
        break;
    case MainWindowLayoutError::ConflictingZoneAnchors:
        hint = "Revise la distribución de las zonas para evitar solapamientos en la ventana.";
        break;
    case MainWindowLayoutError::ZeroContentAreaBudget:
        hint = "Reduzca el tamaño o la visibilidad de las barras para garantizar el espacio necesario "
               "para el contenido.";
        break;
    case MainWindowLayoutError::NegativeMarginOrPadding:
        hint = "Asegúrese de que todas las medidas y márgenes sean valores positivos o cero.";
        break;
    case MainWindowLayoutError::InvalidDensityConfiguration:
        hint = "Seleccione una densidad visual compatible con las dimensiones de la pantalla.";
        break;
    case MainWindowLayoutError::LayoutPersistenceCorrupted:
        hint = "Se restablecerá automáticamente la disposición predeterminada de la ventana.";
        break;
    }

    if (!details.empty()) {
        hint += " Detalle: ";
        hint.append(details);
    }

    return hint;
}

} // namespace vigilio
