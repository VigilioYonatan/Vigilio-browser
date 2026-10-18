#pragma once

#include "core/CMakePresetsTypes.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador desacoplado del dominio para especificaciones CMake Presets (v6 / v3+).
///
/// Implementa análisis estático en memoria libre de dependencias hacia QApplication,
/// QWidget o QWebEngineView.
class CMakePresetsValidator {
  public:
    CMakePresetsValidator() = delete;

    /// @brief Analiza y valida el contenido JSON de una especificación de presets.
    /// @param jsonContent Cadena con el documento JSON completo.
    /// @return Reporte consolidado con resultado, diagnóstico y listas de presets detectados.
    [[nodiscard]] static CMakePresetsValidationReport
    validatePresetsJson(const std::string& jsonContent);

    /// @brief Carga y valida un archivo de presets desde el sistema de archivos local.
    /// @param path Ruta al archivo CMakePresets.json.
    /// @return Reporte consolidado con resultado y diagnóstico.
    [[nodiscard]] static CMakePresetsValidationReport
    validatePresetsFile(const std::filesystem::path& path);

    /// @brief Comprueba si un directorio de compilación binario cumple las políticas de seguridad.
    /// Rechaza secuencias de escape (..), rutas absolutas fijadas al host y exige prefijo build-*.
    /// @param binaryDir Ruta a evaluar.
    /// @return true si es seguro y conforme a la convención; false en caso contrario.
    [[nodiscard]] static bool isSafeBinaryDir(const std::string& binaryDir);

    /// @brief Obtiene la lista canónica de presets mínimos indispensables para Vigilio Browser.
    /// @return Vector con "core-debug" y "desktop-debug".
    [[nodiscard]] static std::vector<std::string> canonicalRequiredPresets();

    /// @brief Alias de canonicalRequiredPresets() para compatibilidad de nomenclatura.
    [[nodiscard]] static std::vector<std::string> canonicalPresetNames();

    /// @brief Obtiene el catálogo completo de los 9 perfiles canónicos estandarizados.
    [[nodiscard]] static std::vector<std::string> canonicalAllPresets();

    /// @brief Traduce un código de resultado a una descripción textual legible en español técnico.
    /// Conforme a las directrices de UX-COPY-PUBLICO.md.
    /// @param outcome Código de resultado de la evaluación.
    /// @return Cadena explicativa del resultado.
    [[nodiscard]] static std::string outcomeToString(CMakePresetValidationOutcome outcome);
};

} // namespace vigilio
