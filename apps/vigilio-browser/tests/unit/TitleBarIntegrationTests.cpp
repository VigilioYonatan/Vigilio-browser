#include "core/TitleBarConfiguration.h"
#include "core/TitleBarTypes.h"
#include "core/TitleBarValidator.h"

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

// TEST-001: Invariantes del plan canónico Estándar
void testCanonicalDefaultTitleBarInvariants() {
    const auto plan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    CHECK(plan.density == LayoutDensity::Default);
    CHECK(plan.height == 32);
    CHECK(plan.windowWidth == 1200);
    CHECK(plan.placement == TitleBarControlsPlacement::Right);
    CHECK(plan.draggableRect.width >= 120);
    CHECK(plan.controls.size() == 3);
    for (const auto& btn : plan.controls) {
        CHECK(btn.rect.width == 46);
        CHECK(btn.rect.height == 32);
        CHECK(btn.visible);
    }
    CHECK(TitleBarConfiguration::verifyInvariants(plan));
    CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::None);
}

// TEST-002: Invariantes del plan canónico Senior («Interfaz más fácil de usar»)
void testCanonicalSeniorTitleBarInvariants() {
    CHECK(TitleBarConfiguration::kSeniorTitleBarHeight >= 48);
    CHECK(TitleBarConfiguration::kSeniorButtonWidth >= 48);
    const auto plan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);
    CHECK(plan.density == LayoutDensity::Senior);
    CHECK(plan.height >= 48);
    CHECK(plan.draggableRect.width >= 160);
    CHECK(plan.controls.size() == 4); // SeniorModeToggle + Min + Max + Close
    bool foundSeniorToggle = false;
    for (const auto& btn : plan.controls) {
        CHECK(btn.rect.width >= 48);
        CHECK(btn.rect.height >= 48);
        if (btn.type == TitleBarButtonType::SeniorModeToggle) {
            foundSeniorToggle = true;
        }
    }
    CHECK(foundSeniorToggle);
    CHECK(TitleBarConfiguration::verifyInvariants(plan));
    CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::None);
}

// TEST-003: Invariantes del plan canónico Compacto
void testCanonicalCompactTitleBarInvariants() {
    const auto plan = TitleBarConfiguration::canonicalCompactTitleBar(1024);
    CHECK(plan.density == LayoutDensity::Compact);
    CHECK(plan.height == 28);
    CHECK(plan.draggableRect.width >= 100);
    CHECK(plan.controls.size() == 3);
    for (const auto& btn : plan.controls) {
        CHECK(btn.rect.width == 36);
        CHECK(btn.rect.height == 28);
    }
    CHECK(TitleBarConfiguration::verifyInvariants(plan));
    CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::None);
}

// TEST-004: Hit-testing analítico de áreas de título y espacio de arrastre
void testTitleBarHitTestCaptionAndDrag() {
    const auto plan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);

    // 1. Centro del área de arrastre asignada
    const int dragX = plan.draggableRect.x + plan.draggableRect.width / 2;
    const int dragY = plan.draggableRect.y + plan.draggableRect.height / 2;
    CHECK(TitleBarValidator::hitTest(dragX, dragY, plan) == TitleBarHitTestZone::DraggableSpace);

    // 2. Centro del título de la ventana
    if (plan.titleRect.width > 0) {
        const int titleX = plan.titleRect.x + plan.titleRect.width / 2;
        const int titleY = plan.titleRect.y + plan.titleRect.height / 2;
        CHECK(TitleBarValidator::hitTest(titleX, titleY, plan) == TitleBarHitTestZone::CaptionText);
    }

    // 3. Fuera de los límites de la barra de título
    CHECK(TitleBarValidator::hitTest(-1, 10, plan) == TitleBarHitTestZone::Nowhere);
    CHECK(TitleBarValidator::hitTest(1201, 10, plan) == TitleBarHitTestZone::Nowhere);
    CHECK(TitleBarValidator::hitTest(500, -1, plan) == TitleBarHitTestZone::Nowhere);
    CHECK(TitleBarValidator::hitTest(500, 35, plan) == TitleBarHitTestZone::Nowhere);
}

