# Design — module-foundations-task0021

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Secuencia de inicialización en `apps/vigilio-browser/src/app.cpp`**:
   - Actualmente, `src/app.cpp` ejecuta una secuencia procedural y lineal codificada directamente en `main()`:
     - Inspecciona de forma cruda las variables de entorno `DISPLAY` y `WAYLAND_DISPLAY` mediante `std::getenv`.
     - Invoca `vigilio::QtDependencyValidator::validateRuntimeEnvironment(...)` para verificar Qt >= 6.10.0 y presencia de servidor gráfico.
     - Invoca `vigilio::WebEngineVersionValidator::validateStartup(...)` para verificar que Chromium WebEngine cumpla el milestone mínimo 128.
     - Invoca `vigilio::ResourceInclusionService::validateStartupResources()` para verificar los 35 recursos QRC y los 8 críticos.
     - Si alguna validación devuelve error crítico, imprime un mensaje en `std::cerr` y aborta retornando código `1`.
     - Instancia inmediatamente `QApplication app(argc, argv)` configurando propiedades globales de la aplicación (`organizationName`, `applicationName`, `applicationVersion`) y el patrón de logging `qSetMessagePattern`.
     - Instancia el widget principal `BrowserWindow window; window.show();` y cede el control al bucle de eventos `app.exec()`.

2. **Componentes base en `vigilio_core`**:
   - `QtDependencyValidator`: Provee validación de versión y entorno gráfico.
   - `WebEngineVersionValidator`: Provee validación de versión de Chromium y compatibilidad con modo degradado headless.
   - `ResourceQrcCatalog` y `ResourceInclusionService`: Proveen el inventario inmutable de 35 recursos bajo `:/vigilio` y validación de criticidad para el arranque.
   - `UrlInput`, `TabModel`, `SessionFile`: Manejo de entradas, modelo de pestañas y serialización de sesión en disco.

3. **Almacenamiento local en `vigilio_storage`**:
   - `LocalRepository`: Encapsula la base de datos SQLite local para historiales y configuraciones bajo `apps/vigilio-browser/src/storage`.

### Estado objetivo (TARGET)

1. **Arquitectura desacoplada de arranque en `vigilio_core`**:
   - Centralizar y formalizar toda la lógica del ciclo de vida de arranque en una entidad coordinadora de dominio: `StartupBootstrapCoordinator`.
   - La clase `StartupBootstrapCoordinator` es completamente independiente de Qt Widgets, de `QApplication` y de procesos WebEngine, residiendo en la biblioteca estática `vigilio_core`.
   - Permite evaluar de manera determinista la viabilidad de inicio en microsegundos tanto en entornos de escritorio como en servidores CI sin servidor gráfico.

2. **Máquina de estados determinista (`StartupPhase`)**:
   - Formalizar 11 fases discretas: `Uninitialized`, `EnvironmentInspection`, `CommandLineParsing`, `DependencyVerification`, `ResourceVerification`, `StorageAndProfileResolution`, `ApplicationConfiguration`, `Ready`, `DegradedHeadless`, `Aborted`, `Failed`.
   - Cada fase cuenta con precondiciones explícitas, transiciones observables y códigos de salida unificados en `StartupStatusCode`.

3. **Parseo y saneamiento estructurado de línea de comandos (`StartupCommandLine`)**:
   - Parseador nativo en C++20 que procesa `argc`/`argv` reconociendo banderas canónicas:
     - `--headless`: Operación de testing o diagnósticos sin inicializar ventanas.
     - `--safe-mode`: Modo seguro a prueba de fallos sin extensiones ni aceleraciones experimentales.
     - `--incognito`: Modo efímero en memoria volátil; sin persistencia en disco de historial ni sesión.
     - `--profile-dir <ruta>`: Directorio de perfil local con saneamiento estricto contra Path Traversal (`../`).
     - `--help`, `-h`: Ayuda del sistema (terminación limpia con `Aborted` y exit code 0).
     - `--version`, `-v`: Versión del software (terminación limpia con `Aborted` y exit code 0).
     - URLs iniciales validadas.
   - Defensa activa contra argumentos peligrosos de Chromium (`--no-sandbox`, `--disable-web-security`, `--remote-debugging-port`), provocando rechazo inmediato con código `InvalidCommandLineArgs`.

