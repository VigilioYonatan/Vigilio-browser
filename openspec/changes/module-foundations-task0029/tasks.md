# Tasks — module-foundations-task0029

Status: PROPOSED

- [ ] TASK-001: Definir enumeraciones fuertemente tipadas (`CrashDetectionState`, `RecoveryAction`, `CrashReason`) y estructuras inmutables (`CrashSentinel`, `CrashRecoveryConfig`, `CrashRecoveryReport`) en `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`
  - *Criterio de verificación*: El archivo declara formalmente `CrashDetectionState` (con 6 estados discretos), `RecoveryAction` (con 5 acciones de recuperación), `CrashReason` (con 6 causas tipadas), la estructura `CrashSentinel` serializable, `CrashRecoveryConfig` con soporte para incógnito y umbral de bucle de caídas, y `CrashRecoveryReport` con método `hasRecoverableTabs()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CrashRecoveryTypes.h && grep -E "enum class CrashDetectionState|enum class RecoveryAction|struct CrashSentinel|struct CrashRecoveryReport" apps/vigilio-browser/src/core/CrashRecoveryTypes.h`

- [ ] TASK-002: Declarar la interfaz estática desacoplada del coordinador de dominio (`CrashRecoveryCoordinator`) en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`
  - *Criterio de verificación*: Declaración de la clase estática `CrashRecoveryCoordinator` en el espacio de nombres `vigilio` con métodos puros de inspección (`inspectSessionState`), mutación atómica de centinela (`markSessionActive`, `markSessionClean`, `updateHeartbeat`), saneamiento de URLs (`sanitizeRecoverableUrls`), archivo seguro de sesiones corruptas (`archiveCorruptedSession`) y conversiones de texto (`stateToString`, `actionToString`, `reasonToString`), completamente independiente de `QApplication` y `QWebEngineView`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h && grep -E "class CrashRecoveryCoordinator|inspectSessionState|markSessionActive|sanitizeRecoverableUrls" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`

- [ ] TASK-003: Implementar la inspección determinista de centinela, detección de caída anómala y protección contra bucle de caídas en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `inspectSessionState` evalúa el centinela en disco; si `cleanExit == false`, identifica caída anormal e incrementa el conteo de caídas consecutivas; si el conteo alcanza `crashLoopThreshold`, escala el estado a `RepeatedCrashes` recomendando `SafeModeRecovery`; si `config.isIncognito == true`, garantiza cero lecturas o escrituras retornando `NoPreviousSession` y `StartClean`.
  - *Comando*: `grep -E "inspectSessionState|RepeatedCrashes|SafeModeRecovery|isIncognito" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-004: Implementar el protocolo de marcado atómico de sesión activa (`markSessionActive`), latido (`updateHeartbeat`) y cierre limpio (`markSessionClean`) en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `markSessionActive` escribe un JSON compacto en `.tmp` y lo reemplaza atómicamente (`MoveFileExW` / `std::filesystem::rename`) con `clean_exit: false`; `markSessionClean` lo actualiza con `clean_exit: true` y resetea el contador de caídas a 0; en modo incógnito, no realiza operaciones en disco.
  - *Comando*: `grep -E "markSessionActive|markSessionClean|updateHeartbeat" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-005: Implementar el saneamiento riguroso de URLs y archivo de sesiones dañadas en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `sanitizeRecoverableUrls` utiliza `resolveInput`, descarta esquemas inseguros (`javascript:`, `data:`, `file:`), descarta URLs que excedan 8192 bytes, limita el resultado a 1000 pestañas y elimina duplicados contiguos; `archiveCorruptedSession` renombra el archivo dañado a `.corrupt.bak`.
  - *Comando*: `grep -E "sanitizeRecoverableUrls|archiveCorruptedSession" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-006: Implementar la suite de pruebas unitarias y de resiliencia en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: Suite completa cubriendo los 11 escenarios de la matriz de pruebas: perfil limpio sin centinela, salida previa limpia, caída anómala, bucle de caídas consecutivas, aislamiento absoluto de incógnito (cero I/O), centinela corrupto, archivo de sesión corrupto, saneamiento de URLs, ciclo de vida del centinela, latido y conversiones de texto.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp && grep -E "test_crash_recovery_no_previous_session|test_crash_recovery_abnormal_termination|test_crash_recovery_repeated_crashes_loop" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-007: Incorporar el coordinador en `vigilio_core`, configurar el target ejecutable `vigilio_crash_recovery_tests` y el test `crash_recovery_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `vigilio_core` incluye `src/core/CrashRecoveryCoordinator.cpp`, `CMakeLists.txt` declara `add_executable(vigilio_crash_recovery_tests tests/unit/CrashRecoveryTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, y registra `add_test(NAME crash_recovery_tests COMMAND vigilio_crash_recovery_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "crash_recovery_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-008: Ejecutar el pipeline de comprobación continua local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror`, Clang-Format y 100% de tests pasando en CTest
  - *Criterio de verificación*: Compilación Ninja limpia sin advertencias bajo flags estrictos y `-Werror`; todas las pruebas de CTest pasando exitosamente (incluyendo `crash_recovery_tests`) con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-009: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0085.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0085-module.foundations.task0029.propose.lock.json --project-root .`

- [ ] TASK-010: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0029-crash-recovery-policy/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase y se mantiene coherente con el modelo de dominio implementado.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0029-crash-recovery-policy/task.md && grep -E "module.foundations.task0029" docs/rules/01-modules/foundations/micro/0029-crash-recovery-policy/task.md`
