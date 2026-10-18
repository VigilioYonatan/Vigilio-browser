#include "core/ClangTidyConfiguration.h"
#include "core/ClangTidyService.h"
#include "core/ClangTidyTypes.h"
#include "core/ClangTidyValidator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

class TempDirGuard {
  public:
    TempDirGuard() {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("vigilio_tidy_srv_" + std::to_string(::getpid()) + "_" + std::to_string(now));
        std::error_code ec;
        std::filesystem::create_directories(path_, ec);
    }

    ~TempDirGuard() {
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(
                 path_, std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            std::filesystem::permissions(it->path(), std::filesystem::perms::all, ec);
        }
        std::filesystem::remove_all(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

void writeFile(const std::filesystem::path& filePath, std::string_view content) {
    std::ofstream ofs(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        throw std::runtime_error("No fue posible crear archivo temporal: " + filePath.string());
    }
    ofs << content;
}

std::string readFile(const std::filesystem::path& filePath) {
    std::ifstream ifs(filePath, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("No fue posible leer archivo temporal: " + filePath.string());
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// TEST-TIDY-SRV-001: test_tidy_service_initialization
void test_tidy_service_initialization() {
    ClangTidyService unconfigured;
    CHECK(!unconfigured.isConfigured());

    const auto canonical = ClangTidyConfiguration::canonicalConfig();
    ClangTidyService configured(canonical);
    CHECK(configured.isConfigured());
    CHECK(configured.options() == canonical);

    ClangTidyOptions custom = canonical;
    custom.checks = "-*,bugprone-*";
    unconfigured.setOptions(custom);
    CHECK(unconfigured.isConfigured());
    CHECK(unconfigured.options().checks == "-*,bugprone-*");

#ifdef VIGILIO_CLANG_TIDY_PATH
    if (std::filesystem::exists(VIGILIO_CLANG_TIDY_PATH)) {
        ClangTidyService fromFile;
        CHECK(fromFile.loadConfiguration(VIGILIO_CLANG_TIDY_PATH));
        CHECK(fromFile.isConfigured());
        CHECK(fromFile.options() == canonical);
    }
#endif
}

// TEST-TIDY-SRV-002: test_tidy_service_in_memory
void test_tidy_service_in_memory() {
    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    const std::string clean =
        "void test() {\n    void* p = nullptr;\n    if (p == nullptr) {\n        return;\n    }\n}\n";
    const auto inspClean = service.inspectSource(clean, "clean.cpp");
    CHECK(inspClean.isClean);
    CHECK(inspClean.issuesCount == 0);
    CHECK(inspClean.summary.empty());

    const std::string dirty =
        "void test() {\n    void* p = NULL;\n    if (p == NULL)\n        return;\n}\n";
    const auto inspDirty = service.inspectSource(dirty, "dirty.cpp");
    CHECK(!inspDirty.isClean);
    CHECK(inspDirty.issuesCount >= 2);
    CHECK(!inspDirty.summary.empty());

    bool foundNullptr = false;
    bool foundBraces = false;
    for (const auto& issue : inspDirty.issues) {
        if (issue.checkName == "modernize-use-nullptr") {
            foundNullptr = true;
        }
        if (issue.checkName == "readability-braces-around-statements") {
            foundBraces = true;
        }
    }
    CHECK(foundNullptr);
    CHECK(foundBraces);
}

// TEST-TIDY-SRV-003: test_tidy_service_autofix_in_memory
void test_tidy_service_autofix_in_memory() {
    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    const std::string dirty =
        "void test() {\n    void* p = NULL;\n    if (p == NULL)\n        return;\n}\n";
    const std::string fixed = service.applyFixesToSource(dirty);

    CHECK(!fixed.empty());
    CHECK(fixed.find("NULL") == std::string::npos);
    CHECK(fixed.find("nullptr") != std::string::npos);
    CHECK(fixed.find("if (p == nullptr) {") != std::string::npos);

    const auto inspFixed = service.inspectSource(fixed, "fixed.cpp");
    CHECK(inspFixed.isClean);
    CHECK(inspFixed.issuesCount == 0);
}

// TEST-TIDY-SRV-004: test_tidy_service_dry_run
void test_tidy_service_dry_run() {
    TempDirGuard tempDir;
    const auto cleanPath = tempDir.path() / "clean.cpp";
    const auto dirtyPath = tempDir.path() / "dirty.cpp";

    const std::string cleanContent =
        "void func() {\n    int* p = nullptr;\n    if (p == nullptr) {\n        return;\n    }\n}\n";
    const std::string dirtyContent =
        "void func() {\n    int* p = NULL;\n    if (p == NULL)\n        return;\n}\n";

    writeFile(cleanPath, cleanContent);
    writeFile(dirtyPath, dirtyContent);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    const auto inspClean = service.inspectFile(cleanPath);
    CHECK(inspClean.isClean);
    CHECK(readFile(cleanPath) == cleanContent);

    const auto inspDirty = service.inspectFile(dirtyPath);
    CHECK(!inspDirty.isClean);
    CHECK(inspDirty.issuesCount >= 2);
    CHECK(readFile(dirtyPath) == dirtyContent);
}

// TEST-TIDY-SRV-005: test_tidy_service_physical_fix
void test_tidy_service_physical_fix() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "to_fix.cpp";
    const std::string dirtyContent =
        "void func() {\n    int* p = NULL;\n    if (p == NULL)\n        return;\n}\n";
    writeFile(filePath, dirtyContent);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    const auto res = service.applyFixesToFile(filePath);

    CHECK(res.status == TidyExecutionStatus::Success);
    CHECK(res.filesExamined == 1);
    CHECK(res.filesFixed == 1);

    const std::string after = readFile(filePath);
    CHECK(after != dirtyContent);
    CHECK(after.find("NULL") == std::string::npos);
    CHECK(after.find("nullptr") != std::string::npos);
    CHECK(after.find("if (p == nullptr) {") != std::string::npos);

    CHECK(!std::filesystem::exists(tempDir.path() / "to_fix.cpp.bak"));
}

// TEST-TIDY-SRV-006: test_tidy_service_backup_and_rollback
void test_tidy_service_backup_and_rollback() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "backup_test.cpp";
    const std::string pristine = "// Original Pristine Tidy Content\nint value = 42;\n";
    writeFile(filePath, pristine);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    std::string backupPath;
    CHECK(service.createBackup(filePath, &backupPath));
    CHECK(std::filesystem::exists(backupPath));

    writeFile(filePath, "// Mutated Content\nint broken = 0;\n");
    CHECK(readFile(filePath) != pristine);

    CHECK(service.rollbackFile(filePath));
    CHECK(readFile(filePath) == pristine);

    CHECK(service.removeBackup(filePath));
    CHECK(!std::filesystem::exists(backupPath));
}

// TEST-TIDY-SRV-007: test_tidy_service_retry_backoff (TASK-010)
void test_tidy_service_retry_backoff() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "retry_test.cpp";
    const std::string code = "void test() {\n    void* p = NULL;\n}\n";
    writeFile(filePath, code);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    TidyServiceOptions options;
    options.maxRetries = 2;
    options.retryBackoffMs = 10;

    // 1. Caso base exitoso
    const auto res = service.applyFixesWithRetry(filePath, options);
    CHECK(res.status == TidyExecutionStatus::Success);
    CHECK(res.filesFixed == 1);

    // 2. Contención transitoria simulada: el archivo comienza sin permisos de escritura
    const auto transientPath = tempDir.path() / "transient_contention.cpp";
    writeFile(transientPath, "void test() {\n    void* p = NULL;\n}\n");
    std::filesystem::permissions(transientPath, std::filesystem::perms::owner_read);

    options.maxRetries = 3;
    options.retryBackoffMs = 20;

    // Hilo secundario que restaura permisos de escritura tras breve retraso
    std::thread unlocker([transientPath]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        std::error_code ec;
        std::filesystem::permissions(
            transientPath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);
    });

    const auto retryRes = service.applyFixesWithRetry(transientPath, options);
    if (unlocker.joinable()) {
        unlocker.join();
    }

    // Asegurar permisos para limpieza en TempDirGuard
    std::error_code ec;
    std::filesystem::permissions(
        transientPath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);

    CHECK(retryRes.status == TidyExecutionStatus::Success);
    CHECK(retryRes.filesFixed == 1);

    // 3. Fallo persistente que excede maxRetries -> retorna FileLocked
    const auto lockedPath = tempDir.path() / "locked_file.cpp";
    writeFile(lockedPath, "void test() {\n    void* p = NULL;\n}\n");
    std::filesystem::permissions(lockedPath, std::filesystem::perms::owner_read);

    TidyServiceOptions failOptions;
    failOptions.maxRetries = 2;
    failOptions.retryBackoffMs = 5;

    const auto failRes = service.applyFixesWithRetry(lockedPath, failOptions);
    CHECK(failRes.status == TidyExecutionStatus::FileLocked);

    // Restaurar permisos para limpieza
    std::filesystem::permissions(
        lockedPath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);
}

// TEST-TIDY-SRV-008: test_tidy_service_cancellation
void test_tidy_service_cancellation() {
    TempDirGuard tempDir;
    writeFile(tempDir.path() / "a.cpp", "int a = 1;\n");
    writeFile(tempDir.path() / "b.cpp", "int b = 2;\n");

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    CancellationToken cancel;
    cancel.requestCancellation();

    const auto inspectRes = service.inspectDirectory(tempDir.path(), {}, &cancel);
    CHECK(inspectRes.status == TidyExecutionStatus::Cancelled);
    CHECK(inspectRes.remediationHint ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::Cancelled));

