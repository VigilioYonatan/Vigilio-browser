#include "core/TabStripDensityValidator.h"
#include "core/TabStripDensityConfiguration.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

TabStripDensityError TabStripDensityValidator::validateDimensions(int width, int height,
                                                                  LayoutDensity density) noexcept {
    if (width < 0 || height < 0) {
        return TabStripDensityError::WidthBelowMinimum;
    }

    switch (density) {
    case LayoutDensity::Default:
        if (width < TabStripDensityConfiguration::kDefaultMinStripWidth) {
            return TabStripDensityError::WidthBelowMinimum;
        }
        if (height < TabStripDensityConfiguration::kDefaultStripHeight) {
            return TabStripDensityError::HeightBelowMinimum;
        }
        break;
    case LayoutDensity::Senior:
        if (height < TabStripDensityConfiguration::kSeniorStripHeight) {
            return TabStripDensityError::SeniorModeMinimumHeightViolation;
        }
        if (width < TabStripDensityConfiguration::kSeniorMinStripWidth) {
            return TabStripDensityError::WidthBelowMinimum;
        }
        break;
    case LayoutDensity::Compact:
        if (width < TabStripDensityConfiguration::kCompactMinStripWidth) {
            return TabStripDensityError::WidthBelowMinimum;
        }
        if (height < TabStripDensityConfiguration::kCompactStripHeight) {
            return TabStripDensityError::HeightBelowMinimum;
        }
        break;
    }

    return TabStripDensityError::None;
}

TabStripDensityError TabStripDensityValidator::validateDimensions(int width,
                                                                  LayoutDensity density) noexcept {
    int height = TabStripDensityConfiguration::kDefaultStripHeight;
    switch (density) {
    case LayoutDensity::Default:
        height = TabStripDensityConfiguration::kDefaultStripHeight;
        break;
    case LayoutDensity::Senior:
        height = TabStripDensityConfiguration::kSeniorStripHeight;
        break;
    case LayoutDensity::Compact:
        height = TabStripDensityConfiguration::kCompactStripHeight;
        break;
    }
    return validateDimensions(width, height, density);
}

TabStripDensityError TabStripDensityValidator::validatePlan(const TabStripDensityPlan& plan) noexcept {
    const auto dimErr = validateDimensions(plan.availableWidth, plan.height, plan.density);
    if (dimErr != TabStripDensityError::None) {
        return dimErr;
    }

    if (!TabStripDensityConfiguration::verifyInvariants(plan)) {
        if (plan.density == LayoutDensity::Senior) {
            if (plan.height < 48) {
                return TabStripDensityError::SeniorModeMinimumHeightViolation;
            }
            if (plan.newTabButtonRect.width > 0 &&
                (plan.newTabButtonRect.width < 48 || plan.newTabButtonRect.height < 48)) {
                return TabStripDensityError::SeniorModeTapTargetViolation;
            }
            for (const auto& tab : plan.tabs) {
                if (tab.rect.height < 48) {
                    return TabStripDensityError::SeniorModeMinimumHeightViolation;
                }
                if (tab.isPinned && (tab.rect.width < 48 || tab.rect.height < 48)) {
                    return TabStripDensityError::SeniorModeTapTargetViolation;
                }
                if (tab.showCloseButton &&
                    (tab.closeButtonRect.width < 24 || tab.closeButtonRect.height < 24)) {
                    return TabStripDensityError::SeniorModeTapTargetViolation;
                }
            }
        }
        return TabStripDensityError::InvalidTabDimensions;
    }

    return TabStripDensityError::None;
}

