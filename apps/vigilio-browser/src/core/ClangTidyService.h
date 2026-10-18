#pragma once

#include "core/ClangTidyTypes.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa propietaria desacoplada para análisis estático, inspección y corrección en
/// C++20.
///
/// Proporciona análisis en memoria, corrección determinista (autofix), inspección en modo dry-run,
/// corrección física con respaldo atómico y rollback, resiliencia con reintentos exponenciales ante
/// contención de bloqueos, soporte de cancelación cooperativa multihilo (Zero-Zombie) y síntesis
/// determinista de argumentos CLI, sin acoplamiento a GUI ni Chromium.
class ClangTidyService {
  public:
    ClangTidyService();
    explicit ClangTidyService(const ClangTidyOptions& options);
    ~ClangTidyService() = default;

    ClangTidyService(const ClangTidyService&) = delete;
    ClangTidyService& operator=(const ClangTidyService&) = delete;
    ClangTidyService(ClangTidyService&&) noexcept = default;
    ClangTidyService& operator=(ClangTidyService&&) noexcept = default;

    // Configuración del servicio
    [[nodiscard]] bool loadConfiguration(const std::filesystem::path& configPath);
    void setOptions(const ClangTidyOptions& options) noexcept;
    [[nodiscard]] const ClangTidyOptions& options() const noexcept;
    [[nodiscard]] bool isConfigured() const noexcept;

    // Análisis y corrección en memoria pura
    [[nodiscard]] SourceFileTidyInspection inspectSource(std::string_view sourceCode,
                                                         std::string_view virtualPath = "virtual.cpp",
                                                         const ClangTidyOptions& options = {});

    [[nodiscard]] std::string applyFixesToSource(std::string_view sourceCode,
                                                 const ClangTidyOptions& options = {});

    // Inspección en disco (Modo Dry-Run)
    [[nodiscard]] SourceFileTidyInspection inspectFile(const std::filesystem::path& filePath,
                                                       CancellationToken* cancel = nullptr);

    [[nodiscard]] TidyOperationResult inspectDirectory(const std::filesystem::path& dirPath,
                                                       const TidyServiceOptions& options = {},
                                                       CancellationToken* cancel = nullptr);

    // Corrección física en disco con resiliencia y respaldo
    [[nodiscard]] TidyOperationResult applyFixesToFile(const std::filesystem::path& filePath,
                                                       const TidyServiceOptions& options = {},
                                                       CancellationToken* cancel = nullptr);

    [[nodiscard]] TidyOperationResult applyFixesWithRetry(const std::filesystem::path& filePath,
                                                          const TidyServiceOptions& options = {},
                                                          CancellationToken* cancel = nullptr);

    [[nodiscard]] TidyOperationResult applyFixesToDirectory(const std::filesystem::path& dirPath,
                                                            const TidyServiceOptions& options = {},
                                                            CancellationToken* cancel = nullptr);

    // Gestión atómica de respaldos (.bak)
    [[nodiscard]] bool createBackup(const std::filesystem::path& filePath,
                                    std::string* outBackupPath = nullptr);
    [[nodiscard]] bool rollbackFile(const std::filesystem::path& filePath);
    [[nodiscard]] bool removeBackup(const std::filesystem::path& filePath);

    // Integración y síntesis CLI
    [[nodiscard]] std::string buildClangTidyCommandLine(const std::filesystem::path& targetPath,
                                                        const std::filesystem::path& buildPath = "",
                                                        bool fix = false,
                                                        bool warningsAsErrors = false) const;

    // Conversores de diagnósticos y UX Copy
    [[nodiscard]] static std::string executionStatusToString(TidyExecutionStatus status);
    [[nodiscard]] static std::string generateRemediationHint(TidyExecutionStatus status,
                                                             std::size_t issuesCount = 0);

  private:
    ClangTidyOptions options_;
    bool isConfigured_{false};
    mutable std::mutex mutex_;
};

} // namespace vigilio
