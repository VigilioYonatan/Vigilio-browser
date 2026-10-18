#include "core/CMakePresetsValidator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <string_view>
#include <utility>

namespace vigilio {
namespace {

enum class JsonValueType { Null, Boolean, Number, String, Array, Object };

struct SimpleJsonValue {
    JsonValueType type{JsonValueType::Null};
    bool boolVal{false};
    double numVal{0.0};
    std::string strVal;
    std::vector<SimpleJsonValue> arrVal;
    std::vector<std::pair<std::string, SimpleJsonValue>> objVal;

    [[nodiscard]] const SimpleJsonValue* find(std::string_view key) const {
        if (type != JsonValueType::Object) {
            return nullptr;
        }
        for (const auto& [k, v] : objVal) {
            if (k == key) {
                return &v;
            }
        }
        return nullptr;
    }
};

class SimpleJsonParser {
  public:
    explicit SimpleJsonParser(std::string_view text) : m_text(text), m_pos(0) {
    }

    bool parse(SimpleJsonValue& out) {
        skipWhitespace();
        if (m_pos >= m_text.size()) {
            return false;
        }
        if (!parseValue(out)) {
            return false;
        }
        skipWhitespace();
        return m_pos == m_text.size();
    }

  private:
    std::string_view m_text;
    std::size_t m_pos{0};

    void skipWhitespace() {
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++m_pos;
            } else {
                break;
            }
        }
    }

    bool parseValue(SimpleJsonValue& out) {
        skipWhitespace();
        if (m_pos >= m_text.size()) {
            return false;
        }
        const char c = m_text[m_pos];
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '[') {
            return parseArray(out);
        }
        if (c == '"') {
            return parseString(out.strVal) && ((out.type = JsonValueType::String), true);
        }
        if (c == 't' || c == 'f') {
            return parseBool(out);
        }
        if (c == 'n') {
            return parseNull(out);
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(out);
        }
        return false;
    }

    bool parseObject(SimpleJsonValue& out) {
        out.type = JsonValueType::Object;
        out.objVal.clear();
        ++m_pos; // skip '{'
        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
                return false;
            }
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
                return false;
            }
            ++m_pos; // skip ':'
            SimpleJsonValue val;
            if (!parseValue(val)) {
                return false;
            }
            out.objVal.emplace_back(std::move(key), std::move(val));
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                skipWhitespace();
                // Trailing comma check
                if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                    return false;
                }
            } else if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                ++m_pos;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseArray(SimpleJsonValue& out) {
        out.type = JsonValueType::Array;
        out.arrVal.clear();
        ++m_pos; // skip '['
        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            SimpleJsonValue val;
            if (!parseValue(val)) {
                return false;
            }
            out.arrVal.emplace_back(std::move(val));
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                skipWhitespace();
                if (m_pos < m_text.size() && m_text[m_pos] == ']') {
                    return false;
                }
            } else if (m_pos < m_text.size() && m_text[m_pos] == ']') {
                ++m_pos;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseString(std::string& out) {
        out.clear();
        if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
            return false;
        }
        ++m_pos; // skip opening '"'
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos++];
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (m_pos >= m_text.size()) {
                    return false;
                }
                const char esc = m_text[m_pos++];
                switch (esc) {
                case '"':
                    out += '"';
                    break;
                case '\\':
                    out += '\\';
                    break;
                case '/':
                    out += '/';
                    break;
                case 'b':
                    out += '\b';
                    break;
                case 'f':
                    out += '\f';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                default:
                    out += esc;
                    break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }

    bool parseBool(SimpleJsonValue& out) {
        if (m_text.substr(m_pos, 4) == "true") {
            out.type = JsonValueType::Boolean;
            out.boolVal = true;
            m_pos += 4;
            return true;
        }
        if (m_text.substr(m_pos, 5) == "false") {
            out.type = JsonValueType::Boolean;
            out.boolVal = false;
            m_pos += 5;
            return true;
        }
        return false;
    }

    bool parseNull(SimpleJsonValue& out) {
        if (m_text.substr(m_pos, 4) == "null") {
            out.type = JsonValueType::Null;
            m_pos += 4;
            return true;
        }
        return false;
    }

    bool parseNumber(SimpleJsonValue& out) {
        const std::size_t start = m_pos;
        if (m_pos < m_text.size() && m_text[m_pos] == '-') {
            ++m_pos;
        }
        while (m_pos < m_text.size() &&
               ((m_text[m_pos] >= '0' && m_text[m_pos] <= '9') || m_text[m_pos] == '.' ||
                m_text[m_pos] == 'e' || m_text[m_pos] == 'E' || m_text[m_pos] == '+' ||
                m_text[m_pos] == '-')) {
            ++m_pos;
        }
        std::string numStr(m_text.substr(start, m_pos - start));
        try {
            out.type = JsonValueType::Number;
            out.numVal = std::stod(numStr);
            return true;
        } catch (...) {
            return false;
        }
    }
};

