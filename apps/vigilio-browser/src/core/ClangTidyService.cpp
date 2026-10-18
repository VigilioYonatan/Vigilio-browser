#include "core/ClangTidyService.h"
#include "core/ClangTidyConfiguration.h"
#include "core/ClangTidyValidator.h"

#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>

namespace vigilio {

namespace {

bool isCheckRuleEnabled(std::string_view checkName, const ClangTidyOptions& options) {
    std::vector<std::string> enabled;
    std::vector<std::string> disabled;
    ClangTidyValidator::parseCheckTokens(options.checks, enabled, disabled);

    bool isEnabled = false;
    for (const auto& token : enabled) {
        if (token == "*" || token == checkName) {
            isEnabled = true;
        } else if (token.ends_with("*") && checkName.starts_with(token.substr(0, token.size() - 1))) {
            isEnabled = true;
        }
    }
    for (const auto& token : disabled) {
        if (token == "*" || token == checkName) {
            isEnabled = false;
        } else if (token.ends_with("*") && checkName.starts_with(token.substr(0, token.size() - 1))) {
            isEnabled = false;
        }
    }
    return isEnabled;
}

bool isWarningAsError(std::string_view checkName, const ClangTidyOptions& options) {
    if (options.warningsAsErrors.empty()) {
        return false;
    }
    if (options.warningsAsErrors == "*") {
        return true;
    }
    std::vector<std::string> enabled;
    std::vector<std::string> disabled;
    ClangTidyValidator::parseCheckTokens(options.warningsAsErrors, enabled, disabled);
    for (const auto& token : enabled) {
        if (token == "*" || token == checkName) {
            return true;
        }
        if (token.ends_with("*") && checkName.starts_with(token.substr(0, token.size() - 1))) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> splitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        auto end = text.find('\n', start);
        if (end == std::string_view::npos) {
            lines.emplace_back(text.substr(start));
            break;
        }
        lines.emplace_back(text.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

} // namespace

ClangTidyService::ClangTidyService()
    : options_(ClangTidyConfiguration::canonicalConfig()), isConfigured_(false) {
}

ClangTidyService::ClangTidyService(const ClangTidyOptions& options)
    : options_(options),
      isConfigured_(ClangTidyValidator::validateOptions(options) == ClangTidyError::None) {
}

bool ClangTidyService::loadConfiguration(const std::filesystem::path& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto report = ClangTidyValidator::validateConfigurationFile(configPath);
    if (!report.isValid) {
        isConfigured_ = false;
        return false;
    }
    std::ifstream file(configPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        isConfigured_ = false;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    ClangTidyOptions opts;
    if (!ClangTidyValidator::parseConfig(buffer.str(), opts)) {
        isConfigured_ = false;
        return false;
    }
    options_ = opts;
    isConfigured_ = (ClangTidyValidator::validateOptions(options_) == ClangTidyError::None);
    return isConfigured_;
}

void ClangTidyService::setOptions(const ClangTidyOptions& options) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
    isConfigured_ = (ClangTidyValidator::validateOptions(options) == ClangTidyError::None);
}

const ClangTidyOptions& ClangTidyService::options() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

bool ClangTidyService::isConfigured() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return isConfigured_;
}

SourceFileTidyInspection ClangTidyService::inspectSource(std::string_view sourceCode,
                                                         std::string_view virtualPath,
                                                         const ClangTidyOptions& options) {
    SourceFileTidyInspection inspection;
    inspection.relativePath = std::string(virtualPath);

    if (sourceCode.empty()) {
        inspection.isClean = true;
        inspection.issuesCount = 0;
        inspection.summary = "";
        return inspection;
    }

    ClangTidyOptions effectiveOpts = options;
    if (effectiveOpts.checks.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        effectiveOpts = isConfigured_ ? options_ : ClangTidyConfiguration::canonicalConfig();
    }

    const bool checkNullptr = isCheckRuleEnabled("modernize-use-nullptr", effectiveOpts);
    const bool checkBraces = isCheckRuleEnabled("readability-braces-around-statements", effectiveOpts);
    const bool checkBugprone = isCheckRuleEnabled("bugprone-assignment-in-if-condition", effectiveOpts);
    const bool checkPerformance = isCheckRuleEnabled("performance-for-range-copy", effectiveOpts);

    const auto lines = splitLines(sourceCode);
    bool inMultiLineComment = false;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        const int lineNum = static_cast<int>(i + 1);

        // modernize-use-nullptr
        if (checkNullptr) {
            std::size_t pos = 0;
            while (pos < line.size()) {
                if (inMultiLineComment) {
                    auto commentEnd = line.find("*/", pos);
                    if (commentEnd == std::string::npos) {
                        break;
                    }
                    inMultiLineComment = false;
                    pos = commentEnd + 2;
                    continue;
                }
                auto commentStart = line.find("/*", pos);
                auto lineComment = line.find("//", pos);
                auto nullPos = line.find("NULL", pos);

                if (nullPos == std::string::npos) {
                    break;
                }

                if (lineComment != std::string::npos && lineComment < nullPos) {
                    break;
                }
                if (commentStart != std::string::npos && commentStart < nullPos) {
                    inMultiLineComment = true;
                    pos = commentStart + 2;
                    continue;
                }

                bool leftBoundary =
                    (nullPos == 0 || (!std::isalnum(static_cast<unsigned char>(line[nullPos - 1])) &&
                                      line[nullPos - 1] != '_'));
                bool rightBoundary = (nullPos + 4 >= line.size() ||
                                      (!std::isalnum(static_cast<unsigned char>(line[nullPos + 4])) &&
                                       line[nullPos + 4] != '_'));

                if (leftBoundary && rightBoundary) {
                    ClangTidyIssueInspection issue;
                    issue.filePath = std::string(virtualPath);
                    issue.lineNumber = lineNum;
                    issue.columnNumber = static_cast<int>(nullPos + 1);
                    issue.checkName = "modernize-use-nullptr";
                    issue.message =
                        "Utilice 'nullptr' en lugar de 'NULL' o '0' para literales de puntero.";
                    issue.isWarningAsError = isWarningAsError(issue.checkName, effectiveOpts);
                    inspection.issues.push_back(issue);
                }
                pos = nullPos + 4;
            }
        }

        // readability-braces-around-statements
        if (checkBraces) {
            std::size_t firstNonSpace = line.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos) {
                std::string trimmed = line.substr(firstNonSpace);
                bool isControl = false;
                std::size_t startPos = 0;

                if (trimmed.starts_with("if") &&
                    (trimmed.size() == 2 || trimmed[2] == ' ' || trimmed[2] == '(')) {
                    isControl = true;
                    startPos = 2;
                } else if (trimmed.starts_with("while") &&
                           (trimmed.size() == 5 || trimmed[5] == ' ' || trimmed[5] == '(')) {
                    isControl = true;
                    startPos = 5;
                } else if (trimmed.starts_with("for") &&
                           (trimmed.size() == 3 || trimmed[3] == ' ' || trimmed[3] == '(')) {
                    isControl = true;
                    startPos = 3;
                }

                if (isControl) {
                    auto openParen = trimmed.find('(', startPos);
                    if (openParen != std::string::npos) {
                        int parenDepth = 1;
                        std::size_t closeParen = openParen + 1;
                        while (closeParen < trimmed.size() && parenDepth > 0) {
                            if (trimmed[closeParen] == '(') {
                                ++parenDepth;
                            } else if (trimmed[closeParen] == ')') {
                                --parenDepth;
                            }
                            ++closeParen;
                        }

                        if (parenDepth == 0) {
                            std::string afterParen = trimmed.substr(closeParen);
                            auto nonSpaceAfter = afterParen.find_first_not_of(" \t");

                            if (nonSpaceAfter != std::string::npos) {
                                if (afterParen[nonSpaceAfter] != '{' &&
                                    afterParen[nonSpaceAfter] != ';') {
                                    ClangTidyIssueInspection issue;
                                    issue.filePath = std::string(virtualPath);
                                    issue.lineNumber = lineNum;
                                    issue.columnNumber = static_cast<int>(firstNonSpace + 1);
                                    issue.checkName = "readability-braces-around-statements";
                                    issue.message = "Las instrucciones de control de flujo deben estar "
                                                    "delimitadas por llaves explícitas.";
                                    issue.isWarningAsError =
                                        isWarningAsError(issue.checkName, effectiveOpts);
                                    inspection.issues.push_back(issue);
                                }
                            } else {
                                if (i + 1 < lines.size()) {
                                    auto nextNonSpace = lines[i + 1].find_first_not_of(" \t");
                                    if (nextNonSpace != std::string::npos &&
                                        lines[i + 1][nextNonSpace] != '{') {
                                        ClangTidyIssueInspection issue;
                                        issue.filePath = std::string(virtualPath);
                                        issue.lineNumber = lineNum;
                                        issue.columnNumber = static_cast<int>(firstNonSpace + 1);
                                        issue.checkName = "readability-braces-around-statements";
                                        issue.message = "Las instrucciones de control de flujo deben "
                                                        "estar delimitadas por llaves explícitas.";
                                        issue.isWarningAsError =
                                            isWarningAsError(issue.checkName, effectiveOpts);
                                        inspection.issues.push_back(issue);
                                    }
                                }
                            }
                        }
                    }
                } else if (trimmed.starts_with("else") &&
                           (trimmed.size() == 4 || trimmed[4] == ' ' || trimmed[4] == '\t')) {
                    std::string afterElse = trimmed.substr(4);
                    auto nonSpaceAfter = afterElse.find_first_not_of(" \t");
                    if (nonSpaceAfter != std::string::npos &&
                        !afterElse.substr(nonSpaceAfter).starts_with("if")) {
                        if (afterElse[nonSpaceAfter] != '{') {
                            ClangTidyIssueInspection issue;
                            issue.filePath = std::string(virtualPath);
                            issue.lineNumber = lineNum;
                            issue.columnNumber = static_cast<int>(firstNonSpace + 1);
                            issue.checkName = "readability-braces-around-statements";
                            issue.message = "Las instrucciones de control de flujo deben estar "
                                            "delimitadas por llaves explícitas.";
                            issue.isWarningAsError = isWarningAsError(issue.checkName, effectiveOpts);
                            inspection.issues.push_back(issue);
                        }
                    } else if (nonSpaceAfter == std::string::npos && i + 1 < lines.size()) {
                        auto nextNonSpace = lines[i + 1].find_first_not_of(" \t");
                        if (nextNonSpace != std::string::npos && lines[i + 1][nextNonSpace] != '{' &&
                            !lines[i + 1].substr(nextNonSpace).starts_with("if")) {
                            ClangTidyIssueInspection issue;
                            issue.filePath = std::string(virtualPath);
                            issue.lineNumber = lineNum;
                            issue.columnNumber = static_cast<int>(firstNonSpace + 1);
                            issue.checkName = "readability-braces-around-statements";
                            issue.message = "Las instrucciones de control de flujo deben estar "
                                            "delimitadas por llaves explícitas.";
                            issue.isWarningAsError = isWarningAsError(issue.checkName, effectiveOpts);
                            inspection.issues.push_back(issue);
                        }
                    }
                }
            }
        }

        // bugprone-*
        if (checkBugprone) {
            std::regex assignInIf(R"(\bif\s*\([^=!<>\n]*=[^=!<>\n]*\))");
            if (std::regex_search(line, assignInIf)) {
                ClangTidyIssueInspection issue;
                issue.filePath = std::string(virtualPath);
                issue.lineNumber = lineNum;
                issue.columnNumber = 1;
                issue.checkName = "bugprone-assignment-in-if-condition";
                issue.message =
                    "Posible asignación no intencionada en condición lógica; use '==' para comparación.";
                issue.isWarningAsError = isWarningAsError(issue.checkName, effectiveOpts);
                inspection.issues.push_back(issue);
            }
        }

        // performance-*
        if (checkPerformance) {
            std::regex rangeCopy(
                R"(for\s*\(\s*(?:const\s+)?(?:auto|[A-Za-z0-9_:]+)\s+[a-zA-Z0-9_]+\s*:\s*[a-zA-Z0-9_]+\s*\))");
            if (std::regex_search(line, rangeCopy) && line.find('&') == std::string::npos &&
                line.find('*') == std::string::npos) {
                ClangTidyIssueInspection issue;
                issue.filePath = std::string(virtualPath);
                issue.lineNumber = lineNum;
                issue.columnNumber = 1;
                issue.checkName = "performance-for-range-copy";
                issue.message = "El bucle sobre rango realiza una copia del elemento; considere usar "
                                "una referencia constante ('const auto&').";
                issue.isWarningAsError = isWarningAsError(issue.checkName, effectiveOpts);
                inspection.issues.push_back(issue);
            }
        }
    }

