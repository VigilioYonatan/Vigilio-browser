#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutService.h"
#include "core/MainWindowLayoutTypes.h"
#include "core/MainWindowLayoutValidator.h"

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

// Helper to create a unique temporary file path
std::filesystem::path createTempFilePath(const std::string& prefix) {
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return tempDir / (prefix + "_" + std::to_string(now) + ".json");
}

// TEST-SRV-001 & TEST-SRV-002: test_service_initialization
void test_service_initialization() {
    // Inicialización por defecto
    MainWindowLayoutService defaultService;
    const auto defaultPlan = defaultService.plan();
    CHECK(defaultPlan.density == LayoutDensity::Default);
    CHECK(defaultPlan.window.width == 1200);
    CHECK(defaultPlan.window.height == 780);
    CHECK(defaultPlan.window.minWidth == 640);
    CHECK(defaultPlan.window.minHeight == 480);
    CHECK(defaultPlan.sidebar.state == SidebarState::Collapsed);
    CHECK(defaultPlan.sidebar.width == 48);
    CHECK(!defaultService.isIncognitoMode());

    // Inicialización con plan para Interfaz más fácil de usar
    MainWindowLayoutService seniorService(MainWindowLayoutConfiguration::canonicalSeniorLayout());
    const auto seniorPlan = seniorService.plan();
    CHECK(seniorPlan.density == LayoutDensity::Senior);
    CHECK(seniorPlan.window.minWidth >= 720);
    CHECK(seniorPlan.window.minHeight >= 520);
    CHECK(MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior >= 56);
    CHECK(MainWindowLayoutConfiguration::kSeniorMinTapTargetSize >= 48);
}

// TEST-SRV-003 & TEST-SRV-004: test_service_density_transitions
void test_service_density_transitions() {
    MainWindowLayoutService service;
    bool callbackFired = false;
    service.setLayoutChangedCallback(
        [&](const MainWindowLayoutPlan& plan, const MainWindowLayoutReport& report) {
            callbackFired = true;
            CHECK(report.valid);
            CHECK(plan.density == LayoutDensity::Senior);
        });

    // Conmutación a Interfaz más fácil de usar (Senior)
    const auto reportSenior = service.setDensity(LayoutDensity::Senior);
    CHECK(reportSenior.valid);
    CHECK(callbackFired);
    const auto planSenior = service.plan();
    CHECK(planSenior.density == LayoutDensity::Senior);
    CHECK(planSenior.window.minWidth >= 720);
    CHECK(planSenior.window.minHeight >= 520);

    for (const auto& zb : planSenior.zones) {
        if (zb.zone == LayoutZone::ToolBar && zb.visible) {
            CHECK(zb.preferredHeight >= 56);
        }
        if (zb.zone == LayoutZone::TabStrip && zb.visible) {
            CHECK(zb.preferredHeight >= 48);
        }
    }

    // Conmutación a densidad Compacta
    service.clearLayoutChangedCallback();
    const auto reportCompact = service.setDensity(LayoutDensity::Compact);
    CHECK(reportCompact.valid);
    const auto planCompact = service.plan();
    CHECK(planCompact.density == LayoutDensity::Compact);
    CHECK(planCompact.window.minWidth >= 500);
    CHECK(planCompact.window.minHeight >= 380);

    for (const auto& zb : planCompact.zones) {
        if (zb.zone == LayoutZone::ToolBar && zb.visible) {
            CHECK(zb.preferredHeight == 36);
        }
        if (zb.zone == LayoutZone::TabStrip && zb.visible) {
            CHECK(zb.preferredHeight == 32);
        }
    }

    // Retorno a densidad estándar
    const auto reportDefault = service.setDensity(LayoutDensity::Default);
    CHECK(reportDefault.valid);
    CHECK(service.plan().density == LayoutDensity::Default);
}

