#include "core/ClangTidyConfiguration.h"
#include "core/ClangTidyService.h"
#include "core/ClangTidyTypes.h"
#include "core/ClangTidyValidator.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

// TEST-TIDY-SRV-007: test_tidy_service_retry_backoff
void test_tidy_service_retry_backoff() {
    TempDirGuard tempDir;
    const auto filePath = tempDir.path() / "retry_test.cpp";
    const std::string code = "void test() {\n    void* p = NULL;\n}\n";
    writeFile(filePath, code);

    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());
    TidyServiceOptions options;
    options.maxRetries = 2;
    options.retryBackoffMs = 10;

    const auto res = service.applyFixesWithRetry(filePath, options);
    CHECK(res.status == TidyExecutionStatus::Success);
    CHECK(res.filesFixed == 1);
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

// TEST-TIDY-SRV-011: test_tidy_service_cli_synthesis
void test_tidy_service_cli_synthesis() {
    ClangTidyService service(ClangTidyConfiguration::canonicalConfig());

    const std::string cmd =
        service.buildClangTidyCommandLine("apps/vigilio-browser/src/main.cpp", "_build", true, true);
    CHECK(cmd.find("clang-tidy") != std::string::npos);
    CHECK(cmd.find("--checks=") != std::string::npos);
    CHECK(cmd.find("-fix") != std::string::npos);
    CHECK(cmd.find("-warnings-as-errors=") != std::string::npos);
    CHECK(cmd.find("-p _build") != std::string::npos);
    CHECK(cmd.find("apps/vigilio-browser/src/main.cpp") != std::string::npos);
}

// TEST-TIDY-SRV-012: test_tidy_service_public_copy_compliance
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

} // namespace

int main() {
    std::cout << "[ClangTidyServiceTests] Running 12 unit tests..." << std::endl;

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

        std::cout << "[ClangTidyServiceTests] All 12 tests PASSED successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
        return 1;
    }
}
