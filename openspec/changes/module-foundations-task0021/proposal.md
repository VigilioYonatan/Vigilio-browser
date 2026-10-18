# Change Proposal — module-foundations-task0021

Pack: `module.foundations.task0021`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **startup bootstrap** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0021 — Definir el comportamiento y las invariantes: startup bootstrap`). 

Habiendo formalizado y certificado en las capacidades precedentes la compatibilidad de compilador y warnings (`task0005`-`task0008`), la validación de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`) y la inclusión determinista del sistema de recursos canónicos QRC (`task0017`-`task0020`), esta propuesta aborda el diseño del modelo de dominio, la máquina de estados, los tipos de valor, las políticas de seguridad y las invariantes que coordinan la secuencia integral de inicio de la aplicación.

El alcance concreto abarca:
- **Taxonomía de fases del ciclo de vida de arranque (`StartupPhase`)**:
  - `Uninitialized`: Estado inicial antes de cualquier evaluación ambiental o de configuración.
  - `EnvironmentInspection`: Detección de variables del sistema operativo (`DISPLAY`, `WAYLAND_DISPLAY`, perfiles locales, variables de testing headless como `VIGILIO_HEADLESS`).
  - `CommandLineParsing`: Parseo sintáctico y semántico desacoplado de argumentos CLI (`--headless`, `--safe-mode`, `--incognito`, `--profile-dir <path>`, `--help`, `--version`, URLs iniciales), con rechazo estricto de banderas no autorizadas o peligrosas.
  - `DependencyVerification`: Orquestación no bloqueante de la compatibilidad de Qt (>= 6.10.0 según decisión Q-002) y Chromium WebEngine (milestone >= 128) reutilizando `QtDependencyValidator` y `WebEngineVersionValidator`.
  - `ResourceVerification`: Verificación previa de integridad del catálogo de 35 recursos canónicos y disponibilidad obligatoria de los 8 recursos críticos mediante `ResourceInclusionService`.
  - `StorageAndProfileResolution`: Resolución del directorio de datos de usuario con `QStandardPaths` / fallback local, validación de permisos de escritura, pre-flight de base de datos SQLite y aislamiento estricto de perfiles efímeros de incógnito.
  - `ApplicationConfiguration`: Configuración de metadatos globales (`organizationName`, `applicationName`, versión `0.1.0-alpha`), patrón de mensajes de logging estructurado y directivas de ahorro de memoria.
  - `Ready`: Transferencia formal de control al bucle de eventos (`QApplication::exec()`) y presentación de la ventana principal (`BrowserWindow`).
  - `DegradedHeadless`: Modo operativo degradado pero seguro cuando falta el servidor gráfico (X11/Wayland) o WebEngine se degrada, permitiendo diagnósticos, herramientas CLI y suites de tests unitarios sin cuelgues ni ventanas.
  - `Aborted`: Terminación ordenada no fallida solicitada por el usuario (e.g. `--help` o `--version`).
  - `Failed`: Terminación por falla irrecuperable en alguna de las fases críticas con código de salida tipado, diagnóstico estructurado y remediación clara.
- **Entidades de dominio y tipos de valor inmutables (`StartupTypes.h`)**:
  - `StartupMode`: Modos operativos (`StandardGui`, `Headless`, `SafeMode`, `Incognito`).
  - `StartupStatusCode`: Códigos de estado y error tipados (`Success`, `DegradedHeadless`, `UserHelpOrVersionRequested`, `InvalidCommandLineArgs`, `IncompatibleQtVersion`, `IncompatibleWebEngine`, `CorruptCriticalResources`, `StorageAccessDenied`, `DisplayServerMissing`, `FatalInitializationError`).
  - `StartupCommandLine`: Estructura inmutable que almacena el resultado del parseo de argumentos, URLs iniciales, indicadores de modo, directorio de perfil validado y lista de argumentos rechazados.
  - `StartupDiagnosticReport`: Reporte estructurado con estado general, fase alcanzada, modo efectivo, código de salida del proceso, ruta de almacenamiento resuelta, indicador de persistencia, mensaje de diagnóstico, acción de remediación sugerida y latencia de inicialización en microsegundos.
