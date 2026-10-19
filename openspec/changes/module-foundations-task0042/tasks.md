# Tasks — module-foundations-task0042

Status: PROPOSED

- [ ] TASK-001: Extender el modelo de dominio con las enumeraciones fuertemente tipadas y estructuras inmutables operativas en `apps/vigilio-browser/src/core/ClangFormatTypes.h`
  - *Criterio de verificación*: `ClangFormatTypes.h` define `enum class FormatExecutionStatus : uint8_t`, `struct FormatOperationResult`, `struct FormatServiceOptions`, y el token de cancelación cooperativa `struct CancellationToken` protegido por macro guard, compilando limpiamente bajo C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatTypes.h && grep -E "enum class FormatExecutionStatus|struct FormatOperationResult|struct FormatServiceOptions|struct CancellationToken" apps/vigilio-browser/src/core/ClangFormatTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/ClangFormatService.h`
  - *Criterio de verificación*: `ClangFormatService.h` declara la clase `ClangFormatService` en el namespace `vigilio` con métodos de configuración (`loadConfiguration`, `setOptions`, `options`, `isConfigured`), formateo e inspección en memoria (`formatSourceCode`, `inspectSource`), inspección y formateo en disco (`inspectFile`, `inspectDirectory`, `formatFile`, `formatWithRetry`, `formatDirectory`), gestión de respaldo (`createBackup`, `rollbackFile`, `removeBackup`), síntesis CLI (`buildClangFormatCommandLine`) y diagnósticos (`executionStatusToString`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.h && grep -E "class ClangFormatService|formatSourceCode|inspectFile|formatWithRetry|createBackup|rollbackFile|buildClangFormatCommandLine" apps/vigilio-browser/src/core/ClangFormatService.h`

- [ ] TASK-003: Implementar la inicialización, configuración, inspección y formateo en memoria en `apps/vigilio-browser/src/core/ClangFormatService.cpp`
  - *Criterio de verificación*: `ClangFormatService.cpp` implementa `loadConfiguration`, `setOptions`, `options`, `isConfigured`, `formatSourceCode` e `inspectSource`, realizando transformaciones de texto deterministas y validando opciones con `ClangFormatValidator`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.cpp && grep -E "loadConfiguration|formatSourceCode|inspectSource|validateOptions" apps/vigilio-browser/src/core/ClangFormatService.cpp`

- [ ] TASK-004: Implementar la inspección en modo dry-run y formateo físico de archivos con soporte de copias de seguridad (.bak) y rollback en `apps/vigilio-browser/src/core/ClangFormatService.cpp`
  - *Criterio de verificación*: `inspectFile` evalúa archivos fuente sin modificarlos; `formatFile` crea una copia `.bak` atómica si `options.createBackup` está habilitado, escribe el contenido formateado y purga el respaldo tras el éxito, o ejecuta `rollbackFile` en caso de error.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.cpp && grep -E "inspectFile|formatFile|createBackup|rollbackFile|removeBackup" apps/vigilio-browser/src/core/ClangFormatService.cpp`

- [ ] TASK-005: Implementar la resiliencia ante bloqueos de archivos mediante reintentos con retroceso exponencial (`formatWithRetry`) en `apps/vigilio-browser/src/core/ClangFormatService.cpp`
  - *Criterio de verificación*: `formatWithRetry` implementa el bucle de reintento determinista con espera exponencial ($50\text{ ms} \times 2^i$) y comprobación periódica de `CancellationToken*`, devolviendo `FormatExecutionStatus::Cancelled` o `FormatExecutionStatus::FileLocked` si se agotan los reintentos.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.cpp && grep -E "formatWithRetry|retryBackoffMs|sleep_for|isCancelled" apps/vigilio-browser/src/core/ClangFormatService.cpp`

- [ ] TASK-006: Implementar el escaneo por lotes de directorios con filtrado de exclusiones, límites de profundidad y cancelación cooperativa multihilo en `apps/vigilio-browser/src/core/ClangFormatService.cpp`
  - *Criterio de verificación*: `inspectDirectory` y `formatDirectory` recorren recursivamente hasta `maxScanDepth = 32`, filtran archivos con `ClangFormatConfiguration::isSupportedSourceExtension` e ignoran carpetas con `ClangFormatConfiguration::isExcludedDirectory`, comprobando `isCancelled()` en cada archivo y acumulando métricas en `FormatOperationResult`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.cpp && grep -E "inspectDirectory|formatDirectory|isSupportedSourceExtension|isExcludedDirectory|maxScanDepth" apps/vigilio-browser/src/core/ClangFormatService.cpp`

- [ ] TASK-007: Implementar la síntesis determinista de argumentos de línea de comandos CLI y funciones de copy público en español neutro en `apps/vigilio-browser/src/core/ClangFormatService.cpp`
  - *Criterio de verificación*: `buildClangFormatCommandLine` genera cadenas CLI canónicas con `--dry-run`, `-i` y `--Werror`; `executionStatusToString` mapea todos los valores de `FormatExecutionStatus` a textos accesibles en español neutro conforme a `docs/UX-COPY-PUBLICO.md` sin jerga técnica interna.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatService.cpp && grep -E "buildClangFormatCommandLine|executionStatusToString|remediationHint" apps/vigilio-browser/src/core/ClangFormatService.cpp`

- [ ] TASK-008: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-FORMAT-SRV-001` a `TEST-FORMAT-SRV-012`), cubriendo inicialización, formateo en memoria, inspección dry-run limpia, detección de discrepancias, formateo físico, backups y rollback, reintentos con backoff, cancelación cooperativa con token, control de cuotas (> 10 MB), seguridad de rutas (`..`), argumentos CLI y auditoría de textos en español.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp && grep -E "test_format_service_initialization|test_format_service_in_memory|test_format_service_dry_run|test_format_service_cancellation" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-009: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_clang_format_service_tests` y registrar `clang_format_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/ClangFormatService.cpp` en `vigilio_core`, define `add_executable(vigilio_clang_format_service_tests tests/unit/ClangFormatServiceTests.cpp)` y registra `add_test(NAME clang_format_service_tests COMMAND vigilio_clang_format_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "clang_format_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-010: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (17/17 tests) en menos de 2 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-011: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las 42 referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0124.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0124-module.foundations.task0042.propose.lock.json --project-root .`

- [ ] TASK-012: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0042-clang-format-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `implementation` y se mantiene alineado con los contratos y el arnés de servicio acordados.
  - *Comando*: `grep -E "module.foundations.task0042" docs/rules/01-modules/foundations/micro/0042-clang-format-configuration/task.md`
