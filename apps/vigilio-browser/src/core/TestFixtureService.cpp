#include "core/TestFixtureService.h"
#include "core/TestFixtureValidator.h"

#include <chrono>
#include <fstream>
#include <sqlite3.h>
#include <thread>

namespace vigilio {

TestFixtureService::~TestFixtureService() {
    releaseAllFixtures();
}

std::string TestFixtureService::operationStatusToString(FixtureOperationStatus status) {
    switch (status) {
    case FixtureOperationStatus::Success:
        return "Operación completada exitosamente";
    case FixtureOperationStatus::Cancelled:
        return "La operación sobre los archivos de prueba fue cancelada";
    case FixtureOperationStatus::QuotaExceeded:
        return "El espacio en disco asignado para la prueba ha sido superado";
    case FixtureOperationStatus::PathUnsafe:
        return "Ruta de destino no segura o fuera de la carpeta temporal autorizada";
    case FixtureOperationStatus::IOError:
        return "Error de entrada o salida en el sistema de archivos";
    case FixtureOperationStatus::LockContention:
        return "El archivo de prueba está en uso por otro proceso; se reintentará";
    case FixtureOperationStatus::NotFound:
        return "Entorno o punto de control no encontrado";
    case FixtureOperationStatus::Timeout:
        return "Tiempo de espera agotado en la operación sobre los archivos";
    }
    return "Estado operativo desconocido";
}

TestFixtureFoundation* TestFixtureService::createManagedFixture(const TestEnvironmentConfig& config) {
    if (TestFixtureValidator::validateConfig(config) != FixtureError::None) {
        return nullptr;
    }

    auto fixture = std::make_unique<TestFixtureFoundation>(config);
    if (!fixture->isValid()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto* rawPtr = fixture.get();
    m_managedFixtures[fixture->fixtureId()] = std::move(fixture);
    return rawPtr;
}

bool TestFixtureService::releaseFixture(const std::string& fixtureId) {
    std::unique_ptr<TestFixtureFoundation> target;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_managedFixtures.find(fixtureId);
        if (it == m_managedFixtures.end()) {
            return false;
        }
        target = std::move(it->second);
        m_managedFixtures.erase(it);
    }
    return target->clean();
}

FixtureOperationResult TestFixtureService::releaseAllFixtures(CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    std::vector<std::unique_ptr<TestFixtureFoundation>> toRelease;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [_, fixture] : m_managedFixtures) {
            toRelease.push_back(std::move(fixture));
        }
        m_managedFixtures.clear();
    }

    for (auto& fixture : toRelease) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = FixtureOperationStatus::Cancelled;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
            break;
        }
        if (fixture) {
            fixture->clean();
            result.filesAffected++;
        }
    }

    if (result.status != FixtureOperationStatus::Cancelled) {
        result.status = FixtureOperationStatus::Success;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
    }

    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

std::size_t TestFixtureService::activeFixtureCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_managedFixtures.size();
}

TestFixtureFoundation* TestFixtureService::findFixture(const std::string& fixtureId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_managedFixtures.find(fixtureId);
    if (it != m_managedFixtures.end()) {
        return it->second.get();
    }
    return nullptr;
}

FixtureOperationResult TestFixtureService::cleanWithRetry(TestFixtureFoundation& fixture,
                                                          uint32_t maxRetries, uint32_t backoffMs,
                                                          CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    if (fixture.clean()) {
        result.status = FixtureOperationStatus::Success;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
        auto endTime = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return result;
    }

    // Bucle de reintentos con retroceso exponencial (exponential backoff)
    for (uint32_t retry = 1; retry <= maxRetries; ++retry) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = FixtureOperationStatus::Cancelled;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
            return result;
        }

        uint32_t currentDelayMs = backoffMs * (1U << (retry - 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(currentDelayMs));

        if (fixture.clean()) {
            result.status = FixtureOperationStatus::Success;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
            auto endTime = std::chrono::steady_clock::now();
            result.durationMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
            return result;
        }
    }

    result.status = FixtureOperationStatus::LockContention;
    result.remediationHint = operationStatusToString(FixtureOperationStatus::LockContention);
    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

