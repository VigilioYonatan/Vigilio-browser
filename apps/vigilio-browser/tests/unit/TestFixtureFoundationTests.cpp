#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureValidator.h"
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
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

void test_fixture_canonical_config() {
    TestEnvironmentConfig config;
    CHECK(TestFixtureValidator::validateConfig(config) == FixtureError::None);
    CHECK(config.prefix == "vigilio_test");
    CHECK(config.maxTempDirSizeBytes == 52428800);
    CHECK(config.cleanPolicy == FixtureCleanPolicy::AlwaysClean);
    CHECK(!config.isolateEnvironment);
    CHECK(config.autoCleanupWal);
}

void test_fixture_lifecycle_raii() {
    std::filesystem::path fixturePath;
    {
        TestFixtureFoundation fixture;
        CHECK(fixture.isValid());
        CHECK(fixture.state() == TestEnvironmentState::Active);
        fixturePath = fixture.rootPath();
        CHECK(!fixturePath.empty());
        CHECK(std::filesystem::exists(fixturePath));
        CHECK(TestFixtureValidator::isSafeTestDirectory(fixturePath));
    }
    CHECK(!std::filesystem::exists(fixturePath));
}

void test_fixture_unsafe_path_rejection() {
    // 1. Hostile or out-of-bounds directories
    CHECK(!TestFixtureValidator::isSafeTestDirectory("/etc"));
    CHECK(!TestFixtureValidator::isSafeTestDirectory("/home"));
    CHECK(!TestFixtureValidator::isSafeTestDirectory("/"));
    CHECK(!TestFixtureValidator::isSafeTestDirectory("../escape"));
    CHECK(!TestFixtureValidator::isSafeTestDirectory(std::filesystem::temp_directory_path()));

    const auto validDir = std::filesystem::temp_directory_path() / "vigilio_test_safe_sample_123";
    CHECK(TestFixtureValidator::isSafeTestDirectory(validDir));
    CHECK(TestFixtureValidator::isSafeTestDirectory(validDir / "cache"));

    const auto nonVigilioDir = std::filesystem::temp_directory_path() / "other_prefix_123";
    CHECK(!TestFixtureValidator::isSafeTestDirectory(nonVigilioDir));

    // 2. Config validation rejections
    TestEnvironmentConfig cfgTraversal;
    cfgTraversal.prefix = "../escape";
    CHECK(TestFixtureValidator::validateConfig(cfgTraversal) == FixtureError::UnsafePathDetected);

    TestEnvironmentConfig cfgSlash;
    cfgSlash.prefix = "sub/dir";
    CHECK(TestFixtureValidator::validateConfig(cfgSlash) == FixtureError::UnsafePathDetected);

    TestEnvironmentConfig cfgBackslash;
    cfgBackslash.prefix = "sub\\dir";
    CHECK(TestFixtureValidator::validateConfig(cfgBackslash) == FixtureError::UnsafePathDetected);

    TestEnvironmentConfig cfgEmpty;
    cfgEmpty.prefix = "";
    CHECK(TestFixtureValidator::validateConfig(cfgEmpty) == FixtureError::InvalidConfiguration);

    TestEnvironmentConfig cfgLowQuota;
    cfgLowQuota.prefix = "valid";
    cfgLowQuota.maxTempDirSizeBytes = 500;
    CHECK(TestFixtureValidator::validateConfig(cfgLowQuota) == FixtureError::InvalidConfiguration);

    TestEnvironmentConfig cfgHighQuota;
    cfgHighQuota.prefix = "valid";
    cfgHighQuota.maxTempDirSizeBytes = 600ULL * 1024 * 1024;
    CHECK(TestFixtureValidator::validateConfig(cfgHighQuota) == FixtureError::InvalidConfiguration);
}

void test_fixture_subdirectories_and_files() {
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    const auto sub = fixture.createSubdirectory("cache/v1");
    CHECK(!sub.empty());
    CHECK(std::filesystem::is_directory(sub));

    const std::string textContent = "{\"tabs\":[]}";
    const auto textFile = fixture.createTestFile("cache/v1/session.json", textContent);
    CHECK(!textFile.empty());
    CHECK(std::filesystem::is_regular_file(textFile));

    std::ifstream ifs(textFile);
    std::string readBack;
    ifs >> readBack;
    CHECK(readBack == textContent);

    const std::vector<uint8_t> binContent = {0x01, 0x02, 0x03, 0xFF};
    const auto binFile = fixture.createTestBinaryFile("data.bin", binContent);
    CHECK(!binFile.empty());
    CHECK(std::filesystem::is_regular_file(binFile));

    const auto totalSize = fixture.calculateTotalSizeBytes();
    CHECK(totalSize == textContent.size() + binContent.size());

    const auto rep = fixture.report();
    CHECK(rep.activeFilesCount == 2);
    CHECK(rep.totalSizeBytes == totalSize);
    CHECK(!rep.isClean);
    CHECK(rep.state == TestEnvironmentState::Active);
}