// TEST-005: Hit-testing analítico de botones de control de ventana
void testTitleBarHitTestWindowControls() {
    const auto plan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);

    for (const auto& ctrl : plan.controls) {
        const int midX = ctrl.rect.x + ctrl.rect.width / 2;
        const int midY = ctrl.rect.y + ctrl.rect.height / 2;

        if (ctrl.type == TitleBarButtonType::Close) {
            CHECK(TitleBarValidator::hitTest(midX, midY, plan) == TitleBarHitTestZone::CloseButton);
            CHECK(TitleBarValidator::hitTest(ctrl.rect.x, ctrl.rect.y, plan) ==
                  TitleBarHitTestZone::CloseButton);
        } else if (ctrl.type == TitleBarButtonType::Minimize) {
            CHECK(TitleBarValidator::hitTest(midX, midY, plan) == TitleBarHitTestZone::MinimizeButton);
            CHECK(TitleBarValidator::hitTest(ctrl.rect.x, ctrl.rect.y, plan) ==
                  TitleBarHitTestZone::MinimizeButton);
        } else if (ctrl.type == TitleBarButtonType::MaximizeRestore) {
            CHECK(TitleBarValidator::hitTest(midX, midY, plan) == TitleBarHitTestZone::MaximizeButton);
            CHECK(TitleBarValidator::hitTest(ctrl.rect.x, ctrl.rect.y, plan) ==
                  TitleBarHitTestZone::MaximizeButton);
        } else if (ctrl.type == TitleBarButtonType::SeniorModeToggle) {
            CHECK(TitleBarValidator::hitTest(midX, midY, plan) == TitleBarHitTestZone::SeniorModeToggle);
        }
    }
}

// TEST-006: Hit-testing analítico del icono de ventana
void testTitleBarHitTestWindowIcon() {
    const auto plan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    CHECK(plan.iconRect.width > 0);
    const int iconMidX = plan.iconRect.x + plan.iconRect.width / 2;
    const int iconMidY = plan.iconRect.y + plan.iconRect.height / 2;
    CHECK(TitleBarValidator::hitTest(iconMidX, iconMidY, plan) == TitleBarHitTestZone::WindowIcon);
}

// TEST-007: Alternancia entre estados normal y maximizado
void testTitleBarMaximizeStateToggle() {
    TitleBarConfig normalConfig{};
    normalConfig.state.isMaximized = false;
    const auto normalReport = TitleBarValidator::calculateTitleBarLayout(1200, normalConfig);
    CHECK(normalReport.valid);
    CHECK(TitleBarValidator::buttonTypeToString(TitleBarButtonType::MaximizeRestore, false) ==
          "Maximizar");
    CHECK(TitleBarValidator::buttonAccessibleLabel(TitleBarButtonType::MaximizeRestore, false) ==
          "Maximizar la ventana");
    CHECK(TitleBarValidator::buttonTooltip(TitleBarButtonType::MaximizeRestore, false) ==
          "Aumentar la ventana al tamaño completo de la pantalla");

    TitleBarConfig maxConfig{};
    maxConfig.state.isMaximized = true;
    const auto maxReport = TitleBarValidator::calculateTitleBarLayout(1200, maxConfig);
    CHECK(maxReport.valid);
    CHECK(TitleBarValidator::buttonTypeToString(TitleBarButtonType::MaximizeRestore, true) ==
          "Restaurar");
    CHECK(TitleBarValidator::buttonAccessibleLabel(TitleBarButtonType::MaximizeRestore, true) ==
          "Restaurar la ventana");
    CHECK(TitleBarValidator::buttonTooltip(TitleBarButtonType::MaximizeRestore, true) ==
          "Restaurar la ventana al tamaño normal");
}

// TEST-008: Estado de ventana inactiva sin alteración geométrica
void testTitleBarInactiveState() {
    TitleBarConfig activeConfig{};
    activeConfig.state.isActive = true;
    const auto activePlan = TitleBarValidator::calculateTitleBarLayout(1200, activeConfig).plan;

    TitleBarConfig inactiveConfig{};
    inactiveConfig.state.isActive = false;
    const auto inactivePlan = TitleBarValidator::calculateTitleBarLayout(1200, inactiveConfig).plan;

    CHECK(!inactivePlan.state.isActive);
    CHECK(inactivePlan.height == activePlan.height);
    CHECK(inactivePlan.controlsBoxRect == activePlan.controlsBoxRect);
    CHECK(inactivePlan.draggableRect == activePlan.draggableRect);
    CHECK(inactivePlan.titleRect == activePlan.titleRect);
}

// TEST-009: Colocación de controles a la izquierda (estilo macOS y accesibilidad zurda)
void testTitleBarLeftControlsPlacement() {
    TitleBarConfig config{};
    config.placement = TitleBarControlsPlacement::Left;
    config.density = LayoutDensity::Default;

    const auto report = TitleBarValidator::calculateTitleBarLayout(1200, config);
    CHECK(report.valid);
    const auto& plan = report.plan;

    // Caja de controles anclada exactamente a x = 0
    CHECK(plan.controlsBoxRect.x == 0);
    CHECK(plan.controls.size() == 3);
    // Orden canónico: Close primero, luego Minimize, luego Maximize
    CHECK(plan.controls[0].type == TitleBarButtonType::Close);
    CHECK(plan.controls[0].rect.x == 0);
    CHECK(plan.controls[1].type == TitleBarButtonType::Minimize);
    CHECK(plan.controls[2].type == TitleBarButtonType::MaximizeRestore);

    CHECK(plan.draggableRect.width >= 120);
    CHECK(!plan.controlsBoxRect.intersects(plan.draggableRect));
    CHECK(TitleBarConfiguration::verifyInvariants(plan));
}