    inspection.issuesCount = static_cast<int>(inspection.issues.size());
    inspection.isClean = inspection.issues.empty();
    if (!inspection.isClean) {
        inspection.summary = "Se detectaron " + std::to_string(inspection.issuesCount) +
                             " problemas en el código fuente.";
    } else {
        inspection.summary = "";
    }

    return inspection;
}

std::string ClangTidyService::applyFixesToSource(std::string_view sourceCode,
                                                 const ClangTidyOptions& options) {
    if (sourceCode.empty()) {
        return "";
    }

    ClangTidyOptions effectiveOpts = options;
    if (effectiveOpts.checks.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        effectiveOpts = isConfigured_ ? options_ : ClangTidyConfiguration::canonicalConfig();
    }

    std::string result = std::string(sourceCode);

    // 1. modernize-use-nullptr
    if (isCheckRuleEnabled("modernize-use-nullptr", effectiveOpts)) {
        std::regex nullRegex(R"(\bNULL\b)");
        result = std::regex_replace(result, nullRegex, "nullptr");
    }

    // 2. readability-braces-around-statements
    if (isCheckRuleEnabled("readability-braces-around-statements", effectiveOpts)) {
        auto lines = splitLines(result);
        for (std::size_t i = 0; i < lines.size(); ++i) {
            std::size_t firstNonSpace = lines[i].find_first_not_of(" \t");
            if (firstNonSpace == std::string::npos) {
                continue;
            }

            std::string indent = lines[i].substr(0, firstNonSpace);
            std::string trimmed = lines[i].substr(firstNonSpace);

            bool isControl = false;
            std::size_t startPos = 0;

            if (trimmed.starts_with("if") &&
                (trimmed.size() == 2 || trimmed[2] == ' ' || trimmed[2] == '(')) {
                isControl = true;
                startPos = 2;
            } else if (trimmed.starts_with("while") &&
                       (trimmed.size() == 5 || trimmed[5] == ' ' || trimmed[5] == '(')) {
                isControl = true;
                startPos = 5;
            } else if (trimmed.starts_with("for") &&
                       (trimmed.size() == 3 || trimmed[3] == ' ' || trimmed[3] == '(')) {
                isControl = true;
                startPos = 3;
            }

            if (isControl) {
                auto openParen = trimmed.find('(', startPos);
                if (openParen != std::string::npos) {
                    int parenDepth = 1;
                    std::size_t closeParen = openParen + 1;
                    while (closeParen < trimmed.size() && parenDepth > 0) {
                        if (trimmed[closeParen] == '(') {
                            ++parenDepth;
                        } else if (trimmed[closeParen] == ')') {
                            --parenDepth;
                        }
                        ++closeParen;
                    }

                    if (parenDepth == 0) {
                        std::string afterParen = trimmed.substr(closeParen);
                        auto nonSpaceAfter = afterParen.find_first_not_of(" \t");

                        if (nonSpaceAfter != std::string::npos) {
                            if (afterParen[nonSpaceAfter] != '{' && afterParen[nonSpaceAfter] != ';') {
                                std::string stmt = afterParen.substr(nonSpaceAfter);
                                lines[i] = indent + trimmed.substr(0, closeParen) + " {";
                                lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                             indent + "    " + stmt);
                                lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 2),
                                             indent + "}");
                                i += 2;
                            }
                        } else {
                            if (i + 1 < lines.size()) {
                                auto nextNonSpace = lines[i + 1].find_first_not_of(" \t");
                                if (nextNonSpace != std::string::npos &&
                                    lines[i + 1][nextNonSpace] != '{') {
                                    lines[i] = lines[i] + " {";
                                    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 2),
                                                 indent + "}");
                                    i += 2;
                                }
                            }
                        }
                    }
                }
            } else if (trimmed.starts_with("else") &&
                       (trimmed.size() == 4 || trimmed[4] == ' ' || trimmed[4] == '\t')) {
                std::string afterElse = trimmed.substr(4);
                auto nonSpaceAfter = afterElse.find_first_not_of(" \t");
                if (nonSpaceAfter != std::string::npos &&
                    !afterElse.substr(nonSpaceAfter).starts_with("if")) {
                    if (afterElse[nonSpaceAfter] != '{') {
                        std::string stmt = afterElse.substr(nonSpaceAfter);
                        lines[i] = indent + "else {";
                        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                     indent + "    " + stmt);
                        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 2), indent + "}");
                        i += 2;
                    }
                } else if (nonSpaceAfter == std::string::npos && i + 1 < lines.size()) {
                    auto nextNonSpace = lines[i + 1].find_first_not_of(" \t");
                    if (nextNonSpace != std::string::npos && lines[i + 1][nextNonSpace] != '{' &&
                        !lines[i + 1].substr(nextNonSpace).starts_with("if")) {
                        lines[i] = lines[i] + " {";
                        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(i + 2), indent + "}");
                        i += 2;
                    }
                }
            }
        }

        std::ostringstream oss;
        for (std::size_t idx = 0; idx < lines.size(); ++idx) {
            oss << lines[idx];
            if (idx + 1 < lines.size() || (sourceCode.ends_with('\n'))) {
                oss << "\n";
            }
        }
        result = oss.str();
    }

    return result;
}

