#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vigilio {

/// @brief Ámbito de aislamiento del fixture de pruebas.
enum class FixtureScope : uint8_t { PerTest = 0, TestSuite = 1, IsolatedProcess = 2 };

/// @brief Política de limpieza de artefactos tras la ejecución del fixture.
enum class FixtureCleanPolicy : uint8_t { AlwaysClean = 0, RetainOnFailure = 1, ManualCleanup = 2 };

/// @brief Estados formales del ciclo de vida del entorno de pruebas.
enum class TestEnvironmentState : uint8_t {
    Uninitialized = 0,
    Configured = 1,
    Active = 2,
    TornDown = 3,
    Corrupted = 4
};

/// @brief Estrategia de nombrado para aislamiento de carpetas temporales.
enum class TempDirectoryMode : uint8_t { UniqueTimestamped = 0, NamedPrefix = 1, SandboxedIsolated = 2 };

/// @brief Eventos de transición observables durante el ciclo de vida del fixture.
enum class FixtureLifecycleEvent : uint8_t {
    Created = 0,
    DirectoryPrepared = 1,
    EnvOverridden = 2,
    TornDown = 3,
    Cleaned = 4
};

/// @brief Clasificación exhaustiva y tipada de fallos en la infraestructura de prueba.
enum class FixtureError : uint8_t {
    None = 0,
    DirectoryCreationFailed = 1,
    CleanupFailed = 2,
    QuotaExceeded = 3,
    EnvironmentOverrideFailed = 4,
    InvalidConfiguration = 5,
    DirectoryLocked = 6,
    UnsafePathDetected = 7
};

/// @brief Opciones inmutables de configuración para inicializar un entorno de pruebas.
struct TestEnvironmentConfig {
    std::string prefix{"vigilio_test"};
    FixtureCleanPolicy cleanPolicy{FixtureCleanPolicy::AlwaysClean};
    std::size_t maxTempDirSizeBytes{52428800}; // 50 MB límite canónico
    bool isolateEnvironment{false};
    bool autoCleanupWal{true};

    TestEnvironmentConfig() = default;
};

/// @brief Reporte inmutable del estado del fixture y recursos asignados.
struct TestFixtureReport {
    std::string fixtureId;
    std::filesystem::path rootPath;
    std::size_t activeFilesCount{0};
    std::size_t totalSizeBytes{0};
    bool isClean{false};
    TestEnvironmentState state{TestEnvironmentState::Uninitialized};
    std::string diagnosticMessage;

    [[nodiscard]] bool isValid() const noexcept {
        return state != TestEnvironmentState::Corrupted && !rootPath.empty();
    }
};

/// @brief Registro de sobrescritura de variable de entorno para restauración determinista.
struct EnvironmentVariableOverride {
    std::string name;
    bool hasOriginalValue{false};
    std::string originalValue;
    std::string newValue;
};

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

/// @brief Taxonomía fuertemente tipada de resultados operativos del servicio de fixtures.
enum class FixtureOperationStatus : uint8_t {
    Success = 0,
    Cancelled = 1,
    QuotaExceeded = 2,
    PathUnsafe = 3,
    IOError = 4,
    LockContention = 5,
    NotFound = 6,
    Timeout = 7
};

/// @brief Estructura inmutable con el resultado detallado de una operación del servicio.
struct FixtureOperationResult {
    FixtureOperationStatus status{FixtureOperationStatus::Success};
    uint64_t durationMs{0};
    std::size_t filesAffected{0};
    std::size_t bytesAffected{0};
    std::string remediationHint;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == FixtureOperationStatus::Success;
    }
};

/// @brief Reporte inmutable de la detección y purga segura de fixtures huérfanos.
struct OrphanScanReport {
    std::size_t scannedCount{0};
    std::size_t purgedCount{0};
    std::size_t bytesReclaimed{0};
    std::size_t failedCount{0};
    std::string diagnosticMessage;
};

/// @brief Opciones inmutables para el sembrado sintético de un perfil de navegador para pruebas.
struct BrowserProfileSeedOptions {
    std::string profileName{"default"};
    bool includeHistoryDb{true};
    bool includePreferences{true};
    bool includeCache{true};
    std::size_t mockHistoryCount{10};
};

} // namespace vigilio
