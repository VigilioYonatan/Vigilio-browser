# Design — module-foundations-task0025

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Gestión procedural de cierre en `apps/vigilio-browser`**:
   - Actualmente, la aplicación inicia el bucle de eventos mediante `return app.exec();` en `apps/vigilio-browser/src/app.cpp` tras la ejecución exitosa de `StartupBootstrapCoordinator::executeBootstrap`.
   - El cierre de la aplicación se gestiona de forma pasiva y dependiente exclusivamente de Qt a través de `BrowserWindow::closeEvent`:
     ```cpp
     void BrowserWindow::closeEvent(QCloseEvent* event) {
         saveSession();
         QMainWindow::closeEvent(event);
     }
     ```
   - La persistencia de pestañas en `saveSession()` utiliza directamente `QSettings().setValue("session/urls", urls);`, sin pasar por el módulo de persistencia atómica ya implementado en `apps/vigilio-browser/src/core/SessionFile.h` (`vigilio::saveSession`).
   - No existe un modelo de dominio formal para el ciclo de vida de la aplicación (`Application Lifetime`): los estados del ciclo de vida no están tipados, no se validan las transiciones legales y no se registran reportes de terminación.

2. **Ausencia de captura segura de señales del sistema operativo**:
   - Si la aplicación es ejecutada en una terminal y el usuario presiona `Ctrl+C` (`SIGINT`), o si el sistema operativo envía `SIGTERM` durante un apagado o reinicio de sesión, el proceso termina abruptamente sin invocar `BrowserWindow::closeEvent`.
   - Esta terminación abrupta puede provocar:
     - Pérdida completa de las pestañas abiertas en la sesión activa.
     - Posible corrupción o transacciones pendientes en la base de datos local SQLite (`LocalRepository`).
     - Falta de liberación limpia de recursos compartidos o subprocesos Chromium auxiliares.

3. **Falta de temporizador de seguridad contra procesos colgados (Zero-Zombie)**:
   - No existe un mecanismo de vigilancia (watchdog) con temporizador máximo de apagado. Si una operación de persistencia en disco o un callback de desmontaje de subsistemas se bloquea indefinidamente (por ejemplo por un bloqueo en el sistema de archivos de red o contención en SQLite), la aplicación queda colgada como un proceso zombi.

4. **Persistencia atómica y aislamiento de incógnito existentes**:
   - En `apps/vigilio-browser/src/core/SessionFile.h` existe una implementación probada y atómica de guardado de URLs (`saveSession`) que utiliza escritura en `.tmp` y renombrado atómico (`MOVEFILE_REPLACE_EXISTING` en Windows y `std::filesystem::rename` en POSIX).
   - En `BrowserWindow`, existe una bandera `isIncognito_` que previene el guardado en `QSettings`, pero esta protección no está coordinada centralizadamente a nivel de dominio.

### Estado objetivo (TARGET)

1. **Coordinador desacoplado de dominio (`ApplicationLifetimeCoordinator`)**:
   - Centralizar y formalizar toda la lógica del ciclo de vida en `ApplicationLifetimeCoordinator`, alojado en la biblioteca estática `vigilio_core`.
   - La clase es completamente independiente de Qt Widgets y de `QApplication`, operando con tipos nativos de C++20 (`std::string`, `std::function`, `std::atomic`, `std::chrono`).
   - Proporciona un control determinista de estados y orquesta el vaciado de datos y desmantelamiento de subsistemas tanto en entornos GUI como en ejecuciones automatizadas headless.

2. **Máquina de estados determinista (`AppLifetimeState`)**:
   - Formalizar 9 estados discretos: `Uninitialized`, `Starting`, `Running`, `Suspending`, `ShutdownRequested`, `FlushingState`, `TerminatingSubsystems`, `Terminated`, `EmergencyAbort`.
   - Cada estado cuenta con precondiciones rigurosas, matriz de transiciones permitidas (`canTransition`) y causas tipadas de terminación (`ShutdownReason`).

3. **Orquestación segura de apagado con persistencia atómica**:
   - Implementar `executeShutdown` para coordinar el vaciado de estado:
     1. Recepción y validación de la solicitud (`ShutdownReason`).
     2. Transición a `ShutdownRequested` con deduplicación de solicitudes concurrentes.
     3. Transición a `FlushingState`: ejecución de guardado atómico de pestañas (delegado en `vigilio::saveSession`) y confirmación de transacciones en SQLite (`LocalRepository`).
     4. Transición a `TerminatingSubsystems`: desmontaje ordenado de recursos en orden inverso a su dependencia.
     5. Transición a `Terminated`: emisión de reporte inmutable `LifetimeReport`.

