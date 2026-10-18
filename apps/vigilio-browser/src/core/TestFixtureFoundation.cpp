#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureValidator.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace vigilio {

namespace {

std::atomic<uint64_t> g_fixtureCounter{0};

uint32_t getCurrentProcessId() noexcept {
#ifdef _WIN32
    return static_cast<uint32_t>(_getpid());
#else
    return static_cast<uint32_t>(getpid());
#endif
}

} // namespace

TestFixtureFoundation::TestFixtureFoundation(TestEnvironmentConfig config)
    : m_config(std::move(config)) {
    const auto configError = TestFixtureValidator::validateConfig(m_config);
    if (configError != FixtureError::None) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = configError;
        m_diagnosticMessage = TestFixtureValidator::fixtureErrorToString(configError);
        return;
    }

    std::string prefix = m_config.prefix;
    if (prefix == "vigilio_test") {
        prefix = "vigilio_test";
    } else if (!prefix.starts_with("vigilio_test_")) {
        prefix = "vigilio_test_" + prefix;
    }

    const auto nowNanos =
        static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto seq = g_fixtureCounter.fetch_add(1, std::memory_order_relaxed);
    const auto pid = getCurrentProcessId();

    m_fixtureId =
        prefix + "_" + std::to_string(pid) + "_" + std::to_string(nowNanos) + "_" + std::to_string(seq);
    m_rootPath = std::filesystem::temp_directory_path() / m_fixtureId;

    std::error_code ec;
    if (!std::filesystem::create_directories(m_rootPath, ec) && ec) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return;
    }

    std::filesystem::permissions(m_rootPath, std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::replace, ec);

    m_state = TestEnvironmentState::Active;
    m_diagnosticMessage = TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Active);
}

TestFixtureFoundation::~TestFixtureFoundation() {
    if (m_rootPath.empty() || m_state == TestEnvironmentState::TornDown) {
        return;
    }
    if (m_config.cleanPolicy == FixtureCleanPolicy::AlwaysClean) {
        clean();
    } else if (m_config.cleanPolicy == FixtureCleanPolicy::RetainOnFailure) {
        if (m_state != TestEnvironmentState::Corrupted) {
            clean();
        }
    }
}

TestFixtureFoundation::TestFixtureFoundation(TestFixtureFoundation&& other) noexcept
    : m_config(std::move(other.m_config)), m_rootPath(std::move(other.m_rootPath)),
      m_fixtureId(std::move(other.m_fixtureId)), m_state(other.m_state),
      m_createdFilesCount(other.m_createdFilesCount.load(std::memory_order_relaxed)),
      m_lastError(other.m_lastError), m_diagnosticMessage(std::move(other.m_diagnosticMessage)) {
    other.m_state = TestEnvironmentState::TornDown;
    other.m_rootPath.clear();
}

TestFixtureFoundation& TestFixtureFoundation::operator=(TestFixtureFoundation&& other) noexcept {
    if (this != &other) {
        if (m_state == TestEnvironmentState::Active && !m_rootPath.empty()) {
            clean();
        }
        m_config = std::move(other.m_config);
        m_rootPath = std::move(other.m_rootPath);
        m_fixtureId = std::move(other.m_fixtureId);
        m_state = other.m_state;
        m_createdFilesCount.store(other.m_createdFilesCount.load(std::memory_order_relaxed));
        m_lastError = other.m_lastError;
        m_diagnosticMessage = std::move(other.m_diagnosticMessage);

        other.m_state = TestEnvironmentState::TornDown;
        other.m_rootPath.clear();
    }
    return *this;
}

std::filesystem::path TestFixtureFoundation::createSubdirectory(const std::string& relPath) {
    if (m_state == TestEnvironmentState::Corrupted || m_rootPath.empty()) {
        return {};
    }
    if (relPath.empty() || relPath.find("..") != std::string::npos || relPath.starts_with('/') ||
        relPath.starts_with('\\')) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }
    const auto fullPath = m_rootPath / relPath;
    if (!TestFixtureValidator::isWithinTempRoot(fullPath, m_rootPath)) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }
    std::error_code ec;
    std::filesystem::create_directories(fullPath, ec);
    if (ec) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return {};
    }
    return fullPath;
}

