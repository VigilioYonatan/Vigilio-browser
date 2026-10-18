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

/// @brief Taxonomía fuertemente tipada de resultados operativos del servicio de formateo.
enum class FormatExecutionStatus : uint8_t {
    Success = 0,
    DiscrepancyFound = 1,
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

/// @brief Opciones inmutables de configuración para la capa operativa de servicio.
struct FormatServiceOptions {
    bool dryRun{false};
    bool createBackup{true};
    std::size_t maxFileSizeBytes{10 * 1024 * 1024};
    uint32_t timeoutMs{15000};
    uint32_t maxRetries{3};
    uint32_t retryBackoffMs{50};
    bool recursive{true};
    std::size_t maxScanDepth{32};
};

/// @brief Estructura inmutable que encapsula el resultado consolidado de una operación de formateo.
struct FormatOperationResult {
    FormatExecutionStatus status{FormatExecutionStatus::Success};
    std::size_t filesExamined{0};
    std::size_t filesFormatted{0};
    std::size_t discrepanciesCount{0};
    uint64_t durationMs{0};
    std::string remediationHint;
    std::vector<SourceFileFormatInspection> details;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == FormatExecutionStatus::Success;
    }
};

} // namespace vigilio
