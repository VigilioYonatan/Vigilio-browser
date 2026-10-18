#include "core/CMakePresetsService.h"
#include "core/CMakePresetsValidator.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace vigilio {
namespace {

enum class JsonType { Null, Boolean, Number, String, Array, Object };

struct JsonVal {
    JsonType type{JsonType::Null};
    bool boolVal{false};
    double numVal{0.0};
    std::string strVal;
    std::vector<JsonVal> arrVal;
    std::vector<std::pair<std::string, JsonVal>> objVal;

    [[nodiscard]] const JsonVal* find(std::string_view key) const {
        if (type != JsonType::Object) {
            return nullptr;
        }
        for (const auto& [k, v] : objVal) {
            if (k == key) {
                return &v;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::string asString() const {
        if (type == JsonType::String) {
            return strVal;
        }
        if (type == JsonType::Boolean) {
            return boolVal ? "ON" : "OFF";
        }
        if (type == JsonType::Number) {
            if (numVal == static_cast<int64_t>(numVal)) {
                return std::to_string(static_cast<int64_t>(numVal));
            }
            return std::to_string(numVal);
        }
        return "";
    }
};

class SimpleParser {
  public:
    explicit SimpleParser(std::string_view text) : m_text(text), m_pos(0) {
    }

    bool parse(JsonVal& out) {
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

    bool parseValue(JsonVal& out) {
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
            return parseString(out.strVal) && ((out.type = JsonType::String), true);
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

    bool parseObject(JsonVal& out) {
        out.type = JsonType::Object;
        out.objVal.clear();
        ++m_pos;
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
            ++m_pos;
            JsonVal val;
            if (!parseValue(val)) {
                return false;
            }
            out.objVal.emplace_back(std::move(key), std::move(val));
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                skipWhitespace();
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

    bool parseArray(JsonVal& out) {
        out.type = JsonType::Array;
        out.arrVal.clear();
        ++m_pos;
        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            JsonVal val;
            if (!parseValue(val)) {
                return false;
            }
            out.arrVal.push_back(std::move(val));
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
        ++m_pos;
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c == '"') {
                ++m_pos;
                return true;
            }
            if (c == '\\') {
                ++m_pos;
                if (m_pos >= m_text.size()) {
                    return false;
                }
                const char esc = m_text[m_pos];
                switch (esc) {
                case '"':
                case '\\':
                case '/':
                    out.push_back(esc);
                    break;
                case 'b':
                    out.push_back('\b');
                    break;
                case 'f':
                    out.push_back('\f');
                    break;
                case 'n':
                    out.push_back('\n');
                    break;
                case 'r':
                    out.push_back('\r');
                    break;
                case 't':
                    out.push_back('\t');
                    break;
                default:
                    out.push_back(esc);
                    break;
                }
                ++m_pos;
            } else {
                out.push_back(c);
                ++m_pos;
            }
        }
        return false;
    }

    bool parseBool(JsonVal& out) {
        if (m_text.substr(m_pos, 4) == "true") {
            out.type = JsonType::Boolean;
            out.boolVal = true;
            m_pos += 4;
            return true;
        }
        if (m_text.substr(m_pos, 5) == "false") {
            out.type = JsonType::Boolean;
            out.boolVal = false;
            m_pos += 5;
            return true;
        }
        return false;
    }

    bool parseNull(JsonVal& out) {
        if (m_text.substr(m_pos, 4) == "null") {
            out.type = JsonType::Null;
            m_pos += 4;
            return true;
        }
        return false;
    }

    bool parseNumber(JsonVal& out) {
        std::size_t start = m_pos;
        if (m_pos < m_text.size() && m_text[m_pos] == '-') {
            ++m_pos;
        }
        while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') {
            ++m_pos;
        }
        if (m_pos < m_text.size() && m_text[m_pos] == '.') {
            ++m_pos;
            while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') {
                ++m_pos;
            }
        }
        if (start == m_pos) {
            return false;
        }
        std::string numStr(m_text.substr(start, m_pos - start));
        try {
            out.type = JsonType::Number;
            out.numVal = std::stod(numStr);
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace

bool CMakePresetsService::loadFromFile(const std::filesystem::path& presetsPath) {
    std::ifstream file(presetsPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        isLoaded_ = false;
        report_.outcome = CMakePresetValidationOutcome::MalformedJson;
        report_.message = "No fue posible abrir el archivo de presets: " + presetsPath.string();
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::filesystem::path parentDir = presetsPath.parent_path();
    return loadFromJson(ss.str(), parentDir.empty() ? "." : parentDir);
}

bool CMakePresetsService::loadFromJson(const std::string& jsonContent,
                                       const std::filesystem::path& sourceDir) {
    sourceDirectory_ = sourceDir.empty() ? "." : sourceDir;
    configurePresets_.clear();
    buildPresets_.clear();
    testPresets_.clear();
    packagePresets_.clear();
    workflowPresets_.clear();

    report_ = CMakePresetsValidator::validatePresetsJson(jsonContent);

    SimpleParser parser(jsonContent);
    JsonVal root;
    if (!parser.parse(root) || root.type != JsonType::Object) {
        isLoaded_ = false;
        return false;
    }

    // 1. configurePresets
    const auto* cfgArr = root.find("configurePresets");
    if (cfgArr && cfgArr->type == JsonType::Array) {
        for (const auto& item : cfgArr->arrVal) {
            if (item.type != JsonType::Object) {
                continue;
            }
            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonType::String) {
                continue;
            }

            CMakePresetDefinition def;
            def.kind = CMakePresetKind::Configure;
            def.name = nameVal->strVal;

            if (const auto* dName = item.find("displayName"); dName && dName->type == JsonType::String) {
                def.displayName = dName->strVal;
            }
            if (const auto* desc = item.find("description"); desc && desc->type == JsonType::String) {
                def.description = desc->strVal;
            }
            if (const auto* gen = item.find("generator"); gen && gen->type == JsonType::String) {
                def.generator = gen->strVal;
            } else {
                def.generator = "Ninja";
            }
            if (const auto* bin = item.find("binaryDir"); bin && bin->type == JsonType::String) {
                def.binaryDir = bin->strVal;
            }
            if (const auto* cacheVars = item.find("cacheVariables");
                cacheVars && cacheVars->type == JsonType::Object) {
                for (const auto& [k, v] : cacheVars->objVal) {
                    def.cacheVariables[k] = v.asString();
                }
            }
            if (const auto* env = item.find("environment"); env && env->type == JsonType::Object) {
                for (const auto& [k, v] : env->objVal) {
                    def.environment[k] = v.asString();
                }
            }
            configurePresets_.push_back(std::move(def));
        }
    }

    // 2. buildPresets
    const auto* bldArr = root.find("buildPresets");
    if (bldArr && bldArr->type == JsonType::Array) {
        for (const auto& item : bldArr->arrVal) {
            if (item.type != JsonType::Object) {
                continue;
            }
            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonType::String) {
                continue;
            }

            CMakePresetDefinition def;
            def.kind = CMakePresetKind::Build;
            def.name = nameVal->strVal;

            if (const auto* dName = item.find("displayName"); dName && dName->type == JsonType::String) {
                def.displayName = dName->strVal;
            }
            if (const auto* desc = item.find("description"); desc && desc->type == JsonType::String) {
                def.description = desc->strVal;
            }
            if (const auto* cfg = item.find("configurePreset"); cfg && cfg->type == JsonType::String) {
                def.configurePreset = cfg->strVal;
            }
            if (const auto* env = item.find("environment"); env && env->type == JsonType::Object) {
                for (const auto& [k, v] : env->objVal) {
                    def.environment[k] = v.asString();
                }
            }
            buildPresets_.push_back(std::move(def));
        }
    }

    // 3. testPresets
    const auto* tstArr = root.find("testPresets");
    if (tstArr && tstArr->type == JsonType::Array) {
        for (const auto& item : tstArr->arrVal) {
            if (item.type != JsonType::Object) {
                continue;
            }
            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonType::String) {
                continue;
            }

            CMakePresetDefinition def;
            def.kind = CMakePresetKind::Test;
            def.name = nameVal->strVal;

            if (const auto* dName = item.find("displayName"); dName && dName->type == JsonType::String) {
                def.displayName = dName->strVal;
            }
            if (const auto* desc = item.find("description"); desc && desc->type == JsonType::String) {
                def.description = desc->strVal;
            }
            if (const auto* cfg = item.find("configurePreset"); cfg && cfg->type == JsonType::String) {
                def.configurePreset = cfg->strVal;
            }
            if (const auto* env = item.find("environment"); env && env->type == JsonType::Object) {
                for (const auto& [k, v] : env->objVal) {
                    def.environment[k] = v.asString();
                }
            }
            if (const auto* outObj = item.find("output"); outObj && outObj->type == JsonType::Object) {
                if (const auto* oof = outObj->find("outputOnFailure");
                    oof && oof->type == JsonType::Boolean) {
                    def.outputOnFailure = oof->boolVal;
                }
            }
            testPresets_.push_back(std::move(def));
        }
    }

    // 4. packagePresets
    const auto* pkgArr = root.find("packagePresets");
    if (pkgArr && pkgArr->type == JsonType::Array) {
        for (const auto& item : pkgArr->arrVal) {
            if (item.type != JsonType::Object) {
                continue;
            }
            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonType::String) {
                continue;
            }

            CMakePresetDefinition def;
            def.kind = CMakePresetKind::Package;
            def.name = nameVal->strVal;
            packagePresets_.push_back(std::move(def));
        }
    }

    // 5. workflowPresets
    const auto* wfArr = root.find("workflowPresets");
    if (wfArr && wfArr->type == JsonType::Array) {
        for (const auto& item : wfArr->arrVal) {
            if (item.type != JsonType::Object) {
                continue;
            }
            const auto* nameVal = item.find("name");
            if (!nameVal || nameVal->type != JsonType::String) {
                continue;
            }

            CMakePresetDefinition def;
            def.kind = CMakePresetKind::Workflow;
            def.name = nameVal->strVal;
            workflowPresets_.push_back(std::move(def));
        }
    }

    isLoaded_ = report_.isValid();
    return isLoaded_;
}

bool CMakePresetsService::isLoaded() const noexcept {
    return isLoaded_;
}

const CMakePresetsValidationReport& CMakePresetsService::validationReport() const noexcept {
    return report_;
}

CMakePresetsSummary CMakePresetsService::summary() const {
    CMakePresetsSummary s;
    s.schemaVersion = report_.schemaVersion;
    s.totalConfigurePresets = configurePresets_.size();
    s.totalBuildPresets = buildPresets_.size();
    s.totalTestPresets = testPresets_.size();
    s.totalPackagePresets = packagePresets_.size();
    s.totalWorkflowPresets = workflowPresets_.size();

    s.configurePresetNames.reserve(configurePresets_.size());
    for (const auto& p : configurePresets_) {
        s.configurePresetNames.push_back(p.name);
    }
    s.buildPresetNames.reserve(buildPresets_.size());
    for (const auto& p : buildPresets_) {
        s.buildPresetNames.push_back(p.name);
    }
    s.testPresetNames.reserve(testPresets_.size());
    for (const auto& p : testPresets_) {
        s.testPresetNames.push_back(p.name);
    }
    return s;
}

std::optional<CMakePresetDefinition> CMakePresetsService::findPreset(std::string_view name,
                                                                     CMakePresetKind kind) const {
    const auto list = listPresetsByKind(kind);
    for (const auto& p : list) {
        if (p.name == name) {
            return p;
        }
    }
    return std::nullopt;
}

std::vector<CMakePresetDefinition> CMakePresetsService::listPresetsByKind(CMakePresetKind kind) const {
    switch (kind) {
    case CMakePresetKind::Configure:
        return configurePresets_;
    case CMakePresetKind::Build:
        return buildPresets_;
    case CMakePresetKind::Test:
        return testPresets_;
    case CMakePresetKind::Package:
        return packagePresets_;
    case CMakePresetKind::Workflow:
        return workflowPresets_;
    }
    return {};
}

std::string CMakePresetsService::resolveAssociatedConfigurePreset(std::string_view presetName,
                                                                  CMakePresetKind kind) const {
    if (kind == CMakePresetKind::Configure) {
        if (findPreset(presetName, CMakePresetKind::Configure).has_value()) {
            return std::string(presetName);
        }
        return {};
    }
    if (kind == CMakePresetKind::Build) {
        auto opt = findPreset(presetName, CMakePresetKind::Build);
        if (opt.has_value()) {
            return opt->configurePreset;
        }
        return {};
    }
    if (kind == CMakePresetKind::Test) {
        auto opt = findPreset(presetName, CMakePresetKind::Test);
        if (opt.has_value()) {
            return opt->configurePreset;
        }
        return {};
    }
    return {};
}

std::string CMakePresetsService::expandMacros(const std::string& input,
                                              std::string_view presetName) const {
    std::string result = input;
    const std::string srcPlaceholder = "${sourceDir}";
    const std::string presetPlaceholder = "${presetName}";

    std::size_t pos = 0;
    while ((pos = result.find(srcPlaceholder, pos)) != std::string::npos) {
        result.replace(pos, srcPlaceholder.length(), sourceDirectory_.string());
        pos += sourceDirectory_.string().length();
    }

    pos = 0;
    while ((pos = result.find(presetPlaceholder, pos)) != std::string::npos) {
        result.replace(pos, presetPlaceholder.length(), presetName);
        pos += presetName.length();
    }
    return result;
}

std::string
CMakePresetsService::buildConfigureCommandLine(std::string_view presetName,
                                               const CMakePresetCommandOptions& options) const {
    std::string cmd = "cmake --preset " + std::string(presetName);
    for (const auto& arg : options.additionalArgs) {
        cmd += " " + expandMacros(arg, presetName);
    }
    return cmd;
}

std::string CMakePresetsService::buildBuildCommandLine(std::string_view presetName,
                                                       const CMakePresetCommandOptions& options) const {
    std::string cmd = "cmake --build --preset " + std::string(presetName);
    for (const auto& arg : options.additionalArgs) {
        cmd += " " + expandMacros(arg, presetName);
    }
    return cmd;
}

std::string CMakePresetsService::buildTestCommandLine(std::string_view presetName,
                                                      const CMakePresetCommandOptions& options) const {
    std::string cmd = "ctest --preset " + std::string(presetName);
    for (const auto& arg : options.additionalArgs) {
        cmd += " " + expandMacros(arg, presetName);
    }
    return cmd;
}

CMakePresetExecutionResult CMakePresetsService::executeConfigure(
    std::string_view presetName, const CMakePresetCommandOptions& options, CancellationToken* cancel) {
    std::string cmdLine = buildConfigureCommandLine(presetName, options);
    return executeInternal(presetName, CMakePresetKind::Configure, cmdLine, options, cancel);
}

CMakePresetExecutionResult CMakePresetsService::executeBuild(std::string_view presetName,
                                                             const CMakePresetCommandOptions& options,
                                                             CancellationToken* cancel) {
    std::string cmdLine = buildBuildCommandLine(presetName, options);
    return executeInternal(presetName, CMakePresetKind::Build, cmdLine, options, cancel);
}

CMakePresetExecutionResult CMakePresetsService::executeTest(std::string_view presetName,
                                                            const CMakePresetCommandOptions& options,
                                                            CancellationToken* cancel) {
    std::string cmdLine = buildTestCommandLine(presetName, options);
    return executeInternal(presetName, CMakePresetKind::Test, cmdLine, options, cancel);
}

CMakePresetExecutionResult CMakePresetsService::executeInternal(std::string_view presetName,
                                                                CMakePresetKind kind,
                                                                const std::string& cmdLine,
                                                                const CMakePresetCommandOptions& options,
                                                                CancellationToken* cancel) {
    if (cancel && cancel->isCancelled()) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::Cancelled;
        res.exitCode = -1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
        return res;
    }

    if (!isLoaded_ || !report_.isValid()) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::ValidationError;
        res.exitCode = 1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "La especificación de presets no es válida o no ha sido cargada.";
        return res;
    }

