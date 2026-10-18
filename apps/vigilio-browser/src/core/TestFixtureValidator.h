#pragma once

#include "core/TestFixtureTypes.h"
#include <filesystem>
#include <string>

namespace vigilio {

/// @brief Validador estático de contención de rutas temporales, cuotas y mensajes descriptivos.
class TestFixtureValidator {
  public:
    /// @brief Comprueba si una ruta cumple estrictamente con las políticas de contención y seguridad.
    [[nodiscard]] static bool isSafeTestDirectory(const std::filesystem::path& path);

    /// @brief Valida la coherencia de una configuración de entorno de pruebas.
    [[nodiscard]] static FixtureError validateConfig(const TestEnvironmentConfig& config);

    /// @brief Verifica si una ruta objetivo reside estrictamente dentro de un directorio base.
    [[nodiscard]] static bool isWithinTempRoot(const std::filesystem::path& target,
                                               const std::filesystem::path& base);

    /// @brief Convierte un estado de ciclo de vida en texto observable en español neutro (UX copy).
    [[nodiscard]] static std::string fixtureStateToString(TestEnvironmentState state);

    /// @brief Convierte un código de error de infraestructura en texto observable en español neutro (UX
    /// copy).
    [[nodiscard]] static std::string fixtureErrorToString(FixtureError error);
};

} // namespace vigilio
