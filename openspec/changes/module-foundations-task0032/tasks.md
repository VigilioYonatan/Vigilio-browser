# Tasks — module-foundations-task0032

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos límite y variaciones de centinela corrupto en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_corrupted_sentinel_variations`) que evalúa centinela de 0 bytes, centinela truncado en JSON, centinela con `schema_version != 1`, centinela con tipos invertidos (cadena en booleano) y números fuera de rango, verificando que todos generan `.corrupt.bak` y reportan estado anómalo sin abortar el proceso.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp && grep -E "test_crash_recovery_corrupted_sentinel_variations" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-002: Implementar pruebas de sesiones corruptas y límites de memoria (Bounded Buffers) en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_corrupted_session_variations`) que evalúa `session.json` vacío (0 bytes), cabecera corrupta sin `VIGILIO_SESSION_V1`, listas con más de 1000 URLs (verificando truncado estricto a 1000 pestañas) y URLs mayores a 8192 bytes.
  - *Comando*: `grep -E "test_crash_recovery_corrupted_session_variations" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-003: Implementar pruebas de seguridad contra esquemas no autorizados en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_security_schemes`) que certifica el rechazo sistemático de esquemas `javascript:`, `data:`, `file:`, `blob:`, `about:`, `vbscript:`, URLs con caracteres de control (`\0`, `\r`, `\n`) y URLs vacías, preservando únicamente `http://`, `https://` y `vigilio://`.
  - *Comando*: `grep -E "test_crash_recovery_security_schemes" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-004: Implementar pruebas de concurrencia multihilo en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_concurrency_and_reentrancy`) que ejecuta al menos 4 hilos concurrentes invocando `updateHeartbeat` y `recordHeartbeat` simultáneamente sobre el mismo archivo centinela; tras sincronizar (`join`), valida que el centinela final en disco es sintácticamente íntegro, legible y no quedan archivos temporales huérfanos.
  - *Comando*: `grep -E "test_crash_recovery_concurrency_and_reentrancy" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-005: Implementar pruebas de fallos inyectados en sistema de archivos y permisos en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_filesystem_io_errors`) que simula directorios de solo lectura y rutas no escribibles, comprobando que `markSessionActive`, `markSessionClean` y `updateHeartbeat` retornan `false` de manera limpia, sin arrojar excepciones no controladas y limpiando cualquier archivo `.tmp`.
  - *Comando*: `grep -E "test_crash_recovery_filesystem_io_errors" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-006: Implementar pruebas de máquina de estados, transiciones inválidas e idempotencia en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_service_state_machine_and_idempotence`) que verifica la invocación de `applyRecovery` sobre un servicio no inicializado, llamadas sucesivas e idempotentes a `discardRecovery` y `cancelRecovery`, y la re-inicialización segura de un servicio activo.
  - *Comando*: `grep -E "test_crash_recovery_service_state_machine_and_idempotence" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-007: Implementar pruebas de sobreescritura de backups en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`
  - *Criterio de verificación*: `CrashRecoveryTests.cpp` incluye una función de prueba (`test_crash_recovery_backup_overwriting`) que valida que al archivar una sesión o centinela corrupto cuando ya existe un backup previo (`.corrupt.bak` o `.discarded.bak`), este es sobrescrito de forma atómica y ordenada sin bloquear ni generar errores.
  - *Comando*: `grep -E "test_crash_recovery_backup_overwriting" apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`

- [ ] TASK-008: Ejecutar el pipeline de compilación y verificación continua local (`apps/vigilio-browser/scripts/check.sh`)
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests de CTest pasando exitosamente en < 1 segundo (exit code 0).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-009: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0094.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0094-module.foundations.task0032.propose.lock.json --project-root .`

- [ ] TASK-010: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase de pruebas automatizadas y se mantiene coherente con la matriz de pruebas de crash recovery policy.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md && grep -E "module.foundations.task0032" docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md`
