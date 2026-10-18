#include "core/StartupBootstrapCoordinator.h"
#include "core/QtDependencyValidator.h"
#include "core/ResourceInclusionService.h"
#include "core/WebEngineVersionValidator.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

namespace {

bool stringEqualsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.length() != b.length()) {
        return false;
    }
    for (size_t i = 0; i < a.length(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool stringStartsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept {
    if (text.length() < prefix.length()) {
        return false;
    }
    return stringEqualsIgnoreCase(text.substr(0, prefix.length()), prefix);
}

} // namespace

bool StartupBootstrapCoordinator::isArgumentForbidden(std::string_view arg) noexcept {
    if (arg == "--no-sandbox" || arg == "--disable-web-security" || arg == "--remote-debugging-pipe" ||
        arg == "--allow-file-access-from-files") {
        return true;
    }
    if (arg == "--remote-debugging-port" || arg.starts_with("--remote-debugging-port=")) {
        return true;
    }
    return false;
}

bool StartupBootstrapCoordinator::isArgumentAllowed(std::string_view arg) noexcept {
    if (isArgumentForbidden(arg)) {
        return false;
    }
    if (arg == "--headless" || arg == "--safe-mode" || arg == "--incognito" || arg == "-h" ||
        arg == "--help" || arg == "-v" || arg == "--version" || arg == "--profile-dir" ||
        arg.starts_with("--profile-dir=")) {
        return true;
    }
    // URLs or positional arguments not starting with dash
    if (!arg.starts_with("-")) {
        return true;
    }
    return false;
}

bool StartupBootstrapCoordinator::isValidProfilePath(std::string_view path) noexcept {
    if (path.empty() || path.length() > 4096) {
        return false;
    }

    // Reject control characters or shell injection metacharacters
    for (const char ch : path) {
        const auto uch = static_cast<unsigned char>(ch);
        if (uch < 32 || uch == 127) {
            return false;
        }
        if (ch == '|' || ch == ';' || ch == '&' || ch == '$' || ch == '`' || ch == '<' || ch == '>' ||
            ch == '"' || ch == '\'') {
            return false;
        }
    }

    // Reject directory traversal sequences
    if (path.find("..") != std::string_view::npos) {
        return false;
    }

    // Reject prohibited system root directories on UNIX
    if (path == "/" || path == "/etc" || path.starts_with("/etc/") || path == "/boot" ||
        path.starts_with("/boot/") || path == "/usr" || path.starts_with("/usr/") || path == "/var" ||
        path.starts_with("/var/") || path == "/bin" || path.starts_with("/bin/") || path == "/sbin" ||
        path.starts_with("/sbin/") || path == "/lib" || path.starts_with("/lib/") || path == "/lib64" ||
        path.starts_with("/lib64/") || path == "/proc" || path.starts_with("/proc/") || path == "/sys" ||
        path.starts_with("/sys/") || path == "/dev" || path.starts_with("/dev/") || path == "/root" ||
        path.starts_with("/root/")) {
        return false;
    }

    // Reject Windows system directories and UNC paths
    if (path.starts_with("\\\\")) {
        return false;
    }
    if (stringStartsWithIgnoreCase(path, "C:\\Windows") ||
        stringStartsWithIgnoreCase(path, "C:/Windows") ||
        stringStartsWithIgnoreCase(path, "C:\\Program Files") ||
        stringStartsWithIgnoreCase(path, "C:/Program Files") ||
        stringStartsWithIgnoreCase(path, "C:\\System32") ||
        stringStartsWithIgnoreCase(path, "C:/System32")) {
        return false;
    }

    return true;
}

StartupCommandLine StartupBootstrapCoordinator::parseCommandLine(int argc,
                                                                 const char* const* argv) noexcept {
    std::vector<std::string_view> args;
    if (argc > 1 && argv != nullptr) {
        args.reserve(static_cast<size_t>(argc - 1));
        for (int i = 1; i < argc; ++i) {
            if (argv[i] != nullptr) {
                args.emplace_back(argv[i]);
            }
        }
    }
    return parseCommandLine(args);
}

StartupCommandLine
StartupBootstrapCoordinator::parseCommandLine(const std::vector<std::string_view>& args) noexcept {
    StartupCommandLine cmdLine;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];

        if (isArgumentForbidden(arg)) {
            cmdLine.isValid = false;
            cmdLine.rejectedArguments.emplace_back(std::string(arg));
            if (cmdLine.parseErrorMessage.empty()) {
                cmdLine.parseErrorMessage =
                    "Forbidden security-sensitive flag detected: " + std::string(arg);
            }
            continue;
        }

        if (arg == "-h" || arg == "--help") {
            cmdLine.helpRequested = true;
        } else if (arg == "-v" || arg == "--version") {
            cmdLine.versionRequested = true;
        } else if (arg == "--headless") {
            cmdLine.isHeadless = true;
            if (cmdLine.mode == StartupMode::StandardGui) {
                cmdLine.mode = StartupMode::Headless;
            }
        } else if (arg == "--safe-mode") {
            cmdLine.isSafeMode = true;
            cmdLine.mode = StartupMode::SafeMode;
        } else if (arg == "--incognito") {
            cmdLine.isIncognito = true;
            cmdLine.mode = StartupMode::Incognito;
        } else if (arg == "--profile-dir") {
            if (i + 1 < args.size() && !args[i + 1].starts_with("-")) {
                ++i;
                const std::string_view profilePath = args[i];
                if (!isValidProfilePath(profilePath)) {
                    cmdLine.isValid = false;
                    cmdLine.rejectedArguments.emplace_back(std::string(profilePath));
                    if (cmdLine.parseErrorMessage.empty()) {
                        cmdLine.parseErrorMessage =
                            "Profile directory path contains invalid characters or path traversal: " +
                            std::string(profilePath);
                    }
                } else {
                    cmdLine.profileDirectory = std::string(profilePath);
                }
            } else {
                cmdLine.isValid = false;
                cmdLine.rejectedArguments.emplace_back(std::string(arg));
                if (cmdLine.parseErrorMessage.empty()) {
                    cmdLine.parseErrorMessage = "Option --profile-dir requires a valid directory path.";
                }
            }
        } else if (arg.starts_with("--profile-dir=")) {
            const std::string_view profilePath = arg.substr(14);
            if (!isValidProfilePath(profilePath)) {
                cmdLine.isValid = false;
                cmdLine.rejectedArguments.emplace_back(std::string(profilePath));
                if (cmdLine.parseErrorMessage.empty()) {
                    cmdLine.parseErrorMessage =
                        "Profile directory path contains invalid characters or path traversal: " +
                        std::string(profilePath);
                }
            } else {
                cmdLine.profileDirectory = std::string(profilePath);
            }
        } else if (arg.starts_with("-")) {
            cmdLine.isValid = false;
            cmdLine.rejectedArguments.emplace_back(std::string(arg));
            if (cmdLine.parseErrorMessage.empty()) {
                cmdLine.parseErrorMessage = "Unrecognized command line argument: " + std::string(arg);
            }
        } else {
            cmdLine.initialUrls.emplace_back(std::string(arg));
        }
    }

    return cmdLine;
}

