#include "core/TitleBarConfiguration.h"
#include "core/TitleBarService.h"
#include "core/TitleBarTypes.h"
#include "core/TitleBarValidator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

std::filesystem::path createTempFilePath(const std::string& prefix) {
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return tempDir / (prefix + "_" + std::to_string(now) + ".json");
}

// TEST-TBS-001: test_service_initialization
void test_service_initialization() {
    TitleBarService defaultService;
    const auto defaultPlan = defaultService.plan();
    CHECK(defaultPlan.density == LayoutDensity::Default);
    CHECK(defaultPlan.height == 32);
    CHECK(defaultPlan.windowWidth == 1200);
    CHECK(defaultPlan.placement == TitleBarControlsPlacement::Right);
    CHECK(defaultPlan.controls.size() == 3);
    for (const auto& btn : defaultPlan.controls) {
        CHECK(btn.rect.width == 46);
        CHECK(btn.rect.height == 32);
        CHECK(btn.visible);
    }
    CHECK(!defaultService.isIncognitoMode());

    TitleBarService seniorService(LayoutDensity::Senior, 1280);
    const auto seniorPlan = seniorService.plan();
    CHECK(seniorPlan.density == LayoutDensity::Senior);
    CHECK(seniorPlan.height >= 48);
    CHECK(seniorPlan.draggableRect.width >= 160);
    CHECK(seniorPlan.controls.size() == 4);
    for (const auto& btn : seniorPlan.controls) {
        CHECK(btn.rect.width >= 48);
        CHECK(btn.rect.height >= 48);
    }

    TitleBarService compactService(LayoutDensity::Compact, 1024);
    const auto compactPlan = compactService.plan();
    CHECK(compactPlan.density == LayoutDensity::Compact);
    CHECK(compactPlan.height == 28);
    CHECK(compactPlan.controls.size() == 3);
    for (const auto& btn : compactPlan.controls) {
        CHECK(btn.rect.width == 36);
        CHECK(btn.rect.height == 28);
    }
}

// TEST-TBS-002: test_service_custom_initialization
void test_service_custom_initialization() {
    TitleBarConfig customConfig;
    customConfig.placement = TitleBarControlsPlacement::Left;
    customConfig.title = "Custom Browser";
    TitleBarService customService(customConfig, 1000);
    const auto plan = customService.plan();
    CHECK(plan.placement == TitleBarControlsPlacement::Left);
    CHECK(plan.controlsBoxRect.x == 0);
    CHECK(plan.windowWidth == 1000);
    CHECK(TitleBarValidator::validatePlan(plan) == TitleBarError::None);
}

// TEST-TBS-003 & TEST-TBS-004: test_service_density_transitions
void test_service_density_transitions() {
    TitleBarService service;

    // Transición hacia la interfaz accesible
    auto repSenior = service.setDensity(LayoutDensity::Senior);
    CHECK(repSenior.valid);
    const auto planSenior = service.plan();
    CHECK(planSenior.density == LayoutDensity::Senior);
    CHECK(planSenior.height >= 48);
    CHECK(planSenior.draggableRect.width >= 160);
    CHECK(planSenior.controls.size() == 4);
    for (const auto& btn : planSenior.controls) {
        CHECK(btn.rect.width >= 48);
        CHECK(btn.rect.height >= 48);
    }

    // Transición a densidad compacta
    auto repCompact = service.setDensity(LayoutDensity::Compact);
    CHECK(repCompact.valid);
    const auto planCompact = service.plan();
    CHECK(planCompact.density == LayoutDensity::Compact);
    CHECK(planCompact.height == 28);
    CHECK(planCompact.draggableRect.width >= 100);

    // Retorno a densidad estándar
    auto repDefault = service.setDensity(LayoutDensity::Default);
    CHECK(repDefault.valid);
    const auto planDef = service.plan();
    CHECK(planDef.density == LayoutDensity::Default);
    CHECK(planDef.height == 32);
}