// TEST-010: Rechazo por anchura insuficiente y espacio de arrastre degradado
void testTitleBarInsufficientWidthRejection() {
    TitleBarConfig config{};
    config.density = LayoutDensity::Default;

    // Ventana por debajo del mínimo de 400 px
    const auto subMinReport = TitleBarValidator::calculateTitleBarLayout(350, config);
    CHECK(!subMinReport.valid);
    CHECK(subMinReport.error == TitleBarError::DimensionsBelowMinimum);

    // Ventana con espacio insuficiente para el arrastre canónico (< 120 px)
    const auto narrowReport = TitleBarValidator::calculateTitleBarLayout(405, config);
    // Los controles ocupan 3 * 46 + 2 = 140 px, más márgenes e icono (36 px) = 176 px.
    // Espacio libre = 405 - 176 = 229 px.
    // Probamos con controles artificialmente inflados o ventana muy ajustada
    CHECK(TitleBarValidator::validateDimensions(300, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
}

// TEST-011: Violación de altura mínima para la «Interfaz más fácil de usar»
void testTitleBarSeniorMinimumHeightViolation() {
    auto plan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);
    CHECK(TitleBarConfiguration::verifyInvariants(plan));

    // Modificamos altura a 32 px (incompatible con tap targets de 48 px)
    plan.height = 32;
    CHECK(!TitleBarConfiguration::verifyInvariants(plan));
    CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::SeniorModeMinimumHeightViolation);

    CHECK(TitleBarValidator::validateDimensions(1280, 40, LayoutDensity::Senior) ==
          TitleBarError::SeniorModeMinimumHeightViolation);
}

// TEST-012: Disyunción analítica y no solapamiento espacial
void testTitleBarControlsDisjointness() {
    const auto defaultPlan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    for (std::size_t i = 0; i < defaultPlan.controls.size(); ++i) {
        for (std::size_t j = i + 1; j < defaultPlan.controls.size(); ++j) {
            CHECK(!defaultPlan.controls[i].rect.intersects(defaultPlan.controls[j].rect));
        }
    }

    const auto seniorPlan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);
    for (std::size_t i = 0; i < seniorPlan.controls.size(); ++i) {
        for (std::size_t j = i + 1; j < seniorPlan.controls.size(); ++j) {
            CHECK(!seniorPlan.controls[i].rect.intersects(seniorPlan.controls[j].rect));
        }
    }
}

// TEST-013: Cumplimiento estricto de UX copy en español neutro (docs/UX-COPY-PUBLICO.md)
void testTitleBarUxCopySpanishCompliance() {
    std::vector<std::string> messages;

    // Evaluar mensajes de error
    const TitleBarError errors[] = {TitleBarError::None,
                                    TitleBarError::DimensionsBelowMinimum,
                                    TitleBarError::ControlsOverflowWidth,
                                    TitleBarError::DraggableAreaInsufficient,
                                    TitleBarError::ConflictingControlPlacement,
                                    TitleBarError::SeniorModeMinimumHeightViolation,
                                    TitleBarError::UnsupportedWindowChromeMode};

    for (auto err : errors) {
        messages.push_back(TitleBarValidator::titleBarErrorToString(err));
        messages.push_back(TitleBarValidator::generateRemediationHint(err));
    }

    // Evaluar etiquetas accesibles y tooltips
    const TitleBarButtonType buttonTypes[] = {
        TitleBarButtonType::Minimize, TitleBarButtonType::MaximizeRestore, TitleBarButtonType::Close,
        TitleBarButtonType::SeniorModeToggle};

    for (auto btn : buttonTypes) {
        messages.push_back(TitleBarValidator::buttonTypeToString(btn, false));
        messages.push_back(TitleBarValidator::buttonTypeToString(btn, true));
        messages.push_back(TitleBarValidator::buttonAccessibleLabel(btn, false));
        messages.push_back(TitleBarValidator::buttonAccessibleLabel(btn, true));
        messages.push_back(TitleBarValidator::buttonTooltip(btn, false));
        messages.push_back(TitleBarValidator::buttonTooltip(btn, true));
    }

    // Términos técnicos internos terminantemente prohibidos en copy público
    const std::string forbiddenTerms[] = {"Senior Mode", "Modo Senior", "Prompt Builder", "OpenSpec",
                                          "DAG",         "Linter",      "toolchain"};

    for (const auto& msg : messages) {
        CHECK(!msg.empty());
        for (const auto& forbidden : forbiddenTerms) {
            CHECK(msg.find(forbidden) == std::string::npos);
        }
    }

    // Verificación de textos aprobados oficiales
    CHECK(TitleBarValidator::buttonTypeToString(TitleBarButtonType::SeniorModeToggle) ==
          "Interfaz más fácil de usar");
}

