# Change Proposal — module-foundations-task0029

Pack: `module.foundations.task0029`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0029 — Definir el comportamiento y las invariantes: crash recovery policy`).

Habiendo implementado y certificado en las capacidades precedentes la compilación y warnings (`task0005`-`task0008`), la validación de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión de recursos QRC (`task0017`-`task0020`), el arranque bootstrap completo (`task0021`-`task0024`), y el ciclo de vida y apagado ordenado de la aplicación (`task0025`-`task0028`), esta propuesta formaliza el **modelo de dominio, máquina de estados, tipos de valor inmutables, detección de caídas anómalas (unclean termination), protección contra bucles de caídas repetidas (crash loop protection), protocolo de archivo centinela atómico, reglas de decisión de recuperación, saneamiento riguroso de URLs y garantías constitucionales de privacidad para la política de recuperación tras fallos (Crash Recovery Policy)**.

El alcance concreto comprende:
- **Taxonomía formal de estados de detección de caídas (`CrashDetectionState`)**:
  - `NoPreviousSession`: Primera ejecución del navegador en el perfil o perfil limpio sin rastro previo; se arranca en frío.
  - `CleanExit`: La sesión anterior cerró ordenadamente (`cleanExit == true` en centinela); permite restauración transparente o inicio limpio.
  - `AbnormalTermination`: La sesión previa finalizó de manera abrupta (crash por señal fatal `SIGSEGV`/`SIGABRT`, corte de energía, muerte por OOM o terminación forzada por el usuario/SO).
  - `RepeatedCrashes`: Detección de bucle de caídas consecutivas ($\ge 2$ fallos seguidos sin sesión estable intercalada); requiere intervención protectora.
  - `CorruptedSession`: El archivo de sesión o el centinela están truncados, malformados o contienen bytes inválidos.
  - `Recovered`: Estado terminal de la evaluación tras haber aplicado la política (restaurando pestañas o iniciando en limpio).
- **Acciones resolutivas tipadas de recuperación (`RecoveryAction`)**:
  - `StartClean`: Iniciar con página de inicio limpia (descartando sesión previa o por decisión del usuario).
  - `RestoreSession`: Restaurar automáticamente las pestañas guardadas (comportamiento estándar o tras salida limpia).
  - `PromptRestore`: Solicitar al usuario mediante un aviso no bloqueante (banner accesible) si desea restaurar sus pestañas o empezar de cero.
  - `SafeModeRecovery`: Aconsejar o activar el inicio en Modo Seguro (`--safe-mode`) debido a caídas consecutivas repetidas, desactivando configuraciones experimentales.
  - `DiscardCorrupted`: Archivar de forma segura el archivo de sesión dañado (`.corrupt.bak`) y reiniciar con perfil limpio sin fallos.
- **Tipos de valor inmutables y estructuras de dominio (`CrashRecoveryTypes.h`)**:
  - `CrashDetectionState`: Enumeración fuertemente tipada con los 6 estados de detección.
  - `RecoveryAction`: Enumeración con las 5 acciones de recuperación de sesión.
  - `CrashReason`: Enumeración con 6 causas tipadas de terminación (`Unknown`, `ProcessCrash`, `WatchdogTimeout`, `MemoryExhaustion`, `StorageFailure`, `CleanShutdown`).
  - `CrashSentinel`: Estructura serializable que registra el estado de la sesión activa (`schemaVersion`, `cleanExit`, `consecutiveCrashes`, `pid`, `sessionStartTime`, `lastHeartbeatTime`, `lastCrashReason`, `appVersion`).
  - `CrashRecoveryConfig`: Estructura inmutable con parámetros de configuración: modo incógnito, modo seguro, umbral de bucle de caídas (`crashLoopThreshold = 2`), intervalo de latido (`heartbeatIntervalMs = 30000`), rutas canónicas al centinela y archivo de sesión.
  - `CrashRecoveryReport`: Reporte inmutable consolidado que detalla el estado detectado, acción recomendada, conteo de caídas consecutivas, lista de URLs saneadas recuperables, banderas de advertencia de modo seguro y mensaje de diagnóstico.
- **Coordinador desacoplado de dominio (`CrashRecoveryCoordinator`)**:
  - Componente de lógica pura en C++20 ubicado en `vigilio_core`, sin dependencias directas de Qt Widgets, `QApplication` ni `QWebEngineView`.
  - Inspección determinista y de alta velocidad del sistema de archivos al inicio (`inspectSessionState`).
  - Marcado atómico de sesión activa (`markSessionActive`) y cierre limpio (`markSessionClean`) mediante reemplazo atómico (`.tmp` y renombrado atómico en Windows y POSIX).
  - Actualización periódica de latido (`updateHeartbeat`).
  - Saneamiento y validación estricta de URLs (`sanitizeRecoverableUrls`), restringiendo esquemas a `http` y `https` (o esquemas internos aprobados `vigilio://`), con longitud máxima de 8192 caracteres y tope de 1000 pestañas.
  - Archivo seguro de sesiones corruptas (`archiveCorruptedSession`) a `.corrupt.bak` para evitar reincidencia de crashes y permitir análisis local.