// TEST-TBS-005: test_service_placement_transitions
void test_service_placement_transitions() {
    TitleBarService service;
    CHECK(service.plan().placement == TitleBarControlsPlacement::Right);

    auto repLeft = service.setControlsPlacement(TitleBarControlsPlacement::Left);
    CHECK(repLeft.valid);
    CHECK(service.plan().placement == TitleBarControlsPlacement::Left);
    CHECK(service.plan().controlsBoxRect.x == 0);

    auto repRight = service.setControlsPlacement(TitleBarControlsPlacement::Right);
    CHECK(repRight.valid);
    CHECK(service.plan().placement == TitleBarControlsPlacement::Right);
    CHECK(service.plan().controlsBoxRect.x > 0);
}

// TEST-TBS-006: test_service_title_sanitization
void test_service_title_sanitization() {
    TitleBarService service;
    auto rep = service.setTitle("  https://admin:secret123@vigilio.org/dashboard?token=xyz#tab1  ");
    CHECK(rep.valid);
    const std::string title = service.config().title;
    CHECK(title.find("https://") == std::string::npos);
    CHECK(title.find("secret123") == std::string::npos);
    CHECK(title.find("token=xyz") == std::string::npos);
    CHECK(title.find("tab1") == std::string::npos);
    CHECK(title.find("dashboard") != std::string::npos);
}

// TEST-TBS-007: test_service_auxiliary_toggles
void test_service_auxiliary_toggles() {
    TitleBarService service;
    CHECK(service.plan().iconRect.width > 0);
    const int origDrag = service.plan().draggableRect.width;

    auto repNoIcon = service.setShowIcon(false);
    CHECK(repNoIcon.valid);
    CHECK(service.plan().iconRect.width == 0);
    CHECK(service.plan().draggableRect.width > origDrag);

    auto repSeniorToggle = service.setShowSeniorToggle(true);
    CHECK(repSeniorToggle.valid);
    bool foundToggle = false;
    for (const auto& btn : service.plan().controls) {
        if (btn.type == TitleBarButtonType::SeniorModeToggle) {
            foundToggle = true;
        }
    }
    CHECK(foundToggle);
}

// TEST-TBS-008: test_service_window_state_mutations
void test_service_window_state_mutations() {
    TitleBarService service;
    TitleBarState st;
    st.isMaximized = false;
    CHECK(service.setWindowState(st).valid);
    for (const auto& btn : service.plan().controls) {
        if (btn.type == TitleBarButtonType::MaximizeRestore) {
            CHECK(btn.accessibleLabel == "Maximizar la ventana");
        }
    }

    st.isMaximized = true;
    CHECK(service.setWindowState(st).valid);
    for (const auto& btn : service.plan().controls) {
        if (btn.type == TitleBarButtonType::MaximizeRestore) {
            CHECK(btn.accessibleLabel == "Restaurar la ventana");
        }
    }

    st.isFullscreen = true;
    CHECK(service.setWindowState(st).valid);
    for (const auto& btn : service.plan().controls) {
        CHECK(!btn.visible);
    }

    st.isFullscreen = false;
    CHECK(service.setWindowState(st).valid);
    auto btnRep = service.updateButtonState(TitleBarButtonType::Close, TitleBarButtonState::Hovered);
    CHECK(btnRep.valid);
    for (const auto& btn : service.plan().controls) {
        if (btn.type == TitleBarButtonType::Close) {
            CHECK(btn.state == TitleBarButtonState::Hovered);
        }
    }
}

// TEST-TBS-009: test_service_window_resize
void test_service_window_resize() {
    TitleBarService service;
    auto rep800 = service.setWindowWidth(800);
    CHECK(rep800.valid);
    CHECK(service.windowWidth() == 800);
    CHECK(service.plan().windowWidth == 800);

    auto repSmall = service.setWindowWidth(300);
    CHECK(!repSmall.valid);
    CHECK(repSmall.error == TitleBarError::DimensionsBelowMinimum);
    CHECK(service.windowWidth() == 800);
}