void test_fixture_quota_enforcement() {
    TestEnvironmentConfig cfg;
    cfg.maxTempDirSizeBytes = 1024; // 1 KB
    TestFixtureFoundation fixture(cfg);
    CHECK(fixture.isValid());

    const std::string initialChunk(500, 'A');
    const auto file1 = fixture.createTestFile("initial.dat", initialChunk);
    CHECK(!file1.empty());
    CHECK(fixture.checkQuota());

    // Attempting to write 1000 bytes will exceed 1024 bytes (500 + 1000 = 1500 > 1024)
    const std::string oversizedChunk(1000, 'B');
    const auto file2 = fixture.createTestFile("overflow.dat", oversizedChunk);
    CHECK(file2.empty());
    CHECK(fixture.lastError() == FixtureError::QuotaExceeded);
    CHECK(fixture.state() == TestEnvironmentState::Corrupted);
    CHECK(!fixture.checkQuota());
}

void test_fixture_sqlite_cleanup() {
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    fixture.createTestFile("database.sqlite", "SQLITE_HEADER");
    fixture.createTestFile("database.sqlite-wal", "WAL_JOURNAL");
    fixture.createTestFile("database.sqlite-shm", "SHM_INDEX");

    const auto dbPath = fixture.rootPath() / "database.sqlite";
    const auto walPath = fixture.rootPath() / "database.sqlite-wal";
    const auto shmPath = fixture.rootPath() / "database.sqlite-shm";

    CHECK(std::filesystem::exists(dbPath));
    CHECK(std::filesystem::exists(walPath));
    CHECK(std::filesystem::exists(shmPath));

    CHECK(fixture.cleanupSqliteWalShm("database.sqlite"));

    CHECK(!std::filesystem::exists(dbPath));
    CHECK(!std::filesystem::exists(walPath));
    CHECK(!std::filesystem::exists(shmPath));
}

void test_fixture_environment_guard() {
    const std::string varName = "VIGILIO_TEST_FIXTURE_ENV_VAR";

    // Subcase 1: Variable previously not set
#ifdef _WIN32
    _putenv_s(varName.c_str(), "");
#else
    unsetenv(varName.c_str());
#endif
    CHECK(std::getenv(varName.c_str()) == nullptr);

    {
        TestEnvironmentGuard guard;
        CHECK(guard.set(varName, "temporary_value_alpha"));
        const char* current = std::getenv(varName.c_str());
        CHECK(current != nullptr);
        CHECK(std::string(current) == "temporary_value_alpha");
    }
    CHECK(std::getenv(varName.c_str()) == nullptr);

    // Subcase 2: Variable previously set
#ifdef _WIN32
    _putenv_s(varName.c_str(), "original_stable_value");
#else
    setenv(varName.c_str(), "original_stable_value", 1);
#endif
    CHECK(std::getenv(varName.c_str()) != nullptr);

    {
        TestEnvironmentGuard guard;
        CHECK(guard.set(varName, "temporary_value_beta"));
        const char* current = std::getenv(varName.c_str());
        CHECK(current != nullptr);
        CHECK(std::string(current) == "temporary_value_beta");

        // Test unset within guard
        CHECK(guard.unset(varName));
        CHECK(std::getenv(varName.c_str()) == nullptr);
    }

    const char* restored = std::getenv(varName.c_str());
    CHECK(restored != nullptr);
    CHECK(std::string(restored) == "original_stable_value");

#ifdef _WIN32
    _putenv_s(varName.c_str(), "");
#else
    unsetenv(varName.c_str());
#endif
}

void test_fixture_concurrency_isolation() {
    std::vector<std::unique_ptr<TestFixtureFoundation>> fixtures;
    std::set<std::filesystem::path> paths;
    std::set<std::string> ids;

    for (int i = 0; i < 10; ++i) {
        auto fixture = std::make_unique<TestFixtureFoundation>();
        CHECK(fixture->isValid());
        CHECK(paths.find(fixture->rootPath()) == paths.end());
        CHECK(ids.find(fixture->fixtureId()) == ids.end());
        paths.insert(fixture->rootPath());
        ids.insert(fixture->fixtureId());
        fixtures.push_back(std::move(fixture));
    }

    CHECK(paths.size() == 10);
    CHECK(ids.size() == 10);

    for (const auto& p : paths) {
        CHECK(std::filesystem::exists(p));
    }

    fixtures.clear();

    for (const auto& p : paths) {
        CHECK(!std::filesystem::exists(p));
    }
}

