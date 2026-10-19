#include "core/MainWindowLayoutTypes.h"
#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityTypes.h"
#include "core/TabStripDensityValidator.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

// TEST-001: Invariantes canónicas para densidad estándar
void testCanonicalDefaultInvariants() {
    const auto plan = TabStripDensityConfiguration::canonicalDefaultDensity(1000);
    CHECK(plan.density == LayoutDensity::Default);
    CHECK(plan.height == 36);
    CHECK(plan.availableWidth == 1000);
    CHECK(plan.tabs.size() == 4);
    CHECK(plan.effectiveTabWidth == 200);
    CHECK(!plan.isOverflowing);
    CHECK(plan.overflowMode == TabStripOverflowMode::None);

    for (std::size_t i = 0; i < plan.tabs.size(); ++i) {
        for (std::size_t j = i + 1; j < plan.tabs.size(); ++j) {
            CHECK(!plan.tabs[i].rect.intersects(plan.tabs[j].rect));
        }
    }

    CHECK(TabStripDensityConfiguration::verifyInvariants(plan));
    CHECK(TabStripDensityValidator::validatePlan(plan) == TabStripDensityError::None);
}

// TEST-002: Invariantes canónicas para la «Interfaz más fácil de usar» (Senior)
void testCanonicalSeniorInvariants() {
    const auto plan = TabStripDensityConfiguration::canonicalSeniorDensity(1000);
    CHECK(plan.density == LayoutDensity::Senior);
    CHECK(plan.height >= 48);
    CHECK(plan.tabs.size() == 3);
    CHECK(plan.newTabButtonRect.width >= 48);
    CHECK(plan.newTabButtonRect.height >= 48);

    for (std::size_t i = 0; i < plan.tabs.size(); ++i) {
        CHECK(plan.tabs[i].rect.height >= 48);
        CHECK(plan.tabs[i].showCloseButton);
        if (i + 1 < plan.tabs.size()) {
            const int space = plan.tabs[i + 1].rect.x - (plan.tabs[i].rect.x + plan.tabs[i].rect.width);
            CHECK(space >= TabStripDensityConfiguration::kSeniorTabSpacing);
        }
    }

    CHECK(TabStripDensityConfiguration::verifyInvariants(plan));
    CHECK(TabStripDensityValidator::validatePlan(plan) == TabStripDensityError::None);
}

// TEST-003: Invariantes canónicas para densidad compacta
void testCanonicalCompactInvariants() {
    const auto plan = TabStripDensityConfiguration::canonicalCompactDensity(800);
    CHECK(plan.density == LayoutDensity::Compact);
    CHECK(plan.height == 28);
    CHECK(plan.availableWidth == 800);
    CHECK(plan.tabs.size() == 5);
    CHECK(TabStripDensityConfiguration::verifyInvariants(plan));
    CHECK(TabStripDensityValidator::validatePlan(plan) == TabStripDensityError::None);
}

// TEST-004: Separación geométrica y tamaño invariante de pestañas fijadas
void testPinnedTabsGeometricSeparation() {
    TabSpec p1{.id = 1, .title = "P1", .pinState = TabPinState::Pinned};
    TabSpec p2{.id = 2, .title = "P2", .pinState = TabPinState::Pinned};
    TabSpec r1{.id = 3, .title = "R1", .pinState = TabPinState::Unpinned};
    TabSpec r2{.id = 4, .title = "R2", .pinState = TabPinState::Unpinned};
    TabSpec r3{.id = 5, .title = "R3", .pinState = TabPinState::Unpinned};

    TabStripDensityConfig cfg{.density = LayoutDensity::Default, .showNewTabButton = true};
    const auto report =
        TabStripDensityValidator::calculateTabStripLayout(1000, cfg, {p1, p2, r1, r2, r3});
    CHECK(report.valid);
    CHECK(report.plan.pinnedTabsCount == 2);
    CHECK(report.plan.regularTabsCount == 3);
    CHECK(report.plan.tabs.size() == 5);

    CHECK(report.plan.tabs[0].isPinned);
    CHECK(report.plan.tabs[1].isPinned);
    CHECK(!report.plan.tabs[2].isPinned);
    CHECK(!report.plan.tabs[3].isPinned);
    CHECK(!report.plan.tabs[4].isPinned);

    CHECK(report.plan.tabs[0].rect.width == TabStripDensityConfiguration::kDefaultPinnedTabWidth);
    CHECK(report.plan.tabs[1].rect.width == TabStripDensityConfiguration::kDefaultPinnedTabWidth);

    CHECK(report.plan.tabs[0].rect.x + report.plan.tabs[0].rect.width <= report.plan.tabs[1].rect.x);
    CHECK(report.plan.tabs[1].rect.x + report.plan.tabs[1].rect.width <= report.plan.tabs[2].rect.x);

    CHECK(TabStripDensityConfiguration::verifyInvariants(report.plan));
}