// TEST-TBS-010: test_service_hit_testing
void test_service_hit_testing() {
    TitleBarService service;
    const auto plan = service.plan();
    for (const auto& ctrl : plan.controls) {
        const int testX = ctrl.rect.x + ctrl.rect.width / 2;
        const int testY = ctrl.rect.y + ctrl.rect.height / 2;
        const auto zone = service.hitTest(testX, testY);
        if (ctrl.type == TitleBarButtonType::Close) {
            CHECK(zone == TitleBarHitTestZone::CloseButton);
        } else if (ctrl.type == TitleBarButtonType::Minimize) {
            CHECK(zone == TitleBarHitTestZone::MinimizeButton);
        } else if (ctrl.type == TitleBarButtonType::MaximizeRestore) {
            CHECK(zone == TitleBarHitTestZone::MaximizeButton);
        }
    }

    if (plan.iconRect.width > 0) {
        const auto zone = service.hitTest(plan.iconRect.x + 2, plan.iconRect.y + 2);
        CHECK(zone == TitleBarHitTestZone::WindowIcon);
    }

    if (plan.draggableRect.width > 0) {
        const auto zone = service.hitTest(plan.draggableRect.x + plan.draggableRect.width / 2,
                                          plan.draggableRect.y + plan.draggableRect.height / 2);
        CHECK(zone == TitleBarHitTestZone::DraggableSpace);
    }

    CHECK(service.hitTest(-10, 10) == TitleBarHitTestZone::Nowhere);
    CHECK(service.hitTest(10, -5) == TitleBarHitTestZone::Nowhere);
    CHECK(service.hitTest(2000, 10) == TitleBarHitTestZone::Nowhere);
}

// TEST-TBS-011: test_service_persistence_roundtrip
void test_service_persistence_roundtrip() {
    const auto tempFile = createTempFilePath("tbs_roundtrip");
    TitleBarService service;
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    CHECK(service.setTitle("Roundtrip Test").valid);
    CHECK(service.setControlsPlacement(TitleBarControlsPlacement::Left).valid);
    CHECK(service.saveStateToFile(tempFile));
    CHECK(std::filesystem::exists(tempFile));

    TitleBarService loadedService;
    CHECK(loadedService.loadStateFromFile(tempFile));
    CHECK(loadedService.config().density == LayoutDensity::Senior);
    CHECK(loadedService.config().placement == TitleBarControlsPlacement::Left);

    std::error_code ec;
    std::filesystem::remove(tempFile, ec);
}

// TEST-TBS-012: test_service_persistence_corrupted_fallback
void test_service_persistence_corrupted_fallback() {
    const auto tempFile = createTempFilePath("tbs_corrupt");
    {
        std::ofstream out(tempFile);
        out << "{ \"version\": 1, \"corrupt\": true, invalid syntax... ";
    }

    TitleBarService service;
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    CHECK(!service.loadStateFromFile(tempFile));
    CHECK(service.config().density == LayoutDensity::Default);
    CHECK(service.plan().height == 32);

    std::error_code ec;
    std::filesystem::remove(tempFile, ec);
}

// TEST-TBS-013: test_service_incognito_isolation
void test_service_incognito_isolation() {
    const auto tempFile = createTempFilePath("tbs_incognito");
    TitleBarService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());
    CHECK(!service.saveStateToFile(tempFile));
    CHECK(!std::filesystem::exists(tempFile));
    CHECK(!service.loadStateFromFile(tempFile));
}

// TEST-TBS-014: test_service_cancellation
void test_service_cancellation() {
    TitleBarService service;
    CancellationToken cancel;
    cancel.requestCancellation();
    auto report = service.computeLayout(&cancel);
    CHECK(!report.valid);

    const auto tempFile = createTempFilePath("tbs_cancel");
    CHECK(!service.saveStateToFile(tempFile, &cancel));
    CHECK(!std::filesystem::exists(tempFile));
}