    auto defOpt = findPreset(presetName, kind);
    if (!defOpt.has_value()) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::PresetNotFound;
        res.exitCode = 1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "El perfil solicitado no existe en el catálogo de presets cargados.";
        return res;
    }

    if (kind == CMakePresetKind::Configure) {
        if (!CMakePresetsValidator::isSafeBinaryDir(defOpt->binaryDir)) {
            CMakePresetExecutionResult res;
            res.status = CMakePresetExecutionStatus::BinaryDirUnsafe;
            res.exitCode = 1;
            res.commandLine = cmdLine;
            res.workingDirectory = sourceDirectory_.string();
            res.remediationHint =
                "El directorio binario del preset no cumple con las políticas de seguridad.";
            return res;
        }
        if (options.cleanBuildDirFirst) {
            static_cast<void>(cleanBinaryDir(presetName));
        }
    } else if (kind == CMakePresetKind::Build || kind == CMakePresetKind::Test) {
        std::string cfgName = resolveAssociatedConfigurePreset(presetName, kind);
        auto cfgOpt = findPreset(cfgName, CMakePresetKind::Configure);
        if (cfgOpt.has_value() && !CMakePresetsValidator::isSafeBinaryDir(cfgOpt->binaryDir)) {
            CMakePresetExecutionResult res;
            res.status = CMakePresetExecutionStatus::BinaryDirUnsafe;
            res.exitCode = 1;
            res.commandLine = cmdLine;
            res.workingDirectory = sourceDirectory_.string();
            res.remediationHint =
                "El directorio binario del preset de configuración asociado no es seguro.";
            return res;
        }
    }

