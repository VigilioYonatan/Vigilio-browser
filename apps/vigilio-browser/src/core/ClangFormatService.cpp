#include "core/ClangFormatService.h"
#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatValidator.h"

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace vigilio {

ClangFormatService::ClangFormatService()
    : options_(ClangFormatConfiguration::canonicalConfig()), isConfigured_(false) {
}

ClangFormatService::ClangFormatService(const ClangFormatOptions& options)
    : options_(options),
      isConfigured_(ClangFormatValidator::validateOptions(options) == ClangFormatError::None) {
}

bool ClangFormatService::loadConfiguration(const std::filesystem::path& configPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto report = ClangFormatValidator::validateConfigurationFile(configPath);
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
    ClangFormatOptions opts;
    if (!ClangFormatValidator::parseConfig(buffer.str(), opts)) {
        isConfigured_ = false;
        return false;
    }
    options_ = opts;
    isConfigured_ = true;
    return true;
}

void ClangFormatService::setOptions(const ClangFormatOptions& options) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
    isConfigured_ = (ClangFormatValidator::validateOptions(options) == ClangFormatError::None);
}

const ClangFormatOptions& ClangFormatService::options() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

bool ClangFormatService::isConfigured() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return isConfigured_;
}

std::string ClangFormatService::formatSourceCode(std::string_view sourceCode,
                                                 const ClangFormatOptions& options) {
    if (ClangFormatValidator::validateOptions(options) != ClangFormatError::None) {
        return std::string(sourceCode);
    }
    if (sourceCode.empty()) {
        return "";
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::string styleArg =
        "--style={BasedOnStyle: " + ClangFormatValidator::stylePresetToString(options.stylePreset) +
        ", IndentWidth: " + std::to_string(options.indentWidth) +
        ", ColumnLimit: " + std::to_string(options.columnLimit) + ", PointerAlignment: " +
        ClangFormatValidator::pointerAlignmentToString(options.pointerAlignment) +
        ", AllowShortFunctionsOnASingleLine: " +
        ClangFormatValidator::shortFunctionPolicyToString(options.allowShortFunctions);
    if (!options.cppStandard.empty()) {
        styleArg += ", Standard: " + options.cppStandard;
    }
    styleArg += "}";

    int pipeIn[2];
    int pipeOut[2];
    if (::pipe2(pipeIn, O_CLOEXEC) != 0) {
        return std::string(sourceCode);
    }
    if (::pipe2(pipeOut, O_CLOEXEC) != 0) {
        ::close(pipeIn[0]);
        ::close(pipeIn[1]);
        return std::string(sourceCode);
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipeIn[0]);
        ::close(pipeIn[1]);
        ::close(pipeOut[0]);
        ::close(pipeOut[1]);
        return std::string(sourceCode);
    }

    if (pid == 0) {
        ::close(pipeIn[1]);
        ::close(pipeOut[0]);
        ::dup2(pipeIn[0], STDIN_FILENO);
        ::dup2(pipeOut[1], STDOUT_FILENO);
        int devNull = ::open("/dev/null", O_WRONLY);
        if (devNull >= 0) {
            ::dup2(devNull, STDERR_FILENO);
            ::close(devNull);
        }
        ::close(pipeIn[0]);
        ::close(pipeOut[1]);

        ::execlp("clang-format", "clang-format", styleArg.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }

    ::close(pipeIn[0]);
    ::close(pipeOut[1]);

    struct sigaction sa{};
    struct sigaction old_sa{};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ::sigaction(SIGPIPE, &sa, &old_sa);

    std::size_t written = 0;
    while (written < sourceCode.size()) {
        ssize_t n = ::write(pipeIn[1], sourceCode.data() + written, sourceCode.size() - written);
        if (n <= 0) {
            break;
        }
        written += static_cast<std::size_t>(n);
    }
    ::close(pipeIn[1]);
    ::sigaction(SIGPIPE, &old_sa, nullptr);

    std::string formatted;
    char buffer[4096];
    ssize_t bytesRead = 0;
    while ((bytesRead = ::read(pipeOut[0], buffer, sizeof(buffer))) > 0) {
        formatted.append(buffer, static_cast<std::size_t>(bytesRead));
    }
    ::close(pipeOut[0]);

    int status = 0;
    ::waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && !formatted.empty()) {
        return formatted;
    }
    return std::string(sourceCode);
}

