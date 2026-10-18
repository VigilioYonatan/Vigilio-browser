#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureService.h"
#include "core/TestFixtureValidator.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
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

// TEST-FIXTURE-SRV-001: Registro y ciclo de vida de fixtures gestionados
void test_fixture_service_lifecycle() {
    TestFixtureService service;
    CHECK(service.activeFixtureCount() == 0);

    auto* f1 = service.createManagedFixture();
    CHECK(f1 != nullptr);
    CHECK(f1->isValid());
    CHECK(service.activeFixtureCount() == 1);

    std::string id1 = f1->fixtureId();
    CHECK(!id1.empty());
    CHECK(service.findFixture(id1) == f1);
    CHECK(service.findFixture("non_existent_id") == nullptr);

    auto* f2 = service.createManagedFixture();
    CHECK(f2 != nullptr);
    CHECK(f2->isValid());
    CHECK(service.activeFixtureCount() == 2);

    CHECK(service.releaseFixture(id1));
    CHECK(service.activeFixtureCount() == 1);
    CHECK(service.findFixture(id1) == nullptr);
    CHECK(!service.releaseFixture(id1)); // Idempotencia ante doble liberación
}

// TEST-FIXTURE-SRV-002: Liberación individual y en lote de fixtures
void test_fixture_service_release_batch() {
    TestFixtureService service;
    auto* f1 = service.createManagedFixture();
    auto* f2 = service.createManagedFixture();
    CHECK(f1 != nullptr && f2 != nullptr);

    auto path1 = f1->rootPath();
    auto path2 = f2->rootPath();
    CHECK(std::filesystem::exists(path1));
    CHECK(std::filesystem::exists(path2));

    auto res = service.releaseAllFixtures();
    CHECK(res.isSuccess());
    CHECK(service.activeFixtureCount() == 0);
    CHECK(!std::filesystem::exists(path1));
    CHECK(!std::filesystem::exists(path2));
}

// TEST-FIXTURE-SRV-003: Cancelación cooperativa multihilo vía CancellationToken
void test_fixture_service_cancellation() {
    TestFixtureService service;
    CancellationToken token;
    token.requestCancellation();
    CHECK(token.isCancelled());

    auto* f = service.createManagedFixture();
    CHECK(f != nullptr);

    auto snapRes = service.createSnapshot(*f, "snap1", &token);
    CHECK(snapRes.status == FixtureOperationStatus::Cancelled);

    auto rollbackRes = service.rollbackToSnapshot(*f, "snap1", &token);
    CHECK(rollbackRes.status == FixtureOperationStatus::Cancelled);

    BrowserProfileSeedOptions opts;
    auto seedRes = service.seedBrowserProfile(*f, opts, &token);
    CHECK(seedRes.status == FixtureOperationStatus::Cancelled);

    auto orphanReport = service.scanAndPurgeOrphanFixtures(3600, &token);
    CHECK(orphanReport.diagnosticMessage ==
          TestFixtureService::operationStatusToString(FixtureOperationStatus::Cancelled));

    auto retryRes = service.cleanWithRetry(*f, 3, 50, &token);
    CHECK(retryRes.status == FixtureOperationStatus::Cancelled);
}

// TEST-FIXTURE-SRV-004: Detección y purga segura de fixtures huérfanos
void test_fixture_service_orphan_purge() {
    auto tempDir = std::filesystem::temp_directory_path();
    auto orphanPath = tempDir / "vigilio_test_simulated_orphan_srv_12345";
    std::error_code ec;
    std::filesystem::create_directories(orphanPath, ec);
    std::ofstream(orphanPath / "residual.txt") << "datos de prueba residuales";
    CHECK(std::filesystem::exists(orphanPath));
    CHECK(TestFixtureValidator::isSafeTestDirectory(orphanPath));

    TestFixtureService service;
    auto report = service.scanAndPurgeOrphanFixtures(0, nullptr);
    CHECK(report.purgedCount >= 1);
    CHECK(!std::filesystem::exists(orphanPath));
}