// TEST-SRV-005 & TEST-SRV-006: test_service_window_resize
void test_service_window_resize() {
    MainWindowLayoutService service;

    // Redimensionamiento válido
    const auto validReport = service.resizeWindow(1400, 900);
    CHECK(validReport.valid);
    CHECK(service.plan().window.width == 1400);
    CHECK(service.plan().window.height == 900);

    // Intento de redimensionamiento por debajo del mínimo en densidad Default (640x480)
    const auto invalidReport = service.resizeWindow(600, 400);
    CHECK(!invalidReport.valid);
    CHECK(invalidReport.error == MainWindowLayoutError::DimensionsBelowMinimum);
    // El plan debe conservar las dimensiones válidas previas
    CHECK(service.plan().window.width == 1400);
    CHECK(service.plan().window.height == 900);
}

// TEST-SRV-007: test_service_sidebar_toggle
void test_service_sidebar_toggle() {
    MainWindowLayoutService service;
    CHECK(service.plan().sidebar.state == SidebarState::Collapsed);
    CHECK(service.plan().sidebar.width == 48);

    // Toggle a expandida
    const auto r1 = service.toggleSidebar();
    CHECK(r1.valid);
    CHECK(service.plan().sidebar.state == SidebarState::Expanded);
    CHECK(service.plan().sidebar.width == 240);

    // Toggle de vuelta a colapsada
    const auto r2 = service.toggleSidebar();
    CHECK(r2.valid);
    CHECK(service.plan().sidebar.state == SidebarState::Collapsed);
    CHECK(service.plan().sidebar.width == 48);
}

// TEST-SRV-008: test_service_sidebar_width_adjustment
void test_service_sidebar_width_adjustment() {
    MainWindowLayoutService service;

    // Ajuste válido dentro del rango [200, 360]
    const auto r1 = service.setSidebarWidth(280);
    CHECK(r1.valid);
    CHECK(service.plan().sidebar.state == SidebarState::Expanded);
    CHECK(service.plan().sidebar.width == 280);

    // Ajuste a ancho colapsado exacto (48 px)
    const auto r2 = service.setSidebarWidth(48);
    CHECK(r2.valid);
    CHECK(service.plan().sidebar.state == SidebarState::Collapsed);
    CHECK(service.plan().sidebar.width == 48);

    // Intento de ajuste fuera de rango (100 px no es 48 ni está en [200, 360])
    const auto r3 = service.setSidebarWidth(100);
    CHECK(!r3.valid);
    CHECK(r3.error == MainWindowLayoutError::InvalidSidebarWidth);
    // El plan debe permanecer intacto
    CHECK(service.plan().sidebar.state == SidebarState::Collapsed);
    CHECK(service.plan().sidebar.width == 48);
}

// TEST-SRV-009: test_service_sidebar_dock_position
void test_service_sidebar_dock_position() {
    MainWindowLayoutService service;
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Left);

    const auto rRight = service.setSidebarDockPosition(SidebarDockPosition::Right);
    CHECK(rRight.valid);
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Right);

    // Verificar en las áreas geométricas que la barra lateral está anclada a la derecha
    const auto report = service.computeLayout();
    CHECK(report.valid);
    for (const auto& zg : report.allocatedZones) {
        if (zg.zone == LayoutZone::Sidebar && zg.visible) {
            CHECK(zg.rect.x == service.plan().window.width - service.plan().sidebar.width);
        }
    }

    const auto rLeft = service.setSidebarDockPosition(SidebarDockPosition::Left);
    CHECK(rLeft.valid);
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Left);
}

// TEST-SRV-010: test_service_zone_visibility
void test_service_zone_visibility() {
    MainWindowLayoutService service;
    const auto reportNoBanner = service.computeLayout();
    const int heightBefore = reportNoBanner.contentArea.height;

    // Mostrar barra de aviso (NotificationBanner)
    const auto rBanner = service.showNotificationBanner(true);
    CHECK(rBanner.valid);
    CHECK(service.plan().showBanner);

    const auto reportWithBanner = service.computeLayout();
    const int heightAfter = reportWithBanner.contentArea.height;
    // En densidad Default, la barra de aviso tiene 44 px de altura
    CHECK(heightBefore - heightAfter == 44);

    // Ocultar barra de estado
    const auto rStatus = service.showStatusBar(false);
    CHECK(rStatus.valid);
    CHECK(!service.plan().showStatusBar);
}

