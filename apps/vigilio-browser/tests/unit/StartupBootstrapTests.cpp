#include "core/StartupBootstrapCoordinator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error(std::string("CHECK failed at line ") + std::to_string(__LINE__) +  \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

void test_bootstrap_normal_gui() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine(
        {"https://vigilio.local/home", "https://example.com/test"});
    CHECK(cmd.isValid);
    CHECK(cmd.mode == StartupMode::StandardGui);
    CHECK(!cmd.isHeadless);
    CHECK(!cmd.isSafeMode);
    CHECK(!cmd.isIncognito);
    CHECK(cmd.initialUrls.size() == 2);
    CHECK(cmd.initialUrls[0] == "https://vigilio.local/home");
    CHECK(cmd.initialUrls[1] == "https://example.com/test");

    const std::filesystem::path tempDir =
        std::filesystem::temp_directory_path() / "vigilio_test_bootstrap_normal_gui";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    const auto report =
        StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr, tempDir.string().c_str());
    CHECK(report.isSuccess());
    CHECK(report.status == StartupStatusCode::Success);
    CHECK(report.failedPhase == StartupPhase::Ready);
    CHECK(report.effectiveMode == StartupMode::StandardGui);
    CHECK(report.isStoragePersistent == true);
    CHECK(report.exitCode == 0);
    CHECK(report.storagePath == tempDir.string());
    CHECK(report.elapsedMicroseconds < 50000);

    std::filesystem::remove_all(tempDir, ec);
}

void test_bootstrap_headless_explicit() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--headless"});
    CHECK(cmd.isValid);
    CHECK(cmd.isHeadless);
    CHECK(cmd.mode == StartupMode::Headless);

    const std::filesystem::path tempDir =
        std::filesystem::temp_directory_path() / "vigilio_test_bootstrap_headless";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    // Headless explicitly should succeed even with null display and wayland
    const auto report =
        StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr, tempDir.string().c_str());
    CHECK(report.isSuccess());
    CHECK(report.status == StartupStatusCode::Success);
    CHECK(report.effectiveMode == StartupMode::Headless);
    CHECK(report.exitCode == 0);
    CHECK(report.elapsedMicroseconds < 50000);

    std::filesystem::remove_all(tempDir, ec);
}

void test_bootstrap_degraded_headless_missing_display() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({});
    CHECK(!cmd.isHeadless);
    CHECK(cmd.mode == StartupMode::StandardGui);

    // Missing display and wayland without --headless triggers DegradedHeadless
    const auto report = StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr);
    CHECK(report.isSuccess()); // isSuccess returns true for DegradedHeadless
    CHECK(report.status == StartupStatusCode::DegradedHeadless);
    CHECK(report.failedPhase == StartupPhase::DegradedHeadless);
    CHECK(report.effectiveMode == StartupMode::Headless);
    CHECK(report.exitCode == 1);
    CHECK(report.diagnosticMessage.find("No graphical display server found") != std::string::npos);
    CHECK(!report.remediationAction.empty());
    CHECK(report.elapsedMicroseconds < 50000);
}

void test_bootstrap_incognito_volatile_storage() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--incognito"});
    CHECK(cmd.isValid);
    CHECK(cmd.isIncognito);
    CHECK(cmd.mode == StartupMode::Incognito);

    const std::filesystem::path tempDir =
        std::filesystem::temp_directory_path() / "vigilio_test_bootstrap_incognito";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    const auto report =
        StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr, tempDir.string().c_str());
    CHECK(report.isSuccess());
    CHECK(report.status == StartupStatusCode::Success);
    CHECK(report.effectiveMode == StartupMode::Incognito);
    CHECK(report.isStoragePersistent == false); // Invariant!
    CHECK(report.storagePath == ":memory:");
    CHECK(report.exitCode == 0);
    CHECK(report.elapsedMicroseconds < 50000);

    // Directory must NOT have been created on disk for incognito mode
    CHECK(!std::filesystem::exists(tempDir));
}