4. **Reporte diagnóstico estructurado (`StartupDiagnosticReport`)**:
   - Valor inmutable que consolida el estado final, la fase alcanzada, el modo efectivo, el código de salida numérico, el indicador de persistencia de almacenamiento (`isStoragePersistent`), los diagnósticos humanos y las acciones de remediación.

5. **Aislamiento estricto de privacidad e incógnito**:
   - Si `isIncognito` es verdadero, el coordinador garantiza que `isStoragePersistent == false`, forzando el uso de almacenamiento efímero en memoria y prohibiendo la apertura de bases de datos de historial o escritura en `SessionFile`.

### Brechas identificadas (GAPS)

1. **Ausencia de abstracción de arranque**: Toda la lógica previa estaba mezclada proceduralmente en `app.cpp` dentro del flujo `main()`, impidiendo su verificación unitaria desacoplada.
2. **Cero soporte para argumentos CLI**: No existía mecanismo para arrancar en `--headless`, `--safe-mode`, `--incognito` o `--profile-dir`.
3. **Falta de protección contra inyección de argumentos inseguros**: No se inspeccionaban banderas potencialmente catastróficas para la seguridad del navegador (`--no-sandbox`).
4. **Verificación de permisos de almacenamiento ausente**: La aplicación intentaba inicializar la interfaz antes de comprobar si el directorio de datos local tenía permisos de lectura y escritura.
5. **Aislamiento de sesiones efímeras no coordinado**: No existía un contrato formal a nivel de dominio que garantizara que un arranque de incógnito deshabilitara la persistencia en disco.

---

## Detailed Architecture and Domain Model

### 1. Diagrama de flujo de la máquina de estados de arranque

```
                                  [ Inicio ]
                                      │
                                      ▼
                             [ Uninitialized ]
                                      │
                                      ▼
                        [ EnvironmentInspection ]
                       /                         \
           (Variables display válidas)    (Sin display: DISPLAY / WAYLAND nulos)
                     /                             \
                    /                      ¿Flag --headless activo?
                   /                       /                      \
                  /                     (Sí)                      (No)
                 │                        │                         │
                 │                        ▼                         ▼
                 │               [ Modo Headless ]        [ DegradedHeadless ]
                 │                        │               (Exit 1 / Diagnóstico)
                 ▼                        ▼
                       [ CommandLineParsing ]
                      /           │          \
         (Args válidos)   (Arg inseguro)   (--help / --version)
               │                  │                   │
               │                  ▼                   ▼
               │              [ Failed ]         [ Aborted ]
               │      (InvalidCommandLineArgs)    (Exit 0)
               ▼
                    [ DependencyVerification ]
                      /                      \
              (Qt >= 6.10 & WebEngine >= 128)  (Incompatible)
                     │                                │
                     │                                ▼
                     │                            [ Failed ]
                     │                      (IncompatibleQt/WebEngine)
                     ▼
                     [ ResourceVerification ]
                      /                      \
               (35 recursos OK)        (Crítico faltante o XML corrupto)
                     │                                │
                     │                                ▼
                     │                            [ Failed ]
                     │                     (CorruptCriticalResources)
                     ▼
                  [ StorageAndProfileResolution ]
                      /                         \
            (Permisos disco OK)         (Error de permisos / Path traversal)
                     │                                  │
                     │                                  ▼
                     │                              [ Failed ]
                     │                       (StorageAccessDenied)
                     ▼
                  [ ApplicationConfiguration ]
                               │
                               ▼
                            [ Ready ]
                               │
             (Handoff a QApplication::exec() & BrowserWindow)
```

### 2. Estructuras de datos de dominio (`StartupTypes.h`)