// TEST-005: Compresión elástica progresiva de pestañas regulares
void testElasticShrinkingUnderHighTabCount() {
    TabStripDensityConfig cfg{.density = LayoutDensity::Default, .showNewTabButton = true};
    std::vector<TabSpec> tabs;
    int prevWidth = TabStripDensityConfiguration::kDefaultMaxTabWidth;

    for (int count = 1; count <= 7; ++count) {
        tabs.push_back(TabSpec{.id = static_cast<uint64_t>(count),
                               .title = "Pestaña " + std::to_string(count),
                               .pinState = TabPinState::Unpinned});
        const auto report = TabStripDensityValidator::calculateTabStripLayout(1000, cfg, tabs);
        CHECK(report.valid);
        CHECK(report.plan.effectiveTabWidth <= prevWidth);
        CHECK(report.plan.effectiveTabWidth >= TabStripDensityConfiguration::kDefaultMinTabWidth);
        prevWidth = report.plan.effectiveTabWidth;
    }
}

// TEST-006: Detección de desbordamiento horizontal y activación de scroll
void testOverflowDetectionAndModeActivation() {
    TabStripDensityConfig cfg{.density = LayoutDensity::Default,
                              .showNewTabButton = true,
                              .overflowMode = TabStripOverflowMode::Scrollable};
    std::vector<TabSpec> tabs;
    for (int i = 1; i <= 20; ++i) {
        tabs.push_back(TabSpec{.id = static_cast<uint64_t>(i),
                               .title = "Pestaña " + std::to_string(i),
                               .pinState = TabPinState::Unpinned});
    }

    const auto report = TabStripDensityValidator::calculateTabStripLayout(1000, cfg, tabs);
    CHECK(report.valid);
    CHECK(report.plan.isOverflowing);
    CHECK(report.plan.overflowMode == TabStripOverflowMode::Scrollable);
    CHECK(report.plan.scrollLeftButtonRect.width > 0);
    CHECK(report.plan.scrollRightButtonRect.width > 0);
}

// TEST-007: Ocultación de botón de cierre en pestañas angostas inactivas
void testCloseButtonVisibilityThreshold() {
    TabItemLayout unselectedNarrowTab{};
    unselectedNarrowTab.rect = LayoutRect{0, 0, 85, 36};
    unselectedNarrowTab.isSelected = false;
    unselectedNarrowTab.showCloseButton = (85 >= TabStripDensityConfiguration::kDefaultCloseThreshold);
    CHECK(!unselectedNarrowTab.showCloseButton);

    TabItemLayout selectedNarrowTab{};
    selectedNarrowTab.rect = LayoutRect{0, 0, 85, 36};
    selectedNarrowTab.isSelected = true;
    selectedNarrowTab.showCloseButton = true;
    CHECK(selectedNarrowTab.showCloseButton);

    TabSpec tabActive{.id = 1, .title = "Activa", .isSelected = true};
    TabSpec tabInactive{.id = 2, .title = "Inactiva", .isSelected = false};
    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        1000, TabStripDensityConfig{.density = LayoutDensity::Default}, {tabActive, tabInactive});
    CHECK(report.valid);
    CHECK(report.plan.tabs[0].showCloseButton);
    CHECK(report.plan.tabs[1].showCloseButton);
}

// TEST-008: Permanencia obligatoria del botón de cierre en la «Interfaz más fácil de usar»
void testSeniorModeCloseButtonPermanence() {
    TabSpec t1{.id = 1, .title = "Accesible 1", .isSelected = false};
    TabSpec t2{.id = 2, .title = "Accesible 2", .isSelected = true};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        1000, TabStripDensityConfig{.density = LayoutDensity::Senior}, {t1, t2});
    CHECK(report.valid);
    CHECK(report.plan.tabs[0].showCloseButton);
    CHECK(report.plan.tabs[1].showCloseButton);
    CHECK(report.plan.tabs[0].closeButtonRect.width >= 24);
    CHECK(report.plan.tabs[0].closeButtonRect.height >= 24);
    CHECK(report.plan.tabs[1].closeButtonRect.width >= 24);
    CHECK(report.plan.tabs[1].closeButtonRect.height >= 24);
}

// TEST-009: Rechazo de anchuras inferiores al mínimo canónico
void testSubMinimumWidthViolation() {
    const auto err = TabStripDensityValidator::validateDimensions(350, LayoutDensity::Default);
    CHECK(err == TabStripDensityError::WidthBelowMinimum);

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        350, TabStripDensityConfig{.density = LayoutDensity::Default}, {});
    CHECK(!report.valid);
    CHECK(report.error == TabStripDensityError::WidthBelowMinimum);
}

