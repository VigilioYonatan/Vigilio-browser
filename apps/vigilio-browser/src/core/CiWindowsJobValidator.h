#pragma once

#include "core/CiWindowsJobTypes.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador estático desacoplado de planes de CI Windows, orden de etapas, seguridad de rutas y
/// UX copy.
class CiWindowsJobValidator {
  public:
    CiWindowsJobValidator() = delete;

    /// @brief Valida la coherencia formal, límites de recursos y orden de etapas de un plan de CI
    /// Windows.
    [[nodiscard]] static CiWindowsJobError validatePlan(const CiWindowsJobPlan& plan);

    /// @brief Verifica el orden topológico y las dependencias de precedencia entre etapas del pipeline.
    [[nodiscard]] static CiWindowsJobError
    validateStepSequence(const std::vector<CiWindowsPipelineStep>& steps);

    /// @brief Verifica que el entorno de compilación satisfaga las versiones mínimas de herramientas
    /// Windows y C++20.
    [[nodiscard]] static CiWindowsJobError validateEnvironment(const CiWindowsEnvironment& env);

    /// @brief Determina si una ruta de compilación es relativa y segura con defensas específicas de
    /// Windows.
    [[nodiscard]] static bool isSafeBuildPath(const std::filesystem::path& path);

    /// @brief Convierte un estado de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobStatusToString(CiWindowsJobStatus status);

    /// @brief Convierte un código de error de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobErrorToString(CiWindowsJobError error);

    /// @brief Convierte un identificador de etapa a nombre descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string stepToString(CiWindowsPipelineStep step);

    /// @brief Convierte un estado de ejecución de etapa a texto en español neutro (UX copy).
    [[nodiscard]] static std::string stepStatusToString(CiWindowsStepStatus status);

    /// @brief Convierte una arquitectura de CPU a su representación en cadena estándar.
    [[nodiscard]] static std::string architectureToString(CiWindowsArchitecture arch);

    /// @brief Convierte un compilador a su representación en cadena estándar.
    [[nodiscard]] static std::string compilerToString(CiWindowsCompiler compiler);

    /// @brief Genera una pista de remediación clara, explicativa y accionable en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(CiWindowsJobError error,
                                                             std::string_view stepName = {});
};

// Funciones libres para compatibilidad y ergonomía en namespace vigilio
[[nodiscard]] inline std::string jobStatusToString(CiWindowsJobStatus status) {
    return CiWindowsJobValidator::jobStatusToString(status);
}

[[nodiscard]] inline std::string jobErrorToString(CiWindowsJobError error) {
    return CiWindowsJobValidator::jobErrorToString(error);
}

[[nodiscard]] inline std::string stepToString(CiWindowsPipelineStep step) {
    return CiWindowsJobValidator::stepToString(step);
}

[[nodiscard]] inline std::string stepStatusToString(CiWindowsStepStatus status) {
    return CiWindowsJobValidator::stepStatusToString(status);
}

[[nodiscard]] inline std::string architectureToString(CiWindowsArchitecture arch) {
    return CiWindowsJobValidator::architectureToString(arch);
}

[[nodiscard]] inline std::string compilerToString(CiWindowsCompiler compiler) {
    return CiWindowsJobValidator::compilerToString(compiler);
}

[[nodiscard]] inline std::string generateRemediationHint(CiWindowsJobError error,
                                                         std::string_view stepName = {}) {
    return CiWindowsJobValidator::generateRemediationHint(error, stepName);
}

} // namespace vigilio