- **Coordinador desacoplado de dominio (`StartupBootstrapCoordinator`)**:
  - Orquestador estático puro en C++20 que reside en `vigilio_core`, completamente independiente de `QApplication`, widgets de UI o instancias de WebEngine.
  - Permite evaluar la viabilidad de arranque e inspeccionar el sistema en menos de 10 ms sin iniciar procesos Chromium pesados ni requerir display gráfico.
  - Soporta inyección de dependencias para testing determinista de entornos simulados (headless, fallas de recursos, argumentos inválidos).
- **Invariantes constitucionales de privacidad y seguridad**:
  - *Local-First Constitucional (Q-001)*: Cero peticiones de red, telemetría, ping de actualización o solicitud de cuentas Vigilio durante la secuencia de inicio.
  - *Aislamiento Estricto de Incógnito*: Modo incógnito fuerza `isStoragePersistent = false`, desactivando cualquier escritura de historial, caché de sesión o cookies a disco en `SessionFile` o SQLite.
  - *Defensa contra Inyección de Argumentos Chromium Peligrosos*: Detección y rechazo de banderas como `--no-sandbox`, `--disable-web-security`, `--remote-debugging-port` o `--allow-file-access-from-files`, abortando el arranque con código de error específico.
  - *Defensa contra Path Traversal en Directorios de Perfil*: Inspección rigurosa de `--profile-dir` para bloquear secuencias de escape (`../`) y rutas del sistema operativo protegidas.
- **Transición limpia hacia capas subsiguientes**:
  - Provee la base contractual exacta que será implementada en `apps/vigilio-browser/src/core/StartupTypes.h`, `StartupBootstrapCoordinator.h` y `StartupBootstrapCoordinator.cpp` durante la etapa `core` (`module.foundations.task0022`), conectada a la UI en `task0023` y certificada en `task0024`.

## Out of scope

- Implementación del código fuente C++20 en `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp` y su target en `CMakeLists.txt` (diferida a la etapa `implementation` en `module.foundations.task0022`).
- Conexión de diálogos gráficos de error de arranque en Qt Widgets dentro de `BrowserWindow` (diferida a la etapa `ui` en `module.foundations.task0023`).
- Implementación de la suite integral de pruebas y fixtures de arranque en `tests/unit/StartupBootstrapTests.cpp` (diferida a la etapa `tests` en `module.foundations.task0024`).
- Políticas de recuperación post-crash y restauración de pestañas tras cierre abrupto (perteneciente a la capacidad `crash recovery policy`, microtareas `task0029`-`task0032`).
- Modificación de cualquier archivo bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE, la cual está estrictamente confinada a `openspec/changes/module-foundations-task0021/**`.

## Contexto y evidencia

Se inspeccionó rigurosamente el estado del código fuente existente en `apps/vigilio-browser`, los contratos de arquitectura y el historial de capacidades integradas:
1. **Punto de entrada existente (`apps/vigilio-browser/src/app.cpp`)**:
   - El archivo `src/app.cpp` actualmente implementa una secuencia de inicialización procedural y lineal:
     - Lee manualmente las variables de entorno `DISPLAY` y `WAYLAND_DISPLAY`.
     - Invoca `vigilio::QtDependencyValidator::validateRuntimeEnvironment(qVersion(), QT_VERSION_STR, display, wayland)`.
     - Invoca `vigilio::WebEngineVersionValidator::validateStartup(report.foundVersion, "128.0.6613.119", "", false)`.
     - Invoca `vigilio::ResourceInclusionService::validateStartupResources()`.
     - Si alguna validación falla de forma crítica, imprime un mensaje en `std::cerr` y retorna `1`.
     - Instancia `QApplication app(argc, argv)`, configura metadatos (`organizationName`, `applicationName`, `applicationVersion`) y el patrón de logging `qSetMessagePattern`.
     - Instancia `BrowserWindow window; window.show(); return app.exec();`.
   - **Brecha observada**: No existe un modelo de dominio formal para el arranque. Las fases no están tipadas; no hay parseo de línea de comandos estructurado (`--safe-mode`, `--headless`, `--incognito`); no se valida el aislamiento de almacenamiento ni permisos de disco antes de crear la GUI; y `app.cpp` acopla la validación a la creación directa de widgets sin permitir ejecución en modo headless para pruebas automatizadas o diagnósticos.
