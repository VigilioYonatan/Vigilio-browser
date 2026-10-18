#include "core/ClangFormatValidator.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace vigilio {

namespace {

[[nodiscard]] std::string_view trim(std::string_view str) {
    while (!str.empty() &&
           (str.front() == ' ' || str.front() == '\t' || str.front() == '\r' || str.front() == '\n')) {
        str.remove_prefix(1);
    }
    while (!str.empty() &&
           (str.back() == ' ' || str.back() == '\t' || str.back() == '\r' || str.back() == '\n')) {
        str.remove_suffix(1);
    }
    return str;
}

[[nodiscard]] std::string_view stripQuotes(std::string_view str) {
    if (str.size() >= 2) {
        if ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\'')) {
            str.remove_prefix(1);
            str.remove_suffix(1);
        }
    }
    return str;
}

} // namespace

ClangFormatError ClangFormatValidator::validateOptions(const ClangFormatOptions& options) {
    if (options.indentWidth < 2 || options.indentWidth > 8 || (options.indentWidth % 2 != 0)) {
        return ClangFormatError::InvalidIndentWidth;
    }

    if (options.columnLimit < 80 || options.columnLimit > 140) {
        return ClangFormatError::InvalidColumnLimit;
    }

    if (options.stylePreset != ClangFormatStylePreset::LLVM) {
        return ClangFormatError::CorruptedRules;
    }

    if (options.pointerAlignment != PointerAlignmentPolicy::Left) {
        return ClangFormatError::CorruptedRules;
    }

    if (options.allowShortFunctions != ShortFunctionPolicy::None) {
        return ClangFormatError::CorruptedRules;
    }

    if (options.cppStandard.empty()) {
        return ClangFormatError::CorruptedRules;
    }

    return ClangFormatError::None;
}

bool ClangFormatValidator::isSafeConfigPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }

    const std::string pathStr = path.string();

    // Comprobación de bytes nulos y caracteres de control
    for (char ch : pathStr) {
        if (static_cast<unsigned char>(ch) < 32 || ch == 127) {
            return false;
        }
    }

    // Prevención de Directory Traversal
    if (pathStr.find("..") != std::string::npos) {
        return false;
    }

    // Rechazo de rutas de sistema sensibles
    if (pathStr.starts_with("/etc") || pathStr.starts_with("/var") || pathStr.starts_with("/root") ||
        pathStr.starts_with("/sys") || pathStr.starts_with("/proc") || pathStr.starts_with("/dev") ||
        pathStr.starts_with("/boot") || pathStr.starts_with("/usr") || pathStr.starts_with("/bin") ||
        pathStr.starts_with("/sbin")) {
        return false;
    }

    // Rechazo de rutas absolutas de sistema en Windows
    if (pathStr.find(":\\Windows") != std::string::npos ||
        pathStr.find(":/Windows") != std::string::npos) {
        return false;
    }

    return true;
}