void test_bootstrap_safe_mode() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--safe-mode"});
    CHECK(cmd.isValid);
    CHECK(cmd.isSafeMode);
    CHECK(cmd.mode == StartupMode::SafeMode);

    const std::filesystem::path tempDir =
        std::filesystem::temp_directory_path() / "vigilio_test_bootstrap_safe_mode";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    const auto report =
        StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr, tempDir.string().c_str());
    CHECK(report.isSuccess());
    CHECK(report.status == StartupStatusCode::Success);
    CHECK(report.effectiveMode == StartupMode::SafeMode);
    CHECK(report.exitCode == 0);
    CHECK(report.elapsedMicroseconds < 50000);

    std::filesystem::remove_all(tempDir, ec);
}

void test_bootstrap_profile_dir_variations() {
    std::error_code ec;

    // Two-token syntax: --profile-dir <path>
    const std::filesystem::path tempDir1 =
        std::filesystem::temp_directory_path() / "vigilio_test_profile_dir_twotokens";
    std::filesystem::remove_all(tempDir1, ec);

    const auto cmd1 =
        StartupBootstrapCoordinator::parseCommandLine({"--profile-dir", tempDir1.string()});
    CHECK(cmd1.isValid);
    CHECK(cmd1.profileDirectory == tempDir1.string());

    const auto report1 = StartupBootstrapCoordinator::executeBootstrap(cmd1, ":0", nullptr);
    CHECK(report1.isSuccess());
    CHECK(report1.status == StartupStatusCode::Success);
    CHECK(report1.storagePath == tempDir1.string());
    CHECK(report1.isStoragePersistent == true);
    CHECK(std::filesystem::exists(tempDir1));
    CHECK(report1.elapsedMicroseconds < 50000);
    std::filesystem::remove_all(tempDir1, ec);

    // Single-token assignment syntax: --profile-dir=<path>
    const std::filesystem::path tempDir2 =
        std::filesystem::temp_directory_path() / "vigilio_test_profile_dir_singaltoken";
    std::filesystem::remove_all(tempDir2, ec);

    const std::string arg2 = "--profile-dir=" + tempDir2.string();
    const auto cmd2 = StartupBootstrapCoordinator::parseCommandLine({arg2});
    CHECK(cmd2.isValid);
    CHECK(cmd2.profileDirectory == tempDir2.string());

    const auto report2 = StartupBootstrapCoordinator::executeBootstrap(cmd2, ":0", nullptr);
    CHECK(report2.isSuccess());
    CHECK(report2.status == StartupStatusCode::Success);
    CHECK(report2.storagePath == tempDir2.string());
    CHECK(report2.isStoragePersistent == true);
    CHECK(std::filesystem::exists(tempDir2));
    CHECK(report2.elapsedMicroseconds < 50000);
    std::filesystem::remove_all(tempDir2, ec);

    // Valid relative directory path
    const auto cmdRel =
        StartupBootstrapCoordinator::parseCommandLine({"--profile-dir", "profiles/custom"});
    CHECK(cmdRel.isValid);
    CHECK(cmdRel.profileDirectory == "profiles/custom");

    // Missing path argument after --profile-dir
    const auto cmdMissing = StartupBootstrapCoordinator::parseCommandLine({"--profile-dir"});
    CHECK(!cmdMissing.isValid);
    CHECK(!cmdMissing.rejectedArguments.empty());
    CHECK(cmdMissing.rejectedArguments[0] == "--profile-dir");

    // Next argument is another option rather than a path
    const auto cmdFollowedByOption =
        StartupBootstrapCoordinator::parseCommandLine({"--profile-dir", "--headless"});
    CHECK(!cmdFollowedByOption.isValid);
    CHECK(!cmdFollowedByOption.rejectedArguments.empty());
    CHECK(cmdFollowedByOption.rejectedArguments[0] == "--profile-dir");
}

