# Tasks — module-foundations-task0044

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos de frontera y límites numéricos de configuración en `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangFormatConfigurationTests.cpp` incluye la función de prueba `test_clang_format_validator_boundary_values` que evalúa exhaustivamente los valores numéricos permitidos en sus límites exactos (`indentWidth` = 1 y 16, `columnLimit` = 20 y 500) y rechaza valores fuera de rango (0, 17, 19, 501, negativos), comprobando que se emiten los códigos de error tipados `ClangFormatError::InvalidIndentWidth` y `ClangFormatError::InvalidColumnLimit`.
  - *Comando*: `grep -E "test_clang_format_validator_boundary_values" apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`

- [ ] TASK-002: Implementar pruebas de robustez ante documentos YAML malformados, truncados o corruptos en `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangFormatConfigurationTests.cpp` incluye la función de prueba `test_clang_format_malformed_yaml_inputs` que valida la respuesta segura de `ClangFormatValidator::validateConfigContent` ante documentos YAML vacíos, truncados a mitad de clave, con caracteres nulos o binarios arbitrarios, retornando `isValid == false` y `error == ClangFormatError::ParseError` sin excepciones no capturadas.
  - *Comando*: `grep -E "test_clang_format_malformed_yaml_inputs" apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`

- [ ] TASK-003: Implementar pruebas de rechazo de ataques de Directory Traversal complejos en `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangFormatConfigurationTests.cpp` incluye la función de prueba `test_clang_format_complex_path_traversal` que comprueba que `ClangFormatValidator::isSafeConfigPath` detecta y rechaza vectores avanzados de escape de directorio (`./../..`, `foo/../../bar`, `..\\windows\\style`, `/etc/shadow`, `/usr/bin`), retornando `false` y garantizando contención estricta.
  - *Comando*: `grep -E "test_clang_format_complex_path_traversal" apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`

- [ ] TASK-004: Implementar pruebas de serialización roundtrip con políticas no estándar en `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangFormatConfigurationTests.cpp` incluye la función de prueba `test_clang_format_roundtrip_variants` que evalúa combinaciones no predeterminadas de `PointerAlignmentPolicy` (Right, Middle) y `ShortFunctionPolicy` (Inline, Empty, All), certificando la preservación bidireccional exacta de todos los campos serializados y deserializados.
  - *Comando*: `grep -E "test_clang_format_roundtrip_variants" apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`

- [ ] TASK-005: Implementar pruebas de formateo en memoria pura para múltiples dialectos C++ en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_in_memory_dialects` que evalúa `formatSourceCode` e `inspectSource` sobre fragmentos de código con extensiones `.cpp`, `.h`, `.hpp`, `.cc` y `.cxx`, validando la corrección de indentación (4 espacios), alineación de punteros (`*` a la izquierda) y prohibición de funciones cortas en una sola línea.
  - *Comando*: `grep -E "test_format_service_in_memory_dialects" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de inspección dry-run recursiva de directorios multinivel anidados con exclusión de carpetas en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_recursive_directory_tree` que construye una jerarquía de directorios sintéticos con subdirectorios anidados (`a/b/c/`), archivos conformes, archivos no conformes, archivos no C++ (`.txt`, `.json`) y carpetas excluidas (`_build`, `.git`), comprobando que `inspectDirectory` reporta el número exacto de archivos examinados y discrepancias sin mutar los archivos en disco.
  - *Comando*: `grep -E "test_format_service_recursive_directory_tree" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de formateo físico con respaldo atómico (`.bak`) y rollback determinista en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_backup_atomic_rollback` que evalúa el ciclo de vida completo de respaldo (`createBackup`), mutación destructiva deliberada, restauración exacta (`rollbackFile`) y comprobación de idempotencia ante llamadas redundantes a `rollbackFile` o invocaciones sobre archivos sin copia previa.
  - *Comando*: `grep -E "test_format_service_backup_atomic_rollback" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de concurrencia masiva multihilo (16 hilos) sobre formateo e inspección en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_massive_multithreading_concurrency` que ejecuta al menos 16 hilos (`std::thread`) concurrentes realizando simultáneamente operaciones de formateo en memoria pura e inspección dry-run sobre una instancia compartida de `ClangFormatService`, comprobando ausencia total de carreras de datos (data races), contención sincronizada por `std::mutex` y finalización exitosa de todos los workers.
  - *Comando*: `grep -E "test_format_service_massive_multithreading_concurrency" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de estrés de cancelación cooperativa multihilo y política Zero-Zombie en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_cooperative_cancellation_stress` que evalúa la activación concurrente de `CancellationToken` desde un hilo secundario durante el escaneo masivo de árboles de directorios, verificando el retorno inmediato con `FormatExecutionStatus::Cancelled`, sin procesos zombis ni fugas de descriptores de archivo.
  - *Comando*: `grep -E "test_format_service_cooperative_cancellation_stress" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de recuperación ante bloqueos transitorios y reintentos con backoff exponencial (`formatWithRetry`) en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_transient_retry_backoff` que simula contención transitoria sobre un archivo de código y verifica que `formatWithRetry` reintenta la operación según los parámetros configurados (`maxRetries = 3`, `retryBackoffMs = 20`), concluyendo con `FormatExecutionStatus::Success` una vez superada la contención.
  - *Comando*: `grep -E "test_format_service_transient_retry_backoff" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-011: Implementar pruebas de inyección de fallos de permisos de archivo y contención en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_permission_failure_injection` que remueve los permisos de escritura de un archivo temporal (`std::filesystem::perms::owner_read`), invoca `formatFile` y comprueba que se retorna limpiamente `FormatExecutionStatus::PermissionDenied` o `WriteError`, preservando íntegro el archivo original sin corrupción de datos.
  - *Comando*: `grep -E "test_format_service_permission_failure_injection" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-012: Implementar pruebas de límites de cuota de archivo (`maxFileSizeBytes`) y rechazo limpio `FileTooLarge` en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` incluye la función de prueba `test_format_service_quota_limit_enforcement` que valida el comportamiento estricto ante archivos que superan la cuota de seguridad (e.g. archivo de 2 KB con cuota de 512 bytes), comprobando que se retorna `FormatExecutionStatus::FileTooLarge` con el mensaje de remediación accesible correspondiente y sin saturación de memoria.
  - *Comando*: `grep -E "test_format_service_quota_limit_enforcement" apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`

- [ ] TASK-013: Auditar y validar cumplimiento estricto de textos públicos y UX copy en español conforme a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp`
  - *Criterio de verificación*: `ClangFormatServiceTests.cpp` audita programáticamente que la totalidad de los 12 estados de `FormatExecutionStatus` devueltos por `executionStatusToString` emplean lenguaje natural en español neutro y no contienen términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/ClangFormat*.cpp || echo "AUDIT_CLEAN"`

- [ ] TASK-014: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de suites de prueba en CTest aprobadas exitosamente (17/17 tests en < 2s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-015: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0130.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0130-module.foundations.task0044.propose.lock.json --project-root .`

- [ ] TASK-016: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0044-clang-format-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de clang-format configuration.
  - *Comando*: `grep -E "module.foundations.task0044" docs/rules/01-modules/foundations/micro/0044-clang-format-configuration/task.md`
