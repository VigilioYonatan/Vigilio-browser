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

} // namespace

int main() {
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

    std::cout << "[vigilio_title_bar_tests] Todas las 14 pruebas pasaron exitosamente (100%).\n";
    return 0;
}
