# Tasks — module-foundations-task0030

Status: PROPOSED

- [ ] TASK-001: Refinar y declarar tipos operativos de servicio (`RecoveryServiceState`, operaciones de resolución) en `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`
  - *Criterio de verificación*: El archivo declara formalmente los estados operativos del servicio (`RecoveryServiceState`) y las estructuras de soporte para decisiones y cancelaciones de recuperación, manteniendo C++20 estricto e inmutabilidad en los reportes.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CrashRecoveryTypes.h && grep -E "enum class RecoveryServiceState|RecoveryAction" apps/vigilio-browser/src/core/CrashRecoveryTypes.h`

- [ ] TASK-002: Declarar la interfaz del servicio operativo de recuperación (`CrashRecoveryService` / capa operativa de coordinación) en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`
  - *Criterio de verificación*: Declaración de la clase u operaciones de servicio en el espacio de nombres `vigilio` con métodos `initialize`, `applyRecovery`, `discardRecovery`, `cancelRecovery`, `recordHeartbeat`, y `finalizeShutdown`, completamente independiente de Qt Widgets y WebEngine.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h && grep -E "class CrashRecoveryService|initialize|applyRecovery|discardRecovery" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`

- [ ] TASK-003: Implementar la orquestación de inicialización (`initialize`) y retención de estado en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `initialize` evalúa el centinela en disco; si la sesión previa terminó limpiamente o no existía rastro previo, transiciona a sesión activa marcando el centinela; si se detectó caída anómala, transiciona a `AwaitingDecision` reteniendo las URLs saneadas en memoria sin iniciar WebEngine.
  - *Comando*: `grep -E "initialize|AwaitingDecision|SessionActive" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-004: Implementar las operaciones de resolución de recuperación (`applyRecovery`, `discardRecovery`, `cancelRecovery`) en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `applyRecovery` entrega las URLs saneadas y activa la sesión; `discardRecovery` descarta las URLs y archiva de forma segura el archivo de sesión a `.discarded.bak`; `cancelRecovery` revierte la solicitud manteniendo el estado accesible.
  - *Comando*: `grep -E "applyRecovery|discardRecovery|cancelRecovery" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-005: Implementar el registro de latido periódico (`recordHeartbeat`) y finalización de apagado limpio (`finalizeShutdown`) en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: `recordHeartbeat` actualiza `last_heartbeat_time` atómicamente; `finalizeShutdown` recibe el resultado de `ApplicationLifetimeCoordinator` y ejecuta `markSessionClean` únicamente si el cierre fue ordenado.
  - *Comando*: `grep -E "recordHeartbeat|finalizeShutdown|markSessionClean" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-006: Garantizar el aislamiento estricto de Incógnito (Q-001) y tolerancia a errores de disco en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`
  - *Criterio de verificación*: Si `config.isIncognito == true`, todas las operaciones devuelven éxito inmediato sin tocar el sistema de archivos; los errores de E/S o permisos denegados se gestionan degradadamente sin provocar excepciones no capturadas.
  - *Comando*: `grep -E "isIncognito" apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`

- [ ] TASK-007: Ampliar la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: Suite completa cubriendo los 12 escenarios de la matriz de pruebas, incluyendo inicialización de servicio, retención de URLs en caída previa, restauración, descarte con archivo seguro, cancelación, latido y apagado ordenado.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp && grep -E "test_crash_recovery_service|test_crash_recovery_discard|test_crash_recovery_cancel" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-008: Verificar target CMake `vigilio_crash_recovery_tests` y ejecutar el pipeline de comprobación continua local (`apps/vigilio-browser/scripts/check.sh`)
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests de CTest pasando exitosamente (exit code 0).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-009: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0088.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0088-module.foundations.task0030.propose.lock.json --project-root .`

- [ ] TASK-010: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase y se mantiene coherente con la implementación propietaria C++20.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md && grep -E "module.foundations.task0030" docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md`