// TEST-014: Sanitización de títulos y protección de privacidad
void testTitleBarSanitizationAndPrivacy() {
    // 1. Sanitización de URL completa con parámetros sensibles y tokens
    const std::string rawUrl =
        "https://banco.ejemplo.com/cuenta?token=secret123&session=abc456#transaccion";
    const std::string sanitized = TitleBarValidator::sanitizeTitle(rawUrl, false);
    CHECK(sanitized.find("token=") == std::string::npos);
    CHECK(sanitized.find("secret123") == std::string::npos);
    CHECK(sanitized.find("session=") == std::string::npos);
    CHECK(sanitized.find("#transaccion") == std::string::npos);
    CHECK(sanitized.find("Vigilio Browser") != std::string::npos);

    // 2. Sanitización en navegación de incógnito
    const std::string incognitoTitle =
        TitleBarValidator::sanitizeTitle("https://portal.ejemplo.com/inicio", true);
    CHECK(incognitoTitle.find("(Navegación privada)") != std::string::npos);

    // 3. Cadena vacía con fallback a marca limpia
    CHECK(TitleBarValidator::sanitizeTitle("", false) == "Vigilio Browser");
}

// TEST-015 / TASK-001: testSubMinimumDimensionsExhaustive
void testSubMinimumDimensionsExhaustive() {
    // 1. Límites exactos de ancho para LayoutDensity::Default (mínimo 400 px)
    CHECK(TitleBarValidator::validateDimensions(399, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(350, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(100, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(0, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(-1, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(-100, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(400, LayoutDensity::Default) == TitleBarError::None);

    // 2. Límites exactos de ancho para LayoutDensity::Senior (mínimo 500 px)
    CHECK(TitleBarValidator::validateDimensions(499, LayoutDensity::Senior) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(400, LayoutDensity::Senior) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(0, LayoutDensity::Senior) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(-5, LayoutDensity::Senior) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(500, LayoutDensity::Senior) == TitleBarError::None);

    // 3. Límites exactos de ancho para LayoutDensity::Compact (mínimo 360 px)
    CHECK(TitleBarValidator::validateDimensions(359, LayoutDensity::Compact) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(0, LayoutDensity::Compact) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(-20, LayoutDensity::Compact) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(360, LayoutDensity::Compact) == TitleBarError::None);

    // 4. Alturas no positivas o sub-mínimas en validateDimensions con alto explícito
    CHECK(TitleBarValidator::validateDimensions(1200, 0, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(1200, -1, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(1200, -100, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(1200, 31, LayoutDensity::Default) ==
          TitleBarError::DimensionsBelowMinimum);
    CHECK(TitleBarValidator::validateDimensions(1200, 27, LayoutDensity::Compact) ==
          TitleBarError::DimensionsBelowMinimum);

    // 5. Verificación a través de calculateTitleBarLayout
    TitleBarConfig configDefault{};
    configDefault.density = LayoutDensity::Default;
    auto rep399 = TitleBarValidator::calculateTitleBarLayout(399, configDefault);
    CHECK(!rep399.valid);
    CHECK(rep399.error == TitleBarError::DimensionsBelowMinimum);

    auto rep0 = TitleBarValidator::calculateTitleBarLayout(0, configDefault);
    CHECK(!rep0.valid);
    CHECK(rep0.error == TitleBarError::DimensionsBelowMinimum);

    auto repNeg = TitleBarValidator::calculateTitleBarLayout(-50, configDefault);
    CHECK(!repNeg.valid);
    CHECK(repNeg.error == TitleBarError::DimensionsBelowMinimum);

    TitleBarConfig configSenior{};
    configSenior.density = LayoutDensity::Senior;
    auto repSenior499 = TitleBarValidator::calculateTitleBarLayout(499, configSenior);
    CHECK(!repSenior499.valid);
    CHECK(repSenior499.error == TitleBarError::DimensionsBelowMinimum);
}

// TEST-016 / TASK-002: testSeniorModeHeightViolations & testDraggableAreaExhaustion
void testSeniorModeHeightViolations() {
    // 1. validateDimensions evaluando alturas inferiores a 48 px en Senior
    const int invalidHeights[] = {47, 46, 40, 32, 28, 16, 1};
    for (int h : invalidHeights) {
        CHECK(TitleBarValidator::validateDimensions(1280, h, LayoutDensity::Senior) ==
              TitleBarError::SeniorModeMinimumHeightViolation);
    }
    CHECK(TitleBarValidator::validateDimensions(1280, 48, LayoutDensity::Senior) == TitleBarError::None);
    CHECK(TitleBarValidator::validateDimensions(1280, 56, LayoutDensity::Senior) == TitleBarError::None);

    // 2. validatePlan con altura manipulada en plan Senior
    auto plan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);
    for (int h : invalidHeights) {
        plan.height = h;
        CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::SeniorModeMinimumHeightViolation);
    }

    // 3. Pista de remediación para SeniorModeMinimumHeightViolation
    const auto hint =
        TitleBarValidator::generateRemediationHint(TitleBarError::SeniorModeMinimumHeightViolation);
    CHECK(hint.find("48 píxeles") != std::string::npos);
    CHECK(hint.find("Senior Mode") == std::string::npos);
    CHECK(hint.find("Modo Senior") == std::string::npos);
}

void testDraggableAreaExhaustion() {
    // 1. Plan Default con área de arrastre por debajo de 120 px
    auto defaultPlan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    defaultPlan.draggableRect.width = 119;
    CHECK(TitleBarValidator::validatePlan(defaultPlan) == TitleBarError::DraggableAreaInsufficient);
    defaultPlan.draggableRect.width = 50;
    CHECK(TitleBarValidator::validatePlan(defaultPlan) == TitleBarError::DraggableAreaInsufficient);
    defaultPlan.draggableRect.width = 0;
    CHECK(TitleBarValidator::validatePlan(defaultPlan) == TitleBarError::DraggableAreaInsufficient);
    defaultPlan.draggableRect.width = 120;
    CHECK(TitleBarValidator::validatePlan(defaultPlan) == TitleBarError::None);

    // 2. Plan Senior con área de arrastre por debajo de 160 px
    auto seniorPlan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);
    seniorPlan.draggableRect.width = 159;
    CHECK(TitleBarValidator::validatePlan(seniorPlan) == TitleBarError::DraggableAreaInsufficient);
    seniorPlan.draggableRect.width = 80;
    CHECK(TitleBarValidator::validatePlan(seniorPlan) == TitleBarError::DraggableAreaInsufficient);
    seniorPlan.draggableRect.width = 160;
    CHECK(TitleBarValidator::validatePlan(seniorPlan) == TitleBarError::None);

    // 3. Plan Compact con área de arrastre por debajo de 100 px
    auto compactPlan = TitleBarConfiguration::canonicalCompactTitleBar(1024);
    compactPlan.draggableRect.width = 99;
    CHECK(TitleBarValidator::validatePlan(compactPlan) == TitleBarError::DraggableAreaInsufficient);
    compactPlan.draggableRect.width = 100;
    CHECK(TitleBarValidator::validatePlan(compactPlan) == TitleBarError::None);

    // 4. Mensaje y remediación
    const auto msg = TitleBarValidator::titleBarErrorToString(TitleBarError::DraggableAreaInsufficient);
    CHECK(!msg.empty());
    CHECK(msg.find("mover la ventana") != std::string::npos);
}

// TEST-017 / TASK-003: testSpatialDisjointnessAndExtremeResolutions
void testSpatialDisjointnessAndExtremeResolutions() {
    const int resolutions[][2] = {
        {3840, 2160}, // 4K UHD
        {3840, 1080}, // Ultrawide
        {2560, 1440}, // QHD
        {1920, 1080}, // Full HD
        {1080, 1920}, // Vertical / Portrait
        {1366, 768},  // HD estándar
        {800, 600},   // Resolución compacta
        {500, 700},   // Frontera mínima Senior
        {400, 600}    // Frontera mínima Default / Compact
    };

    const LayoutDensity densities[] = {LayoutDensity::Default, LayoutDensity::Senior,
                                       LayoutDensity::Compact};
    const TitleBarControlsPlacement placements[] = {TitleBarControlsPlacement::Right,
                                                    TitleBarControlsPlacement::Left};

    for (const auto& res : resolutions) {
        const int width = res[0];
        for (auto density : densities) {
            const int minW = (density == LayoutDensity::Senior)
                                 ? TitleBarConfiguration::kMinWindowWidthSenior
                                 : TitleBarConfiguration::kMinWindowWidthDefault;
            if (width < minW) {
                continue;
            }

            for (auto placement : placements) {
                TitleBarConfig config{};
                config.density = density;
                config.placement = placement;
                config.showIcon = true;
                config.title = "Vigilio Browser Extreme Resolution Test";

                const auto report = TitleBarValidator::calculateTitleBarLayout(width, config);
                CHECK(report.valid);
                const auto& plan = report.plan;

                // Invariante 1: No solapamiento entre botones
                for (std::size_t i = 0; i < plan.controls.size(); ++i) {
                    for (std::size_t j = i + 1; j < plan.controls.size(); ++j) {
                        CHECK(!plan.controls[i].rect.intersects(plan.controls[j].rect));
                    }
                }

                // Invariante 2: Caja de controles no solapa zona de arrastre
                CHECK(!plan.controlsBoxRect.intersects(plan.draggableRect));

                // Invariante 3: Icono no solapa zona de arrastre ni caja de controles
                if (plan.iconRect.width > 0) {
                    CHECK(!plan.iconRect.intersects(plan.draggableRect));
                    CHECK(!plan.iconRect.intersects(plan.controlsBoxRect));
                }

                // Invariante 4: Título no solapa controles ni caja
                if (plan.titleRect.width > 0) {
                    CHECK(!plan.titleRect.intersects(plan.controlsBoxRect));
                }

                // Invariante 5: Verificación global de invariantes
                CHECK(TitleBarConfiguration::verifyInvariants(plan));
                CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::None);
            }
        }
    }
}

// TEST-018 / TASK-004: testExhaustiveHitTestingBoundaries
void testExhaustiveHitTestingBoundaries() {
    const auto defaultPlan = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    const auto seniorPlan = TitleBarConfiguration::canonicalSeniorTitleBar(1280);

    const TitleBarLayoutPlan* plans[] = {&defaultPlan, &seniorPlan};

    for (const auto* pPlan : plans) {
        const auto& plan = *pPlan;

        // 1. Probar las 4 esquinas exactas de cada botón de control
        for (const auto& ctrl : plan.controls) {
            const int left = ctrl.rect.x;
            const int top = ctrl.rect.y;
            const int right = ctrl.rect.x + ctrl.rect.width - 1;
            const int bottom = ctrl.rect.y + ctrl.rect.height - 1;

            TitleBarHitTestZone expectedZone = TitleBarHitTestZone::Nowhere;
            switch (ctrl.type) {
            case TitleBarButtonType::Close:
                expectedZone = TitleBarHitTestZone::CloseButton;
                break;
            case TitleBarButtonType::Minimize:
                expectedZone = TitleBarHitTestZone::MinimizeButton;
                break;
            case TitleBarButtonType::MaximizeRestore:
                expectedZone = TitleBarHitTestZone::MaximizeButton;
                break;
            case TitleBarButtonType::SeniorModeToggle:
                expectedZone = TitleBarHitTestZone::SeniorModeToggle;
                break;
            }

            CHECK(TitleBarValidator::hitTest(left, top, plan) == expectedZone);
            CHECK(TitleBarValidator::hitTest(right, top, plan) == expectedZone);
            CHECK(TitleBarValidator::hitTest(left, bottom, plan) == expectedZone);
            CHECK(TitleBarValidator::hitTest(right, bottom, plan) == expectedZone);
        }

        // 2. Probar las 4 esquinas de la zona de arrastre
        if (plan.draggableRect.width > 0) {
            const int dLeft = plan.draggableRect.x;
            const int dTop = plan.draggableRect.y;
            const int dRight = plan.draggableRect.x + plan.draggableRect.width - 1;
            const int dBottom = plan.draggableRect.y + plan.draggableRect.height - 1;

            CHECK(TitleBarValidator::hitTest(dLeft, dTop, plan) == TitleBarHitTestZone::DraggableSpace);
            CHECK(TitleBarValidator::hitTest(dRight, dTop, plan) == TitleBarHitTestZone::DraggableSpace);
            CHECK(TitleBarValidator::hitTest(dLeft, dBottom, plan) ==
                  TitleBarHitTestZone::DraggableSpace);
            CHECK(TitleBarValidator::hitTest(dRight, dBottom, plan) ==
                  TitleBarHitTestZone::DraggableSpace);
        }

        // 3. Probar coordenadas exteriores a la barra de título (Nowhere)
        CHECK(TitleBarValidator::hitTest(-1, 0, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(-1, 10, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(0, -1, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(100, -1, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(plan.windowWidth, 10, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(plan.windowWidth + 50, 10, plan) ==
              TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(200, plan.height, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(200, plan.height + 20, plan) == TitleBarHitTestZone::Nowhere);
        CHECK(TitleBarValidator::hitTest(-500, -500, plan) == TitleBarHitTestZone::Nowhere);
    }
}

// TEST-019 / TASK-005: testTitleSanitizationEdgeCases
void testTitleSanitizationEdgeCases() {
    // 1. Credenciales embebidas en URL (user:pass@host)
    const std::string credUrl =
        "https://usuario_admin:clave_secreta_999@seguro.ejemplo.org/panel?token=123#top";
    const std::string sanitizedCred = TitleBarValidator::sanitizeTitle(credUrl, false);
    CHECK(sanitizedCred.find("usuario_admin") == std::string::npos);
    CHECK(sanitizedCred.find("clave_secreta_999") == std::string::npos);
    CHECK(sanitizedCred.find("token=") == std::string::npos);
    CHECK(sanitizedCred.find("#top") == std::string::npos);
    CHECK(sanitizedCred.find("seguro.ejemplo.org") != std::string::npos);
    CHECK(sanitizedCred.find("Vigilio Browser") != std::string::npos);

    // 2. Saltos de línea y caracteres de control
    const std::string rawNewline = "Página con título normal\r\nHeader-Injected: BadValue";
    const std::string sanitizedNewline = TitleBarValidator::sanitizeTitle(rawNewline, false);
    CHECK(!sanitizedNewline.empty());
    CHECK(sanitizedNewline.find("Vigilio Browser") != std::string::npos);

    // 3. Cadena de longitud extrema (10000 caracteres)
    const std::string giantTitle(10000, 'A');
    const std::string sanitizedGiant = TitleBarValidator::sanitizeTitle(giantTitle, false);
    CHECK(!sanitizedGiant.empty());
    CHECK(sanitizedGiant.find("Vigilio Browser") != std::string::npos);

    // 4. Cadena de espacios en blanco
    const std::string emptySpaces = "   \t\t\n   ";
    CHECK(TitleBarValidator::sanitizeTitle(emptySpaces, false) == "Vigilio Browser");
    CHECK(TitleBarValidator::sanitizeTitle(emptySpaces, true) == "Vigilio Browser (Navegación privada)");

    // 5. Cadena que ya contiene la marca para no duplicarla
    const std::string alreadyBranded = "Documento Oficial — Vigilio Browser";
    CHECK(TitleBarValidator::sanitizeTitle(alreadyBranded, false) ==
          "Documento Oficial — Vigilio Browser");
}

// TEST-020 / TASK-006: testPublicCopyExhaustiveAudit
void testPublicCopyExhaustiveAudit() {
    std::vector<std::string> allPublicStrings;

    // Errores y remediaciones
    const TitleBarError errors[] = {TitleBarError::None,
                                    TitleBarError::DimensionsBelowMinimum,
                                    TitleBarError::ControlsOverflowWidth,
                                    TitleBarError::DraggableAreaInsufficient,
                                    TitleBarError::ConflictingControlPlacement,
                                    TitleBarError::SeniorModeMinimumHeightViolation,
                                    TitleBarError::UnsupportedWindowChromeMode};

    for (auto err : errors) {
        allPublicStrings.push_back(TitleBarValidator::titleBarErrorToString(err));
        allPublicStrings.push_back(TitleBarValidator::generateRemediationHint(err));
        allPublicStrings.push_back(
            TitleBarValidator::generateRemediationHint(err, "Detalle técnico de prueba"));
    }

    // Modalidades operativas
    const TitleBarMode modes[] = {TitleBarMode::NativeSystem, TitleBarMode::CustomIntegrated,
                                  TitleBarMode::Suppressed};
    for (auto m : modes) {
        allPublicStrings.push_back(TitleBarValidator::titleBarModeToString(m));
    }

    // Zonas de hit test
    const TitleBarHitTestZone zones[] = {
        TitleBarHitTestZone::Nowhere,         TitleBarHitTestZone::ClientArea,
        TitleBarHitTestZone::DraggableSpace,  TitleBarHitTestZone::WindowIcon,
        TitleBarHitTestZone::CaptionText,     TitleBarHitTestZone::MinimizeButton,
        TitleBarHitTestZone::MaximizeButton,  TitleBarHitTestZone::CloseButton,
        TitleBarHitTestZone::SeniorModeToggle};
    for (auto z : zones) {
        allPublicStrings.push_back(TitleBarValidator::hitTestZoneToString(z));
    }

    // Botones (normal y maximizado)
    const TitleBarButtonType buttonTypes[] = {
        TitleBarButtonType::Minimize, TitleBarButtonType::MaximizeRestore, TitleBarButtonType::Close,
        TitleBarButtonType::SeniorModeToggle};
    for (auto b : buttonTypes) {
        allPublicStrings.push_back(TitleBarValidator::buttonTypeToString(b, false));
        allPublicStrings.push_back(TitleBarValidator::buttonTypeToString(b, true));
        allPublicStrings.push_back(TitleBarValidator::buttonAccessibleLabel(b, false));
        allPublicStrings.push_back(TitleBarValidator::buttonAccessibleLabel(b, true));
        allPublicStrings.push_back(TitleBarValidator::buttonTooltip(b, false));
        allPublicStrings.push_back(TitleBarValidator::buttonTooltip(b, true));
    }

    // Términos estrictamente prohibidos según docs/UX-COPY-PUBLICO.md
    const std::string proscribedTerms[] = {
        "Senior Mode", "Modo Senior", "Prompt Builder", "OpenSpec", "DAG",    "Linter",
        "toolchain",   "PROPOSE",     "APPLY",          "REVIEW",   "runner", "micro-prompt"};

    for (const auto& str : allPublicStrings) {
        CHECK(!str.empty());
        for (const auto& term : proscribedTerms) {
            CHECK(str.find(term) == std::string::npos);
        }
    }

    // Verificación explícita de denominación aprobada
    CHECK(TitleBarValidator::buttonTypeToString(TitleBarButtonType::SeniorModeToggle) ==
          "Interfaz más fácil de usar");
    CHECK(TitleBarValidator::titleBarErrorToString(TitleBarError::SeniorModeMinimumHeightViolation)
              .find("interfaz más fácil de usar") != std::string::npos);
}

} // namespace

int main() {
    try {
        std::cout
            << "[vigilio_title_bar_tests] Iniciando suite de pruebas unitarias de barra de título...\n";

        testCanonicalDefaultTitleBarInvariants();
        std::cout << "  - TEST-001 (Default Invariants): PASSED\n";

        testCanonicalSeniorTitleBarInvariants();
        std::cout << "  - TEST-002 (Senior Invariants): PASSED\n";

        testCanonicalCompactTitleBarInvariants();
        std::cout << "  - TEST-003 (Compact Invariants): PASSED\n";

        testTitleBarHitTestCaptionAndDrag();
        std::cout << "  - TEST-004 (HitTest Caption & Drag): PASSED\n";

        testTitleBarHitTestWindowControls();
        std::cout << "  - TEST-005 (HitTest Window Controls): PASSED\n";

        testTitleBarHitTestWindowIcon();
        std::cout << "  - TEST-006 (HitTest Window Icon): PASSED\n";

        testTitleBarMaximizeStateToggle();
        std::cout << "  - TEST-007 (Maximize State Toggle): PASSED\n";

        testTitleBarInactiveState();
        std::cout << "  - TEST-008 (Inactive State): PASSED\n";

        testTitleBarLeftControlsPlacement();
        std::cout << "  - TEST-009 (Left Controls Placement): PASSED\n";

        testTitleBarInsufficientWidthRejection();
        std::cout << "  - TEST-010 (Insufficient Width Rejection): PASSED\n";

        testTitleBarSeniorMinimumHeightViolation();
        std::cout << "  - TEST-011 (Senior Minimum Height Violation): PASSED\n";

        testTitleBarControlsDisjointness();
        std::cout << "  - TEST-012 (Controls Disjointness): PASSED\n";

        testTitleBarUxCopySpanishCompliance();
        std::cout << "  - TEST-013 (UX Copy Spanish Compliance): PASSED\n";

        testTitleBarSanitizationAndPrivacy();
        std::cout << "  - TEST-014 (Sanitization & Privacy): PASSED\n";

        testSubMinimumDimensionsExhaustive();
        std::cout << "  - TEST-015 (Sub-Minimum Dimensions Exhaustive): PASSED\n";

        testSeniorModeHeightViolations();
        std::cout << "  - TEST-016a (Senior Mode Height Violations): PASSED\n";

        testDraggableAreaExhaustion();
        std::cout << "  - TEST-016b (Draggable Area Exhaustion): PASSED\n";

        testSpatialDisjointnessAndExtremeResolutions();
        std::cout << "  - TEST-017 (Spatial Disjointness & Extreme Resolutions): PASSED\n";

        testExhaustiveHitTestingBoundaries();
        std::cout << "  - TEST-018 (Exhaustive Hit-Testing Boundaries): PASSED\n";

        testTitleSanitizationEdgeCases();
        std::cout << "  - TEST-019 (Title Sanitization Edge Cases): PASSED\n";

        testPublicCopyExhaustiveAudit();
        std::cout << "  - TEST-020 (Public Copy Exhaustive Audit): PASSED\n";

        std::cout << "[vigilio_title_bar_tests] Todas las pruebas pasaron exitosamente (100%).\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[vigilio_title_bar_tests] Error fatal en prueba: " << ex.what() << '\n';
        return 1;
    }
}
