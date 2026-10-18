#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatService.h"
#include "core/ClangFormatTypes.h"
#include "core/ClangFormatValidator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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
                ("vigilio_fmt_srv_" + std::to_string(::getpid()) + "_" + std::to_string(now));
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

// TEST-FORMAT-SRV-001: test_format_service_initialization
void test_format_service_initialization() {
    ClangFormatService unconfigured;
    CHECK(!unconfigured.isConfigured());

    const auto canonical = ClangFormatConfiguration::canonicalConfig();
    ClangFormatService configured(canonical);
    CHECK(configured.isConfigured());
    CHECK(configured.options() == canonical);

    ClangFormatOptions custom = canonical;
    custom.indentWidth = 2;
    unconfigured.setOptions(custom);
    CHECK(unconfigured.isConfigured());
    CHECK(unconfigured.options().indentWidth == 2);

#ifdef VIGILIO_CLANG_FORMAT_PATH
    if (std::filesystem::exists(VIGILIO_CLANG_FORMAT_PATH)) {
        ClangFormatService fromFile;
        CHECK(fromFile.loadConfiguration(VIGILIO_CLANG_FORMAT_PATH));
        CHECK(fromFile.isConfigured());
        CHECK(fromFile.options() == canonical);
    }
#endif
}

// TEST-FORMAT-SRV-002: test_format_service_in_memory
void test_format_service_in_memory() {
    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());

    const std::string unformatted = "void test(int *x){\nint y=1;\nreturn;\n}\n";
    const std::string formatted =
        service.formatSourceCode(unformatted, ClangFormatConfiguration::canonicalConfig());

    CHECK(!formatted.empty());
    CHECK(formatted.find("void test(int* x) {") != std::string::npos);
    CHECK(formatted.find("    int y = 1;") != std::string::npos);
    CHECK(formatted.find("    return;") != std::string::npos);

    const auto unformattedInsp = service.inspectSource(unformatted, "memory_test.cpp");
    CHECK(!unformattedInsp.isCompliant);
    CHECK(!unformattedInsp.discrepancySummary.empty());

    const auto formattedInsp = service.inspectSource(formatted, "memory_test.cpp");
    CHECK(formattedInsp.isCompliant);
    CHECK(formattedInsp.discrepancySummary.empty());
}

// TEST-FORMAT-SRV-003: test_format_service_dry_run
void test_format_service_dry_run() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "compliant.cpp";
    const std::string pristine = "int main() {\n    return 0;\n}\n";
    writeFile(filePath, pristine);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto insp = service.inspectFile(filePath);

    CHECK(insp.isCompliant);
    CHECK(insp.discrepancySummary.empty());
    CHECK(readFile(filePath) == pristine);
}

// TEST-FORMAT-SRV-004: test_format_service_dry_run_discrepancy
void test_format_service_dry_run_discrepancy() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "dirty.cpp";
    const std::string dirty = "int main(){\nreturn 0;\n}\n";
    writeFile(filePath, dirty);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto insp = service.inspectFile(filePath);

    CHECK(!insp.isCompliant);
    CHECK(!insp.discrepancySummary.empty());
    CHECK(readFile(filePath) == dirty);
}

// TEST-FORMAT-SRV-005: test_format_service_physical_format
void test_format_service_physical_format() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "to_format.cpp";
    const std::string dirty = "int main(){\nreturn 0;\n}\n";
    writeFile(filePath, dirty);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto res = service.formatFile(filePath);

    CHECK(res.status == FormatExecutionStatus::Success);
    CHECK(res.filesExamined == 1);
    CHECK(res.filesFormatted == 1);
    CHECK(res.discrepanciesCount == 1);

    const std::string after = readFile(filePath);
    CHECK(after != dirty);
    CHECK(after.find("int main() {") != std::string::npos);
    CHECK(after.find("    return 0;") != std::string::npos);
}

// TEST-FORMAT-SRV-006: test_format_service_backup_and_rollback
void test_format_service_backup_and_rollback() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "backup_test.cpp";
    const std::string pristine = "// Original Pristine Code\nint a = 42;\n";
    writeFile(filePath, pristine);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
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

// TEST-FORMAT-SRV-007: test_format_service_retry_backoff
void test_format_service_retry_backoff() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "retry_test.cpp";
    const std::string code = "int x=1;\n";
    writeFile(filePath, code);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    FormatServiceOptions options;
    options.maxRetries = 2;
    options.retryBackoffMs = 10;

    const auto res = service.formatWithRetry(filePath, options);
    CHECK(res.status == FormatExecutionStatus::Success);
    CHECK(res.filesFormatted == 1);
}