// TEST-FIXTURE-SRV-005: Reintentos con retroceso exponencial ante bloqueos transitorios
void test_fixture_service_retry_backoff() {
    TestFixtureService service;
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    auto res = service.cleanWithRetry(fixture, 2, 5, nullptr);
    CHECK(res.isSuccess());
    CHECK(!std::filesystem::exists(fixture.rootPath()));
}

// TEST-FIXTURE-SRV-006: Creación y rollback atómico de instantáneas (Snapshots)
void test_fixture_service_snapshots() {
    TestFixtureService service;
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    fixture.createTestFile("initial.txt", "estado original 123");
    CHECK(std::filesystem::exists(fixture.rootPath() / "initial.txt"));

    auto snapRes = service.createSnapshot(fixture, "snap_checkpoint", nullptr);
    CHECK(snapRes.isSuccess());
    CHECK(
        std::filesystem::exists(fixture.rootPath() / "_snapshots" / "snap_checkpoint" / "initial.txt"));

    // Mutar el fixture
    fixture.createTestFile("added_after.txt", "archivo no deseado");
    fixture.createTestFile("initial.txt", "contenido alterado!");
    CHECK(std::filesystem::exists(fixture.rootPath() / "added_after.txt"));

    // Rollback atómico
    auto rbRes = service.rollbackToSnapshot(fixture, "snap_checkpoint", nullptr);
    CHECK(rbRes.isSuccess());
    CHECK(!std::filesystem::exists(fixture.rootPath() / "added_after.txt"));

    std::ifstream in(fixture.rootPath() / "initial.txt");
    std::string restored;
    std::getline(in, restored);
    CHECK(restored == "estado original 123");

    // Rollback a snapshot inexistente
    auto rbMissing = service.rollbackToSnapshot(fixture, "non_existent_snapshot", nullptr);
    CHECK(rbMissing.status == FixtureOperationStatus::NotFound);

    // Eliminación de snapshot
    CHECK(service.removeSnapshot(fixture, "snap_checkpoint"));
    CHECK(!std::filesystem::exists(fixture.rootPath() / "_snapshots" / "snap_checkpoint"));
}

// TEST-FIXTURE-SRV-007: Sembrado sintético de perfil de navegador
void test_fixture_service_seed_profile() {
    TestFixtureService service;
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    BrowserProfileSeedOptions opts;
    opts.profileName = "perfil_pruebas";
    opts.includeHistoryDb = true;
    opts.includePreferences = true;
    opts.includeCache = true;

    auto res = service.seedBrowserProfile(fixture, opts, nullptr);
    CHECK(res.isSuccess());
    CHECK(std::filesystem::exists(fixture.rootPath() / "profiles" / "perfil_pruebas" /
                                  "preferences.json"));
    CHECK(
        std::filesystem::exists(fixture.rootPath() / "profiles" / "perfil_pruebas" / "history.sqlite"));
    CHECK(std::filesystem::exists(fixture.rootPath() / "cache"));
    CHECK(std::filesystem::exists(fixture.rootPath() / "storage" / "local"));

    std::ifstream prefFile(fixture.rootPath() / "profiles" / "perfil_pruebas" / "preferences.json");
    std::string prefContent((std::istreambuf_iterator<char>(prefFile)),
                            std::istreambuf_iterator<char>());
    CHECK(prefContent.find("\"name\": \"perfil_pruebas\"") != std::string::npos);
    CHECK(prefContent.find("\"easy_interface_enabled\": false") != std::string::npos);
    CHECK(prefContent.find("Senior Mode") == std::string::npos);
    CHECK(prefContent.find("Modo Senior") == std::string::npos);
}