#### Enumeración de fases (`StartupPhase`)
Representa cada etapa atómica de la secuencia de arranque:
- `Uninitialized`: Estado previo a la evaluación.
- `EnvironmentInspection`: Detección de variables de entorno del sistema (`DISPLAY`, `WAYLAND_DISPLAY`, `VIGILIO_HEADLESS`).
- `CommandLineParsing`: Parseo y validación de argumentos en `argv`.
- `DependencyVerification`: Verificación de versiones de Qt y WebEngine.
- `ResourceVerification`: Verificación de catálogo de recursos QRC.
- `StorageAndProfileResolution`: Verificación de rutas de almacenamiento local y aislamiento de incógnito.
- `ApplicationConfiguration`: Establecimiento de metadatos de la aplicación.
- `Ready`: Secuencia completada exitosamente; lista para la interfaz gráfica.
- `DegradedHeadless`: Entorno sin display gráfico pero operable para CLI o tests.
- `Aborted`: Finalización limpia solicitada por el usuario (ayuda, versión).
- `Failed`: Falla irrecuperable en alguna fase crítica.

#### Enumeración de modos de arranque (`StartupMode`)
- `StandardGui`: Modo estándar de navegador con ventana gráfica Qt.
- `Headless`: Modo sin display para automatización, CI y diagnósticos.
- `SafeMode`: Modo a prueba de fallos sin extensiones ni aceleraciones experimentales.
- `Incognito`: Modo de navegación privada con almacenamiento 100% volátil en memoria.

#### Enumeración de códigos de estado (`StartupStatusCode`)
- `Success = 0`: Arranque plenamente satisfactorio.
- `DegradedHeadless = 1`: Ejecución en modo headless degradado por ausencia de display.
- `UserHelpOrVersionRequested = 2`: Terminación limpia tras mostrar ayuda o versión.
- `InvalidCommandLineArgs = 10`: Argumentos malformados, rutas ilegales o banderas inseguras.
- `DisplayServerMissing = 11`: Imposible inicializar GUI por falta de display.
- `IncompatibleQtVersion = 12`: Versión de Qt runtime incompatible (< 6.10.0).
- `IncompatibleWebEngine = 13`: Versión de Chromium WebEngine incompatible (< 128).
- `CorruptCriticalResources = 14`: Recursos indispensables de interfaz dañados o ausentes.
- `StorageAccessDenied = 15`: Directorio de datos inaccesible o sin permisos de escritura.
- `FatalInitializationError = 20`: Error general no recuperable.

#### Estructura `StartupCommandLine`
Contiene los resultados del parseo de argumentos de línea de comandos:
- `StartupMode mode`: Modo de ejecución resuelto.
- `std::string profileDirectory`: Ruta al directorio de perfil personalizado (o vacío para el predeterminado).
- `std::vector<std::string> initialUrls`: Lista de URLs pasadas como argumentos para abrir al inicio.
- `bool isHeadless`: `true` si se especificó `--headless`.
- `bool isSafeMode`: `true` si se especificó `--safe-mode`.
- `bool isIncognito`: `true` si se especificó `--incognito`.
- `bool helpRequested`: `true` si se especificó `-h` o `--help`.
- `bool versionRequested`: `true` si se especificó `-v` o `--version`.
- `std::vector<std::string> rejectedArguments`: Lista de argumentos prohibidos o no reconocidos.
- `std::string parseErrorMessage`: Mensaje explicativo en caso de error de parseo.
- `bool isValid`: `true` si la línea de comandos no contiene errores sintácticos ni argumentos prohibidos.

#### Estructura `StartupDiagnosticReport`
Contiene el balance completo de la ejecución de bootstrap:
- `StartupStatusCode status`: Código de estado resultante.
- `StartupPhase failedPhase`: Última fase evaluada o fase en la que ocurrió el fallo.
- `StartupMode effectiveMode`: Modo de operación efectivo adoptado.
- `int exitCode`: Código de retorno para `main()` / sistema operativo.
- `std::string storagePath`: Ruta del directorio de datos de usuario resuelta.
- `bool isStoragePersistent`: `false` en incógnito; `true` en modo estándar.
- `std::string diagnosticMessage`: Explicación comprensible del resultado.
- `std::string remediationAction`: Pasos concretos de solución ante errores.
- `uint64_t elapsedMicroseconds`: Duración total de la evaluación de bootstrap.
- Método de conveniencia: `bool isSuccess() const noexcept` (retorna `true` para `Success` y `DegradedHeadless`).