- **Invariantes constitucionales de privacidad y robustez**:
  - *Aislamiento estricto de Incógnito (Decisión Q-001)*: Cuando `config.isIncognito == true`, la política de recuperación queda completamente inactiva: jamás se crea ni lee archivo centinela ni archivo de sesión en disco, devolviendo siempre `NoPreviousSession` y `StartClean`.
  - *Local-First Constitucional (Q-001)*: Cero envío de reportes de caída (crash reports, minidumps, backtraces o URLs) a servidores remotos o nube. Toda la información diagnóstica es estrictamente local en el equipo.
  - *Prevención de bucles de bloqueo (Zero-Zombie / Anti-Crash Loop)*: Si el usuario experimenta caídas consecutivas, el sistema no reintenta ciegamente abrir las mismas pestañas causantes del fallo; escala a `SafeModeRecovery` o inicio limpio.
  - *Resiliencia de E/S atómica*: Toda mutación de estado utiliza sustitución atómica mediante archivos temporales `.tmp` y renombrado seguro del sistema de archivos (`std::filesystem::rename` en POSIX y `MoveFileExW` con `MOVEFILE_REPLACE_EXISTING` en Windows).

## Out of scope

- Implementación del código fuente C++20 en `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`, `CrashRecoveryCoordinator.h` y `CrashRecoveryCoordinator.cpp` (reservado para la etapa `implementation` en `module.foundations.task0030`).
- Construcción e integración del banner visual Qt Widgets accesible (`vigilio-crash-recovery-banner`), diálogos de confirmación y atajos de teclado en `BrowserWindow` (reservado para la etapa `ui` en `module.foundations.task0031`).
- Implementación de la suite completa de pruebas unitarias y de estrés en `tests/unit/CrashRecoveryTests.cpp` (reservado para la etapa `tests` en `module.foundations.task0032`).
- Modificación de cualquier archivo bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE, la cual está estrictamente restringida a `openspec/changes/module-foundations-task0029/**`.
- Recolección de volcados de memoria pesados (core dumps de Chromium o minidumps binarios de WebEngine), los cuales son administrados por el sistema operativo.

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia y la suite de pruebas:
1. **Coordinador de ciclo de vida existente (`apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h` y `.cpp`)**:
   - Implementado y probado en `task0026`-`task0028`. Dispone de `LifetimeReport` con el método `isCleanExit()` que verifica `finalState == AppLifetimeState::Terminated && exitCode == 0 && !timeoutTriggered`.
   - Sin embargo, esta información solo existía en memoria durante la ejecución del proceso; no se persistía en un archivo centinela de estado durable para que la subsiguiente invocación del navegador pudiera evaluar si la sesión previa terminó limpiamente o cayó por crash.
2. **Persistencia atómica de sesión (`apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`)**:
   - Implementa `saveSession` y `loadSession` con validación estricta de esquemas, límite de 1000 pestañas, longitud máxima de 8192 caracteres por URL, y reemplazo atómico en disco mediante archivo temporal (`.tmp`) y renombrado atómico (`MoveFileExW` / `std::filesystem::rename`).
   - `BrowserWindow` actualmente invoca `restoreSession()` de forma ciega en el arranque si no se pasan URLs por línea de comandos; si una pestaña causó un crash fatal de renderizado o memoria, el navegador vuelve a abrirla inmediatamente al arrancar, creando un bucle infinito de caídas.