void test_bootstrap_forbidden_arguments() {
    const std::vector<std::string_view> forbidden = {"--no-sandbox",
                                                     "--disable-web-security",
                                                     "--remote-debugging-port",
                                                     "--remote-debugging-port=9222",
                                                     "--remote-debugging-pipe",
                                                     "--allow-file-access-from-files"};

    for (const auto flag : forbidden) {
        CHECK(StartupBootstrapCoordinator::isArgumentForbidden(flag));
        CHECK(!StartupBootstrapCoordinator::isArgumentAllowed(flag));

        const auto cmd = StartupBootstrapCoordinator::parseCommandLine({flag});
        CHECK(!cmd.isValid);
        CHECK(!cmd.rejectedArguments.empty());
        CHECK(cmd.rejectedArguments[0] == flag);

        const auto report = StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr);
        CHECK(!report.isSuccess());
        CHECK(report.status == StartupStatusCode::InvalidCommandLineArgs);
        CHECK(report.failedPhase == StartupPhase::CommandLineParsing);
        CHECK(report.exitCode == 10);
    }

    // Unrecognized options
    const std::vector<std::string_view> unrecognized = {"--unsupported-flag",
                                                        "--enable-features=NetworkService", "--flag"};
    for (const auto opt : unrecognized) {
        const auto cmdUnknown = StartupBootstrapCoordinator::parseCommandLine({opt});
        CHECK(!cmdUnknown.isValid);
        CHECK(!cmdUnknown.rejectedArguments.empty());
        CHECK(cmdUnknown.rejectedArguments[0] == opt);

        const auto report = StartupBootstrapCoordinator::executeBootstrap(cmdUnknown, ":0", nullptr);
        CHECK(!report.isSuccess());
        CHECK(report.status == StartupStatusCode::InvalidCommandLineArgs);
        CHECK(report.exitCode == 10);
    }
}