SourceFileFormatInspection ClangFormatService::inspectSource(std::string_view sourceCode,
                                                             std::string_view virtualPath) {
    SourceFileFormatInspection inspection;
    inspection.relativePath = std::string(virtualPath);
    if (sourceCode.empty()) {
        inspection.isCompliant = true;
        inspection.lineCount = 0;
        inspection.discrepancySummary = "";
        return inspection;
    }

    int lines = 1;
    for (char c : sourceCode) {
        if (c == '\n') {
            ++lines;
        }
    }
    inspection.lineCount = lines;

    ClangFormatOptions opts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        opts = isConfigured_ ? options_ : ClangFormatConfiguration::canonicalConfig();
    }

    std::string formatted = formatSourceCode(sourceCode, opts);
    if (formatted == sourceCode) {
        inspection.isCompliant = true;
        inspection.discrepancySummary = "";
    } else {
        inspection.isCompliant = false;
        inspection.discrepancySummary = "Discrepancia detectada en sangría o alineación de estilo.";
    }
    return inspection;
}

SourceFileFormatInspection ClangFormatService::inspectFile(const std::filesystem::path& filePath,
                                                           CancellationToken* cancel) {
    SourceFileFormatInspection inspection;
    inspection.relativePath = filePath.string();

    if (cancel != nullptr && cancel->isCancelled()) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::Cancelled);
        return inspection;
    }

    if (!ClangFormatValidator::isSafeConfigPath(filePath)) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::UnsafePath);
        return inspection;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::FileNotFound);
        return inspection;
    }

    auto sz = std::filesystem::file_size(filePath, ec);
    if (sz > 10 * 1024 * 1024) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::FileTooLarge);
        return inspection;
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::ReadError);
        return inspection;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    if (cancel != nullptr && cancel->isCancelled()) {
        inspection.isCompliant = false;
        inspection.discrepancySummary = executionStatusToString(FormatExecutionStatus::Cancelled);
        return inspection;
    }

    return inspectSource(content, filePath.string());
}

FormatOperationResult ClangFormatService::inspectDirectory(const std::filesystem::path& dirPath,
                                                           const FormatServiceOptions& options,
                                                           CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    FormatOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FormatExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangFormatValidator::isSafeConfigPath(dirPath)) {
        result.status = FormatExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
        result.status = FormatExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::filesystem::recursive_directory_iterator it(
        dirPath, std::filesystem::directory_options::skip_permission_denied, ec);
    const std::filesystem::recursive_directory_iterator endIt;

    while (it != endIt && !ec) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = FormatExecutionStatus::Cancelled;
            result.remediationHint = executionStatusToString(result.status);
            break;
        }

        if (static_cast<std::size_t>(it.depth()) > options.maxScanDepth) {
            it.pop();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_directory(ec)) {
            if (ClangFormatConfiguration::isExcludedDirectory(entry.path())) {
                it.disable_recursion_pending();
            }
            it.increment(ec);
            continue;
        }

        if (entry.is_regular_file(ec)) {
            if (ClangFormatConfiguration::isSupportedSourceExtension(entry.path())) {
                result.filesExamined++;
                auto insp = inspectFile(entry.path(), cancel);
                if (!insp.isCompliant) {
                    result.discrepanciesCount++;
                }
                result.details.push_back(insp);
            }
        }
        it.increment(ec);
    }

    if (result.status != FormatExecutionStatus::Cancelled) {
        if (result.discrepanciesCount > 0) {
            result.status = FormatExecutionStatus::DiscrepancyFound;
        } else {
            result.status = FormatExecutionStatus::Success;
        }
        result.remediationHint = executionStatusToString(result.status);
    }

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

