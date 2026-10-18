#pragma once

#include "core/CMakePresetsTypes.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa desacoplada del dominio para la gestión y ejecución de CMake Presets.
///
/// Proporciona descubrimiento, consultas indexadas, síntesis determinista de líneas de comando CLI,
/// ejecución acotada con soporte para dry-run y cancelación cooperativa, y saneamiento seguro
/// del árbol de compilación, libre de dependencias de GUI o Chromium.
class CMakePresetsService {
  public:
    CMakePresetsService() = default;
    ~CMakePresetsService() = default;

    // Carga y validación inicial
    [[nodiscard]] bool loadFromFile(const std::filesystem::path& presetsPath);
    [[nodiscard]] bool loadFromJson(const std::string& jsonContent,
                                    const std::filesystem::path& sourceDir = ".");
    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] const CMakePresetsValidationReport& validationReport() const noexcept;
    [[nodiscard]] CMakePresetsSummary summary() const;

    // Consultas y resolución de jerarquías
    [[nodiscard]] std::optional<CMakePresetDefinition> findPreset(std::string_view name,
                                                                  CMakePresetKind kind) const;
    [[nodiscard]] std::vector<CMakePresetDefinition> listPresetsByKind(CMakePresetKind kind) const;
    [[nodiscard]] std::string resolveAssociatedConfigurePreset(std::string_view presetName,
                                                               CMakePresetKind kind) const;

    // Síntesis determinista de líneas de comando
    [[nodiscard]] std::string
    buildConfigureCommandLine(std::string_view presetName,
                              const CMakePresetCommandOptions& options = {}) const;
    [[nodiscard]] std::string buildBuildCommandLine(std::string_view presetName,
                                                    const CMakePresetCommandOptions& options = {}) const;
    [[nodiscard]] std::string buildTestCommandLine(std::string_view presetName,
                                                   const CMakePresetCommandOptions& options = {}) const;

    // Ejecución y simulación operativa
    [[nodiscard]] CMakePresetExecutionResult
    executeConfigure(std::string_view presetName, const CMakePresetCommandOptions& options = {},
                     CancellationToken* cancel = nullptr);
    [[nodiscard]] CMakePresetExecutionResult executeBuild(std::string_view presetName,
                                                          const CMakePresetCommandOptions& options = {},
                                                          CancellationToken* cancel = nullptr);
    [[nodiscard]] CMakePresetExecutionResult executeTest(std::string_view presetName,
                                                         const CMakePresetCommandOptions& options = {},
                                                         CancellationToken* cancel = nullptr);

    // Operaciones de saneamiento y recuperación
    [[nodiscard]] bool cleanBinaryDir(std::string_view configurePresetName,
                                      std::string* errorMessage = nullptr);

    // Formateo observable de diagnósticos
    [[nodiscard]] static std::string executionStatusToString(CMakePresetExecutionStatus status);
    [[nodiscard]] static std::string executionStateToString(CMakePresetExecutionState state);

  private:
    std::filesystem::path sourceDirectory_{"."};
    bool isLoaded_{false};
    CMakePresetsValidationReport report_;
    std::vector<CMakePresetDefinition> configurePresets_;
    std::vector<CMakePresetDefinition> buildPresets_;
    std::vector<CMakePresetDefinition> testPresets_;
    std::vector<CMakePresetDefinition> packagePresets_;
    std::vector<CMakePresetDefinition> workflowPresets_;

    [[nodiscard]] std::string expandMacros(const std::string& input, std::string_view presetName) const;
    [[nodiscard]] CMakePresetExecutionResult
    executeInternal(std::string_view presetName, CMakePresetKind kind, const std::string& cmdLine,
                    const CMakePresetCommandOptions& options, CancellationToken* cancel);
};

} // namespace vigilio