    if (cancel && cancel->isCancelled()) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::Cancelled;
        res.exitCode = -1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
        return res;
    }

    if (options.dryRun) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::Success;
        res.exitCode = 0;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.standardOutput = "[dry-run] Comando validado y sintetizado correctamente: " + cmdLine;
        res.standardError = "";
        res.elapsedMilliseconds = 0;
        res.remediationHint = "";
        return res;
    }

    // Ejecución local acotada bajo política Zero-Zombie
    int pipeOut[2];
    int pipeErr[2];
    if (::pipe(pipeOut) != 0 || ::pipe(pipeErr) != 0) {
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::IOError;
        res.exitCode = -1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "Fallo al crear descriptores de tubería para captura de procesos.";
        return res;
    }

    const auto startTime = std::chrono::steady_clock::now();
    pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipeOut[0]);
        ::close(pipeOut[1]);
        ::close(pipeErr[0]);
        ::close(pipeErr[1]);
        CMakePresetExecutionResult res;
        res.status = CMakePresetExecutionStatus::IOError;
        res.exitCode = -1;
        res.commandLine = cmdLine;
        res.workingDirectory = sourceDirectory_.string();
        res.remediationHint = "Fallo al bifurcar proceso (fork).";
        return res;
    }

    if (pid == 0) {
        // Subproceso hijo
        ::close(pipeOut[0]);
        ::close(pipeErr[0]);
        ::dup2(pipeOut[1], STDOUT_FILENO);
        ::dup2(pipeErr[1], STDERR_FILENO);
        ::close(pipeOut[1]);
        ::close(pipeErr[1]);

        if (!sourceDirectory_.empty() && sourceDirectory_ != ".") {
            if (::chdir(sourceDirectory_.c_str()) != 0) {
                ::_exit(126);
            }
        }
        for (const auto& [k, v] : options.environmentOverrides) {
            ::setenv(k.c_str(), v.c_str(), 1);
        }
        ::execl("/bin/sh", "sh", "-c", cmdLine.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }

    // Proceso padre supervisor
    ::close(pipeOut[1]);
    ::close(pipeErr[1]);

    ::fcntl(pipeOut[0], F_SETFL, ::fcntl(pipeOut[0], F_GETFL) | O_NONBLOCK);
    ::fcntl(pipeErr[0], F_SETFL, ::fcntl(pipeErr[0], F_GETFL) | O_NONBLOCK);

    CMakePresetExecutionResult res;
    res.commandLine = cmdLine;
    res.workingDirectory = sourceDirectory_.string();

    const std::size_t maxBytes = (options.maxOutputBytes > 0) ? options.maxOutputBytes : 65536;
    bool outTruncated = false;
    bool errTruncated = false;
    bool timedOut = false;
    bool wasCancelled = false;

    std::vector<struct pollfd> pfds(2);
    pfds[0].fd = pipeOut[0];
    pfds[0].events = POLLIN;
    pfds[1].fd = pipeErr[0];
    pfds[1].events = POLLIN;

    char buf[4096];
    while (pfds[0].fd >= 0 || pfds[1].fd >= 0) {
        if (cancel && cancel->isCancelled()) {
            wasCancelled = true;
            ::kill(pid, SIGKILL);
            break;
        }
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (options.timeoutMs > 0 && elapsed >= static_cast<int64_t>(options.timeoutMs)) {
            timedOut = true;
            ::kill(pid, SIGKILL);
            break;
        }

        const int pret = ::poll(pfds.data(), pfds.size(), 50);
        if (pret > 0) {
            if (pfds[0].fd >= 0 && (pfds[0].revents & POLLIN)) {
                const ssize_t count = ::read(pipeOut[0], buf, sizeof(buf));
                if (count > 0) {
                    if (res.standardOutput.size() < maxBytes) {
                        const std::size_t toAppend = std::min(static_cast<std::size_t>(count),
                                                              maxBytes - res.standardOutput.size());
                        res.standardOutput.append(buf, toAppend);
                        if (toAppend < static_cast<std::size_t>(count)) {
                            outTruncated = true;
                        }
                    } else {
                        outTruncated = true;
                    }
                } else if (count == 0) {
                    ::close(pipeOut[0]);
                    pfds[0].fd = -1;
                }
            } else if (pfds[0].fd >= 0 && (pfds[0].revents & (POLLHUP | POLLERR))) {
                ssize_t count = 0;
                while ((count = ::read(pipeOut[0], buf, sizeof(buf))) > 0) {
                    if (res.standardOutput.size() < maxBytes) {
                        const std::size_t toAppend = std::min(static_cast<std::size_t>(count),
                                                              maxBytes - res.standardOutput.size());
                        res.standardOutput.append(buf, toAppend);
                        if (toAppend < static_cast<std::size_t>(count)) {
                            outTruncated = true;
                        }
                    } else {
                        outTruncated = true;
                    }
                }
                ::close(pipeOut[0]);
                pfds[0].fd = -1;
            }

            if (pfds[1].fd >= 0 && (pfds[1].revents & POLLIN)) {
                const ssize_t count = ::read(pipeErr[0], buf, sizeof(buf));
                if (count > 0) {
                    if (res.standardError.size() < maxBytes) {
                        const std::size_t toAppend = std::min(static_cast<std::size_t>(count),
                                                              maxBytes - res.standardError.size());
                        res.standardError.append(buf, toAppend);
                        if (toAppend < static_cast<std::size_t>(count)) {
                            errTruncated = true;
                        }
                    } else {
                        errTruncated = true;
                    }
                } else if (count == 0) {
                    ::close(pipeErr[0]);
                    pfds[1].fd = -1;
                }
            } else if (pfds[1].fd >= 0 && (pfds[1].revents & (POLLHUP | POLLERR))) {
                ssize_t count = 0;
                while ((count = ::read(pipeErr[0], buf, sizeof(buf))) > 0) {
                    if (res.standardError.size() < maxBytes) {
                        const std::size_t toAppend = std::min(static_cast<std::size_t>(count),
                                                              maxBytes - res.standardError.size());
                        res.standardError.append(buf, toAppend);
                        if (toAppend < static_cast<std::size_t>(count)) {
                            errTruncated = true;
                        }
                    } else {
                        errTruncated = true;
                    }
                }
                ::close(pipeErr[0]);
                pfds[1].fd = -1;
            }
        }
    }

    if (pfds[0].fd >= 0) {
        ::close(pipeOut[0]);
    }
    if (pfds[1].fd >= 0) {
        ::close(pipeErr[0]);
    }

    int status = 0;
    ::waitpid(pid, &status, 0);

    const auto endTime = std::chrono::steady_clock::now();
    res.elapsedMilliseconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    if (outTruncated) {
        res.standardOutput += "\n[... salida truncada para preservar límites de memoria ...]";
    }
    if (errTruncated) {
        res.standardError += "\n[... salida truncada para preservar límites de memoria ...]";
    }

    if (wasCancelled) {
        res.status = CMakePresetExecutionStatus::Cancelled;
        res.exitCode = -1;
        res.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
    } else if (timedOut) {
        res.status = CMakePresetExecutionStatus::Timeout;
        res.exitCode = -1;
        res.remediationHint = "La operación excedió el tiempo límite configurado (" +
                              std::to_string(options.timeoutMs) + " ms).";
    } else if (WIFEXITED(status)) {
        res.exitCode = WEXITSTATUS(status);
        if (res.exitCode == 0) {
            res.status = CMakePresetExecutionStatus::Success;
        } else if (res.exitCode == 127) {
            res.status = CMakePresetExecutionStatus::ToolNotFound;
            res.remediationHint = "Herramienta de compilación o comando no encontrado.";
        } else {
            res.status = CMakePresetExecutionStatus::ProcessFailed;
            res.remediationHint =
                "El proceso finalizó con código de error " + std::to_string(res.exitCode);
        }
    } else {
        res.status = CMakePresetExecutionStatus::ProcessFailed;
        res.exitCode = -1;
        res.remediationHint = "El proceso fue terminado de forma anómala.";
    }

    return res;
}

