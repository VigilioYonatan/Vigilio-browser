#include "core/MainWindowLayoutTypes.h"
#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityService.h"
#include "core/TabStripDensityTypes.h"
#include "core/TabStripDensityValidator.h"

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

// TEST-TSS-001: test_service_initialization
void test_service_initialization() {
    TabStripDensityService defaultService;
    const auto defaultPlan = defaultService.plan();
    CHECK(defaultPlan.density == LayoutDensity::Default);
    CHECK(defaultPlan.height == 36);
    CHECK(defaultPlan.availableWidth == 1000);
    CHECK(defaultPlan.tabs.empty());
    CHECK(defaultService.tabCount() == 0);
    CHECK(!defaultService.isOverflowing());
    CHECK(!defaultService.isIncognitoMode());

    TabStripDensityService seniorService(LayoutDensity::Senior, 1000);
    const auto seniorPlan = seniorService.plan();
    CHECK(seniorPlan.density == LayoutDensity::Senior);
    CHECK(seniorPlan.height >= 48);
    CHECK(seniorPlan.newTabButtonRect.width >= 48);
    CHECK(seniorPlan.newTabButtonRect.height >= 48);

    TabStripDensityService compactService(LayoutDensity::Compact, 800);
    const auto compactPlan = compactService.plan();
    CHECK(compactPlan.density == LayoutDensity::Compact);
    CHECK(compactPlan.height == 28);
}

// TEST-TSS-002: test_service_density_transitions
void test_service_density_transitions() {
    TabStripDensityService service;
    TabSpec t1{.id = 1, .title = "Página 1", .url = "https://vigilio.internal/1", .isSelected = true};
    TabSpec t2{.id = 2, .title = "Página 2", .url = "https://vigilio.internal/2", .isSelected = false};
    CHECK(service.addTab(t1).valid);
    CHECK(service.addTab(t2).valid);

    auto repSenior = service.setDensity(LayoutDensity::Senior);
    CHECK(repSenior.valid);
    const auto planSenior = service.plan();
    CHECK(planSenior.density == LayoutDensity::Senior);
    CHECK(planSenior.height >= 48);
    CHECK(planSenior.newTabButtonRect.width >= 48);
    CHECK(planSenior.newTabButtonRect.height >= 48);
    for (const auto& tab : planSenior.tabs) {
        CHECK(tab.rect.height >= 48);
        CHECK(tab.showCloseButton);
    }

    auto repCompact = service.setDensity(LayoutDensity::Compact);
    CHECK(repCompact.valid);
    const auto planCompact = service.plan();
    CHECK(planCompact.density == LayoutDensity::Compact);
    CHECK(planCompact.height == 28);

    auto repDefault = service.setDensity(LayoutDensity::Default);
    CHECK(repDefault.valid);
    CHECK(service.plan().density == LayoutDensity::Default);
    CHECK(service.plan().height == 36);
}

// TEST-TSS-003: test_service_custom_initialization
void test_service_custom_initialization() {
    TabStripDensityConfig customConfig{.density = LayoutDensity::Default,
                                       .showNewTabButton = false,
                                       .overflowMode = TabStripOverflowMode::DropdownMenu,
                                       .scrollOffset = 0};
    TabSpec tab{.id = 10,
                .title = "Inicio",
                .url = "https://vigilio.internal",
                .pinState = TabPinState::Unpinned,
                .isSelected = true};
    TabStripDensityService service(customConfig, 800, {tab});
    CHECK(service.tabCount() == 1);
    CHECK(service.config().overflowMode == TabStripOverflowMode::DropdownMenu);
    CHECK(!service.config().showNewTabButton);
    CHECK(service.availableWidth() == 800);
}

// TEST-TSS-004: test_service_tab_mutations
void test_service_tab_mutations() {
    TabStripDensityService service;
    TabSpec t1{.id = 1, .title = "Tab 1", .isSelected = true};
    TabSpec t2{.id = 2, .title = "Tab 2", .isSelected = false};
    CHECK(service.addTab(t1).valid);
    CHECK(service.addTab(t2).valid);
    CHECK(service.tabCount() == 2);

    TabSpec t3{.id = 3, .title = "Tab 3", .isSelected = false};
    CHECK(service.insertTab(1, t3).valid);
    CHECK(service.tabCount() == 3);
    CHECK(service.tabAtIndex(1)->id == 3);
}

