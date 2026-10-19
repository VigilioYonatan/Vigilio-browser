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

/// @brief Etapas canónicas y secuenciales del pipeline de integración continua en Linux.
enum class CiLinuxPipelineStep {
    EnvironmentCheck,
    FormatVerification,
    StaticAnalysis,
    ConfigurePreset,
    BuildCompilation,
    TestSuiteExecution,
    SanitizerVerification,
    ArtifactPackaging
};

/// @brief Estados de ciclo de vida del trabajo de CI en Linux.
enum class CiLinuxJobStatus { Pending, Running, Passed, Failed, Cancelled, TimedOut };

/// @brief Estados de ejecución por etapa individual.
enum class CiLinuxStepStatus { NotStarted, InProgress, Success, Failure, Omitted };

/// @brief Clasificación exhaustiva y fuertemente tipada de errores de CI Linux.
enum class CiLinuxJobError {
    None,
    ToolchainMissing,
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

/// @brief Arquitecturas de CPU admitidas en el entorno Linux.
enum class CiLinuxArchitecture { X86_64, AArch64 };

/// @brief Compiladores C++20 admitidos en Linux.
enum class CiLinuxCompiler { GCC, Clang };

/// @brief Modos y configuraciones de construcción soportados.
enum class CiLinuxBuildConfiguration { Debug, Release, Sanitizer };

/// @brief Metadatos inmutables del entorno de ejecución de CI Linux.
struct CiLinuxEnvironment {
    std::string distribution{"Ubuntu 24.04 LTS"};
    std::string kernelVersion{"6.8.0"};
    CiLinuxArchitecture architecture{CiLinuxArchitecture::X86_64};
    CiLinuxCompiler compiler{CiLinuxCompiler::GCC};
    uint32_t compilerMajorVersion{14};
    uint32_t compilerMinorVersion{0};
    uint32_t cmakeMajorVersion{3};
    uint32_t cmakeMinorVersion{28};
    uint32_t ninjaMajorVersion{1};
    uint32_t ninjaMinorVersion{12};
    uint64_t totalMemoryBytes{4294967296ULL}; // 4 GB
    uint32_t availableCores{4};

    bool operator==(const CiLinuxEnvironment& other) const = default;
};

/// @brief Opciones operativas por etapa individual de pipeline.
struct CiLinuxStepOptions {
    uint32_t timeoutMs{15000};
    bool continueOnError{false};
    std::vector<std::string> requiredEnvVars{};
    std::string workingDirectory{"."};

    bool operator==(const CiLinuxStepOptions& other) const = default;
};

/// @brief Definición declarativa inmutable de una etapa de pipeline.
struct CiLinuxStepDefinition {
    CiLinuxPipelineStep step{CiLinuxPipelineStep::EnvironmentCheck};
    std::string name;
    std::string command;
    bool isMandatory{true};
    CiLinuxStepOptions options{};

    bool operator==(const CiLinuxStepDefinition& other) const = default;
};

/// @brief Resultado individual de ejecución de una etapa.
struct CiLinuxStepResult {
    CiLinuxPipelineStep step{CiLinuxPipelineStep::EnvironmentCheck};
    CiLinuxStepStatus status{CiLinuxStepStatus::NotStarted};
    int exitCode{0};
    uint64_t durationMs{0};
    std::string outputSummary;
    std::string errorMessage;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == CiLinuxStepStatus::Success;
    }

    bool operator==(const CiLinuxStepResult& other) const = default;
};

/// @brief Plan inmutable de trabajo de CI Linux.
struct CiLinuxJobPlan {
    std::string presetName{"ci-linux-debug"};
    std::filesystem::path buildDir{"build-ci-debug"};
    std::vector<CiLinuxStepDefinition> steps{};
    uint32_t totalTimeoutSeconds{600};
    uint64_t maxMemoryBytes{4294967296ULL}; // 4 GB
    bool failFast{true};

    bool operator==(const CiLinuxJobPlan& other) const = default;
};

/// @brief Reporte inmutable consolidado de la ejecución de un trabajo de CI Linux.
struct CiLinuxJobReport {
    CiLinuxJobStatus status{CiLinuxJobStatus::Pending};
    std::string planName;
    uint64_t durationMs{0};
    std::vector<CiLinuxStepResult> stepResults{};
    bool isClean{false};
    std::string remediationHint;

    [[nodiscard]] std::size_t successfulStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiLinuxStepStatus::Success) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t failedStepsCount() const noexcept {
        std::size_t count = 0;
        for (const auto& res : stepResults) {
            if (res.status == CiLinuxStepStatus::Failure) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] bool isPassed() const noexcept {
        return status == CiLinuxJobStatus::Passed;
    }

    bool operator==(const CiLinuxJobReport& other) const = default;
};

} // namespace vigilio
