#pragma once

#include "core/ClangFormatTypes.h"
#include <filesystem>

namespace vigilio {

/// @brief Entidad de configuración canónica de Clang-Format y políticas de filtrado de fuentes.
class ClangFormatConfiguration {
  public:
    ClangFormatConfiguration() = delete;

    /// @brief Obtiene la configuración canónica oficial de Vigilio Browser.
    [[nodiscard]] static ClangFormatOptions canonicalConfig();

    /// @brief Determina si la extensión de un archivo corresponde a código C/C++ admitido.
    [[nodiscard]] static bool isSupportedSourceExtension(const std::filesystem::path& path);

    /// @brief Determina si una carpeta corresponde a directorios excluidos del formateo.
    [[nodiscard]] static bool isExcludedDirectory(const std::filesystem::path& path);

    /// @brief Verifica si un conjunto de opciones cumple todas las invariantes de dominio.
    [[nodiscard]] static bool verifyInvariants(const ClangFormatOptions& options);
};

} // namespace vigilio