// TEST-TSS-005: test_service_tab_remove_adjacent_selection
void test_service_tab_remove_adjacent_selection() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .title = "Tab 1", .isSelected = false}).valid);
    CHECK(service.addTab(TabSpec{.id = 2, .title = "Tab 2", .isSelected = true}).valid);
    CHECK(service.addTab(TabSpec{.id = 3, .title = "Tab 3", .isSelected = false}).valid);
    CHECK(service.selectedTabId() == 2);

    // Cerrar la pestaña central seleccionada (id 2) -> la siguiente a la derecha (id 3) toma selección
    CHECK(service.removeTab(2).valid);
    CHECK(service.tabCount() == 2);
    CHECK(service.selectedTabId() == 3);

    // Cerrar la última pestaña seleccionada (id 3) -> la anterior a la izquierda (id 1) toma selección
    CHECK(service.removeTab(3).valid);
    CHECK(service.tabCount() == 1);
    CHECK(service.selectedTabId() == 1);
}

// TEST-TSS-006: test_service_tab_pinning_precedence
void test_service_tab_pinning_precedence() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .title = "Reg 1", .pinState = TabPinState::Unpinned}).valid);
    CHECK(service.addTab(TabSpec{.id = 2, .title = "Reg 2", .pinState = TabPinState::Unpinned}).valid);
    CHECK(service.addTab(TabSpec{.id = 3, .title = "Pin 1", .pinState = TabPinState::Pinned}).valid);

    // Pestaña fijada (id 3) debe preceder a las regulares
    CHECK(service.tabAtIndex(0)->id == 3);
    CHECK(service.tabAtIndex(1)->id == 1);
    CHECK(service.tabAtIndex(2)->id == 2);

    const auto plan = service.plan();
    CHECK(plan.tabs[0].isPinned);
    CHECK(!plan.tabs[1].isPinned);
    CHECK(!plan.tabs[2].isPinned);
    CHECK(plan.tabs[0].rect.x < plan.tabs[1].rect.x);
    CHECK(plan.tabs[1].rect.x < plan.tabs[2].rect.x);

    // Alternar tab 2 a fijada:
    CHECK(service.setTabPinned(2, true).valid);
    CHECK(service.tabAtIndex(0)->id == 3);
    CHECK(service.tabAtIndex(1)->id == 2);
    CHECK(service.tabAtIndex(2)->id == 1);
}

// TEST-TSS-007: test_service_tab_reordering
void test_service_tab_reordering() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .pinState = TabPinState::Pinned}).valid);
    CHECK(service.addTab(TabSpec{.id = 2, .pinState = TabPinState::Pinned}).valid);
    CHECK(service.addTab(TabSpec{.id = 3, .pinState = TabPinState::Unpinned}).valid);
    CHECK(service.addTab(TabSpec{.id = 4, .pinState = TabPinState::Unpinned}).valid);

    // Reordenar dentro de la partición de fijadas
    CHECK(service.moveTab(1, 1).valid);
    CHECK(service.tabAtIndex(0)->id == 2);
    CHECK(service.tabAtIndex(1)->id == 1);

    // Reordenar dentro de la partición de regulares
    CHECK(service.moveTab(3, 3).valid);
    CHECK(service.tabAtIndex(2)->id == 4);
    CHECK(service.tabAtIndex(3)->id == 3);
}

// TEST-TSS-008: test_service_media_state_and_audio_toggle
void test_service_media_state_and_audio_toggle() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .title = "Video", .mediaState = TabMediaState::AudioPlaying})
              .valid);

    auto plan = service.plan();
    CHECK(plan.tabs[0].showAudioIndicator);

    CHECK(service.toggleTabAudioMute(1).valid);
    CHECK(service.tab(1)->mediaState == TabMediaState::AudioMuted);

    CHECK(service.toggleTabAudioMute(1).valid);
    CHECK(service.tab(1)->mediaState == TabMediaState::AudioPlaying);
}