4. **Invariante constitucional de incógnito y privacidad (Q-001)**:
   - Cuando `config.isIncognito == true`, `ApplicationLifetimeCoordinator` garantiza que `sessionSaver` jamás sea invocado y ningún dato de sesión, historial o preferencias se escriba en disco al salir.

5. **Temporizador de seguridad contra procesos zombis**:
   - Límite máximo de apagado de 3000 ms (`shutdownTimeoutMs`). Si el vaciado de datos o desmontaje supera este plazo, el coordinador transiciona inmediatamente a `EmergencyAbort`, abortando de forma controlada y evitando procesos colgados.

6. **Manejo seguro de señales e interrupciones del SO**:
   - La detección de señales del sistema operativo (`SIGINT`, `SIGTERM`) se desacopla mediante banderas atómicas (`std::atomic<bool>`) y razones tipadas (`ShutdownReason::SignalInterrupt`, `ShutdownReason::SignalTerminate`), garantizando signal-safety sin llamadas reentrantes a memoria dinámica o widgets dentro del manejador de señales.

### Brechas identificadas (GAPS)

1. **Brecha de abstracción**: El ciclo de vida de la aplicación no está modelado como una entidad de dominio en `vigilio_core`, sino diluido en métodos dispersos de UI.
2. **Brecha de transiciones**: No existe validación de qué estados son legales ni protección contra transiciones anómalas (por ejemplo, reanudar una ejecución que ya está en proceso de apagado).
3. **Brecha de señales del SO**: El cierre ante señales `SIGINT`/`SIGTERM` no está contemplado a nivel de arquitectura, provocando cierres abruptos y pérdida de estado.
4. **Brecha de persistencia atómica**: `BrowserWindow` persiste la sesión mediante `QSettings` ordinario en lugar de utilizar el módulo atómico `SessionFile.h`.
5. **Brecha de temporizador de seguridad**: Ausencia de límite temporal de apagado que garantice el cumplimiento de la política Zero-Zombie.

---

## Detailed Architecture and Domain Model

### 1. Diagrama de flujo de la máquina de estados de ciclo de vida

```
                           [ Proceso iniciado ]
                                     │
                                     ▼
                            [ Uninitialized ]
                                     │
                    (Inicialización del coordinador)
                                     │
                                     ▼
                                [ Starting ] ────(Fallo irrecuperable)───► [ EmergencyAbort ]
                                     │                                            ▲
                     (Bootstrap exitoso -> app.exec())                            │
                                     │                                            │
                                     ▼                                            │
               ┌──────────────► [ Running ] ──────────────────────────────────────┤
               │                     │                                            │
   (Reanudar tras suspensión)        ├──────────────────┐                         │
               │                     │                  │                         │
               │                     ▼                  ▼                         │
               └─────────────── [ Suspending ]  [ ShutdownRequested ]             │
                                                        │                         │
                                            (Aceptación de apagado)               │
                                                        │                         │
                                                        ▼                         │
                                                 [ FlushingState ] ───────────────┤ (Timeout > 3000ms
                                                        │                           o fallo crítico)
                                            (Persistencia completada)             │
                                                        │                         │
                                                        ▼                         │
                                             [ TerminatingSubsystems ] ───────────┘
                                                        │
                                             (Desmontaje completado)
                                                        │
                                                        ▼
                                                  [ Terminated ]
                                                        │
                                                        ▼
                                                 [ Proceso finalizado ]
```

### 2. Matriz de transiciones legales (`canTransition`)

La función pura `canTransition` evalúa la legalidad de cualquier cambio de estado, rechazando transiciones anómalas:

```cpp
constexpr bool ApplicationLifetimeCoordinator::canTransition(
    AppLifetimeState from, AppLifetimeState to) noexcept {
    switch (from) {
        case AppLifetimeState::Uninitialized:
            return to == AppLifetimeState::Starting;
        case AppLifetimeState::Starting:
            return to == AppLifetimeState::Running ||
                   to == AppLifetimeState::ShutdownRequested ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::Running:
            return to == AppLifetimeState::Suspending ||
                   to == AppLifetimeState::ShutdownRequested ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::Suspending:
            return to == AppLifetimeState::Running ||
                   to == AppLifetimeState::ShutdownRequested ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::ShutdownRequested:
            return to == AppLifetimeState::FlushingState ||
                   to == AppLifetimeState::Running || // En caso de que el usuario cancele el cierre
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::FlushingState:
            return to == AppLifetimeState::TerminatingSubsystems ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::TerminatingSubsystems:
            return to == AppLifetimeState::Terminated ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::EmergencyAbort:
            return to == AppLifetimeState::Terminated;
        case AppLifetimeState::Terminated:
            return false; // Estado final inmutable
    }
    return false;
}
```

