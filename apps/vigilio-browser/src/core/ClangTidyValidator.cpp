#include "core/ClangTidyValidator.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

bool ClangTidyValidator::isSafeRegex(std::string_view regexStr) {
    if (regexStr.empty()) {
        return true;
    }

    if (regexStr.size() > 256) {
        return false;
    }

    for (char ch : regexStr) {
        if (static_cast<unsigned char>(ch) < 32 || ch == 127) {
            return false;
        }
    }

    try {
        const std::string pattern(regexStr);
        const std::regex compiled(pattern);
        (void)compiled;
    } catch (...) {
        return false;
    }

    return true;
}

bool ClangTidyValidator::isSafeConfigPath(const std::filesystem::path& path) {
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

    // Rechazo de rutas de sistema sensibles en entornos POSIX
    if (pathStr.starts_with("/etc") || pathStr.starts_with("/var") || pathStr.starts_with("/root") ||
        pathStr.starts_with("/sys") || pathStr.starts_with("/proc") || pathStr.starts_with("/dev") ||
        pathStr.starts_with("/boot") || pathStr.starts_with("/usr") || pathStr.starts_with("/bin") ||
        pathStr.starts_with("/sbin")) {
        return false;
    }

    // Rechazo de rutas absolutas de sistema en entornos Windows
    if (pathStr.find(":\\Windows") != std::string::npos ||
        pathStr.find(":/Windows") != std::string::npos) {
        return false;
    }

    return true;
}

void ClangTidyValidator::parseCheckTokens(std::string_view checksStr,
                                          std::vector<std::string>& outEnabled,
                                          std::vector<std::string>& outDisabled) {
    outEnabled.clear();
    outDisabled.clear();

    std::size_t start = 0;
    while (start < checksStr.size()) {
        const std::size_t end = checksStr.find(',', start);
        std::string_view token = (end == std::string_view::npos) ? checksStr.substr(start)
                                                                 : checksStr.substr(start, end - start);
        start = (end == std::string_view::npos) ? checksStr.size() : end + 1;

        token = trim(token);
        token = stripQuotes(token);
        if (token.empty()) {
            continue;
        }

        if (token.front() == '-') {
            outDisabled.push_back(std::string(token));
        } else if (token.front() == '+') {
            outEnabled.push_back(std::string(token.substr(1)));
        } else {
            outEnabled.push_back(std::string(token));
        }
    }
}

ClangTidyError ClangTidyValidator::validateOptions(const ClangTidyOptions& options) {
    if (options.checks.empty() || trim(options.checks).empty()) {
        return ClangTidyError::EmptyChecks;
    }

    for (char ch : options.checks) {
        if (ch == '\0') {
            return ClangTidyError::CorruptedRules;
        }
    }

    std::vector<std::string> enabled;
    std::vector<std::string> disabled;
    parseCheckTokens(options.checks, enabled, disabled);

    if (enabled.empty()) {
        return ClangTidyError::EmptyChecks;
    }

    if (!options.headerFilterRegex.empty()) {
        if (!isSafeRegex(options.headerFilterRegex)) {
            return ClangTidyError::InvalidHeaderFilterRegex;
        }
    }

    return ClangTidyError::None;
}

bool ClangTidyValidator::parseConfig(std::string_view yamlContent, ClangTidyOptions& outOptions,
                                     std::string* outErrorMessage) {
    bool hasChecks = false;

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

        if (key == "Checks") {
            hasChecks = true;
            outOptions.checks = std::string(val);
        } else if (key == "WarningsAsErrors") {
            outOptions.warningsAsErrors = std::string(val);
        } else if (key == "HeaderFilterRegex") {
            outOptions.headerFilterRegex = std::string(val);
        } else if (key == "FormatStyle") {
            outOptions.formatStyle = std::string(val);
        }
    }

    if (!hasChecks) {
        if (outErrorMessage) {
            *outErrorMessage = "Falta directiva obligatoria: Checks";
        }
        return false;
    }

    return true;
}

