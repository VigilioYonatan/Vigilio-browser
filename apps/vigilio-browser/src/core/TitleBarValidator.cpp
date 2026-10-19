#include "core/TitleBarValidator.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

TitleBarError TitleBarValidator::validateDimensions(int width, int height,
                                                    LayoutDensity density) noexcept {
    if (width < 0 || height < 0) {
        return TitleBarError::DimensionsBelowMinimum;
    }

    switch (density) {
    case LayoutDensity::Default:
        if (width < TitleBarConfiguration::kMinWindowWidthDefault) {
            return TitleBarError::DimensionsBelowMinimum;
        }
        if (height < TitleBarConfiguration::kDefaultTitleBarHeight) {
            return TitleBarError::DimensionsBelowMinimum;
        }
        break;
    case LayoutDensity::Senior:
        // Invariante de accesibilidad: altura mínima obligatoria de 48 px
        if (height < TitleBarConfiguration::kSeniorTitleBarHeight) {
            return TitleBarError::SeniorModeMinimumHeightViolation;
        }
        if (width < TitleBarConfiguration::kMinWindowWidthSenior) {
            return TitleBarError::DimensionsBelowMinimum;
        }
        break;
    case LayoutDensity::Compact:
        if (width < TitleBarConfiguration::kMinWindowWidthCompact) {
            return TitleBarError::DimensionsBelowMinimum;
        }
        if (height < TitleBarConfiguration::kCompactTitleBarHeight) {
            return TitleBarError::DimensionsBelowMinimum;
        }
        break;
    }

    return TitleBarError::None;
}

TitleBarError TitleBarValidator::validateDimensions(int width, LayoutDensity density) noexcept {
    int defaultHeight = TitleBarConfiguration::kDefaultTitleBarHeight;
    if (density == LayoutDensity::Senior) {
        defaultHeight = TitleBarConfiguration::kSeniorTitleBarHeight;
    } else if (density == LayoutDensity::Compact) {
        defaultHeight = TitleBarConfiguration::kCompactTitleBarHeight;
    }
    return validateDimensions(width, defaultHeight, density);
}

TitleBarError TitleBarValidator::validatePlan(const TitleBarLayoutPlan& plan) noexcept {
    if (plan.mode == TitleBarMode::Suppressed || plan.mode == TitleBarMode::NativeSystem) {
        return TitleBarError::None;
    }

    if (plan.density == LayoutDensity::Senior &&
        plan.height < TitleBarConfiguration::kSeniorTitleBarHeight) {
        return TitleBarError::SeniorModeMinimumHeightViolation;
    }

    const auto dimErr = validateDimensions(plan.windowWidth, plan.height, plan.density);
    if (dimErr != TitleBarError::None) {
        return dimErr;
    }

    if (plan.controlsBoxRect.width > plan.windowWidth) {
        return TitleBarError::ControlsOverflowWidth;
    }

    int minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthDefault;
    if (plan.density == LayoutDensity::Senior) {
        minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthSenior;
    } else if (plan.density == LayoutDensity::Compact) {
        minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthCompact;
    }

    if (plan.draggableRect.width < minDraggable) {
        return TitleBarError::DraggableAreaInsufficient;
    }

    // Comprobación de no solapamiento entre botones
    const std::size_t count = plan.controls.size();
    for (std::size_t i = 0; i < count; ++i) {
        if (!plan.controls[i].visible) {
            continue;
        }
        for (std::size_t j = i + 1; j < count; ++j) {
            if (!plan.controls[j].visible) {
                continue;
            }
            if (plan.controls[i].rect.intersects(plan.controls[j].rect)) {
                return TitleBarError::ConflictingControlPlacement;
            }
        }
    }

    if (!TitleBarConfiguration::verifyInvariants(plan)) {
        return TitleBarError::ConflictingControlPlacement;
    }

    return TitleBarError::None;
}

