#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatService.h"
#include "core/ClangFormatTypes.h"
#include "core/ClangFormatValidator.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
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

        std::cout << "ClangFormatServiceTests: ALL 12 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ClangFormatServiceTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