// Observability and copy verification
void test_service_observability_ux_copy() {
    TitleBarService service;
    bool callbackInvoked = false;
    service.setTitleBarChangedCallback(
        [&](const TitleBarLayoutPlan&, const TitleBarReport&) { callbackInvoked = true; });
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    CHECK(callbackInvoked);

    const std::string summary =
        TitleBarService::formatTitleBarSummary(service.plan(), service.computeLayout());
    CHECK(!summary.empty());
    CHECK(summary.find("Interfaz más fácil de usar") != std::string::npos);
    CHECK(summary.find("Senior Mode") == std::string::npos);
    CHECK(summary.find("Modo Senior") == std::string::npos);
    CHECK(summary.find("Prompt Builder") == std::string::npos);
    CHECK(summary.find("OpenSpec") == std::string::npos);
}

// Multithreaded concurrency stress test
void test_service_multithreaded_concurrency() {
    TitleBarService service;
    std::atomic<bool> stopFlag{false};
    std::vector<std::thread> workers;
    workers.reserve(8);

    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&service, &stopFlag, i]() {
            int counter = 0;
            while (!stopFlag.load(std::memory_order_relaxed) && counter < 100) {
                if (i % 3 == 0) {
                    (void)service.setDensity((counter % 2 == 0) ? LayoutDensity::Senior
                                                                : LayoutDensity::Default);
                } else if (i % 3 == 1) {
                    (void)service.setControlsPlacement((counter % 2 == 0)
                                                           ? TitleBarControlsPlacement::Left
                                                           : TitleBarControlsPlacement::Right);
                } else {
                    (void)service.hitTest(100 + (counter % 200), 10);
                }
                ++counter;
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    CHECK(service.plan().windowWidth > 0);
}

// TEST-TBS-015 / TASK-007: testMultiThreadedHighStressConcurrency16Threads
void testMultiThreadedHighStressConcurrency16Threads() {
    TitleBarService service;
    std::atomic<bool> startFlag{false};
    std::atomic<int> completedWorkers{0};
    std::vector<std::thread> workers;
    workers.reserve(16);

    for (int i = 0; i < 16; ++i) {
        workers.emplace_back([&service, &startFlag, &completedWorkers, i]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (int counter = 0; counter < 250; ++counter) {
                switch (i % 4) {
                case 0: {
                    const LayoutDensity d = (counter % 3 == 0)   ? LayoutDensity::Senior
                                            : (counter % 3 == 1) ? LayoutDensity::Compact
                                                                 : LayoutDensity::Default;
                    (void)service.setDensity(d);
                    break;
                }
                case 1: {
                    const auto p = (counter % 2 == 0) ? TitleBarControlsPlacement::Left
                                                      : TitleBarControlsPlacement::Right;
                    (void)service.setControlsPlacement(p);
                    break;
                }
                case 2: {
                    const int w = 600 + (counter % 800);
                    (void)service.setWindowWidth(w);
                    break;
                }
                case 3: {
                    (void)service.hitTest(150 + (counter % 300), 10);
                    const auto p = service.plan();
                    (void)service.computeLayout();
                    break;
                }
                default:
                    break;
                }
            }
            completedWorkers.fetch_add(1, std::memory_order_relaxed);
        });
    }

    startFlag.store(true, std::memory_order_release);

    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    CHECK(completedWorkers.load() == 16);
    CHECK(service.plan().windowWidth >= 400);
    CHECK(TitleBarConfiguration::verifyInvariants(service.plan()));
}

// TEST-TBS-016 / TASK-008: testCooperativeCancellationStress
void testCooperativeCancellationStress() {
    TitleBarService service;
    const auto tempFile = createTempFilePath("tbs_cancel_stress");

    for (int i = 0; i < 50; ++i) {
        CancellationToken cancel;
        cancel.requestCancellation();
        CHECK(cancel.isCancelled());

        const auto start = std::chrono::steady_clock::now();

        // 1. computeLayout con token cancelado responde de inmediato (< 10 ms)
        auto rep = service.computeLayout(&cancel);
        CHECK(!rep.valid);

        // 2. saveStateToFile con token cancelado responde de inmediato y no escribe en disco
        const bool saved = service.saveStateToFile(tempFile, &cancel);
        CHECK(!saved);
        CHECK(!std::filesystem::exists(tempFile));
        CHECK(!std::filesystem::exists(tempFile.string() + ".tmp"));

        // 3. loadStateFromFile con token cancelado responde de inmediato
        const bool loaded = service.loadStateFromFile(tempFile, &cancel);
        CHECK(!loaded);

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        CHECK(elapsed < 10);
    }

    // Invariante: plan activo intacto y válido
    CHECK(TitleBarConfiguration::verifyInvariants(service.plan()));
}