3. **Coordinador de arranque (`apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` y `.cpp`)**:
   - Evalúa `StartupCommandLine` y ejecuta la inicialización de entorno, dependencias y almacenamiento, soportando modos `StandardGui`, `Headless`, `SafeMode` e `Incognito`.
   - La capacidad de `crash recovery policy` se acoplará de forma natural entre el bootstrap exitoso y la apertura de pestañas en `BrowserWindow`.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con C++20 y `-Werror` bajo CMake y Ninja.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: 10/10 pruebas pasadas con 100% de éxito en 0.54s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.06s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.09s)
     - `resource_qrc_catalog_tests`: Passed (0.03s)
     - `resource_inclusion_service_tests`: Passed (0.07s)
     - `resource_qrc_inclusion_tests`: Passed (0.06s)
     - `startup_bootstrap_tests`: Passed (0.07s)
     - `application_lifetime_tests`: Passed (0.14s)
5. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0085-module.foundations.task0029.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 29/29 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma que las decisiones Q-001 a Q-006 están resueltas (la única pendiente Q-007 solo bloquea `release.public-release`).
6. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal con pestañas y buscador.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de tecnicismos como «crash dump», «SIGSEGV», «watchdog abort» o «Senior Mode» en la interfaz visible. Obligación de usar «Interfaz más fácil de usar».

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0029`.
- **Capacidad**: `crash recovery policy`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0029/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0029/proposal.md`
  - `openspec/changes/module-foundations-task0029/design.md`
  - `openspec/changes/module-foundations-task0029/tasks.md`
  - `openspec/changes/module-foundations-task0029/execution-memory.md`
- **Límites de modificación**: Exclusivamente `openspec/changes/module-foundations-task0029/**`. Ningún archivo de producción en `apps/**` o de reglas en `docs/rules/**` es modificado en esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la capacidad **crash recovery policy** en su etapa **domain** define la arquitectura del subsistema de detección y recuperación dentro del espacio de nombres `vigilio`:

### 1. Modelo conceptual y ciclo de detección
El subsistema opera mediante un archivo centinela de estado (`sentinel.json`) ubicado en el directorio de perfil del usuario, gestionado con sincronización transaccional:
```
           [ Inicio de la aplicación (StartupBootstrap exitoso) ]
                                    │
                                    ▼
                 [ CrashRecoveryCoordinator::inspectSessionState ]
                                    │
            ┌───────────────────────┼────────────────────────┐
            │                       │                        │
  [ config.isIncognito ]     [ Sin centinela previo ]    [ Centinela presente ]
            │                       │                        │
            ▼                       ▼                        ├─ cleanExit == true ──► [ CleanExit ] ──► Restore / Clean
     [ StartClean ]           [ NoPrevious ]                 │
  (cero escrituras)                 │                        └─ cleanExit == false ─► [ AbnormalTermination ]
                                    ▼                                                     │
                             [ StartClean ]                                 ┌─────────────┴─────────────┐
                                                                            │                           │
                                                            consecutiveCrashes < threshold   consecutiveCrashes >= threshold
                                                                            │                           │
                                                                            ▼                           ▼
                                                                     [ PromptRestore ]         [ RepeatedCrashes ]
                                                                 (Aviso no bloqueante)         (Aviso Modo Seguro)
```

### 2. Formato del archivo centinela (`sentinel.json`)
El centinela almacena un JSON canónico compacto (menor a 512 bytes):
```json
{
  "schema_version": 1,
  "clean_exit": false,
  "consecutive_crashes": 1,
  "pid": 12345,
  "session_start_time": 1729240000,
  "last_heartbeat_time": 1729240030,
  "last_crash_reason": 0,
  "app_version": "0.1.0-alpha"
}
```
- Al pasar a `Running`: `clean_exit` se escribe como `false`.
- Al apagar ordenadamente (`Terminated` en `executeShutdown`): `clean_exit` se escribe como `true` y `consecutive_crashes` se resetea a 0.
- Si el proceso muere por crash, corte de luz o `kill`: el centinela permanece con `clean_exit: false`.

