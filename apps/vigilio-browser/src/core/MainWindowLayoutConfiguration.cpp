#include "core/MainWindowLayoutConfiguration.h"

namespace vigilio {

MainWindowLayoutPlan MainWindowLayoutConfiguration::canonicalDefaultLayout() {
    MainWindowLayoutPlan plan{};
    plan.window = WindowDimensions{.width = kDefaultWindowWidth,
                                   .height = kDefaultWindowHeight,
                                   .minWidth = kMinWindowWidthDefault,
                                   .minHeight = kMinWindowHeightDefault};
    plan.density = LayoutDensity::Default;
    plan.chromeMode = WindowChromeMode::Standard;
    plan.sidebar = SidebarConfig{.state = SidebarState::Collapsed,
                                 .position = SidebarDockPosition::Left,
                                 .width = kCanonicalSidebarCollapsedWidth,
                                 .collapsedWidth = kCanonicalSidebarCollapsedWidth,
                                 .defaultExpandedWidth = kCanonicalSidebarDefaultExpandedWidth,
                                 .minExpandedWidth = kCanonicalSidebarMinExpandedWidth,
                                 .maxExpandedWidth = kCanonicalSidebarMaxExpandedWidth};
    plan.showTitleBar = false;
    plan.showTabStrip = true;
    plan.showToolBar = true;
    plan.showBanner = false;
    plan.showSidebar = true;
    plan.showStatusBar = true;

    plan.zones = {ZoneBudget{LayoutZone::TitleBar, false, kCanonicalTitleBarHeight,
                             kCanonicalTitleBarHeight, kCanonicalTitleBarHeight, 0, 0, 0},
                  ZoneBudget{LayoutZone::TabStrip, true, kCanonicalTabStripHeightDefault,
                             kCanonicalTabStripHeightDefault, kCanonicalTabStripHeightDefault, 0, 0, 0},
                  ZoneBudget{LayoutZone::ToolBar, true, kCanonicalToolBarHeightDefault,
                             kCanonicalToolBarHeightDefault, kCanonicalToolBarHeightDefault, 0, 0, 0},
                  ZoneBudget{LayoutZone::NotificationBanner, false, kCanonicalBannerHeightDefault,
                             kCanonicalBannerHeightDefault, kCanonicalBannerHeightDefault, 0, 0, 0},
                  ZoneBudget{LayoutZone::Sidebar, true, 0, 0, 0, kCanonicalSidebarCollapsedWidth,
                             kCanonicalSidebarCollapsedWidth, kCanonicalSidebarMaxExpandedWidth},
                  ZoneBudget{LayoutZone::Content, true, 0, kMinContentHeight, 0, 0, kMinContentWidth, 0},
                  ZoneBudget{LayoutZone::StatusBar, true, kCanonicalStatusBarHeightDefault,
                             kCanonicalStatusBarHeightDefault, kCanonicalStatusBarHeightDefault, 0, 0,
                             0}};

    return plan;
}

MainWindowLayoutPlan MainWindowLayoutConfiguration::canonicalSeniorLayout() {
    MainWindowLayoutPlan plan{};
    plan.window = WindowDimensions{.width = kDefaultWindowWidthSenior,
                                   .height = kDefaultWindowHeightSenior,
                                   .minWidth = kMinWindowWidthSenior,
                                   .minHeight = kMinWindowHeightSenior};
    plan.density = LayoutDensity::Senior;
    plan.chromeMode = WindowChromeMode::Standard;
    plan.sidebar = SidebarConfig{.state = SidebarState::Collapsed,
                                 .position = SidebarDockPosition::Left,
                                 .width = kCanonicalSidebarCollapsedWidth,
                                 .collapsedWidth = kCanonicalSidebarCollapsedWidth,
                                 .defaultExpandedWidth = kCanonicalSidebarDefaultExpandedWidth,
                                 .minExpandedWidth = kCanonicalSidebarMinExpandedWidth,
                                 .maxExpandedWidth = kCanonicalSidebarMaxExpandedWidth};
    plan.showTitleBar = false;
    plan.showTabStrip = true;
    plan.showToolBar = true;
    plan.showBanner = false;
    plan.showSidebar = true;
    plan.showStatusBar = true;

    plan.zones = {ZoneBudget{LayoutZone::TitleBar, false, kCanonicalTitleBarHeight,
                             kCanonicalTitleBarHeight, kCanonicalTitleBarHeight, 0, 0, 0},
                  ZoneBudget{LayoutZone::TabStrip, true, kCanonicalTabStripHeightSenior,
                             kCanonicalTabStripHeightSenior, kCanonicalTabStripHeightSenior, 0, 0, 0},
                  ZoneBudget{LayoutZone::ToolBar, true, kCanonicalToolBarHeightSenior,
                             kCanonicalToolBarHeightSenior, kCanonicalToolBarHeightSenior, 0, 0, 0},
                  ZoneBudget{LayoutZone::NotificationBanner, false, kCanonicalBannerHeightSenior,
                             kCanonicalBannerHeightSenior, kCanonicalBannerHeightSenior, 0, 0, 0},
                  ZoneBudget{LayoutZone::Sidebar, true, 0, 0, 0, kCanonicalSidebarCollapsedWidth,
                             kCanonicalSidebarCollapsedWidth, kCanonicalSidebarMaxExpandedWidth},
                  ZoneBudget{LayoutZone::Content, true, 0, kMinContentHeight, 0, 0, kMinContentWidth, 0},
                  ZoneBudget{LayoutZone::StatusBar, true, kCanonicalStatusBarHeightSenior,
                             kCanonicalStatusBarHeightSenior, kCanonicalStatusBarHeightSenior, 0, 0, 0}};

    return plan;
}

MainWindowLayoutPlan MainWindowLayoutConfiguration::canonicalCompactLayout() {
    MainWindowLayoutPlan plan{};
    plan.window = WindowDimensions{.width = kDefaultWindowWidthCompact,
                                   .height = kDefaultWindowHeightCompact,
                                   .minWidth = kMinWindowWidthCompact,
                                   .minHeight = kMinWindowHeightCompact};
    plan.density = LayoutDensity::Compact;
    plan.chromeMode = WindowChromeMode::Standard;
    plan.sidebar = SidebarConfig{.state = SidebarState::Collapsed,
                                 .position = SidebarDockPosition::Left,
                                 .width = kCanonicalSidebarCollapsedWidth,
                                 .collapsedWidth = kCanonicalSidebarCollapsedWidth,
                                 .defaultExpandedWidth = kCanonicalSidebarDefaultExpandedWidth,
                                 .minExpandedWidth = kCanonicalSidebarMinExpandedWidth,
                                 .maxExpandedWidth = kCanonicalSidebarMaxExpandedWidth};
    plan.showTitleBar = false;
    plan.showTabStrip = true;
    plan.showToolBar = true;
    plan.showBanner = false;
    plan.showSidebar = true;
    plan.showStatusBar = true;

    plan.zones = {ZoneBudget{LayoutZone::TitleBar, false, kCanonicalTitleBarHeight,
                             kCanonicalTitleBarHeight, kCanonicalTitleBarHeight, 0, 0, 0},
                  ZoneBudget{LayoutZone::TabStrip, true, kCanonicalTabStripHeightCompact,
                             kCanonicalTabStripHeightCompact, kCanonicalTabStripHeightCompact, 0, 0, 0},
                  ZoneBudget{LayoutZone::ToolBar, true, kCanonicalToolBarHeightCompact,
                             kCanonicalToolBarHeightCompact, kCanonicalToolBarHeightCompact, 0, 0, 0},
                  ZoneBudget{LayoutZone::NotificationBanner, false, kCanonicalBannerHeightCompact,
                             kCanonicalBannerHeightCompact, kCanonicalBannerHeightCompact, 0, 0, 0},
                  ZoneBudget{LayoutZone::Sidebar, true, 0, 0, 0, kCanonicalSidebarCollapsedWidth,
                             kCanonicalSidebarCollapsedWidth, kCanonicalSidebarMaxExpandedWidth},
                  ZoneBudget{LayoutZone::Content, true, 0, kMinContentHeight, 0, 0, kMinContentWidth, 0},
                  ZoneBudget{LayoutZone::StatusBar, true, kCanonicalStatusBarHeightCompact,
                             kCanonicalStatusBarHeightCompact, kCanonicalStatusBarHeightCompact, 0, 0,
                             0}};

    return plan;
}

bool MainWindowLayoutConfiguration::verifyInvariants(const MainWindowLayoutPlan& plan) noexcept {
    // 1. Límites y no negatividad de dimensiones de ventana
    if (plan.window.width <= 0 || plan.window.height <= 0 || plan.window.minWidth <= 0 ||
        plan.window.minHeight <= 0) {
        return false;
    }
    if (plan.window.width < plan.window.minWidth || plan.window.height < plan.window.minHeight) {
        return false;
    }

    // Comprobación de mínimos según la densidad activa
    switch (plan.density) {
    case LayoutDensity::Default:
        if (plan.window.minWidth < kMinWindowWidthDefault ||
            plan.window.minHeight < kMinWindowHeightDefault) {
            return false;
        }
        break;
    case LayoutDensity::Senior:
        if (plan.window.minWidth < kMinWindowWidthSenior ||
            plan.window.minHeight < kMinWindowHeightSenior) {
            return false;
        }
        break;
    case LayoutDensity::Compact:
        if (plan.window.minWidth < kMinWindowWidthCompact ||
            plan.window.minHeight < kMinWindowHeightCompact) {
            return false;
        }
        break;
    }

    // 2. Invariantes de la barra lateral
    if (plan.sidebar.collapsedWidth != kCanonicalSidebarCollapsedWidth) {
        return false;
    }
    if (plan.sidebar.minExpandedWidth < kCanonicalSidebarMinExpandedWidth ||
        plan.sidebar.maxExpandedWidth > kCanonicalSidebarMaxExpandedWidth ||
        plan.sidebar.minExpandedWidth > plan.sidebar.maxExpandedWidth) {
        return false;
    }
    if (plan.sidebar.state == SidebarState::Collapsed) {
        if (plan.sidebar.width != plan.sidebar.collapsedWidth) {
            return false;
        }
    } else if (plan.sidebar.state == SidebarState::Expanded) {
        if (plan.sidebar.width < plan.sidebar.minExpandedWidth ||
            plan.sidebar.width > plan.sidebar.maxExpandedWidth) {
            return false;
        }
    } else if (plan.sidebar.state == SidebarState::Hidden) {
        if (plan.sidebar.width < 0) {
            return false;
        }
    }

    // 3. Invariantes específicas de la «Interfaz más fácil de usar» (Modo Senior)
    if (plan.density == LayoutDensity::Senior) {
        for (const auto& zone : plan.zones) {
            if (zone.zone == LayoutZone::ToolBar && zone.visible) {
                if (zone.preferredHeight < kCanonicalToolBarHeightSenior ||
                    zone.minHeight < kCanonicalToolBarHeightSenior) {
                    return false;
                }
            }
            if (zone.zone == LayoutZone::TabStrip && zone.visible) {
                if (zone.preferredHeight < kCanonicalTabStripHeightSenior ||
                    zone.minHeight < kCanonicalTabStripHeightSenior) {
                    return false;
                }
            }
        }
    }

    // 4. Presupuesto mínimo de área central de contenido
    int sidebarWidth = 0;
    if (plan.showSidebar && plan.sidebar.state != SidebarState::Hidden) {
        sidebarWidth = (plan.sidebar.state == SidebarState::Collapsed) ? plan.sidebar.collapsedWidth
                                                                       : plan.sidebar.width;
    }
    int contentWidth = plan.window.width - sidebarWidth;
    if (contentWidth < kMinContentWidth) {
        return false;
    }

    int topBars = 0;
    if (plan.showTitleBar) {
        topBars += kCanonicalTitleBarHeight;
    }
    if (plan.showTabStrip) {
        int tabH = (plan.density == LayoutDensity::Senior)    ? kCanonicalTabStripHeightSenior
                   : (plan.density == LayoutDensity::Compact) ? kCanonicalTabStripHeightCompact
                                                              : kCanonicalTabStripHeightDefault;
        topBars += tabH;
    }
    if (plan.showToolBar) {
        int toolH = (plan.density == LayoutDensity::Senior)    ? kCanonicalToolBarHeightSenior
                    : (plan.density == LayoutDensity::Compact) ? kCanonicalToolBarHeightCompact
                                                               : kCanonicalToolBarHeightDefault;
        topBars += toolH;
    }
    if (plan.showBanner) {
        int banH = (plan.density == LayoutDensity::Senior)    ? kCanonicalBannerHeightSenior
                   : (plan.density == LayoutDensity::Compact) ? kCanonicalBannerHeightCompact
                                                              : kCanonicalBannerHeightDefault;
        topBars += banH;
    }
    int bottomBars = 0;
    if (plan.showStatusBar) {
        int statH = (plan.density == LayoutDensity::Senior)    ? kCanonicalStatusBarHeightSenior
                    : (plan.density == LayoutDensity::Compact) ? kCanonicalStatusBarHeightCompact
                                                               : kCanonicalStatusBarHeightDefault;
        bottomBars += statH;
    }

    int contentHeight = plan.window.height - topBars - bottomBars;
    if (contentHeight < kMinContentHeight) {
        return false;
    }

    return true;
}

} // namespace vigilio