// TEST-TSS-009: test_service_memory_saver_discard_state
void test_service_memory_saver_discard_state() {
    TabStripDensityService service;
    CHECK(service
              .addTab(TabSpec{.id = 1, .title = "Inactive Tab", .discardState = TabDiscardState::Active})
              .valid);

    CHECK(service.setTabDiscardState(1, TabDiscardState::Discarded).valid);
    CHECK(service.tab(1)->discardState == TabDiscardState::Discarded);

    const auto plan = service.plan();
    CHECK(plan.tabs[0].tooltip.find("reposo") != std::string::npos ||
          plan.tabs[0].accessibleLabel.find("reposo") != std::string::npos);
}

// TEST-TSS-010: test_service_title_and_url_sanitization
void test_service_title_and_url_sanitization() {
    TabStripDensityService service;
    CHECK(service
              .addTab(TabSpec{.id = 1,
                              .title = "https://user:secret@example.com/login?token=abc#frag",
                              .url = "https://user:secret@example.com/login?token=abc"})
              .valid);

    const auto tab = service.tab(1);
    CHECK(tab.has_value());
    CHECK(tab->title.find("user:secret") == std::string::npos);
    CHECK(tab->title.find("token=abc") == std::string::npos);
    CHECK(tab->url.find("user:secret") == std::string::npos);
    CHECK(tab->url.find("token=abc") == std::string::npos);
}

// TEST-TSS-011: test_service_width_resize_and_elastic_compression
void test_service_width_resize_and_elastic_compression() {
    TabStripDensityService service;
    for (std::uint64_t i = 1; i <= 4; ++i) {
        CHECK(service.addTab(TabSpec{.id = i, .title = "Tab " + std::to_string(i)}).valid);
    }

    CHECK(service.plan().effectiveTabWidth == 200);

    auto rep = service.setAvailableWidth(600);
    CHECK(rep.valid);
    CHECK(service.plan().effectiveTabWidth < 200);

    auto repBelow = service.setAvailableWidth(200);
    CHECK(!repBelow.valid);
    CHECK(repBelow.error == TabStripDensityError::WidthBelowMinimum);
}

// TEST-TSS-012: test_service_horizontal_overflow
void test_service_horizontal_overflow() {
    TabStripDensityService service;
    CHECK(service.setAvailableWidth(500).valid);
    for (std::uint64_t i = 1; i <= 15; ++i) {
        CHECK(service.addTab(TabSpec{.id = i, .title = "Tab " + std::to_string(i)}).valid);
    }

    CHECK(service.isOverflowing());
    CHECK(service.plan().isOverflowing);
    CHECK(service.plan().scrollRightButtonRect.width > 0);
    CHECK(service.plan().scrollLeftButtonRect.width > 0);
}

// TEST-TSS-013: test_service_hit_test_and_tab_at_point
void test_service_hit_test_and_tab_at_point() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 42, .title = "Hit Test Tab", .isSelected = true}).valid);
    const auto plan = service.plan();
    CHECK(!plan.tabs.empty());
    const auto& tabLayout = plan.tabs[0];

    const auto favZone = service.hitTest(tabLayout.faviconRect.x + 2, tabLayout.faviconRect.y + 2);
    CHECK(favZone == TabStripHitTestZone::TabFavicon);

    const auto bodyZone = service.hitTest(tabLayout.rect.x + 2, tabLayout.rect.y + 2);
    CHECK(bodyZone == TabStripHitTestZone::TabBody || bodyZone == TabStripHitTestZone::TabFavicon);

    const auto hitTabId = service.tabAtPoint(tabLayout.rect.x + 10, tabLayout.rect.y + 10);
    CHECK(hitTabId.has_value());
    CHECK(*hitTabId == 42);

    CHECK(service.hitTest(-5, -5) == TabStripHitTestZone::Nowhere);
    CHECK(!service.tabAtPoint(-5, -5).has_value());
}