// TEST-FORMAT-SRV-008: test_format_service_cancellation
void test_format_service_cancellation() {
    TempDirGuard tempDir;
    writeFile(tempDir.path() / "a.cpp", "int a = 1;\n");
    writeFile(tempDir.path() / "b.cpp", "int b = 2;\n");

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    CancellationToken cancel;
    cancel.requestCancellation();

    const auto inspectRes = service.inspectDirectory(tempDir.path(), {}, &cancel);
    CHECK(inspectRes.status == FormatExecutionStatus::Cancelled);
    CHECK(inspectRes.remediationHint ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::Cancelled));

    const auto formatRes = service.formatFile(tempDir.path() / "a.cpp", {}, &cancel);
    CHECK(formatRes.status == FormatExecutionStatus::Cancelled);
}

// TEST-FORMAT-SRV-009: test_format_service_quota_rejection
void test_format_service_quota_rejection() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "too_large.cpp";
    std::string largeContent(2048, 'x');
    writeFile(filePath, largeContent);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    FormatServiceOptions options;
    options.maxFileSizeBytes = 512;

    const auto res = service.formatFile(filePath, options);
    CHECK(res.status == FormatExecutionStatus::FileTooLarge);
    CHECK(res.remediationHint ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::FileTooLarge));
}

// TEST-FORMAT-SRV-010: test_format_service_unsafe_path
void test_format_service_unsafe_path() {
    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());

    const auto insp = service.inspectFile("../../etc/passwd");
    CHECK(!insp.isCompliant);
    CHECK(insp.discrepancySummary ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::UnsafePath));

    const auto res = service.formatFile("../escape/source.cpp");
    CHECK(res.status == FormatExecutionStatus::UnsafePath);
    CHECK(res.remediationHint ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::UnsafePath));
}

// TEST-FORMAT-SRV-011: test_format_service_cli_synthesis
void test_format_service_cli_synthesis() {
    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());

    const std::string dryRunCmd =
        service.buildClangFormatCommandLine("apps/vigilio-browser/src/main.cpp", true, false);
    CHECK(dryRunCmd.find("--dry-run") != std::string::npos);
    CHECK(dryRunCmd.find("--Werror") != std::string::npos);
    CHECK(dryRunCmd.find("--style=file") != std::string::npos);
    CHECK(dryRunCmd.find("apps/vigilio-browser/src/main.cpp") != std::string::npos);

    const std::string inPlaceCmd =
        service.buildClangFormatCommandLine("apps/vigilio-browser/src/main.cpp", false, true);
    CHECK(inPlaceCmd.find("-i") != std::string::npos);
    CHECK(inPlaceCmd.find("--style=file") != std::string::npos);
}

// TEST-FORMAT-SRV-012: test_format_service_public_copy_compliance
void test_format_service_public_copy_compliance() {
    const std::vector<std::string> forbiddenTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                     "DAG",         "OpenSpec",    "toolchain"};

    const std::vector<FormatExecutionStatus> allStatuses = {FormatExecutionStatus::Success,
                                                            FormatExecutionStatus::DiscrepancyFound,
                                                            FormatExecutionStatus::Cancelled,
                                                            FormatExecutionStatus::FileLocked,
                                                            FormatExecutionStatus::PermissionDenied,
                                                            FormatExecutionStatus::ReadError,
                                                            FormatExecutionStatus::WriteError,
                                                            FormatExecutionStatus::FileTooLarge,
                                                            FormatExecutionStatus::UnsafePath,
                                                            FormatExecutionStatus::FileNotFound,
                                                            FormatExecutionStatus::ConfigurationError,
                                                            FormatExecutionStatus::ToolUnavailable};

    for (const auto st : allStatuses) {
        const std::string text = ClangFormatService::executionStatusToString(st);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }
}

// TEST-FORMAT-SRV-013: test_format_service_in_memory_dialects (TASK-005)
void test_format_service_in_memory_dialects() {
    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto canonical = ClangFormatConfiguration::canonicalConfig();

    const std::vector<std::string> dialects = {"snippet.cpp", "snippet.h", "snippet.hpp", "snippet.cc",
                                               "snippet.cxx"};

    const std::string unformatted = "int compute(int *ptr){\n"
                                    "int val=*ptr;\n"
                                    "return val*2;\n"
                                    "}\n";

    for (const auto& dialect : dialects) {
        const std::string formatted = service.formatSourceCode(unformatted, canonical);
        CHECK(!formatted.empty());
        CHECK(formatted.find("int compute(int* ptr) {") != std::string::npos);
        CHECK(formatted.find("    int val = *ptr;") != std::string::npos);
        CHECK(formatted.find("    return val * 2;") != std::string::npos);

        const auto unformattedInsp = service.inspectSource(unformatted, dialect);
        CHECK(!unformattedInsp.isCompliant);
        CHECK(!unformattedInsp.discrepancySummary.empty());

        const auto formattedInsp = service.inspectSource(formatted, dialect);
        CHECK(formattedInsp.isCompliant);
        CHECK(formattedInsp.discrepancySummary.empty());
    }

    // Prohibición de funciones cortas en una sola línea (AllowShortFunctionsOnASingleLine: None)
    const std::string singleLineFunc = "int getValue() { return 42; }\n";
    const std::string expanded = service.formatSourceCode(singleLineFunc, canonical);
    CHECK(expanded.find("int getValue() {\n    return 42;\n}") != std::string::npos);
}