// TEST-SRV-011: test_service_persistence_roundtrip
void test_service_persistence_roundtrip() {
    const auto tempPath = createTempFilePath("vigilio_layout_test_roundtrip");

    MainWindowLayoutService service1;
    CHECK(service1.resizeWindow(1350, 850).valid);
    CHECK(service1.setDensity(LayoutDensity::Senior).valid);
    CHECK(service1.setSidebarState(SidebarState::Expanded).valid);
    CHECK(service1.setSidebarWidth(300).valid);
    CHECK(service1.setSidebarDockPosition(SidebarDockPosition::Right).valid);
    CHECK(service1.showNotificationBanner(true).valid);

    const bool saved = service1.saveLayoutStateToFile(tempPath);
    CHECK(saved);
    CHECK(std::filesystem::exists(tempPath));

    MainWindowLayoutService service2;
    const bool loaded = service2.loadLayoutStateFromFile(tempPath);
    CHECK(loaded);

    const auto p1 = service1.plan();
    const auto p2 = service2.plan();

    CHECK(p2.window.width == p1.window.width);
    CHECK(p2.window.height == p1.window.height);
    CHECK(p2.window.minWidth == p1.window.minWidth);
    CHECK(p2.window.minHeight == p1.window.minHeight);
    CHECK(p2.density == p1.density);
    CHECK(p2.sidebar.state == p1.sidebar.state);
    CHECK(p2.sidebar.width == p1.sidebar.width);
    CHECK(p2.sidebar.position == p1.sidebar.position);
    CHECK(p2.showBanner == p1.showBanner);
    CHECK(p2.showStatusBar == p1.showStatusBar);
    CHECK(p2.showToolBar == p1.showToolBar);
    CHECK(p2.showTabStrip == p1.showTabStrip);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-012: test_service_persistence_corrupted_fallback
void test_service_persistence_corrupted_fallback() {
    const auto tempPath = createTempFilePath("vigilio_layout_test_corrupt");

    // Escribir un archivo JSON truncado e inválido
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        out << "{\n  \"version\": 1,\n  \"windowWidth\": 1200,\n  \"density\": \"Inva";
    }

    MainWindowLayoutService service;
    CHECK(service.resizeWindow(1400, 900).valid); // estado previo no default
    bool corruptFired = false;
    service.setLayoutChangedCallback(
        [&](const MainWindowLayoutPlan& plan, const MainWindowLayoutReport& report) {
            if (report.error == MainWindowLayoutError::LayoutPersistenceCorrupted) {
                corruptFired = true;
                CHECK(!report.valid);
                CHECK(plan.density == LayoutDensity::Default);
            }
        });

    const bool loaded = service.loadLayoutStateFromFile(tempPath);
    CHECK(!loaded);
    CHECK(corruptFired);

    // Debe haberse restaurado canonicalDefaultLayout()
    const auto restoredPlan = service.plan();
    CHECK(restoredPlan.density == LayoutDensity::Default);
    CHECK(restoredPlan.window.width == 1200);
    CHECK(restoredPlan.window.height == 780);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-013: test_service_incognito_isolation
void test_service_incognito_isolation() {
    const auto tempPath = createTempFilePath("vigilio_layout_test_incognito");

    MainWindowLayoutService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());

    MainWindowLayoutServiceOptions opts = service.options();
    opts.stateFilePath = tempPath;
    service.setOptions(opts);

    // Ningún intento de guardar en disco debe permitirse bajo modo incógnito
    const bool savedExplicit = service.saveLayoutStateToFile(tempPath);
    CHECK(!savedExplicit);
    CHECK(!std::filesystem::exists(tempPath));

    const bool savedCurrent = service.saveCurrentState();
    CHECK(!savedCurrent);
    CHECK(!std::filesystem::exists(tempPath));

    // Si se desactiva modo incógnito, la persistencia procede normalmente
    service.setIncognitoMode(false);
    CHECK(!service.isIncognitoMode());
    const bool savedNow = service.saveLayoutStateToFile(tempPath);
    CHECK(savedNow);
    CHECK(std::filesystem::exists(tempPath));

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-014: test_service_cancellation
void test_service_cancellation() {
    const auto tempPath = createTempFilePath("vigilio_layout_test_cancel");

    MainWindowLayoutService service;
    CancellationToken cancel;
    cancel.requestCancellation();
    CHECK(cancel.isCancelled());

    // computeLayout con token cancelado retorna reporte inválido
    const auto report = service.computeLayout(&cancel);
    CHECK(!report.valid);

    // saveLayoutStateToFile con token cancelado retorna false
    const bool saved = service.saveLayoutStateToFile(tempPath, &cancel);
    CHECK(!saved);
    CHECK(!std::filesystem::exists(tempPath));

    // loadLayoutStateFromFile con token cancelado retorna false
    const bool loaded = service.loadLayoutStateFromFile(tempPath, &cancel);
    CHECK(!loaded);
}

// Observabilidad y cumplimiento de docs/UX-COPY-PUBLICO.md
void test_service_observability_ux_copy() {
    MainWindowLayoutService service;
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    const auto report = service.computeLayout();
    const auto summary = MainWindowLayoutService::formatLayoutSummary(service.plan(), report);

    CHECK(!summary.empty());
    // Debe incluir la denominación oficial aprobada
    CHECK(summary.find("Interfaz más fácil de usar") != std::string::npos);

    // Prohibido terminantemente incluir tecnicismos de ingeniería interna
    CHECK(summary.find("Senior Mode") == std::string::npos);
    CHECK(summary.find("Modo Senior") == std::string::npos);
    CHECK(summary.find("Prompt Builder") == std::string::npos);
    CHECK(summary.find("OpenSpec") == std::string::npos);
}

// Move constructor & Move assignment
void test_service_move_semantics() {
    MainWindowLayoutService service1;
    CHECK(service1.resizeWindow(1300, 800).valid);
    CHECK(service1.setDensity(LayoutDensity::Compact).valid);

    MainWindowLayoutService service2 = std::move(service1);
    CHECK(service2.plan().window.width == 1300);
    CHECK(service2.plan().density == LayoutDensity::Compact);

    MainWindowLayoutService service3;
    service3 = std::move(service2);
    CHECK(service3.plan().window.width == 1300);
    CHECK(service3.plan().density == LayoutDensity::Compact);
}

// TEST-SRV-015: testMultiThreadedHighStressConcurrency16Threads
void testMultiThreadedHighStressConcurrency16Threads() {
    MainWindowLayoutService service;
    std::atomic<bool> startFlag{false};
    std::atomic<bool> stopFlag{false};
    std::vector<std::thread> workers;
    workers.reserve(16);

    // 4 threads resizing window
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&service, &startFlag, &stopFlag, t]() {
            while (!startFlag.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            int step = 0;
            while (!stopFlag.load(std::memory_order_relaxed) && step < 100) {
                int w = 1000 + ((step + t) % 10) * 80;
                int h = 700 + ((step + t) % 8) * 50;
                (void)service.resizeWindow(w, h);
                ++step;
            }
        });
    }

    // 4 threads toggling and switching densities
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&service, &startFlag, &stopFlag]() {
            while (!startFlag.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            int step = 0;
            while (!stopFlag.load(std::memory_order_relaxed) && step < 100) {
                const auto d = (step % 3 == 0)   ? LayoutDensity::Default
                               : (step % 3 == 1) ? LayoutDensity::Senior
                                                 : LayoutDensity::Compact;
                (void)service.setDensity(d);
                ++step;
            }
        });
    }

    // 4 threads mutating sidebar
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&service, &startFlag, &stopFlag, t]() {
            while (!startFlag.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            int step = 0;
            while (!stopFlag.load(std::memory_order_relaxed) && step < 100) {
                if (step % 3 == 0) {
                    (void)service.toggleSidebar();
                } else if (step % 3 == 1) {
                    (void)service.setSidebarWidth(200 + ((step + t) % 160));
                } else {
                    (void)service.setSidebarDockPosition((step % 2 == 0) ? SidebarDockPosition::Left
                                                                         : SidebarDockPosition::Right);
                }
                ++step;
            }
        });
    }

    // 4 threads reading plan and reports
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&service, &startFlag, &stopFlag]() {
            while (!startFlag.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            int step = 0;
            while (!stopFlag.load(std::memory_order_relaxed) && step < 100) {
                const auto plan = service.plan();
                CHECK(plan.window.width > 0);
                const auto report = service.computeLayout();
                CHECK(!report.message.empty());
                ++step;
            }
        });
    }

    // Fire threads simultaneously
    startFlag.store(true, std::memory_order_release);

    for (auto& th : workers) {
        if (th.joinable()) {
            th.join();
        }
    }

    // Final integrity verification
    const auto finalPlan = service.plan();
    CHECK(MainWindowLayoutConfiguration::verifyInvariants(finalPlan));
    CHECK(MainWindowLayoutValidator::validatePlan(finalPlan) == MainWindowLayoutError::None);
    const auto finalReport = service.computeLayout();
    CHECK(finalReport.valid);
    CHECK(finalReport.error == MainWindowLayoutError::None);
    CHECK(finalReport.contentArea.width >= 400);
    CHECK(finalReport.contentArea.height >= 250);
}

