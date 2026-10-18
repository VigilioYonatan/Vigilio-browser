#pragma once

#include "core/ClangFormatTypes.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace vigilio {

/// @brief Validador estático desacoplado y parser YAML para Clang-Format.
/// Libre de dependencias hacia QApplication, QWidget o Chromium.
class ClangFormatValidator {
  public:
    ClangFormatValidator() = delete;

    /// @brief Valida los límites y políticas de las opciones de Clang-Format.
    [[nodiscard]] static ClangFormatError validateOptions(const ClangFormatOptions& options);

    /// @brief Carga, parsea y valida un archivo de configuración en disco.
    [[nodiscard]] static ClangFormatValidationReport
    validateConfigurationFile(const std::filesystem::path& configPath);

    /// @brief Valida el contenido YAML de configuración directamente en memoria.
    [[nodiscard]] static ClangFormatValidationReport validateConfigContent(std::string_view yamlContent);

    /// @brief Parsea deterministamente el contenido YAML de un archivo .clang-format.
    [[nodiscard]] static bool parseConfig(std::string_view yamlContent, ClangFormatOptions& outOptions,
                                          std::string* outErrorMessage = nullptr);

    /// @brief Serializa un conjunto de opciones a formato YAML canónico normalizado.
    [[nodiscard]] static std::string serializeConfig(const ClangFormatOptions& options);

    /// @brief Comprueba si una ruta de archivo es segura contra directory traversal y contención.
    [[nodiscard]] static bool isSafeConfigPath(const std::filesystem::path& path);

    /// @brief Convierte un estado de validación a texto legible en español neutro (UX copy).
    [[nodiscard]] static std::string formatStateToString(ClangFormatValidationState state);

    /// @brief Convierte un error de validación a texto legible en español neutro (UX copy).
    [[nodiscard]] static std::string formatErrorToString(ClangFormatError error);

    /// @brief Convierte un preset de estilo a su representación en cadena de texto.
    [[nodiscard]] static std::string stylePresetToString(ClangFormatStylePreset preset);

    /// @brief Convierte una política de alineación de punteros a su representación en texto.
    [[nodiscard]] static std::string pointerAlignmentToString(PointerAlignmentPolicy policy);

    /// @brief Convierte una regla de funciones cortas a su representación en texto.
    [[nodiscard]] static std::string shortFunctionPolicyToString(ShortFunctionPolicy policy);
};

} // namespace vigilio