// TEST-TSS-014: test_service_cancellation_token
void test_service_cancellation_token() {
    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .title = "Tab"}).valid);
    CancellationToken cancel;
    cancel.requestCancellation();

    auto report = service.computeLayout(&cancel);
    CHECK(!report.valid);
    CHECK(report.message.find("cancelado") != std::string::npos);
}

// TEST-TSS-015: test_service_persistence_roundtrip
void test_service_persistence_roundtrip() {
    const auto tempPath = createTempFilePath("tab_strip_test");
    TabStripDensityService service;
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    CHECK(service
              .addTab(TabSpec{.id = 101,
                              .title = "Persisted Tab 1",
                              .pinState = TabPinState::Pinned,
                              .isSelected = true})
              .valid);
    CHECK(service
              .addTab(TabSpec{.id = 102,
                              .title = "Persisted Tab 2",
                              .pinState = TabPinState::Unpinned,
                              .mediaState = TabMediaState::AudioPlaying})
              .valid);

    CHECK(service.saveStateToFile(tempPath));
    CHECK(std::filesystem::exists(tempPath));

    TabStripDensityService loadedService;
    CHECK(loadedService.loadStateFromFile(tempPath));
    CHECK(loadedService.config().density == LayoutDensity::Senior);
    CHECK(loadedService.tabCount() == 2);
    CHECK(loadedService.tab(101)->pinState == TabPinState::Pinned);
    CHECK(loadedService.tab(102)->mediaState == TabMediaState::AudioPlaying);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-TSS-016: test_service_corruption_recovery
void test_service_corruption_recovery() {
    const auto tempPath = createTempFilePath("tab_strip_corrupt");
    {
        std::ofstream out(tempPath);
        out << "{ corrupted json not closed ...";
    }

    TabStripDensityService service;
    CHECK(!service.loadStateFromFile(tempPath));
    CHECK(service.config().density == LayoutDensity::Default);

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

// TEST-TSS-017: test_service_incognito_isolation
void test_service_incognito_isolation() {
    const auto tempPath = createTempFilePath("tab_strip_incognito");
    TabStripDensityService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());

    CHECK(service.addTab(TabSpec{.id = 1, .title = "Secret Tab"}).valid);
    CHECK(!service.saveStateToFile(tempPath));
    CHECK(!std::filesystem::exists(tempPath));
    CHECK(!service.loadStateFromFile(tempPath));
}

// TEST-TSS-018: test_service_public_copy_audit
void test_service_public_copy_audit() {
    TabStripDensityService service;
    CHECK(service.setDensity(LayoutDensity::Senior).valid);
    const auto summary =
        TabStripDensityService::formatTabStripSummary(service.plan(), service.computeLayout());

    CHECK(summary.find("Interfaz más fácil de usar") != std::string::npos);
    CHECK(summary.find("Senior Mode") == std::string::npos);
    CHECK(summary.find("Modo Senior") == std::string::npos);
    CHECK(summary.find("Prompt Builder") == std::string::npos);
    CHECK(summary.find("OpenSpec") == std::string::npos);
    CHECK(summary.find("DAG") == std::string::npos);
}

// TEST-TSS-019: Concurrencia masiva con 16 hilos compitiendo (>= 200 iteraciones por hilo) (TASK-008)
void testMultiThreadedHighStressConcurrency16Threads() {
    TabStripDensityService service;
    std::atomic<bool> startFlag{false};
    std::vector<std::thread> threads;
    threads.reserve(16);

    // 4 hilos mutando pestañas
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&service, &startFlag, t]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < 200; ++i) {
                const uint64_t tabId = static_cast<uint64_t>(1000 + t * 500 + i);
                (void)service.addTab(
                    TabSpec{.id = tabId, .title = "ThreadTab " + std::to_string(tabId)});
                if (i % 2 == 1) {
                    (void)service.removeTab(tabId);
                }
            }
        });
    }

    // 4 hilos redimensionando ventana
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&service, &startFlag, t]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            const int widths[] = {500, 800, 1024, 1280, 1600, 1920};
            for (int i = 0; i < 200; ++i) {
                (void)service.setAvailableWidth(widths[(t + i) % 6]);
            }
        });
    }

    // 4 hilos conmutando densidades
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&service, &startFlag, t]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            const LayoutDensity densities[] = {LayoutDensity::Default, LayoutDensity::Senior,
                                               LayoutDensity::Compact};
            for (int i = 0; i < 200; ++i) {
                (void)service.setDensity(densities[(t + i) % 3]);
            }
        });
    }

    // 4 hilos ejecutando consultas analíticas de plan y hit-test
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&service, &startFlag]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < 200; ++i) {
                const auto p = service.plan();
                (void)p;
                (void)service.hitTest(100, 18);
                (void)service.tabCount();
                (void)service.isOverflowing();
            }
        });
    }

    startFlag.store(true, std::memory_order_release);
    for (auto& th : threads) {
        th.join();
    }

    const auto finalPlan = service.plan();
    CHECK(finalPlan.availableWidth >= TabStripDensityConfiguration::kCompactMinStripWidth);
    CHECK(service.tabCount() > 0);
    CHECK(finalPlan.tabs.size() == service.tabCount());
    CHECK(finalPlan.height >= 28);
    CHECK(service.computeLayout().valid);

    CHECK(service.restoreDefault());
    CHECK(TabStripDensityConfiguration::verifyInvariants(service.plan()));
}