// TEST-010: Rechazo de altura < 48 px en la «Interfaz más fácil de usar»
void testSeniorHeightViolation() {
    const auto err = TabStripDensityValidator::validateDimensions(1000, 36, LayoutDensity::Senior);
    CHECK(err == TabStripDensityError::SeniorModeMinimumHeightViolation);
}

// TEST-011: Hit-testing analítico exhaustivo de todas las zonas de la tira
void testExhaustiveHitTestingZones() {
    TabSpec p1{.id = 1,
               .title = "Fijada con Audio",
               .pinState = TabPinState::Pinned,
               .mediaState = TabMediaState::AudioPlaying};
    TabSpec r1{
        .id = 2, .title = "Pestaña Regular", .pinState = TabPinState::Unpinned, .isSelected = true};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        1000, TabStripDensityConfig{.density = LayoutDensity::Default, .showNewTabButton = true},
        {p1, r1});
    CHECK(report.valid);
    const auto& plan = report.plan;

    // 1. Favicon de pestaña regular
    const auto& regTab = plan.tabs[1];
    const int favX = regTab.faviconRect.x + regTab.faviconRect.width / 2;
    const int favY = regTab.faviconRect.y + regTab.faviconRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(favX, favY, plan) == TabStripHitTestZone::TabFavicon);

    // 2. Título de pestaña regular
    const int titleX = regTab.titleRect.x + regTab.titleRect.width / 2;
    const int titleY = regTab.titleRect.y + regTab.titleRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(titleX, titleY, plan) == TabStripHitTestZone::TabTitle);

    // 3. Botón de cierre
    const int closeX = regTab.closeButtonRect.x + regTab.closeButtonRect.width / 2;
    const int closeY = regTab.closeButtonRect.y + regTab.closeButtonRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(closeX, closeY, plan) ==
          TabStripHitTestZone::TabCloseButton);

    // 4. Indicador de audio de pestaña fijada
    const auto& pinTab = plan.tabs[0];
    const int audioX = pinTab.audioIndicatorRect.x + pinTab.audioIndicatorRect.width / 2;
    const int audioY = pinTab.audioIndicatorRect.y + pinTab.audioIndicatorRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(audioX, audioY, plan) ==
          TabStripHitTestZone::TabAudioIndicator);

    // 5. Botón de nueva pestaña
    const int newTabX = plan.newTabButtonRect.x + plan.newTabButtonRect.width / 2;
    const int newTabY = plan.newTabButtonRect.y + plan.newTabButtonRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(newTabX, newTabY, plan) ==
          TabStripHitTestZone::NewTabButton);

    // 6. Espacio libre al final de la tira
    CHECK(TabStripDensityValidator::hitTest(950, 18, plan) == TabStripHitTestZone::EmptySpace);
}