### 3. Enumeraciones y tipos de valor (`LifetimeTypes.h`)

Las estructuras y enumeraciones proporcionan tipos fuertemente tipados para toda la arquitectura:

```cpp
namespace vigilio {

enum class AppLifetimeState : uint8_t {
    Uninitialized = 0,
    Starting = 1,
    Running = 2,
    Suspending = 3,
    ShutdownRequested = 4,
    FlushingState = 5,
    TerminatingSubsystems = 6,
    Terminated = 7,
    EmergencyAbort = 8
};

enum class ShutdownReason : uint8_t {
    UserWindowClose = 0,
    UserExplicitQuit = 1,
    SignalInterrupt = 2,
    SignalTerminate = 3,
    SessionLogout = 4,
    StorageFailure = 5,
    MemoryExhaustion = 6,
    EmergencyAbort = 7
};

enum class ShutdownAction : uint8_t {
    ProceedGraceful = 0,
    ProceedImmediate = 1,
    PromptUser = 2,
    IgnoreRequest = 3
};

struct LifetimeConfig {
    bool isIncognito = false;
    bool isSafeMode = false;
    uint32_t shutdownTimeoutMs = 3000;
    uint32_t sessionAutosaveIntervalMs = 0;
    std::string sessionFilePath;
    std::string profileDirectory;
};

struct LifetimeReport {
    AppLifetimeState finalState = AppLifetimeState::Uninitialized;
    ShutdownReason reason = ShutdownReason::UserWindowClose;
    int exitCode = 0;
    bool sessionSaved = false;
    bool databaseClosedCleanly = false;
    bool timeoutTriggered = false;
    uint64_t sessionFlushElapsedMicroseconds = 0;
    uint64_t teardownElapsedMicroseconds = 0;
    std::string diagnosticMessage;

    [[nodiscard]] constexpr bool isCleanExit() const noexcept {
        return (finalState == AppLifetimeState::Terminated && exitCode == 0 && !timeoutTriggered);
    }
};

} // namespace vigilio
```

### 4. Especificación del coordinador de dominio (`ApplicationLifetimeCoordinator`)

El coordinador encapsula el estado atómico del ciclo de vida y coordina el vaciado de datos:

```cpp
namespace vigilio {

class ApplicationLifetimeCoordinator {
public:
    explicit ApplicationLifetimeCoordinator(LifetimeConfig config) noexcept
        : config_(std::move(config)) {}

    [[nodiscard]] AppLifetimeState currentState() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool isShutdownRequested() const noexcept {
        const auto st = currentState();
        return st == AppLifetimeState::ShutdownRequested ||
               st == AppLifetimeState::FlushingState ||
               st == AppLifetimeState::TerminatingSubsystems ||
               st == AppLifetimeState::EmergencyAbort ||
               st == AppLifetimeState::Terminated;
    }

    [[nodiscard]] bool isIncognito() const noexcept {
        return config_.isIncognito;
    }

    [[nodiscard]] const LifetimeConfig& configuration() const noexcept {
        return config_;
    }

    bool transitionTo(AppLifetimeState newState) noexcept {
        auto expected = state_.load(std::memory_order_relaxed);
        while (canTransition(expected, newState)) {
            if (state_.compare_exchange_weak(expected, newState,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    ShutdownAction requestShutdown(ShutdownReason reason) noexcept {
        const uint32_t count = shutdownRequestCount_.fetch_add(1, std::memory_order_relaxed);
        
        // Si ya hay un apagado en curso:
        if (shutdownInProgress_.load(std::memory_order_acquire)) {
            // Si el usuario envía señales repetidas (ej. 3 veces Ctrl+C), forzar salida inmediata
            if (count >= 2 && (reason == ShutdownReason::SignalInterrupt || reason == ShutdownReason::SignalTerminate)) {
                transitionTo(AppLifetimeState::EmergencyAbort);
                return ShutdownAction::ProceedImmediate;
            }
            return ShutdownAction::IgnoreRequest;
        }

        if (reason == ShutdownReason::EmergencyAbort || reason == ShutdownReason::MemoryExhaustion) {
            transitionTo(AppLifetimeState::EmergencyAbort);
            return ShutdownAction::ProceedImmediate;
        }

        if (transitionTo(AppLifetimeState::ShutdownRequested)) {
            return ShutdownAction::ProceedGraceful;
        }

        return ShutdownAction::IgnoreRequest;
    }

    LifetimeReport executeShutdown(
        ShutdownReason reason,
        std::function<bool()> sessionSaver = nullptr,
        std::function<bool()> dbCloser = nullptr) noexcept;

    static constexpr bool canTransition(AppLifetimeState from, AppLifetimeState to) noexcept;
    static constexpr bool isSignalSafe(ShutdownReason reason) noexcept;
    static constexpr std::string_view stateToString(AppLifetimeState state) noexcept;
    static constexpr std::string_view reasonToString(ShutdownReason reason) noexcept;
    static constexpr std::string_view actionToString(ShutdownAction action) noexcept;

private:
    LifetimeConfig config_;
    std::atomic<AppLifetimeState> state_{AppLifetimeState::Uninitialized};
    std::atomic<bool> shutdownInProgress_{false};
    std::atomic<uint32_t> shutdownRequestCount_{0};
};

} // namespace vigilio
```

