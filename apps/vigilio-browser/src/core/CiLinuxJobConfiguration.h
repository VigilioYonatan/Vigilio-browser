#pragma once

#include "core/CiLinuxJobTypes.h"

#include <string_view>

namespace vigilio {

/// @brief Entidad estática de configuración canónica e invariantes de CI Linux job.
class CiLinuxJobConfiguration {
  public:
    CiLinuxJobConfiguration() = delete;

    /// @brief Obtiene el plan canónico de compilación y pruebas para depuración en Linux.
    [[nodiscard]] static CiLinuxJobPlan canonicalDebugPlan();

    /// @brief Obtiene el plan canónico de compilación optimizada y empaquetado para release en Linux.
    [[nodiscard]] static CiLinuxJobPlan canonicalReleasePlan();

    /// @brief Verifica de manera estricta que un plan cumpla con todas las invariantes de dominio.
    [[nodiscard]] static bool verifyInvariants(const CiLinuxJobPlan& plan);

    /// @brief Determina si una distribución Linux especificada forma parte del catálogo soportado.
    [[nodiscard]] static bool isSupportedLinuxDistro(std::string_view distro);
};

} // namespace vigilio