// TEST-TBS-017 / TASK-009: testCorruptedPersistenceFuzzingAndRecovery
void testCorruptedPersistenceFuzzingAndRecovery() {
    const std::string fuzzedPayloads[] = {
        "",                                                          // Archivo vacío
        "   \t\n   ",                                                // Solo whitespace
        "{",                                                         // Truncado inicio
        "{\"version\": 1, \"config\": {",                            // Truncado intermedio
        "{\"version\": 1, \"config\": {\"density\": 2, \"title\": ", // Truncado clave
        "{ bad_json: true, invalid: [1, 2, }",                       // Sintaxis rota
        "{\"version\": \"uno\", \"windowWidth\": \"grande\"}",       // Tipos dispares
        "{\"version\": 1, \"windowWidth\": -999}",                   // Ancho patológico negativo
        "{\"version\": 1, \"windowWidth\": 200, \"config\": {\"density\": 1}}", // Ancho sub-mínimo
                                                                                // Senior (200 < 500)
        "{\"version\": 999, \"windowWidth\": 1200}"                             // Versión incompatible
    };

    for (std::size_t i = 0; i < sizeof(fuzzedPayloads) / sizeof(fuzzedPayloads[0]); ++i) {
        const auto tempFile = createTempFilePath("tbs_fuzz_" + std::to_string(i));
        {
            std::ofstream out(tempFile, std::ios::binary);
            out.write(fuzzedPayloads[i].data(), static_cast<std::streamsize>(fuzzedPayloads[i].size()));
        }

        TitleBarService service;
        // Mutar estado previo para certificar restablecimiento a valores por defecto
        CHECK(service.setDensity(LayoutDensity::Senior).valid);
        CHECK(service.setControlsPlacement(TitleBarControlsPlacement::Left).valid);

        // Inyección de archivo corrupto/fuzzeado
        const bool loaded = service.loadStateFromFile(tempFile);
        CHECK(!loaded);

        // Invariante de recuperación: restaura de inmediato canonicalDefaultTitleBar(1200)
        CHECK(service.config().density == LayoutDensity::Default);
        CHECK(service.config().placement == TitleBarControlsPlacement::Right);
        CHECK(service.windowWidth() == 1200);
        CHECK(service.plan().height == 32);
        CHECK(TitleBarConfiguration::verifyInvariants(service.plan()));

        std::error_code ec;
        std::filesystem::remove(tempFile, ec);
    }
}

// TEST-TBS-018 / TASK-010: testDiskIoFailureHandling
void testDiskIoFailureHandling() {
    TitleBarService service;

    // 1. Ruta completamente inválida / no accesible
    const std::filesystem::path invalidPath = "/dev/null/impossible_dir/deep/nested/state_file.json";
    CHECK(!service.saveStateToFile(invalidPath));

    // 2. Ruta con nombre vacío
    CHECK(!service.saveStateToFile(std::filesystem::path{}));

    // 3. Simulación de directorio sin permisos de escritura
    const auto tempDir = std::filesystem::temp_directory_path() /
                         ("tbs_readonly_dir_" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);

    if (!ec && std::filesystem::exists(tempDir)) {
        // Remover permisos de escritura (solo lectura)
        std::filesystem::permissions(tempDir,
                                     std::filesystem::perms::owner_read |
                                         std::filesystem::perms::group_read |
                                         std::filesystem::perms::others_read,
                                     std::filesystem::perm_options::replace, ec);

        const auto filePath = tempDir / "readonly_save.json";
        (void)service.saveStateToFile(filePath);

        // Restaurar permisos para limpiar
        std::filesystem::permissions(tempDir, std::filesystem::perms::all,
                                     std::filesystem::perm_options::replace, ec);
        std::filesystem::remove_all(tempDir, ec);
    }
}