// TEST-SRV-016: testCooperativeCancellationStress
void testCooperativeCancellationStress() {
    MainWindowLayoutService service;
    const auto tempPath = createTempFilePath("vigilio_cancel_stress");

    for (int iter = 0; iter < 10; ++iter) {
        CancellationToken cancel;
        std::atomic<bool> workerStarted{false};

        std::thread t([&service, &tempPath, &cancel, &workerStarted]() {
            workerStarted.store(true, std::memory_order_release);
            for (int i = 0; i < 50; ++i) {
                (void)service.computeLayout(&cancel);
                (void)service.saveLayoutStateToFile(tempPath, &cancel);
                (void)service.loadLayoutStateFromFile(tempPath, &cancel);
            }
        });

        while (!workerStarted.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        cancel.requestCancellation();

        if (t.joinable()) {
            t.join();
        }
        CHECK(cancel.isCancelled());
    }

    // Plan must remain valid and consistent
    const auto plan = service.plan();
    CHECK(MainWindowLayoutValidator::validatePlan(plan) == MainWindowLayoutError::None);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-017 & TEST-SRV-018: testCorruptedLayoutPersistenceRecovery
void testCorruptedLayoutPersistenceRecovery() {
    const auto tempPath = createTempFilePath("vigilio_corrupt_recovery");
    MainWindowLayoutService service;
    const auto canonicalDefault = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    CHECK(service.resizeWindow(1400, 900).valid); // non-default state

    bool callbackInvoked = false;
    service.setLayoutChangedCallback(
        [&](const MainWindowLayoutPlan& plan, const MainWindowLayoutReport& report) {
            if (report.error == MainWindowLayoutError::LayoutPersistenceCorrupted) {
                callbackInvoked = true;
                CHECK(!report.valid);
                CHECK(plan.density == LayoutDensity::Default);
            }
        });

    // 1. Truncated JSON
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        out << "{\n  \"version\": 1,\n  \"windowWidth\": 1200,\n  \"density\": \"Inva";
    }
    CHECK(!service.loadLayoutStateFromFile(tempPath));
    CHECK(callbackInvoked);
    CHECK(service.plan().density == canonicalDefault.density);
    CHECK(service.plan().window.width == canonicalDefault.window.width);

    // 2. Empty JSON file
    callbackInvoked = false;
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    }
    CHECK(!service.loadLayoutStateFromFile(tempPath));
    CHECK(callbackInvoked);
    CHECK(service.plan().window.width == canonicalDefault.window.width);

    // 3. Valid JSON syntax but violating geometric constraints (sub-minimum window dimensions)
    callbackInvoked = false;
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        out << "{\n"
            << "  \"version\": 1,\n"
            << "  \"windowWidth\": 200,\n"
            << "  \"windowHeight\": 150,\n"
            << "  \"windowMinWidth\": 640,\n"
            << "  \"windowMinHeight\": 480,\n"
            << "  \"density\": \"Default\",\n"
            << "  \"chromeMode\": \"Standard\",\n"
            << "  \"sidebarState\": \"Collapsed\",\n"
            << "  \"sidebarDockPosition\": \"Left\",\n"
            << "  \"sidebarWidth\": 48,\n"
            << "  \"sidebarCollapsedWidth\": 48,\n"
            << "  \"sidebarDefaultExpandedWidth\": 240,\n"
            << "  \"sidebarMinExpandedWidth\": 200,\n"
            << "  \"sidebarMaxExpandedWidth\": 360,\n"
            << "  \"showTitleBar\": true,\n"
            << "  \"showTabStrip\": true,\n"
            << "  \"showToolBar\": true,\n"
            << "  \"showBanner\": false,\n"
            << "  \"showSidebar\": true,\n"
            << "  \"showStatusBar\": true\n"
            << "}\n";
    }
    CHECK(!service.loadLayoutStateFromFile(tempPath));
    CHECK(callbackInvoked);
    CHECK(service.plan().window.width == canonicalDefault.window.width);
    CHECK(service.plan().window.height == canonicalDefault.window.height);

    // 4. Valid JSON syntax but invalid sidebar width in expanded state
    callbackInvoked = false;
    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        out << "{\n"
            << "  \"version\": 1,\n"
            << "  \"windowWidth\": 1200,\n"
            << "  \"windowHeight\": 780,\n"
            << "  \"windowMinWidth\": 640,\n"
            << "  \"windowMinHeight\": 480,\n"
            << "  \"density\": \"Default\",\n"
            << "  \"chromeMode\": \"Standard\",\n"
            << "  \"sidebarState\": \"Expanded\",\n"
            << "  \"sidebarDockPosition\": \"Left\",\n"
            << "  \"sidebarWidth\": 100,\n"
            << "  \"sidebarCollapsedWidth\": 48,\n"
            << "  \"sidebarDefaultExpandedWidth\": 240,\n"
            << "  \"sidebarMinExpandedWidth\": 200,\n"
            << "  \"sidebarMaxExpandedWidth\": 360,\n"
            << "  \"showTitleBar\": true,\n"
            << "  \"showTabStrip\": true,\n"
            << "  \"showToolBar\": true,\n"
            << "  \"showBanner\": false,\n"
            << "  \"showSidebar\": true,\n"
            << "  \"showStatusBar\": true\n"
            << "}\n";
    }
    CHECK(!service.loadLayoutStateFromFile(tempPath));
    CHECK(callbackInvoked);
    CHECK(service.plan().sidebar.state == canonicalDefault.sidebar.state);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-019: testDiskFailureHandling
