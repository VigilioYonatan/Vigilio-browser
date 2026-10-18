# Tasks — module-foundations-task0022

Status: PROPOSED

- [ ] TASK-001: Implementar tipos de valor, enumeraciones de ciclo de vida (`StartupPhase`, `StartupMode`, `StartupStatusCode`) y estructuras inmutables (`StartupCommandLine`, `StartupDiagnosticReport`) en `apps/vigilio-browser/src/core/StartupTypes.h`
  - *Criterio de verificación*: El archivo declara formalmente las enumeraciones `StartupPhase` (con 11 fases discretas), `StartupMode` (`StandardGui`, `Headless`, `SafeMode`, `Incognito`), `StartupStatusCode` (con códigos tipados del 0 al 20), la estructura `StartupCommandLine` y la estructura `StartupDiagnosticReport` con método `isSuccess()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/StartupTypes.h && grep -E "enum class StartupPhase|enum class StartupMode|struct StartupCommandLine|struct StartupDiagnosticReport" apps/vigilio-browser/src/core/StartupTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del coordinador de arranque (`StartupBootstrapCoordinator`) en `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h`
  - *Criterio de verificación*: Declaración de la clase `StartupBootstrapCoordinator` en el espacio de nombres `vigilio` con métodos estáticos puros: `parseCommandLine`, `executeBootstrap`, `phaseToString`, `statusToString`, `modeToString`, `isArgumentAllowed`, `isArgumentForbidden` y `isValidProfilePath`, sin dependencias directas de `QApplication` ni de widgets de UI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h && grep -E "class StartupBootstrapCoordinator|executeBootstrap|isValidProfilePath" apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h`

- [ ] TASK-003: Implementar el motor de parseo y saneamiento de línea de comandos (`parseCommandLine`) con detección de banderas permitidas y rechazo de banderas inseguras en `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`
  - *Criterio de verificación*: `parseCommandLine` procesa correctamente `--headless`, `--safe-mode`, `--incognito`, `--profile-dir <ruta>`, `-h`, `--help`, `-v`, `--version` y URLs iniciales; y bloquea inmediatamente banderas peligrosas (`--no-sandbox`, `--disable-web-security`, `--remote-debugging-pipe`, `--allow-file-access-from-files`, `--remote-debugging-port`), marcando `isValid = false` y registrándolas en `rejectedArguments`.
  - *Comando*: `grep -E "parseCommandLine|--no-sandbox|rejectedArguments" apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`

- [ ] TASK-004: Implementar la validación y saneamiento de rutas de perfiles de usuario (`isValidProfilePath`) contra Directory Traversal en `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`
  - *Criterio de verificación*: `isValidProfilePath` rechaza rutas vacías, longitud > 4096 caracteres, caracteres de control, metacaracteres de inyección de shell, secuencias de escape (`..`), rutas del sistema operativo UNIX (`/etc`, `/boot`, `/usr`, `/var`, `/proc`, `/sys`, `/dev`, `/root`) y rutas protegidas de Windows (`C:\Windows`, `C:\Program Files`, UNC `\\`).
  - *Comando*: `grep -E "isValidProfilePath" apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`

- [ ] TASK-005: Implementar la máquina de estados y orquestación secuencial de bootstrap (`executeBootstrap`) en `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`
  - *Criterio de verificación*: `executeBootstrap` orquesta las 7 fases en orden determinista: `EnvironmentInspection` (validación de display y variables), `CommandLineParsing`, `DependencyVerification` (Qt >= 6.10 y WebEngine >= 128), `ResourceVerification` (35 recursos QRC y 8 críticos), `StorageAndProfileResolution` (validación de permisos y aislamiento de incógnito forzando `isStoragePersistent = false`), y `ApplicationConfiguration`, retornando un `StartupDiagnosticReport` inmutable y preciso en menos de 10 ms.
  - *Comando*: `grep -E "executeBootstrap|StartupPhase::StorageAndProfileResolution" apps/vigilio-browser/src/core/StartupBootstrapCoordinator.cpp`

- [ ] TASK-006: Implementar la suite de pruebas unitarias automatizadas con 11 casos de prueba en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Suite completa cubriendo: arranque GUI estándar, modo headless explícito, ausencia de display en modo degradado, aislamiento de incógnito con almacenamiento volátil, modo safe-mode, rechazo de argumentos prohibidos, defensa contra Directory Traversal en perfil, solicitudes de ayuda/versión, simulación de Qt incompatible, simulación de recursos corruptos y prueba de estrés multihilo con 16 hilos.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp && grep -E "test_bootstrap_normal_gui|test_bootstrap_forbidden_arguments|test_bootstrap_concurrency_stress" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-007: Registrar el coordinador en `vigilio_core`, configurar el target ejecutable `vigilio_bootstrap_tests` y el test `startup_bootstrap_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `vigilio_core` incluye `src/core/StartupBootstrapCoordinator.cpp`, `CMakeLists.txt` declara `add_executable(vigilio_bootstrap_tests tests/unit/StartupBootstrapTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, y registra `add_test(NAME startup_bootstrap_tests COMMAND vigilio_bootstrap_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "startup_bootstrap_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-008: Refactorizar el punto de entrada principal `apps/vigilio-browser/src/app.cpp` para delegar la orquestación de inicio en `StartupBootstrapCoordinator`
  - *Criterio de verificación*: `app.cpp` invoca `StartupBootstrapCoordinator::parseCommandLine` y `executeBootstrap`, manejando los códigos de retorno y mostrando diagnósticos estructurados antes de instanciar `QApplication` y `BrowserWindow`.
  - *Comando*: `grep -E "StartupBootstrapCoordinator::executeBootstrap" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-009: Ejecutar el pipeline de comprobación continua local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror`, Clang-Format y 100% de tests pasando en CTest
  - *Criterio de verificación*: Compilación Ninja limpia sin advertencias bajo flags estrictos y `-Werror`; 9/9 pruebas de CTest pasando exitosamente (incluyendo `startup_bootstrap_tests`) con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0022-startup-bootstrap/task.md` y validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: El documento de la microtarea actualiza su estado a `EXECUTED` tras la fase APPLY, y las herramientas de validación de Prompt Builder certifican `valid: true`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0064-module.foundations.task0022.propose.lock.json --project-root .`