void test_fixture_permission_resilience() {
    TestFixtureFoundation fixture;
    CHECK(fixture.isValid());

    const auto sub = fixture.createSubdirectory("locked_sub");
    CHECK(!sub.empty());
    fixture.createTestFile("locked_sub/dummy.txt", "content");

#ifndef _WIN32
    const bool isRoot = (geteuid() == 0);
#else
    const bool isRoot = false;
#endif

    std::error_code ec;
    // Remove write and execute permissions on locked_sub
    std::filesystem::permissions(sub, std::filesystem::perms::owner_read,
                                 std::filesystem::perm_options::replace, ec);

    const bool cleaned = fixture.clean();
    if (isRoot) {
        CHECK(cleaned);
    } else {
        CHECK(!cleaned);
        CHECK(fixture.lastError() == FixtureError::CleanupFailed);
        CHECK(fixture.state() == TestEnvironmentState::Corrupted);

        // Restore permissions so no residual directory remains
        std::filesystem::permissions(sub, std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::replace, ec);
        std::filesystem::remove_all(fixture.rootPath(), ec);
    }
}

void test_fixture_public_copy_audit() {
    CHECK(TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Uninitialized) ==
          "Entorno de prueba no inicializado");
    CHECK(TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Configured) ==
          "Configuración de prueba lista para activación");
    CHECK(TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Active) ==
          "Entorno de prueba activo y aislado");
    CHECK(TestFixtureValidator::fixtureStateToString(TestEnvironmentState::TornDown) ==
          "Entorno de prueba desmontado y limpiado");
    CHECK(TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Corrupted) ==
          "Error en el estado del entorno de prueba");

    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::None) ==
          "Operación completada exitosamente");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed) ==
          "No se pudo crear la carpeta temporal de prueba");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::CleanupFailed) ==
          "No se pudo limpiar completamente la carpeta temporal");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::QuotaExceeded) ==
          "El espacio en disco asignado para la prueba ha sido superado");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::EnvironmentOverrideFailed) ==
          "No se pudo configurar la variable de entorno temporal");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::InvalidConfiguration) ==
          "Configuración del entorno de prueba no válida");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryLocked) ==
          "La carpeta temporal está bloqueada o en uso por otro proceso");
    CHECK(TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected) ==
          "Ruta de destino no segura o fuera de la carpeta temporal autorizada");

    const std::vector<std::string> prohibited = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                 "DAG",         "OpenSpec",    "toolchain"};

    const std::vector<std::string> samples = {
        TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Uninitialized),
        TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Configured),
        TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Active),
        TestFixtureValidator::fixtureStateToString(TestEnvironmentState::TornDown),
        TestFixtureValidator::fixtureStateToString(TestEnvironmentState::Corrupted),
        TestFixtureValidator::fixtureErrorToString(FixtureError::None),
        TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryCreationFailed),
        TestFixtureValidator::fixtureErrorToString(FixtureError::CleanupFailed),
        TestFixtureValidator::fixtureErrorToString(FixtureError::QuotaExceeded),
        TestFixtureValidator::fixtureErrorToString(FixtureError::EnvironmentOverrideFailed),
        TestFixtureValidator::fixtureErrorToString(FixtureError::InvalidConfiguration),
        TestFixtureValidator::fixtureErrorToString(FixtureError::DirectoryLocked),
        TestFixtureValidator::fixtureErrorToString(FixtureError::UnsafePathDetected)};

    for (const auto& sample : samples) {
        for (const auto& word : prohibited) {
            CHECK(sample.find(word) == std::string::npos);
        }
    }
}

} // namespace

int main() {
    try {
        test_fixture_canonical_config();
        test_fixture_lifecycle_raii();
        test_fixture_unsafe_path_rejection();
        test_fixture_subdirectories_and_files();
        test_fixture_quota_enforcement();
        test_fixture_sqlite_cleanup();
        test_fixture_environment_guard();
        test_fixture_concurrency_isolation();
        test_fixture_permission_resilience();
        test_fixture_public_copy_audit();

        std::cout << "TestFixtureFoundationTests: ALL 10 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "TestFixtureFoundationTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
