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

/// @brief Etapas canónicas y secuenciales del pipeline de integración continua en macOS.
enum class CiMacOsPipelineStep : uint8_t {
    EnvironmentCheck,
    FormatVerification,
    StaticAnalysis,
    ConfigurePreset,
    BuildCompilation,
    TestSuiteExecution,
    SanitizerVerification,
    BundleVerification
};

/// @brief Estados de ciclo de vida del trabajo de CI en macOS.
enum class CiMacOsJobStatus : uint8_t { Pending, Running, Passed, Failed, Cancelled, TimedOut };

/// @brief Estados de ejecución por etapa individual.
enum class CiMacOsStepStatus : uint8_t { NotStarted, InProgress, Success, Failure, Omitted };

/// @brief Modos de ejecución operativa para el trabajo de CI macOS.
enum class CiMacOsExecutionMode : uint8_t { DryRun, Simulated, Live };

/// @brief Opciones operativas del servicio de CI macOS.
struct CiMacOsServiceOptions {
    CiMacOsExecutionMode mode{CiMacOsExecutionMode::DryRun};
    bool failFast{true};
    uint32_t maxRetries{3};
    uint32_t retryBackoffMs{50};
    bool captureOutput{true};

    bool operator==(const CiMacOsServiceOptions& other) const = default;
};

/// @brief Clasificación exhaustiva y fuertemente tipada de errores de CI macOS.
enum class CiMacOsJobError : uint8_t {
    None,
    ToolchainMissing,
    MacOsSdkMissing,
    XcodeToolsMissing,
    ConfigurationFailed,
    CompilationFailed,
    TestExecutionFailed,
    FormatDiscrepancy,
    StaticAnalysisIssue,
    UnsafePipelinePath,
    TimeoutExceeded,
    InvalidWorkflowSchema,
    DependencyOrderingViolation,
    MemoryLimitExceeded,
    BundleVerificationFailed
};

/// @brief Arquitecturas de CPU admitidas en el entorno macOS.
enum class CiMacOsArchitecture : uint8_t { ARM64, X86_64, Universal };

/// @brief Compiladores C++20 admitidos en macOS.
enum class CiMacOsCompiler : uint8_t { AppleClang, HomebrewClang };

/// @brief Modos y configuraciones de construcción soportados.
enum class CiMacOsBuildConfiguration : uint8_t { Debug, Release, Sanitizer };

/// @brief Metadatos inmutables del entorno de ejecución de CI macOS.
struct CiMacOsEnvironment {
    std::string osVersion{"macOS 14 Sonoma"};
    std::string darwinKernelVersion{"23.4.0"};
    CiMacOsArchitecture architecture{CiMacOsArchitecture::ARM64};
    CiMacOsCompiler compiler{CiMacOsCompiler::AppleClang};
    uint32_t compilerMajorVersion{15};
    uint32_t compilerMinorVersion{0};
    std::string sdkVersion{"14.2"};
    std::string macOsSdkVersion{};
    bool xcodeToolsInstalled{true};
    uint32_t cmakeMajorVersion{3};
    uint32_t cmakeMinorVersion{28};
    uint32_t ninjaMajorVersion{1};
    uint32_t ninjaMinorVersion{12};
    uint64_t totalMemoryBytes{4294967296ULL}; // 4 GB
    uint32_t availableCores{8};

    bool operator==(const CiMacOsEnvironment& other) const = default;
};

/// @brief Opciones operativas por etapa individual de pipeline.
struct CiMacOsStepOptions {
    uint32_t timeoutMs{15000};
    bool continueOnError{false};
    std::vector<std::string> requiredEnvVars{};
    std::string workingDirectory{"."};

    bool operator==(const CiMacOsStepOptions& other) const = default;
};

/// @brief Definición declarativa inmutable de una etapa de pipeline.
struct CiMacOsStepDefinition {
    CiMacOsPipelineStep step{CiMacOsPipelineStep::EnvironmentCheck};
    std::string name;
    std::string command;
    bool isMandatory{true};
    CiMacOsStepOptions options{};

    bool operator==(const CiMacOsStepDefinition& other) const = default;
};

/// @brief Resultado individual de ejecución de una etapa.
struct CiMacOsStepResult {
    CiMacOsPipelineStep step{CiMacOsPipelineStep::EnvironmentCheck};
    CiMacOsStepStatus status{CiMacOsStepStatus::NotStarted};
    int exitCode{0};
    uint64_t durationMs{0};
    std::string outputSummary;
    std::string errorMessage;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == CiMacOsStepStatus::Success;
    }

    bool operator==(const CiMacOsStepResult& other) const = default;
};

/// @brief Plan inmutable de trabajo de CI macOS.
struct CiMacOsJobPlan {
    std::string presetName{"ci-macos-debug"};
    std::filesystem::path buildDir{"build-ci-macos-debug"};
    std::vector<CiMacOsStepDefinition> steps{};
    uint32_t totalTimeoutSeconds{600};
    uint64_t maxMemoryBytes{4294967296ULL}; // 4 GB
    bool failFast{true};

    bool operator==(const CiMacOsJobPlan& other) const = default;
};

/// @brief Reporte inmutable consolidado de la ejecución de un trabajo de CI macOS.
struct CiMacOsJobReport {
    CiMacOsJobStatus status{CiMacOsJobStatus::Pending};
    std::string planName;
    uint64_t durationMs{0};
    std::vector<CiMacOsStepResult> stepResults{};
    bool isClean{false};
    std::string remediationHint;

    [[nodiscard]] std::size_t successfulStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiMacOsStepStatus::Success) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t failedStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiMacOsStepStatus::Failure) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] bool isPassed() const noexcept {
        return status == CiMacOsJobStatus::Passed;
    }

    bool operator==(const CiMacOsJobReport& other) const = default;
};

} // namespace vigilio