    const auto fixRes = service.applyFixesToFile(tempDir.path() / "a.cpp", {}, &cancel);
    CHECK(fixRes.status == TidyExecutionStatus::Cancelled);
}

// TEST-TIDY-SRV-009: test_tidy_service_directory_scan_and_filters
void test_tidy_service_directory_scan_and_filters() {
    TempDirGuard tempDir;
    const auto srcDir = tempDir.path() / "src";
    const auto incDir = tempDir.path() / "include";
    const auto buildDir = tempDir.path() / "_build";
    const auto gitDir = tempDir.path() / ".git";
    const auto assetDir = tempDir.path() / "assets";

    std::filesystem::create_directories(srcDir);
    std::filesystem::create_directories(incDir);
    std::filesystem::create_directories(buildDir);
    std::filesystem::create_directories(gitDir);
    std::filesystem::create_directories(assetDir);

    const std::string cleanContent = "void ok() {\n    void* p = nullptr;\n}\n";
    const std::string dirtyContent = "void bad() {\n    void* p = NULL;\n}\n";

    writeFile(srcDir / "main.cpp", cleanContent);
    writeFile(incDir / "header.h", cleanContent);
    writeFile(buildDir / "ignored.cpp", dirtyContent);
    writeFile(gitDir / "ignored.h", dirtyContent);
    writeFile(assetDir / "image.png", "binary_data");

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    const auto res = service.inspectDirectory(tempDir.path());

    CHECK(res.status == TidyExecutionStatus::Success);
    CHECK(res.filesExamined == 2);
    CHECK(res.issuesCount == 0);
}