bool CMakePresetsService::cleanBinaryDir(std::string_view configurePresetName,
                                         std::string* errorMessage) {
    auto opt = findPreset(configurePresetName, CMakePresetKind::Configure);
    if (!opt.has_value()) {
        if (errorMessage) {
            *errorMessage = "Preset de configuración no encontrado: " + std::string(configurePresetName);
        }
        return false;
    }

    if (!CMakePresetsValidator::isSafeBinaryDir(opt->binaryDir)) {
        if (errorMessage) {
            *errorMessage = "Ruta de directorio binario no segura: " + opt->binaryDir;
        }
        return false;
    }

    std::string expanded = expandMacros(opt->binaryDir, configurePresetName);
    std::filesystem::path binPath;
    if (std::filesystem::path(expanded).is_absolute()) {
        binPath = expanded;
    } else {
        binPath = sourceDirectory_ / expanded;
    }

    std::error_code ec;
    if (std::filesystem::exists(binPath, ec)) {
        auto cacheFile = binPath / "CMakeCache.txt";
        if (std::filesystem::exists(cacheFile, ec)) {
            std::filesystem::remove(cacheFile, ec);
            if (ec) {
                if (errorMessage) {
                    *errorMessage = "Error al eliminar archivo de caché: " + ec.message();
                }
                return false;
            }
        }
        auto cmakeFiles = binPath / "CMakeFiles";
        if (std::filesystem::exists(cmakeFiles, ec)) {
            std::filesystem::remove_all(cmakeFiles, ec);
            if (ec) {
                if (errorMessage) {
                    *errorMessage = "Error al limpiar directorio de build: " + ec.message();
                }
                return false;
            }
        }
    } else if (ec) {
        if (errorMessage) {
            *errorMessage = "Error al acceder al directorio binario: " + ec.message();
        }
        return false;
    }
    return true;
}