2. **Capacidades precedentes integradas en `vigilio_core`**:
   - `QtDependencyValidator.h` / `.cpp`: Valida Qt >= 6.10.0, variables de entorno de display y reporta `QtValidationOutcome`.
   - `WebEngineVersionValidator.h` / `.cpp`: Valida versión de Chromium embebido (milestone >= 128) y compatibilidad headless.
   - `ResourceQrcCatalog.h` / `.cpp`: Catálogo inmutable de 35 recursos canónicos bajo el prefijo `:/vigilio`.
   - `ResourceInclusionService.h` / `.cpp`: Servicio de validación de arranque de activos (`validateStartupResources`) con diagnóstico de criticidad (`MissingCritical`, `CatalogCorruption`).
   - `LocalRepository.h` / `.cpp`: Capa de persistencia SQLite local en `src/storage`.
   - `SessionFile.h` / `.cpp`: Serialización de estado de pestañas y ventanas.
3. **Pipeline de integración continua local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Se ejecutó `bash apps/vigilio-browser/scripts/check.sh` con salida 100% exitosa (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.04s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.03s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.02s)
     - `resource_qrc_inclusion_tests`: Passed (0.02s)
     - Total: 8/8 tests pasados limpiamente en 0.14s.
4. **Validación de herramientas canónicas de Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 21 referencias validadas con estado `valid: true`.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0061-module.foundations.task0021.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true`.
   - Registro de decisiones humanas `docs/rules/answers.md`: Todas las decisiones aplicables (Q-001 local-first, Q-002 C++20/Qt6.10, Q-004 multiplataforma, Q-006 assets) están en estado `ANSWERED`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0021`.