TitleBarReport TitleBarValidator::calculateTitleBarLayout(int windowWidth,
                                                          const TitleBarConfig& config) {
    int height = TitleBarConfiguration::kDefaultTitleBarHeight;
    int iconSize = TitleBarConfiguration::kDefaultIconSize;
    int iconMargin = TitleBarConfiguration::kDefaultIconMargin;
    int buttonWidth = TitleBarConfiguration::kDefaultButtonWidth;
    int buttonHeight = TitleBarConfiguration::kDefaultButtonHeight;
    int buttonSpacing = TitleBarConfiguration::kDefaultButtonSpacing;
    int minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthDefault;
    int minWindowWidth = TitleBarConfiguration::kMinWindowWidthDefault;

    if (config.density == LayoutDensity::Senior) {
        height = TitleBarConfiguration::kSeniorTitleBarHeight;
        iconSize = TitleBarConfiguration::kSeniorIconSize;
        iconMargin = TitleBarConfiguration::kSeniorIconMargin;
        buttonWidth = TitleBarConfiguration::kSeniorButtonWidth;
        buttonHeight = TitleBarConfiguration::kSeniorButtonHeight;
        buttonSpacing = TitleBarConfiguration::kSeniorButtonSpacing;
        minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthSenior;
        minWindowWidth = TitleBarConfiguration::kMinWindowWidthSenior;
    } else if (config.density == LayoutDensity::Compact) {
        height = TitleBarConfiguration::kCompactTitleBarHeight;
        iconSize = TitleBarConfiguration::kCompactIconSize;
        iconMargin = TitleBarConfiguration::kCompactIconMargin;
        buttonWidth = TitleBarConfiguration::kCompactButtonWidth;
        buttonHeight = TitleBarConfiguration::kCompactButtonHeight;
        buttonSpacing = TitleBarConfiguration::kCompactButtonSpacing;
        minDraggable = TitleBarConfiguration::kMinDraggableAreaWidthCompact;
        minWindowWidth = TitleBarConfiguration::kMinWindowWidthCompact;
    }

    TitleBarLayoutPlan plan{};
    plan.windowWidth = windowWidth;
    plan.height = height;
    plan.density = config.density;
    plan.mode = config.mode;
    plan.placement = config.placement;
    plan.state = config.state;

    // Manejo de modos sin decoración personalizada integrada
    if (config.mode == TitleBarMode::Suppressed) {
        plan.height = 0;
        return TitleBarReport{.valid = true,
                              .error = TitleBarError::None,
                              .plan = plan,
                              .message = "Barra de título suprimida.",
                              .remediationHint = generateRemediationHint(TitleBarError::None)};
    }
    if (config.mode == TitleBarMode::NativeSystem) {
        return TitleBarReport{.valid = true,
                              .error = TitleBarError::None,
                              .plan = plan,
                              .message = "Barra de título provista por el sistema operativo.",
                              .remediationHint = generateRemediationHint(TitleBarError::None)};
    }

    // Comprobación de dimensiones mínimas de ventana
    if (windowWidth < minWindowWidth) {
        return TitleBarReport{.valid = false,
                              .error = TitleBarError::DimensionsBelowMinimum,
                              .plan = plan,
                              .message = titleBarErrorToString(TitleBarError::DimensionsBelowMinimum),
                              .remediationHint =
                                  generateRemediationHint(TitleBarError::DimensionsBelowMinimum)};
    }

    // Determinación de controles de ventana según la orientación
    std::vector<TitleBarControlSpec> controls{};
    if (config.placement == TitleBarControlsPlacement::Right) {
        if (config.showSeniorToggle) {
            controls.push_back(TitleBarControlSpec{
                .type = TitleBarButtonType::SeniorModeToggle,
                .rect = {},
                .state = TitleBarButtonState::Normal,
                .visible = true,
                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::SeniorModeToggle),
                .tooltip = buttonTooltip(TitleBarButtonType::SeniorModeToggle)});
        }
        controls.push_back(
            TitleBarControlSpec{.type = TitleBarButtonType::Minimize,
                                .rect = {},
                                .state = TitleBarButtonState::Normal,
                                .visible = true,
                                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::Minimize),
                                .tooltip = buttonTooltip(TitleBarButtonType::Minimize)});
        controls.push_back(TitleBarControlSpec{
            .type = TitleBarButtonType::MaximizeRestore,
            .rect = {},
            .state = TitleBarButtonState::Normal,
            .visible = true,
            .accessibleLabel =
                buttonAccessibleLabel(TitleBarButtonType::MaximizeRestore, config.state.isMaximized),
            .tooltip = buttonTooltip(TitleBarButtonType::MaximizeRestore, config.state.isMaximized)});
        controls.push_back(
            TitleBarControlSpec{.type = TitleBarButtonType::Close,
                                .rect = {},
                                .state = TitleBarButtonState::Normal,
                                .visible = true,
                                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::Close),
                                .tooltip = buttonTooltip(TitleBarButtonType::Close)});
    } else {
        // Disposición a la izquierda (macOS y accesibilidad zurda)
        controls.push_back(
            TitleBarControlSpec{.type = TitleBarButtonType::Close,
                                .rect = {},
                                .state = TitleBarButtonState::Normal,
                                .visible = true,
                                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::Close),
                                .tooltip = buttonTooltip(TitleBarButtonType::Close)});
        controls.push_back(
            TitleBarControlSpec{.type = TitleBarButtonType::Minimize,
                                .rect = {},
                                .state = TitleBarButtonState::Normal,
                                .visible = true,
                                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::Minimize),
                                .tooltip = buttonTooltip(TitleBarButtonType::Minimize)});
        controls.push_back(TitleBarControlSpec{
            .type = TitleBarButtonType::MaximizeRestore,
            .rect = {},
            .state = TitleBarButtonState::Normal,
            .visible = true,
            .accessibleLabel =
                buttonAccessibleLabel(TitleBarButtonType::MaximizeRestore, config.state.isMaximized),
            .tooltip = buttonTooltip(TitleBarButtonType::MaximizeRestore, config.state.isMaximized)});
        if (config.showSeniorToggle) {
            controls.push_back(TitleBarControlSpec{
                .type = TitleBarButtonType::SeniorModeToggle,
                .rect = {},
                .state = TitleBarButtonState::Normal,
                .visible = true,
                .accessibleLabel = buttonAccessibleLabel(TitleBarButtonType::SeniorModeToggle),
                .tooltip = buttonTooltip(TitleBarButtonType::SeniorModeToggle)});
        }
    }

    const int numControls = static_cast<int>(controls.size());
    const int totalControlsWidth =
        numControls * buttonWidth + ((numControls > 1) ? (numControls - 1) * buttonSpacing : 0);

    if (totalControlsWidth > windowWidth) {
        return TitleBarReport{.valid = false,
                              .error = TitleBarError::ControlsOverflowWidth,
                              .plan = plan,
                              .message = titleBarErrorToString(TitleBarError::ControlsOverflowWidth),
                              .remediationHint =
                                  generateRemediationHint(TitleBarError::ControlsOverflowWidth)};
    }

    int boxX = 0;
    if (config.placement == TitleBarControlsPlacement::Right) {
        boxX = windowWidth - totalControlsWidth;
    } else {
        boxX = 0;
    }

    plan.controlsBoxRect = LayoutRect{.x = boxX, .y = 0, .width = totalControlsWidth, .height = height};

    // Distribución geométrica cartesiana de cada botón dentro de la caja de controles
    int currentBtnX = boxX;
    for (auto& ctrl : controls) {
        ctrl.rect = LayoutRect{.x = currentBtnX,
                               .y = (height - buttonHeight) / 2,
                               .width = buttonWidth,
                               .height = buttonHeight};
        currentBtnX += buttonWidth + buttonSpacing;
    }
    plan.controls = std::move(controls);

    // Asignación de icono de marca
    if (config.showIcon) {
        if (config.placement == TitleBarControlsPlacement::Right) {
            plan.iconRect = LayoutRect{
                .x = iconMargin, .y = (height - iconSize) / 2, .width = iconSize, .height = iconSize};
        } else {
            plan.iconRect = LayoutRect{.x = windowWidth - iconMargin - iconSize,
                                       .y = (height - iconSize) / 2,
                                       .width = iconSize,
                                       .height = iconSize};
        }
    }

    // Cálculo del espacio neto restante entre márgenes e icono y caja de controles
    int leftBoundary = 0;
    int rightBoundary = 0;

    if (config.placement == TitleBarControlsPlacement::Right) {
        leftBoundary =
            config.showIcon ? (plan.iconRect.x + plan.iconRect.width + iconMargin) : iconMargin;
        rightBoundary = plan.controlsBoxRect.x;
    } else {
        leftBoundary = plan.controlsBoxRect.x + plan.controlsBoxRect.width + iconMargin;
        rightBoundary = config.showIcon ? (plan.iconRect.x - iconMargin) : (windowWidth - iconMargin);
    }

    const int availableSpan = rightBoundary - leftBoundary;

    if (availableSpan < minDraggable) {
        plan.draggableRect =
            LayoutRect{.x = leftBoundary, .y = 0, .width = std::max(0, availableSpan), .height = height};
        return TitleBarReport{.valid = false,
                              .error = TitleBarError::DraggableAreaInsufficient,
                              .plan = plan,
                              .message = titleBarErrorToString(TitleBarError::DraggableAreaInsufficient),
                              .remediationHint =
                                  generateRemediationHint(TitleBarError::DraggableAreaInsufficient)};
    }

    // División del espacio horizontal disponible entre título y zona de arrastre pura
    const std::string sanitized = sanitizeTitle(config.title);
    int captionWidth = 0;
    if (!sanitized.empty()) {
        const int maxCaption = availableSpan - minDraggable;
        captionWidth = std::min(240, std::max(0, maxCaption));
    }

    if (captionWidth > 0) {
        plan.titleRect = LayoutRect{.x = leftBoundary, .y = 0, .width = captionWidth, .height = height};
        plan.draggableRect = LayoutRect{.x = leftBoundary + captionWidth,
                                        .y = 0,
                                        .width = availableSpan - captionWidth,
                                        .height = height};
    } else {
        plan.draggableRect =
            LayoutRect{.x = leftBoundary, .y = 0, .width = availableSpan, .height = height};
    }

    if (!TitleBarConfiguration::verifyInvariants(plan)) {
        return TitleBarReport{
            .valid = false,
            .error = TitleBarError::ConflictingControlPlacement,
            .plan = plan,
            .message = titleBarErrorToString(TitleBarError::ConflictingControlPlacement),
            .remediationHint = generateRemediationHint(TitleBarError::ConflictingControlPlacement)};
    }

    return TitleBarReport{.valid = true,
                          .error = TitleBarError::None,
                          .plan = plan,
                          .message = titleBarErrorToString(TitleBarError::None),
                          .remediationHint = generateRemediationHint(TitleBarError::None)};
}