// TEST-TIDY-SRV-010: test_tidy_service_quota_and_unsafe_path
void test_tidy_service_quota_and_unsafe_path() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "too_large.cpp";
    std::string largeContent(2048, 'x');
    writeFile(filePath, largeContent);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    TidyServiceOptions options;
    options.maxFileSizeBytes = 512;

    const auto res = service.applyFixesToFile(filePath, options);
    CHECK(res.status == TidyExecutionStatus::FileTooLarge);
    CHECK(res.remediationHint ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::FileTooLarge));

    const auto inspUnsafe = service.inspectFile("../../etc/passwd");
    CHECK(!inspUnsafe.isClean);
    CHECK(inspUnsafe.summary ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::UnsafePath));

    const auto fixUnsafe = service.applyFixesToFile("../escape/source.cpp");
    CHECK(fixUnsafe.status == TidyExecutionStatus::UnsafePath);
    CHECK(fixUnsafe.remediationHint ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::UnsafePath));
}

// TEST-TIDY-SRV-011: test_tidy_service_cli_synthesis (TASK-011)
void test_tidy_service_cli_synthesis() {
    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    // 1. Línea de comandos estándar con -fix y -warnings-as-errors
    const std::string cmd =
        service.buildClangTidyCommandLine("apps/vigilio-browser/src/main.cpp", "_build", true, true);
    CHECK(cmd.find("clang-tidy") != std::string::npos);
    CHECK(cmd.find("--checks=") != std::string::npos);
    CHECK(cmd.find("-fix") != std::string::npos);
    CHECK(cmd.find("-warnings-as-errors=") != std::string::npos);
    CHECK(cmd.find("-p _build") != std::string::npos);
    CHECK(cmd.find("apps/vigilio-browser/src/main.cpp") != std::string::npos);

    // 2. Rutas con espacios
    const std::string cmdSpaces = service.buildClangTidyCommandLine("path with spaces/file name.cpp",
                                                                    "custom build dir", true, false);
    CHECK(cmdSpaces.find("path with spaces/file name.cpp") != std::string::npos);
    CHECK(cmdSpaces.find("-p custom build dir") != std::string::npos);
    CHECK(cmdSpaces.find("-fix") != std::string::npos);
    CHECK(cmdSpaces.find("-warnings-as-errors") == std::string::npos);

    // 3. Parámetros absolutos y relativos sin -fix
    const std::string cmdNoFix =
        service.buildClangTidyCommandLine("/usr/local/include/header.h", "", false, false);
    CHECK(cmdNoFix.find("/usr/local/include/header.h") != std::string::npos);
    CHECK(cmdNoFix.find("-fix") == std::string::npos);
    CHECK(cmdNoFix.find("-p") == std::string::npos);
}