### 3. Matriz de decisión de recuperación
| Estado previo | `isIncognito` | Centinela previo | Caídas consecutivas | Estado resultante | Acción recomendada |
| --- | --- | --- | --- | --- | --- |
| Cualquier estado | `true` | N/A (no se lee/escribe) | 0 | `NoPreviousSession` | `StartClean` |
| Arranque inicial | `false` | Inexistente | 0 | `NoPreviousSession` | `StartClean` |
| Arranque | `false` | `clean_exit: true` | 0 | `CleanExit` | `RestoreSession` |
| Arranque tras crash | `false` | `clean_exit: false` | 1 | `AbnormalTermination` | `PromptRestore` |
| Arranque tras crash loop | `false` | `clean_exit: false` | $\ge 2$ | `RepeatedCrashes` | `SafeModeRecovery` |
| Arranque con sesión corrupta | `false` | JSON roto / sesión truncada | N/A | `CorruptedSession` | `DiscardCorrupted` |

### 4. Saneamiento estricto de URLs (`sanitizeRecoverableUrls`)
Para evitar que una URL malformada o un exploit provoque caídas cíclicas al restaurar pestañas:
- Se evalúa cada URL con `vigilio::resolveInput`.
- Se admiten exclusivamente esquemas `http` y `https` (y esquemas locales `vigilio://`).
- Se descartan esquemas `javascript:`, `data:`, `file:` o cadenas vacías.
- Longitud máxima por URL: 8192 caracteres (descartando URLs gigantescas que agoten memoria).
- Número máximo de pestañas: 1000.
- Se eliminan duplicados inmediatos o entradas corruptas.

### 5. Estructuras de dominio C++20 (`CrashRecoveryTypes.h`)
```cpp
namespace vigilio {

enum class CrashDetectionState : uint8_t {
    NoPreviousSession = 0,
    CleanExit = 1,
    AbnormalTermination = 2,
    RepeatedCrashes = 3,
    CorruptedSession = 4,
    Recovered = 5
};

enum class RecoveryAction : uint8_t {
    StartClean = 0,
    RestoreSession = 1,
    PromptRestore = 2,
    SafeModeRecovery = 3,
    DiscardCorrupted = 4
};

enum class CrashReason : uint8_t {
    Unknown = 0,
    ProcessCrash = 1,
    WatchdogTimeout = 2,
    MemoryExhaustion = 3,
    StorageFailure = 4,
    CleanShutdown = 5
};

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

struct CrashRecoveryConfig {
    bool isIncognito = false;
    bool isSafeMode = false;
    uint32_t crashLoopThreshold = 2;
    uint32_t sessionAutosaveIntervalMs = 30000;
    std::string sentinelFilePath;
    std::string sessionFilePath;
    std::string profileDirectory;
};

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
};

} // namespace vigilio
```

### 6. Interfaz del coordinador de dominio (`CrashRecoveryCoordinator.h`)
```cpp
namespace vigilio {

class CrashRecoveryCoordinator {
public:
    [[nodiscard]] static CrashRecoveryReport inspectSessionState(
        const CrashRecoveryConfig& config) noexcept;

    [[nodiscard]] static bool markSessionActive(
        const CrashRecoveryConfig& config,
        uint64_t pid,
        int64_t timestamp) noexcept;

    [[nodiscard]] static bool markSessionClean(
        const CrashRecoveryConfig& config) noexcept;

    [[nodiscard]] static bool updateHeartbeat(
        const CrashRecoveryConfig& config,
        int64_t timestamp) noexcept;

    [[nodiscard]] static std::vector<std::string> sanitizeRecoverableUrls(
        const std::vector<std::string>& rawUrls) noexcept;

    [[nodiscard]] static bool archiveCorruptedSession(
        const std::filesystem::path& sessionPath) noexcept;

    [[nodiscard]] static constexpr std::string_view stateToString(
        CrashDetectionState state) noexcept;

    [[nodiscard]] static constexpr std::string_view actionToString(
        RecoveryAction action) noexcept;

    [[nodiscard]] static constexpr std::string_view reasonToString(
        CrashReason reason) noexcept;
};

} // namespace vigilio
```

## Archivos afectados

### Durante esta fase PROPOSE (únicos archivos modificados):
- `openspec/changes/module-foundations-task0029/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0029/proposal.md`
- `openspec/changes/module-foundations-task0029/design.md`
- `openspec/changes/module-foundations-task0029/tasks.md`
- `openspec/changes/module-foundations-task0029/execution-memory.md`