StartupDiagnosticReport
StartupBootstrapCoordinator::executeBootstrap(const StartupCommandLine& cmdLine, const char* displayEnv,
                                              const char* waylandEnv, const char* customStorageRoot,
                                              const char* simulatedQtVersion,
                                              const char* simulatedQrcContent) noexcept {
    const auto startTime = std::chrono::steady_clock::now();

    StartupDiagnosticReport report;
    report.status = StartupStatusCode::Success;
    report.failedPhase = StartupPhase::Ready;
    report.effectiveMode = cmdLine.mode;
    report.exitCode = 0;
    report.isStoragePersistent = !cmdLine.isIncognito;

    auto finishReport = [&](StartupStatusCode status, StartupPhase phase, int code, std::string msg,
                            std::string remediation) noexcept -> StartupDiagnosticReport {
        report.status = status;
        report.failedPhase = phase;
        report.exitCode = code;
        report.diagnosticMessage = std::move(msg);
        report.remediationAction = std::move(remediation);
        const auto endTime = std::chrono::steady_clock::now();
        report.elapsedMicroseconds = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
        return report;
    };

    // Phase 1: EnvironmentInspection
    bool isHeadless = cmdLine.isHeadless;
    if (!isHeadless) {
        const char* headlessEnv = std::getenv("VIGILIO_HEADLESS");
        if (headlessEnv != nullptr && headlessEnv[0] != '\0') {
            isHeadless = true;
        }
    }

    if (isHeadless) {
        report.effectiveMode = StartupMode::Headless;
    } else {
        const bool hasDisplay = (displayEnv != nullptr && displayEnv[0] != '\0');
        const bool hasWayland = (waylandEnv != nullptr && waylandEnv[0] != '\0');
        if (!hasDisplay && !hasWayland) {
            report.effectiveMode = StartupMode::Headless;
            return finishReport(StartupStatusCode::DegradedHeadless, StartupPhase::DegradedHeadless, 1,
                                "No graphical display server found (DISPLAY and WAYLAND_DISPLAY are "
                                "unset). Running in degraded headless mode.",
                                "Connect to an X11 or Wayland display server, or pass --headless "
                                "explicitly.");
        }
    }

    // Phase 2: CommandLineParsing
    if (!cmdLine.isValid) {
        return finishReport(
            StartupStatusCode::InvalidCommandLineArgs, StartupPhase::CommandLineParsing, 10,
            cmdLine.parseErrorMessage.empty() ? "Invalid command line arguments detected."
                                              : cmdLine.parseErrorMessage,
            "Remove unauthorized, dangerous, or malformed flags from command line.");
    }

    if (cmdLine.helpRequested || cmdLine.versionRequested) {
        return finishReport(StartupStatusCode::UserHelpOrVersionRequested, StartupPhase::Aborted, 0,
                            cmdLine.helpRequested ? "User requested help message."
                                                  : "User requested application version.",
                            "Display help/version text and terminate normally.");
    }

    // Phase 3: DependencyVerification
    const char* qtVerStr = simulatedQtVersion;
    if (qtVerStr == nullptr) {
        const char* envSim = std::getenv("VIGILIO_SIMULATE_QT_VERSION");
        if (envSim != nullptr && envSim[0] != '\0') {
            qtVerStr = envSim;
        } else {
            qtVerStr = "6.10.0";
        }
    }

    const auto parsedQtVer = SemanticVersion::parse(qtVerStr);
    if (!parsedQtVer.has_value() || !parsedQtVer->isCompatibleWith(SemanticVersion{6, 10, 0})) {
        return finishReport(
            StartupStatusCode::IncompatibleQtVersion, StartupPhase::DependencyVerification, 12,
            "Incompatible Qt runtime version: found " + std::string(qtVerStr) + ", required >= 6.10.0.",
            "Upgrade system Qt libraries to version 6.10.0 or higher.");
    }

    const auto webEngineReport =
        WebEngineVersionValidator::validateStartup(parsedQtVer, "128.0.6613.119", "", false);
    if (webEngineReport.status != WebEngineCompatibilityStatus::Compatible &&
        webEngineReport.status != WebEngineCompatibilityStatus::DegradedHeadless) {
        return finishReport(
            StartupStatusCode::IncompatibleWebEngine, StartupPhase::DependencyVerification, 13,
            webEngineReport.diagnosticMessage.empty() ? "Incompatible Chromium WebEngine version."
                                                      : webEngineReport.diagnosticMessage,
            webEngineReport.remediationAction.empty()
                ? "Ensure Qt WebEngine is updated to Chromium milestone 128 or higher."
                : webEngineReport.remediationAction);
    }

    // Phase 4: ResourceVerification
    ResourceStartupReport resourceReport;
    if (simulatedQrcContent != nullptr) {
        resourceReport = ResourceInclusionService::validateStartupResources(simulatedQrcContent);
    } else {
        resourceReport = ResourceInclusionService::validateStartupResources();
    }

    if (resourceReport.status == ResourceStartupStatus::MissingCritical ||
        resourceReport.status == ResourceStartupStatus::CatalogCorruption) {
        return finishReport(StartupStatusCode::CorruptCriticalResources,
                            StartupPhase::ResourceVerification, 14,
                            resourceReport.diagnosticMessage.empty()
                                ? "Critical application resources are corrupt or missing."
                                : resourceReport.diagnosticMessage,
                            resourceReport.remediationAction.empty()
                                ? "Verify bundle installation or reinstall application."
                                : resourceReport.remediationAction);
    }

    // Phase 5: StorageAndProfileResolution
    std::string targetStorage;
    if (!cmdLine.profileDirectory.empty()) {
        if (!isValidProfilePath(cmdLine.profileDirectory)) {
            return finishReport(StartupStatusCode::InvalidCommandLineArgs,
                                StartupPhase::StorageAndProfileResolution, 10,
                                "Profile directory path failed validation against Directory Traversal.",
                                "Provide a clean, valid profile directory path.");
        }
        targetStorage = cmdLine.profileDirectory;
    } else if (customStorageRoot != nullptr && customStorageRoot[0] != '\0') {
        targetStorage = customStorageRoot;
    } else {
        const char* home = std::getenv("HOME");
        if (home != nullptr && home[0] != '\0') {
            targetStorage = std::string(home) + "/.local/share/vigilio";
        } else {
            targetStorage = "/tmp/vigilio";
        }
    }

    if (cmdLine.isIncognito) {
        report.isStoragePersistent = false;
        report.storagePath = ":memory:";
    } else {
        report.isStoragePersistent = true;
        report.storagePath = targetStorage;

        std::error_code ec;
        std::filesystem::create_directories(targetStorage, ec);
        if (ec) {
            return finishReport(
                StartupStatusCode::StorageAccessDenied, StartupPhase::StorageAndProfileResolution, 15,
                "Cannot access or create storage directory: " + targetStorage + " (" + ec.message() +
                    ")",
                "Ensure write permissions are granted for the specified profile directory.");
        }

        const auto probePath = std::filesystem::path(targetStorage) / ".probe_write";
        std::ofstream probe(probePath);
        if (!probe.is_open()) {
            return finishReport(StartupStatusCode::StorageAccessDenied,
                                StartupPhase::StorageAndProfileResolution, 15,
                                "Storage directory is not writable: " + targetStorage,
                                "Verify filesystem permissions for the storage location.");
        }
        probe.close();
        std::filesystem::remove(probePath, ec);
    }

    // Phase 6: ApplicationConfiguration
    return finishReport(StartupStatusCode::Success, StartupPhase::Ready, 0,
                        "Startup bootstrap sequence completed successfully.", "");
}

} // namespace vigilio