OrphanScanReport TestFixtureService::scanAndPurgeOrphanFixtures(uint32_t maxAgeSeconds,
                                                                CancellationToken* cancel) {
    OrphanScanReport report;
    std::error_code ec;
    auto tempRoot = std::filesystem::temp_directory_path(ec);
    if (ec) {
        report.diagnosticMessage = "Error al acceder a la carpeta temporal";
        return report;
    }

    for (const auto& entry : std::filesystem::directory_iterator(tempRoot, ec)) {
        if (cancel != nullptr && cancel->isCancelled()) {
            report.diagnosticMessage = operationStatusToString(FixtureOperationStatus::Cancelled);
            return report;
        }
        if (ec) {
            break;
        }

        std::error_code entryEc;
        if (!entry.is_directory(entryEc)) {
            continue;
        }

        const auto& path = entry.path();
        std::string filename = path.filename().string();
        if (!filename.starts_with("vigilio_test_")) {
            continue;
        }

        if (!TestFixtureValidator::isSafeTestDirectory(path)) {
            continue;
        }

        report.scannedCount++;

        auto lwt = std::filesystem::last_write_time(path, entryEc);
        bool isOrphan = false;
        if (!entryEc) {
            auto now = std::filesystem::file_time_type::clock::now();
            auto ageSec = std::chrono::duration_cast<std::chrono::seconds>(now - lwt).count();
            if (maxAgeSeconds == 0 || ageSec >= static_cast<int64_t>(maxAgeSeconds)) {
                isOrphan = true;
            }
        } else {
            isOrphan = true;
        }

        if (isOrphan) {
            std::size_t dirBytes = 0;
            for (const auto& subEntry : std::filesystem::recursive_directory_iterator(path, entryEc)) {
                if (subEntry.is_regular_file(entryEc)) {
                    dirBytes += subEntry.file_size(entryEc);
                }
            }

            std::filesystem::remove_all(path, entryEc);
            if (!entryEc) {
                report.purgedCount++;
                report.bytesReclaimed += dirBytes;
            } else {
                report.failedCount++;
            }
        }
    }

    if (report.purgedCount > 0) {
        report.diagnosticMessage =
            "Se eliminaron las carpetas temporales residuales de sesiones anteriores";
    } else {
        report.diagnosticMessage = operationStatusToString(FixtureOperationStatus::Success);
    }
    return report;
}

FixtureOperationResult TestFixtureService::createSnapshot(TestFixtureFoundation& fixture,
                                                          const std::string& snapshotId,
                                                          CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    if (!fixture.isValid()) {
        result.status = FixtureOperationStatus::NotFound;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::NotFound);
        return result;
    }

    if (snapshotId.empty() || snapshotId.find("..") != std::string::npos ||
        snapshotId.find('/') != std::string::npos || snapshotId.find('\\') != std::string::npos) {
        result.status = FixtureOperationStatus::PathUnsafe;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::PathUnsafe);
        return result;
    }

    // Comprobar presupuesto de cuota acumulada
    std::size_t currentSizeBytes = fixture.calculateTotalSizeBytes();
    if (currentSizeBytes * 2 > fixture.config().maxTempDirSizeBytes) {
        result.status = FixtureOperationStatus::QuotaExceeded;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::QuotaExceeded);
        return result;
    }

    auto snapshotDir = fixture.rootPath() / "_snapshots" / snapshotId;
    std::error_code ec;
    std::filesystem::create_directories(snapshotDir, ec);
    if (ec) {
        result.status = FixtureOperationStatus::IOError;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::IOError);
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(fixture.rootPath(), ec)) {
        if (cancel != nullptr && cancel->isCancelled()) {
            std::filesystem::remove_all(snapshotDir, ec);
            result.status = FixtureOperationStatus::Cancelled;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
            return result;
        }

        if (entry.path().filename() == "_snapshots") {
            continue;
        }

        std::filesystem::copy(entry.path(), snapshotDir / entry.path().filename(),
                              std::filesystem::copy_options::recursive |
                                  std::filesystem::copy_options::overwrite_existing,
                              ec);
        if (ec) {
            std::filesystem::remove_all(snapshotDir, ec);
            result.status = FixtureOperationStatus::IOError;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::IOError);
            return result;
        }
        result.filesAffected++;
    }

    result.status = FixtureOperationStatus::Success;
    result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

FixtureOperationResult TestFixtureService::rollbackToSnapshot(TestFixtureFoundation& fixture,
                                                              const std::string& snapshotId,
                                                              CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    if (!fixture.isValid()) {
        result.status = FixtureOperationStatus::NotFound;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::NotFound);
        return result;
    }

    if (snapshotId.empty() || snapshotId.find("..") != std::string::npos ||
        snapshotId.find('/') != std::string::npos || snapshotId.find('\\') != std::string::npos) {
        result.status = FixtureOperationStatus::PathUnsafe;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::PathUnsafe);
        return result;
    }

    auto snapshotDir = fixture.rootPath() / "_snapshots" / snapshotId;
    std::error_code ec;
    if (!std::filesystem::exists(snapshotDir, ec) || !std::filesystem::is_directory(snapshotDir, ec)) {
        result.status = FixtureOperationStatus::NotFound;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::NotFound);
        return result;
    }

    // Purgar todos los archivos actuales fuera de _snapshots
    for (const auto& entry : std::filesystem::directory_iterator(fixture.rootPath(), ec)) {
        if (entry.path().filename() == "_snapshots") {
            continue;
        }
        std::filesystem::remove_all(entry.path(), ec);
    }

    // Restaurar desde snapshot
    for (const auto& entry : std::filesystem::directory_iterator(snapshotDir, ec)) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = FixtureOperationStatus::Cancelled;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
            return result;
        }

        std::filesystem::copy(entry.path(), fixture.rootPath() / entry.path().filename(),
                              std::filesystem::copy_options::recursive |
                                  std::filesystem::copy_options::overwrite_existing,
                              ec);
        if (ec) {
            result.status = FixtureOperationStatus::IOError;
            result.remediationHint = operationStatusToString(FixtureOperationStatus::IOError);
            return result;
        }
        result.filesAffected++;
    }

    result.status = FixtureOperationStatus::Success;
    result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