### En las fases APPLY subsiguientes:
- **`module.foundations.task0030` (Implementation)**:
  - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Definición de tipos de datos, enumeraciones y estructuras inmutables.
  - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`: Declaración de la clase coordinadora estática de dominio.
  - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`: Implementación de inspección de centinela, saneamiento de URLs, reemplazo atómico y archivo de sesiones dañadas.
  - `apps/vigilio-browser/CMakeLists.txt`: Incorporación de los archivos en la biblioteca `vigilio_core`.
- **`module.foundations.task0031` (UI)**:
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Integración del banner accesible de recuperación `vigilio-crash-recovery-banner`, adaptado a Senior Mode («Interfaz más fácil de usar») y textos públicos.
- **`module.foundations.task0032` (Tests)**:
  - `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`: Suite de pruebas unitarias y de resiliencia automatizadas en CTest.

## Contrato UX/UI

- **Sin cambio visual directo en la etapa domain**:
  - La presente etapa `domain` modela exclusivamente las estructuras de datos, detección en disco, saneamiento de URLs y reglas de decisión en `vigilio_core`.
  - No se modifica ningún layout visual, ventana o componente gráfico en esta microtarea.
- **Preparación contractual para la etapa UI (`task0031`)**:
  - Cuando se conecte a Qt Widgets en `module.foundations.task0031`:
    - En caso de `RecoveryAction::PromptRestore`:
      - Se mostrará un banner superior accesible (`vigilio-crash-recovery-banner`), insertado en el layout central sobre las pestañas, sin bloquear modalmente la ventana.
      - **Texto público conforme a `docs/UX-COPY-PUBLICO.md`**:
        - Título: «Vigilio Browser».
        - Mensaje: «La sesión anterior se cerró de forma inesperada. ¿Quieres restaurar tus pestañas abiertas?».
        - Botón primario: «Restaurar sesión» (abre las pestañas en estado diferido / lazy loading para no saturar memoria).
        - Botón secundario: «Iniciar sesión en blanco» (descarta el aviso y limpia el archivo de sesión anterior).
        - Botón de cierre: «X» o atajo `Escape`.
    - En caso de `RecoveryAction::SafeModeRecovery` (Crash Loop):
      - Mensaje: «El navegador se ha cerrado varias veces seguidas. Te sugerimos continuar en una sesión limpia para solucionar la dificultad.».
      - Botones: «Iniciar sesión en blanco» (recomendado) y «Reintentar restauración».
    - **Adaptación a «Interfaz más fácil de usar» (Modo Senior)**:
      - Botones con altura mínima de 48px y anchura mínima de 160px.
      - Tipografía Inter/sistema escalada a 15px con contraste WCAG AAA.
      - Foco accesible inicial en el botón primario.
      - Soporte de navegación por teclado y etiquetas completas para lectores de pantalla (`setAccessibleName`, `setAccessibleDescription`).
      - Cero tecnicismos visibles: prohibidas palabras como «crash recovery policy», «SIGSEGV», «core dump», «watchdog abort», «Senior Mode», «OpenSpec».

## Reglas y fallos

| Escenario de fallo / condición | Causa raíz | Comportamiento del coordinador | Resultado observable |
| --- | --- | --- | --- |
| Sesión en incógnito (`isIncognito == true`) | Privacidad estricta | Ignora centinela y archivo de sesión en disco | `NoPreviousSession` → `StartClean`, cero escrituras en disco |
| Primera ejecución en perfil nuevo | Centinela no existe | Identifica perfil limpio | `NoPreviousSession` → `StartClean` |
| Salida previa limpia | Centinela con `clean_exit == true` | Sesión finalizada ordenadamente | `CleanExit` → `RestoreSession` (o `StartClean` si no había pestañas) |
| Cierre por crash / corte de energía | Centinela con `clean_exit == false` | Detecta cierre anormal, incrementa conteo | `AbnormalTermination` → `PromptRestore`, URLs saneadas listas |
| Bucle de caídas consecutivas ($\ge 2$) | Pestaña maliciosa o fallo persistente | Supera `crashLoopThreshold` | `RepeatedCrashes` → `SafeModeRecovery`, `isSafeModeAdvised == true` |
| Centinela con JSON corrupto o truncado | Corte de energía durante escritura | Atrapa error de parseo, renombra a `.corrupt.bak` | Trata como caída anormal no fatal, previene crashes de parseo |
| Archivo de sesión `session.json` corrupto | Corrupción en cabecera `VIGILIO_SESSION_V1` | `loadSession` devuelve vector vacío | `CorruptedSession` → `DiscardCorrupted`, inicia limpio |
| URL con esquema malicioso (`javascript:`) | Intento de inyección | `sanitizeRecoverableUrls` descarta la entrada | URL eliminada de la lista de restauración |
| URL gigantesca (> 8192 bytes) | Desbordamiento de memoria | `sanitizeRecoverableUrls` descarta la entrada | Se omite la URL protegiendo la estabilidad del proceso |
| Más de 1000 pestañas en sesión | Saturación de recursos | Limita la lista a las primeras 1000 pestañas | Previene agotamiento de memoria y file descriptors |
| Fallo de permisos de escritura en disco | Carpeta de perfil bloqueada o solo lectura | Captura `std::error_code` en renombrado atómico | Retorna `false` limpiamente sin lanzar excepciones al hilo principal |

