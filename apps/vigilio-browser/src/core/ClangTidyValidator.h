#pragma once

#include "core/ClangTidyTypes.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador estático desacoplado y parser YAML para Clang-Tidy.
/// Libre de dependencias hacia QApplication, QWidget o Chromium.
class ClangTidyValidator {
  public:
    ClangTidyValidator() = delete;

    /// @brief Valida los límites y consistencia de las opciones de Clang-Tidy.
    [[nodiscard]] static ClangTidyError validateOptions(const ClangTidyOptions& options);

    /// @brief Carga, parsea y valida un archivo de configuración en disco.
    [[nodiscard]] static ClangTidyValidationReport
    validateConfigurationFile(const std::filesystem::path& configPath);

    /// @brief Valida el contenido YAML de configuración directamente en memoria.
    [[nodiscard]] static ClangTidyValidationReport validateConfigContent(std::string_view yamlContent);

    /// @brief Parsea deterministamente el contenido YAML de un archivo .clang-tidy.
    [[nodiscard]] static bool parseConfig(std::string_view yamlContent, ClangTidyOptions& outOptions,
                                          std::string* outErrorMessage = nullptr);

    /// @brief Extrae y discrimina los tokens de reglas habilitadas (+) y deshabilitadas (-).
    static void parseCheckTokens(std::string_view checksStr, std::vector<std::string>& outEnabled,
                                 std::vector<std::string>& outDisabled);

    /// @brief Serializa un conjunto de opciones a formato YAML canónico normalizado.
    [[nodiscard]] static std::string serializeConfig(const ClangTidyOptions& options);

    /// @brief Comprueba si una ruta de archivo es segura contra directory traversal y contención.
    [[nodiscard]] static bool isSafeConfigPath(const std::filesystem::path& path);

    /// @brief Valida si una expresión regular es sintácticamente correcta y segura para compilación.
    [[nodiscard]] static bool isSafeRegex(std::string_view regexStr);

    /// @brief Convierte un estado de validación a texto legible en español neutro (UX copy).
    [[nodiscard]] static std::string tidyStateToString(ClangTidyValidationState state);

    /// @brief Convierte un error de validación a texto legible en español neutro (UX copy).
    [[nodiscard]] static std::string tidyErrorToString(ClangTidyError error);

    /// @brief Convierte una categoría de comprobaciones a su representación en cadena de texto.
    [[nodiscard]] static std::string categoryToString(ClangTidyCheckCategory category);
};

} // namespace vigilio
