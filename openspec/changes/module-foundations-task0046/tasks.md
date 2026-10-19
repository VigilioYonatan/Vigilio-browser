# Tasks — module-foundations-task0046

Status: PROPOSED

- [ ] TASK-001: Extender el modelo de dominio con las enumeraciones fuertemente tipadas y estructuras inmutables operativas en `apps/vigilio-browser/src/core/ClangTidyTypes.h`
  - *Criterio de verificación*: `ClangTidyTypes.h` define `enum class TidyExecutionStatus : uint8_t`, `struct SourceFileTidyInspection`, `struct TidyServiceOptions`, `struct TidyOperationResult`, y el token de cancelación cooperativa `struct CancellationToken` protegido por macro guard `VIGILIO_CANCELLATION_TOKEN_DEFINED`, compilando limpiamente bajo C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyTypes.h && grep -E "enum class TidyExecutionStatus|struct SourceFileTidyInspection|struct TidyServiceOptions|struct TidyOperationResult|struct CancellationToken" apps/vigilio-browser/src/core/ClangTidyTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/ClangTidyService.h`
  - *Criterio de verificación*: `ClangTidyService.h` declara la clase `ClangTidyService` en el namespace `vigilio` con métodos de configuración (`loadConfiguration`, `setOptions`, `options`, `isConfigured`), análisis y corrección en memoria (`inspectSource`, `applyFixesToSource`), inspección y corrección en disco (`inspectFile`, `inspectDirectory`, `applyFixesToFile`, `applyFixesWithRetry`, `applyFixesToDirectory`), gestión de respaldo (`createBackup`, `rollbackFile`, `removeBackup`), síntesis CLI (`buildClangTidyCommandLine`) y diagnósticos (`executionStatusToString`, `generateRemediationHint`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.h && grep -E "class ClangTidyService|inspectSource|applyFixesToSource|inspectFile|applyFixesWithRetry|createBackup|rollbackFile|buildClangTidyCommandLine" apps/vigilio-browser/src/core/ClangTidyService.h`

- [ ] TASK-003: Implementar la inicialización, configuración, inspección y corrección determinista en memoria en `apps/vigilio-browser/src/core/ClangTidyService.cpp`
  - *Criterio de verificación*: `ClangTidyService.cpp` implementa `loadConfiguration`, `setOptions`, `options`, `isConfigured`, `inspectSource` y `applyFixesToSource`, evaluando reglas canónicas (`modernize-use-nullptr`, `readability-braces-around-statements`, `bugprone-*`, `performance-*`) y aplicando transformaciones deterministas en memoria.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.cpp && grep -E "loadConfiguration|inspectSource|applyFixesToSource|validateOptions" apps/vigilio-browser/src/core/ClangTidyService.cpp`

- [ ] TASK-004: Implementar la inspección en modo dry-run y corrección física de archivos con soporte de copias de seguridad (.bak) y rollback en `apps/vigilio-browser/src/core/ClangTidyService.cpp`
  - *Criterio de verificación*: `inspectFile` evalúa archivos fuente sin modificarlos; `applyFixesToFile` crea una copia `.bak` atómica si `options.createBackup` está habilitado, escribe el contenido corregido y purga el respaldo tras el éxito, o ejecuta `rollbackFile` en caso de anomalía de E/S.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.cpp && grep -E "inspectFile|applyFixesToFile|createBackup|rollbackFile|removeBackup" apps/vigilio-browser/src/core/ClangTidyService.cpp`

- [ ] TASK-005: Implementar la resiliencia ante bloqueos de archivos mediante reintentos con retroceso exponencial (`applyFixesWithRetry`) en `apps/vigilio-browser/src/core/ClangTidyService.cpp`
  - *Criterio de verificación*: `applyFixesWithRetry` implementa el bucle de reintento determinista con espera exponencial ($50\text{ ms} \times 2^i$, hasta 3 reintentos) y comprobación periódica de `CancellationToken*`, devolviendo `TidyExecutionStatus::Cancelled` o `TidyExecutionStatus::FileLocked` si se agotan los intentos.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.cpp && grep -E "applyFixesWithRetry|retryBackoffMs|sleep_for|isCancelled" apps/vigilio-browser/src/core/ClangTidyService.cpp`

- [ ] TASK-006: Implementar el escaneo por lotes de directorios con filtrado de exclusiones, límites de profundidad y cancelación cooperativa multihilo en `apps/vigilio-browser/src/core/ClangTidyService.cpp`
  - *Criterio de verificación*: `inspectDirectory` y `applyFixesToDirectory` recorren recursivamente hasta `maxScanDepth = 32`, filtran archivos con `ClangTidyConfiguration::isSupportedSourceExtension` e ignoran carpetas con `ClangTidyConfiguration::isExcludedDirectory`, comprobando `isCancelled()` en cada archivo y acumulando métricas en `TidyOperationResult`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.cpp && grep -E "inspectDirectory|applyFixesToDirectory|isSupportedSourceExtension|isExcludedDirectory|maxScanDepth" apps/vigilio-browser/src/core/ClangTidyService.cpp`

- [ ] TASK-007: Implementar la síntesis determinista de argumentos de línea de comandos CLI y funciones de copy público en español neutro en `apps/vigilio-browser/src/core/ClangTidyService.cpp`
  - *Criterio de verificación*: `buildClangTidyCommandLine` genera argumentos CLI canónicos con `-p`, `--checks`, `-fix` y `-warnings-as-errors`; `executionStatusToString` y `generateRemediationHint` mapean todos los valores de `TidyExecutionStatus` a textos accesibles en español neutro conforme a `docs/UX-COPY-PUBLICO.md` sin jerga técnica interna.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyService.cpp && grep -E "buildClangTidyCommandLine|executionStatusToString|generateRemediationHint" apps/vigilio-browser/src/core/ClangTidyService.cpp`

- [ ] TASK-008: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-TIDY-SRV-001` a `TEST-TIDY-SRV-012`), cubriendo inicialización, inspección en memoria, correcciones automáticas en memoria, inspección dry-run en disco, correcciones físicas, copias de seguridad y rollback, reintentos con backoff, cancelación cooperativa con token, recorrido de directorios con filtros, control de cuotas (> 10 MB) y rutas inseguras (`..`), argumentos CLI y auditoría de textos en español.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp && grep -E "test_tidy_service_initialization|test_tidy_service_in_memory|test_tidy_service_dry_run|test_tidy_service_cancellation" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-009: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_clang_tidy_service_tests` y registrar `clang_tidy_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/ClangTidyService.cpp` en `vigilio_core`, define `add_executable(vigilio_clang_tidy_service_tests tests/unit/ClangTidyServiceTests.cpp)` y registra `add_test(NAME clang_tidy_service_tests COMMAND vigilio_clang_tidy_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "clang_tidy_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-010: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (19/19 tests) en menos de 3 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-011: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0136.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0136-module.foundations.task0046.propose.lock.json --project-root .`

- [ ] TASK-012: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `implementation` y se mantiene alineado con los contratos y el arnés de servicio acordados.
  - *Comando*: `grep -E "module.foundations.task0046" docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md`