std::filesystem::path TestFixtureFoundation::createTestFile(const std::string& relPath,
                                                            const std::string& content) {
    if (m_state == TestEnvironmentState::Corrupted || m_rootPath.empty()) {
        return {};
    }
    if (relPath.empty() || relPath.find("..") != std::string::npos || relPath.starts_with('/') ||
        relPath.starts_with('\\')) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }
    const auto currentSize = calculateTotalSizeBytes();
    if (currentSize + content.size() > m_config.maxTempDirSizeBytes) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::QuotaExceeded;
        m_diagnosticMessage = TestFixtureValidator::fixtureErrorToString(FixtureError::QuotaExceeded);
        return {};
    }
    const auto fullPath = m_rootPath / relPath;
    if (!TestFixtureValidator::isWithinTempRoot(fullPath, m_rootPath)) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }

    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);
    if (ec) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return {};
    }

    std::ofstream ofs(fullPath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return {};
    }
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    ofs.close();

    m_createdFilesCount.fetch_add(1, std::memory_order_relaxed);
    return fullPath;
}

std::filesystem::path TestFixtureFoundation::createTestBinaryFile(const std::string& relPath,
                                                                  const std::vector<uint8_t>& bytes) {
    if (m_state == TestEnvironmentState::Corrupted || m_rootPath.empty()) {
        return {};
    }
    if (relPath.empty() || relPath.find("..") != std::string::npos || relPath.starts_with('/') ||
        relPath.starts_with('\\')) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }
    const auto currentSize = calculateTotalSizeBytes();
    if (currentSize + bytes.size() > m_config.maxTempDirSizeBytes) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::QuotaExceeded;
        m_diagnosticMessage = TestFixtureValidator::fixtureErrorToString(FixtureError::QuotaExceeded);
        return {};
    }
    const auto fullPath = m_rootPath / relPath;
    if (!TestFixtureValidator::isWithinTempRoot(fullPath, m_rootPath)) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return {};
    }

    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);
    if (ec) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return {};
    }

    std::ofstream ofs(fullPath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::DirectoryCreationFailed;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed);
        return {};
    }
    ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ofs.close();

    m_createdFilesCount.fetch_add(1, std::memory_order_relaxed);
    return fullPath;
}

bool TestFixtureFoundation::cleanupSqliteWalShm(const std::filesystem::path& dbPath) {
    if (m_rootPath.empty() || dbPath.empty()) {
        return false;
    }
    std::filesystem::path fullDbPath;
    if (dbPath.is_relative()) {
        fullDbPath = m_rootPath / dbPath;
    } else {
        fullDbPath = dbPath;
    }

    if (!TestFixtureValidator::isWithinTempRoot(fullDbPath, m_rootPath) &&
        fullDbPath.parent_path() != m_rootPath) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return false;
    }

    std::string basePathStr = fullDbPath.string();
    if (basePathStr.ends_with("-wal")) {
        basePathStr = basePathStr.substr(0, basePathStr.size() - 4);
    } else if (basePathStr.ends_with("-shm")) {
        basePathStr = basePathStr.substr(0, basePathStr.size() - 4);
    }

    const std::filesystem::path mainDb{basePathStr};
    const std::filesystem::path walFile{basePathStr + "-wal"};
    const std::filesystem::path shmFile{basePathStr + "-shm"};

    std::error_code ec;
    std::filesystem::remove(mainDb, ec);
    std::filesystem::remove(walFile, ec);
    std::filesystem::remove(shmFile, ec);

    return true;
}

std::size_t TestFixtureFoundation::calculateTotalSizeBytes() const {
    if (m_rootPath.empty()) {
        return 0;
    }
    std::error_code ec;
    if (!std::filesystem::exists(m_rootPath, ec)) {
        return 0;
    }
    std::size_t total = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             m_rootPath, std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            break;
        }
        if (entry.is_regular_file(ec) && !ec) {
            total += entry.file_size(ec);
        }
    }
    return total;
}

bool TestFixtureFoundation::checkQuota() {
    if (m_state == TestEnvironmentState::Corrupted) {
        return false;
    }
    const auto total = calculateTotalSizeBytes();
    if (total > m_config.maxTempDirSizeBytes) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::QuotaExceeded;
        m_diagnosticMessage = TestFixtureValidator::fixtureErrorToString(FixtureError::QuotaExceeded);
        return false;
    }
    return true;
}

