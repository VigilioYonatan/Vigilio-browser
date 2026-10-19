#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityValidator.h"

namespace vigilio {

TabStripDimensions TabStripDensityConfiguration::dimensionsForDensity(LayoutDensity density) noexcept {
    TabStripDimensions dims{};
    switch (density) {
    case LayoutDensity::Default:
        dims.height = kDefaultStripHeight;
        dims.minStripWidth = kDefaultMinStripWidth;
        dims.preferredTabWidth = kDefaultPreferredTabWidth;
        dims.minTabWidth = kDefaultMinTabWidth;
        dims.maxTabWidth = kDefaultMaxTabWidth;
        dims.pinnedTabWidth = kDefaultPinnedTabWidth;
        dims.newTabButtonWidth = kDefaultNewTabButtonSize;
        dims.newTabButtonHeight = kDefaultNewTabButtonSize;
        dims.faviconSize = kDefaultFaviconSize;
        dims.closeButtonSize = kDefaultCloseButtonSize;
        dims.audioIndicatorSize = kDefaultAudioIndicatorSize;
        dims.tabSpacing = kDefaultTabSpacing;
        dims.closeButtonThresholdWidth = kDefaultCloseThreshold;
        dims.paddingLeft = kDefaultPaddingLeft;
        dims.paddingRight = kDefaultPaddingRight;
        break;
    case LayoutDensity::Senior:
        dims.height = kSeniorStripHeight;
        dims.minStripWidth = kSeniorMinStripWidth;
        dims.preferredTabWidth = kSeniorPreferredTabWidth;
        dims.minTabWidth = kSeniorMinTabWidth;
        dims.maxTabWidth = kSeniorMaxTabWidth;
        dims.pinnedTabWidth = kSeniorPinnedTabWidth;
        dims.newTabButtonWidth = kSeniorNewTabButtonSize;
        dims.newTabButtonHeight = kSeniorNewTabButtonSize;
        dims.faviconSize = kSeniorFaviconSize;
        dims.closeButtonSize = kSeniorCloseButtonSize;
        dims.audioIndicatorSize = kSeniorAudioIndicatorSize;
        dims.tabSpacing = kSeniorTabSpacing;
        dims.closeButtonThresholdWidth = kSeniorCloseThreshold;
        dims.paddingLeft = kSeniorPaddingLeft;
        dims.paddingRight = kSeniorPaddingRight;
        break;
    case LayoutDensity::Compact:
        dims.height = kCompactStripHeight;
        dims.minStripWidth = kCompactMinStripWidth;
        dims.preferredTabWidth = kCompactPreferredTabWidth;
        dims.minTabWidth = kCompactMinTabWidth;
        dims.maxTabWidth = kCompactMaxTabWidth;
        dims.pinnedTabWidth = kCompactPinnedTabWidth;
        dims.newTabButtonWidth = kCompactNewTabButtonSize;
        dims.newTabButtonHeight = kCompactNewTabButtonSize;
        dims.faviconSize = kCompactFaviconSize;
        dims.closeButtonSize = kCompactCloseButtonSize;
        dims.audioIndicatorSize = kCompactAudioIndicatorSize;
        dims.tabSpacing = kCompactTabSpacing;
        dims.closeButtonThresholdWidth = kCompactCloseThreshold;
        dims.paddingLeft = kCompactPaddingLeft;
        dims.paddingRight = kCompactPaddingRight;
        break;
    }
    return dims;
}

TabStripDensityPlan TabStripDensityConfiguration::canonicalDefaultDensity(int availableWidth) {
    TabSpec tab1{.id = 1,
                 .title = "Página principal",
                 .url = "https://vigilio.internal",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = true,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab2{.id = 2,
                 .title = "Documentación",
                 .url = "https://vigilio.internal/docs",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab3{.id = 3,
                 .title = "Noticias",
                 .url = "https://vigilio.internal/news",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab4{.id = 4,
                 .title = "Herramientas",
                 .url = "https://vigilio.internal/tools",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};

    TabStripDensityConfig config{.density = LayoutDensity::Default,
                                 .showNewTabButton = true,
                                 .overflowMode = TabStripOverflowMode::Scrollable,
                                 .scrollOffset = 0};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(availableWidth, config,
                                                                          {tab1, tab2, tab3, tab4});
    return report.plan;
}

TabStripDensityPlan TabStripDensityConfiguration::canonicalSeniorDensity(int availableWidth) {
    TabSpec tab1{.id = 1,
                 .title = "Página principal",
                 .url = "https://vigilio.internal",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = true,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab2{.id = 2,
                 .title = "Accesibilidad",
                 .url = "https://vigilio.internal/access",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab3{.id = 3,
                 .title = "Ayuda",
                 .url = "https://vigilio.internal/help",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};

    TabStripDensityConfig config{.density = LayoutDensity::Senior,
                                 .showNewTabButton = true,
                                 .overflowMode = TabStripOverflowMode::Scrollable,
                                 .scrollOffset = 0};

    const auto report =
        TabStripDensityValidator::calculateTabStripLayout(availableWidth, config, {tab1, tab2, tab3});
    return report.plan;
}

TabStripDensityPlan TabStripDensityConfiguration::canonicalCompactDensity(int availableWidth) {
    TabSpec tab1{.id = 1,
                 .title = "Pestaña 1",
                 .url = "https://vigilio.internal/1",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = true,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab2{.id = 2,
                 .title = "Pestaña 2",
                 .url = "https://vigilio.internal/2",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab3{.id = 3,
                 .title = "Pestaña 3",
                 .url = "https://vigilio.internal/3",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab4{.id = 4,
                 .title = "Pestaña 4",
                 .url = "https://vigilio.internal/4",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};
    TabSpec tab5{.id = 5,
                 .title = "Pestaña 5",
                 .url = "https://vigilio.internal/5",
                 .pinState = TabPinState::Unpinned,
                 .isSelected = false,
                 .mediaState = TabMediaState::None,
                 .discardState = TabDiscardState::Active,
                 .loadingState = TabLoadingState::Idle,
                 .interactiveState = TabInteractiveState::Normal};

    TabStripDensityConfig config{.density = LayoutDensity::Compact,
                                 .showNewTabButton = true,
                                 .overflowMode = TabStripOverflowMode::Scrollable,
                                 .scrollOffset = 0};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        availableWidth, config, {tab1, tab2, tab3, tab4, tab5});
    return report.plan;
}

bool TabStripDensityConfiguration::verifyInvariants(const TabStripDensityPlan& plan) noexcept {
    if (plan.availableWidth < 0 || plan.height < 0) {
        return false;
    }

    // 1. Umbrales dimensionales de ancho disponible y altura
    switch (plan.density) {
    case LayoutDensity::Default:
        if (plan.availableWidth < kDefaultMinStripWidth || plan.height < kDefaultStripHeight) {
            return false;
        }
        break;
    case LayoutDensity::Senior:
        if (plan.availableWidth < kSeniorMinStripWidth || plan.height < kSeniorStripHeight) {
            return false;
        }
        break;
    case LayoutDensity::Compact:
        if (plan.availableWidth < kCompactMinStripWidth || plan.height < kCompactStripHeight) {
            return false;
        }
        break;
    }

    // 2. Invariante de accesibilidad y tap targets en la «Interfaz más fácil de usar»
    if (plan.density == LayoutDensity::Senior) {
        if (plan.height < 48) {
            return false;
        }
        if (plan.newTabButtonRect.width > 0 &&
            (plan.newTabButtonRect.width < 48 || plan.newTabButtonRect.height < 48)) {
            return false;
        }
        for (const auto& tab : plan.tabs) {
            if (tab.rect.height < 48) {
                return false;
            }
            if (tab.isPinned && (tab.rect.width < 48 || tab.rect.height < 48)) {
                return false;
            }
            if (tab.showCloseButton &&
                (tab.closeButtonRect.width < 24 || tab.closeButtonRect.height < 24)) {
                return false;
            }
        }
    }

    // 3. Precedencia de pestañas fijadas (x_pinned < x_regular)
    bool seenRegular = false;
    for (const auto& tab : plan.tabs) {
        if (!tab.isPinned) {
            seenRegular = true;
        } else if (seenRegular) {
            return false;
        }
    }

    // 4. Disyunción espacial entre pestañas (no solapamiento: Ti ∩ Tj = ∅)
    const std::size_t tabCount = plan.tabs.size();
    for (std::size_t i = 0; i < tabCount; ++i) {
        for (std::size_t j = i + 1; j < tabCount; ++j) {
            if (plan.tabs[i].rect.intersects(plan.tabs[j].rect)) {
                return false;
            }
        }
        if (plan.density == LayoutDensity::Senior && i + 1 < tabCount) {
            if (plan.tabs[i + 1].rect.x - (plan.tabs[i].rect.x + plan.tabs[i].rect.width) <
                kSeniorTabSpacing) {
                return false;
            }
        }
    }

    // 5. Disyunción y contención interna en cada pestaña
    for (const auto& tab : plan.tabs) {
        if (tab.showFavicon && tab.showTitle && tab.faviconRect.intersects(tab.titleRect)) {
            return false;
        }
        if (tab.showTitle && tab.showCloseButton && tab.titleRect.intersects(tab.closeButtonRect)) {
            return false;
        }
        if (tab.showFavicon && tab.showCloseButton && tab.faviconRect.intersects(tab.closeButtonRect)) {
            return false;
        }
        if (tab.showAudioIndicator) {
            if (tab.showFavicon && tab.audioIndicatorRect.intersects(tab.faviconRect)) {
                return false;
            }
            if (tab.showTitle && tab.audioIndicatorRect.intersects(tab.titleRect)) {
                return false;
            }
            if (tab.showCloseButton && tab.audioIndicatorRect.intersects(tab.closeButtonRect)) {
                return false;
            }
        }

        const int tabRight = tab.rect.x + tab.rect.width;
        if (tab.showFavicon &&
            (tab.faviconRect.x < tab.rect.x || tab.faviconRect.x + tab.faviconRect.width > tabRight)) {
            return false;
        }
        if (tab.showTitle &&
            (tab.titleRect.x < tab.rect.x || tab.titleRect.x + tab.titleRect.width > tabRight)) {
            return false;
        }
        if (tab.showCloseButton && (tab.closeButtonRect.x < tab.rect.x ||
                                    tab.closeButtonRect.x + tab.closeButtonRect.width > tabRight)) {
            return false;
        }
        if (tab.showAudioIndicator &&
            (tab.audioIndicatorRect.x < tab.rect.x ||
             tab.audioIndicatorRect.x + tab.audioIndicatorRect.width > tabRight)) {
            return false;
        }
    }

    // 6. Disyunción de botón de nueva pestaña con las pestañas (en modo no desbordado)
    if (!plan.isOverflowing && plan.newTabButtonRect.width > 0 && plan.newTabButtonRect.height > 0) {
        for (const auto& tab : plan.tabs) {
            if (plan.newTabButtonRect.intersects(tab.rect)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace vigilio