// TEST-TIDY-SRV-012: test_tidy_service_public_copy_compliance (TASK-012)
void test_tidy_service_public_copy_compliance() {
    const std::vector<std::string> forbiddenTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                     "DAG",         "OpenSpec",    "toolchain"};

    const std::vector<TidyExecutionStatus> allStatuses = {TidyExecutionStatus::Success,
                                                          TidyExecutionStatus::IssuesFound,
                                                          TidyExecutionStatus::Cancelled,
                                                          TidyExecutionStatus::FileLocked,
                                                          TidyExecutionStatus::PermissionDenied,
                                                          TidyExecutionStatus::ReadError,
                                                          TidyExecutionStatus::WriteError,
                                                          TidyExecutionStatus::FileTooLarge,
                                                          TidyExecutionStatus::UnsafePath,
                                                          TidyExecutionStatus::FileNotFound,
                                                          TidyExecutionStatus::ConfigurationError,
                                                          TidyExecutionStatus::ToolUnavailable};

    for (const auto st : allStatuses) {
        const std::string text = ClangTidyService::executionStatusToString(st);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }

        const std::string hint = ClangTidyService::generateRemediationHint(st, 0);
        CHECK(!hint.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(hint.find(term) == std::string::npos);
        }
    }
}

// TEST-TIDY-SRV-013: test_tidy_service_massive_multithreading_concurrency (TASK-005)
void test_tidy_service_massive_multithreading_concurrency() {
    TempDirGuard tempDir;
    const auto diskCleanPath = tempDir.path() / "disk_clean.cpp";
    writeFile(diskCleanPath, "void func() {\n    int* p = nullptr;\n}\n");

    const auto canonical = ClangTidyConfiguration::canonicalConfig();
    ClangTidyService sharedService(canonical);

    const int numThreads = 16;
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    std::atomic<bool> allPassed{true};

    for (int i = 0; i < numThreads; ++i) {
        workers.emplace_back([&sharedService, &diskCleanPath, &allPassed, i]() {
            try {
                for (int iter = 0; iter < 4; ++iter) {
                    if (i % 3 == 0) {
                        const std::string code =
                            "void worker" + std::to_string(i) +
                            "() {\n    void* p = " + (iter % 2 == 0 ? "NULL" : "nullptr") + ";\n}\n";
                        const auto insp =
                            sharedService.inspectSource(code, "worker_" + std::to_string(i) + ".cpp");
                        if (iter % 2 == 0 && insp.isClean) {
                            allPassed = false;
                        } else if (iter % 2 != 0 && !insp.isClean) {
                            allPassed = false;
                        }
                    } else if (i % 3 == 1) {
                        const std::string dirty =
                            "void fixWorker" + std::to_string(i) + "() {\n    void* p = NULL;\n}\n";
                        const std::string fixed = sharedService.applyFixesToSource(dirty);
                        if (fixed.find("nullptr") == std::string::npos ||
                            fixed.find("NULL") != std::string::npos) {
                            allPassed = false;
                        }
                    } else {
                        const auto fileInsp = sharedService.inspectFile(diskCleanPath);
                        if (!fileInsp.isClean) {
                            allPassed = false;
                        }
                    }
                }
            } catch (...) {
                allPassed = false;
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    CHECK(allPassed.load());
    CHECK(sharedService.isConfigured());
}

// TEST-TIDY-SRV-014: test_tidy_service_cooperative_cancellation_stress (TASK-006)
void test_tidy_service_cooperative_cancellation_stress() {
    TempDirGuard tempDir;
    for (int i = 0; i < 25; ++i) {
        writeFile(tempDir.path() / ("cancel_file_" + std::to_string(i) + ".cpp"),
                  "void func" + std::to_string(i) + "() {\n    void* p = NULL;\n}\n");
    }

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    CancellationToken cancel;

    // Disparo concurrente asíncrono desde hilo secundario
    std::thread cancelThread([&cancel]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        cancel.requestCancellation();
    });

    const auto res = service.inspectDirectory(tempDir.path(), {}, &cancel);
    if (cancelThread.joinable()) {
        cancelThread.join();
    }

    CHECK(cancel.isCancelled());

    // Cancelación previa inmediata
    CancellationToken immediateCancel;
    immediateCancel.requestCancellation();
    const auto cancelRes = service.inspectDirectory(tempDir.path(), {}, &immediateCancel);
    CHECK(cancelRes.status == TidyExecutionStatus::Cancelled);
    CHECK(cancelRes.remediationHint ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::Cancelled));

    const auto fixRes =
        service.applyFixesToFile(tempDir.path() / "cancel_file_0.cpp", {}, &immediateCancel);
    CHECK(fixRes.status == TidyExecutionStatus::Cancelled);
    CHECK(fixRes.remediationHint ==
          ClangTidyService::executionStatusToString(TidyExecutionStatus::Cancelled));
}

// TEST-TIDY-SRV-015: test_tidy_service_permission_failure_injection (TASK-007)
void test_tidy_service_permission_failure_injection() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "readonly_file.cpp";
    const std::string original = "void test() {\n    void* p = NULL;\n}\n";
    writeFile(filePath, original);

    // Remover permisos de escritura
    std::error_code ec;
    std::filesystem::permissions(filePath, std::filesystem::perms::owner_read, ec);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    const auto res = service.applyFixesToFile(filePath);

    // Debe retornar PermissionDenied o WriteError limpiamente
    CHECK(res.status == TidyExecutionStatus::PermissionDenied ||
          res.status == TidyExecutionStatus::WriteError);

    // Restaurar permisos para verificación y saneamiento
    std::filesystem::permissions(
        filePath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);

    // Comprobar que el archivo original se mantiene íntegro sin corrupción
    CHECK(readFile(filePath) == original);
}

// TEST-TIDY-SRV-016: test_tidy_service_recursive_directory_tree (TASK-008)
void test_tidy_service_recursive_directory_tree() {
    TempDirGuard tempDir;
    const auto root = tempDir.path();

    const auto nestedDir = root / "a" / "b" / "c" / "d";
    std::filesystem::create_directories(nestedDir);

    const auto buildDir = root / "_build";
    std::filesystem::create_directories(buildDir);

    const auto gitDir = root / ".git";
    std::filesystem::create_directories(gitDir);

    const auto generatedDir = root / "generated";
    std::filesystem::create_directories(generatedDir);

    const auto assetsDir = root / "assets";
    std::filesystem::create_directories(assetsDir);

    const std::string compliantCode = "void ok() {\n    void* p = nullptr;\n}\n";
    const std::string dirtyCode = "void bad() {\n    void* p = NULL;\n}\n";

    writeFile(nestedDir / "compliant.cpp", compliantCode);
    writeFile(nestedDir / "dirty.hpp", dirtyCode);
    writeFile(root / "a" / "other_dirty.cc", dirtyCode);
    writeFile(root / "a" / "notes.txt", "Plain text notes.");
    writeFile(nestedDir / "config.json", "{\"valid\": true}");
    writeFile(assetsDir / "logo.png", "binary_png_data");
    writeFile(buildDir / "ignored_build.cpp", dirtyCode);
    writeFile(gitDir / "ignored_git.cpp", dirtyCode);
    writeFile(generatedDir / "ignored_gen.cpp", dirtyCode);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    const auto res = service.inspectDirectory(root);

    // Archivos C++ válidos examinados: compliant.cpp, dirty.hpp, other_dirty.cc (3)
    // Carpetas ignoradas: _build, .git, generated, assets
    // Archivos no C++ ignorados: notes.txt, config.json, logo.png
    CHECK(res.filesExamined == 3);
    CHECK(res.issuesCount >= 2);
    CHECK(res.status == TidyExecutionStatus::IssuesFound);

    // Archivos en disco permanecen estrictamente inmutables (dry-run)
    CHECK(readFile(nestedDir / "compliant.cpp") == compliantCode);
    CHECK(readFile(nestedDir / "dirty.hpp") == dirtyCode);
    CHECK(readFile(root / "a" / "other_dirty.cc") == dirtyCode);
}

// TEST-TIDY-SRV-017: test_tidy_service_backup_atomic_idempotence (TASK-009)
void test_tidy_service_backup_atomic_idempotence() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "atomic_idempotence_test.cpp";
    const std::string original = "// Canonical Source Code\n"
                                 "void calculate() {\n"
                                 "    int* p = nullptr;\n"
                                 "}\n";
    writeFile(filePath, original);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    // 1. Respaldo atómico inicial
    std::string backupPath;
    CHECK(service.createBackup(filePath, &backupPath));
    CHECK(std::filesystem::exists(backupPath));

    // 2. Mutación deliberada
    writeFile(filePath, "// MUTATED CONTENT\nvoid calculate() { int* p = NULL; }\n");
    CHECK(readFile(filePath) != original);

    // 3. Rollback determinista
    CHECK(service.rollbackFile(filePath));
    CHECK(readFile(filePath) == original);

    // 4. Idempotencia ante llamadas sucesivas redundantes de rollback
    CHECK(service.rollbackFile(filePath));
    CHECK(readFile(filePath) == original);

    // 5. Eliminación de respaldo y prueba de rollback sobre archivo sin backup
    CHECK(service.removeBackup(filePath));
    CHECK(!std::filesystem::exists(backupPath));
    CHECK(!service.rollbackFile(filePath));

    // 6. Rollback sobre archivo inexistente
    CHECK(!service.rollbackFile(tempDir.path() / "non_existent.cpp"));
}

} // namespace