bool TestFixtureFoundation::clean() {
    if (m_rootPath.empty()) {
        m_state = TestEnvironmentState::TornDown;
        return true;
    }
    std::error_code ec;
    if (!std::filesystem::exists(m_rootPath, ec)) {
        m_state = TestEnvironmentState::TornDown;
        m_diagnosticMessage = TestFixtureValidator::fixtureStateToString(TestEnvironmentState::TornDown);
        return true;
    }
    if (!TestFixtureValidator::isSafeTestDirectory(m_rootPath)) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::UnsafePathDetected;
        m_diagnosticMessage =
            TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected);
        return false;
    }

    std::filesystem::remove_all(m_rootPath, ec);
    if (ec) {
        m_state = TestEnvironmentState::Corrupted;
        m_lastError = FixtureError::CleanupFailed;
        m_diagnosticMessage = TestFixtureValidator::fixtureErrorToString(FixtureError::CleanupFailed);
        return false;
    }

    m_state = TestEnvironmentState::TornDown;
    m_diagnosticMessage = TestFixtureValidator::fixtureStateToString(TestEnvironmentState::TornDown);
    return true;
}

TestFixtureReport TestFixtureFoundation::report() const {
    TestFixtureReport rep;
    rep.fixtureId = m_fixtureId;
    rep.rootPath = m_rootPath;
    rep.totalSizeBytes = calculateTotalSizeBytes();
    rep.activeFilesCount = 0;

    std::error_code ec;
    if (!m_rootPath.empty() && std::filesystem::exists(m_rootPath, ec)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 m_rootPath, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                break;
            }
            if (entry.is_regular_file(ec) && !ec) {
                rep.activeFilesCount++;
            }
        }
    }

    rep.isClean =
        (m_state == TestEnvironmentState::TornDown) || (!std::filesystem::exists(m_rootPath, ec));
    rep.state = m_state;
    rep.diagnosticMessage = m_diagnosticMessage;
    return rep;
}

TestEnvironmentGuard::TestEnvironmentGuard(TestEnvironmentGuard&& other) noexcept
    : m_overrides(std::move(other.m_overrides)) {
}

TestEnvironmentGuard& TestEnvironmentGuard::operator=(TestEnvironmentGuard&& other) noexcept {
    if (this != &other) {
        restoreAll();
        m_overrides = std::move(other.m_overrides);
    }
    return *this;
}

bool TestEnvironmentGuard::set(const std::string& name, const std::string& value) {
    if (name.empty() || name.find('=') != std::string::npos) {
        return false;
    }
    bool found = false;
    for (const auto& ov : m_overrides) {
        if (ov.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        EnvironmentVariableOverride ov;
        ov.name = name;
        ov.newValue = value;
        const char* current = std::getenv(name.c_str());
        if (current != nullptr) {
            ov.hasOriginalValue = true;
            ov.originalValue = current;
        } else {
            ov.hasOriginalValue = false;
            ov.originalValue = "";
        }
        m_overrides.push_back(ov);
    }

#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

bool TestEnvironmentGuard::unset(const std::string& name) {
    if (name.empty() || name.find('=') != std::string::npos) {
        return false;
    }
    bool found = false;
    for (const auto& ov : m_overrides) {
        if (ov.name == name) {
            found = true;
            break;
        }
    }
    if (!found) {
        EnvironmentVariableOverride ov;
        ov.name = name;
        ov.newValue = "";
        const char* current = std::getenv(name.c_str());
        if (current != nullptr) {
            ov.hasOriginalValue = true;
            ov.originalValue = current;
        } else {
            ov.hasOriginalValue = false;
            ov.originalValue = "";
        }
        m_overrides.push_back(ov);
    }

#ifdef _WIN32
    return _putenv_s(name.c_str(), "") == 0;
#else
    return unsetenv(name.c_str()) == 0;
#endif
}

void TestEnvironmentGuard::restoreAll() {
    for (auto it = m_overrides.rbegin(); it != m_overrides.rend(); ++it) {
        if (it->hasOriginalValue) {
#ifdef _WIN32
            _putenv_s(it->name.c_str(), it->originalValue.c_str());
#else
            setenv(it->name.c_str(), it->originalValue.c_str(), 1);
#endif
        } else {
#ifdef _WIN32
            _putenv_s(it->name.c_str(), "");
#else
            unsetenv(it->name.c_str());
#endif
        }
    }
    m_overrides.clear();
}

} // namespace vigilio