SourceFileTidyInspection ClangTidyService::inspectFile(const std::filesystem::path& filePath,
                                                       CancellationToken* cancel) {
    SourceFileTidyInspection inspection;
    inspection.relativePath = filePath.string();

    if (cancel != nullptr && cancel->isCancelled()) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::Cancelled);
        return inspection;
    }

    if (!ClangTidyValidator::isSafeConfigPath(filePath)) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::UnsafePath);
        return inspection;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::FileNotFound);
        return inspection;
    }

    auto sz = std::filesystem::file_size(filePath, ec);
    if (sz > 10 * 1024 * 1024) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::FileTooLarge);
        return inspection;
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::ReadError);
        return inspection;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    if (cancel != nullptr && cancel->isCancelled()) {
        inspection.isClean = false;
        inspection.summary = executionStatusToString(TidyExecutionStatus::Cancelled);
        return inspection;
    }

    return inspectSource(content, filePath.string());
}

TidyOperationResult ClangTidyService::inspectDirectory(const std::filesystem::path& dirPath,
                                                       const TidyServiceOptions& options,
                                                       CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    TidyOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = TidyExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangTidyValidator::isSafeConfigPath(dirPath)) {
        result.status = TidyExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
        result.status = TidyExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::filesystem::recursive_directory_iterator it(
        dirPath, std::filesystem::directory_options::skip_permission_denied, ec);
    const std::filesystem::recursive_directory_iterator endIt;

    while (it != endIt && !ec) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = TidyExecutionStatus::Cancelled;
            result.remediationHint = executionStatusToString(result.status);
            break;
        }

        if (static_cast<std::size_t>(it.depth()) > options.maxScanDepth) {
            it.pop();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_directory(ec)) {
            if (ClangTidyConfiguration::isExcludedDirectory(entry.path())) {
                it.disable_recursion_pending();
            }
            it.increment(ec);
            continue;
        }

        if (entry.is_regular_file(ec)) {
            if (ClangTidyConfiguration::isSupportedSourceExtension(entry.path())) {
                result.filesExamined++;
                auto insp = inspectFile(entry.path(), cancel);
                if (!insp.isClean) {
                    result.issuesCount += static_cast<std::size_t>(insp.issuesCount);
                }
                result.details.push_back(insp);
            }
        }
        it.increment(ec);
    }

    if (result.status != TidyExecutionStatus::Cancelled) {
        if (result.issuesCount > 0) {
            result.status = TidyExecutionStatus::IssuesFound;
        } else {
            result.status = TidyExecutionStatus::Success;
        }
        result.remediationHint = executionStatusToString(result.status);
    }

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

