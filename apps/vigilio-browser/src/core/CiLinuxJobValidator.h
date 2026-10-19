#pragma once

#include "core/CiLinuxJobTypes.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador estático desacoplado de planes de CI, orden de etapas, seguridad de rutas y UX copy.
class CiLinuxJobValidator {
  public:
    CiLinuxJobValidator() = delete;

    /// @brief Valida la coherencia formal, límites de recursos y orden de etapas de un plan de CI Linux.
    [[nodiscard]] static CiLinuxJobError validatePlan(const CiLinuxJobPlan& plan);

    /// @brief Verifica el orden topológico y las dependencias de precedencia entre etapas del pipeline.
    [[nodiscard]] static CiLinuxJobError
    validateStepSequence(const std::vector<CiLinuxPipelineStep>& steps);

    /// @brief Verifica que el entorno de compilación satisfaga las versiones mínimas de herramientas
    /// C++20.
    [[nodiscard]] static CiLinuxJobError validateEnvironment(const CiLinuxEnvironment& env);

    /// @brief Determina si una ruta de compilación es relativa y segura frente a Directory Traversal.
    [[nodiscard]] static bool isSafeBuildPath(const std::filesystem::path& path);

    /// @brief Convierte un estado de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobStatusToString(CiLinuxJobStatus status);

    /// @brief Convierte un código de error de trabajo a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string jobErrorToString(CiLinuxJobError error);

    /// @brief Convierte un identificador de etapa a nombre descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string stepToString(CiLinuxPipelineStep step);

    /// @brief Convierte un estado de ejecución de etapa a texto en español neutro (UX copy).
    [[nodiscard]] static std::string stepStatusToString(CiLinuxStepStatus status);

    /// @brief Convierte una arquitectura de CPU a su representación en cadena estándar.
    [[nodiscard]] static std::string architectureToString(CiLinuxArchitecture arch);

    /// @brief Convierte un compilador a su representación en cadena estándar.
    [[nodiscard]] static std::string compilerToString(CiLinuxCompiler compiler);

    /// @brief Genera una pista de remediación clara, explicativa y accionable en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(CiLinuxJobError error,
                                                             std::string_view stepName = {});
};

// Funciones libres para compatibilidad y ergonomía en namespace vigilio
[[nodiscard]] inline std::string jobStatusToString(CiLinuxJobStatus status) {
    return CiLinuxJobValidator::jobStatusToString(status);
}

[[nodiscard]] inline std::string jobErrorToString(CiLinuxJobError error) {
    return CiLinuxJobValidator::jobErrorToString(error);
}

[[nodiscard]] inline std::string stepToString(CiLinuxPipelineStep step) {
    return CiLinuxJobValidator::stepToString(step);
}

[[nodiscard]] inline std::string stepStatusToString(CiLinuxStepStatus status) {
    return CiLinuxJobValidator::stepStatusToString(status);
}

[[nodiscard]] inline std::string architectureToString(CiLinuxArchitecture arch) {
    return CiLinuxJobValidator::architectureToString(arch);
}

[[nodiscard]] inline std::string compilerToString(CiLinuxCompiler compiler) {
    return CiLinuxJobValidator::compilerToString(compiler);
}

[[nodiscard]] inline std::string generateRemediationHint(CiLinuxJobError error,
                                                         std::string_view stepName = {}) {
    return CiLinuxJobValidator::generateRemediationHint(error, stepName);
}

} // namespace vigilio
