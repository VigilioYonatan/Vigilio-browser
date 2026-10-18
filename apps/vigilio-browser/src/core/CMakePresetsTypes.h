#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Clasificación de presets según la especificación formal de CMake Presets.
enum class CMakePresetKind : uint8_t { Configure = 0, Build = 1, Test = 2, Package = 3, Workflow = 4 };

/// @brief Tipos estándar canónicos de construcción para CMake.
enum class CMakeBuildType : uint8_t { Debug = 0, Release = 1, RelWithDebInfo = 2, MinSizeRel = 3 };

/// @brief Generadores de compilación compatibles en Vigilio Browser.
enum class CMakeGenerator : uint8_t {
    Ninja = 0,
    UnixMakefiles = 1,
    NinjaMultiConfig = 2,
    VisualStudiov17 = 3
};

/// @brief Taxonomía exhaustiva de resultados de validación de CMake Presets.
enum class CMakePresetValidationOutcome : uint8_t {
    Valid = 0,
    InvalidSchemaVersion = 1,
    MissingRequiredPreset = 2,
    OrphanConfigurePresetReference = 3,
    DuplicatePresetName = 4,
    UnsafeBinaryDir = 5,
    ConflictingVariables = 6,
    MalformedJson = 7
};

/// @brief Estado operativo del flujo de ejecución de presets.
enum class CMakePresetExecutionState : uint8_t {
    Idle = 0,
    Validating = 1,
    Configuring = 2,
    Building = 3,
    Testing = 4,
    Completed = 5,
    Failed = 6,
    Cancelled = 7
};

/// @brief Taxonomía exhaustiva de resultados de ejecución de presets.
enum class CMakePresetExecutionStatus : uint8_t {
    Success = 0,
    ValidationError = 1,
    PresetNotFound = 2,
    BinaryDirUnsafe = 3,
    ToolNotFound = 4,
    ProcessFailed = 5,
    Timeout = 6,
    Cancelled = 7,
    IOError = 8
};

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

/// @brief Opciones configurables para la síntesis y ejecución de comandos de preset.
struct CMakePresetCommandOptions {
    bool dryRun{false};
    uint32_t timeoutMs{60000};
    bool cleanBuildDirFirst{false};
    std::vector<std::string> additionalArgs;
    std::map<std::string, std::string> environmentOverrides;
    std::size_t maxOutputBytes{65536}; // 64 KB límite estricto de buffer
};

/// @brief Resultado inmutable de la ejecución o simulación de un preset.
struct CMakePresetExecutionResult {
    CMakePresetExecutionStatus status{CMakePresetExecutionStatus::Success};
    int exitCode{0};
    std::string commandLine;
    std::string workingDirectory;
    std::string standardOutput;
    std::string standardError;
    uint64_t elapsedMilliseconds{0};
    std::string remediationHint;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == CMakePresetExecutionStatus::Success && exitCode == 0;
    }
};

/// @brief Definición inmutable de un preset individual en memoria.
struct CMakePresetDefinition {
    CMakePresetKind kind{CMakePresetKind::Configure};
    std::string name;
    std::string displayName;
    std::string description;
    std::string generator{"Ninja"};
    std::string binaryDir;
    std::string configurePreset; // Requerido para buildPresets y testPresets
    std::map<std::string, std::string> cacheVariables;
    std::map<std::string, std::string> environment;
    bool outputOnFailure{true};
};

/// @brief Reporte consolidado de la evaluación estática de CMakePresets.json.
struct CMakePresetsValidationReport {
    CMakePresetValidationOutcome outcome{CMakePresetValidationOutcome::Valid};
    std::string message;
    std::vector<std::string> configurePresets;
    std::vector<std::string> buildPresets;
    std::vector<std::string> testPresets;
    std::vector<std::string> packagePresets;
    std::vector<std::string> workflowPresets;
    int schemaVersion{0};

    [[nodiscard]] bool isValid() const noexcept {
        return outcome == CMakePresetValidationOutcome::Valid;
    }
};

/// @brief Resumen consolidado del catálogo de presets cargados en el servicio.
struct CMakePresetsSummary {
    int schemaVersion{0};
    std::size_t totalConfigurePresets{0};
    std::size_t totalBuildPresets{0};
    std::size_t totalTestPresets{0};
    std::size_t totalPackagePresets{0};
    std::size_t totalWorkflowPresets{0};
    std::vector<std::string> configurePresetNames;
    std::vector<std::string> buildPresetNames;
    std::vector<std::string> testPresetNames;
};

} // namespace vigilio