// TEST-TBS-019 / TASK-011: testIncognitoStrictPrivacyIsolation
void testIncognitoStrictPrivacyIsolation() {
    TitleBarService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());

    const auto tempPath = createTempFilePath("tbs_incognito_privacy");

    // 1. Guardado debe abortar de inmediato sin escribir en disco
    CHECK(!service.saveStateToFile(tempPath));
    CHECK(!std::filesystem::exists(tempPath));
    CHECK(!std::filesystem::exists(tempPath.string() + ".tmp"));

    // 2. Crear un archivo real con datos válidos
    {
        std::ofstream out(tempPath, std::ios::binary);
        out << "{\"version\": 1, \"windowWidth\": 1200, \"config\": {\"density\": 0}}";
    }
    CHECK(std::filesystem::exists(tempPath));

    // 3. Lectura en modo incógnito debe rechazar cualquier operación de I/O
    CHECK(!service.loadStateFromFile(tempPath));

    // 4. saveCurrentState también debe retornar false en incógnito
    TitleBarServiceOptions opts;
    opts.isIncognitoSession = true;
    opts.stateFilePath = tempPath;
    service.setOptions(opts);
    CHECK(!service.saveCurrentState());

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-TBS-020 / TASK-012: testControlsPlacementAndStateTransitionsExhaustive
void testControlsPlacementAndStateTransitionsExhaustive() {
    TitleBarService service;

    // 1. Alternancia continua de colocación Left <-> Right
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Colocación Izquierda (macOS / accesibilidad zurda)
        auto repLeft = service.setControlsPlacement(TitleBarControlsPlacement::Left);
        CHECK(repLeft.valid);
        auto planLeft = service.plan();
        CHECK(planLeft.placement == TitleBarControlsPlacement::Left);
        CHECK(planLeft.controlsBoxRect.x == 0);
        CHECK(planLeft.controls.size() >= 3);
        // Orden canónico: Close primero, luego Minimize, luego Maximize
        CHECK(planLeft.controls[0].type == TitleBarButtonType::Close);
        CHECK(planLeft.controls[1].type == TitleBarButtonType::Minimize);
        CHECK(planLeft.controls[2].type == TitleBarButtonType::MaximizeRestore);
        CHECK(TitleBarConfiguration::verifyInvariants(planLeft));

        // Colocación Derecha (estándar Windows / Linux)
        auto repRight = service.setControlsPlacement(TitleBarControlsPlacement::Right);
        CHECK(repRight.valid);
        auto planRight = service.plan();
        CHECK(planRight.placement == TitleBarControlsPlacement::Right);
        CHECK(planRight.controlsBoxRect.x > 0);
        CHECK(planRight.controls.size() >= 3);
        // Último botón a la derecha es Close
        CHECK(planRight.controls.back().type == TitleBarButtonType::Close);
        CHECK(TitleBarConfiguration::verifyInvariants(planRight));
    }

    // 2. Mutaciones de estado de ventana continuas
    TitleBarState state{};
    state.isMaximized = true;
    CHECK(service.setWindowState(state).valid);
    for (const auto& btn : service.plan().controls) {
        if (btn.type == TitleBarButtonType::MaximizeRestore) {
            CHECK(btn.accessibleLabel == "Restaurar la ventana");
        }
    }

    state.isFullscreen = true;
    CHECK(service.setWindowState(state).valid);
    for (const auto& btn : service.plan().controls) {
        CHECK(!btn.visible);
    }

    state.isFullscreen = false;
    state.isMaximized = false;
    state.isActive = false;
    CHECK(service.setWindowState(state).valid);
    CHECK(!service.plan().state.isActive);

    // 3. Mutación de estados de botones (Hovered, Pressed, Disabled, Normal)
    const TitleBarButtonType btnTypes[] = {
        TitleBarButtonType::Minimize, TitleBarButtonType::MaximizeRestore, TitleBarButtonType::Close,
        TitleBarButtonType::SeniorModeToggle};
    const TitleBarButtonState btnStates[] = {TitleBarButtonState::Hovered, TitleBarButtonState::Pressed,
                                             TitleBarButtonState::Disabled, TitleBarButtonState::Normal};

    for (auto bt : btnTypes) {
        for (auto bs : btnStates) {
            auto rep = service.updateButtonState(bt, bs);
            // Si el botón existe en el plan activo, debe reflejar el estado
            if (rep.valid) {
                for (const auto& c : service.plan().controls) {
                    if (c.type == bt) {
                        CHECK(c.state == bs);
                    }
                }
            }
        }
    }

    CHECK(TitleBarConfiguration::verifyInvariants(service.plan()));
}

} // namespace

