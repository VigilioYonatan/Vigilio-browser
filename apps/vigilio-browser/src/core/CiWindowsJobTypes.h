#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED
#define VIGILIO_CANCELLATION_TOKEN_DEFINED
/// @brief Token ligero para soporte de cancelación cooperativa multihilo.
struct CancellationToken {
    std::atomic<bool> cancelled{false};

    void requestCancellation() noexcept {
        cancelled.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled.load(std::memory_order_acquire);
    }
};
#endif

/// @brief Etapas canónicas y secuenciales del pipeline de integración continua en Windows.
enum class CiWindowsPipelineStep {
    EnvironmentCheck,
    FormatVerification,
    StaticAnalysis,
    ConfigurePreset,
    BuildCompilation,
    TestSuiteExecution,
    SanitizerVerification,
    ArtifactPackaging
};

/// @brief Estados de ciclo de vida del trabajo de CI en Windows.
enum class CiWindowsJobStatus { Pending, Running, Passed, Failed, Cancelled, TimedOut };

/// @brief Estados de ejecución por etapa individual.
enum class CiWindowsStepStatus { NotStarted, InProgress, Success, Failure, Omitted };

/// @brief Modos de ejecución operativa para el trabajo de CI Windows.
enum class CiWindowsExecutionMode : uint8_t { DryRun, Simulated, Live };

/// @brief Opciones operativas del servicio de CI Windows.
struct CiWindowsServiceOptions {
    CiWindowsExecutionMode mode{CiWindowsExecutionMode::DryRun};
    bool failFast{true};
    uint32_t maxRetries{3};
    uint32_t retryBackoffMs{50};
    bool captureOutput{true};

    bool operator==(const CiWindowsServiceOptions& other) const = default;
};

/// @brief Clasificación exhaustiva y fuertemente tipada de errores de CI Windows.
enum class CiWindowsJobError {
    None,
    ToolchainMissing,
    WindowsSdkMissing,
    ConfigurationFailed,
    CompilationFailed,
    TestExecutionFailed,
    FormatDiscrepancy,
    StaticAnalysisIssue,
    UnsafePipelinePath,
    TimeoutExceeded,
    InvalidWorkflowSchema,
    DependencyOrderingViolation,
    MemoryLimitExceeded
};

/// @brief Arquitecturas de CPU admitidas en el entorno Windows.
enum class CiWindowsArchitecture { X86_64, ARM64 };

/// @brief Compiladores C++20 admitidos en Windows.
enum class CiWindowsCompiler { MSVC, ClangCl, MinGW };

/// @brief Modos y configuraciones de construcción soportados.
enum class CiWindowsBuildConfiguration { Debug, Release, Sanitizer };

/// @brief Metadatos inmutables del entorno de ejecución de CI Windows.
struct CiWindowsEnvironment {
    std::string windowsVersion{"Windows 11 23H2"};
    std::string buildNumber{"22631.3007"};
    CiWindowsArchitecture architecture{CiWindowsArchitecture::X86_64};
    CiWindowsCompiler compiler{CiWindowsCompiler::MSVC};
    uint32_t compilerMajorVersion{19};
    uint32_t compilerMinorVersion{38};
    std::string windowsSdkVersion{"10.0.22621.0"};
    uint32_t cmakeMajorVersion{3};
    uint32_t cmakeMinorVersion{28};
    uint32_t ninjaMajorVersion{1};
    uint32_t ninjaMinorVersion{12};
    uint64_t totalMemoryBytes{4294967296ULL}; // 4 GB
    uint32_t availableCores{4};

    bool operator==(const CiWindowsEnvironment& other) const = default;
};

/// @brief Opciones operativas por etapa individual de pipeline.
struct CiWindowsStepOptions {
    uint32_t timeoutMs{15000};
    bool continueOnError{false};
    std::vector<std::string> requiredEnvVars{};
    std::string workingDirectory{"."};

    bool operator==(const CiWindowsStepOptions& other) const = default;
};

/// @brief Definición declarativa inmutable de una etapa de pipeline.
struct CiWindowsStepDefinition {
    CiWindowsPipelineStep step{CiWindowsPipelineStep::EnvironmentCheck};
    std::string name;
    std::string command;
    bool isMandatory{true};
    CiWindowsStepOptions options{};

    bool operator==(const CiWindowsStepDefinition& other) const = default;
};

/// @brief Resultado individual de ejecución de una etapa.
struct CiWindowsStepResult {
    CiWindowsPipelineStep step{CiWindowsPipelineStep::EnvironmentCheck};
    CiWindowsStepStatus status{CiWindowsStepStatus::NotStarted};
    int exitCode{0};
    uint64_t durationMs{0};
    std::string outputSummary;
    std::string errorMessage;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == CiWindowsStepStatus::Success;
    }

    bool operator==(const CiWindowsStepResult& other) const = default;
};

/// @brief Plan inmutable de trabajo de CI Windows.
struct CiWindowsJobPlan {
    std::string presetName{"ci-windows-debug"};
    std::filesystem::path buildDir{"build-ci-win-debug"};
    std::vector<CiWindowsStepDefinition> steps{};
    uint32_t totalTimeoutSeconds{600};
    uint64_t maxMemoryBytes{4294967296ULL}; // 4 GB
    bool failFast{true};

    bool operator==(const CiWindowsJobPlan& other) const = default;
};

/// @brief Reporte inmutable consolidado de la ejecución de un trabajo de CI Windows.
struct CiWindowsJobReport {
    CiWindowsJobStatus status{CiWindowsJobStatus::Pending};
    std::string planName;
    uint64_t durationMs{0};
    std::vector<CiWindowsStepResult> stepResults{};
    bool isClean{false};
    std::string remediationHint;

    [[nodiscard]] std::size_t successfulStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiWindowsStepStatus::Success) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t failedStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiWindowsStepStatus::Failure) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] bool isPassed() const noexcept {
        return status == CiWindowsJobStatus::Passed;
    }

    bool operator==(const CiWindowsJobReport& other) const = default;
};

} // namespace vigilio