void test_bootstrap_directory_traversal_defense() {
    // Check isValidProfilePath directly
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath(""));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("../profile"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("foo/../../bar"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath(".."));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("sub/.."));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("..\\system32"));

    // Excessive length (> 4096)
    const std::string longPath(4097, 'a');
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath(longPath));

    // Prohibited system root directories
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/etc"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/etc/shadow"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/boot"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/usr"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/var"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/bin"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/proc"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/sys"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/dev"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("/root"));

    // Prohibited Windows system directories and UNC paths
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("\\\\server\\share"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("C:\\Windows"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("C:\\Windows\\System32"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("c:/windows/system32"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("C:\\Program Files"));

    // Control characters and injection
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("profile\nname"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("profile;rm -rf /"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("profile|cat"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("profile&echo"));
    CHECK(!StartupBootstrapCoordinator::isValidProfilePath("profile$var"));

    // Valid paths
    CHECK(StartupBootstrapCoordinator::isValidProfilePath("/tmp/my_profile"));
    CHECK(StartupBootstrapCoordinator::isValidProfilePath("/home/user/.vigilio"));
    CHECK(StartupBootstrapCoordinator::isValidProfilePath("profiles/profile1"));
    CHECK(StartupBootstrapCoordinator::isValidProfilePath("custom_profile_dir"));

    // Parse command line with Directory Traversal in --profile-dir
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--profile-dir", "../evil_profile"});
    CHECK(!cmd.isValid);
    const auto report = StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr);
    CHECK(!report.isSuccess());
    CHECK(report.status == StartupStatusCode::InvalidCommandLineArgs);
    CHECK(report.exitCode == 10);
}

void test_bootstrap_help_and_version_requests() {
    // --help
    const auto cmdHelp = StartupBootstrapCoordinator::parseCommandLine({"--help"});
    CHECK(cmdHelp.helpRequested);
    const auto reportHelp = StartupBootstrapCoordinator::executeBootstrap(cmdHelp, ":0", nullptr);
    CHECK(!reportHelp.isSuccess());
    CHECK(reportHelp.status == StartupStatusCode::UserHelpOrVersionRequested);
    CHECK(reportHelp.failedPhase == StartupPhase::Aborted);
    CHECK(reportHelp.exitCode == 0);

    // -h
    const auto cmdH = StartupBootstrapCoordinator::parseCommandLine({"-h"});
    CHECK(cmdH.helpRequested);

    // --version
    const auto cmdVersion = StartupBootstrapCoordinator::parseCommandLine({"--version"});
    CHECK(cmdVersion.versionRequested);
    const auto reportVersion = StartupBootstrapCoordinator::executeBootstrap(cmdVersion, ":0", nullptr);
    CHECK(!reportVersion.isSuccess());
    CHECK(reportVersion.status == StartupStatusCode::UserHelpOrVersionRequested);
    CHECK(reportVersion.failedPhase == StartupPhase::Aborted);
    CHECK(reportVersion.exitCode == 0);

    // -v
    const auto cmdV = StartupBootstrapCoordinator::parseCommandLine({"-v"});
    CHECK(cmdV.versionRequested);
}

void test_bootstrap_incompatible_qt_simulation() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--headless"});

    // Simulation 1: Qt 5.15.2
    {
        const auto report =
            StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr, nullptr, "5.15.2");
        CHECK(!report.isSuccess());
        CHECK(report.status == StartupStatusCode::IncompatibleQtVersion);
        CHECK(report.failedPhase == StartupPhase::DependencyVerification);
        CHECK(report.exitCode == 12);
        CHECK(report.diagnosticMessage.find("Incompatible Qt runtime version") != std::string::npos);
        CHECK(!report.remediationAction.empty());
    }

    // Simulation 2: Qt 6.9.3 (< 6.10.0 baseline)
    {
        const auto report =
            StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr, nullptr, "6.9.3");
        CHECK(!report.isSuccess());
        CHECK(report.status == StartupStatusCode::IncompatibleQtVersion);
        CHECK(report.failedPhase == StartupPhase::DependencyVerification);
        CHECK(report.exitCode == 12);
        CHECK(!report.remediationAction.empty());
    }
}

void test_bootstrap_corrupt_resources_simulation() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--headless"});
    const char* corruptQrc = "<RCC><qresource prefix=\"/invalid\"></qresource></RCC>";
    const auto report = StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr, nullptr,
                                                                      nullptr, corruptQrc);
    CHECK(!report.isSuccess());
    CHECK(report.status == StartupStatusCode::CorruptCriticalResources);
    CHECK(report.failedPhase == StartupPhase::ResourceVerification);
    CHECK(report.exitCode == 14);
    CHECK(!report.remediationAction.empty());
}

void test_bootstrap_storage_access_denied() {
    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--headless"});
    const auto report = StartupBootstrapCoordinator::executeBootstrap(
        cmd, nullptr, nullptr, "/proc/vigilio_forbidden_dir_test");
    CHECK(!report.isSuccess());
    CHECK(report.status == StartupStatusCode::StorageAccessDenied);
    CHECK(report.failedPhase == StartupPhase::StorageAndProfileResolution);
    CHECK(report.exitCode == 15);
    CHECK(!report.remediationAction.empty());
}