void testDiskFailureHandling() {
    MainWindowLayoutService service;

    // 1. Empty path handling
    CHECK(!service.saveLayoutStateToFile(""));

    // 2. Directory with read-only permissions
    const auto tempDir = std::filesystem::temp_directory_path() / "vigilio_ro_test_dir";
    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);

    std::filesystem::permissions(tempDir,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::group_read |
                                     std::filesystem::perms::others_read,
                                 std::filesystem::perm_options::replace, ec);

    const auto roFile = tempDir / "read_only_test.json";
    const bool saveResult = service.saveLayoutStateToFile(roFile);
    (void)saveResult;

    // Restore full permissions so tempDir can be removed cleanly
    std::filesystem::permissions(tempDir, std::filesystem::perms::all,
                                 std::filesystem::perm_options::replace, ec);
    std::filesystem::remove_all(tempDir, ec);

    // 3. Invalid non-creatable file path like a file path pointing through a regular file
    const auto dummyFile = std::filesystem::temp_directory_path() / "vigilio_dummy_file.tmp";
    {
        std::ofstream out(dummyFile);
        out << "content";
    }
    const auto impossiblePath = dummyFile / "subfile.json";
    CHECK(!service.saveLayoutStateToFile(impossiblePath));
    std::filesystem::remove(dummyFile, ec);
}