## Seguridad y privacidad

- **Filosofía Local-First Constitucional (Decisión Q-001)**:
  - Todo el ciclo de detección y recuperación es 100% local.
  - Queda terminantemente prohibido enviar reportes de fallo, trazas de pila (stack traces), minidumps o listas de URLs a servidores externos, nube o servicios de analítica.
- **Invariante Constitucional de Incógnito**:
  - Si `config.isIncognito == true`, `CrashRecoveryCoordinator` garantiza que ni el archivo centinela (`sentinel.json`) ni el archivo de sesión (`session.json`) se leen ni se escriben en disco.
  - La navegación privada no deja ningún rastro forense recuperable tras la salida o crash del proceso.
- **Prevención de bucles de denegación de servicio (Anti-Crash Loop)**:
  - Si una página web maliciosa o con código WebAssembly dañado provoca el cierre forzado del motor de renderizado, la protección contra bucles de caídas impide que el usuario quede atrapado en un ciclo infinito de arranque-caída, ofreciendo arranque limpio o modo seguro.
- **Saneamiento estricto de URLs recuperables**:
  - No se ejecutan scripts ni se interpretan cadenas crudas; solo esquemas navegables seguros (`http`, `https`, `vigilio://`).
- **Resiliencia en operaciones de archivo**:
  - Sustitución atómica mediante archivos temporales `.tmp` y renombrado atómico (`MoveFileExW` / `std::filesystem::rename`) previene archivos dañados ante pérdidas de alimentación eléctrica.

## Matriz de pruebas

