# Tasks — module-foundations-task0036

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos negativos extremos y sintaxis corrupta en `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`
  - *Criterio de verificación*: `CMakePresetsTests.cpp` incluye la función de prueba `test_presets_corrupted_json_syntax_variations` que evalúa documentos JSON con caracteres binarios nulos (`\0`), llaves y corchetes desbalanceados, comillas no cerradas y arrays en la raíz, comprobando que retornan `outcome == CMakePresetValidationOutcome::MalformedJson` y `isValid() == false` sin abortar el proceso.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp && grep -E "test_presets_corrupted_json_syntax_variations" apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`

- [ ] TASK-002: Implementar pruebas de inversión de tipos y estructuras malformadas en `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`
  - *Criterio de verificación*: `CMakePresetsTests.cpp` incluye la función de prueba `test_presets_type_inversion_and_malformed_structures` que evalúa versiones de esquema como cadena (`"version": "6"`), `configurePresets` declarado como objeto asociativo en lugar de lista, y `cacheVariables` con tipos anómalos, validando que el validador rechaza el documento de forma segura.
  - *Comando*: `grep -E "test_presets_type_inversion_and_malformed_structures" apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`

- [ ] TASK-003: Implementar pruebas de defensa contra inyección de comandos shell y path traversal en `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`
  - *Criterio de verificación*: `CMakePresetsTests.cpp` incluye la función de prueba `test_presets_security_injection_and_path_hygiene` que evalúa nombres de presets hostiles (`; rm -rf /`, `&& ls`, `| cat`, `$(calc)`) y rutas binarias con directory traversal avanzado (`${sourceDir}/build/../../secret`, rutas UNC `\\server\share`), certificando su rechazo categórico con `UnsafeBinaryDir` o sanitización estricta.
  - *Comando*: `grep -E "test_presets_security_injection_and_path_hygiene" apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`

- [ ] TASK-004: Implementar pruebas de concurrencia multihilo de lectura en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` incluye la función de prueba `test_service_concurrency_multi_threaded_reads` que ejecuta al menos 8 hilos (`std::thread`) concurrentes realizando consultas (`findPreset`, `listPresetsByKind`, `buildConfigureCommandLine`, `summary`) sobre una instancia compartida de `CMakePresetsService`, comprobando ausencia total de carreras de datos y sincronización limpia con `join()`.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp && grep -E "test_service_concurrency_multi_threaded_reads" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-005: Implementar pruebas de estrés de cancelación atómica e idempotencia en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` incluye la función de prueba `test_service_cancellation_stress_and_idempotence` que evalúa múltiples llamadas concurrentes a `requestCancellation()` sobre `CancellationToken`, comprobando atomicidad, idempotencia y detención inmediata de la operación con estado `Cancelled` y exitCode -1.
  - *Comando*: `grep -E "test_service_cancellation_stress_and_idempotence" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de inyección de errores de sistema de archivos en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` incluye la función de prueba `test_service_filesystem_io_errors_and_recovery` que simula directorios de solo lectura y rutas bloqueadas durante la ejecución de `cleanBinaryDir`, comprobando que retorna `false` limpiamente, asigna diagnóstico descriptivo en `errorMessage` y no propaga excepciones no capturadas.
  - *Comando*: `grep -E "test_service_filesystem_io_errors_and_recovery" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de bounded buffers (64 KB) y política Zero-Zombie en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` incluye la función de prueba `test_service_bounded_buffers_and_zero_zombie` que comprueba el truncado estricto de captura de stdout/stderr a 64 KB (`maxOutputBytes = 65536`) y el respeto de los límites de timeout.
  - *Comando*: `grep -E "test_service_bounded_buffers_and_zero_zombie" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-008: Implementar prueba de auditoría y cumplimiento de `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` incluye la función de prueba `test_service_public_copy_compliance` que audita todas las cadenas devueltas por `outcomeToString`, `executionStatusToString` y `executionStateToString`, certificando la ausencia de términos prohibidos de ingeniería («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "test_service_public_copy_compliance" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-009: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, Clang-Format verificado y 100% de suites de prueba en CTest aprobadas exitosamente (exit code 0 en < 1s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0106.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0106-module.foundations.task0036.propose.lock.json --project-root .`

- [ ] TASK-011: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0036-cmake-presets/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de CMake Presets.
  - *Comando*: `grep -E "module.foundations.task0036" docs/rules/01-modules/foundations/micro/0036-cmake-presets/task.md`
