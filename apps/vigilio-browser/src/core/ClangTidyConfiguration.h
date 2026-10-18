#pragma once

#include "core/ClangTidyTypes.h"

#include <filesystem>

namespace vigilio {

/// @brief Entidad de configuración canónica de Clang-Tidy y políticas de filtrado de fuentes.
class ClangTidyConfiguration {
  public:
    ClangTidyConfiguration() = delete;

    /// @brief Obtiene la configuración canónica oficial de Vigilio Browser.
    [[nodiscard]] static ClangTidyOptions canonicalConfig();

    /// @brief Determina si la extensión de un archivo corresponde a código C/C++ admitido.
    [[nodiscard]] static bool isSupportedSourceExtension(const std::filesystem::path& path);

    /// @brief Determina si una carpeta corresponde a directorios excluidos del análisis.
    [[nodiscard]] static bool isExcludedDirectory(const std::filesystem::path& path);

    /// @brief Verifica si un conjunto de opciones cumple todas las invariantes de dominio.
    [[nodiscard]] static bool verifyInvariants(const ClangTidyOptions& options);
};

} // namespace vigilio