### 5. Protocolo de persistencia atómica y vaciado ordenado

El método `executeShutdown` orquesta la persistencia y desmantelamiento con medición precisa de latencia:
1. Marca `shutdownInProgress_ = true`.
2. Si el estado actual es `EmergencyAbort`, salta directamente a la liberación mínima y finaliza con código de error o señal.
3. Transiciona a `FlushingState`:
   - Mide el tiempo de vaciado con `std::chrono::steady_clock`.
   - **Invariante de Incógnito**: Si `config_.isIncognito` es verdadero, **omite taxativamente** la invocación de `sessionSaver`, establece `report.sessionSaved = false` y no escribe ningún archivo en disco.
   - En sesión ordinaria: invoca `sessionSaver()` que serializa las pestañas atómicamente a través de `vigilio::saveSession`.
   - Si la duración de `sessionSaver` o de las operaciones de disco supera `config_.shutdownTimeoutMs` (3000 ms), aborta inmediatamente transicionando a `EmergencyAbort` con `report.timeoutTriggered = true`.
4. Transiciona a `TerminatingSubsystems`:
   - Invoca `dbCloser()` para cerrar ordenadamente las conexiones de `LocalRepository` y consolidar el WAL de SQLite.
5. Transiciona a `Terminated`:
   - Consolida el reporte inmutable `LifetimeReport`.
   - Retorna con exit code 0 (salvo en caso de aborto o señal forzada).

---

## Compatibility, migration and rollback

### Compatibilidad de compilación y runtime
- **C++20 portable**: Implementación 100% estándar basada en `<atomic>`, `<chrono>`, `<string_view>`, `<functional>` y `<filesystem>`, compatible con Clang (>= 16), GCC (>= 12) y MSVC (>= 19.34).
- **Independencia de WebEngine**: El núcleo de `vigilio_core` no incluye cabeceras de WebEngine ni depende de la disponibilidad de Chromium, permitiendo ejecutar suites completas de pruebas unitarias en servidores headless de CI sin GPU ni display.
- **Soporte multiplataforma (Q-004)**: Compatible con Linux (x64), Windows (x64) y macOS (x64/arm64), utilizando primitivas nativas de sistema de archivos seguras en cada SO.

### Compatibilidad con versiones previas y `SessionFile`
- El formato de serialización de pestañas se mantiene idéntico al estándar canónico `VIGILIO_SESSION_V1` definido en `apps/vigilio-browser/src/core/SessionFile.h`, garantizando interoperabilidad bidireccional sin migraciones de esquema de disco.
- La configuración existente en `QSettings` se sincroniza de forma transparente sin alterar claves de usuario existentes.

### Plan de rollback y mitigación de contingencias
- Si se detecta una degradación inesperada en la velocidad de apagado o un fallo de aserción en las transiciones de estado, la aplicación puede operar de forma segura revirtiendo al cierre estándar de `QMainWindow::closeEvent` sin comprometer la integridad de la base de datos de historial ni filtrar datos de incógnito.