| ID Prueba | Tipo | Escenario / Entrada | Comportamiento esperado | Criterio de verificación / Comando |
| --- | --- | --- | --- | --- |
| `test_crash_recovery_no_previous_session` | Unitario | Perfil limpio sin centinela ni sesión | Estado `NoPreviousSession`, acción `StartClean`, 0 caídas | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_clean_exit` | Unitario | Centinela con `clean_exit: true` | Estado `CleanExit`, acción `RestoreSession`, 0 caídas | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_abnormal_termination` | Unitario | Centinela con `clean_exit: false`, 0 caídas previas | Estado `AbnormalTermination`, acción `PromptRestore`, conteo = 1 | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_repeated_crashes_loop` | Resiliencia | Centinela con `clean_exit: false`, conteo previo = 1 | Estado `RepeatedCrashes`, acción `SafeModeRecovery`, `isSafeModeAdvised == true` | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_incognito_zero_io` | Seguridad | `config.isIncognito = true` con centinela previo en disco | Retorna `StartClean`, no lee ni modifica archivos en disco | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_corrupted_sentinel` | Resiliencia | Centinela con JSON truncado o basura binaria | Captura error, genera backup `.corrupt.bak`, no crashea | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_corrupted_session_file` | Resiliencia | Archivo `session.json` sin cabecera `VIGILIO_SESSION_V1` | Detecta corrupción, acción `DiscardCorrupted`, URLs vacías | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_url_sanitization` | Seguridad | Lista con URLs válidas, `javascript:`, vacías y > 8192 bytes | Filtra inválidas, conserva solo `http`/`https` válidas $\le 8192$ chars | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_mark_active_and_clean` | Integración | Secuencia de vida: arranque (`markActive`) y apagado (`markClean`) | Centinela pasa de `clean_exit: false` a `clean_exit: true` con I/O atómico | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_heartbeat_update` | Unitario | Actualización de timestamp de latido durante runtime | Modifica `last_heartbeat_time` sin alterar `clean_exit: false` | `ctest -R crash_recovery_tests` |
| `test_crash_recovery_string_conversions` | Unitario | Todas las enumeraciones de estados, acciones y razones | Conversión canónica a string sin cadenas vacías ni excepciones | `ctest -R crash_recovery_tests` |

## Pasos APPLY

Para la fase de implementación incremental (`module.foundations.task0030`), el orden determinista de ejecución será:
1. Crear el archivo de encabezado `apps/vigilio-browser/src/core/CrashRecoveryTypes.h` definiendo `CrashDetectionState`, `RecoveryAction`, `CrashReason`, `CrashSentinel`, `CrashRecoveryConfig` y `CrashRecoveryReport`.
2. Crear el archivo de encabezado `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` declarando la clase coordinadora estática de dominio, sus métodos de inspección, saneamiento, marcado de centinela y archivo de sesiones dañadas.
3. Crear el archivo de implementación `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp` implementando la lógica de parseo, reemplazo atómico de centinela, detección de crash loop y filtrado de URLs.
4. Crear la suite de pruebas unitarias `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp` cubriendo exhaustivamente los 11 escenarios de la matriz de pruebas.
5. Actualizar `apps/vigilio-browser/CMakeLists.txt` incorporando `CrashRecoveryCoordinator.cpp` a la biblioteca estática `vigilio_core`, registrando el ejecutable `vigilio_crash_recovery_tests` y la prueba en CTest `crash_recovery_tests` con etiquetas `LABELS "unit;fast"`.
6. Ejecutar `bash apps/vigilio-browser/scripts/check.sh` verificando compilación limpia bajo C++20 con `-Werror`, cero advertencias de Clang-Tidy, formato impecable y 100% de pruebas pasando en CTest.
7. Sincronizar la documentación canónica en `docs/rules/01-modules/foundations/micro/0029-crash-recovery-policy/task.md`.

## Definición de terminado

La microtarea de propuesta para `crash recovery policy` se considerará completamente terminada cuando:
- Los documentos de OpenSpec (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`) estén redactados con especificación técnica exhaustiva y coherente, sin marcadores provisionales ni campos pendientes.
- El modelo de dominio de `crash recovery policy` formalice la máquina de estados con 6 estados de detección, 5 acciones de recuperación, protección contra bucles de caídas, reemplazo atómico de centinela, saneamiento de URLs y política estricta de incógnito.
- La herramienta canónica `validate-references` confirme que todas las referencias canónicas son válidas.
- La herramienta canónica `verify-lock` certifique la integridad estricta del archivo lock `0085-module.foundations.task0029.propose.lock.json`.
- El reporte de automatización declare el estado `READY` conforme a los lineamientos del orquestador.

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0028`: Completada y certificada exitosamente (10/10 tests pasando al 100% en CTest).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Bucle infinito de caídas si una pestaña con contenido malicioso o no soportado por WebEngine se restaura automáticamente tras un crash.
    *Mitigación*: Detección de bucle de caídas (`RepeatedCrashes`) ante $\ge 2$ caídas consecutivas, forzando la recomendación de inicio en Modo Seguro o sesión en blanco antes de restaurar pestañas.
  - *Riesgo*: Corrupción del archivo centinela si el equipo sufre un corte de energía mientras se actualiza el estado.
    *Mitigación*: Escritura siempre en archivo temporal `.tmp` con sustitución atómica del sistema de archivos (`std::filesystem::rename` / `MoveFileExW`). Si el centinela resulta corrupto, el coordinador lo aisla como `.corrupt.bak` y procede con degradación segura sin crashear.
  - *Riesgo*: Violación de privacidad en modo incógnito si se guardan rastros de pestañas tras una caída inesperada.
    *Mitigación*: Invariante constitucional de incógnito (Q-001): cuando `isIncognito == true`, el coordinador omite cualquier operación de centinela o sesión en disco.
  - *Riesgo*: Retardos de E/S en disco durante el arranque de la aplicación.
    *Mitigación*: El archivo centinela es un documento JSON mínimo (< 512 bytes), cuya lectura y parseo toma menos de 0.5 ms, garantizando un arranque instantáneo.

## Commit propuesto

`docs(foundations): domain crash-recovery-policy`
