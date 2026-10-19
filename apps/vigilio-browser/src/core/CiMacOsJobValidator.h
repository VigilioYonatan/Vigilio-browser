#pragma once

#include "core/CiMacOsJobTypes.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador estático desacoplado de planes de CI macOS, orden de etapas, seguridad de rutas y
/// UX copy.
class CiMacOsJobValidator {
  public:
    CiMacOsJobValidator() = delete;

    /// @brief Valida la coherencia formal, límites de recursos y orden de etapas de un plan de CI macOS.
    [[nodiscard]] static CiMacOsJobError validatePlan(const CiMacOsJobPlan& plan);

    /// @brief Verifica el orden topológico y las dependencias de precedencia entre etapas del pipeline.
    [[nodiscard]] static CiMacOsJobError
    validateStepSequence(const std::vector<CiMacOsPipelineStep>& steps);

    /// @brief Verifica que el entorno de compilación satisfaga las versiones mínimas de herramientas
    /// macOS, Xcode y C++20.
    [[nodiscard]] static CiMacOsJobError validateEnvironment(const CiMacOsEnvironment& env);

    /// @brief Determina si una ruta de compilación es relativa y segura con defensas específicas de
    /// macOS.
    [[nodiscard]] static bool isSafeBuildPath(const std::filesystem::path& path);

    /// @brief Convierte un estado de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobStatusToString(CiMacOsJobStatus status);

    /// @brief Convierte un código de error de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobErrorToString(CiMacOsJobError error);

    /// @brief Convierte un identificador de etapa a nombre descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string stepToString(CiMacOsPipelineStep step);

    /// @brief Convierte un estado de ejecución de etapa a texto en español neutro (UX copy).
    [[nodiscard]] static std::string stepStatusToString(CiMacOsStepStatus status);

    /// @brief Convierte una arquitectura de CPU a su representación en cadena estándar.
    [[nodiscard]] static std::string architectureToString(CiMacOsArchitecture arch);

    /// @brief Convierte un compilador a su representación en cadena estándar.
    [[nodiscard]] static std::string compilerToString(CiMacOsCompiler compiler);

    /// @brief Genera una pista de remediación clara, explicativa y accionable en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(CiMacOsJobError error,
                                                             std::string_view stepName = {});
};

// Funciones libres para compatibilidad y ergonomía en namespace vigilio
[[nodiscard]] inline std::string jobStatusToString(CiMacOsJobStatus status) {
    return CiMacOsJobValidator::jobStatusToString(status);
}

[[nodiscard]] inline std::string jobErrorToString(CiMacOsJobError error) {
    return CiMacOsJobValidator::jobErrorToString(error);
}

[[nodiscard]] inline std::string stepToString(CiMacOsPipelineStep step) {
    return CiMacOsJobValidator::stepToString(step);
}

[[nodiscard]] inline std::string stepStatusToString(CiMacOsStepStatus status) {
    return CiMacOsJobValidator::stepStatusToString(status);
}

[[nodiscard]] inline std::string architectureToString(CiMacOsArchitecture arch) {
    return CiMacOsJobValidator::architectureToString(arch);
}

[[nodiscard]] inline std::string compilerToString(CiMacOsCompiler compiler) {
    return CiMacOsJobValidator::compilerToString(compiler);
}

[[nodiscard]] inline std::string generateRemediationHint(CiMacOsJobError error,
                                                         std::string_view stepName = {}) {
    return CiMacOsJobValidator::generateRemediationHint(error, stepName);
}

} // namespace vigilio