// TEST-FORMAT-SRV-014: test_format_service_recursive_directory_tree (TASK-006)
void test_format_service_recursive_directory_tree() {
    TempDirGuard tempDir;
    const auto root = tempDir.path();

    const auto nestedDir = root / "a" / "b" / "c";
    std::filesystem::create_directories(nestedDir);

    const auto buildDir = root / "_build";
    std::filesystem::create_directories(buildDir);

    const auto gitDir = root / ".git";
    std::filesystem::create_directories(gitDir);

    const std::string compliantCode = "int main() {\n    return 0;\n}\n";
    const std::string dirtyCode = "int main(){\nreturn 0;\n}\n";

    writeFile(nestedDir / "compliant.cpp", compliantCode);
    writeFile(nestedDir / "dirty.hpp", dirtyCode);
    writeFile(root / "a" / "other_dirty.cc", dirtyCode);
    writeFile(root / "a" / "notes.txt", "Plain text notes.");
    writeFile(nestedDir / "config.json", "{\"valid\": true}");
    writeFile(buildDir / "artifact.cpp", dirtyCode);
    writeFile(gitDir / "hook.cpp", dirtyCode);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto res = service.inspectDirectory(root);

    // Archivos C++ válidos examinados: compliant.cpp, dirty.hpp, other_dirty.cc (3)
    // Carpetas ignoradas: _build, .git
    // Extensiones no C++ ignoradas: .txt, .json
    CHECK(res.filesExamined == 3);
    CHECK(res.discrepanciesCount == 2);
    CHECK(res.status == FormatExecutionStatus::DiscrepancyFound);

    // Archivos en disco permanecen inmutables (dry-run)
    CHECK(readFile(nestedDir / "compliant.cpp") == compliantCode);
    CHECK(readFile(nestedDir / "dirty.hpp") == dirtyCode);
    CHECK(readFile(root / "a" / "other_dirty.cc") == dirtyCode);
}

// TEST-FORMAT-SRV-015: test_format_service_backup_atomic_rollback (TASK-007)
void test_format_service_backup_atomic_rollback() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "atomic_rollback_test.cpp";
    const std::string original = "// Canonical Source Code\n"
                                 "int multiply(int a, int b) {\n"
                                 "    return a * b;\n"
                                 "}\n";
    writeFile(filePath, original);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());

    // 1. Respaldo atómico
    std::string backupPath;
    CHECK(service.createBackup(filePath, &backupPath));
    CHECK(std::filesystem::exists(backupPath));

    // 2. Mutación destructiva deliberada
    writeFile(filePath, "// MUTATED TRUNCATED CONTENT\n");
    CHECK(readFile(filePath) != original);

    // 3. Rollback determinista
    CHECK(service.rollbackFile(filePath));
    CHECK(readFile(filePath) == original);

    // 4. Idempotencia ante llamadas redundantes
    CHECK(service.rollbackFile(filePath));
    CHECK(readFile(filePath) == original);

    // 5. Eliminación de respaldo y prueba de rollback sobre archivo sin copia
    CHECK(service.removeBackup(filePath));
    CHECK(!std::filesystem::exists(backupPath));
    CHECK(!service.rollbackFile(filePath));

    // 6. Rollback sobre archivo inexistente
    CHECK(!service.rollbackFile(tempDir.path() / "non_existent.cpp"));
}

