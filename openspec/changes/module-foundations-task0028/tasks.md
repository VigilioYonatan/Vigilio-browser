# Tasks — module-foundations-task0028

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de cancelación voluntaria de apagado y retorno a estado operativo (`test_lifetime_cancellation_and_recovery`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida que al solicitar apagado (`ShutdownRequested`), cancelar la operación revirtiendo a `Running` restaura `isShutdownRequested() == false` y permite tramitar con éxito posteriores solicitudes de cierre ordenado.
  - *Comando*: `grep -E "test_lifetime_cancellation_and_recovery" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-002: Implementar pruebas de resiliencia ante fallos en callbacks inyectados y punteros nulos (`test_lifetime_callback_failures_resilience`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida que si `sessionSaver` o `dbCloser` devuelven `false` o se pasan como `nullptr`, el coordinador maneja la situación sin excepciones no capturadas ni crashes, registrando los flags en `LifetimeReport` y completando el cierre.
  - *Comando*: `grep -E "test_lifetime_callback_failures_resilience" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-003: Implementar pruebas de invocación directa de `executeShutdown` desde estados iniciales sin `requestShutdown` previo en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida que invocar `executeShutdown` directamente en estados `Starting` o `Running` fuerza la transición interna a `ShutdownRequested` o `EmergencyAbort` y concluye en `Terminated`.
  - *Comando*: `grep -E "test_lifetime_direct_execute_shutdown" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-004: Implementar pruebas de parámetros extremos de configuración y frontera (`test_lifetime_extreme_timeouts`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida el comportamiento con `shutdownTimeoutMs = 0` (disparo inmediato de timeout si hay retardo) y con `shutdownTimeoutMs = UINT32_MAX`, así como rutas nulas o vacías en `LifetimeConfig`.
  - *Comando*: `grep -E "test_lifetime_extreme_timeouts" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-005: Implementar pruebas de contención y carreras críticas multi-hilo sobre `executeShutdown` (`test_lifetime_concurrent_execute_shutdown`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: 16 hilos compiten invocando `executeShutdown` simultáneamente; la bandera `shutdownInProgress_` asegura la serialización y que las lambdas de persistencia se ejecutan en un único hilo.
  - *Comando*: `grep -E "test_lifetime_concurrent_execute_shutdown" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-006: Implementar pruebas de motivos críticos de parada como agotamiento de memoria y fallos de almacenamiento (`test_lifetime_critical_reasons`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida que `MemoryExhaustion` devuelve `ProceedImmediate`, transiciona a `EmergencyAbort`, omite el vaciado de estado a disco y finaliza con código 1.
  - *Comando*: `grep -E "test_lifetime_critical_reasons" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-007: Implementar pruebas de verificación de métricas de temporización de alta resolución (`test_lifetime_timing_metrics`) en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`
  - *Criterio de verificación*: Valida que `sessionFlushElapsedMicroseconds` y `teardownElapsedMicroseconds` se registran con valores coherentes y no negativos en `LifetimeReport`.
  - *Comando*: `grep -E "test_lifetime_timing_metrics" apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`

- [ ] TASK-008: Verificar target `vigilio_lifetime_tests` en `apps/vigilio-browser/CMakeLists.txt` con labels `"unit;fast"` y timeout de 15 segundos
  - *Criterio de verificación*: `CMakeLists.txt` define el target `vigilio_lifetime_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`, y `add_test(NAME application_lifetime_tests)` con labels `"unit;fast"`.
  - *Comando*: `grep -A 4 "add_executable(vigilio_lifetime_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-009: Ejecutar el pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror`, Clang-Format y 100% de tests pasando en CTest
  - *Criterio de verificación*: Compilación limpia sin advertencias bajo Ninja y C++20; 100% de pruebas pasando en CTest con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0082.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0082-module.foundations.task0028.propose.lock.json --project-root .`

- [ ] TASK-011: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase y se mantiene coherente con la implementación de pruebas en C++20.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md && grep -E "module.foundations.task0028" docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md`