TitleBarHitTestZone TitleBarValidator::hitTest(int x, int y, const TitleBarLayoutPlan& plan) noexcept {
    if (x < 0 || y < 0 || x >= plan.windowWidth || y >= plan.height) {
        return TitleBarHitTestZone::Nowhere;
    }

    // Prioridad 1: Botones de control de ventana
    for (const auto& ctrl : plan.controls) {
        if (ctrl.visible && ctrl.rect.contains(x, y)) {
            switch (ctrl.type) {
            case TitleBarButtonType::Minimize:
                return TitleBarHitTestZone::MinimizeButton;
            case TitleBarButtonType::MaximizeRestore:
                return TitleBarHitTestZone::MaximizeButton;
            case TitleBarButtonType::Close:
                return TitleBarHitTestZone::CloseButton;
            case TitleBarButtonType::SeniorModeToggle:
                return TitleBarHitTestZone::SeniorModeToggle;
            }
        }
    }

    // Prioridad 2: Icono de ventana
    if (plan.iconRect.width > 0 && plan.iconRect.contains(x, y)) {
        return TitleBarHitTestZone::WindowIcon;
    }

    // Prioridad 3: Título de ventana
    if (plan.titleRect.width > 0 && plan.titleRect.contains(x, y)) {
        return TitleBarHitTestZone::CaptionText;
    }

    // Prioridad 4: Espacio de arrastre asignado
    if (plan.draggableRect.width > 0 && plan.draggableRect.contains(x, y)) {
        return TitleBarHitTestZone::DraggableSpace;
    }

    // Por defecto dentro de la barra de título: zona de arrastre para mover la ventana
    return TitleBarHitTestZone::DraggableSpace;
}