---

## Invariantes y Políticas de Seguridad

1. **Invariante Local-First Constitucional (Decisión Q-001)**:
   - Toda la secuencia de arranque se ejecuta de forma completamente aislada de la red.
   - No se emiten pings, telemetría, consultas de actualización en segundo plano ni verificaciones de licencia remota.

2. **Invariante de Aislamiento de Incógnito**:
   - Cuando `mode == StartupMode::Incognito` o `cmdLine.isIncognito == true`:
     - El reporte resultante garantiza `isStoragePersistent == false`.
     - Queda terminantemente prohibido acceder a la base de datos de historial ordinario o guardar el archivo de sesión (`SessionFile`).
     - Todo dato temporal debe dirigirse a memoria RAM o directorio efímero autodescartable al cierre del proceso.

3. **Invariante de Defensa contra Banderas Peligrosas**:
   - Se prohíbe explícitamente el paso de argumentos que degraden las protecciones de seguridad de Chromium:
     - `--no-sandbox`
     - `--disable-web-security`
     - `--remote-debugging-port`
     - `--remote-debugging-pipe`
     - `--allow-file-access-from-files`
   - La presencia de cualquiera de estas banderas provoca que `isValid` sea `false`, se agregue la bandera a `rejectedArguments`, y el bootstrap aborte de inmediato con `InvalidCommandLineArgs` (exit code 10).

4. **Invariante de Saneamiento de Rutas de Perfil**:
   - Las rutas especificadas mediante `--profile-dir` se someten a `isValidProfilePath`:
     - Rechazo estricto de secuencias de Path Traversal (`../`, `..\`).
     - Rechazo de caracteres de control o inyecciones de comandos (`|`, `;`, `&`, `$`).
     - Bloqueo de rutas de sistema protegidas (`/`, `/etc`, `/boot`, `/usr`, `/var`, `C:\Windows`, `C:\Program Files`).

5. **Invariante de Presupuesto y Desacoplamiento Headless**:
   - La evaluación completa de `StartupBootstrapCoordinator::executeBootstrap` debe completarse en menos de 10 milisegundos en pruebas locales.
   - La lógica no requiere inicializar `QApplication` ni crear ventanas `QWidget`, posibilitando su ejecución en cualquier arnés de prueba de CTest sin requerir servidor gráfico X11 ni Wayland.

---

## Compatibility, migration and rollback

### Compatibilidad con versiones anteriores
- La introducción de `StartupBootstrapCoordinator` no rompe ningún contrato público ni privado existente.
- Preserva la semántica de arranque de `app.cpp`:
  - Si el entorno es un escritorio válido, la aplicación se iniciará de manera idéntica a la actual.
  - Si falta display, la aplicación retornará código 1 informando del error como lo hace actualmente, pero de forma estructurada y con diagnóstico claro.
- Es 100% compatible con los targets existentes de CTest (`core_units`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`, `webengine_version_validator_tests`, `resource_qrc_catalog_tests`, `resource_inclusion_service_tests`, `resource_qrc_inclusion_tests`).

### Estrategia de migración
- La capacidad se introduce en tres etapas secuenciales:
  1. **Etapa domain (esta microtarea 0021)**: Definición formal de contratos, estructuras, estados e invariantes en OpenSpec.
  2. **Etapa core (microtarea 0022)**: Implementación de `StartupTypes.h`, `StartupBootstrapCoordinator.h` y `StartupBootstrapCoordinator.cpp` en `vigilio_core`, refactorización de `app.cpp` y suite de pruebas unitarias.
  3. **Etapa ui (microtarea 0023)**: Integración con diálogos de interfaz gráfica y adaptación para «Interfaz más fácil de usar».
  4. **Etapa tests (microtarea 0024)**: Batería exhaustiva de pruebas de regresión, fuzzing de argumentos y concurrencia.

### Plan de rollback
- En caso de contingencia o detección de regresión en las fases subsiguientes, el rollback es trivial: `app.cpp` puede restituirse a su versión procedural original sin afectar a otros módulos, dado que `vigilio_core` y `vigilio_storage` mantienen sus interfaces existentes intactas.
