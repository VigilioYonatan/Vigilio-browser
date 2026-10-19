#include "core/TitleBarConfiguration.h"
#include "core/TitleBarValidator.h"

namespace vigilio {

TitleBarLayoutPlan TitleBarConfiguration::canonicalDefaultTitleBar(int windowWidth) {
    TitleBarConfig config{};
    config.mode = TitleBarMode::CustomIntegrated;
    config.placement = TitleBarControlsPlacement::Right;
    config.title = "Vigilio Browser";
    config.showIcon = true;
    config.showSeniorToggle = false;
    config.density = LayoutDensity::Default;
    config.state = TitleBarState{
        .isMaximized = false, .isActive = true, .isFullscreen = false, .seniorModeActive = false};

    const auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth, config);
    return report.plan;
}

TitleBarLayoutPlan TitleBarConfiguration::canonicalSeniorTitleBar(int windowWidth) {
    TitleBarConfig config{};
    config.mode = TitleBarMode::CustomIntegrated;
    config.placement = TitleBarControlsPlacement::Right;
    config.title = "Vigilio Browser";
    config.showIcon = true;
    config.showSeniorToggle = true;
    config.density = LayoutDensity::Senior;
    config.state = TitleBarState{
        .isMaximized = false, .isActive = true, .isFullscreen = false, .seniorModeActive = true};

    const auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth, config);
    return report.plan;
}

TitleBarLayoutPlan TitleBarConfiguration::canonicalCompactTitleBar(int windowWidth) {
    TitleBarConfig config{};
    config.mode = TitleBarMode::CustomIntegrated;
    config.placement = TitleBarControlsPlacement::Right;
    config.title = "Vigilio Browser";
    config.showIcon = true;
    config.showSeniorToggle = false;
    config.density = LayoutDensity::Compact;
    config.state = TitleBarState{
        .isMaximized = false, .isActive = true, .isFullscreen = false, .seniorModeActive = false};

    const auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth, config);
    return report.plan;
}

bool TitleBarConfiguration::verifyInvariants(const TitleBarLayoutPlan& plan) noexcept {
    if (plan.windowWidth < 0 || plan.height < 0) {
        return false;
    }

    if (plan.mode == TitleBarMode::Suppressed || plan.mode == TitleBarMode::NativeSystem) {
        return true;
    }

    // 1. Umbrales dimensionales de ventana y altura de barra
    switch (plan.density) {
    case LayoutDensity::Default:
        if (plan.windowWidth < kMinWindowWidthDefault || plan.height < kDefaultTitleBarHeight) {
            return false;
        }
        break;
    case LayoutDensity::Senior:
        // Invariante de accesibilidad: tap targets y barra >= 48 px obligatorios
        if (plan.windowWidth < kMinWindowWidthSenior || plan.height < kSeniorTitleBarHeight) {
            return false;
        }
        break;
    case LayoutDensity::Compact:
        if (plan.windowWidth < kMinWindowWidthCompact || plan.height < kCompactTitleBarHeight) {
            return false;
        }
        break;
    }

    // 2. Verificación de dimensiones de botones de control
    for (const auto& ctrl : plan.controls) {
        if (!ctrl.visible) {
            continue;
        }
        if (plan.density == LayoutDensity::Senior) {
            if (ctrl.rect.width < kSeniorButtonWidth || ctrl.rect.height < kSeniorButtonHeight) {
                return false;
            }
        } else if (plan.density == LayoutDensity::Default) {
            if (ctrl.rect.width < 30 || ctrl.rect.height < kDefaultButtonHeight) {
                return false;
            }
        } else if (plan.density == LayoutDensity::Compact) {
            if (ctrl.rect.width < 24 || ctrl.rect.height < kCompactButtonHeight) {
                return false;
            }
        }
    }

    // 3. Disyunción espacial de controles (no solapamiento: Ci ∩ Cj = ∅)
    const std::size_t ctrlCount = plan.controls.size();
    for (std::size_t i = 0; i < ctrlCount; ++i) {
        if (!plan.controls[i].visible) {
            continue;
        }
        for (std::size_t j = i + 1; j < ctrlCount; ++j) {
            if (!plan.controls[j].visible) {
                continue;
            }
            if (plan.controls[i].rect.intersects(plan.controls[j].rect)) {
                return false;
            }
        }
    }

    // 4. Disyunción entre bloques mayores
    if (plan.controlsBoxRect.width > 0 && plan.controlsBoxRect.height > 0) {
        if (plan.draggableRect.width > 0 && plan.controlsBoxRect.intersects(plan.draggableRect)) {
            return false;
        }
        if (plan.titleRect.width > 0 && plan.controlsBoxRect.intersects(plan.titleRect)) {
            return false;
        }
        if (plan.iconRect.width > 0 && plan.controlsBoxRect.intersects(plan.iconRect)) {
            return false;
        }
    }

    if (plan.draggableRect.width > 0 && plan.titleRect.width > 0) {
        if (plan.draggableRect.intersects(plan.titleRect)) {
            return false;
        }
    }
    if (plan.draggableRect.width > 0 && plan.iconRect.width > 0) {
        if (plan.draggableRect.intersects(plan.iconRect)) {
            return false;
        }
    }
    if (plan.titleRect.width > 0 && plan.iconRect.width > 0) {
        if (plan.titleRect.intersects(plan.iconRect)) {
            return false;
        }
    }

    // 5. Suficiencia de área de arrastre
    int minDraggable = kMinDraggableAreaWidthDefault;
    if (plan.density == LayoutDensity::Senior) {
        minDraggable = kMinDraggableAreaWidthSenior;
    } else if (plan.density == LayoutDensity::Compact) {
        minDraggable = kMinDraggableAreaWidthCompact;
    }

    if (plan.draggableRect.width < minDraggable) {
        return false;
    }

    // 6. Límites de contención respecto al marco de ventana
    auto checkBounds = [&](const LayoutRect& r) {
        if (r.width <= 0 || r.height <= 0) {
            return true;
        }
        return r.x >= 0 && r.y >= 0 && (r.x + r.width) <= plan.windowWidth &&
               (r.y + r.height) <= plan.height;
    };

    if (!checkBounds(plan.controlsBoxRect) || !checkBounds(plan.draggableRect) ||
        !checkBounds(plan.titleRect) || !checkBounds(plan.iconRect)) {
        return false;
    }

    for (const auto& ctrl : plan.controls) {
        if (ctrl.visible && !checkBounds(ctrl.rect)) {
            return false;
        }
    }

    return true;
}

} // namespace vigilio