bool TestFixtureService::removeSnapshot(TestFixtureFoundation& fixture, const std::string& snapshotId) {
    if (!fixture.isValid() || snapshotId.empty() || snapshotId.find("..") != std::string::npos) {
        return false;
    }

    auto snapshotDir = fixture.rootPath() / "_snapshots" / snapshotId;
    std::error_code ec;
    if (!std::filesystem::exists(snapshotDir, ec)) {
        return false;
    }

    std::filesystem::remove_all(snapshotDir, ec);

    // Si la carpeta de snapshots queda vacía, sanearla también
    auto snapshotsRoot = fixture.rootPath() / "_snapshots";
    if (std::filesystem::is_empty(snapshotsRoot, ec)) {
        std::filesystem::remove(snapshotsRoot, ec);
    }
    return !ec;
}

FixtureOperationResult TestFixtureService::seedBrowserProfile(TestFixtureFoundation& fixture,
                                                              const BrowserProfileSeedOptions& options,
                                                              CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (!fixture.isValid()) {
        result.status = FixtureOperationStatus::NotFound;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::NotFound);
        return result;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    // Estructura canónica de carpetas
    std::string profileRel = "profiles/" + options.profileName;
    fixture.createSubdirectory(profileRel);
    if (options.includeCache) {
        fixture.createSubdirectory("cache");
    }
    fixture.createSubdirectory("storage/local");

    // preferences.json
    if (options.includePreferences) {
        std::string prefContent = "{\n"
                                  "  \"version\": 1,\n"
                                  "  \"profile\": {\n"
                                  "    \"name\": \"" +
                                  options.profileName +
                                  "\",\n"
                                  "    \"created_at\": 1729250000\n"
                                  "  },\n"
                                  "  \"privacy\": {\n"
                                  "    \"block_trackers\": true,\n"
                                  "    \"block_ads\": true,\n"
                                  "    \"local_first_strict\": true\n"
                                  "  },\n"
                                  "  \"ui\": {\n"
                                  "    \"easy_interface_enabled\": false,\n"
                                  "    \"theme\": \"dark\"\n"
                                  "  }\n"
                                  "}\n";
        fixture.createTestFile(profileRel + "/preferences.json", prefContent);
        result.filesAffected++;
    }

    // Base de datos de historial
    if (options.includeHistoryDb) {
        auto dbResult = seedMockDatabase(fixture, profileRel + "/history.sqlite", "", cancel);
        if (!dbResult.isSuccess()) {
            return dbResult;
        }
        result.filesAffected += dbResult.filesAffected;
    }

    result.status = FixtureOperationStatus::Success;
    result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

FixtureOperationResult TestFixtureService::seedMockDatabase(TestFixtureFoundation& fixture,
                                                            const std::string& relDbPath,
                                                            const std::string& schemaSql,
                                                            CancellationToken* cancel) {
    auto startTime = std::chrono::steady_clock::now();
    FixtureOperationResult result;

    if (!fixture.isValid()) {
        result.status = FixtureOperationStatus::NotFound;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::NotFound);
        return result;
    }

    if (relDbPath.empty() || relDbPath.find("..") != std::string::npos) {
        result.status = FixtureOperationStatus::PathUnsafe;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::PathUnsafe);
        return result;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FixtureOperationStatus::Cancelled;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::Cancelled);
        return result;
    }

    auto fullDbPath = fixture.rootPath() / relDbPath;
    std::error_code ec;
    std::filesystem::create_directories(fullDbPath.parent_path(), ec);

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(fullDbPath.string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK) {
        if (db != nullptr) {
            sqlite3_close(db);
        }
        result.status = FixtureOperationStatus::IOError;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::IOError);
        return result;
    }

    sqlite3_busy_timeout(db, 2000);
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

    if (!schemaSql.empty()) {
        sqlite3_exec(db, schemaSql.c_str(), nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db,
                     "CREATE TABLE IF NOT EXISTS test_history ("
                     "id INTEGER PRIMARY KEY, "
                     "url TEXT NOT NULL, "
                     "title TEXT NOT NULL, "
                     "visited_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);",
                     nullptr, nullptr, nullptr);
        sqlite3_exec(db,
                     "INSERT INTO test_history (url, title) VALUES "
                     "('vigilio://welcome', 'Bienvenido a Vigilio'), "
                     "('vigilio://settings', 'Configuración');",
                     nullptr, nullptr, nullptr);
    }

    sqlite3_close(db);

    if (!fixture.checkQuota()) {
        result.status = FixtureOperationStatus::QuotaExceeded;
        result.remediationHint = operationStatusToString(FixtureOperationStatus::QuotaExceeded);
        return result;
    }

    result.filesAffected++;
    result.status = FixtureOperationStatus::Success;
    result.remediationHint = operationStatusToString(FixtureOperationStatus::Success);
    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

} // namespace vigilio