// TEST-012: Hit-testing en puntos exteriores o fuera de la tira
void testHitTestingNowhereAndBoundaries() {
    const auto plan = TabStripDensityConfiguration::canonicalDefaultDensity(1000);
    CHECK(TabStripDensityValidator::hitTest(-1, 10, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(1001, 10, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(500, -1, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(500, 100, plan) == TabStripHitTestZone::Nowhere);
}

// TEST-013: Representación y control de audio/silencio
void testMediaIndicatorStateAndHitTest() {
    TabSpec tabPlaying{.id = 1, .title = "Música", .mediaState = TabMediaState::AudioPlaying};
    TabSpec tabMuted{.id = 2, .title = "Video Silenciado", .mediaState = TabMediaState::AudioMuted};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        1000, TabStripDensityConfig{.density = LayoutDensity::Default}, {tabPlaying, tabMuted});
    CHECK(report.valid);
    CHECK(report.plan.tabs[0].showAudioIndicator);
    CHECK(report.plan.tabs[0].audioIndicatorRect.width > 0);

    const int audioX =
        report.plan.tabs[0].audioIndicatorRect.x + report.plan.tabs[0].audioIndicatorRect.width / 2;
    const int audioY =
        report.plan.tabs[0].audioIndicatorRect.y + report.plan.tabs[0].audioIndicatorRect.height / 2;
    CHECK(TabStripDensityValidator::hitTest(audioX, audioY, report.plan) ==
          TabStripHitTestZone::TabAudioIndicator);

    const auto labelMuted =
        TabStripDensityValidator::tabAccessibleLabel(tabMuted, false, LayoutDensity::Default);
    CHECK(labelMuted.find("audio silenciado") != std::string::npos);

    CHECK(TabStripDensityValidator::audioButtonAccessibleLabel(false) == "Silenciar pestaña");
    CHECK(TabStripDensityValidator::audioButtonAccessibleLabel(true) == "Reactivar audio");
}

// TEST-014: Representación y etiquetas de pestañas en ahorro de memoria («Pestaña en reposo»)
void testTabDiscardStateRepresentation() {
    TabSpec tabDiscarded{
        .id = 1, .title = "Documento Pesado", .discardState = TabDiscardState::Discarded};

    const auto label =
        TabStripDensityValidator::tabAccessibleLabel(tabDiscarded, false, LayoutDensity::Default);
    CHECK(label.find("Pestaña en reposo") != std::string::npos);

    const auto tooltip = TabStripDensityValidator::tabTooltip(tabDiscarded, false);
    CHECK(tooltip.find("Liberar memoria de pestañas inactivas") != std::string::npos);
}

// TEST-015: Sanitización de títulos para protección de privacidad en modo incógnito
void testTitleSanitizationForPrivacy() {
    const std::string rawUrl = "https://usuario:secreto123@vigilio.internal/perfil?token=abc456#seccion";
    const auto sanitized = TabStripDensityValidator::sanitizeTitle(rawUrl, false);
    CHECK(sanitized.find("secreto123") == std::string::npos);
    CHECK(sanitized.find("usuario:") == std::string::npos);
    CHECK(sanitized.find("token=") == std::string::npos);
    CHECK(sanitized.find("#seccion") == std::string::npos);
    CHECK(sanitized.find("https://") == std::string::npos);
    CHECK(sanitized == "vigilio.internal/perfil");

    const auto incognito = TabStripDensityValidator::sanitizeTitle(rawUrl, true);
    CHECK(incognito.find("(Navegación privada)") != std::string::npos);
}

// TEST-016: Auditoría exhaustiva frente a docs/UX-COPY-PUBLICO.md
void testUxCopySpanishCompliance() {
    const std::vector<std::string> proscribedTerms = {
        "Senior Mode", "Modo Senior", "Prompt Builder", "DAG", "OpenSpec", "toolchain", "runner"};

    auto assertNoProscribed = [&](const std::string& text) {
        for (const auto& term : proscribedTerms) {
            if (text.find(term) != std::string::npos) {
                std::string err = "Violación de UX-COPY-PUBLICO: '";
                err += term;
                err += "' en: ";
                err += text;
                throw std::runtime_error(err);
            }
        }
    };

    const TabStripDensityError allErrors[] = {TabStripDensityError::None,
                                              TabStripDensityError::WidthBelowMinimum,
                                              TabStripDensityError::HeightBelowMinimum,
                                              TabStripDensityError::SeniorModeMinimumHeightViolation,
                                              TabStripDensityError::SeniorModeTapTargetViolation,
                                              TabStripDensityError::InvalidTabDimensions,
                                              TabStripDensityError::InvalidTabCount,
                                              TabStripDensityError::NegativeMarginOrSpacing,
                                              TabStripDensityError::AvailableWidthExhausted};

    for (auto err : allErrors) {
        const auto msg = TabStripDensityValidator::tabStripErrorToString(err);
        assertNoProscribed(msg);
        const auto hint = TabStripDensityValidator::generateRemediationHint(err);
        assertNoProscribed(hint);
    }

    CHECK(TabStripDensityValidator::densityToString(LayoutDensity::Senior) ==
          "Interfaz más fácil de usar");
    assertNoProscribed(TabStripDensityValidator::densityToString(LayoutDensity::Senior));
    assertNoProscribed(TabStripDensityValidator::densityToString(LayoutDensity::Default));
    assertNoProscribed(TabStripDensityValidator::densityToString(LayoutDensity::Compact));

    const TabStripHitTestZone allZones[] = {
        TabStripHitTestZone::Nowhere,           TabStripHitTestZone::EmptySpace,
        TabStripHitTestZone::TabBody,           TabStripHitTestZone::TabFavicon,
        TabStripHitTestZone::TabTitle,          TabStripHitTestZone::TabCloseButton,
        TabStripHitTestZone::TabAudioIndicator, TabStripHitTestZone::NewTabButton,
        TabStripHitTestZone::ScrollLeftButton,  TabStripHitTestZone::ScrollRightButton,
        TabStripHitTestZone::OverflowMenuButton};

    for (auto zone : allZones) {
        assertNoProscribed(TabStripDensityValidator::hitTestZoneToString(zone));
    }

    assertNoProscribed(TabStripDensityValidator::newTabButtonAccessibleLabel());
    assertNoProscribed(TabStripDensityValidator::audioButtonAccessibleLabel(true));
    assertNoProscribed(TabStripDensityValidator::audioButtonAccessibleLabel(false));
}

// TEST-017: Fronteras dimensionales exactas y entradas negativas (TASK-001)
void testSubMinimumDimensionsExhaustive() {
    // 1. Anchura disponible sub-mínima en Default (mínimo canónico 400 px)
    CHECK(TabStripDensityValidator::validateDimensions(399, LayoutDensity::Default) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(200, LayoutDensity::Default) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(0, LayoutDensity::Default) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(-1, LayoutDensity::Default) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(-500, LayoutDensity::Default) ==
          TabStripDensityError::WidthBelowMinimum);

    // 2. Anchura disponible sub-mínima en Senior (mínimo canónico 500 px)
    CHECK(TabStripDensityValidator::validateDimensions(499, LayoutDensity::Senior) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(400, LayoutDensity::Senior) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(0, LayoutDensity::Senior) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(-10, LayoutDensity::Senior) ==
          TabStripDensityError::WidthBelowMinimum);

    // 3. Anchura disponible sub-mínima en Compact (mínimo canónico 360 px)
    CHECK(TabStripDensityValidator::validateDimensions(359, LayoutDensity::Compact) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(100, LayoutDensity::Compact) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(0, LayoutDensity::Compact) ==
          TabStripDensityError::WidthBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(-1, LayoutDensity::Compact) ==
          TabStripDensityError::WidthBelowMinimum);

    // 4. Altura sub-mínima en Default y Compact (con anchura válida)
    CHECK(TabStripDensityValidator::validateDimensions(1000, 0, LayoutDensity::Default) ==
          TabStripDensityError::HeightBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(1000, 35, LayoutDensity::Default) ==
          TabStripDensityError::HeightBelowMinimum);
    CHECK(TabStripDensityValidator::validateDimensions(800, 27, LayoutDensity::Compact) ==
          TabStripDensityError::HeightBelowMinimum);

    // 5. Verificación a través de calculateTabStripLayout
    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        399, TabStripDensityConfig{.density = LayoutDensity::Default}, {});
    CHECK(!report.valid);
    CHECK(report.error == TabStripDensityError::WidthBelowMinimum);
    CHECK(!report.message.empty());
}

// TEST-018: Violaciones de altura mínima en Modo Senior y tap targets accesibles (TASK-002)
void testSeniorModeHeightViolations() {
    // 1. Alturas inferiores a 48 px en Senior
    const int invalidHeights[] = {47, 40, 36, 28, 10, 0};
    for (int h : invalidHeights) {
        const auto err = TabStripDensityValidator::validateDimensions(1000, h, LayoutDensity::Senior);
        CHECK(err == TabStripDensityError::SeniorModeMinimumHeightViolation);
        const auto hint = TabStripDensityValidator::generateRemediationHint(err);
        CHECK(!hint.empty());
        CHECK(hint.find("48") != std::string::npos);
    }

    // 2. Tap target del botón de nueva pestaña < 48x48 en Senior
    auto planWithSmallNewTab = TabStripDensityConfiguration::canonicalSeniorDensity(1000);
    planWithSmallNewTab.newTabButtonRect.width = 40;
    planWithSmallNewTab.newTabButtonRect.height = 40;
    CHECK(TabStripDensityValidator::validatePlan(planWithSmallNewTab) ==
          TabStripDensityError::SeniorModeTapTargetViolation);

    // 3. Pestaña fijada con tap target < 48x48 en Senior
    auto planWithSmallPinnedTab = TabStripDensityConfiguration::canonicalSeniorDensity(1000);
    TabItemLayout smallPinned{};
    smallPinned.id = 99;
    smallPinned.isPinned = true;
    smallPinned.rect = LayoutRect{0, 0, 36, 48};
    planWithSmallPinnedTab.tabs.push_back(smallPinned);
    CHECK(TabStripDensityValidator::validatePlan(planWithSmallPinnedTab) ==
          TabStripDensityError::SeniorModeTapTargetViolation);

    // 4. Botón de cierre < 24x24 en Senior
    auto planWithSmallClose = TabStripDensityConfiguration::canonicalSeniorDensity(1000);
    if (!planWithSmallClose.tabs.empty()) {
        planWithSmallClose.tabs[0].closeButtonRect = LayoutRect{0, 0, 20, 20};
        CHECK(TabStripDensityValidator::validatePlan(planWithSmallClose) ==
              TabStripDensityError::SeniorModeTapTargetViolation);
    }
}

// TEST-019: Compresión elástica extrema (100 pestañas en 400 px) y desbordamiento (TASK-003)
void testExtremeOverflowAndElasticCompression() {
    TabStripDensityConfig cfg{.density = LayoutDensity::Default,
                              .showNewTabButton = true,
                              .overflowMode = TabStripOverflowMode::Scrollable};
    std::vector<TabSpec> tabs;
    tabs.reserve(100);
    for (int i = 1; i <= 100; ++i) {
        tabs.push_back(TabSpec{.id = static_cast<uint64_t>(i),
                               .title = "Tab " + std::to_string(i),
                               .pinState = TabPinState::Unpinned,
                               .isSelected = (i == 1)});
    }

    const auto report = TabStripDensityValidator::calculateTabStripLayout(400, cfg, tabs);
    CHECK(report.valid);
    CHECK(report.plan.isOverflowing);
    CHECK(report.plan.overflowMode == TabStripOverflowMode::Scrollable);
    CHECK(report.plan.effectiveTabWidth == TabStripDensityConfiguration::kDefaultMinTabWidth);
    CHECK(report.plan.scrollLeftButtonRect.width > 0);
    CHECK(report.plan.scrollRightButtonRect.width > 0);
    CHECK(report.plan.tabs.size() == 100);

    for (std::size_t i = 0; i < report.plan.tabs.size(); ++i) {
        const auto& tab = report.plan.tabs[i];
        CHECK(tab.rect.width == TabStripDensityConfiguration::kDefaultMinTabWidth);
        CHECK(tab.rect.height == TabStripDensityConfiguration::kDefaultStripHeight);
        if (i + 1 < report.plan.tabs.size()) {
            CHECK(tab.rect.x + tab.rect.width <= report.plan.tabs[i + 1].rect.x);
        }
    }
}

// TEST-020: Saturación por pestañas fijadas y agotamiento de ancho disponible (TASK-004)
void testPinnedTabsSaturationAndSpaceExhaustion() {
    TabStripDensityConfig cfg{.density = LayoutDensity::Default, .showNewTabButton = true};
    std::vector<TabSpec> saturatedTabs;
    for (int i = 1; i <= 12; ++i) {
        saturatedTabs.push_back(TabSpec{.id = static_cast<uint64_t>(i),
                                        .title = "P" + std::to_string(i),
                                        .pinState = TabPinState::Pinned});
    }

    const auto report = TabStripDensityValidator::calculateTabStripLayout(400, cfg, saturatedTabs);
    CHECK(!report.valid);
    CHECK(report.error == TabStripDensityError::AvailableWidthExhausted);
    CHECK(!report.message.empty());
    CHECK(!report.remediationHint.empty());

    const auto okReport = TabStripDensityValidator::calculateTabStripLayout(1000, cfg, saturatedTabs);
    CHECK(okReport.valid);
    CHECK(okReport.plan.pinnedTabsCount == 12);
    CHECK(okReport.plan.newTabButtonRect.width > 0);
}

// TEST-021: Hit-testing exhaustivo de esquinas, bordes y puntos exteriores (TASK-005)
void testExhaustiveHitTestingBoundaries() {
    TabSpec t1{.id = 1,
               .title = "Hit Test Tab",
               .pinState = TabPinState::Unpinned,
               .isSelected = true,
               .mediaState = TabMediaState::AudioPlaying};

    const auto report = TabStripDensityValidator::calculateTabStripLayout(
        1000, TabStripDensityConfig{.density = LayoutDensity::Default, .showNewTabButton = true}, {t1});
    CHECK(report.valid);
    const auto& plan = report.plan;
    const auto& tab = plan.tabs[0];

    CHECK(TabStripDensityValidator::hitTest(tab.rect.x, tab.rect.y, plan) !=
          TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(tab.rect.x + tab.rect.width - 1, tab.rect.y, plan) !=
          TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(tab.rect.x, tab.rect.y + tab.rect.height - 1, plan) !=
          TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(tab.rect.x + tab.rect.width - 1,
                                            tab.rect.y + tab.rect.height - 1,
                                            plan) != TabStripHitTestZone::Nowhere);

    CHECK(TabStripDensityValidator::hitTest(tab.closeButtonRect.x, tab.closeButtonRect.y, plan) ==
          TabStripHitTestZone::TabCloseButton);
    CHECK(TabStripDensityValidator::hitTest(tab.closeButtonRect.x + tab.closeButtonRect.width - 1,
                                            tab.closeButtonRect.y + tab.closeButtonRect.height - 1,
                                            plan) == TabStripHitTestZone::TabCloseButton);

    CHECK(TabStripDensityValidator::hitTest(tab.audioIndicatorRect.x, tab.audioIndicatorRect.y, plan) ==
          TabStripHitTestZone::TabAudioIndicator);
    CHECK(TabStripDensityValidator::hitTest(tab.audioIndicatorRect.x + tab.audioIndicatorRect.width - 1,
                                            tab.audioIndicatorRect.y + tab.audioIndicatorRect.height - 1,
                                            plan) == TabStripHitTestZone::TabAudioIndicator);

    CHECK(TabStripDensityValidator::hitTest(tab.faviconRect.x, tab.faviconRect.y, plan) ==
          TabStripHitTestZone::TabFavicon);
    CHECK(TabStripDensityValidator::hitTest(tab.faviconRect.x + tab.faviconRect.width - 1,
                                            tab.faviconRect.y + tab.faviconRect.height - 1,
                                            plan) == TabStripHitTestZone::TabFavicon);

    CHECK(TabStripDensityValidator::hitTest(-1, 10, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(1000, 10, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(500, -1, plan) == TabStripHitTestZone::Nowhere);
    CHECK(TabStripDensityValidator::hitTest(500, 36, plan) == TabStripHitTestZone::Nowhere);
}

// TEST-022: Sanitización hostil de títulos y protección de privacidad (TASK-006)
void testHostileTitleSanitizationAndPrivacy() {
    const std::string hostile = "https://admin:SuperSecretToken123!@intranet.corp.internal/"
                                "dashboard?auth_token=eyJhbGciOiJIUzI1NiI&key=abc999#session";
    const auto sanitized = TabStripDensityValidator::sanitizeTitle(hostile, false);
    CHECK(sanitized.find("SuperSecretToken123!") == std::string::npos);
    CHECK(sanitized.find("admin:") == std::string::npos);
    CHECK(sanitized.find("auth_token=") == std::string::npos);
    CHECK(sanitized.find("key=") == std::string::npos);
    CHECK(sanitized.find("#session") == std::string::npos);
    CHECK(sanitized == "intranet.corp.internal/dashboard");

    const std::string withControls = "Título\r\nCon\tSaltos\nDe\rLínea";
    const auto cleaned = TabStripDensityValidator::sanitizeTitle(withControls, false);
    CHECK(cleaned.find('\r') == std::string::npos);
    CHECK(cleaned.find('\n') == std::string::npos);
    CHECK(cleaned.find('\t') == std::string::npos);

    const std::string hugeTitle(10000, 'X');
    const auto safeHuge = TabStripDensityValidator::sanitizeTitle(hugeTitle, false);
    CHECK(!safeHuge.empty());
    CHECK(safeHuge.size() <= 10000);

    CHECK(TabStripDensityValidator::sanitizeTitle("", false) == "Nueva pestaña");
    CHECK(TabStripDensityValidator::sanitizeTitle("   ", false) == "Nueva pestaña");
    CHECK(TabStripDensityValidator::sanitizeTitle("///", false) == "Nueva pestaña");

    const auto incognito = TabStripDensityValidator::sanitizeTitle("Documento", true);
    CHECK(incognito.find("(Navegación privada)") != std::string::npos);
}

// TEST-023: Auditoría exhaustiva de textos públicos frente a docs/UX-COPY-PUBLICO.md (TASK-007)
void testPublicCopyExhaustiveAudit() {
    const std::vector<std::string> proscribedTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                      "DAG",         "OpenSpec",    "toolchain",
                                                      "runner",      "Linter"};

    auto assertClean = [&](const std::string& text) {
        for (const auto& term : proscribedTerms) {
            if (text.find(term) != std::string::npos) {
                throw std::runtime_error("Violación UX-COPY-PUBLICO: '" + term + "' en: " + text);
            }
        }
    };

    const TabStripDensityError allErrors[] = {TabStripDensityError::None,
                                              TabStripDensityError::WidthBelowMinimum,
                                              TabStripDensityError::HeightBelowMinimum,
                                              TabStripDensityError::SeniorModeMinimumHeightViolation,
                                              TabStripDensityError::SeniorModeTapTargetViolation,
                                              TabStripDensityError::InvalidTabDimensions,
                                              TabStripDensityError::InvalidTabCount,
                                              TabStripDensityError::NegativeMarginOrSpacing,
                                              TabStripDensityError::AvailableWidthExhausted};

    for (auto err : allErrors) {
        assertClean(TabStripDensityValidator::tabStripErrorToString(err));
        assertClean(TabStripDensityValidator::generateRemediationHint(err));
    }

    assertClean(TabStripDensityValidator::densityToString(LayoutDensity::Senior));
    assertClean(TabStripDensityValidator::densityToString(LayoutDensity::Default));
    assertClean(TabStripDensityValidator::densityToString(LayoutDensity::Compact));

    CHECK(TabStripDensityValidator::densityToString(LayoutDensity::Senior) ==
          "Interfaz más fácil de usar");

    const TabStripHitTestZone allZones[] = {
        TabStripHitTestZone::Nowhere,           TabStripHitTestZone::EmptySpace,
        TabStripHitTestZone::TabBody,           TabStripHitTestZone::TabFavicon,
        TabStripHitTestZone::TabTitle,          TabStripHitTestZone::TabCloseButton,
        TabStripHitTestZone::TabAudioIndicator, TabStripHitTestZone::NewTabButton,
        TabStripHitTestZone::ScrollLeftButton,  TabStripHitTestZone::ScrollRightButton,
        TabStripHitTestZone::OverflowMenuButton};

    for (auto zone : allZones) {
        assertClean(TabStripDensityValidator::hitTestZoneToString(zone));
    }

    assertClean(TabStripDensityValidator::newTabButtonAccessibleLabel());
    assertClean(TabStripDensityValidator::audioButtonAccessibleLabel(true));
    assertClean(TabStripDensityValidator::audioButtonAccessibleLabel(false));

    TabSpec sampleTab{.id = 1, .title = "Prueba", .mediaState = TabMediaState::AudioMuted};
    assertClean(TabStripDensityValidator::closeButtonAccessibleLabel(sampleTab));
    assertClean(TabStripDensityValidator::tabAccessibleLabel(sampleTab, false, LayoutDensity::Senior));
    assertClean(TabStripDensityValidator::tabTooltip(sampleTab, false));
}

} // namespace

int main() {
    try {
        std::cout << "[vigilio_tab_strip_density_tests] Iniciando suite de pruebas unitarias...\n";

        testCanonicalDefaultInvariants();
        std::cout << "  - TEST-001 (Canonical Default Invariants): PASSED\n";

        testCanonicalSeniorInvariants();
        std::cout << "  - TEST-002 (Canonical Senior Invariants): PASSED\n";

        testCanonicalCompactInvariants();
        std::cout << "  - TEST-003 (Canonical Compact Invariants): PASSED\n";

        testPinnedTabsGeometricSeparation();
        std::cout << "  - TEST-004 (Pinned Tabs Geometric Separation): PASSED\n";

        testElasticShrinkingUnderHighTabCount();
        std::cout << "  - TEST-005 (Elastic Shrinking Under High Tab Count): PASSED\n";

        testOverflowDetectionAndModeActivation();
        std::cout << "  - TEST-006 (Overflow Detection & Mode Activation): PASSED\n";

        testCloseButtonVisibilityThreshold();
        std::cout << "  - TEST-007 (Close Button Visibility Threshold): PASSED\n";

        testSeniorModeCloseButtonPermanence();
        std::cout << "  - TEST-008 (Senior Mode Close Button Permanence): PASSED\n";

        testSubMinimumWidthViolation();
        std::cout << "  - TEST-009 (Sub-Minimum Width Violation): PASSED\n";

        testSeniorHeightViolation();
        std::cout << "  - TEST-010 (Senior Height Violation): PASSED\n";

        testExhaustiveHitTestingZones();
        std::cout << "  - TEST-011 (Exhaustive Hit-Testing Zones): PASSED\n";

        testHitTestingNowhereAndBoundaries();
        std::cout << "  - TEST-012 (Hit-Testing Nowhere & Boundaries): PASSED\n";

        testMediaIndicatorStateAndHitTest();
        std::cout << "  - TEST-013 (Media Indicator State & Hit-Test): PASSED\n";

        testTabDiscardStateRepresentation();
        std::cout << "  - TEST-014 (Tab Discard State Representation): PASSED\n";

        testTitleSanitizationForPrivacy();
        std::cout << "  - TEST-015 (Title Sanitization For Privacy): PASSED\n";

        testUxCopySpanishCompliance();
        std::cout << "  - TEST-016 (UX Copy Spanish Compliance): PASSED\n";

        testSubMinimumDimensionsExhaustive();
        std::cout << "  - TEST-017 (Sub-Minimum Dimensions Exhaustive): PASSED\n";

        testSeniorModeHeightViolations();
        std::cout << "  - TEST-018 (Senior Mode Height Violations): PASSED\n";

        testExtremeOverflowAndElasticCompression();
        std::cout << "  - TEST-019 (Extreme Overflow & Elastic Compression): PASSED\n";

        testPinnedTabsSaturationAndSpaceExhaustion();
        std::cout << "  - TEST-020 (Pinned Tabs Saturation & Space Exhaustion): PASSED\n";

        testExhaustiveHitTestingBoundaries();
        std::cout << "  - TEST-021 (Exhaustive Hit-Testing Boundaries): PASSED\n";

        testHostileTitleSanitizationAndPrivacy();
        std::cout << "  - TEST-022 (Hostile Title Sanitization & Privacy): PASSED\n";

        testPublicCopyExhaustiveAudit();
        std::cout << "  - TEST-023 (Public Copy Exhaustive Audit): PASSED\n";

        std::cout
            << "[vigilio_tab_strip_density_tests] Todas las 23 pruebas pasaron exitosamente (100%).\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[vigilio_tab_strip_density_tests] Error fatal en prueba: " << ex.what() << '\n';
        return 1;
    }
}
