# Change Proposal — module-foundations-task0025

Pack: `module.foundations.task0025`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **application lifetime** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0025 — Definir el comportamiento y las invariantes: application lifetime`).

Habiendo formalizado, implementado y certificado en las capacidades precedentes la compilación y warnings (`task0005`-`task0008`), la validación de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión del catálogo canónico de recursos QRC (`task0017`-`task0020`), y la secuencia completa de arranque bootstrap con soporte headless e incógnito (`task0021`-`task0024`), esta propuesta formaliza el **modelo de dominio, máquina de estados, tipos de valor inmutables, políticas de apagado ordenado (graceful shutdown), gestión segura de señales del sistema operativo (SIGINT, SIGTERM), vaciado atómico de estado y garantías constitucionales de privacidad para el ciclo de vida completo de la aplicación (Application Lifetime)**.

El alcance concreto comprende:
- **Taxonomía formal de estados del ciclo de vida (`AppLifetimeState`)**:
  - `Uninitialized`: Estado inicial antes de la instanciación de subsistemas o registro de manejadores de ciclo de vida.
  - `Starting`: Fase de arranque coordinada por `StartupBootstrapCoordinator` tras validación exitosa de entorno, dependencias y almacenamiento.
  - `Running`: Bucle de eventos de interfaz de usuario activo (`QApplication::exec()`), ventanas interactivas visibles, navegación web operativa y recepción normal de eventos.
  - `Suspending`: Estado transitorio de reposo o segundo plano (minimizada o suspensión de energía) con optimización de consumo de memoria y CPU.
  - `ShutdownRequested`: Recepción y validación de una solicitud de cierre originada por interacción del usuario (`[X]`, `Ctrl+Q`), señal del sistema (`SIGINT`, `SIGTERM`), gestor de sesión del SO o error crítico irrecuperable.
  - `FlushingState`: Vaciado secuencial y atómico de datos volátiles a almacenamiento persistente local (guardado atómico de pestañas en `SessionFile`, sincronización de preferencias en disco y confirmación de transacciones en `LocalRepository`), aplicando estrictamente la excepción de incógnito.
  - `TerminatingSubsystems`: Desmantelamiento y liberación determinista de subsistemas en orden inverso a su inicialización (páginas WebEngine, vistas de pestañas, repositorios SQLite, logs locales y sockets auxiliares).
  - `Terminated`: Bucle de eventos finalizado, recursos liberados, reporte de diagnóstico de ciclo de vida completado y código de salida final establecido.
  - `EmergencyAbort`: Ruta de salida de emergencia ante fallos irrecuperables, corrupción inminente o agotamiento del temporizador de seguridad (watchdog timeout), garantizando la política Zero-Zombie.
- **Tipos de valor inmutables y estructuras de dominio (`LifetimeTypes.h`)**:
  - `AppLifetimeState`: Enumeración fuertemente tipada (`enum class`) con los 9 estados discretos del ciclo de vida.
  - `ShutdownReason`: Enumeración con las 8 causas de terminación tipadas (`UserWindowClose`, `UserExplicitQuit`, `SignalInterrupt`, `SignalTerminate`, `SessionLogout`, `StorageFailure`, `MemoryExhaustion`, `EmergencyAbort`).
  - `ShutdownAction`: Enumeración con las 4 acciones de resolución de apagado (`ProceedGraceful`, `ProceedImmediate`, `PromptUser`, `IgnoreRequest`).
  - `LifetimeConfig`: Estructura inmutable con los parámetros de configuración de ciclo de vida: modo incógnito, modo seguro, tiempo límite de apagado (`shutdownTimeoutMs`), ruta canónica del archivo de sesión y directorio de perfil.
  - `LifetimeReport`: Reporte inmutable consolidado con el estado final alcanzado, causa de apagado, código de salida del proceso, confirmación de guardado de sesión, estado de cierre de base de datos, tiempo transcurrido en microsegundos y mensaje de diagnóstico.
- **Coordinador desacoplado de dominio (`ApplicationLifetimeCoordinator`)**:
  - Componente de lógica pura en C++20 ubicado en `vigilio_core`, sin dependencias directas de `QApplication` ni de widgets de UI.
  - Gestión determinista de la matriz de transiciones válidas (`canTransition`) rechazando transiciones ilegales.
  - Orquestación desacoplada del flujo de apagado (`executeShutdown`) con inyección de funciones de persistencia y temporizador de seguridad de 3000 ms.
  - Funciones de conversión textual canónica `stateToString`, `reasonToString` y `actionToString` para observabilidad y trazabilidad en logs locales.
  - Funciones puras de inspección: `isSignalSafe`, `isGracefulExit` y `canBypassConfirmation`.
- **Invariantes constitucionales de privacidad y robustez**:
  - *Aislamiento estricto de Incógnito*: Cuando `config.isIncognito == true`, `FlushingState` omite taxativamente el guardado del archivo de sesión y cualquier escritura en disco, limpiando buffers volátiles en memoria sin dejar huellas.
  - *Local-First Constitucional (Q-001)*: Cero llamadas de red, sincronización cloud o pings telemétricos al salir de la aplicación.
  - *Persistencia atómica y resiliente*: El guardado de sesión se delega en `vigilio::saveSession` que escribe en `.tmp` y reemplaza atómicamente el archivo final, evitando archivos truncados ante cortes abruptos de energía.
  - *Política Zero-Zombie y temporizador de seguridad*: Límite estricto de 3000 ms para completar la persistencia y desmantelamiento de subsistemas; si algún componente se bloquea, se fuerza la transición a `EmergencyAbort` para evitar procesos colgados.
  - *Idempotencia y deduplicación de señales*: Señales repetidas de terminación no desencadenan múltiples ciclos de persistencia concurrentes; la primera inicia el apagado y las subsiguientes son ignoradas o aceleran el cierre de emergencia.

## Out of scope

- Implementación del código fuente C++20 en `apps/vigilio-browser/src/core/LifetimeTypes.h`, `ApplicationLifetimeCoordinator.h` y `ApplicationLifetimeCoordinator.cpp` (reservado para la etapa `implementation` en `module.foundations.task0026`).
- Integración de señales de Qt Widgets en `BrowserWindow::closeEvent`, diálogos de confirmación de cierre y eventos de gestor de sesión del SO (reservado para la etapa `ui` en `module.foundations.task0027`).
- Implementación de la suite completa de pruebas unitarias y de estrés en `tests/unit/ApplicationLifetimeTests.cpp` (reservado para la etapa `tests` en `module.foundations.task0028`).
- Políticas de recuperación post-crash y reconstrucción de pestañas tras fallos inesperados del proceso (reservado para la capacidad `crash recovery policy`, tareas `task0029`-`task0032`).
- Modificación de cualquier archivo bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE, la cual está estrictamente restringida a `openspec/changes/module-foundations-task0025/**`.

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos de dominio y el pipeline de pruebas:
1. **Punto de entrada principal (`apps/vigilio-browser/src/app.cpp`)**:
   - Actualmente orquesta la inicialización llamando a `StartupBootstrapCoordinator::parseCommandLine` y `executeBootstrap`, pero al arrancar la GUI invoca directamente `return app.exec();` sin registrar un coordinador de ciclo de vida ni capturar señales del sistema operativo (`SIGINT`, `SIGTERM`).
   - El cierre de la aplicación ocurre de forma reactiva cuando Qt cierra la última ventana, sin garantizar un orden determinista de desmantelamiento ni temporizador de apagado.
2. **Ventana principal (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`)**:
   - `BrowserWindow::closeEvent` contiene actualmente:
     ```cpp
     void BrowserWindow::closeEvent(QCloseEvent* event) {
         saveSession();
         QMainWindow::closeEvent(event);
     }
     ```
   - En `saveSession()`, el guardado de URLs se realiza directamente a través de `QSettings().setValue("session/urls", urls)`. Si bien existe `src/core/SessionFile.h` con persistencia atómica en disco (`.tmp` y reemplazo atómico), `BrowserWindow` aún no está desacoplado a través de un coordinador formal de ciclo de vida.
   - Si la aplicación recibe `SIGINT` o `SIGTERM` desde la terminal o un orquestador del SO, `BrowserWindow::closeEvent` no es invocado, perdiéndose el estado de las pestañas o dejando transacciones SQLite abiertas.
3. **Persistencia atómica existente (`apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`)**:
   - Provee las funciones puras `saveSession(const std::filesystem::path& path, const std::vector<std::string>& urls)` y `loadSession(path)`.
   - Limita a 1000 pestañas y URLs de hasta 8192 caracteres con formato canónico `VIGILIO_SESSION_V1`.
   - Implementa escritura atómica en archivo temporal `.tmp` con reemplazo seguro en Windows (`MoveFileExW` con `MOVEFILE_REPLACE_EXISTING`) y POSIX (`std::filesystem::rename`).
4. **Almacenamiento SQLite local (`apps/vigilio-browser/src/storage/LocalRepository.h` y `.cpp`)**:
   - Gestiona marcadores e historial local mediante SQLite en modo local-first. Requiere un cierre ordenado para vaciar el WAL (Write-Ahead Log) y liberar bloqueos de archivo.
5. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia bajo C++20 con `-Werror` bajo CMake y Ninja.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: 9/9 pruebas pasadas con 100% de éxito en 0.34s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.05s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.05s)
     - `resource_qrc_catalog_tests`: Passed (0.03s)
     - `resource_inclusion_service_tests`: Passed (0.08s)
     - `resource_qrc_inclusion_tests`: Passed (0.05s)
     - `startup_bootstrap_tests`: Passed (0.06s)
6. **Verificación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0073-module.foundations.task0025.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 25/25 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Todas las decisiones de foundations resueltas (Q-001 local-first, Q-002 C++20/Qt6.10, Q-004 multiplataforma, Q-006 assets en app). La única decisión abierta es Q-007, que bloquea únicamente `release.public-release`.
7. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal con pestañas y buscador central.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Panel de accesibilidad con tipografía ampliada.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición de tecnicismos internos en diálogos públicos; uso obligado de «Interfaz más fácil de usar».

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0025`.
- **Capacidad**: `application lifetime`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0025/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0025/proposal.md`
  - `openspec/changes/module-foundations-task0025/design.md`
  - `openspec/changes/module-foundations-task0025/tasks.md`
  - `openspec/changes/module-foundations-task0025/execution-memory.md`
- **Límites de modificación**: Exclusivamente `openspec/changes/module-foundations-task0025/**`. Ningún archivo bajo `apps/**` o `docs/rules/**` es modificado en esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la capacidad **application lifetime** en su etapa **domain** define la arquitectura del ciclo de vida de la aplicación dentro del espacio de nombres `vigilio`:

### 1. Máquina de estados determinista (`AppLifetimeState`)
La progresión de estados del ciclo de vida se formaliza como un autómata determinista con transiciones controladas:
```
[Uninitialized]
       │
       ▼
   [Starting]  ──(Fallo irrecuperable de bootstrap)──► [EmergencyAbort]
       │                                                      ▲
       ▼                                                      │
   [Running]   ───────────────────────────────────────────────┤ (Watchdog / Timeout)
    │     ▲                                                   │
    ▼     │                                                   │
 [Suspending]                                                 │
    │                                                         │
    ▼                                                         │
[ShutdownRequested] ──(Cancelado por usuario)──► [Running]    │
    │                                                         │
    ▼                                                         │
[FlushingState] ──────────────────────────────────────────────┤
    │                                                         │
    ▼                                                         │
[TerminatingSubsystems] ──────────────────────────────────────┘
    │
    ▼
[Terminated]
```

### 2. Matriz de transiciones legales (`canTransition`)
| Estado actual | Estados destino permitidos | Condición / Disparador |
| --- | --- | --- |
| `Uninitialized` | `Starting` | Inicialización del coordinador tras creación del proceso. |
| `Starting` | `Running`, `ShutdownRequested`, `EmergencyAbort` | Bootstrap exitoso cede control a event loop; fallo o aborto temprano. |
| `Running` | `Suspending`, `ShutdownRequested`, `EmergencyAbort` | Minimización/ahorro; solicitud de cierre; error fatal en runtime. |
| `Suspending` | `Running`, `ShutdownRequested`, `EmergencyAbort` | Restauración a primer plano; cierre durante suspensión; fallo crítico. |
| `ShutdownRequested` | `FlushingState`, `Running`, `EmergencyAbort` | Aceptación de cierre; cancelación por el usuario; forzado inmediato. |
| `FlushingState` | `TerminatingSubsystems`, `EmergencyAbort` | Finalización exitosa de persistencia; agotamiento del timeout (3000 ms). |
| `TerminatingSubsystems` | `Terminated`, `EmergencyAbort` | Liberación limpia de recursos; error fatal durante desmontaje. |
| `EmergencyAbort` | `Terminated` | Cierre forzado del proceso tras registro mínimo del fallo. |
| `Terminated` | Ninguno | Estado final inmutable; el proceso finaliza su ejecución. |

### 3. Enumeraciones y tipos de valor de ciclo de vida (`LifetimeTypes.h`)
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
    UserWindowClose = 0,    // Usuario cierra la última ventana abierta [X]
    UserExplicitQuit = 1,   // Atajo Ctrl+Q o menú Archivo -> Salir
    SignalInterrupt = 2,    // Señal POSIX SIGINT (Ctrl+C en terminal)
    SignalTerminate = 3,    // Señal POSIX SIGTERM (apagado del sistema o systemd)
    SessionLogout = 4,      // Gestor de sesión del SO solicitando cierre
    StorageFailure = 5,     // Error crítico no recuperable de escritura en disco
    MemoryExhaustion = 6,   // Presupuesto de memoria RAM excedido
    EmergencyAbort = 7      // Aserción fatal o agotamiento de tiempo de apagado
};

enum class ShutdownAction : uint8_t {
    ProceedGraceful = 0,    // Continuar con vaciado ordenado y desmantelamiento
    ProceedImmediate = 1,   // Cierre rápido de emergencia omitiendo esperas
    PromptUser = 2,         // Deferir a confirmación en UI (ej. descargas en curso)
    IgnoreRequest = 3       // Descartar solicitud duplicada o inválida
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

### 4. Interfaz del coordinador de dominio (`ApplicationLifetimeCoordinator.h`)
```cpp
namespace vigilio {

class ApplicationLifetimeCoordinator {
public:
    explicit ApplicationLifetimeCoordinator(LifetimeConfig config) noexcept;

    [[nodiscard]] AppLifetimeState currentState() const noexcept;
    [[nodiscard]] bool isShutdownRequested() const noexcept;
    [[nodiscard]] bool isIncognito() const noexcept;
    [[nodiscard]] const LifetimeConfig& configuration() const noexcept;

    bool transitionTo(AppLifetimeState newState) noexcept;
    ShutdownAction requestShutdown(ShutdownReason reason) noexcept;

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

## Archivos afectados

### Durante esta fase PROPOSE (únicos archivos modificados):
- `openspec/changes/module-foundations-task0025/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0025/proposal.md`
- `openspec/changes/module-foundations-task0025/design.md`
- `openspec/changes/module-foundations-task0025/tasks.md`
- `openspec/changes/module-foundations-task0025/execution-memory.md`

### En la fase APPLY subsiguiente (`module.foundations.task0026`):
- `apps/vigilio-browser/src/core/LifetimeTypes.h`: Definición de enumeraciones y tipos de valor del ciclo de vida.
- `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h`: Declaración de la clase coordinadora de ciclo de vida.
- `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`: Implementación de la máquina de estados, deduplicación de señales y ejecución de apagado.
- `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`: Suite de pruebas unitarias focalizadas para estados de ciclo de vida.
- `apps/vigilio-browser/CMakeLists.txt`: Incorporación de los archivos de ciclo de vida en `vigilio_core` y registro del test en CTest.

## Contrato UX/UI

- **Sin cambio visual directo en la etapa domain**:
  - La presente etapa `domain` modela exclusivamente las estructuras de datos, transiciones de estado, políticas de seguridad y orden de vaciado en `vigilio_core`.
  - No se modifica ningún layout visual, ventana o componente gráfico en esta microtarea.
- **Preparación contractual para la etapa UI (`task0027`)**:
  - Cuando se conecte a Qt Widgets en `module.foundations.task0027`:
    - El evento `BrowserWindow::closeEvent` delegará la decisión en `ApplicationLifetimeCoordinator::requestShutdown`.
    - En caso de requerirse confirmación (por ejemplo, si existen descargas activas en curso), se aplicará `docs/UX-COPY-PUBLICO.md`:
      - Diálogo modal accesible con título: «Vigilio Browser».
      - Mensaje claro: «¿Deseas salir del navegador?».
      - Acciones etiquetadas: «Cerrar navegador» (botón primario) y «Continuar navegando» (botón secundario).
    - Adaptación a «Interfaz más fácil de usar» (Modo Senior):
      - Botones ampliados con altura mínima de 44px.
      - Tipografía Inter o del sistema escalada a 15px/16px con contraste WCAG AAA.
      - Foco inicial predeterminado en el botón primario de confirmación.
      - Cero jerga técnica («graceful teardown», «kill signal», «flush WAL» quedan estrictamente prohibidos).

## Reglas y fallos

| Disparador de entrada | Estado previo | Estado resultante | Acción ejecutada | Persistencia y retorno |
| --- | --- | --- | --- | --- |
| Cierre normal de ventana por el usuario (`[X]`) | `Running` | `ShutdownRequested` → `FlushingState` → `TerminatingSubsystems` → `Terminated` | Guarda sesión atómicamente, cierra base de datos y libera recursos. | `report.sessionSaved = true`, exit code 0. |
| Atajo de salida `Ctrl+Q` | `Running` | `ShutdownRequested` → `FlushingState` → `TerminatingSubsystems` → `Terminated` | Vaciado ordenado de todas las pestañas y cierre de ventana. | `report.sessionSaved = true`, exit code 0. |
| Recepción de señal POSIX `SIGINT` o `SIGTERM` | `Running` | `ShutdownRequested` → `FlushingState` → `TerminatingSubsystems` → `Terminated` | Interrupción controlada; el hilo de señales notifica al bucle principal sin invocar código no seguro. | `report.sessionSaved = true`, exit code 0. |
| Cierre en sesión de Incógnito (`isIncognito == true`) | `Running` | `ShutdownRequested` → `FlushingState` → `TerminatingSubsystems` → `Terminated` | Omite taxativamente la persistencia en disco; limpia buffers en RAM. | `report.sessionSaved = false`, sin tocar disco, exit code 0. |
| Solicitud repetida de cierre mientras ya se está apagando | `FlushingState` | `FlushingState` (sin cambio) | Deduplicación de evento; se ignora la solicitud adicional evitando condiciones de carrera. | Se mantiene el ciclo de apagado original. |
| Señal de emergencia repetida (> 2 veces) | `FlushingState` | `EmergencyAbort` → `Terminated` | Aceleración forzada de terminación si el usuario presiona repetidamente Ctrl+C. | Exit code 130/143 inmediato. |
| Fallo o bloqueo en vaciado de sesión (> 3000 ms) | `FlushingState` | `EmergencyAbort` → `Terminated` | El temporizador de seguridad corta la espera y aborta el proceso de forma limpia. | `report.timeoutTriggered = true`, exit code 1. |
| Error crítico irrecuperable de disco o base de datos | `Running` | `EmergencyAbort` → `Terminated` | Interrupción inmediata sin sobreescribir archivos corruptos. | `report.exitCode = 15` (`StorageAccessDenied`) o `20`. |

## Seguridad y privacidad

- **Filosofía Local-First Constitucional (Decisión Q-001)**:
  - Todo el ciclo de vida y apagado es estrictamente local.
  - Queda prohibida cualquier conexión de red, ping de cierre, telemetría o sincronización cloud al finalizar el proceso.
- **Invariante Constitucional de Incógnito**:
  - Si `config.isIncognito == true`, el coordinador garantiza que `sessionSaver` jamás sea invocado en disco.
  - El archivo `session.json` no se crea, no se modifica ni se persiste en ninguna ruta del sistema de archivos.
  - La memoria volátil se libera limpiamente asegurando que ninguna URL o dato de formulario quede accesible tras el cierre.
- **Persistencia atómica y prevención de corrupción de sesión**:
  - La persistencia utiliza escritura en archivo temporal `.tmp` con reemplazo atómico (`MOVEFILE_REPLACE_EXISTING` en Windows, `rename` en POSIX).
  - Si el proceso sufre un corte de energía o interrupción abrupta durante la escritura, el archivo de sesión anterior permanece intacto, evitando pérdidas de sesión.
- **Política Zero-Zombie y temporizador de seguridad**:
  - El coordinador impone un temporizador de seguridad estricto (`shutdownTimeoutMs = 3000`).
  - Ningún subproceso Chromium auxiliar (`QtWebEngineProcess`) ni hilo en segundo plano quedará huérfano o colgado consumiendo recursos del sistema operativo.
- **Seguridad en manejo de señales del sistema**:
  - Los manejadores de señales `SIGINT` y `SIGTERM` operan únicamente sobre tipos atómicos estándar (`std::atomic<bool>`) o tuberías autocontenidas (self-pipe trick), evitando llamadas no reentrantes a `malloc`, locks de Qt o funciones de rendering dentro del contexto de señal del SO.

## Matriz de pruebas

| ID Prueba | Tipo | Entrada / Fixture | Comportamiento esperado | Criterio de aceptación / Comando |
| --- | --- | --- | --- | --- |
| `test_lifetime_initial_state` | Unitario | Instanciación con configuración estándar | Estado inicial `Uninitialized`, transiciona limpiamente a `Starting` y `Running` | `ctest -R application_lifetime_tests` |
| `test_lifetime_transition_matrix` | Unitario | Evaluación de todas las combinaciones de estados origen y destino | `canTransition` aprueba transiciones legales y rechaza taxativamente ilegales | `ctest -R application_lifetime_tests` |
| `test_lifetime_graceful_shutdown` | Unitario | Solicitud `UserWindowClose` en modo normal | Transición `Running` → `ShutdownRequested` → `FlushingState` → `Terminated`, `sessionSaved == true`, exit code 0 | `ctest -R application_lifetime_tests` |
| `test_lifetime_incognito_zero_trace` | Seguridad / Privacidad | Solicitud de cierre con `isIncognito = true` | `sessionSaver` nunca es invocado, `report.sessionSaved == false`, cero archivos creados en disco | `ctest -R application_lifetime_tests` |
| `test_lifetime_signal_interrupt_handling` | Unitario | Solicitud `SignalInterrupt` (`SIGINT`) | `isSignalSafe == true`, acción `ProceedGraceful`, reporte con `ShutdownReason::SignalInterrupt` | `ctest -R application_lifetime_tests` |
| `test_lifetime_signal_terminate_handling` | Unitario | Solicitud `SignalTerminate` (`SIGTERM`) | Transición ordenada a `Terminated`, base de datos cerrada limpiamente | `ctest -R application_lifetime_tests` |
| `test_lifetime_idempotent_shutdown_requests` | Concurrencia | Múltiples hilos solicitando apagado simultáneo (32 hilos) | Una única ejecución del ciclo de vaciado; solicitudes secundarias ignoradas (`IgnoreRequest`) | `ctest -R application_lifetime_tests` |
| `test_lifetime_repeated_signals_emergency` | Resiliencia | > 2 solicitudes de señal durante `FlushingState` | Aceleración inmediata a `EmergencyAbort`, evitando bloqueos prolongados | `ctest -R application_lifetime_tests` |
| `test_lifetime_timeout_watchdog` | Resiliencia | Inyección de `sessionSaver` con retardo simulado > 3000 ms | Activación de watchdog de seguridad, transición a `EmergencyAbort`, `timeoutTriggered == true` | `ctest -R application_lifetime_tests` |
| `test_lifetime_database_close_cleanly` | Integración | Cierre con callback de base de datos simulado | `report.databaseClosedCleanly == true`, sin fugas de conexiones SQLite | `ctest -R application_lifetime_tests` |
| `test_lifetime_string_conversions` | Unitario | Todas las enumeraciones de estados, causas y acciones | Conversión biunívoca sin cadenas vacías ni excepciones | `ctest -R application_lifetime_tests` |

## Pasos APPLY

Para la fase de implementación incremental (`module.foundations.task0026`), el orden determinista de ejecución será:
1. Crear el archivo de encabezado `apps/vigilio-browser/src/core/LifetimeTypes.h` definiendo `AppLifetimeState`, `ShutdownReason`, `ShutdownAction`, `LifetimeConfig` y `LifetimeReport`.
2. Crear el archivo de encabezado `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h` declarando la clase coordinadora de dominio, la máquina de estados y los métodos de orquestación de apagado.
3. Crear el archivo de implementación `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp` implementando las transiciones legales, el temporizador de apagado de seguridad (3000 ms), la protección estricta de incógnito y las conversiones de texto.
4. Crear la suite de pruebas unitarias `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp` cubriendo exhaustivamente los 11 casos de la matriz de pruebas.
5. Actualizar `apps/vigilio-browser/CMakeLists.txt` agregando `ApplicationLifetimeCoordinator.cpp` a la biblioteca estática `vigilio_core`, registrando el ejecutable `vigilio_lifetime_tests` y la prueba de CTest `application_lifetime_tests` con etiquetas `LABELS "unit;fast"`.
6. Ejecutar `bash apps/vigilio-browser/scripts/check.sh` verificando compilación limpia bajo C++20 con `-Werror`, cero advertencias de Clang-Tidy, formato impecable y 10/10 pruebas pasando al 100%.
7. Actualizar `docs/rules/01-modules/foundations/micro/0025-application-lifetime/task.md` sincronizando su estado de especificación.

## Definición de terminado

La microtarea de propuesta para `application lifetime` se considerará completamente terminada cuando:
- Los documentos de OpenSpec (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`) estén redactados con especificación técnica exhaustiva sin marcadores provisionales ni campos pendientes.
- El modelo de dominio de `application lifetime` formalice la máquina de estados con 9 estados, 8 causas de apagado, políticas de persistencia atómica, manejo seguro de señales y aislamiento de incógnito.
- La herramienta canónica `validate-references` confirme que todas las referencias canónicas son válidas.
- La herramienta canónica `verify-lock` certifique la integridad estricta del archivo lock `0073-module.foundations.task0025.propose.lock.json`.
- El reporte de automatización declare el estado `READY` conforme a los lineamientos del orquestador.

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0024`: Completada y certificada exitosamente (9/9 tests pasando al 100% en CTest).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Bloqueo del proceso al salir debido a operaciones de disco lentas o conexiones no liberadas, dejando procesos colgados (zombies).
    *Mitigación*: Implementación de un temporizador de seguridad de 3000 ms en el coordinador de ciclo de vida que fuerza el paso a `EmergencyAbort` ante retardos excesivos.
  - *Riesgo*: Condiciones de carrera o comportamiento indefinido por ejecución de código no reentrante dentro de manejadores de señales POSIX (`SIGINT`, `SIGTERM`).
    *Mitigación*: El coordinador desacopla la detección de señal del flujo de apagado; los manejadores de señal únicamente activan banderas atómicas estándar (`std::atomic<bool>`), permitiendo que el hilo principal procese el apagado dentro de su flujo determinista.
  - *Riesgo*: Pérdida o corrupción del archivo de sesión si el proceso es interrumpido durante la escritura.
    *Mitigación*: Delegar la persistencia en `vigilio::saveSession` (`SessionFile.h`), garantizando escritura atómica mediante archivos temporales `.tmp` y reemplazo atómico a nivel del sistema de archivos.
  - *Riesgo*: Filtración de datos de navegación en modo incógnito durante la secuencia de apagado.
    *Mitigación*: Invariante estricto en el coordinador: si `config.isIncognito == true`, `sessionSaver` jamás es ejecutado y no se escribe ningún archivo en disco.

## Commit propuesto

`docs(foundations): domain application-lifetime`
