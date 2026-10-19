#pragma once

#include "core/CiMacOsJobTypes.h"

#include <string_view>

namespace vigilio {

/// @brief Entidad estática de configuración canónica e invariantes de CI macOS job.
class CiMacOsJobConfiguration {
  public:
    CiMacOsJobConfiguration() = delete;

    /// @brief Obtiene el plan canónico de compilación y pruebas para depuración en macOS.
    [[nodiscard]] static CiMacOsJobPlan canonicalDebugPlan();

    /// @brief Obtiene el plan canónico de compilación optimizada y verificación de bundle en macOS.
    [[nodiscard]] static CiMacOsJobPlan canonicalReleasePlan();

    /// @brief Verifica de manera estricta que un plan cumpla con todas las invariantes de dominio.
    [[nodiscard]] static bool verifyInvariants(const CiMacOsJobPlan& plan);

    /// @brief Determina si una versión de macOS especificada forma parte del catálogo soportado.
    [[nodiscard]] static bool isSupportedMacOsVersion(std::string_view version);
};

} // namespace vigilio