ClangFormatValidationReport
ClangFormatValidator::validateConfigurationFile(const std::filesystem::path& configPath) {
    ClangFormatValidationReport report;
    report.configPath = configPath.string();

    if (!isSafeConfigPath(configPath)) {
        report.isValid = false;
        report.error = ClangFormatError::UnsafeFilePath;
        report.errorMessage = formatErrorToString(report.error);
        return report;
    }

    std::error_code ec;
    if (!std::filesystem::exists(configPath, ec) || ec) {
        report.isValid = false;
        report.error = ClangFormatError::FileNotFound;
        report.errorMessage = formatErrorToString(report.error);
        return report;
    }

    if (!std::filesystem::is_regular_file(configPath, ec) || ec) {
        report.isValid = false;
        report.error = ClangFormatError::FileNotFound;
        report.errorMessage = formatErrorToString(report.error);
        return report;
    }

    std::ifstream file(configPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        report.isValid = false;
        report.error = ClangFormatError::FileNotFound;
        report.errorMessage = formatErrorToString(report.error);
        return report;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    report = validateConfigContent(content);
    report.configPath = configPath.string();
    return report;
}

ClangFormatValidationReport ClangFormatValidator::validateConfigContent(std::string_view yamlContent) {
    ClangFormatValidationReport report;
    report.configPath = "<in-memory>";

    if (yamlContent.empty()) {
        report.isValid = false;
        report.error = ClangFormatError::ParseError;
        report.errorMessage = formatErrorToString(report.error);
        return report;
    }

    // Comprobación de bytes nulos o caracteres binarios
    for (char ch : yamlContent) {
        if (ch == '\0') {
            report.isValid = false;
            report.error = ClangFormatError::ParseError;
            report.errorMessage = formatErrorToString(report.error);
            return report;
        }
    }

    ClangFormatOptions options;
    std::string parseErrMsg;
    if (!parseConfig(yamlContent, options, &parseErrMsg)) {
        report.isValid = false;
        report.error = ClangFormatError::ParseError;
        report.errorMessage = parseErrMsg.empty() ? formatErrorToString(report.error) : parseErrMsg;
        return report;
    }

    const ClangFormatError optErr = validateOptions(options);
    if (optErr != ClangFormatError::None) {
        report.isValid = false;
        report.error = optErr;
        report.errorMessage = formatErrorToString(optErr);
        return report;
    }

    report.isValid = true;
    report.error = ClangFormatError::None;
    report.errorMessage = formatErrorToString(report.error);
    report.discrepancyCount = 0;
    return report;
}

bool ClangFormatValidator::parseConfig(std::string_view yamlContent, ClangFormatOptions& outOptions,
                                       std::string* outErrorMessage) {
    bool hasBasedOnStyle = false;
    bool hasIndentWidth = false;
    bool hasColumnLimit = false;

    std::size_t pos = 0;
    while (pos < yamlContent.size()) {
        const std::size_t lineEnd = yamlContent.find('\n', pos);
        std::string_view line = (lineEnd == std::string_view::npos)
                                    ? yamlContent.substr(pos)
                                    : yamlContent.substr(pos, lineEnd - pos);
        pos = (lineEnd == std::string_view::npos) ? yamlContent.size() : lineEnd + 1;

        // Eliminar comentarios iniciando con '#'
        const std::size_t commentPos = line.find('#');
        if (commentPos != std::string_view::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const std::size_t colonPos = line.find(':');
        if (colonPos == std::string_view::npos) {
            if (outErrorMessage) {
                *outErrorMessage = "Línea malformada en archivo YAML: clave sin separador dos puntos";
            }
            return false;
        }

        const std::string_view key = trim(line.substr(0, colonPos));
        const std::string_view val = stripQuotes(trim(line.substr(colonPos + 1)));

        if (key == "BasedOnStyle") {
            hasBasedOnStyle = true;
            if (val == "LLVM") {
                outOptions.stylePreset = ClangFormatStylePreset::LLVM;
            } else if (val == "Google") {
                outOptions.stylePreset = ClangFormatStylePreset::Google;
            } else if (val == "Chromium") {
                outOptions.stylePreset = ClangFormatStylePreset::Chromium;
            } else if (val == "Mozilla") {
                outOptions.stylePreset = ClangFormatStylePreset::Mozilla;
            } else if (val == "WebKit") {
                outOptions.stylePreset = ClangFormatStylePreset::WebKit;
            } else if (val == "Microsoft") {
                outOptions.stylePreset = ClangFormatStylePreset::Microsoft;
            } else if (val == "GNU") {
                outOptions.stylePreset = ClangFormatStylePreset::GNU;
            } else if (val == "Custom") {
                outOptions.stylePreset = ClangFormatStylePreset::Custom;
            } else {
                if (outErrorMessage) {
                    *outErrorMessage = "BasedOnStyle no reconocido: " + std::string(val);
                }
                return false;
            }
        } else if (key == "IndentWidth") {
            hasIndentWidth = true;
            try {
                std::size_t processed = 0;
                const int parsed = std::stoi(std::string(val), &processed);
                if (processed != val.size()) {
                    if (outErrorMessage) {
                        *outErrorMessage = "IndentWidth contiene caracteres no numéricos";
                    }
                    return false;
                }
                outOptions.indentWidth = parsed;
            } catch (...) {
                if (outErrorMessage) {
                    *outErrorMessage = "IndentWidth no es un entero válido";
                }
                return false;
            }
        } else if (key == "ColumnLimit") {
            hasColumnLimit = true;
            try {
                std::size_t processed = 0;
                const int parsed = std::stoi(std::string(val), &processed);
                if (processed != val.size()) {
                    if (outErrorMessage) {
                        *outErrorMessage = "ColumnLimit contiene caracteres no numéricos";
                    }
                    return false;
                }
                outOptions.columnLimit = parsed;
            } catch (...) {
                if (outErrorMessage) {
                    *outErrorMessage = "ColumnLimit no es un entero válido";
                }
                return false;
            }
        } else if (key == "PointerAlignment") {
            if (val == "Left") {
                outOptions.pointerAlignment = PointerAlignmentPolicy::Left;
            } else if (val == "Right") {
                outOptions.pointerAlignment = PointerAlignmentPolicy::Right;
            } else if (val == "Middle") {
                outOptions.pointerAlignment = PointerAlignmentPolicy::Middle;
            } else {
                if (outErrorMessage) {
                    *outErrorMessage = "PointerAlignment no reconocido: " + std::string(val);
                }
                return false;
            }
        } else if (key == "AllowShortFunctionsOnASingleLine") {
            if (val == "None") {
                outOptions.allowShortFunctions = ShortFunctionPolicy::None;
            } else if (val == "Inline") {
                outOptions.allowShortFunctions = ShortFunctionPolicy::Inline;
            } else if (val == "Empty") {
                outOptions.allowShortFunctions = ShortFunctionPolicy::Empty;
            } else if (val == "All") {
                outOptions.allowShortFunctions = ShortFunctionPolicy::All;
            } else {
                if (outErrorMessage) {
                    *outErrorMessage =
                        "AllowShortFunctionsOnASingleLine no reconocido: " + std::string(val);
                }
                return false;
            }
        } else if (key == "Standard") {
            outOptions.cppStandard = std::string(val);
        }
    }

    if (!hasBasedOnStyle || !hasIndentWidth || !hasColumnLimit) {
        if (outErrorMessage) {
            *outErrorMessage = "Faltan claves obligatorias en la configuración de formato "
                               "(BasedOnStyle, ColumnLimit o IndentWidth)";
        }
        return false;
    }

    return true;
}

std::string ClangFormatValidator::serializeConfig(const ClangFormatOptions& options) {
    std::ostringstream oss;
    oss << "BasedOnStyle: " << stylePresetToString(options.stylePreset) << "\n";
    oss << "IndentWidth: " << options.indentWidth << "\n";
    oss << "ColumnLimit: " << options.columnLimit << "\n";
    oss << "PointerAlignment: " << pointerAlignmentToString(options.pointerAlignment) << "\n";
    oss << "AllowShortFunctionsOnASingleLine: "
        << shortFunctionPolicyToString(options.allowShortFunctions) << "\n";
    if (!options.cppStandard.empty()) {
        oss << "Standard: " << options.cppStandard << "\n";
    }
    return oss.str();
}

std::string ClangFormatValidator::formatStateToString(ClangFormatValidationState state) {
    switch (state) {
    case ClangFormatValidationState::Unconfigured:
        return "Configuración no inicializada";
    case ClangFormatValidationState::Configured:
        return "Configuración de formato detectada";
    case ClangFormatValidationState::Valid:
        return "Configuración de formato válida";
    case ClangFormatValidationState::Invalid:
        return "Configuración de formato no válida";
    case ClangFormatValidationState::DiscrepancyDetected:
        return "Se detectaron diferencias en el formato del código";
    }
    return "Estado de configuración desconocido";
}

std::string ClangFormatValidator::formatErrorToString(ClangFormatError error) {
    switch (error) {
    case ClangFormatError::None:
        return "Sin errores";
    case ClangFormatError::FileNotFound:
        return "Archivo de configuración no encontrado";
    case ClangFormatError::ParseError:
        return "Error al leer el archivo de configuración";
    case ClangFormatError::UnsupportedVersion:
        return "Versión de formato no soportada";
    case ClangFormatError::InvalidIndentWidth:
        return "El ancho de sangría no es válido";
    case ClangFormatError::InvalidColumnLimit:
        return "El límite de columnas no es válido";
    case ClangFormatError::UnsafeFilePath:
        return "Ruta de archivo no permitida";
    case ClangFormatError::CorruptedRules:
        return "Reglas de formato no válidas o corruptas";
    case ClangFormatError::FormattingDiscrepancy:
        return "Discrepancias en el formato del archivo";
    }
    return "Error de formato desconocido";
}

std::string ClangFormatValidator::stylePresetToString(ClangFormatStylePreset preset) {
    switch (preset) {
    case ClangFormatStylePreset::LLVM:
        return "LLVM";
    case ClangFormatStylePreset::Google:
        return "Google";
    case ClangFormatStylePreset::Chromium:
        return "Chromium";
    case ClangFormatStylePreset::Mozilla:
        return "Mozilla";
    case ClangFormatStylePreset::WebKit:
        return "WebKit";
    case ClangFormatStylePreset::Microsoft:
        return "Microsoft";
    case ClangFormatStylePreset::GNU:
        return "GNU";
    case ClangFormatStylePreset::Custom:
        return "Custom";
    }
    return "LLVM";
}

std::string ClangFormatValidator::pointerAlignmentToString(PointerAlignmentPolicy policy) {
    switch (policy) {
    case PointerAlignmentPolicy::Left:
        return "Left";
    case PointerAlignmentPolicy::Right:
        return "Right";
    case PointerAlignmentPolicy::Middle:
        return "Middle";
    }
    return "Left";
}

std::string ClangFormatValidator::shortFunctionPolicyToString(ShortFunctionPolicy policy) {
    switch (policy) {
    case ShortFunctionPolicy::None:
        return "None";
    case ShortFunctionPolicy::Inline:
        return "Inline";
    case ShortFunctionPolicy::Empty:
        return "Empty";
    case ShortFunctionPolicy::All:
        return "All";
    }
    return "None";
}

} // namespace vigilio