// TEST-SRV-020: testIncognitoModeStrictIsolation
void testIncognitoModeStrictIsolation() {
    const auto tempPath = createTempFilePath("vigilio_incognito_strict");

    MainWindowLayoutService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());

    MainWindowLayoutServiceOptions opts = service.options();
    opts.stateFilePath = tempPath;
    service.setOptions(opts);

    // In incognito mode, saving to file is rejected immediately without touching disk
    CHECK(!service.saveLayoutStateToFile(tempPath));
    CHECK(!service.saveCurrentState());
    CHECK(!std::filesystem::exists(tempPath));

    // Create a dummy layout file on disk
    {
        std::ofstream dummy(tempPath);
        dummy << "{\n  \"version\": 1,\n  \"windowWidth\": 1920\n}\n";
    }
    CHECK(std::filesystem::exists(tempPath));

    // In incognito mode, loading from file is rejected immediately without touching disk or altering
    // plan
    const int currentWidth = service.plan().window.width;
    CHECK(!service.loadLayoutStateFromFile(tempPath));
    CHECK(service.plan().window.width == currentWidth);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-SRV-021: testSidebarDockPositionAndVisibilityTransitions
void testSidebarDockPositionAndVisibilityTransitions() {
    MainWindowLayoutService service;
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Left);

    // Transition dock position to Right
    const auto rRight = service.setSidebarDockPosition(SidebarDockPosition::Right);
    CHECK(rRight.valid);
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Right);

    // Verify geometric areas reflect right dock position
    const auto repRight = service.computeLayout();
    CHECK(repRight.valid);
    for (const auto& zg : repRight.allocatedZones) {
        if (zg.zone == LayoutZone::Sidebar && zg.visible) {
            CHECK(zg.rect.x == service.plan().window.width - service.plan().sidebar.width);
        }
        if (zg.zone == LayoutZone::Content && zg.visible) {
            CHECK(zg.rect.x == 0);
            CHECK(zg.rect.width == service.plan().window.width - service.plan().sidebar.width);
        }
    }

    // Transition dock position back to Left
    const auto rLeft = service.setSidebarDockPosition(SidebarDockPosition::Left);
    CHECK(rLeft.valid);
    CHECK(service.plan().sidebar.position == SidebarDockPosition::Left);

    const auto repLeft = service.computeLayout();
    CHECK(repLeft.valid);
    for (const auto& zg : repLeft.allocatedZones) {
        if (zg.zone == LayoutZone::Sidebar && zg.visible) {
            CHECK(zg.rect.x == 0);
        }
        if (zg.zone == LayoutZone::Content && zg.visible) {
            CHECK(zg.rect.x == service.plan().sidebar.width);
        }
    }

    // Toggle optional notification banner
    const int heightBeforeBanner = service.computeLayout().contentArea.height;
    CHECK(service.showNotificationBanner(true).valid);
    CHECK(service.plan().showBanner);
    const int heightWithBanner = service.computeLayout().contentArea.height;
    CHECK(heightBeforeBanner - heightWithBanner ==
          MainWindowLayoutConfiguration::kCanonicalBannerHeightDefault);

    CHECK(service.showNotificationBanner(false).valid);
    CHECK(!service.plan().showBanner);
    CHECK(service.computeLayout().contentArea.height == heightBeforeBanner);

    // Toggle optional status bar
    const int heightWithStatus = service.computeLayout().contentArea.height;
    CHECK(service.showStatusBar(false).valid);
    CHECK(!service.plan().showStatusBar);
    const int heightWithoutStatus = service.computeLayout().contentArea.height;
    CHECK(heightWithoutStatus > heightWithStatus);

    CHECK(service.showStatusBar(true).valid);
    CHECK(service.plan().showStatusBar);
    CHECK(service.computeLayout().contentArea.height == heightWithStatus);

    // Toggle tool bar and tab strip
    CHECK(service.setZoneVisible(LayoutZone::ToolBar, false).valid);
    CHECK(!service.plan().showToolBar);
    CHECK(service.setZoneVisible(LayoutZone::ToolBar, true).valid);
    CHECK(service.plan().showToolBar);
}

} // namespace

