# Tasks — module-foundations-task0026

Status: PROPOSED

- [ ] TASK-001: Implementar tipos de valor, enumeraciones de ciclo de vida (`AppLifetimeState`, `ShutdownReason`, `ShutdownAction`) y estructuras inmutables (`LifetimeConfig`, `LifetimeReport`) en `apps/vigilio-browser/src/core/LifetimeTypes.h`
  - *Criterio de verificación*: El archivo declara formalmente las enumeraciones fuertemente tipadas `AppLifetimeState` (con 9 estados discretos), `ShutdownReason` (con 8 causas tipadas), `ShutdownAction` (con 4 acciones de resolución), la estructura `LifetimeConfig` (con soporte para incógnito, modo seguro y timeout configurable de 3000 ms) y la estructura `LifetimeReport` con método de consulta pura `isCleanExit()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/LifetimeTypes.h && grep -E "enum class AppLifetimeState|enum class ShutdownReason|struct LifetimeReport" apps/vigilio-browser/src/core/LifetimeTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del coordinador de ciclo de vida (`ApplicationLifetimeCoordinator`) en `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h`
  - *Criterio de verificación*: Declaración de la clase `ApplicationLifetimeCoordinator` en el espacio de nombres `vigilio` con métodos de consulta (`currentState`, `isShutdownRequested`, `isIncognito`), métodos de mutación atómica (`transitionTo`, `requestShutdown`, `executeShutdown`), y funciones estáticas de inspección y conversión (`canTransition`, `isSignalSafe`, `stateToString`, `reasonToString`, `actionToString`), completamente independiente de `QApplication` y widgets de UI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h && grep -E "class ApplicationLifetimeCoordinator|executeShutdown" apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h`

- [ ] TASK-003: Implementar la máquina de estados determinista, la matriz de transiciones legales (`canTransition`) y las utilidades puras de conversión a texto en `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`
  - *Criterio de verificación*: `canTransition` valida exhaustivamente las transiciones permitidas según el diseño de dominio y rechaza transiciones anómalas; las funciones `stateToString`, `reasonToString` y `actionToString` retornan representaciones textuales canónicas no vacías.
  - *Comando*: `grep -E "canTransition|stateToString|reasonToString" apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`

- [ ] TASK-004: Implementar la deduplicación atómica de solicitudes de apagado, manejo seguro de señales del SO y aceleración de emergencia ante señales repetidas en `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`
  - *Criterio de verificación*: `requestShutdown` maneja atómicamente múltiples solicitudes concurrentes con `shutdownRequestCount_`, retornando `IgnoreRequest` si ya existe un apagado en curso, o acelerando a `EmergencyAbort` si se reciben múltiples señales repetidas del sistema operativo (`SignalInterrupt` o `SignalTerminate`).
  - *Comando*: `grep -E "requestShutdown|shutdownInProgress_|shutdownRequestCount_" apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`

- [ ] TASK-005: Implementar el flujo de apagado ordenado (`executeShutdown`) con persistencia atómica, aislamiento estricto de incógnito y temporizador de seguridad de 3000 ms en `apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`
  - *Criterio de verificación*: `executeShutdown` orquesta la secuencia ordenada de vaciado y desmontaje; si `config.isIncognito == true`, jamás invoca `sessionSaver` y asegura que no se escriban datos en disco; si la operación excede `shutdownTimeoutMs`, activa `timeoutTriggered = true` y transiciona inmediatamente a `EmergencyAbort`.
  - *Comando*: `grep -E "executeShutdown|isIncognito|shutdownTimeoutMs|timeoutTriggered" apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.cpp`

- [ ] TASK-006: Implementar la suite completa de pruebas unitarias y de resiliencia en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Suite completa de 11 pruebas cubriendo: estado inicial, matriz exhaustiva de transiciones legales e ilegales, apagado ordenado normal con persistencia de sesión, aislamiento absoluto de incógnito (cero escrituras en disco), manejo de señales `SIGINT`/`SIGTERM`, deduplicación de solicitudes multihilo concurrentes con 32 hilos, aceleración de señales repetidas, temporizador de seguridad (watchdog timeout) y conversiones de texto.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp && grep -E "test_lifetime_initial_state|test_lifetime_incognito_zero_trace|test_lifetime_idempotent_shutdown_requests" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-007: Incorporar el coordinador en `vigilio_core`, configurar el target ejecutable `vigilio_lifetime_tests` y el test `application_lifetime_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `vigilio_core` incluye `src/core/ApplicationLifetimeCoordinator.cpp`, `CMakeLists.txt` declara `add_executable(vigilio_lifetime_tests tests/unit/ApplicationLifetimeTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, y registra `add_test(NAME application_lifetime_tests COMMAND vigilio_lifetime_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "application_lifetime_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-008: Ejecutar el pipeline de comprobación continua local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror`, Clang-Format y 100% de tests pasando en CTest (10/10)
  - *Criterio de verificación*: Compilación Ninja limpia sin advertencias bajo flags estrictos y `-Werror`; 10/10 pruebas de CTest pasando exitosamente (incluyendo `application_lifetime_tests`) con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-009: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0076-module.foundations.task0026.propose.lock.json --project-root .`

- [ ] TASK-010: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0026-application-lifetime/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase y se mantiene coherente con la implementación propietaria C++20.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0026-application-lifetime/task.md && grep -E "module.foundations.task0026" docs/rules/01-modules/foundations/micro/0026-application-lifetime/task.md`