// TEST-TSS-020: Cancelación cooperativa asíncrona mediante CancellationToken (TASK-009)
void testCooperativeCancellationStress() {
    TabStripDensityService service;
    for (uint64_t i = 1; i <= 20; ++i) {
        CHECK(service.addTab(TabSpec{.id = i, .title = "Tab " + std::to_string(i)}).valid);
    }

    // 1. Cancelación pre-activada
    CancellationToken preCancelled;
    preCancelled.requestCancellation();
    const auto report = service.computeLayout(&preCancelled);
    CHECK(!report.valid);
    CHECK(report.message.find("cancelado") != std::string::npos);

    // 2. Cancelación en persistencia
    const auto tempPath = createTempFilePath("cancel_test");
    CHECK(!service.saveStateToFile(tempPath, &preCancelled));
    CHECK(!std::filesystem::exists(tempPath));

    // 3. Cancelación asíncrona concurrente desde otro hilo
    CancellationToken asyncCancel;
    std::thread cancelThread([&asyncCancel]() {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        asyncCancel.requestCancellation();
    });

    for (int i = 0; i < 50; ++i) {
        (void)service.computeLayout(&asyncCancel);
    }
    cancelThread.join();

    CHECK(service.tabCount() == 20);
    CHECK(service.plan().tabs.size() == 20);
}

// TEST-TSS-021: Fuzzing de archivos de persistencia corruptos y recuperación (TASK-010)
void testCorruptedPersistenceFuzzingAndRecovery() {
    const std::vector<std::string> corruptedContents = {
        "",
        "{ \"density\": \"senior\", \"available_wid",
        "{ invalid json syntax: 123 }",
        "\"a string instead of object\"",
        "{ \"density\": 9999, \"tabs\": false }",
        "{ \"available_width\": -500, \"tabs\": [] }",
        "{ \"tabs\": [ { \"id\": \"not_a_number\" } ] }"};

    for (std::size_t idx = 0; idx < corruptedContents.size(); ++idx) {
        const auto tempPath = createTempFilePath("fuzz_" + std::to_string(idx));
        {
            std::ofstream out(tempPath);
            out << corruptedContents[idx];
        }

        TabStripDensityService service;
        CHECK(service.addTab(TabSpec{.id = 1, .title = "Pre-load tab"}).valid);
        CHECK(!service.loadStateFromFile(tempPath));
        CHECK(service.config().density == LayoutDensity::Default);
        CHECK(service.tabCount() == 0);
        CHECK(service.availableWidth() == 1000);

        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
    }

    TabStripDensityService missingService;
    CHECK(!missingService.loadStateFromFile("/path/does/not/exist_12345.json"));
    CHECK(missingService.config().density == LayoutDensity::Default);
}