std::string CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus status) {
    switch (status) {
    case CMakePresetExecutionStatus::Success:
        return "Operación completada exitosamente";
    case CMakePresetExecutionStatus::ValidationError:
        return "Error de validación en la especificación de configuración";
    case CMakePresetExecutionStatus::PresetNotFound:
        return "Perfil de configuración no encontrado";
    case CMakePresetExecutionStatus::BinaryDirUnsafe:
        return "Directorio de destino no seguro o fuera de las políticas de contención";
    case CMakePresetExecutionStatus::ToolNotFound:
        return "Herramienta de compilación o prueba no encontrada en el sistema";
    case CMakePresetExecutionStatus::ProcessFailed:
        return "El proceso finalizó con código de error";
    case CMakePresetExecutionStatus::Timeout:
        return "Tiempo límite de espera superado para la operación";
    case CMakePresetExecutionStatus::Cancelled:
        return "Operación cancelada a petición del usuario";
    case CMakePresetExecutionStatus::IOError:
        return "Error de entrada/salida al acceder al archivo o directorio";
    default:
        return "Estado de ejecución desconocido";
    }
}

std::string CMakePresetsService::executionStateToString(CMakePresetExecutionState state) {
    switch (state) {
    case CMakePresetExecutionState::Idle:
        return "En espera";
    case CMakePresetExecutionState::Validating:
        return "Validando especificación";
    case CMakePresetExecutionState::Configuring:
        return "Configurando entorno de compilación";
    case CMakePresetExecutionState::Building:
        return "Compilando proyecto";
    case CMakePresetExecutionState::Testing:
        return "Ejecutando pruebas de verificación";
    case CMakePresetExecutionState::Completed:
        return "Operación completada";
    case CMakePresetExecutionState::Failed:
        return "Operación fallida";
    case CMakePresetExecutionState::Cancelled:
        return "Operación cancelada";
    default:
        return "Estado desconocido";
    }
}

} // namespace vigilio