bool isTruthy(const SimpleJsonValue& val) {
    if (val.type == JsonValueType::Boolean) {
        return val.boolVal;
    }
    if (val.type == JsonValueType::String) {
        std::string s = val.strVal;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s == "ON" || s == "TRUE" || s == "1";
    }
    if (val.type == JsonValueType::Number) {
        return val.numVal != 0.0;
    }
    return false;
}

bool isFalsy(const SimpleJsonValue& val) {
    if (val.type == JsonValueType::Boolean) {
        return !val.boolVal;
    }
    if (val.type == JsonValueType::String) {
        std::string s = val.strVal;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s == "OFF" || s == "FALSE" || s == "0";
    }
    if (val.type == JsonValueType::Number) {
        return val.numVal == 0.0;
    }
    return false;
}

bool isValidPresetName(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

} // namespace

bool CMakePresetsValidator::isSafeBinaryDir(const std::string& binaryDir) {
    if (binaryDir.empty()) {
        return false;
    }
    // Rechazo terminante de directory traversal
    if (binaryDir.find("..") != std::string::npos) {
        return false;
    }
    // Rechazo de rutas absolutas fijadas al sistema de archivos host (POSIX o Windows)
    if (binaryDir.front() == '/' || binaryDir.front() == '\\') {
        return false;
    }
    if (binaryDir.size() >= 2 && std::isalpha(static_cast<unsigned char>(binaryDir[0])) &&
        binaryDir[1] == ':') {
        return false;
    }

    // Convención canónica: dentro de build-* o _build* (opcionalmente precedido por ${sourceDir}/)
    constexpr std::string_view prefix = "${sourceDir}/";
    std::string_view target = binaryDir;
    if (target.substr(0, prefix.size()) == prefix) {
        target = target.substr(prefix.size());
    }

    if (target.rfind("build", 0) == 0 || target.rfind("_build", 0) == 0) {
        return true;
    }
    return false;
}

std::vector<std::string> CMakePresetsValidator::canonicalRequiredPresets() {
    return {"core-debug", "desktop-debug"};
}

std::vector<std::string> CMakePresetsValidator::canonicalPresetNames() {
    return canonicalRequiredPresets();
}

std::vector<std::string> CMakePresetsValidator::canonicalAllPresets() {
    return {"core-debug",     "core-release",     "desktop-debug",  "desktop-release", "desktop-e2e",
            "ci-linux-debug", "ci-linux-release", "sanitizer-asan", "sanitizer-ubsan"};
}

std::string CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome outcome) {
    switch (outcome) {
    case CMakePresetValidationOutcome::Valid:
        return "Configuración de presets válida";
    case CMakePresetValidationOutcome::InvalidSchemaVersion:
        return "Versión de esquema no compatible (se requiere versión >= 3)";
    case CMakePresetValidationOutcome::MissingRequiredPreset:
        return "Preset obligatorio faltante (se requieren core-debug y desktop-debug)";
    case CMakePresetValidationOutcome::OrphanConfigurePresetReference:
        return "Referencia huérfana a preset de configuración inexistente";
    case CMakePresetValidationOutcome::DuplicatePresetName:
        return "Nombre de preset duplicado en la misma categoría";
    case CMakePresetValidationOutcome::UnsafeBinaryDir:
        return "Ruta de directorio binario no segura (rechazada por directorio traversal o ruta "
               "absoluta)";
    case CMakePresetValidationOutcome::ConflictingVariables:
        return "Variables de caché incompatibles o contradictorias";
    case CMakePresetValidationOutcome::MalformedJson:
        return "Documento JSON malformado o ilegible";
    default:
        return "Resultado de validación desconocido";
    }
}

