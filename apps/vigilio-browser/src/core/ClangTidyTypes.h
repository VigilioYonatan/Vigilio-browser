#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Familias de comprobaciones canónicas reconocidas por Clang-Tidy.
enum class ClangTidyCheckCategory {
    Bugprone,
    ClangAnalyzer,
    Performance,
    Modernize,
    Readability,
    CppCoreGuidelines,
    Portability,
    Custom
};

/// @brief Taxonomía exhaustiva de estados de validación de Clang-Tidy.
enum class ClangTidyValidationState { Unconfigured, Configured, Valid, Invalid, DiscrepancyDetected };

/// @brief Clasificación fuertemente tipada de errores de configuración.
enum class ClangTidyError {
    None,
    FileNotFound,
    ParseError,
    EmptyChecks,
    InvalidHeaderFilterRegex,
    UnsafeFilePath,
    CorruptedRules,
    MissingMandatoryKey
};

/// @brief Estructura inmutable representativa del archivo canónico .clang-tidy.
struct ClangTidyOptions {
    std::string checks{"-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-"
                       "braces-around-statements"};
    std::string warningsAsErrors{""};
    std::string headerFilterRegex{"src/.*"};
    std::string formatStyle{"none"};

    bool operator==(const ClangTidyOptions& other) const = default;
};

/// @brief Reporte inmutable de validación de archivo de configuración.
struct ClangTidyValidationReport {
    bool isValid{false};
    ClangTidyError error{ClangTidyError::None};
    std::string configPath;
    std::string errorMessage;
    std::vector<std::string> enabledChecks;
    std::vector<std::string> disabledChecks;
    std::vector<std::string> warnings;
};

/// @brief Estructura de inspección individual de hallazgos de Clang-Tidy.
struct ClangTidyIssueInspection {
    std::string filePath;
    int lineNumber{0};
    int columnNumber{0};
    std::string checkName;
    std::string message;
    bool isWarningAsError{false};
};

} // namespace vigilio
