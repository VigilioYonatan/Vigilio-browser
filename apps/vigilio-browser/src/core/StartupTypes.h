#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Fases discretas del ciclo de vida de arranque (Startup Bootstrap).
enum class StartupPhase : uint8_t {
    Uninitialized,
    EnvironmentInspection,
    CommandLineParsing,
    DependencyVerification,
    ResourceVerification,
    StorageAndProfileResolution,
    ApplicationConfiguration,
    Ready,
    DegradedHeadless,
    Aborted,
    Failed
};

/// @brief Modos operativos admitidos durante el arranque.
enum class StartupMode : uint8_t { StandardGui, Headless, SafeMode, Incognito };

/// @brief Códigos de estado y resultado tipados del subsistema de arranque.
enum class StartupStatusCode : uint8_t {
    Success = 0,
    DegradedHeadless = 1,
    UserHelpOrVersionRequested = 2,
    InvalidCommandLineArgs = 10,
    DisplayServerMissing = 11,
    IncompatibleQtVersion = 12,
    IncompatibleWebEngine = 13,
    CorruptCriticalResources = 14,
    StorageAccessDenied = 15,
    FatalInitializationError = 20
};

/// @brief Representación estructurada de los argumentos de línea de comandos procesados.
struct StartupCommandLine {
    StartupMode mode{StartupMode::StandardGui};
    std::string profileDirectory{};
    std::vector<std::string> initialUrls{};
    bool isHeadless{false};
    bool isSafeMode{false};
    bool isIncognito{false};
    bool helpRequested{false};
    bool versionRequested{false};
    std::vector<std::string> rejectedArguments{};
    std::string parseErrorMessage{};
    bool isValid{true};
};

/// @brief Reporte consolidado e inmutable del diagnóstico del proceso de arranque.
struct StartupDiagnosticReport {
    StartupStatusCode status{StartupStatusCode::Success};
    StartupPhase failedPhase{StartupPhase::Ready};
    StartupMode effectiveMode{StartupMode::StandardGui};
    int exitCode{0};
    std::string storagePath{};
    bool isStoragePersistent{true};
    std::string diagnosticMessage{};
    std::string remediationAction{};
    uint64_t elapsedMicroseconds{0};

    [[nodiscard]] constexpr bool isSuccess() const noexcept {
        return status == StartupStatusCode::Success || status == StartupStatusCode::DegradedHeadless;
    }
};

} // namespace vigilio