// TEST-FIXTURE-SRV-008: Sembrado y verificación de base de datos SQLite sintética
void test_fixture_service_seed_sqlite() {
    TestFixtureService service;
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    auto res = service.seedMockDatabase(fixture, "historial.sqlite", "", nullptr);
    CHECK(res.isSuccess());

    auto dbPath = fixture.rootPath() / "historial.sqlite";
    CHECK(std::filesystem::exists(dbPath));

    sqlite3* db = nullptr;
    CHECK(sqlite3_open_v2(dbPath.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK);

    sqlite3_stmt* stmt = nullptr;
    CHECK(sqlite3_prepare_v2(db, "SELECT count(*) FROM test_history;", -1, &stmt, nullptr) == SQLITE_OK);
    CHECK(sqlite3_step(stmt) == SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    CHECK(count == 2);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// TEST-FIXTURE-SRV-009: Cumplimiento estricto de cuota de almacenamiento
void test_fixture_service_quota_enforcement() {
    TestFixtureService service;
    TestEnvironmentConfig cfg;
    cfg.maxTempDirSizeBytes = 8192; // 8 KB presupuesto mínimo
    TestFixtureFoundation fixture(cfg);
    CHECK(fixture.isValid());

    fixture.createTestFile("payload.dat", std::string(5000, 'Z'));
    auto snapRes = service.createSnapshot(fixture, "snap_quota", nullptr);
    CHECK(snapRes.status == FixtureOperationStatus::QuotaExceeded);
}

// TEST-FIXTURE-SRV-010: Concurrencia multihilo en el servicio de fixtures
void test_fixture_service_concurrency() {
    TestFixtureService service;
    std::vector<std::thread> workers;

    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&service]() {
            for (int i = 0; i < 4; ++i) {
                auto* f = service.createManagedFixture();
                if (f != nullptr) {
                    std::string id = f->fixtureId();
                    f->createTestFile("thread_data.txt", "contenido concurrente");
                    service.releaseFixture(id);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    CHECK(service.activeFixtureCount() == 0);
}

// TEST-FIXTURE-SRV-011: Rechazo riguroso de rutas no seguras o Directory Traversal
void test_fixture_service_unsafe_path_rejection() {
    TestFixtureService service;
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    auto snapTraversal = service.createSnapshot(fixture, "../escape", nullptr);
    CHECK(snapTraversal.status == FixtureOperationStatus::PathUnsafe);

    auto snapSlash = service.createSnapshot(fixture, "sub/dir", nullptr);
    CHECK(snapSlash.status == FixtureOperationStatus::PathUnsafe);

    auto snapEmpty = service.createSnapshot(fixture, "", nullptr);
    CHECK(snapEmpty.status == FixtureOperationStatus::PathUnsafe);

    auto dbTraversal = service.seedMockDatabase(fixture, "../malicious.db", "", nullptr);
    CHECK(dbTraversal.status == FixtureOperationStatus::PathUnsafe);
}

// TEST-FIXTURE-SRV-012: Auditoría de textos públicos y UX Writing en español
void test_fixture_service_public_copy_audit() {
    const std::vector<std::string> prohibited = {
        "Senior Mode", "Modo Senior", "senior mode", "Prompt Builder", "OpenSpec", "DAG", "toolchain"};

    const std::vector<FixtureOperationStatus> allStatuses = {
        FixtureOperationStatus::Success,       FixtureOperationStatus::Cancelled,
        FixtureOperationStatus::QuotaExceeded, FixtureOperationStatus::PathUnsafe,
        FixtureOperationStatus::IOError,       FixtureOperationStatus::LockContention,
        FixtureOperationStatus::NotFound,      FixtureOperationStatus::Timeout};

    for (auto status : allStatuses) {
        std::string copy = TestFixtureService::operationStatusToString(status);
        CHECK(!copy.empty());
        for (const auto& word : prohibited) {
            CHECK(copy.find(word) == std::string::npos);
        }
    }
}

} // namespace

int main() {
    try {
        test_fixture_service_lifecycle();
        test_fixture_service_release_batch();
        test_fixture_service_cancellation();
        test_fixture_service_orphan_purge();
        test_fixture_service_retry_backoff();
        test_fixture_service_snapshots();
        test_fixture_service_seed_profile();
        test_fixture_service_seed_sqlite();
        test_fixture_service_quota_enforcement();
        test_fixture_service_concurrency();
        test_fixture_service_unsafe_path_rejection();
        test_fixture_service_public_copy_audit();

        std::cout << "TestFixtureServiceTests: ALL 12 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "TestFixtureServiceTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