// TEST-FORMAT-SRV-016: test_format_service_massive_multithreading_concurrency (TASK-008)
void test_format_service_massive_multithreading_concurrency() {
    const auto canonical = ClangFormatConfiguration::canonicalConfig();
    ClangFormatService sharedService(canonical);

    const int numThreads = 16;
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    std::atomic<bool> allPassed{true};

    for (int i = 0; i < numThreads; ++i) {
        workers.emplace_back([&sharedService, &canonical, &allPassed, i]() {
            try {
                for (int iter = 0; iter < 2; ++iter) {
                    if (i % 2 == 0) {
                        const std::string unformatted = "int worker" + std::to_string(i) +
                                                        "(int *x){\nint y=" + std::to_string(iter) +
                                                        ";\nreturn y;\n}\n";
                        const std::string formatted =
                            sharedService.formatSourceCode(unformatted, canonical);
                        if (formatted.find("int worker" + std::to_string(i) + "(int* x) {") ==
                                std::string::npos ||
                            formatted.find("    int y = " + std::to_string(iter) + ";") ==
                                std::string::npos) {
                            allPassed = false;
                        }
                    } else {
                        const std::string pristine =
                            "void inspect" + std::to_string(i) + "() {\n    return;\n}\n";
                        const auto insp = sharedService.inspectSource(
                            pristine, "worker_" + std::to_string(i) + ".cpp");
                        if (!insp.isCompliant) {
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

// TEST-FORMAT-SRV-017: test_format_service_cooperative_cancellation_stress (TASK-009)
void test_format_service_cooperative_cancellation_stress() {
    TempDirGuard tempDir;
    for (int i = 0; i < 20; ++i) {
        writeFile(tempDir.path() / ("cancel_file_" + std::to_string(i) + ".cpp"),
                  "int func" + std::to_string(i) + "(){\nreturn " + std::to_string(i) + ";\n}\n");
    }

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
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

    // Cancelación inmediata síncrona
    CancellationToken immediateCancel;
    immediateCancel.requestCancellation();
    const auto cancelRes = service.inspectDirectory(tempDir.path(), {}, &immediateCancel);
    CHECK(cancelRes.status == FormatExecutionStatus::Cancelled);
    CHECK(cancelRes.remediationHint ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::Cancelled));
}

// TEST-FORMAT-SRV-018: test_format_service_transient_retry_backoff (TASK-010)
void test_format_service_transient_retry_backoff() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "transient_retry.cpp";
    const std::string dirty = "int x=42;\n";
    writeFile(filePath, dirty);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    FormatServiceOptions options;
    options.maxRetries = 3;
    options.retryBackoffMs = 20;

    // Quitar permisos de escritura inicialmente
    std::filesystem::permissions(filePath, std::filesystem::perms::owner_read);

    // Hilo secundario que restaura permisos tras un breve retraso
    std::thread unlocker([filePath]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        std::error_code ec;
        std::filesystem::permissions(
            filePath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);
    });

    const auto res = service.formatWithRetry(filePath, options);
    if (unlocker.joinable()) {
        unlocker.join();
    }

    // Asegurar permisos para limpieza
    std::error_code ec;
    std::filesystem::permissions(
        filePath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);

    CHECK(res.status == FormatExecutionStatus::Success);
    CHECK(res.filesFormatted == 1);
}

// TEST-FORMAT-SRV-019: test_format_service_permission_failure_injection (TASK-011)
void test_format_service_permission_failure_injection() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "readonly_file.cpp";
    const std::string original = "int val=100;\n";
    writeFile(filePath, original);

    // Remover permisos de escritura
    std::filesystem::permissions(filePath, std::filesystem::perms::owner_read);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    const auto res = service.formatFile(filePath);

    // Debe retornar WriteError o PermissionDenied limpiamente
    CHECK(res.status == FormatExecutionStatus::WriteError ||
          res.status == FormatExecutionStatus::PermissionDenied);

    // Restaurar permisos para verificación y limpieza
    std::filesystem::permissions(filePath, std::filesystem::perms::owner_read |
                                               std::filesystem::perms::owner_write);

    // Comprobar que el archivo original se mantiene íntegro sin corrupción
    CHECK(readFile(filePath) == original);
}

// TEST-FORMAT-SRV-020: test_format_service_quota_limit_enforcement (TASK-012)
void test_format_service_quota_limit_enforcement() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "oversized.cpp";
    std::string largeContent(2048, 'x');
    writeFile(filePath, largeContent);

    ClangFormatService service(ClangFormatConfiguration::canonicalConfig());
    FormatServiceOptions options;
    options.maxFileSizeBytes = 512;

    const auto res = service.formatFile(filePath, options);
    CHECK(res.status == FormatExecutionStatus::FileTooLarge);
    CHECK(res.remediationHint ==
          ClangFormatService::executionStatusToString(FormatExecutionStatus::FileTooLarge));
    CHECK(res.filesFormatted == 0);
}

} // namespace

int main() {
    try {
        test_format_service_initialization();
        test_format_service_in_memory();
        test_format_service_dry_run();
        test_format_service_dry_run_discrepancy();
        test_format_service_physical_format();
        test_format_service_backup_and_rollback();
        test_format_service_retry_backoff();
        test_format_service_cancellation();
        test_format_service_quota_rejection();
        test_format_service_unsafe_path();
        test_format_service_cli_synthesis();
        test_format_service_public_copy_compliance();
        test_format_service_in_memory_dialects();
        test_format_service_recursive_directory_tree();
        test_format_service_backup_atomic_rollback();
        test_format_service_massive_multithreading_concurrency();
        test_format_service_cooperative_cancellation_stress();
        test_format_service_transient_retry_backoff();
        test_format_service_permission_failure_injection();
        test_format_service_quota_limit_enforcement();

        std::cout << "ClangFormatServiceTests: ALL 20 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ClangFormatServiceTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