TabStripDensityReport TabStripDensityValidator::calculateTabStripLayout(
    int availableWidth, const TabStripDensityConfig& config, const std::vector<TabSpec>& tabs) {
    const auto dims = TabStripDensityConfiguration::dimensionsForDensity(config.density);

    const auto dimError = validateDimensions(availableWidth, dims.height, config.density);
    if (dimError != TabStripDensityError::None) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = dimError;
        failReport.plan.availableWidth = availableWidth;
        failReport.plan.height = dims.height;
        failReport.plan.density = config.density;
        failReport.plan.containerRect = LayoutRect{0, 0, availableWidth, dims.height};
        failReport.message = tabStripErrorToString(dimError);
        failReport.remediationHint = generateRemediationHint(dimError);
        return failReport;
    }

    std::vector<TabSpec> pinnedTabs;
    std::vector<TabSpec> regularTabs;
    pinnedTabs.reserve(tabs.size());
    regularTabs.reserve(tabs.size());
    for (const auto& tab : tabs) {
        if (tab.pinState == TabPinState::Pinned) {
            pinnedTabs.push_back(tab);
        } else {
            regularTabs.push_back(tab);
        }
    }

    const int spacing = (config.density == LayoutDensity::Senior)
                            ? TabStripDensityConfiguration::kSeniorTabSpacing
                            : dims.tabSpacing;
    const int paddingLeft = dims.paddingLeft;
    const int paddingRight = dims.paddingRight;
    const int pinnedTabWidth = dims.pinnedTabWidth;
    const int newTabButtonWidth = dims.newTabButtonWidth;
    const int newTabButtonHeight = dims.newTabButtonHeight;
    const int newTabSpace = config.showNewTabButton ? (newTabButtonWidth + 4) : 0;
    const int pinnedCount = static_cast<int>(pinnedTabs.size());
    const int regularCount = static_cast<int>(regularTabs.size());

    const int totalPinnedWidth =
        pinnedCount * pinnedTabWidth + (pinnedCount > 0 ? (pinnedCount - 1) * spacing : 0);
    const int pinnedToRegularSpacing = (pinnedCount > 0 && regularCount > 0) ? spacing : 0;
    const int fixedSpace =
        paddingLeft + paddingRight + totalPinnedWidth + pinnedToRegularSpacing + newTabSpace;

    if (availableWidth < fixedSpace && pinnedCount > 0 && regularCount == 0) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::AvailableWidthExhausted;
        failReport.plan.availableWidth = availableWidth;
        failReport.plan.height = dims.height;
        failReport.plan.density = config.density;
        failReport.message = tabStripErrorToString(TabStripDensityError::AvailableWidthExhausted);
        failReport.remediationHint =
            generateRemediationHint(TabStripDensityError::AvailableWidthExhausted);
        return failReport;
    }

    const int netRegularWidth = availableWidth - fixedSpace;
    int effectiveTabWidth = dims.preferredTabWidth;
    bool isOverflowing = false;

    if (regularCount > 0) {
        const int totalRegularSpacing = (regularCount - 1) * spacing;
        const int spaceForTabBodies = netRegularWidth - totalRegularSpacing;
        if (spaceForTabBodies <= 0) {
            isOverflowing = true;
            effectiveTabWidth = dims.minTabWidth;
        } else {
            const int idealWidth = spaceForTabBodies / regularCount;
            if (idealWidth >= dims.preferredTabWidth) {
                effectiveTabWidth = dims.preferredTabWidth;
                isOverflowing = false;
            } else if (idealWidth >= dims.minTabWidth) {
                effectiveTabWidth = idealWidth;
                isOverflowing = false;
            } else {
                effectiveTabWidth = dims.minTabWidth;
                isOverflowing = true;
            }
        }
    }

    int currentX = paddingLeft;
    std::vector<TabItemLayout> layoutTabs;
    layoutTabs.reserve(tabs.size());

    for (const auto& tab : pinnedTabs) {
        TabItemLayout item{};
        item.id = tab.id;
        item.isPinned = true;
        item.isSelected = tab.isSelected;
        item.rect = LayoutRect{currentX, 0, pinnedTabWidth, dims.height};

        const int favSize = dims.faviconSize;
        item.showFavicon = true;
        item.faviconRect = LayoutRect{currentX + (pinnedTabWidth - favSize) / 2,
                                      (dims.height - favSize) / 2, favSize, favSize};
        item.showTitle = false;
        item.showCloseButton = false;

        if (tab.mediaState != TabMediaState::None) {
            item.showAudioIndicator = true;
            const int audioSize = dims.audioIndicatorSize;
            item.audioIndicatorRect =
                LayoutRect{currentX + pinnedTabWidth - audioSize - 2, 2, audioSize, audioSize};
        } else {
            item.showAudioIndicator = false;
        }

        item.accessibleLabel = tabAccessibleLabel(tab, tab.isSelected, config.density);
        item.tooltip = tabTooltip(tab, tab.isSelected);
        layoutTabs.push_back(item);
        currentX += pinnedTabWidth + spacing;
    }

    for (const auto& tab : regularTabs) {
        TabItemLayout item{};
        item.id = tab.id;
        item.isPinned = false;
        item.isSelected = tab.isSelected;
        item.rect = LayoutRect{currentX, 0, effectiveTabWidth, dims.height};

        const int innerPad = (config.density == LayoutDensity::Senior) ? 10 : 8;
        const int favSize = dims.faviconSize;
        item.showFavicon = true;
        item.faviconRect =
            LayoutRect{currentX + innerPad, (dims.height - favSize) / 2, favSize, favSize};

        if (config.density == LayoutDensity::Senior || tab.isSelected) {
            item.showCloseButton = true;
        } else {
            item.showCloseButton = (effectiveTabWidth >= dims.closeButtonThresholdWidth);
        }

        if (item.showCloseButton) {
            if (config.density == LayoutDensity::Senior) {
                constexpr int kSeniorTapTarget = 48;
                item.closeButtonRect = LayoutRect{currentX + effectiveTabWidth - kSeniorTapTarget, 0,
                                                  kSeniorTapTarget, kSeniorTapTarget};
            } else {
                const int closeSize = dims.closeButtonSize;
                item.closeButtonRect = LayoutRect{currentX + effectiveTabWidth - innerPad - closeSize,
                                                  (dims.height - closeSize) / 2, closeSize, closeSize};
            }
        }

        if (tab.mediaState != TabMediaState::None) {
            item.showAudioIndicator = true;
            const int audioSize = dims.audioIndicatorSize;
            const int audioRight = item.showCloseButton ? item.closeButtonRect.x - 4
                                                        : (currentX + effectiveTabWidth - innerPad);
            item.audioIndicatorRect =
                LayoutRect{audioRight - audioSize, (dims.height - audioSize) / 2, audioSize, audioSize};
        } else {
            item.showAudioIndicator = false;
        }

        item.showTitle = true;
        const int titleLeft = item.faviconRect.x + item.faviconRect.width + 6;
        int titleRight = currentX + effectiveTabWidth - innerPad;
        if (item.showAudioIndicator) {
            titleRight = item.audioIndicatorRect.x - 4;
        } else if (item.showCloseButton) {
            titleRight = item.closeButtonRect.x - 4;
        }
        const int titleWidth = std::max(0, titleRight - titleLeft);
        const int titleHeight = std::min(20, dims.height - 8);
        item.titleRect = LayoutRect{titleLeft, (dims.height - titleHeight) / 2, titleWidth, titleHeight};

        item.accessibleLabel = tabAccessibleLabel(tab, tab.isSelected, config.density);
        item.tooltip = tabTooltip(tab, tab.isSelected);
        layoutTabs.push_back(item);
        currentX += effectiveTabWidth + spacing;
    }

    LayoutRect newTabBtnRect{};
    LayoutRect scrollLeftRect{};
    LayoutRect scrollRightRect{};
    LayoutRect overflowMenuRect{};

    if (!isOverflowing) {
        if (config.showNewTabButton) {
            newTabBtnRect = LayoutRect{currentX + 4, (dims.height - newTabButtonHeight) / 2,
                                       newTabButtonWidth, newTabButtonHeight};
        }
    } else {
        const int scrollBtnWidth = (config.density == LayoutDensity::Senior) ? 48 : 24;
        const int scrollBtnHeight = (config.density == LayoutDensity::Senior) ? 48 : 24;
        if (config.showNewTabButton) {
            newTabBtnRect = LayoutRect{availableWidth - paddingRight - newTabButtonWidth,
                                       (dims.height - newTabButtonHeight) / 2, newTabButtonWidth,
                                       newTabButtonHeight};
            scrollRightRect =
                LayoutRect{newTabBtnRect.x - spacing - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
            scrollLeftRect =
                LayoutRect{scrollRightRect.x - spacing - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
            overflowMenuRect =
                LayoutRect{scrollLeftRect.x - spacing - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
        } else {
            scrollRightRect =
                LayoutRect{availableWidth - paddingRight - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
            scrollLeftRect =
                LayoutRect{scrollRightRect.x - spacing - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
            overflowMenuRect =
                LayoutRect{scrollLeftRect.x - spacing - scrollBtnWidth,
                           (dims.height - scrollBtnHeight) / 2, scrollBtnWidth, scrollBtnHeight};
        }
    }

    TabStripDensityPlan plan{};
    plan.availableWidth = availableWidth;
    plan.height = dims.height;
    plan.density = config.density;
    plan.containerRect = LayoutRect{0, 0, availableWidth, dims.height};
    plan.tabsBoundsRect = LayoutRect{
        paddingLeft, 0, currentX - paddingLeft - (layoutTabs.empty() ? 0 : spacing), dims.height};
    plan.newTabButtonRect = newTabBtnRect;
    plan.scrollLeftButtonRect = scrollLeftRect;
    plan.scrollRightButtonRect = scrollRightRect;
    plan.overflowMenuButtonRect = overflowMenuRect;
    plan.tabs = std::move(layoutTabs);
    plan.effectiveTabWidth = effectiveTabWidth;
    plan.pinnedTabsCount = pinnedCount;
    plan.regularTabsCount = regularCount;
    plan.isOverflowing = isOverflowing;
    plan.overflowMode = isOverflowing ? TabStripOverflowMode::Scrollable : TabStripOverflowMode::None;
    plan.totalConsumedWidth = currentX - (plan.tabs.empty() ? 0 : spacing) +
                              (newTabBtnRect.width > 0 ? (newTabBtnRect.width + 4) : 0);

    TabStripDensityReport report{};
    report.valid = true;
    report.error = TabStripDensityError::None;
    report.plan = plan;
    report.message = "Distribución de tira de pestañas calculada correctamente.";
    report.remediationHint = "";
    return report;
}

TabStripHitTestZone TabStripDensityValidator::hitTest(int x, int y,
                                                      const TabStripDensityPlan& plan) noexcept {
    if (x < 0 || x >= plan.availableWidth || y < 0 || y >= plan.height) {
        return TabStripHitTestZone::Nowhere;
    }

    if (plan.newTabButtonRect.width > 0 && plan.newTabButtonRect.contains(x, y)) {
        return TabStripHitTestZone::NewTabButton;
    }
    if (plan.scrollLeftButtonRect.width > 0 && plan.scrollLeftButtonRect.contains(x, y)) {
        return TabStripHitTestZone::ScrollLeftButton;
    }
    if (plan.scrollRightButtonRect.width > 0 && plan.scrollRightButtonRect.contains(x, y)) {
        return TabStripHitTestZone::ScrollRightButton;
    }
    if (plan.overflowMenuButtonRect.width > 0 && plan.overflowMenuButtonRect.contains(x, y)) {
        return TabStripHitTestZone::OverflowMenuButton;
    }

    for (const auto& tab : plan.tabs) {
        if (tab.rect.contains(x, y)) {
            if (tab.showCloseButton && tab.closeButtonRect.contains(x, y)) {
                return TabStripHitTestZone::TabCloseButton;
            }
            if (tab.showAudioIndicator && tab.audioIndicatorRect.contains(x, y)) {
                return TabStripHitTestZone::TabAudioIndicator;
            }
            if (tab.showFavicon && tab.faviconRect.contains(x, y)) {
                return TabStripHitTestZone::TabFavicon;
            }
            if (tab.showTitle && tab.titleRect.contains(x, y)) {
                return TabStripHitTestZone::TabTitle;
            }
            return TabStripHitTestZone::TabBody;
        }
    }

    return TabStripHitTestZone::EmptySpace;
}

std::string TabStripDensityValidator::tabStripErrorToString(TabStripDensityError error) {
    switch (error) {
    case TabStripDensityError::None:
        return "Operación correcta.";
    case TabStripDensityError::WidthBelowMinimum:
        return "El ancho disponible para la tira de pestañas es inferior al mínimo requerido.";
    case TabStripDensityError::HeightBelowMinimum:
        return "La altura de la tira de pestañas es inferior al mínimo requerido.";
    case TabStripDensityError::SeniorModeMinimumHeightViolation:
        return "La altura para la Interfaz más fácil de usar no puede ser inferior a 48 píxeles.";
    case TabStripDensityError::SeniorModeTapTargetViolation:
        return "Las áreas interactivas para la Interfaz más fácil de usar deben ser de al menos 48 por "
               "48 píxeles.";
    case TabStripDensityError::InvalidTabDimensions:
        return "Las dimensiones de configuración para las pestañas son inválidas.";
    case TabStripDensityError::InvalidTabCount:
        return "El número de pestañas especificado es inválido.";
    case TabStripDensityError::NegativeMarginOrSpacing:
        return "Los márgenes o el espaciado entre pestañas no pueden ser negativos.";
    case TabStripDensityError::AvailableWidthExhausted:
        return "El ancho disponible se ha agotado para las pestañas fijadas y controles auxiliares.";
    }
    return "Error desconocido en la tira de pestañas.";
}

std::string TabStripDensityValidator::densityToString(LayoutDensity density) {
    switch (density) {
    case LayoutDensity::Default:
        return "Predeterminada";
    case LayoutDensity::Senior:
        return "Interfaz más fácil de usar";
    case LayoutDensity::Compact:
        return "Compacta";
    }
    return "Desconocida";
}

std::string TabStripDensityValidator::hitTestZoneToString(TabStripHitTestZone zone) {
    switch (zone) {
    case TabStripHitTestZone::Nowhere:
        return "Fuera de la tira de pestañas";
    case TabStripHitTestZone::EmptySpace:
        return "Espacio libre de la tira de pestañas";
    case TabStripHitTestZone::TabBody:
        return "Cuerpo de la pestaña";
    case TabStripHitTestZone::TabFavicon:
        return "Icono de la pestaña";
    case TabStripHitTestZone::TabTitle:
        return "Título de la pestaña";
    case TabStripHitTestZone::TabCloseButton:
        return "Botón para cerrar pestaña";
    case TabStripHitTestZone::TabAudioIndicator:
        return "Control de reproducción de audio";
    case TabStripHitTestZone::NewTabButton:
        return "Botón para abrir una nueva pestaña";
    case TabStripHitTestZone::ScrollLeftButton:
        return "Control para desplazar pestañas hacia la izquierda";
    case TabStripHitTestZone::ScrollRightButton:
        return "Control para desplazar pestañas hacia la derecha";
    case TabStripHitTestZone::OverflowMenuButton:
        return "Menú con lista de pestañas abiertas";
    }
    return "Zona desconocida";
}

std::string TabStripDensityValidator::tabAccessibleLabel(const TabSpec& tab, bool isSelected,
                                                         LayoutDensity density) {
    (void)density;
    const std::string cleanTitle = sanitizeTitle(tab.title);

    if (tab.discardState == TabDiscardState::Discarded) {
        return "Pestaña en reposo: " + cleanTitle + ". Haz clic para activarla.";
    }

    std::string prefix;
    if (tab.pinState == TabPinState::Pinned) {
        prefix = "Pestaña fijada: ";
    } else if (isSelected) {
        prefix = "Pestaña activa: ";
    } else {
        prefix = "Pestaña: ";
    }

    std::string label = prefix + cleanTitle;
    if (tab.mediaState == TabMediaState::AudioPlaying) {
        label += " (reproduciendo audio)";
    } else if (tab.mediaState == TabMediaState::AudioMuted) {
        label += " (audio silenciado)";
    }

    return label;
}

std::string TabStripDensityValidator::tabTooltip(const TabSpec& tab, bool isSelected) {
    (void)isSelected;
    std::string cleanTitle = sanitizeTitle(tab.title);
    if (tab.discardState == TabDiscardState::Discarded) {
        return "Pestaña en reposo: " + cleanTitle + " (Liberar memoria de pestañas inactivas)";
    }
    if (!tab.url.empty()) {
        return cleanTitle + "\n" + tab.url;
    }
    return cleanTitle;
}

std::string TabStripDensityValidator::closeButtonAccessibleLabel(const TabSpec& tab) {
    std::string cleanTitle = sanitizeTitle(tab.title);
    return "Cerrar pestaña " + cleanTitle;
}

std::string TabStripDensityValidator::audioButtonAccessibleLabel(bool isMuted) {
    return isMuted ? "Reactivar audio" : "Silenciar pestaña";
}

std::string TabStripDensityValidator::newTabButtonAccessibleLabel() {
    return "Abrir una nueva pestaña";
}

std::string TabStripDensityValidator::generateRemediationHint(TabStripDensityError error,
                                                              std::string_view details) {
    std::string hint;
    switch (error) {
    case TabStripDensityError::None:
        hint = "No se requiere ninguna acción.";
        break;
    case TabStripDensityError::WidthBelowMinimum:
        hint = "Aumente el ancho de la ventana para permitir que la tira de pestañas se muestre "
               "adecuadamente.";
        break;
    case TabStripDensityError::HeightBelowMinimum:
        hint = "Aumente la altura configurada de la tira de pestañas para cumplir con los requisitos "
               "mínimos.";
        break;
    case TabStripDensityError::SeniorModeMinimumHeightViolation:
        hint = "Ajuste la altura de la tira a un mínimo de 48 píxeles para garantizar la accesibilidad "
               "en la Interfaz más fácil de usar.";
        break;
    case TabStripDensityError::SeniorModeTapTargetViolation:
        hint = "Configure los elementos interactivos con un tamaño mínimo de 48 por 48 píxeles para la "
               "Interfaz más fácil de usar.";
        break;
    case TabStripDensityError::InvalidTabDimensions:
        hint = "Verifique que el ancho mínimo de pestaña sea menor o igual al ancho máximo y que todos "
               "los valores sean positivos.";
        break;
    case TabStripDensityError::InvalidTabCount:
        hint = "Proporcione una lista válida de pestañas.";
        break;
    case TabStripDensityError::NegativeMarginOrSpacing:
        hint = "Asegúrese de que todos los márgenes y espaciados sean valores mayores o iguales a cero.";
        break;
    case TabStripDensityError::AvailableWidthExhausted:
        hint = "Reduzca el número de pestañas fijadas o amplíe el ancho de la ventana.";
        break;
    }
    if (!details.empty()) {
        hint += " Detalle: ";
        hint.append(details);
    }
    return hint;
}

std::string TabStripDensityValidator::sanitizeTitle(std::string_view rawTitle, bool isIncognito) {
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

    // 4. Sanitización de caracteres de control
    for (char& ch : title) {
        if (ch == '\r' || ch == '\n' || ch == '\t') {
            ch = ' ';
        }
    }

    // 5. Trim de espacios y barras
    while (!title.empty() && (title.front() == ' ' || title.front() == '/')) {
        title.erase(title.begin());
    }
    while (!title.empty() && (title.back() == ' ' || title.back() == '/')) {
        title.pop_back();
    }

    if (title.empty()) {
        title = "Nueva pestaña";
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
