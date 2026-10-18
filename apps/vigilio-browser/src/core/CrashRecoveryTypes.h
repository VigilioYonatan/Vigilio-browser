#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Estados de detección del estado de la sesión previa en disco.
enum class CrashDetectionState : uint8_t {
    NoPreviousSession = 0,   // Sin sesión previa o perfil nuevo
    CleanExit = 1,           // Cierre previo ordenado
    AbnormalTermination = 2, // Cierre previo por crash o fallo abrupto
    RepeatedCrashes = 3,     // Caídas consecutivas repetidas (bucle de caídas)
    CorruptedSession = 4,    // Archivo de sesión o centinela ilegible
    Recovered = 5            // Decisión procesada (estado terminal)
};

/// @brief Acciones resolutivas recomendadas para el navegador.
enum class RecoveryAction : uint8_t {
    StartClean = 0,       // Abrir página de inicio limpia
    RestoreSession = 1,   // Restaurar pestañas automáticamente
    PromptRestore = 2,    // Mostrar banner accesible al usuario
    SafeModeRecovery = 3, // Recomendar inicio en modo seguro
    DiscardCorrupted = 4  // Archivar datos dañados y reiniciar limpio
};

/// @brief Causa diagnosticada del cierre.
enum class CrashReason : uint8_t {
    Unknown = 0,
    ProcessCrash = 1,
    WatchdogTimeout = 2,
    MemoryExhaustion = 3,
    StorageFailure = 4,
    CleanShutdown = 5
};

/// @brief Estados operativos de la capa de servicio de recuperación.
enum class RecoveryServiceState : uint8_t {
    Uninitialized = 0,
    Inspected = 1,
    AwaitingDecision = 2,
    SessionRestored = 3,
    SessionDiscarded = 4,
    SessionActive = 5,
    SessionClean = 6
};

/// @brief Decisión de resolución tomada respecto a la sesión recuperable.
enum class RecoveryDecision : uint8_t { Pending = 0, Restore = 1, Discard = 2, Cancel = 3 };

/// @brief Registro estructurado de la resolución aplicada por el servicio.
struct RecoveryResolutionResult {
    bool success = false;
    RecoveryServiceState state = RecoveryServiceState::Uninitialized;
    RecoveryDecision decision = RecoveryDecision::Pending;
    std::vector<std::string> restoredUrls;
    std::string diagnosticMessage;
};

/// @brief Estructura del archivo centinela persistido en disco.
struct CrashSentinel {
    uint32_t schemaVersion = 1;
    bool cleanExit = false;
    uint32_t consecutiveCrashes = 0;
    uint64_t pid = 0;
    int64_t sessionStartTime = 0;
    int64_t lastHeartbeatTime = 0;
    CrashReason lastCrashReason = CrashReason::Unknown;
    std::string appVersion = "0.1.0-alpha";
};

/// @brief Parámetros de configuración de la política de recuperación.
struct CrashRecoveryConfig {
    bool isIncognito = false;
    bool isSafeMode = false;
    uint32_t crashLoopThreshold = 2;
    uint32_t sessionAutosaveIntervalMs = 30000;
    std::string sentinelFilePath;
    std::string sessionFilePath;
    std::string profileDirectory;
};

/// @brief Reporte consolidado emitido tras inspeccionar el entorno.
struct CrashRecoveryReport {
    CrashDetectionState state = CrashDetectionState::NoPreviousSession;
    RecoveryAction recommendedAction = RecoveryAction::StartClean;
    uint32_t consecutiveCrashCount = 0;
    std::vector<std::string> recoverableUrls;
    std::string failureReason;
    bool isSafeModeAdvised = false;
    bool sessionCorrupted = false;
    bool backupCreated = false;
    std::string diagnosticMessage;

    [[nodiscard]] constexpr bool hasRecoverableTabs() const noexcept {
        return !recoverableUrls.empty();
    }
};

} // namespace vigilio