- **Capacidad**: `startup bootstrap`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0021/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0021/proposal.md`
  - `openspec/changes/module-foundations-task0021/design.md`
  - `openspec/changes/module-foundations-task0021/tasks.md`
  - `openspec/changes/module-foundations-task0021/execution-memory.md`
- **Límites de modificación**: Exclusivamente `openspec/changes/module-foundations-task0021/**`. Ningún archivo de producción bajo `apps/**` o regla bajo `docs/rules/**` es modificado en esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la capacidad **startup bootstrap** en su etapa **domain** define la arquitectura del subsistema de arranque desacoplado dentro del espacio de nombres `vigilio`:

### 1. Modelo de estados y transiciones (`StartupPhase`)
La secuencia de arranque se formaliza como una máquina de estados determinista y estrictamente secuencial:
```
[Uninitialized]
       ↓
[EnvironmentInspection] → (Variables de display ausentes sin flag headless) → [DegradedHeadless] o [Failed]
       ↓
[CommandLineParsing]   → (Argumento prohibido o malformado)                 → [Failed] (InvalidCommandLineArgs)
                        → (Flag --help o --version)                          → [Aborted] (Exit 0)
       ↓
[DependencyVerification] → (Qt incompatible o WebEngine incompatible)        → [Failed] (IncompatibleQt / IncompatibleWebEngine)
       ↓
[ResourceVerification]   → (Recurso crítico ausente o catálogo corrupto)    → [Failed] (CorruptCriticalResources)
       ↓
[StorageAndProfileResolution] → (Fallo de permisos en almacenamiento local)  → [Failed] (StorageAccessDenied)
       ↓
[ApplicationConfiguration]
       ↓
     [Ready]
```

### 2. Enumeraciones y tipos de valor (`StartupTypes.h`)
```cpp
namespace vigilio {

enum class StartupPhase : uint8_t {
    Uninitialized,
    EnvironmentInspection,
    CommandLineParsing,
    DependencyVerification,
    ResourceVerification,
    StorageAndProfileResolution,
    ApplicationConfiguration,
    Ready,
    DegradedHeadless,
    Aborted,
    Failed
};

enum class StartupMode : uint8_t {
    StandardGui,
    Headless,
    SafeMode,
    Incognito
};

enum class StartupStatusCode : uint8_t {
    Success = 0,
    DegradedHeadless = 1,
    UserHelpOrVersionRequested = 2,
    InvalidCommandLineArgs = 10,
    DisplayServerMissing = 11,
    IncompatibleQtVersion = 12,
    IncompatibleWebEngine = 13,
    CorruptCriticalResources = 14,
    StorageAccessDenied = 15,
    FatalInitializationError = 20
};

struct StartupCommandLine {
    StartupMode mode = StartupMode::StandardGui;
    std::string profileDirectory;
    std::vector<std::string> initialUrls;
    bool isHeadless = false;
    bool isSafeMode = false;
    bool isIncognito = false;
    bool helpRequested = false;
    bool versionRequested = false;
    std::vector<std::string> rejectedArguments;
    std::string parseErrorMessage;
    bool isValid = true;
};

struct StartupDiagnosticReport {
    StartupStatusCode status = StartupStatusCode::Success;
    StartupPhase failedPhase = StartupPhase::Ready;
    StartupMode effectiveMode = StartupMode::StandardGui;
    int exitCode = 0;
    std::string storagePath;
    bool isStoragePersistent = true;
    std::string diagnosticMessage;
    std::string remediationAction;
    uint64_t elapsedMicroseconds = 0;

    [[nodiscard]] constexpr bool isSuccess() const noexcept {
        return status == StartupStatusCode::Success || status == StartupStatusCode::DegradedHeadless;
    }
};

} // namespace vigilio
```

### 3. Coordinador de dominio (`StartupBootstrapCoordinator.h`)
El coordinador expone métodos estáticos puros para parseo y ejecución de bootstrap, permitiendo inyectar parámetros ambientales en tests sin alterar el entorno global:
```cpp
namespace vigilio {

class StartupBootstrapCoordinator {
public:
    static StartupCommandLine parseCommandLine(int argc, const char* const* argv) noexcept;
    static StartupCommandLine parseCommandLine(const std::vector<std::string_view>& args) noexcept;
    
    static StartupDiagnosticReport executeBootstrap(
        const StartupCommandLine& cmdLine,
        const char* displayEnv,
        const char* waylandEnv,
        const char* customStorageRoot = nullptr) noexcept;

    static constexpr std::string_view phaseToString(StartupPhase phase) noexcept;
    static constexpr std::string_view statusToString(StartupStatusCode status) noexcept;
    static constexpr std::string_view modeToString(StartupMode mode) noexcept;

    static bool isArgumentAllowed(std::string_view arg) noexcept;
    static bool isArgumentForbidden(std::string_view arg) noexcept;
    static bool isValidProfilePath(std::string_view path) noexcept;
};

} // namespace vigilio
```

### 4. Políticas de validación de argumentos y seguridad
- **Argumentos permitidos**:
  - `--headless`: Habilita modo headless para pruebas o automatización local.
  - `--safe-mode`: Desactiva aceleración experimental y extensiones, usando sesión limpia.
  - `--incognito`: Modo efímero en memoria volátil; sin persistencia en disco.
  - `--profile-dir <ruta>`: Especifica directorio de datos local (debe pasar `isValidProfilePath`).
  - `--help`, `-h`: Muestra sintaxis de uso y finaliza con `Aborted` (exit code 0).
  - `--version`, `-v`: Muestra versión de la aplicación y finaliza con `Aborted` (exit code 0).
  - URLs válidas (`http://`, `https://`, `about:blank`, rutas locales `file://`).
- **Argumentos prohibidos y peligrosos**:
  - `--no-sandbox`, `--disable-web-security`: Vulneran el sandbox de Chromium.
  - `--remote-debugging-port`, `--remote-debugging-pipe`: Abren puertos no autorizados de depuración remota.
  - `--allow-file-access-from-files`: Permite escalada cruzada de dominios locales.
  - Secuencias de escape (`../`, `..\`) o caracteres de control en argumentos o rutas de perfil.

## Archivos afectados

### En esta fase PROPOSE (únicos archivos modificados):
- `openspec/changes/module-foundations-task0021/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0021/proposal.md`
- `openspec/changes/module-foundations-task0021/design.md`
- `openspec/changes/module-foundations-task0021/tasks.md`
- `openspec/changes/module-foundations-task0021/execution-memory.md`

### En la fase APPLY subsiguiente (`module.foundations.task0022`):
- `apps/vigilio-browser/src/core/StartupTypes.h` (Nuevo archivo con tipos de dominio de arranque).
- `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` (Nueva cabecera del coordinador de arranque).
- `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp` (Nueva implementación del coordinador).
- `apps/vigilio-browser/src/app.cpp` (Refactorización para consumir `StartupBootstrapCoordinator`).
- `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp` (Suite de pruebas unitarias de bootstrap).
- `apps/vigilio-browser/CMakeLists.txt` (Registro del test `startup_bootstrap_tests` en CTest).
- `docs/rules/01-modules/foundations/micro/0021-startup-bootstrap/task.md` (Actualización de estado a `EXECUTED`).

## Contrato UX/UI

- **Sin cambio visual directo en la etapa domain**:
  - La presente etapa `domain` modela exclusivamente las estructuras de datos, transiciones de estado, políticas de seguridad y validaciones lógicas del subsistema de arranque en `vigilio_core`.
  - No se modifica ningún layout visual, ventana, widget o presentación gráfica en esta microtarea.
- **Preparación contractual para la etapa UI (`task0023`)**:
  - Cuando se conecte a la interfaz en `module.foundations.task0023`, cualquier diálogo visible de error o degradación respetará escrupulosamente `docs/UX-COPY-PUBLICO.md`:
    - Términos de ingeniería como «bootstrap», «startup DAG», «QRC», «milestone WebEngine» o «DOM» quedan terminantemente prohibidos en textos públicos.
    - Se usarán términos sencillos: «No se pudo iniciar el navegador», «Comprueba los permisos de almacenamiento» o «Modo sin ventana activado».
    - En caso de presentarse opciones de interfaz, se empleará exclusivamente la denominación aprobada **«Interfaz más fácil de usar»** para el modo accesible.

## Reglas y fallos

| Condición de entrada | Fase detectora | Estado resultante | Exit code | Acción de remediación / Recuperación |
| --- | --- | --- | --- | --- |
| Entorno GUI normal, dependencias satisfechas, recursos válidos | Todas | `StartupStatusCode::Success` | `0` | Transferencia exitosa a `QApplication::exec()`. |
| Entorno headless sin display pero con flag `--headless` | `EnvironmentInspection` | `StartupStatusCode::Success` | `0` | Operación headless autorizada; no se instancia `BrowserWindow`. |
| Entorno sin display (`DISPLAY` y `WAYLAND_DISPLAY` nulos) sin `--headless` | `EnvironmentInspection` | `StartupStatusCode::DegradedHeadless` | `1` | Notificar ejecución en modo degradado; permitir herramientas CLI o sugerir configurar servidor gráfico. |
| Argumento prohibido detectado (`--no-sandbox`, `--disable-web-security`) | `CommandLineParsing` | `StartupStatusCode::InvalidCommandLineArgs` | `10` | Rechazo inmediato; documentar bandera insegura rechazada en `rejectedArguments`. |
| Solicitud de ayuda o versión (`--help`, `--version`) | `CommandLineParsing` | `StartupStatusCode::UserHelpOrVersionRequested` | `0` | Imprimir texto de ayuda/versión y terminar limpiamente sin inicializar subsistemas pesados (`Aborted`). |
| Versión de Qt runtime incompatible (< 6.10.0) | `DependencyVerification` | `StartupStatusCode::IncompatibleQtVersion` | `12` | Emitir diagnóstico con versión encontrada y requerida; sugerir actualizar librerías Qt del sistema. |
| Chromium WebEngine incompatible o ausente | `DependencyVerification` | `StartupStatusCode::IncompatibleWebEngine` | `13` | Si se requiere GUI, abortar con instrucción de instalación; si corre headless, operar en degradado. |
| Recursos críticos ausentes o XML corrupto | `ResourceVerification` | `StartupStatusCode::CorruptCriticalResources` | `14` | Reportar recurso crítico faltante y sugerir verificar integridad del paquete o reinstalar. |
| Permisos insuficientes en directorio de datos | `StorageAndProfileResolution` | `StartupStatusCode::StorageAccessDenied` | `15` | Reportar ruta inaccesible y sugerir otorgar permisos de lectura/escritura al usuario actual. |
| Directorio de perfil con Directory Traversal (`../`) | `StorageAndProfileResolution` | `StartupStatusCode::InvalidCommandLineArgs` | `10` | Rechazar ruta con advertencia de seguridad y exigir ruta absoluta o relativa limpia. |
| Modo incógnito activado (`--incognito`) | `StorageAndProfileResolution` | `StartupStatusCode::Success` | `0` | Forzar `isStoragePersistent = false`; redirigir a almacenamiento temporal en memoria volátil. |

## Seguridad y privacidad

- **Filosofía Local-First Constitucional (Decisión Q-001)**:
  - Todo el proceso de bootstrap se ejecuta de manera 100% local e independiente.
  - Queda prohibida cualquier conexión de red, ping de comprobación de actualizaciones, telemetría o solicitud de credenciales de usuario durante el inicio.
- **Aislamiento de Perfiles de Incógnito**:
  - Cuando se activa el modo incógnito (`--incognito`), el bootstrap garantiza que el almacenamiento sea no persistente (`isStoragePersistent = false`).
  - No se escribirá historial de navegación, caché persistente en disco ni sesiones previas en `SessionFile` o SQLite.
- **Defensa en Profundidad contra Inyección de Argumentos**:
  - Los argumentos suministrados en `argv` se analizan mediante una lista blanca estricta de opciones aprobadas.
  - Toda bandera peligrosa de Chromium que desactive sandboxing, políticas de mismo origen (SOP) o exponga puertos de depuración es bloqueada en seco.
- **Validación de Rutas de Almacenamiento**:
  - Las rutas de datos de usuario se verifican contra secuencias de Path Traversal (`../`), enlaces simbólicos anómalos y directorios críticos del sistema operativo (`/etc`, `/boot`, `/usr`, `C:\Windows`).
- **Presupuesto de Memoria y Prevención de Procesos Zombis**:
  - Evaluación del coordinador de arranque en memoria < 50 KB.
  - En caso de fallo prematuro en cualquier fase de bootstrap, se cancela la ejecución antes de spawnear procesos auxiliares de WebEngine o hilos de renderizado, garantizando cero procesos zombis en el sistema.

## Matriz de pruebas

| ID Prueba | Tipo | Fixture / Entrada | Comportamiento esperado | Criterio de aceptación / Comando |
| --- | --- | --- | --- | --- |
| `test_bootstrap_normal_gui` | Unitario | Entorno con `DISPLAY=:0`, argumentos por defecto | Estado `Success`, fase `Ready`, modo `StandardGui`, exit code `0`, `isStoragePersistent == true` | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_headless_flag` | Unitario | Argumento `--headless`, sin display | Estado `Success`, modo `Headless`, exit code `0`, sin requerir servidor gráfico | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_missing_display_degraded` | Unitario | `DISPLAY=nullptr`, `WAYLAND_DISPLAY=nullptr`, sin flag | Estado `DegradedHeadless`, exit code `1`, diagnóstico accionable emitido | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_incognito_isolation` | Unitario | Argumento `--incognito` | Estado `Success`, modo `Incognito`, `isStoragePersistent == false`, ruta de datos volátil | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_safe_mode` | Unitario | Argumento `--safe-mode` | Estado `Success`, modo `SafeMode`, extensiones y aceleración experimental omitidas | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_forbidden_arguments` | Seguridad | Argumentos `--no-sandbox`, `--disable-web-security` | Estado `InvalidCommandLineArgs`, exit code `10`, argumentos registrados en `rejectedArguments` | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_profile_traversal_defense` | Seguridad | Argumento `--profile-dir ../../../etc/vigilio` | Estado `InvalidCommandLineArgs`, rechazo estricto de Directory Traversal | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_help_and_version` | Unitario | Argumentos `-h`, `--help`, `-v`, `--version` | Estado `UserHelpOrVersionRequested`, fase `Aborted`, exit code `0` | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_qt_incompatible_failure` | Integración | Simulación de Qt 6.5.0 (< 6.10.0) | Estado `IncompatibleQtVersion`, fase `DependencyVerification`, exit code `12`, mensaje claro | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_resource_corruption_failure` | Integración | Simulación de omisión de recurso crítico | Estado `CorruptCriticalResources`, fase `ResourceVerification`, exit code `14`, remediación clara | `ctest -R startup_bootstrap_tests` |
| `test_bootstrap_concurrency_stress` | Estrés | 16 hilos ejecutando 1000 evaluaciones de bootstrap simultáneas | Cero condiciones de carrera bajo ThreadSanitizer, latencia media < 1 ms por evaluación | `ctest -R startup_bootstrap_tests` |

## Pasos APPLY

Para la fase de implementación incremental (`module.foundations.task0022`), el orden determinista de ejecución será:
1. Crear el archivo de encabezado `apps/vigilio-browser/src/core/StartupTypes.h` conteniendo las enumeraciones `StartupPhase`, `StartupMode`, `StartupStatusCode`, y las estructuras `StartupCommandLine` y `StartupDiagnosticReport`.
2. Crear el archivo de encabezado `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` declarando la clase estática pura `StartupBootstrapCoordinator` y sus métodos de parseo, validación y orquestación.
3. Crear el archivo de implementación `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp` con la lógica de transición de estados, validación de argumentos, reglas de exclusión y orquestación secuencial.
4. Crear la suite de pruebas unitarias `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp` cubriendo exhaustivamente los 11 casos de la matriz de pruebas.
5. Actualizar `apps/vigilio-browser/CMakeLists.txt` incorporando `StartupBootstrapCoordinator.cpp` a la biblioteca estática `vigilio_core`, y registrando el target ejecutable `vigilio_bootstrap_tests` y el test de CTest `startup_bootstrap_tests` con etiquetas `LABELS "unit;fast"`.
6. Refactorizar el punto de entrada `apps/vigilio-browser/src/app.cpp` para delegar la secuencia de inicialización al `StartupBootstrapCoordinator`, preservando el comportamiento observable y manejando limpiamente los modos GUI y Headless.
7. Ejecutar `bash apps/vigilio-browser/scripts/check.sh` verificando compilación limpia con `-Werror`, cero advertencias de Clang-Tidy, formato Clang-Format impecable y 9/9 pruebas pasando al 100%.
8. Actualizar `docs/rules/01-modules/foundations/micro/0021-startup-bootstrap/task.md` sincronizando el estado a `EXECUTED`.

## Definición de terminado

La microtarea de propuesta para `startup bootstrap` se considerará completamente terminada cuando:
- Los documentos de OpenSpec (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`) estén redactados con especificación técnica exhaustiva sin marcadores `TODO`.
- El modelo de dominio de `startup bootstrap` defina claramente entidades de valor, la máquina de estados con 11 fases, las políticas de seguridad y la separación de responsabilidades con respecto a `QApplication`.
- La herramienta canónica `validate-references` confirme que todas las referencias canónicas son válidas.
- La herramienta canónica `verify-lock` certifique la integridad estricta del archivo lock `0061-module.foundations.task0021.propose.lock.json`.
- El reporte de automatización declare el estado `READY` conforme a los lineamientos del orquestador.

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0020`: Completada y certificada exitosamente (8/8 tests pasando en pipeline continuo).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Acoplamiento temprano de la validación de arranque a `QApplication` o clases gráficas de Qt Widgets, impidiendo ejecutar pruebas unitarias en servidores headless de CI sin servidor X11.
    *Mitigación*: Mantener `StartupBootstrapCoordinator` estrictamente en `vigilio_core` como una clase C++20 pura desacoplada de `QApplication`, recibiendo variables de entorno y parámetros como argumentos inyectables.
  - *Riesgo*: Inyección inadvertida de parámetros inseguros de línea de comandos en Chromium embebido.
    *Mitigación*: Implementación de un filtro de argumentos estricto con lista blanca explícita de opciones válidas y lista negra de banderas de riesgo que provocan fallo inmediato (`InvalidCommandLineArgs`).
  - *Riesgo*: Fuga de datos de incógnito a disco en caso de arranque en modo privado.
    *Mitigación*: Forzar invariante `isStoragePersistent = false` en `StartupDiagnosticReport` cada vez que `--incognito` esté activo, impidiendo la inicialización de archivos de sesión en disco.

## Commit propuesto

`docs(foundations): domain startup-bootstrap`