int main() {
    std::cout << "[ClangTidyServiceTests] Running 17 unit tests..." << std::endl;

    try {
        test_tidy_service_initialization();
        std::cout << "  PASS: TEST-TIDY-SRV-001 (initialization)" << std::endl;

        test_tidy_service_in_memory();
        std::cout << "  PASS: TEST-TIDY-SRV-002 (in-memory inspection)" << std::endl;

        test_tidy_service_autofix_in_memory();
        std::cout << "  PASS: TEST-TIDY-SRV-003 (in-memory autofix)" << std::endl;

        test_tidy_service_dry_run();
        std::cout << "  PASS: TEST-TIDY-SRV-004 (dry-run inspection)" << std::endl;

        test_tidy_service_physical_fix();
        std::cout << "  PASS: TEST-TIDY-SRV-005 (physical fixes)" << std::endl;

        test_tidy_service_backup_and_rollback();
        std::cout << "  PASS: TEST-TIDY-SRV-006 (backup & rollback)" << std::endl;

        test_tidy_service_retry_backoff();
        std::cout << "  PASS: TEST-TIDY-SRV-007 (retry exponential backoff)" << std::endl;

        test_tidy_service_cancellation();
        std::cout << "  PASS: TEST-TIDY-SRV-008 (cooperative cancellation)" << std::endl;

        test_tidy_service_directory_scan_and_filters();
        std::cout << "  PASS: TEST-TIDY-SRV-009 (directory scan & exclusions)" << std::endl;

        test_tidy_service_quota_and_unsafe_path();
        std::cout << "  PASS: TEST-TIDY-SRV-010 (quota rejection & path traversal)" << std::endl;

        test_tidy_service_cli_synthesis();
        std::cout << "  PASS: TEST-TIDY-SRV-011 (CLI synthesis)" << std::endl;

        test_tidy_service_public_copy_compliance();
        std::cout << "  PASS: TEST-TIDY-SRV-012 (UX public copy compliance)" << std::endl;

        test_tidy_service_massive_multithreading_concurrency();
        std::cout << "  PASS: TEST-TIDY-SRV-013 (massive multithreading concurrency)" << std::endl;

        test_tidy_service_cooperative_cancellation_stress();
        std::cout << "  PASS: TEST-TIDY-SRV-014 (cooperative cancellation stress)" << std::endl;

        test_tidy_service_permission_failure_injection();
        std::cout << "  PASS: TEST-TIDY-SRV-015 (permission failure injection)" << std::endl;

        test_tidy_service_recursive_directory_tree();
        std::cout << "  PASS: TEST-TIDY-SRV-016 (recursive directory tree)" << std::endl;

        test_tidy_service_backup_atomic_idempotence();
        std::cout << "  PASS: TEST-TIDY-SRV-017 (backup atomic idempotence)" << std::endl;

        std::cout << "[ClangTidyServiceTests] All 17 tests PASSED successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
        return 1;
    }
}