std::string TitleBarValidator::titleBarErrorToString(TitleBarError error) {
    switch (error) {
    case TitleBarError::None:
        return "Configuración de barra de título válida";
    case TitleBarError::DimensionsBelowMinimum:
        return "Las dimensiones de la ventana son inferiores al mínimo requerido";
    case TitleBarError::ControlsOverflowWidth:
        return "Los controles de ventana sobrepasan el ancho disponible";
    case TitleBarError::DraggableAreaInsufficient:
        return "El espacio disponible para mover la ventana es insuficiente";
    case TitleBarError::ConflictingControlPlacement:
        return "Conflicto o solapamiento en la disposición de los controles";
    case TitleBarError::SeniorModeMinimumHeightViolation:
        return "La interfaz más fácil de usar requiere una altura de barra de título mínima de 48 "
               "píxeles";
    case TitleBarError::UnsupportedWindowChromeMode:
        return "Modalidad de decoración de ventana no soportada";
    }
    return "Error no identificado en la barra de título";
}

std::string TitleBarValidator::titleBarModeToString(TitleBarMode mode) {
    switch (mode) {
    case TitleBarMode::NativeSystem:
        return "Barra de título del sistema operativo";
    case TitleBarMode::CustomIntegrated:
        return "Barra de título integrada";
    case TitleBarMode::Suppressed:
        return "Barra de título oculta";
    }
    return "Modalidad no definida";
}

