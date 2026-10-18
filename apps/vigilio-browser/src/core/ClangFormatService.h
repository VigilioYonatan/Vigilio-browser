#pragma once

#include "core/ClangFormatTypes.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa propietaria desacoplada para inspección y formateo determinista en C++20.
///
/// Proporciona formateo en memoria, inspección en modo dry-run, formateo físico con respaldo y rollback,
/// resiliencia con reintentos exponenciales ante contención de bloqueos, soporte de cancelación
/// cooperativa multihilo (Zero-Zombie) y síntesis determinista de argumentos CLI, sin acoplamiento a GUI
/// ni Chromium.
class ClangFormatService {
  public:
    ClangFormatService();
    explicit ClangFormatService(const ClangFormatOptions& options);
    ~ClangFormatService() = default;

    ClangFormatService(const ClangFormatService&) = delete;
    ClangFormatService& operator=(const ClangFormatService&) = delete;
    ClangFormatService(ClangFormatService&&) noexcept = default;
    ClangFormatService& operator=(ClangFormatService&&) noexcept = default;

    // Configuración del servicio
    [[nodiscard]] bool loadConfiguration(const std::filesystem::path& configPath);
    void setOptions(const ClangFormatOptions& options) noexcept;
    [[nodiscard]] const ClangFormatOptions& options() const noexcept;
    [[nodiscard]] bool isConfigured() const noexcept;

    // Inspección y formateo en memoria pura
    [[nodiscard]] std::string formatSourceCode(std::string_view sourceCode,
                                               const ClangFormatOptions& options);
    [[nodiscard]] SourceFileFormatInspection inspectSource(std::string_view sourceCode,
                                                           std::string_view virtualPath = "virtual.cpp");

    // Inspección en disco (Modo Dry-Run)
    [[nodiscard]] SourceFileFormatInspection inspectFile(const std::filesystem::path& filePath,
                                                         CancellationToken* cancel = nullptr);
    [[nodiscard]] FormatOperationResult inspectDirectory(const std::filesystem::path& dirPath,
                                                         const FormatServiceOptions& options = {},
                                                         CancellationToken* cancel = nullptr);

    // Formateo en disco con resiliencia y respaldo
    [[nodiscard]] FormatOperationResult formatFile(const std::filesystem::path& filePath,
                                                   const FormatServiceOptions& options = {},
                                                   CancellationToken* cancel = nullptr);
    [[nodiscard]] FormatOperationResult formatWithRetry(const std::filesystem::path& filePath,
                                                        const FormatServiceOptions& options = {},
                                                        CancellationToken* cancel = nullptr);
    [[nodiscard]] FormatOperationResult formatDirectory(const std::filesystem::path& dirPath,
                                                        const FormatServiceOptions& options = {},
                                                        CancellationToken* cancel = nullptr);

    // Gestión atómica de respaldos
    [[nodiscard]] bool createBackup(const std::filesystem::path& filePath,
                                    std::string* outBackupPath = nullptr);
    [[nodiscard]] bool rollbackFile(const std::filesystem::path& filePath);
    [[nodiscard]] bool removeBackup(const std::filesystem::path& filePath);

    // Integración CLI
    [[nodiscard]] std::string buildClangFormatCommandLine(const std::filesystem::path& targetPath,
                                                          bool dryRun = true,
                                                          bool inPlace = false) const;

    // Conversión de diagnósticos y UX Copy
    [[nodiscard]] static std::string executionStatusToString(FormatExecutionStatus status);

  private:
    ClangFormatOptions options_;
    bool isConfigured_{false};
    mutable std::mutex mutex_;
};

} // namespace vigilio