int main() {
    try {
        std::cout << "[TEST] Running Main Window Layout Service Unit Tests...\n";

        test_service_initialization();
        std::cout << "  ✓ TEST-SRV-001/002: test_service_initialization passed.\n";

        test_service_density_transitions();
        std::cout << "  ✓ TEST-SRV-003/004: test_service_density_transitions passed.\n";

        test_service_window_resize();
        std::cout << "  ✓ TEST-SRV-005/006: test_service_window_resize passed.\n";

        test_service_sidebar_toggle();
        std::cout << "  ✓ TEST-SRV-007: test_service_sidebar_toggle passed.\n";

        test_service_sidebar_width_adjustment();
        std::cout << "  ✓ TEST-SRV-008: test_service_sidebar_width_adjustment passed.\n";

        test_service_sidebar_dock_position();
        std::cout << "  ✓ TEST-SRV-009: test_service_sidebar_dock_position passed.\n";

        test_service_zone_visibility();
        std::cout << "  ✓ TEST-SRV-010: test_service_zone_visibility passed.\n";

        test_service_persistence_roundtrip();
        std::cout << "  ✓ TEST-SRV-011: test_service_persistence_roundtrip passed.\n";

        test_service_persistence_corrupted_fallback();
        std::cout << "  ✓ TEST-SRV-012: test_service_persistence_corrupted_fallback passed.\n";

        test_service_incognito_isolation();
        std::cout << "  ✓ TEST-SRV-013: test_service_incognito_isolation passed.\n";

        test_service_cancellation();
        std::cout << "  ✓ TEST-SRV-014: test_service_cancellation passed.\n";

        test_service_observability_ux_copy();
        std::cout << "  ✓ Observability: test_service_observability_ux_copy passed.\n";

        test_service_move_semantics();
        std::cout << "  ✓ Move semantics: test_service_move_semantics passed.\n";

        testMultiThreadedHighStressConcurrency16Threads();
        std::cout << "  ✓ TEST-SRV-015: testMultiThreadedHighStressConcurrency16Threads passed.\n";

        testCooperativeCancellationStress();
        std::cout << "  ✓ TEST-SRV-016: testCooperativeCancellationStress passed.\n";

        testCorruptedLayoutPersistenceRecovery();
        std::cout << "  ✓ TEST-SRV-017/018: testCorruptedLayoutPersistenceRecovery passed.\n";

        testDiskFailureHandling();
        std::cout << "  ✓ TEST-SRV-019: testDiskFailureHandling passed.\n";

        testIncognitoModeStrictIsolation();
        std::cout << "  ✓ TEST-SRV-020: testIncognitoModeStrictIsolation passed.\n";

        testSidebarDockPositionAndVisibilityTransitions();
        std::cout << "  ✓ TEST-SRV-021: testSidebarDockPositionAndVisibilityTransitions passed.\n";

        std::cout << "[SUCCESS] All Main Window Layout Service Unit Tests passed cleanly.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FAILURE] Exception during tests: " << ex.what() << '\n';
        return 1;
    }
}