std::string TitleBarValidator::hitTestZoneToString(TitleBarHitTestZone zone) {
    switch (zone) {
    case TitleBarHitTestZone::Nowhere:
        return "Fuera de la barra de título";
    case TitleBarHitTestZone::ClientArea:
        return "Área de contenido";
    case TitleBarHitTestZone::DraggableSpace:
        return "Espacio para mover la ventana";
    case TitleBarHitTestZone::WindowIcon:
        return "Icono de la aplicación";
    case TitleBarHitTestZone::CaptionText:
        return "Título de la ventana";
    case TitleBarHitTestZone::MinimizeButton:
        return "Botón minimizar";
    case TitleBarHitTestZone::MaximizeButton:
        return "Botón maximizar o restaurar";
    case TitleBarHitTestZone::CloseButton:
        return "Botón cerrar";
    case TitleBarHitTestZone::SeniorModeToggle:
        return "Botón de interfaz más fácil de usar";
    }
    return "Zona no identificada";
}

std::string TitleBarValidator::buttonTypeToString(TitleBarButtonType type, bool isMaximized) {
    switch (type) {
    case TitleBarButtonType::Minimize:
        return "Minimizar";
    case TitleBarButtonType::MaximizeRestore:
        return isMaximized ? "Restaurar" : "Maximizar";
    case TitleBarButtonType::Close:
        return "Cerrar";
    case TitleBarButtonType::SeniorModeToggle:
        return "Interfaz más fácil de usar";
    }
    return "Botón de control";
}

std::string TitleBarValidator::buttonAccessibleLabel(TitleBarButtonType type, bool isMaximized) {
    switch (type) {
    case TitleBarButtonType::Minimize:
        return "Minimizar la ventana";
    case TitleBarButtonType::MaximizeRestore:
        return isMaximized ? "Restaurar la ventana" : "Maximizar la ventana";
    case TitleBarButtonType::Close:
        return "Cerrar la ventana";
    case TitleBarButtonType::SeniorModeToggle:
        return "Activar o desactivar la interfaz más fácil de usar";
    }
    return "Control de ventana";
}

