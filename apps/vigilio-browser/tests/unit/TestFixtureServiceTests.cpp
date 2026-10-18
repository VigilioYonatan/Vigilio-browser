#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureService.h"
#include "core/TestFixtureValidator.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

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

// TEST-FIXTURE-SRV-013: Ciclo de vida integral y persistencia SQLite sintética E2E
void test_fixture_service_full_lifecycle_e2e() {
    TestFixtureService service;
    auto* fixture = service.createManagedFixture();
    CHECK(fixture != nullptr);
    CHECK(fixture->isValid());
    const auto root = fixture->rootPath();
    const std::string fId = fixture->fixtureId();

    // 1. Sembrado de perfil sintético
    BrowserProfileSeedOptions opts;
    opts.profileName = "e2e_user";
    opts.includePreferences = true;
    opts.includeHistoryDb = true;
    opts.includeCache = true;
    auto seedRes = service.seedBrowserProfile(*fixture, opts, nullptr);
    CHECK(seedRes.isSuccess());
    CHECK(std::filesystem::exists(root / "profiles" / "e2e_user" / "preferences.json"));

    // 2. Sembrado de base de datos SQLite sintética con datos simulados
    auto dbRes = service.seedMockDatabase(*fixture, "e2e_database.sqlite", "", nullptr);
    CHECK(dbRes.isSuccess());
    const auto dbPath = root / "e2e_database.sqlite";
    CHECK(std::filesystem::exists(dbPath));

    // 3. Creación de snapshot atómico
    auto snapRes = service.createSnapshot(*fixture, "checkpoint_full_e2e", nullptr);
    CHECK(snapRes.isSuccess());
    CHECK(std::filesystem::exists(root / "_snapshots" / "checkpoint_full_e2e"));

    // 4. Mutación destructiva
    fixture->createTestFile("profiles/e2e_user/preferences.json", "CORRUPTED_PREFERENCES_OVERWRITTEN");
    fixture->createTestFile("rogue_file.tmp", "DATOS_BASURA");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);
    CHECK(!std::filesystem::exists(dbPath));

    // 5. Rollback atómico al snapshot
    auto rbRes = service.rollbackToSnapshot(*fixture, "checkpoint_full_e2e", nullptr);
    CHECK(rbRes.isSuccess());

    // 6. Verificación de consistencia
    CHECK(!std::filesystem::exists(root / "rogue_file.tmp"));
    CHECK(std::filesystem::exists(dbPath));

    std::ifstream prefIn(root / "profiles" / "e2e_user" / "preferences.json");
    std::string prefContent((std::istreambuf_iterator<char>(prefIn)), std::istreambuf_iterator<char>());
    CHECK(prefContent.find("\"name\": \"e2e_user\"") != std::string::npos);
    CHECK(prefContent.find("CORRUPTED") == std::string::npos);

    // Verificar integridad de la base SQLite restaurada
    sqlite3* db = nullptr;
    CHECK(sqlite3_open_v2(dbPath.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK);
    sqlite3_stmt* stmt = nullptr;
    CHECK(sqlite3_prepare_v2(db, "SELECT count(*) FROM test_history;", -1, &stmt, nullptr) == SQLITE_OK);
    CHECK(sqlite3_step(stmt) == SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    CHECK(count == 2);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // 7. Limpieza completa sin residuos en disco
    CHECK(service.releaseFixture(fId));
    CHECK(service.activeFixtureCount() == 0);
    CHECK(!std::filesystem::exists(root));
}

// TEST-FIXTURE-SRV-014: Validación de entradas inválidas en snapshots y perfiles
void test_fixture_service_invalid_snapshot_and_profile_inputs() {
    TestFixtureService service;
    auto* fixture = service.createManagedFixture();
    CHECK(fixture != nullptr);
    CHECK(fixture->isValid());

    // 1. Rollback a snapshot inexistente debe retornar NotFound
    auto rbNotFound = service.rollbackToSnapshot(*fixture, "snapshot_que_no_existe_xyz", nullptr);
    CHECK(rbNotFound.status == FixtureOperationStatus::NotFound);

    // 2. Creación de snapshot con nombres vacíos o caracteres de escape debe retornar PathUnsafe
    CHECK(service.createSnapshot(*fixture, "", nullptr).status == FixtureOperationStatus::PathUnsafe);
    CHECK(service.createSnapshot(*fixture, "../escape", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);
    CHECK(service.createSnapshot(*fixture, "a/../../b", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);
    CHECK(service.createSnapshot(*fixture, "sub/dir", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);
    CHECK(service.createSnapshot(*fixture, "sub\\dir", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);

    // 3. Rollback con nombres no seguros
    CHECK(service.rollbackToSnapshot(*fixture, "", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);
    CHECK(service.rollbackToSnapshot(*fixture, "../escape", nullptr).status ==
          FixtureOperationStatus::PathUnsafe);

    // 4. Rollback con token cancelado
    CancellationToken cancelToken;
    cancelToken.requestCancellation();
    CHECK(cancelToken.isCancelled());
    auto rbCancel = service.rollbackToSnapshot(*fixture, "any_snapshot", &cancelToken);
    CHECK(rbCancel.status == FixtureOperationStatus::Cancelled);

    // 5. Sembrado de base de datos con ruta insegura
    auto dbUnsafe = service.seedMockDatabase(*fixture, "../escaped_db.sqlite", "", nullptr);
    CHECK(dbUnsafe.status == FixtureOperationStatus::PathUnsafe);

    CHECK(service.releaseFixture(fixture->fixtureId()));
}

// TEST-FIXTURE-SRV-015: Concurrencia masiva multihilo (16 hilos) e inmunidad a colisiones de IDs
void test_fixture_service_massive_concurrency_stress() {
    TestFixtureService service;
    const int threadCount = 16;
    const int iterationsPerThread = 4;
    std::vector<std::thread> threads;
    std::vector<std::string> allCreatedIds;
    std::mutex idsMutex;

    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&service, &allCreatedIds, &idsMutex, t, iterationsPerThread]() {
            for (int i = 0; i < iterationsPerThread; ++i) {
                auto* f = service.createManagedFixture();
                if (f != nullptr && f->isValid()) {
                    std::string id = f->fixtureId();
                    {
                        std::lock_guard<std::mutex> lock(idsMutex);
                        allCreatedIds.push_back(id);
                    }

                    f->createTestFile("data_thread_" + std::to_string(t) + ".txt",
                                      "payload_iter_" + std::to_string(i));
                    auto snapRes = service.createSnapshot(*f, "snap_thread", nullptr);
                    CHECK(snapRes.isSuccess());

                    f->createTestFile("after_snap.txt", "temp");
                    auto rbRes = service.rollbackToSnapshot(*f, "snap_thread", nullptr);
                    CHECK(rbRes.isSuccess());

                    bool released = service.releaseFixture(id);
                    CHECK(released);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Comprobar unicidad absoluta de fixtureId entre todos los hilos
    CHECK(allCreatedIds.size() == static_cast<std::size_t>(threadCount * iterationsPerThread));
    std::set<std::string> uniqueIds(allCreatedIds.begin(), allCreatedIds.end());
    CHECK(uniqueIds.size() == allCreatedIds.size());

    // Convergencia del conteo de fixtures activos a cero
    CHECK(service.activeFixtureCount() == 0);
}

// TEST-FIXTURE-SRV-016: Estrés de cancelación cooperativa multihilo en operaciones del servicio
void test_fixture_service_cancellation_token_stress() {
    TestFixtureService service;
    auto* fixture = service.createManagedFixture();
    CHECK(fixture != nullptr);

    // 1. Token pre-cancelado
    CancellationToken token;
    token.requestCancellation();
    CHECK(token.isCancelled());
    // Idempotencia ante múltiples llamadas consecutivas
    token.requestCancellation();
    token.requestCancellation();
    CHECK(token.isCancelled());

    // seedBrowserProfile debe retornar Cancelled
    BrowserProfileSeedOptions opts;
    opts.profileName = "cancel_prof";
    auto seedRes = service.seedBrowserProfile(*fixture, opts, &token);
    CHECK(seedRes.status == FixtureOperationStatus::Cancelled);

    // cleanWithRetry debe retornar Cancelled
    auto cleanRes = service.cleanWithRetry(*fixture, 3, 50, &token);
    CHECK(cleanRes.status == FixtureOperationStatus::Cancelled);

    // scanAndPurgeOrphanFixtures debe retornar Cancelled en el mensaje diagnóstico
    auto orphanRep = service.scanAndPurgeOrphanFixtures(3600, &token);
    CHECK(orphanRep.diagnosticMessage ==
          TestFixtureService::operationStatusToString(FixtureOperationStatus::Cancelled));

    // createSnapshot y rollbackToSnapshot deben retornar Cancelled
    auto snapRes = service.createSnapshot(*fixture, "snap_cancel", &token);
    CHECK(snapRes.status == FixtureOperationStatus::Cancelled);
    auto rbRes = service.rollbackToSnapshot(*fixture, "snap_cancel", &token);
    CHECK(rbRes.status == FixtureOperationStatus::Cancelled);

    // seedMockDatabase debe retornar Cancelled
    auto dbRes = service.seedMockDatabase(*fixture, "cancel.db", "", &token);
    CHECK(dbRes.status == FixtureOperationStatus::Cancelled);

    // releaseAllFixtures debe responder a la cancelación
    auto relRes = service.releaseAllFixtures(&token);
    CHECK(relRes.status == FixtureOperationStatus::Cancelled);

    // Limpieza posterior con token válido
    CHECK(service.releaseFixture(fixture->fixtureId()));
    CHECK(service.activeFixtureCount() == 0);
}

// TEST-FIXTURE-SRV-017: Recuperación ante bloqueos transitorios e inyección de fallos con backoff
void test_fixture_service_transient_lock_and_retry_recovery() {
    TestFixtureService service;

    // Caso 1: Limpieza inmediata sin bloqueo
    {
        TestFixtureFoundation cleanFixture;
        CHECK(cleanFixture.isValid());
        cleanFixture.createTestFile("sample.txt", "data");
        auto res = service.cleanWithRetry(cleanFixture, 3, 5, nullptr);
        CHECK(res.isSuccess());
        CHECK(res.status == FixtureOperationStatus::Success);
        CHECK(!std::filesystem::exists(cleanFixture.rootPath()));
    }

    // Caso 2: Recuperación ante bloqueo transitorio que se libera en segundo plano
    {
        TestFixtureFoundation lockedFixture;
        CHECK(lockedFixture.isValid());
        auto sub = lockedFixture.createSubdirectory("transient_locked_sub");
        CHECK(!sub.empty());
        lockedFixture.createTestFile("transient_locked_sub/locked.txt", "locked_content");

#ifndef _WIN32
        const bool isRoot = (geteuid() == 0);
#else
        const bool isRoot = false;
#endif

        if (!isRoot) {
            std::error_code ec;
            std::filesystem::permissions(sub, std::filesystem::perms::owner_read,
                                         std::filesystem::perm_options::replace, ec);

            // Hilo auxiliar que libera el bloqueo tras un pequeño retraso (30 ms)
            std::thread unlocker([sub]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                std::error_code thEc;
                std::filesystem::permissions(sub, std::filesystem::perms::owner_all,
                                             std::filesystem::perm_options::replace, thEc);
            });

            // cleanWithRetry con 4 reintentos y 20 ms de backoff base (20ms, 40ms, 80ms)
            auto res = service.cleanWithRetry(lockedFixture, 4, 20, nullptr);
            unlocker.join();

            CHECK(res.isSuccess());
            CHECK(!std::filesystem::exists(lockedFixture.rootPath()));
        }
    }

    // Caso 3: Diagnóstico limpio de LockContention ante bloqueo persistente
    {
        TestFixtureFoundation permLockFixture;
        CHECK(permLockFixture.isValid());
        auto sub = permLockFixture.createSubdirectory("perm_locked_sub");
        CHECK(!sub.empty());
        permLockFixture.createTestFile("perm_locked_sub/file.txt", "content");

#ifndef _WIN32
        const bool isRoot = (geteuid() == 0);
#else
        const bool isRoot = false;
#endif

        if (!isRoot) {
            std::error_code ec;
            std::filesystem::permissions(sub, std::filesystem::perms::owner_read,
                                         std::filesystem::perm_options::replace, ec);

            // Reintentos mínimos sin desbloqueo
            auto res = service.cleanWithRetry(permLockFixture, 2, 5, nullptr);
            CHECK(res.status == FixtureOperationStatus::LockContention);
            CHECK(res.remediationHint ==
                  TestFixtureService::operationStatusToString(FixtureOperationStatus::LockContention));

            // Restaurar permisos para evitar residuales
            std::filesystem::permissions(sub, std::filesystem::perms::owner_all,
                                         std::filesystem::perm_options::replace, ec);
            std::filesystem::remove_all(permLockFixture.rootPath(), ec);
        }
    }
}

// TEST-FIXTURE-SRV-018: Escáner de huérfanos con casos de frontera y límites temporales
void test_fixture_service_orphan_purge_boundary_conditions() {
    TestFixtureService service;
    auto tempDir = std::filesystem::temp_directory_path();
    std::error_code ec;

    // 1. Crear carpeta que NO pertenece a Vigilio (prefijo ajeno)
    auto foreignDir = tempDir / "other_application_directory_not_vigilio_9999";
    std::filesystem::create_directories(foreignDir, ec);
    std::ofstream(foreignDir / "foreign.txt") << "datos de otra aplicacion";
    CHECK(std::filesystem::exists(foreignDir));

    // 2. Crear carpeta de fixture de Vigilio recién creada
    auto recentFixtureDir = tempDir / "vigilio_test_recent_boundary_fixture_8888";
    std::filesystem::create_directories(recentFixtureDir, ec);
    std::ofstream(recentFixtureDir / "recent.txt") << "fixture reciente";
    CHECK(std::filesystem::exists(recentFixtureDir));

    // 3. Purga con umbral positivo (1 hora): el fixture reciente NO debe ser eliminado
    auto reportThreshold = service.scanAndPurgeOrphanFixtures(3600, nullptr);
    CHECK(std::filesystem::exists(recentFixtureDir));
    CHECK(std::filesystem::exists(foreignDir));

    // 4. Purga con umbral 0: el fixture de Vigilio debe ser purgado, pero la carpeta ajena JAMÁS
    auto reportZero = service.scanAndPurgeOrphanFixtures(0, nullptr);
    CHECK(!std::filesystem::exists(recentFixtureDir));
    CHECK(std::filesystem::exists(foreignDir)); // Preservación incondicional de directorios ajenos

    // 5. Limpieza segura de la carpeta ajena de prueba
    std::filesystem::remove_all(foreignDir, ec);
    CHECK(!std::filesystem::exists(foreignDir));
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
        test_fixture_service_full_lifecycle_e2e();
        test_fixture_service_invalid_snapshot_and_profile_inputs();
        test_fixture_service_massive_concurrency_stress();
        test_fixture_service_cancellation_token_stress();
        test_fixture_service_transient_lock_and_retry_recovery();
        test_fixture_service_orphan_purge_boundary_conditions();

        std::cout << "TestFixtureServiceTests: ALL 18 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "TestFixtureServiceTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
