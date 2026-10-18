#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vigilio {

#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED
#define VIGILIO_CANCELLATION_TOKEN_DEFINED
/// @brief Token ligero para soporte de cancelación cooperativa multihilo.
struct CancellationToken {
    std::atomic<bool> cancelled{false};

    void requestCancellation() noexcept {
        cancelled.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled.load(std::memory_order_acquire);
    }
};
#endif

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

/// @brief Taxonomía fuertemente tipada de resultados operativos de Clang-Tidy.
enum class TidyExecutionStatus : uint8_t {
    Success = 0,
    IssuesFound = 1,
    Cancelled = 2,
    FileLocked = 3,
    PermissionDenied = 4,
    ReadError = 5,
    WriteError = 6,
    FileTooLarge = 7,
    UnsafePath = 8,
    FileNotFound = 9,
    ConfigurationError = 10,
    ToolUnavailable = 11
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

/// @brief Estructura inmutable que encapsula la inspección individual de un archivo.
struct SourceFileTidyInspection {
    std::string relativePath;
    bool isClean{true};
    int issuesCount{0};
    std::vector<ClangTidyIssueInspection> issues;
    std::string summary;
};

/// @brief Opciones inmutables de configuración operativa para la capa de servicio.
struct TidyServiceOptions {
    bool dryRun{false};
    bool createBackup{true};
    std::size_t maxFileSizeBytes{10 * 1024 * 1024};
    uint32_t timeoutMs{15000};
    uint32_t maxRetries{3};
    uint32_t retryBackoffMs{50};
    bool recursive{true};
    std::size_t maxScanDepth{32};
    std::string buildDirectory{"_build"};
    bool exportFixes{false};
};

/// @brief Estructura inmutable que encapsula el resultado consolidado de la operación de linteo.
struct TidyOperationResult {
    TidyExecutionStatus status{TidyExecutionStatus::Success};
    std::size_t filesExamined{0};
    std::size_t filesFixed{0};
    std::size_t issuesCount{0};
    uint64_t durationMs{0};
    std::string remediationHint;
    std::vector<SourceFileTidyInspection> details;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == TidyExecutionStatus::Success;
    }

    [[nodiscard]] bool hasIssues() const noexcept {
        return issuesCount > 0;
    }
};

} // namespace vigilio