std::string TitleBarValidator::buttonTooltip(TitleBarButtonType type, bool isMaximized) {
    switch (type) {
    case TitleBarButtonType::Minimize:
        return "Minimizar la ventana del navegador";
    case TitleBarButtonType::MaximizeRestore:
        return isMaximized ? "Restaurar la ventana al tamaño normal"
                           : "Aumentar la ventana al tamaño completo de la pantalla";
    case TitleBarButtonType::Close:
        return "Cerrar la ventana del navegador";
    case TitleBarButtonType::SeniorModeToggle:
        return "Aumentar el tamaño de los botones y textos para una lectura más cómoda";
    }
    return "Control de ventana";
}

std::string TitleBarValidator::generateRemediationHint(TitleBarError error, std::string_view details) {
    std::string hint;
    switch (error) {
    case TitleBarError::None:
        return "No se requiere ninguna acción de corrección.";
    case TitleBarError::DimensionsBelowMinimum:
        hint = "Aumente el ancho o la altura de la ventana para cumplir con el espacio mínimo de la "
               "interfaz.";
        break;
    case TitleBarError::ControlsOverflowWidth:
        hint = "Reduzca el número de controles visibles o aumente el ancho de la ventana.";
        break;
    case TitleBarError::DraggableAreaInsufficient:
        hint = "Aumente el ancho de la ventana o simplifique los elementos para dejar al menos 120 "
               "píxeles libres para mover la ventana.";
        break;
    case TitleBarError::ConflictingControlPlacement:
        hint = "Verifique la posición de los controles para evitar que se superpongan entre sí.";
        break;
    case TitleBarError::SeniorModeMinimumHeightViolation:
        hint = "Configure una altura de barra de al menos 48 píxeles para garantizar botones táctiles "
               "cómodos.";
        break;
    case TitleBarError::UnsupportedWindowChromeMode:
        hint = "Seleccione una modalidad de decoración compatible, como la barra estándar o integrada.";
        break;
    }
    if (!details.empty()) {
        hint += " Detalle: ";
        hint.append(details);
    }
    return hint;
}

std::string TitleBarValidator::sanitizeTitle(std::string_view rawTitle, bool isIncognito) {
    std::string title(rawTitle);

    // 1. Eliminar esquemas de URL
    const std::string_view schemes[] = {"http://", "https://", "ftp://", "file://"};
    for (auto scheme : schemes) {
        if (title.starts_with(scheme)) {
            title.erase(0, scheme.length());
            break;
        }
    }

    // 2. Eliminar credenciales user:pass@
    const auto atPos = title.find('@');
    if (atPos != std::string::npos && atPos < title.find('/')) {
        title.erase(0, atPos + 1);
    }

    // 3. Eliminar parámetros de consulta y fragmentos (tokens, sesión, contraseñas)
    const auto queryPos = title.find('?');
    if (queryPos != std::string::npos) {
        title.erase(queryPos);
    }
    const auto hashPos = title.find('#');
    if (hashPos != std::string::npos) {
        title.erase(hashPos);
    }

    // 4. Sanitización de espacios en blanco
    while (!title.empty() && (title.front() == ' ' || title.front() == '/')) {
        title.erase(title.begin());
    }
    while (!title.empty() && (title.back() == ' ' || title.back() == '/')) {
        title.pop_back();
    }

    if (title.empty()) {
        title = "Vigilio Browser";
    } else {
        const std::string brand = "Vigilio Browser";
        if (title.find(brand) == std::string::npos) {
            title += " — " + brand;
        }
    }

    if (isIncognito) {
        const std::string incognitoNotice = "(Navegación privada)";
        if (title.find(incognitoNotice) == std::string::npos) {
            title += " " + incognitoNotice;
        }
    }

    return title;
}

} // namespace vigilio