ClangTidyValidationReport ClangTidyValidator::validateConfigContent(std::string_view yamlContent) {
    ClangTidyValidationReport report;
    report.configPath = "<in-memory>";

    if (yamlContent.empty() || trim(yamlContent).empty()) {
        report.isValid = false;
        report.error = ClangTidyError::ParseError;
        report.errorMessage = tidyErrorToString(report.error);
        return report;
    }

    // Comprobación de bytes nulos o caracteres binarios
    for (char ch : yamlContent) {
        if (ch == '\0') {
            report.isValid = false;
            report.error = ClangTidyError::ParseError;
            report.errorMessage = tidyErrorToString(report.error);
            return report;
        }
    }

    ClangTidyOptions options;
    std::string parseErrMsg;
    if (!parseConfig(yamlContent, options, &parseErrMsg)) {
        report.isValid = false;
        if (parseErrMsg.find("Checks") != std::string::npos) {
            report.error = ClangTidyError::MissingMandatoryKey;
        } else {
            report.error = ClangTidyError::ParseError;
        }
        report.errorMessage = tidyErrorToString(report.error);
        return report;
    }

    const ClangTidyError optErr = validateOptions(options);
    if (optErr != ClangTidyError::None) {
        report.isValid = false;
        report.error = optErr;
        report.errorMessage = tidyErrorToString(optErr);
        return report;
    }

    report.isValid = true;
    report.error = ClangTidyError::None;
    report.errorMessage = tidyErrorToString(report.error);
    parseCheckTokens(options.checks, report.enabledChecks, report.disabledChecks);
    return report;
}

ClangTidyValidationReport
ClangTidyValidator::validateConfigurationFile(const std::filesystem::path& configPath) {
    ClangTidyValidationReport report;
    report.configPath = configPath.string();

    if (!isSafeConfigPath(configPath)) {
        report.isValid = false;
        report.error = ClangTidyError::UnsafeFilePath;
        report.errorMessage = tidyErrorToString(report.error);
        return report;
    }

    std::error_code ec;
    if (!std::filesystem::exists(configPath, ec) || ec ||
        !std::filesystem::is_regular_file(configPath, ec)) {
        report.isValid = false;
        report.error = ClangTidyError::FileNotFound;
        report.errorMessage = tidyErrorToString(report.error);
        return report;
    }

    std::ifstream file(configPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        report.isValid = false;
        report.error = ClangTidyError::FileNotFound;
        report.errorMessage = tidyErrorToString(report.error);
        return report;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    report = validateConfigContent(content);
    report.configPath = configPath.string();
    return report;
}

std::string ClangTidyValidator::serializeConfig(const ClangTidyOptions& options) {
    std::ostringstream oss;
    oss << "Checks: '" << options.checks << "'\n";
    oss << "WarningsAsErrors: '" << options.warningsAsErrors << "'\n";
    oss << "HeaderFilterRegex: '" << options.headerFilterRegex << "'\n";
    if (!options.formatStyle.empty()) {
        oss << "FormatStyle: '" << options.formatStyle << "'\n";
    }
    return oss.str();
}

std::string ClangTidyValidator::tidyStateToString(ClangTidyValidationState state) {
    switch (state) {
    case ClangTidyValidationState::Unconfigured:
        return "Análisis de calidad no configurado";
    case ClangTidyValidationState::Configured:
        return "Análisis de calidad configurado";
    case ClangTidyValidationState::Valid:
        return "Configuración de análisis válida";
    case ClangTidyValidationState::Invalid:
        return "Configuración de análisis no válida";
    case ClangTidyValidationState::DiscrepancyDetected:
        return "Discrepancias detectadas en el análisis";
    }
    return "Estado de análisis desconocido";
}

std::string ClangTidyValidator::tidyErrorToString(ClangTidyError error) {
    switch (error) {
    case ClangTidyError::None:
        return "Sin errores en la configuración";
    case ClangTidyError::FileNotFound:
        return "No se encontró el archivo de configuración";
    case ClangTidyError::ParseError:
        return "Error al interpretar el archivo de configuración";
    case ClangTidyError::EmptyChecks:
        return "No se han especificado reglas de análisis activas";
    case ClangTidyError::InvalidHeaderFilterRegex:
        return "La expresión de filtrado de archivos no es válida";
    case ClangTidyError::UnsafeFilePath:
        return "La ruta del archivo de configuración no es segura";
    case ClangTidyError::CorruptedRules:
        return "Las reglas de análisis contienen valores corruptos";
    case ClangTidyError::MissingMandatoryKey:
        return "Faltan directivas obligatorias en la configuración";
    }
    return "Error de configuración desconocido";
}

std::string ClangTidyValidator::categoryToString(ClangTidyCheckCategory category) {
    switch (category) {
    case ClangTidyCheckCategory::Bugprone:
        return "Bugprone";
    case ClangTidyCheckCategory::ClangAnalyzer:
        return "ClangAnalyzer";
    case ClangTidyCheckCategory::Performance:
        return "Performance";
    case ClangTidyCheckCategory::Modernize:
        return "Modernize";
    case ClangTidyCheckCategory::Readability:
        return "Readability";
    case ClangTidyCheckCategory::CppCoreGuidelines:
        return "CppCoreGuidelines";
    case ClangTidyCheckCategory::Portability:
        return "Portability";
    case ClangTidyCheckCategory::Custom:
        return "Custom";
    }
    return "Custom";
}

} // namespace vigilio
