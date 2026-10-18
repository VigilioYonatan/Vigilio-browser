#pragma once

#include "core/StartupTypes.h"

#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Coordinador estático de dominio para la secuencia integral de inicio de la aplicación.
class StartupBootstrapCoordinator {
  public:
    [[nodiscard]] static StartupCommandLine parseCommandLine(int argc, const char* const* argv) noexcept;
    [[nodiscard]] static StartupCommandLine
    parseCommandLine(const std::vector<std::string_view>& args) noexcept;

    [[nodiscard]] static StartupDiagnosticReport
    executeBootstrap(const StartupCommandLine& cmdLine, const char* displayEnv, const char* waylandEnv,
                     const char* customStorageRoot = nullptr, const char* simulatedQtVersion = nullptr,
                     const char* simulatedQrcContent = nullptr) noexcept;

    [[nodiscard]] static constexpr std::string_view phaseToString(StartupPhase phase) noexcept {
        switch (phase) {
        case StartupPhase::Uninitialized:
            return "Uninitialized";
        case StartupPhase::EnvironmentInspection:
            return "EnvironmentInspection";
        case StartupPhase::CommandLineParsing:
            return "CommandLineParsing";
        case StartupPhase::DependencyVerification:
            return "DependencyVerification";
        case StartupPhase::ResourceVerification:
            return "ResourceVerification";
        case StartupPhase::StorageAndProfileResolution:
            return "StorageAndProfileResolution";
        case StartupPhase::ApplicationConfiguration:
            return "ApplicationConfiguration";
        case StartupPhase::Ready:
            return "Ready";
        case StartupPhase::DegradedHeadless:
            return "DegradedHeadless";
        case StartupPhase::Aborted:
            return "Aborted";
        case StartupPhase::Failed:
            return "Failed";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view statusToString(StartupStatusCode status) noexcept {
        switch (status) {
        case StartupStatusCode::Success:
            return "Success";
        case StartupStatusCode::DegradedHeadless:
            return "DegradedHeadless";
        case StartupStatusCode::UserHelpOrVersionRequested:
            return "UserHelpOrVersionRequested";
        case StartupStatusCode::InvalidCommandLineArgs:
            return "InvalidCommandLineArgs";
        case StartupStatusCode::DisplayServerMissing:
            return "DisplayServerMissing";
        case StartupStatusCode::IncompatibleQtVersion:
            return "IncompatibleQtVersion";
        case StartupStatusCode::IncompatibleWebEngine:
            return "IncompatibleWebEngine";
        case StartupStatusCode::CorruptCriticalResources:
            return "CorruptCriticalResources";
        case StartupStatusCode::StorageAccessDenied:
            return "StorageAccessDenied";
        case StartupStatusCode::FatalInitializationError:
            return "FatalInitializationError";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view modeToString(StartupMode mode) noexcept {
        switch (mode) {
        case StartupMode::StandardGui:
            return "StandardGui";
        case StartupMode::Headless:
            return "Headless";
        case StartupMode::SafeMode:
            return "SafeMode";
        case StartupMode::Incognito:
            return "Incognito";
        }
        return "Unknown";
    }

    [[nodiscard]] static bool isArgumentAllowed(std::string_view arg) noexcept;
    [[nodiscard]] static bool isArgumentForbidden(std::string_view arg) noexcept;
    [[nodiscard]] static bool isValidProfilePath(std::string_view path) noexcept;
};

} // namespace vigilio