int main() {
    try {
        std::cout << "[RUNNING] TitleBarServiceTests...\n";

        test_service_initialization();
        std::cout << "  ✓ TEST-TBS-001: test_service_initialization passed.\n";

        test_service_custom_initialization();
        std::cout << "  ✓ TEST-TBS-002: test_service_custom_initialization passed.\n";

        test_service_density_transitions();
        std::cout << "  ✓ TEST-TBS-003/004: test_service_density_transitions passed.\n";

        test_service_placement_transitions();
        std::cout << "  ✓ TEST-TBS-005: test_service_placement_transitions passed.\n";

        test_service_title_sanitization();
        std::cout << "  ✓ TEST-TBS-006: test_service_title_sanitization passed.\n";

        test_service_auxiliary_toggles();
        std::cout << "  ✓ TEST-TBS-007: test_service_auxiliary_toggles passed.\n";

        test_service_window_state_mutations();
        std::cout << "  ✓ TEST-TBS-008: test_service_window_state_mutations passed.\n";

        test_service_window_resize();
        std::cout << "  ✓ TEST-TBS-009: test_service_window_resize passed.\n";

        test_service_hit_testing();
        std::cout << "  ✓ TEST-TBS-010: test_service_hit_testing passed.\n";

        test_service_persistence_roundtrip();
        std::cout << "  ✓ TEST-TBS-011: test_service_persistence_roundtrip passed.\n";

        test_service_persistence_corrupted_fallback();
        std::cout << "  ✓ TEST-TBS-012: test_service_persistence_corrupted_fallback passed.\n";

        test_service_incognito_isolation();
        std::cout << "  ✓ TEST-TBS-013: test_service_incognito_isolation passed.\n";

        test_service_cancellation();
        std::cout << "  ✓ TEST-TBS-014: test_service_cancellation passed.\n";

        test_service_observability_ux_copy();
        std::cout << "  ✓ Observability: test_service_observability_ux_copy passed.\n";

        test_service_multithreaded_concurrency();
        std::cout << "  ✓ Concurrency (8 threads): test_service_multithreaded_concurrency passed.\n";

        testMultiThreadedHighStressConcurrency16Threads();
        std::cout << "  ✓ TASK-007: testMultiThreadedHighStressConcurrency16Threads passed.\n";

        testCooperativeCancellationStress();
        std::cout << "  ✓ TASK-008: testCooperativeCancellationStress passed.\n";

        testCorruptedPersistenceFuzzingAndRecovery();
        std::cout << "  ✓ TASK-009: testCorruptedPersistenceFuzzingAndRecovery passed.\n";

        testDiskIoFailureHandling();
        std::cout << "  ✓ TASK-010: testDiskIoFailureHandling passed.\n";

        testIncognitoStrictPrivacyIsolation();
        std::cout << "  ✓ TASK-011: testIncognitoStrictPrivacyIsolation passed.\n";

        testControlsPlacementAndStateTransitionsExhaustive();
        std::cout << "  ✓ TASK-012: testControlsPlacementAndStateTransitionsExhaustive passed.\n";

        std::cout << "[SUCCESS] All Title Bar Service Unit Tests passed cleanly.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FAILURE] Exception during tests: " << ex.what() << '\n';
        return 1;
    }
}