bool ClangTidyService::createBackup(const std::filesystem::path& filePath, std::string* outBackupPath) {
    std::filesystem::path backupPath = filePath;
    backupPath += ".bak";
    std::error_code ec;
    std::filesystem::copy_file(filePath, backupPath, std::filesystem::copy_options::overwrite_existing,
                               ec);
    if (ec) {
        return false;
    }
    if (outBackupPath != nullptr) {
        *outBackupPath = backupPath.string();
    }
    return true;
}

bool ClangTidyService::rollbackFile(const std::filesystem::path& filePath) {
    std::filesystem::path backupPath = filePath;
    backupPath += ".bak";
    std::error_code ec;
    if (!std::filesystem::exists(backupPath, ec) || ec) {
        return false;
    }
    std::filesystem::copy_file(backupPath, filePath, std::filesystem::copy_options::overwrite_existing,
                               ec);
    return !ec;
}

bool ClangTidyService::removeBackup(const std::filesystem::path& filePath) {
    std::filesystem::path backupPath = filePath;
    backupPath += ".bak";
    std::error_code ec;
    if (!std::filesystem::exists(backupPath, ec) || ec) {
        return true;
    }
    return std::filesystem::remove(backupPath, ec);
}

TidyOperationResult ClangTidyService::applyFixesToFile(const std::filesystem::path& filePath,
                                                       const TidyServiceOptions& options,
                                                       CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    TidyOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = TidyExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangTidyValidator::isSafeConfigPath(filePath)) {
        result.status = TidyExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        result.status = TidyExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    auto sz = std::filesystem::file_size(filePath, ec);
    if (sz > options.maxFileSizeBytes) {
        result.status = TidyExecutionStatus::FileTooLarge;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        result.status = TidyExecutionStatus::ReadError;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();
    file.close();

    result.filesExamined = 1;

    if (options.dryRun) {
        auto insp = inspectSource(content, filePath.string());
        result.details.push_back(insp);
        if (!insp.isClean) {
            result.issuesCount = static_cast<std::size_t>(insp.issuesCount);
            result.status = TidyExecutionStatus::IssuesFound;
        } else {
            result.status = TidyExecutionStatus::Success;
        }
        result.remediationHint = executionStatusToString(result.status);
        const auto endTime = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return result;
    }

    std::string fixed = applyFixesToSource(content);
    if (fixed == content) {
        result.status = TidyExecutionStatus::Success;
        result.remediationHint = executionStatusToString(result.status);
        SourceFileTidyInspection insp;
        insp.relativePath = filePath.string();
        insp.isClean = true;
        result.details.push_back(insp);
        const auto endTime = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return result;
    }

    if (options.createBackup) {
        if (!createBackup(filePath)) {
            result.status = TidyExecutionStatus::WriteError;
            result.remediationHint = executionStatusToString(result.status);
            return result;
        }
    }

    std::ofstream out(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        if (options.createBackup) {
            static_cast<void>(rollbackFile(filePath));
            static_cast<void>(removeBackup(filePath));
        }
        result.status = TidyExecutionStatus::WriteError;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    out << fixed;
    if (!out.good()) {
        out.close();
        if (options.createBackup) {
            static_cast<void>(rollbackFile(filePath));
            static_cast<void>(removeBackup(filePath));
        }
        result.status = TidyExecutionStatus::WriteError;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }
    out.close();

    if (options.createBackup) {
        static_cast<void>(removeBackup(filePath));
    }

    result.filesFixed = 1;
    result.issuesCount = 1;
    result.status = TidyExecutionStatus::Success;
    result.remediationHint = executionStatusToString(result.status);
    SourceFileTidyInspection insp;
    insp.relativePath = filePath.string();
    insp.isClean = true;
    result.details.push_back(insp);

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

TidyOperationResult ClangTidyService::applyFixesWithRetry(const std::filesystem::path& filePath,
                                                          const TidyServiceOptions& options,
                                                          CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();

    if (cancel != nullptr && cancel->isCancelled()) {
        TidyOperationResult res;
        res.status = TidyExecutionStatus::Cancelled;
        res.remediationHint = executionStatusToString(res.status);
        return res;
    }

    TidyOperationResult res = applyFixesToFile(filePath, options, cancel);
    if (res.status == TidyExecutionStatus::Success || res.status == TidyExecutionStatus::Cancelled ||
        res.status == TidyExecutionStatus::UnsafePath ||
        res.status == TidyExecutionStatus::FileTooLarge ||
        res.status == TidyExecutionStatus::FileNotFound) {
        return res;
    }

    for (uint32_t attempt = 1; attempt <= options.maxRetries; ++attempt) {
        if (cancel != nullptr && cancel->isCancelled()) {
            res.status = TidyExecutionStatus::Cancelled;
            res.remediationHint = executionStatusToString(res.status);
            return res;
        }

        uint32_t delayMs = options.retryBackoffMs * (1U << (attempt - 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        if (cancel != nullptr && cancel->isCancelled()) {
            res.status = TidyExecutionStatus::Cancelled;
            res.remediationHint = executionStatusToString(res.status);
            return res;
        }

        res = applyFixesToFile(filePath, options, cancel);
        if (res.status == TidyExecutionStatus::Success) {
            const auto endTime = std::chrono::steady_clock::now();
            res.durationMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
            return res;
        }
    }

    res.status = TidyExecutionStatus::FileLocked;
    res.remediationHint = executionStatusToString(res.status);
    const auto endTime = std::chrono::steady_clock::now();
    res.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return res;
}

TidyOperationResult ClangTidyService::applyFixesToDirectory(const std::filesystem::path& dirPath,
                                                            const TidyServiceOptions& options,
                                                            CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    TidyOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = TidyExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangTidyValidator::isSafeConfigPath(dirPath)) {
        result.status = TidyExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
        result.status = TidyExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::filesystem::recursive_directory_iterator it(
        dirPath, std::filesystem::directory_options::skip_permission_denied, ec);
    const std::filesystem::recursive_directory_iterator endIt;

    while (it != endIt && !ec) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = TidyExecutionStatus::Cancelled;
            result.remediationHint = executionStatusToString(result.status);
            break;
        }

        if (static_cast<std::size_t>(it.depth()) > options.maxScanDepth) {
            it.pop();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_directory(ec)) {
            if (ClangTidyConfiguration::isExcludedDirectory(entry.path())) {
                it.disable_recursion_pending();
            }
            it.increment(ec);
            continue;
        }

        if (entry.is_regular_file(ec)) {
            if (ClangTidyConfiguration::isSupportedSourceExtension(entry.path())) {
                result.filesExamined++;
                auto fileRes = applyFixesWithRetry(entry.path(), options, cancel);
                if (fileRes.filesFixed > 0) {
                    result.filesFixed += fileRes.filesFixed;
                    result.issuesCount += fileRes.issuesCount;
                }
                for (const auto& d : fileRes.details) {
                    result.details.push_back(d);
                }
                if (fileRes.status == TidyExecutionStatus::Cancelled) {
                    result.status = TidyExecutionStatus::Cancelled;
                    break;
                }
            }
        }
        it.increment(ec);
    }

    if (result.status != TidyExecutionStatus::Cancelled) {
        result.status = TidyExecutionStatus::Success;
        result.remediationHint = executionStatusToString(result.status);
    }

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

std::string ClangTidyService::buildClangTidyCommandLine(const std::filesystem::path& targetPath,
                                                        const std::filesystem::path& buildPath, bool fix,
                                                        bool warningsAsErrors) const {
    std::string cmd = "clang-tidy";

    ClangTidyOptions opts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        opts = isConfigured_ ? options_ : ClangTidyConfiguration::canonicalConfig();
    }

    if (!opts.checks.empty()) {
        cmd += " --checks='" + opts.checks + "'";
    }

    if (warningsAsErrors || !opts.warningsAsErrors.empty()) {
        std::string wae = !opts.warningsAsErrors.empty() ? opts.warningsAsErrors : "*";
        cmd += " -warnings-as-errors='" + wae + "'";
    }

    if (!opts.headerFilterRegex.empty()) {
        cmd += " --header-filter='" + opts.headerFilterRegex + "'";
    }

    if (fix) {
        cmd += " -fix";
    }

    if (!targetPath.empty()) {
        cmd += " ";
        cmd += targetPath.string();
    }

    if (!buildPath.empty()) {
        cmd += " -p " + buildPath.string();
    }

    return cmd;
}

std::string ClangTidyService::executionStatusToString(TidyExecutionStatus status) {
    switch (status) {
    case TidyExecutionStatus::Success:
        return "Análisis de código completado correctamente sin incidencias.";
    case TidyExecutionStatus::IssuesFound:
        return "Se detectaron posibles mejoras o inconsistencias en los archivos analizados.";
    case TidyExecutionStatus::Cancelled:
        return "El análisis de código fue cancelado por el usuario.";
    case TidyExecutionStatus::FileLocked:
        return "El archivo está siendo utilizado por otra aplicación. Por favor, intente nuevamente.";
    case TidyExecutionStatus::PermissionDenied:
        return "No se cuenta con permisos para acceder al archivo seleccionado.";
    case TidyExecutionStatus::ReadError:
        return "Ocurrió un problema de lectura al examinar el archivo fuente.";
    case TidyExecutionStatus::WriteError:
        return "No fue posible guardar las correcciones en el archivo.";
    case TidyExecutionStatus::FileTooLarge:
        return "El tamaño del archivo supera el límite de seguridad permitido.";
    case TidyExecutionStatus::UnsafePath:
        return "La ruta del archivo no es válida o contiene referencias no permitidas.";
    case TidyExecutionStatus::FileNotFound:
        return "No se encontró el archivo o carpeta especificado.";
    case TidyExecutionStatus::ConfigurationError:
        return "La configuración de análisis no es válida o está incompleta.";
    case TidyExecutionStatus::ToolUnavailable:
        return "La herramienta de análisis de código no está disponible en este sistema.";
    }
    return "Estado de análisis desconocido.";
}

std::string ClangTidyService::generateRemediationHint(TidyExecutionStatus status,
                                                      std::size_t issuesCount) {
    static_cast<void>(issuesCount);
    switch (status) {
    case TidyExecutionStatus::Success:
        return "El código cumple con todas las reglas de calidad establecidas.";
    case TidyExecutionStatus::IssuesFound:
        return "Revise las recomendaciones señaladas o aplique las correcciones automáticas "
               "disponibles.";
    case TidyExecutionStatus::Cancelled:
        return "Puede reiniciar la verificación cuando lo desee.";
    case TidyExecutionStatus::FileLocked:
        return "Cierre otras aplicaciones que puedan estar modificando el archivo e inténtelo de nuevo.";
    case TidyExecutionStatus::PermissionDenied:
        return "Verifique que dispone de permisos de lectura y escritura en la carpeta correspondiente.";
    case TidyExecutionStatus::ReadError:
        return "Compruebe la integridad del archivo y asegúrese de que el disco funcione correctamente.";
    case TidyExecutionStatus::WriteError:
        return "Asegúrese de contar con espacio libre suficiente en el disco y permisos de escritura.";
    case TidyExecutionStatus::FileTooLarge:
        return "Divida el archivo en módulos más pequeños que no superen el límite permitido.";
    case TidyExecutionStatus::UnsafePath:
        return "Utilice únicamente rutas relativas seguras dentro del directorio del proyecto.";
    case TidyExecutionStatus::FileNotFound:
        return "Verifique que la ruta indicada exista antes de iniciar el análisis.";
    case TidyExecutionStatus::ConfigurationError:
        return "Revise el archivo de configuración para asegurarse de que las reglas sean válidas.";
    case TidyExecutionStatus::ToolUnavailable:
        return "Instale la herramienta de análisis en el sistema para habilitar esta comprobación.";
    }
    return "Consulte el manual de usuario para más información sobre este mensaje.";
}

} // namespace vigilio