// TEST-TSS-022: Manejo de fallos de E/S de disco y permisos denegados (TASK-011)
void testDiskIoFailureHandling() {
    const auto tempDir = std::filesystem::temp_directory_path() /
                         ("vigilio_ro_dir_" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    std::filesystem::create_directories(tempDir, ec);

    std::filesystem::permissions(tempDir,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::replace, ec);

    TabStripDensityService service;
    CHECK(service.addTab(TabSpec{.id = 1, .title = "Tab IO Fail"}).valid);

    const auto targetFile = tempDir / "state.json";
    const bool saved = service.saveStateToFile(targetFile);
    CHECK(!saved);

    std::filesystem::permissions(tempDir, std::filesystem::perms::all,
                                 std::filesystem::perm_options::replace, ec);
    std::filesystem::remove_all(tempDir, ec);

    CHECK(!service.saveStateToFile(""));
}

// TEST-TSS-023: Aislamiento estricto de privacidad en modo incógnito (TASK-012)
void testIncognitoStrictPrivacyIsolation() {
    TabStripDensityService service;
    service.setIncognitoMode(true);
    CHECK(service.isIncognitoMode());

    CHECK(service.addTab(TabSpec{.id = 1, .title = "Pestaña Secreta"}).valid);
    const auto secretPath = createTempFilePath("incognito_leak_test");

    CHECK(!service.saveStateToFile(secretPath));
    CHECK(!std::filesystem::exists(secretPath));
    CHECK(!service.loadStateFromFile(secretPath));

    {
        std::ofstream out(secretPath);
        out << "{\"available_width\": 1000}";
    }
    CHECK(!service.loadStateFromFile(secretPath));
    std::error_code ec;
    std::filesystem::remove(secretPath, ec);
}

// TEST-TSS-024: Mutaciones masivas de pestañas en ráfaga (50 adiciones y eliminaciones) (TASK-013)
void testContinuousTabMutationsBurst() {
    TabStripDensityService service;

    for (uint64_t i = 1; i <= 50; ++i) {
        CHECK(service
                  .addTab(TabSpec{
                      .id = i, .title = "Burst Tab " + std::to_string(i), .isSelected = (i == 1)})
                  .valid);
        CHECK(service.tabCount() == i);
    }

    CHECK(service.selectedTabId() == 1);

    for (uint64_t i = 1; i <= 25; ++i) {
        CHECK(service.removeTab(i).valid);
        CHECK(service.tabCount() == 50 - i);
    }

    CHECK(service.selectedTabId().has_value());
    CHECK(service.tabCount() == 25);

    for (uint64_t i = 26; i <= 50; ++i) {
        CHECK(service.removeTab(i).valid);
    }
    CHECK(service.tabCount() == 0);
    CHECK(!service.selectedTabId().has_value());
}

} // namespace

int main() {
    try {
        test_service_initialization();
        test_service_density_transitions();
        test_service_custom_initialization();
        test_service_tab_mutations();
        test_service_tab_remove_adjacent_selection();
        test_service_tab_pinning_precedence();
        test_service_tab_reordering();
        test_service_media_state_and_audio_toggle();
        test_service_memory_saver_discard_state();
        test_service_title_and_url_sanitization();
        test_service_width_resize_and_elastic_compression();
        test_service_horizontal_overflow();
        test_service_hit_test_and_tab_at_point();
        test_service_cancellation_token();
        test_service_persistence_roundtrip();
        test_service_corruption_recovery();
        test_service_incognito_isolation();
        test_service_public_copy_audit();

        testMultiThreadedHighStressConcurrency16Threads();
        testCooperativeCancellationStress();
        testCorruptedPersistenceFuzzingAndRecovery();
        testDiskIoFailureHandling();
        testIncognitoStrictPrivacyIsolation();
        testContinuousTabMutationsBurst();

        std::cout << "All 24 TabStripDensityService tests passed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TabStripDensityServiceTests FAILED: " << e.what() << "\n";
        return 1;
    }
}