bool ClangFormatService::createBackup(const std::filesystem::path& filePath,
                                      std::string* outBackupPath) {
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

bool ClangFormatService::rollbackFile(const std::filesystem::path& filePath) {
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

bool ClangFormatService::removeBackup(const std::filesystem::path& filePath) {
    std::filesystem::path backupPath = filePath;
    backupPath += ".bak";
    std::error_code ec;
    if (!std::filesystem::exists(backupPath, ec) || ec) {
        return true;
    }
    return std::filesystem::remove(backupPath, ec);
}

FormatOperationResult ClangFormatService::formatFile(const std::filesystem::path& filePath,
                                                     const FormatServiceOptions& options,
                                                     CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    FormatOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FormatExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangFormatValidator::isSafeConfigPath(filePath)) {
        result.status = FormatExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        result.status = FormatExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    auto sz = std::filesystem::file_size(filePath, ec);
    if (sz > options.maxFileSizeBytes) {
        result.status = FormatExecutionStatus::FileTooLarge;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        result.status = FormatExecutionStatus::ReadError;
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
        if (!insp.isCompliant) {
            result.discrepanciesCount = 1;
            result.status = FormatExecutionStatus::DiscrepancyFound;
        } else {
            result.status = FormatExecutionStatus::Success;
        }
        result.remediationHint = executionStatusToString(result.status);
        const auto endTime = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return result;
    }

    ClangFormatOptions opts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        opts = isConfigured_ ? options_ : ClangFormatConfiguration::canonicalConfig();
    }

    std::string formatted = formatSourceCode(content, opts);
    if (formatted == content) {
        result.status = FormatExecutionStatus::Success;
        result.remediationHint = executionStatusToString(result.status);
        SourceFileFormatInspection insp;
        insp.relativePath = filePath.string();
        insp.isCompliant = true;
        result.details.push_back(insp);
        const auto endTime = std::chrono::steady_clock::now();
        result.durationMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
        return result;
    }

    if (options.createBackup) {
        if (!createBackup(filePath)) {
            result.status = FormatExecutionStatus::WriteError;
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
        result.status = FormatExecutionStatus::WriteError;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    out << formatted;
    if (!out.good()) {
        out.close();
        if (options.createBackup) {
            static_cast<void>(rollbackFile(filePath));
            static_cast<void>(removeBackup(filePath));
        }
        result.status = FormatExecutionStatus::WriteError;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }
    out.close();

    if (options.createBackup) {
        static_cast<void>(removeBackup(filePath));
    }

    result.filesFormatted = 1;
    result.discrepanciesCount = 1;
    result.status = FormatExecutionStatus::Success;
    result.remediationHint = executionStatusToString(result.status);
    SourceFileFormatInspection insp;
    insp.relativePath = filePath.string();
    insp.isCompliant = true;
    result.details.push_back(insp);

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

FormatOperationResult ClangFormatService::formatWithRetry(const std::filesystem::path& filePath,
                                                          const FormatServiceOptions& options,
                                                          CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();

    if (cancel != nullptr && cancel->isCancelled()) {
        FormatOperationResult res;
        res.status = FormatExecutionStatus::Cancelled;
        res.remediationHint = executionStatusToString(res.status);
        return res;
    }

    FormatOperationResult res = formatFile(filePath, options, cancel);
    if (res.status == FormatExecutionStatus::Success || res.status == FormatExecutionStatus::Cancelled ||
        res.status == FormatExecutionStatus::UnsafePath ||
        res.status == FormatExecutionStatus::FileTooLarge ||
        res.status == FormatExecutionStatus::FileNotFound) {
        return res;
    }

    for (uint32_t attempt = 1; attempt <= options.maxRetries; ++attempt) {
        if (cancel != nullptr && cancel->isCancelled()) {
            res.status = FormatExecutionStatus::Cancelled;
            res.remediationHint = executionStatusToString(res.status);
            return res;
        }

        uint32_t delayMs = options.retryBackoffMs * (1U << (attempt - 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        if (cancel != nullptr && cancel->isCancelled()) {
            res.status = FormatExecutionStatus::Cancelled;
            res.remediationHint = executionStatusToString(res.status);
            return res;
        }

        res = formatFile(filePath, options, cancel);
        if (res.status == FormatExecutionStatus::Success) {
            const auto endTime = std::chrono::steady_clock::now();
            res.durationMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
            return res;
        }
    }

    res.status = FormatExecutionStatus::FileLocked;
    res.remediationHint = executionStatusToString(res.status);
    const auto endTime = std::chrono::steady_clock::now();
    res.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return res;
}

FormatOperationResult ClangFormatService::formatDirectory(const std::filesystem::path& dirPath,
                                                          const FormatServiceOptions& options,
                                                          CancellationToken* cancel) {
    const auto startTime = std::chrono::steady_clock::now();
    FormatOperationResult result;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = FormatExecutionStatus::Cancelled;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    if (!ClangFormatValidator::isSafeConfigPath(dirPath)) {
        result.status = FormatExecutionStatus::UnsafePath;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
        result.status = FormatExecutionStatus::FileNotFound;
        result.remediationHint = executionStatusToString(result.status);
        return result;
    }

    std::filesystem::recursive_directory_iterator it(
        dirPath, std::filesystem::directory_options::skip_permission_denied, ec);
    const std::filesystem::recursive_directory_iterator endIt;

    while (it != endIt && !ec) {
        if (cancel != nullptr && cancel->isCancelled()) {
            result.status = FormatExecutionStatus::Cancelled;
            result.remediationHint = executionStatusToString(result.status);
            break;
        }

        if (static_cast<std::size_t>(it.depth()) > options.maxScanDepth) {
            it.pop();
            continue;
        }

        const auto& entry = *it;
        if (entry.is_directory(ec)) {
            if (ClangFormatConfiguration::isExcludedDirectory(entry.path())) {
                it.disable_recursion_pending();
            }
            it.increment(ec);
            continue;
        }

        if (entry.is_regular_file(ec)) {
            if (ClangFormatConfiguration::isSupportedSourceExtension(entry.path())) {
                result.filesExamined++;
                auto fileRes = formatWithRetry(entry.path(), options, cancel);
                if (fileRes.filesFormatted > 0) {
                    result.filesFormatted += fileRes.filesFormatted;
                    result.discrepanciesCount += fileRes.discrepanciesCount;
                }
                for (const auto& d : fileRes.details) {
                    result.details.push_back(d);
                }
                if (fileRes.status == FormatExecutionStatus::Cancelled) {
                    result.status = FormatExecutionStatus::Cancelled;
                    break;
                }
            }
        }
        it.increment(ec);
    }

    if (result.status != FormatExecutionStatus::Cancelled) {
        result.status = FormatExecutionStatus::Success;
        result.remediationHint = executionStatusToString(result.status);
    }

    const auto endTime = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    return result;
}

std::string ClangFormatService::buildClangFormatCommandLine(const std::filesystem::path& targetPath,
                                                            bool dryRun, bool inPlace) const {
    std::string cmd = "clang-format";
    if (dryRun) {
        cmd += " --dry-run --Werror";
    }
    if (inPlace) {
        cmd += " -i";
    }
    cmd += " --style=file";
    if (!targetPath.empty()) {
        cmd += " ";
        cmd += targetPath.string();
    }
    return cmd;
}

std::string ClangFormatService::executionStatusToString(FormatExecutionStatus status) {
    switch (status) {
    case FormatExecutionStatus::Success:
        return "Formato verificado correctamente sin discrepancias.";
    case FormatExecutionStatus::DiscrepancyFound:
        return "Se detectaron diferencias de estilo en los archivos inspeccionados.";
    case FormatExecutionStatus::Cancelled:
        return "La verificación de formato fue cancelada por el usuario.";
    case FormatExecutionStatus::FileLocked:
        return "El archivo está siendo utilizado por otra aplicación. Por favor, intente nuevamente.";
    case FormatExecutionStatus::PermissionDenied:
        return "No se cuenta con permisos para acceder al archivo seleccionado.";
    case FormatExecutionStatus::ReadError:
        return "Ocurrió un problema de lectura al examinar el archivo fuente.";
    case FormatExecutionStatus::WriteError:
        return "No fue posible guardar los cambios de formato en el archivo.";
    case FormatExecutionStatus::FileTooLarge:
        return "El tamaño del archivo supera el límite de seguridad permitido.";
    case FormatExecutionStatus::UnsafePath:
        return "La ruta del archivo no es válida o contiene referencias no permitidas.";
    case FormatExecutionStatus::FileNotFound:
        return "No se encontró el archivo o carpeta especificado.";
    case FormatExecutionStatus::ConfigurationError:
        return "La configuración de formato no es válida o está incompleta.";
    case FormatExecutionStatus::ToolUnavailable:
        return "La herramienta de formato no está disponible en este sistema.";
    }
    return "Estado de formato desconocido.";
}

} // namespace vigilio
