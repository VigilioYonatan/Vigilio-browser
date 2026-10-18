#pragma once

#include <string>
#include <vector>

namespace vigilio {

/// @brief Estilos base de formateo reconocidos por Clang-Format.
enum class ClangFormatStylePreset { LLVM, Google, Chromium, Mozilla, WebKit, Microsoft, GNU, Custom };

/// @brief Política de alineación de punteros y referencias en declaraciones.
enum class PointerAlignmentPolicy { Left, Right, Middle };

/// @brief Regla de colocación de cuerpos de funciones cortas en una sola línea.
enum class ShortFunctionPolicy { None, Inline, Empty, All };

/// @brief Estados formales del ciclo de vida y validación de la configuración de formato.
enum class ClangFormatValidationState { Unconfigured, Configured, Valid, Invalid, DiscrepancyDetected };

/// @brief Clasificación exhaustiva y tipada de errores de formato e infraestructura.
enum class ClangFormatError {
    None,
    FileNotFound,
    ParseError,
    UnsupportedVersion,
    InvalidIndentWidth,
    InvalidColumnLimit,
    UnsafeFilePath,
    CorruptedRules,
    FormattingDiscrepancy
};

/// @brief Opciones inmutables representativas del archivo canónico .clang-format.
struct ClangFormatOptions {
    ClangFormatStylePreset stylePreset{ClangFormatStylePreset::LLVM};
    int indentWidth{4};
    int columnLimit{105};
    PointerAlignmentPolicy pointerAlignment{PointerAlignmentPolicy::Left};
    ShortFunctionPolicy allowShortFunctions{ShortFunctionPolicy::None};
    std::string cppStandard{"c++20"};

    bool operator==(const ClangFormatOptions& other) const = default;
};

/// @brief Reporte inmutable de validación de archivo de configuración.
struct ClangFormatValidationReport {
    bool isValid{false};
    ClangFormatError error{ClangFormatError::None};
    std::string configPath;
    std::string errorMessage;
    int discrepancyCount{0};
    std::vector<std::string> warnings;
};

/// @brief Estructura de inspección individual por archivo fuente.
struct SourceFileFormatInspection {
    std::string relativePath;
    bool isCompliant{true};
    int lineCount{0};
    std::string discrepancySummary;
};

} // namespace vigilio
