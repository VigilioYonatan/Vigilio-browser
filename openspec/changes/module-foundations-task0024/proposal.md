# Change Proposal — module-foundations-task0024

Pack: `module.foundations.task0024`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **startup bootstrap** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0024 — Probar casos positivos, negativos y recuperación: startup bootstrap`).

Habiendo formalizado en `module.foundations.task0021` la especificación del modelo de dominio, las 11 fases discretas del ciclo de vida y los invariantes de seguridad; implementado en `module.foundations.task0022` el coordinador desacoplado en C++20 (`vigilio::StartupBootstrapCoordinator`), la pre-validación de arranque y el soporte para entornos headless; y proyectado en `module.foundations.task0023` la interacción en Qt Widgets (`BrowserWindow`), la gestión de estados visuales (vacío, cargando, error, modo seguro e incógnito), la navegación asistiva por teclado y la adaptación a «Interfaz más fácil de usar» (Modo Senior), esta microtarea establece la **suite de pruebas automatizadas deterministas, focalizadas y de alta resiliencia para certificar el subsistema de startup bootstrap ante casos normales, entradas inválidas, inyección de fallos, estrés concurrente multihilo, recuperación degradada y ausencia de regresiones**, sin requerir la instanciación de procesos Chromium reales ni servidor gráfico.

El alcance concreto de este pack comprende:
- **Suite de pruebas unitarias y de estrés automatizadas (`apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`)**:
  - **Casos positivos y normales**:
    - Validación integral de la secuencia de arranque en modo estándar GUI (`StartupMode::StandardGui`) con URLs iniciales y directorio de perfil válido.
    - Validación de modo headless explícito (`--headless`) ejecutándose con éxito tanto con servidor gráfico presente como en su ausencia total (`DISPLAY=nullptr`, `WAYLAND_DISPLAY=nullptr`).
    - Validación de modo seguro (`--safe-mode`) con preservación de estado e inicialización limpia de la configuración mínima de protección.
    - Validación de sesión privada de incógnito (`--incognito`): confirmación del invariante arquitectónico de persistencia en memoria volátil (`report.storagePath == ":memory:"`, `report.isStoragePersistent == false`) y ausencia absoluta de creación de directorios o archivos en disco.
    - Validación de resolución de directorio de perfil personalizado con sintaxis de dos tokens (`--profile-dir /tmp/dir`) y sintaxis de token único con asignación (`--profile-dir=/tmp/dir`).
    - Validación de apertura y preservación en orden de múltiples URLs iniciales HTTP/HTTPS (`https://vigilio.local/home`, `https://example.com/test`).
    - Validación de solicitudes legítimas de información al usuario: flags `-h`, `--help`, `-v` y `--version` retornando estado `UserHelpOrVersionRequested`, fase `Aborted` y exit code 0.
  - **Casos negativos y robustez ante entradas inválidas o maliciosas (fuzzing y seguridad)**:
    - Rechazo estricto de banderas prohibidas de seguridad de Chromium que comprometerían el sandboxing o la privacidad del usuario:
      - `--no-sandbox`
      - `--disable-web-security`
      - `--remote-debugging-pipe`
      - `--allow-file-access-from-files`
      - `--remote-debugging-port` (tanto flag individual como con asignación de puerto arbitrario `--remote-debugging-port=9222`)
    - Rechazo de opciones de línea de comandos no reconocidas o malformadas (`--unsupported-flag`, `--enable-features=...`, `--flag`).
    - Validación y defensa activa contra secuencias de Directory Traversal en la ruta del perfil (`isValidProfilePath`):
      - Cadenas con secuencias `../`, `..`, `sub/..`, secuencias en Windows `..\system32`.
      - Cadenas vacías o con longitud excesiva (> 4096 caracteres).
      - Caracteres de control ASCII (< 32 o 127) y metacaracteres de inyección de comandos shell (`|`, `;`, `&`, `$`, `` ` ``, `<`, `>`, `"`, `'`).
      - Rutas de sistema protegidas o raíces de sistema operativo prohibidas:
        - Unix: `/`, `/etc`, `/etc/shadow`, `/boot`, `/usr`, `/var`, `/bin`, `/sbin`, `/lib`, `/lib64`, `/proc`, `/sys`, `/dev`, `/root`.
        - Windows: `\\server\share` (rutas UNC), `C:\Windows`, `C:\Windows\System32`, `c:/windows/system32`, `C:\Program Files`.
  - **Inyección de fallos, recuperación determinista y resiliencia**:
    - *Ausencia de servidor gráfico*: cuando la aplicación se arranca en modo GUI estándar sin display disponible (`DISPLAY=nullptr`, `WAYLAND_DISPLAY=nullptr`), no aborta abruptamente ni provoca segmentation fault, sino que degrada de forma controlada a `DegradedHeadless` (exit code 1, `isSuccess() == true`), reportando diagnóstico accionable y recomendación de remediación.
    - *Incompatibilidad inyectada de versión de Qt*: simulación de runtime con versión incompatible (ej. Qt `5.15.2` o `6.9.3` frente al baseline >= `6.10.0`), verificando detención en fase `DependencyVerification`, código de estado `IncompatibleQtVersion`, exit code 12 y diagnóstico estructurado.
    - *Corrupción inyectada de recursos críticos QRC*: simulación de manifiesto QRC malformado o sin el prefijo canónico `/vigilio`, verificando detención en fase `ResourceVerification`, código de estado `CorruptCriticalResources`, exit code 14 y mensaje de remediación.
    - *Denegación de acceso al sistema de archivos*: simulación de directorio de almacenamiento sin permisos de escritura (ej. `/proc/vigilio_forbidden_dir_test`), verificando detención en fase `StorageAndProfileResolution`, código de estado `StorageAccessDenied`, exit code 15 y diagnóstico claro.
  - **Concurrencia multihilo, latencia y control de recursos**:
    - Prueba de estrés masivo multihilo ejecutando 16 hilos en paralelo con 500 iteraciones por hilo (8,000 ciclos completos de evaluación de bootstrap) alternando aleatoriamente entre los modos StandardGui, Headless, Incognito, SafeMode y banderas prohibidas.
    - Certificación de ausencia de condiciones de carrera (data races), confirmando la pureza funcional y la seguridad de hilos (`thread-safe`) bajo C++20.
    - Medición del tiempo de procesamiento (`elapsedMicroseconds`): verificación de que el diagnóstico reporta latencias reales y que cada ciclo de evaluación se ejecuta en menos de 50 milisegundos.
  - **Consistencia y serialización diagnóstica**:
    - Verificación biunívoca de las funciones de conversión de enumeraciones a cadenas legibles: `phaseToString` (11 fases discretas), `statusToString` (10 códigos de estado) y `modeToString` (4 modos operativos).
- **Aislamiento de recursos y política Zero-Zombie**:
  - Las pruebas unitarias se ejecutan de forma completamente autónoma, sin inicializar subprocesos Chromium reales (`QtWebEngineProcess`), sin requerir servidor gráfico ni GPU, sin abrir sockets de red ni generar procesos zombis o colgados.
- **Integración y gobernanza en CTest (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Mantenimiento del target ejecutable `vigilio_bootstrap_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`.
  - Prueba de CTest registrada como `startup_bootstrap_tests` con etiquetas `LABELS "unit;fast"`, timeout de 15 segundos y tiempo de ejecución real < 100 milisegundos.

## Out of scope

- Modificación de la arquitectura y contratos de dominio formalizados en `module.foundations.task0021`.
- Modificación de la implementación funcional del coordinador de arranque (`StartupBootstrapCoordinator.h/.cpp`), completada y congelada en `module.foundations.task0022`.
- Alteraciones de la interfaz de usuario en `BrowserWindow` o cambios de layout visual en Qt Widgets, pertenecientes a la etapa `ui` (`module.foundations.task0023`).
- Pruebas E2E pesadas con navegación web real sobre sitios remotos o interacción visual manual en el escritorio (reservadas para suites E2E dedicadas como `vigilio_e2e_smoke`).
- Implementación de políticas de recuperación post-crash o journaling de sesiones (reservado a `crash recovery policy`, tareas 0029 a 0032).
- Cualquier llamada de red externa, comprobación de actualizaciones remotas, login de usuario o telemetría en la nube (prohibido taxativamente por la política local-first, decisión Q-001).
- Modificación de archivos de código fuente bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida exclusivamente a `openspec/changes/module-foundations-task0024/**`).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos de dominio y las herramientas del entorno local:
1. **Infraestructura de tipos y entidades de arranque (`apps/vigilio-browser/src/core/StartupTypes.h`)**:
   - Define las 11 fases del ciclo de vida (`StartupPhase`): `Uninitialized`, `EnvironmentInspection`, `CommandLineParsing`, `DependencyVerification`, `ResourceVerification`, `StorageAndProfileResolution`, `ApplicationConfiguration`, `Ready`, `DegradedHeadless`, `Aborted`, `Failed`.
   - Define los 4 modos operativos (`StartupMode`): `StandardGui`, `Headless`, `SafeMode`, `Incognito`.
   - Define los 10 códigos de estado (`StartupStatusCode`): `Success` (0), `DegradedHeadless` (1), `UserHelpOrVersionRequested` (2), `InvalidCommandLineArgs` (10), `DisplayServerMissing` (11), `IncompatibleQtVersion` (12), `IncompatibleWebEngine` (13), `CorruptCriticalResources` (14), `StorageAccessDenied` (15), `FatalInitializationError` (20).
   - Estructuras inmutables de datos: `StartupCommandLine` (argumentos parseados, banderas, URLs, parámetros rechazados) y `StartupDiagnosticReport` (estado, fase fallida, modo efectivo, código de salida, ruta de storage, persistencia, mensaje diagnóstico, acción de remediación, microsegundos transcurridos).
2. **Coordinador desacoplado de dominio (`apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` y `.cpp`)**:
   - Componente puro de dominio en `vigilio_core`, sin acoplamiento a widgets ni a procesos WebEngine.
   - Provee los métodos públicos estáticos:
     - `parseCommandLine(int argc, const char* const* argv)` y `parseCommandLine(const std::vector<std::string_view>& args)`
     - `executeBootstrap(const StartupCommandLine& cmdLine, const char* displayEnv, const char* waylandEnv, const char* customStorageRoot, const char* simulatedQtVersion, const char* simulatedQrcContent)`
     - `isArgumentForbidden(std::string_view arg)` y `isArgumentAllowed(std::string_view arg)`
     - `isValidProfilePath(std::string_view path)`
     - Métodos de formato: `phaseToString`, `statusToString`, `modeToString`.
3. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - Realiza la orquestación en runtime: parsea argumentos CLI, invoca `StartupBootstrapCoordinator::executeBootstrap`, evalúa el reporte resultante, y aborta con código de salida limpio o inicializa la aplicación.
4. **Suite de pruebas existente (`apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`)**:
   - Cuenta con una base sólida de 13 funciones de prueba que validan casos normales, modos headless, safe mode e incognito, banderas prohibidas, directory traversal, help/version, fallos inyectados de Qt y QRC, storage access denied, estrés de 16 hilos y conversiones de cadenas.
   - Pasa al 100% en CTest mediante el target `startup_bootstrap_tests` (exit code 0).
5. **Herramientas locales y verificación del pipeline (`scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 y `-Werror`.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: 9/9 pruebas pasadas con éxito total en 0.25 segundos (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.07s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.04s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.03s)
     - `resource_qrc_inclusion_tests`: Passed (0.04s)
     - `startup_bootstrap_tests`: Passed (0.06s)
6. **Verificación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0070-module.foundations.task0024.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 24/24 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: confirmó que todas las decisiones de foundations y bootstrap están resueltas (Q-001 a Q-006, Q-008 resueltas; Q-007 solo aplica al release final).
7. **Referencias visuales y directrices de producto**:
   - `/resultado/01-vigilio-home.png`: Ventana principal del navegador con buscador central, pestañas y diseño sobrio.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Panel de accesibilidad y controles ampliados.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial de marca y logotipo.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición total de tecnicismos internos en cadenas públicas; los mensajes diagnósticos y acciones de remediación deben respetar el lenguaje accesible.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0024`.
- **Capacidad**: `startup bootstrap`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0024/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0024/proposal.md`
  - `openspec/changes/module-foundations-task0024/design.md`
  - `openspec/changes/module-foundations-task0024/tasks.md`
  - `openspec/changes/module-foundations-task0024/execution-memory.md`
- **Restricción estricta de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0024/**`. No se autoriza modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño integral de la capacidad de pruebas automatizadas de startup bootstrap contempla la consolidación de 14 módulos de verificación determinista en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`:
1. `test_bootstrap_normal_gui()`: Inicialización en modo StandardGui con display activo (`:0`), verificación de URLs pasadas por CLI, resolución de almacenamiento persistente en directorio temporal aislado y validación de `report.isSuccess() == true` con exit code 0.
2. `test_bootstrap_headless_explicit()`: Verificación de flag `--headless`, confirmando que opera con éxito independientemente de si las variables `DISPLAY` o `WAYLAND_DISPLAY` están presentes o ausentes.
3. `test_bootstrap_degraded_headless_missing_display()`: Verificación de degradación controlada ante ausencia de servidor gráfico sin `--headless`, comprobando transición a `StartupStatusCode::DegradedHeadless`, exit code 1, `isSuccess() == true`, diagnóstico claro y acción de remediación no vacía.
4. `test_bootstrap_incognito_volatile_storage()`: Verificación del invariante constitucional de privacidad: `--incognito` fuerza `storagePath == ":memory:"`, `isStoragePersistent == false`, y garantiza que el directorio en disco no se crea ni se modifica.
5. `test_bootstrap_safe_mode()`: Verificación de `--safe-mode`, confirmando modo efectivo `StartupMode::SafeMode`, inicialización con almacenamiento persistente y sin degradación imprevista.
6. `test_bootstrap_profile_dir_variations()`: Verificación exhaustiva de parsing de directorio de perfil con dos tokens (`--profile-dir`, `/tmp/custom`) y con token asignado (`--profile-dir=/tmp/custom`), así como directorios relativos válidos (`profiles/profile1`).
7. `test_bootstrap_forbidden_arguments()`: Rechazo taxativo de banderas peligrosas (`--no-sandbox`, `--disable-web-security`, `--remote-debugging-pipe`, `--allow-file-access-from-files`, `--remote-debugging-port`, `--remote-debugging-port=9222`) y opciones arbitrarias no autorizadas, reportando `StartupStatusCode::InvalidCommandLineArgs`, fase `CommandLineParsing` y exit code 10.
8. `test_bootstrap_directory_traversal_defense()`: Fuzzing exhaustivo sobre `isValidProfilePath`: secuencias relativas `..`, rutas Unix protegidas (`/etc`, `/boot`, `/sys`, `/proc`, `/root`), rutas Windows (`\\server\share`, `C:\Windows\System32`), caracteres de control y metacaracteres shell.
9. `test_bootstrap_help_and_version_requests()`: Solicitudes informativas con `--help`, `-h`, `--version` y `-v`, confirmando retorno de `StartupStatusCode::UserHelpOrVersionRequested`, fase `Aborted` y exit code 0 sin inicializar recursos pesados.
10. `test_bootstrap_incompatible_qt_simulation()`: Inyección de versión incompatible de Qt (`5.15.2`, `6.9.3`), validando detención en fase `DependencyVerification`, código `IncompatibleQtVersion`, exit code 12 y mensaje estructurado.
11. `test_bootstrap_corrupt_resources_simulation()`: Inyección de contenido QRC corrupto (sin prefijo `/vigilio` o XML malformado), validando detención en fase `ResourceVerification`, código `CorruptCriticalResources`, exit code 14 y mensaje de remediación.
12. `test_bootstrap_storage_access_denied()`: Inyección de ruta de almacenamiento no escribible (ej. `/proc/vigilio_forbidden_dir_test`), validando detención en fase `StorageAndProfileResolution`, código `StorageAccessDenied`, exit code 15 y diagnóstico claro.
13. `test_bootstrap_concurrency_stress()`: Prueba de estrés multihilo masivo con 16 hilos simultáneos y 500 iteraciones por hilo (8,000 evaluaciones totales) alternando modos y flags, verificando ausencia de data races, thread-safety completa y latencia acotada.
14. `test_bootstrap_string_conversions()`: Verificación biunívoca y exhaustiva de todas las conversiones `phaseToString`, `statusToString` y `modeToString`.

## Archivos afectados

- **Durante la fase PROPOSE (alcance actual)**:
  - `openspec/changes/module-foundations-task0024/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0024/proposal.md`
  - `openspec/changes/module-foundations-task0024/design.md`
  - `openspec/changes/module-foundations-task0024/tasks.md`
  - `openspec/changes/module-foundations-task0024/execution-memory.md`
- **Durante la fase APPLY (incremento de implementación posterior)**:
  - `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`: Refinamiento y consolidación de la suite de pruebas unitarias y de estrés.
  - `apps/vigilio-browser/CMakeLists.txt`: Verificación de configuración del target `vigilio_bootstrap_tests` y propiedades del test de CTest `startup_bootstrap_tests`.
  - `docs/rules/01-modules/foundations/micro/0024-startup-bootstrap/task.md`: Actualización del estado a ejecutado con evidencia verificable.

## Contrato UX/UI

- **Sin cambio visual directo en la ventana principal**: Esta etapa corresponde exclusivamente a pruebas automatizadas de nivel unitario, estrés y diagnóstico de arranque en la capa C++20 de `foundations`. No altera widgets ni introduce nuevos controles gráficos en `BrowserWindow`.
- **Cumplimiento de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Las pruebas verifican que los mensajes diagnósticos (`diagnosticMessage`) y las acciones de remediación (`remediationAction`) generadas por el subsistema de bootstrap no expongan términos prohibidos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec».
  - En caso de degradación a modo headless o advertencias de display, el texto descriptivo utiliza lenguaje claro y comprensible para el usuario.
- **Preservación del resultado visual de referencia (`resultado/01-vigilio-home.png` y `03-vigilio-settings-senior-mode.png`)**:
  - Al no modificar archivos de UI, se preserva íntegramente la estética sobria, los tokens de diseño y la compatibilidad con «Interfaz más fácil de usar».

## Reglas y fallos

- **Taxonomía de códigos de estado de arranque (`StartupStatusCode`)**:
  - `Success (0)`: Arranque completado con éxito; interfaz gráfica lista para desplegar.
  - `DegradedHeadless (1)`: Servidor gráfico no disponible; la aplicación degrada a modo sin interfaz gráfica de forma controlada.
  - `UserHelpOrVersionRequested (2)`: El usuario invocó `-h`, `--help`, `-v` o `--version`; se imprimió la información solicitada y el proceso finaliza limpiamente.
  - `InvalidCommandLineArgs (10)`: Se pasaron banderas prohibidas de Chromium (`--no-sandbox`, etc.), argumentos desconocidos o rutas con Directory Traversal.
  - `DisplayServerMissing (11)`: Error fatal por ausencia de display en configuraciones que exigen estrictamente GUI.
  - `IncompatibleQtVersion (12)`: La versión en tiempo de ejecución de Qt es inferior al umbral canónico (`6.10.0`).
  - `IncompatibleWebEngine (13)`: Incompatibilidad detectada con las capacidades o versión del motor Chromium.
  - `CorruptCriticalResources (14)`: Los recursos embebidos QRC críticos no se encuentran o el catálogo está corrupto.
  - `StorageAccessDenied (15)`: Permiso de escritura denegado en la ruta del perfil o directorio de almacenamiento.
  - `FatalInitializationError (20)`: Excepción imprevista o fallo irrecuperable durante la secuencia de inicio.
- **Reglas de recuperación y degradación**:
  - Toda condición de fallo debe poblar campos no vacíos en `diagnosticMessage` y `remediationAction`, guiando al operador o usuario sobre la acción correctiva.
  - Las sesiones privadas forzadas con `--incognito` nunca deben generar directorios persistentes ni archivos temporales huérfanos.

## Seguridad y privacidad

- **Filosofía Local-First (Decisión canónica Q-001)**:
  - Cero dependencias de red: ninguna función del subsistema de arranque intenta comunicarse con servidores remotos ni realizar consultas DNS/HTTP.
  - Cero telemetría remota: no se envían reportes diagnósticos ni trazas de arranque hacia la nube.
- **Aislamiento en modo incógnito**:
  - Toda la persistencia opera en memoria (`":memory:"`), impidiendo que cookies, historial o caché se escriban en disco.
- **Protección contra ataques de inyección y Directory Traversal**:
  - Bloqueo determinista de secuencias `../` y metacaracteres shell en `--profile-dir`.
  - Prohibición estricta de rutas de sistema (`/etc`, `/boot`, `/sys`, `/proc`, `/root`, `C:\Windows\System32`).
- **Bloqueo de banderas inseguras de Chromium**:
  - Bloqueo de `--no-sandbox` para evitar la ejecución sin aislamiento de procesos.
  - Bloqueo de `--disable-web-security` para preservar la política de mismo origen (SOP).
  - Bloqueo de `--remote-debugging-port` y `--remote-debugging-pipe` para evitar accesos no autorizados a la interfaz DevTools desde la red local o procesos locales.

## Matriz de pruebas

| ID | Categoría | Tipo de caso | Fixture / Entrada | Comando de ejecución | Resultado esperado | Criterio de fallo negativo |
|---|---|---|---|---|---|---|
| TEST-001 | Unitario (APPLY) | Caso feliz | URL `https://vigilio.local/home`, display `:0` | `ctest -R startup_bootstrap_tests` | `isSuccess() == true`, `status == Success`, `exitCode == 0` | Fallo si `status != Success` o `exitCode != 0` |
| TEST-002 | Unitario (APPLY) | Caso feliz | `--headless`, sin display (`nullptr`) | `ctest -R startup_bootstrap_tests` | `isSuccess() == true`, `mode == Headless`, `exitCode == 0` | Fallo si no acepta ejecución headless sin display |
| TEST-003 | Unitario (APPLY) | Recuperación | Modo GUI sin display (`nullptr`, `nullptr`) | `ctest -R startup_bootstrap_tests` | `status == DegradedHeadless`, `exitCode == 1`, remediation no vacía | Fallo si aborta con crash o código no documentado |
| TEST-004 | Seguridad (APPLY) | Aislamiento | `--incognito`, display `:0` | `ctest -R startup_bootstrap_tests` | `storagePath == ":memory:"`, `isStoragePersistent == false`, cero archivos en disco | Fallo si persiste en disco o crea carpetas |
| TEST-005 | Unitario (APPLY) | Caso feliz | `--safe-mode`, display `:0` | `ctest -R startup_bootstrap_tests` | `status == Success`, `mode == SafeMode`, `exitCode == 0` | Fallo si no activa modo seguro o rechaza flag |
| TEST-006 | Seguridad (APPLY) | Entrada inválida | `--no-sandbox`, `--disable-web-security`, `--remote-debugging-port=9222` | `ctest -R startup_bootstrap_tests` | `isValid == false`, `status == InvalidCommandLineArgs`, `exitCode == 10` | Fallo si permite flags peligrosos |
| TEST-007 | Seguridad (APPLY) | Entrada inválida | `--profile-dir ../evil`, `/etc/shadow`, `C:\Windows\System32` | `ctest -R startup_bootstrap_tests` | `isValidProfilePath == false`, `exitCode == 10` | Fallo si acepta secuencias de Directory Traversal o rutas root |
| TEST-008 | Unitario (APPLY) | Caso feliz | `--help`, `-h`, `--version`, `-v` | `ctest -R startup_bootstrap_tests` | `status == UserHelpOrVersionRequested`, `failedPhase == Aborted`, `exitCode == 0` | Fallo si reporta error o código de salida distinto de 0 |
| TEST-009 | Recuperación (APPLY) | Fallo inyectado | Versión simulada de Qt `5.15.2` / `6.9.3` | `ctest -R startup_bootstrap_tests` | `status == IncompatibleQtVersion`, `failedPhase == DependencyVerification`, `exitCode == 12` | Fallo si ignora incompatibilidad de Qt |
| TEST-010 | Recuperación (APPLY) | Fallo inyectado | Manifiesto QRC corrupto sin prefijo `/vigilio` | `ctest -R startup_bootstrap_tests` | `status == CorruptCriticalResources`, `failedPhase == ResourceVerification`, `exitCode == 14` | Fallo si ignora recursos corruptos |
| TEST-011 | Recuperación (APPLY) | Fallo inyectado | Directorio no accesible `/proc/vigilio_forbidden_dir_test` | `ctest -R startup_bootstrap_tests` | `status == StorageAccessDenied`, `failedPhase == StorageAndProfileResolution`, `exitCode == 15` | Fallo si no detecta denegación de almacenamiento |
| TEST-012 | Estrés (APPLY) | Concurrencia | 16 hilos × 500 iteraciones (8,000 llamadas concurrentes) | `ctest -R startup_bootstrap_tests` | 8,000 evaluaciones exitosas, cero data races, latencia total acotada | Fallo si se produce deadlock, race condition o crash |
| TEST-013 | Unitario (APPLY) | Consistencia | Enum conversions para 11 fases, 10 estados y 4 modos | `ctest -R startup_bootstrap_tests` | Cadenas exactas retornadas para cada valor enum | Fallo si retorna cadena vacía o "Unknown" para valor válido |
| TEST-014 | Regresión (APPLY) | Pipeline | Ejecución completa de suite de tests en `check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | 100% pruebas aprobadas en CTest, compilación limpia con `-Werror` | Fallo si algún test de foundations falla |

## Pasos APPLY

1. **Revisión y consolidación de `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`**:
   - Verificar aserciones rigurosas para cada caso positivo, negativo, de recuperación y concurrencia.
   - Asegurar que los fixtures sintéticos temporales se limpien deterministamente con RAII o llamadas explícitas al inicio y al final de cada test.
2. **Verificación de registro en `apps/vigilio-browser/CMakeLists.txt`**:
   - Confirmar que el target ejecutable `vigilio_bootstrap_tests` y el test `startup_bootstrap_tests` estén correctamente registrados con etiquetas `LABELS "unit;fast"` y timeout de 15 segundos.
3. **Ejecución del pipeline continuo de verificación**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` para certificar compilación Ninja limpia y 100% de tests aprobados en CTest.
4. **Sincronización documental**:
   - Actualizar el estado de la microtarea en `docs/rules/01-modules/foundations/micro/0024-startup-bootstrap/task.md` documentando la evidencia de pruebas.
5. **Certificación con Prompt Builder CLI**:
   - Ejecutar `validate-references`, `verify-lock` y `validate-decisions`.

## Definición de terminado

- La suite de pruebas `startup_bootstrap_tests` se compila bajo C++20 con `-Werror` y se ejecuta en CTest aprobando el 100% de los casos sin fallos.
- Todos los casos positivos, negativos, de recuperación degradada, inyección de fallos y concurrencia multihilo están cubiertos y documentados.
- Ninguna prueba requiere dependencias de red, servidor gráfico ni procesos Chromium reales (`QtWebEngineProcess`).
- `bash apps/vigilio-browser/scripts/check.sh` finaliza con código de salida 0.
- El lockfile y las referencias OpenSpec son validados con Prompt Builder CLI sin discrepancias.

## Dependencias y riesgos

- **Dependencia de ejecución**: `module.foundations.task0023` (interacción y estados visuales en Qt completada).
- **Riesgo**: Entornos de CI sin servidor gráfico (sin X11/Wayland).
  - *Mitigación*: Las pruebas están diseñadas para pasar variables `nullptr` en `displayEnv` y `waylandEnv` o verificar la degradación controlada a `DegradedHeadless`, asegurando ejecución determinista en entornos headless de CI.
- **Riesgo**: Conflictos en el sistema de archivos por rutas temporales compartidas.
  - *Mitigación*: Cada test genera prefijos únicos dentro de `std::filesystem::temp_directory_path()` y limpia minuciosamente los directorios antes y después de cada prueba.

## Commit propuesto

`test(foundations): tests startup-bootstrap`
