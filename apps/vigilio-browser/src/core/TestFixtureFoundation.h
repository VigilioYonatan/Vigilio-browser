#pragma once

#include "core/TestFixtureTypes.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Administrador RAII de ciclo de vida, contención y cuotas para carpetas de prueba.
class TestFixtureFoundation {
  public:
    explicit TestFixtureFoundation(TestEnvironmentConfig config = TestEnvironmentConfig{});
    ~TestFixtureFoundation();

    TestFixtureFoundation(const TestFixtureFoundation&) = delete;
    TestFixtureFoundation& operator=(const TestFixtureFoundation&) = delete;

    TestFixtureFoundation(TestFixtureFoundation&& other) noexcept;
    TestFixtureFoundation& operator=(TestFixtureFoundation&& other) noexcept;

    [[nodiscard]] const std::filesystem::path& rootPath() const noexcept {
        return m_rootPath;
    }
    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return m_rootPath;
    }
    [[nodiscard]] TestEnvironmentState state() const noexcept {
        return m_state;
    }
    [[nodiscard]] bool isValid() const noexcept {
        return m_state == TestEnvironmentState::Active && !m_rootPath.empty();
    }
    [[nodiscard]] const TestEnvironmentConfig& config() const noexcept {
        return m_config;
    }
    [[nodiscard]] FixtureError lastError() const noexcept {
        return m_lastError;
    }
    [[nodiscard]] const std::string& diagnosticMessage() const noexcept {
        return m_diagnosticMessage;
    }
    [[nodiscard]] const std::string& fixtureId() const noexcept {
        return m_fixtureId;
    }

    std::filesystem::path createSubdirectory(const std::string& relPath);
    std::filesystem::path createTestFile(const std::string& relPath, const std::string& content);
    std::filesystem::path createTestBinaryFile(const std::string& relPath,
                                               const std::vector<uint8_t>& bytes);

    bool cleanupSqliteWalShm(const std::filesystem::path& dbPath);
    [[nodiscard]] std::size_t calculateTotalSizeBytes() const;
    bool checkQuota();
    bool clean();
    [[nodiscard]] TestFixtureReport report() const;

  private:
    TestEnvironmentConfig m_config;
    std::filesystem::path m_rootPath;
    std::string m_fixtureId;
    TestEnvironmentState m_state{TestEnvironmentState::Uninitialized};
    std::atomic<std::size_t> m_createdFilesCount{0};
    FixtureError m_lastError{FixtureError::None};
    std::string m_diagnosticMessage;
};

/// @brief Guardián RAII para modificación temporal determinista de variables de entorno con reversión
/// LIFO.
class TestEnvironmentGuard {
  public:
    TestEnvironmentGuard() = default;
    ~TestEnvironmentGuard() {
        restoreAll();
    }

    TestEnvironmentGuard(const TestEnvironmentGuard&) = delete;
    TestEnvironmentGuard& operator=(const TestEnvironmentGuard&) = delete;

    TestEnvironmentGuard(TestEnvironmentGuard&& other) noexcept;
    TestEnvironmentGuard& operator=(TestEnvironmentGuard&& other) noexcept;

    bool set(const std::string& name, const std::string& value);
    bool unset(const std::string& name);
    void restoreAll();

    [[nodiscard]] std::size_t size() const noexcept {
        return m_overrides.size();
    }
    [[nodiscard]] bool empty() const noexcept {
        return m_overrides.empty();
    }

  private:
    std::vector<EnvironmentVariableOverride> m_overrides;
};

} // namespace vigilio
