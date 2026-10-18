#pragma once

#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureTypes.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa propietaria desacoplada para gestión, orquestación, resiliencia,
/// instantáneas reversibles y sembrado determinista de fixtures de prueba en C++20.
class TestFixtureService {
  public:
    TestFixtureService() = default;
    ~TestFixtureService();

    TestFixtureService(const TestFixtureService&) = delete;
    TestFixtureService& operator=(const TestFixtureService&) = delete;
    TestFixtureService(TestFixtureService&&) noexcept = default;
    TestFixtureService& operator=(TestFixtureService&&) noexcept = default;

    // Gestión y orquestación de ciclo de vida
    TestFixtureFoundation*
    createManagedFixture(const TestEnvironmentConfig& config = TestEnvironmentConfig{});
    bool releaseFixture(const std::string& fixtureId);
    FixtureOperationResult releaseAllFixtures(CancellationToken* cancel = nullptr);
    [[nodiscard]] std::size_t activeFixtureCount() const noexcept;
    [[nodiscard]] TestFixtureFoundation* findFixture(const std::string& fixtureId) const;

    // Resiliencia y recuperación operativa
    FixtureOperationResult cleanWithRetry(TestFixtureFoundation& fixture, uint32_t maxRetries = 3,
                                          uint32_t backoffMs = 50, CancellationToken* cancel = nullptr);
    OrphanScanReport scanAndPurgeOrphanFixtures(uint32_t maxAgeSeconds = 3600,
                                                CancellationToken* cancel = nullptr);

    // Instantáneas y Rollback atómico
    FixtureOperationResult createSnapshot(TestFixtureFoundation& fixture, const std::string& snapshotId,
                                          CancellationToken* cancel = nullptr);
    FixtureOperationResult rollbackToSnapshot(TestFixtureFoundation& fixture,
                                              const std::string& snapshotId,
                                              CancellationToken* cancel = nullptr);
    bool removeSnapshot(TestFixtureFoundation& fixture, const std::string& snapshotId);

    // Sembrado sintético de datos de prueba
    FixtureOperationResult
    seedBrowserProfile(TestFixtureFoundation& fixture,
                       const BrowserProfileSeedOptions& options = BrowserProfileSeedOptions{},
                       CancellationToken* cancel = nullptr);
    FixtureOperationResult seedMockDatabase(TestFixtureFoundation& fixture,
                                            const std::string& relDbPath = "history.sqlite",
                                            const std::string& schemaSql = "",
                                            CancellationToken* cancel = nullptr);

    // Formateo observable de diagnósticos y UX Copy
    [[nodiscard]] static std::string operationStatusToString(FixtureOperationStatus status);

  private:
    mutable std::mutex m_mutex;
    std::map<std::string, std::unique_ptr<TestFixtureFoundation>> m_managedFixtures;
};

} // namespace vigilio