CMakePresetsValidationReport CMakePresetsValidator::validatePresetsJson(const std::string& jsonContent) {
    CMakePresetsValidationReport report;

    // Validación previa de contenido vacío o no imprimible
    if (jsonContent.empty()) {
        report.outcome = CMakePresetValidationOutcome::MalformedJson;
        report.message = outcomeToString(report.outcome) + ": contenido vacío";
        return report;
    }

    SimpleJsonValue root;
    SimpleJsonParser parser(jsonContent);
    if (!parser.parse(root) || root.type != JsonValueType::Object) {
        report.outcome = CMakePresetValidationOutcome::MalformedJson;
        report.message = outcomeToString(report.outcome);
        return report;
    }

    // 1. Verificación de versión de esquema
    const auto* versionVal = root.find("version");
    if (!versionVal || versionVal->type != JsonValueType::Number) {
        report.outcome = CMakePresetValidationOutcome::InvalidSchemaVersion;
        report.message = outcomeToString(report.outcome) + ": campo 'version' ausente o no numérico";
        return report;
    }

    report.schemaVersion = static_cast<int>(versionVal->numVal);
    if (report.schemaVersion < 3) {
        report.outcome = CMakePresetValidationOutcome::InvalidSchemaVersion;
        report.message = outcomeToString(report.outcome) +
                         " (detectada: " + std::to_string(report.schemaVersion) + ")";
        return report;
    }

    // 2. Parseo y validación de configurePresets
    std::set<std::string> configurePresetNames;
    const auto* configureArr = root.find("configurePresets");
    if (configureArr) {
        if (configureArr->type != JsonValueType::Array) {
            report.outcome = CMakePresetValidationOutcome::MalformedJson;
            report.message = "configurePresets debe ser un arreglo JSON";
            return report;
        }

        for (const auto& item : configureArr->arrVal) {
            if (item.type != JsonValueType::Object) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                report.message = "Elemento de configurePresets no es un objeto JSON válido";
                return report;
            }

            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonValueType::String || nameVal->strVal.empty()) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                report.message = "Preset de configuración carece de 'name' válido";
                return report;
            }

            const std::string& presetName = nameVal->strVal;
            if (!isValidPresetName(presetName)) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                report.message = "Nombre de preset contiene caracteres no permitidos: " + presetName;
                return report;
            }
            if (!configurePresetNames.insert(presetName).second) {
                report.outcome = CMakePresetValidationOutcome::DuplicatePresetName;
                report.message = outcomeToString(report.outcome) + ": " + presetName;
                return report;
            }
            report.configurePresets.push_back(presetName);

            // Validación de binaryDir
            const auto* binDirVal = item.find("binaryDir");
            if (binDirVal && binDirVal->type == JsonValueType::String) {
                if (!isSafeBinaryDir(binDirVal->strVal)) {
                    report.outcome = CMakePresetValidationOutcome::UnsafeBinaryDir;
                    report.message = outcomeToString(report.outcome) + " en preset '" + presetName +
                                     "': " + binDirVal->strVal;
                    return report;
                }
            }

            // Validación de cacheVariables y coherencia de flags
            const auto* cacheVarsVal = item.find("cacheVariables");
            if (cacheVarsVal) {
                if (cacheVarsVal->type != JsonValueType::Object) {
                    report.outcome = CMakePresetValidationOutcome::MalformedJson;
                    report.message =
                        "cacheVariables en preset '" + presetName + "' debe ser un objeto JSON";
                    return report;
                }
                const auto* guiVal = cacheVarsVal->find("VIGILIO_BUILD_GUI");
                const auto* e2eVal = cacheVarsVal->find("VIGILIO_ENABLE_E2E");
                if (guiVal && e2eVal) {
                    if (isFalsy(*guiVal) && isTruthy(*e2eVal)) {
                        report.outcome = CMakePresetValidationOutcome::ConflictingVariables;
                        report.message =
                            outcomeToString(report.outcome) + " en preset '" + presetName +
                            "': VIGILIO_BUILD_GUI=OFF concurrente con VIGILIO_ENABLE_E2E=ON";
                        return report;
                    }
                }
            }
        }
    }

    // 3. Verificación de catálogo mínimo obligatorio
    const auto required = canonicalRequiredPresets();
    for (const auto& req : required) {
        if (configurePresetNames.find(req) == configurePresetNames.end()) {
            report.outcome = CMakePresetValidationOutcome::MissingRequiredPreset;
            report.message = outcomeToString(report.outcome) + ": '" + req + "'";
            return report;
        }
    }

    // 4. Parseo y validación de buildPresets
    std::set<std::string> buildPresetNames;
    const auto* buildArr = root.find("buildPresets");
    if (buildArr) {
        if (buildArr->type != JsonValueType::Array) {
            report.outcome = CMakePresetValidationOutcome::MalformedJson;
            report.message = "buildPresets debe ser un arreglo JSON";
            return report;
        }

        for (const auto& item : buildArr->arrVal) {
            if (item.type != JsonValueType::Object) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                return report;
            }

            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonValueType::String || nameVal->strVal.empty()) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                return report;
            }
            const std::string& buildName = nameVal->strVal;
            if (!isValidPresetName(buildName)) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                report.message =
                    "Nombre de preset de compilación contiene caracteres no permitidos: " + buildName;
                return report;
            }
            if (!buildPresetNames.insert(buildName).second) {
                report.outcome = CMakePresetValidationOutcome::DuplicatePresetName;
                report.message = outcomeToString(report.outcome) + " en buildPresets: " + buildName;
                return report;
            }
            report.buildPresets.push_back(buildName);

            // Integridad referencial: configurePreset debe existir
            const auto* cfgRefVal = item.find("configurePreset");
            if (!cfgRefVal || cfgRefVal->type != JsonValueType::String ||
                configurePresetNames.find(cfgRefVal->strVal) == configurePresetNames.end()) {
                report.outcome = CMakePresetValidationOutcome::OrphanConfigurePresetReference;
                report.message = outcomeToString(report.outcome) + " en build preset '" + buildName +
                                 "' hacia '" + (cfgRefVal ? cfgRefVal->strVal : "null") + "'";
                return report;
            }
        }
    }

    // 5. Parseo y validación de testPresets
    std::set<std::string> testPresetNames;
    const auto* testArr = root.find("testPresets");
    if (testArr) {
        if (testArr->type != JsonValueType::Array) {
            report.outcome = CMakePresetValidationOutcome::MalformedJson;
            report.message = "testPresets debe ser un arreglo JSON";
            return report;
        }

        for (const auto& item : testArr->arrVal) {
            if (item.type != JsonValueType::Object) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                return report;
            }

            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonValueType::String || nameVal->strVal.empty()) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                return report;
            }
            const std::string& testName = nameVal->strVal;
            if (!isValidPresetName(testName)) {
                report.outcome = CMakePresetValidationOutcome::MalformedJson;
                report.message =
                    "Nombre de preset de prueba contiene caracteres no permitidos: " + testName;
                return report;
            }
            if (!testPresetNames.insert(testName).second) {
                report.outcome = CMakePresetValidationOutcome::DuplicatePresetName;
                report.message = outcomeToString(report.outcome) + " en testPresets: " + testName;
                return report;
            }
            report.testPresets.push_back(testName);

            // Integridad referencial: configurePreset debe existir
            const auto* cfgRefVal = item.find("configurePreset");
            if (!cfgRefVal || cfgRefVal->type != JsonValueType::String ||
                configurePresetNames.find(cfgRefVal->strVal) == configurePresetNames.end()) {
                report.outcome = CMakePresetValidationOutcome::OrphanConfigurePresetReference;
                report.message = outcomeToString(report.outcome) + " en test preset '" + testName +
                                 "' hacia '" + (cfgRefVal ? cfgRefVal->strVal : "null") + "'";
                return report;
            }
        }
    }

    // 6. Parseo de packagePresets (opcional)
    const auto* packageArr = root.find("packagePresets");
    if (packageArr && packageArr->type == JsonValueType::Array) {
        for (const auto& item : packageArr->arrVal) {
            if (item.type == JsonValueType::Object) {
                const auto* nameVal = item.find("name");
                if (nameVal && nameVal->type == JsonValueType::String) {
                    report.packagePresets.push_back(nameVal->strVal);
                }
            }
        }
    }

    // 7. Parseo de workflowPresets (opcional)
    const auto* workflowArr = root.find("workflowPresets");
    if (workflowArr && workflowArr->type == JsonValueType::Array) {
        for (const auto& item : workflowArr->arrVal) {
            if (item.type == JsonValueType::Object) {
                const auto* nameVal = item.find("name");
                if (nameVal && nameVal->type == JsonValueType::String) {
                    report.workflowPresets.push_back(nameVal->strVal);
                }
            }
        }
    }

    report.outcome = CMakePresetValidationOutcome::Valid;
    report.message = outcomeToString(report.outcome);
    return report;
}

CMakePresetsValidationReport
CMakePresetsValidator::validatePresetsFile(const std::filesystem::path& path) {
    CMakePresetsValidationReport report;
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        report.outcome = CMakePresetValidationOutcome::MalformedJson;
        report.message = "No fue posible abrir el archivo de presets: " + path.string();
        return report;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return validatePresetsJson(ss.str());
}

} // namespace vigilio
