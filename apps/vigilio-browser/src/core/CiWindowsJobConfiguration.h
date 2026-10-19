#pragma once

#include "core/CiWindowsJobTypes.h"

#include <string_view>

namespace vigilio {

/// @brief Entidad estática de configuración canónica e invariantes de CI Windows job.
class CiWindowsJobConfiguration {
  public:
    CiWindowsJobConfiguration() = delete;

    /// @brief Obtiene el plan canónico de compilación y pruebas para depuración en Windows.
    [[nodiscard]] static CiWindowsJobPlan canonicalDebugPlan();

    /// @brief Obtiene el plan canónico de compilación optimizada y empaquetado para release en Windows.
    [[nodiscard]] static CiWindowsJobPlan canonicalReleasePlan();

    /// @brief Verifica de manera estricta que un plan cumpla con todas las invariantes de dominio.
    [[nodiscard]] static bool verifyInvariants(const CiWindowsJobPlan& plan);

    /// @brief Determina si una versión de Windows especificada forma parte del catálogo soportado.
    [[nodiscard]] static bool isSupportedWindowsVersion(std::string_view version);
};

} // namespace vigilio