void test_bootstrap_concurrency_stress() {
    constexpr size_t kNumThreads = 16;
    constexpr size_t kIterationsPerThread = 500;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<size_t> successCount{0};

    const auto startTime = std::chrono::steady_clock::now();

    for (size_t t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([t, &successCount]() {
            for (size_t iter = 0; iter < kIterationsPerThread; ++iter) {
                // Alternate different startup permutations
                if ((t + iter) % 4 == 0) {
                    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--headless"});
                    const auto report =
                        StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr);
                    if (report.isSuccess() && report.status == StartupStatusCode::Success) {
                        successCount.fetch_add(1, std::memory_order_relaxed);
                    }
                } else if ((t + iter) % 4 == 1) {
                    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--incognito"});
                    const auto report =
                        StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr);
                    if (report.isSuccess() && !report.isStoragePersistent) {
                        successCount.fetch_add(1, std::memory_order_relaxed);
                    }
                } else if ((t + iter) % 4 == 2) {
                    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--safe-mode"});
                    const auto report =
                        StartupBootstrapCoordinator::executeBootstrap(cmd, nullptr, nullptr);
                    if (report.status == StartupStatusCode::DegradedHeadless) {
                        successCount.fetch_add(1, std::memory_order_relaxed);
                    }
                } else {
                    const auto cmd = StartupBootstrapCoordinator::parseCommandLine({"--no-sandbox"});
                    const auto report =
                        StartupBootstrapCoordinator::executeBootstrap(cmd, ":0", nullptr);
                    if (!report.isSuccess() &&
                        report.status == StartupStatusCode::InvalidCommandLineArgs) {
                        successCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto totalDurationMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    CHECK(successCount.load() == kNumThreads * kIterationsPerThread);
    // Performance check: total time for 8,000 operations should be well within bounds
    std::cout << "[Concurrency Stress] " << successCount.load()
              << " bootstrap evaluations completed across 16 threads in " << totalDurationMs << " ms\n";
}

void test_bootstrap_string_conversions() {
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::Uninitialized) == "Uninitialized");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::EnvironmentInspection) ==
          "EnvironmentInspection");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::CommandLineParsing) ==
          "CommandLineParsing");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::DependencyVerification) ==
          "DependencyVerification");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::ResourceVerification) ==
          "ResourceVerification");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::StorageAndProfileResolution) ==
          "StorageAndProfileResolution");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::ApplicationConfiguration) ==
          "ApplicationConfiguration");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::Ready) == "Ready");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::DegradedHeadless) ==
          "DegradedHeadless");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::Aborted) == "Aborted");
    CHECK(StartupBootstrapCoordinator::phaseToString(StartupPhase::Failed) == "Failed");

    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::Success) == "Success");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::DegradedHeadless) ==
          "DegradedHeadless");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::UserHelpOrVersionRequested) ==
          "UserHelpOrVersionRequested");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::InvalidCommandLineArgs) ==
          "InvalidCommandLineArgs");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::DisplayServerMissing) ==
          "DisplayServerMissing");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::IncompatibleQtVersion) ==
          "IncompatibleQtVersion");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::IncompatibleWebEngine) ==
          "IncompatibleWebEngine");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::CorruptCriticalResources) ==
          "CorruptCriticalResources");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::StorageAccessDenied) ==
          "StorageAccessDenied");
    CHECK(StartupBootstrapCoordinator::statusToString(StartupStatusCode::FatalInitializationError) ==
          "FatalInitializationError");

    CHECK(StartupBootstrapCoordinator::modeToString(StartupMode::StandardGui) == "StandardGui");
    CHECK(StartupBootstrapCoordinator::modeToString(StartupMode::Headless) == "Headless");
    CHECK(StartupBootstrapCoordinator::modeToString(StartupMode::SafeMode) == "SafeMode");
    CHECK(StartupBootstrapCoordinator::modeToString(StartupMode::Incognito) == "Incognito");
}

} // namespace

int main() {
    try {
        test_bootstrap_normal_gui();
        test_bootstrap_headless_explicit();
        test_bootstrap_degraded_headless_missing_display();
        test_bootstrap_incognito_volatile_storage();
        test_bootstrap_safe_mode();
        test_bootstrap_profile_dir_variations();
        test_bootstrap_forbidden_arguments();
        test_bootstrap_directory_traversal_defense();
        test_bootstrap_help_and_version_requests();
        test_bootstrap_incompatible_qt_simulation();
        test_bootstrap_corrupt_resources_simulation();
        test_bootstrap_storage_access_denied();
        test_bootstrap_concurrency_stress();
        test_bootstrap_string_conversions();

        std::cout << "Startup bootstrap tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
