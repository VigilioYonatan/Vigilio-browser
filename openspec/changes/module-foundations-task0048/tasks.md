# Tasks — module-foundations-task0048

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos de frontera y límites de expresiones regulares de cabeceras en `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangTidyConfigurationTests.cpp` incluye la función de prueba `test_clang_tidy_validator_regex_boundaries` que evalúa exhaustivamente expresiones regulares válidas complejas (`^src/(core|storage)/.*\\.h$`, regex vacío) y rechaza expresiones sintácticamente inválidas o desbalanceadas (`[a-z(`, `(?+invalid`, `*invalid`), comprobando que se emite el código de error tipado `ClangTidyError::InvalidHeaderFilterRegex`.
  - *Comando*: `grep -E "test_clang_tidy_validator_regex_boundaries" apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`

- [ ] TASK-002: Implementar pruebas de robustez ante documentos YAML malformados, truncados o corruptos en `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangTidyConfigurationTests.cpp` incluye la función de prueba `test_clang_tidy_malformed_yaml_inputs` que valida la respuesta segura de `ClangTidyValidator::validateConfigContent` ante documentos YAML vacíos, truncados a mitad de clave, con caracteres nulos o binarios arbitrarios, retornando `isValid == false` y `error == ClangTidyError::ParseError` sin excepciones no capturadas.
  - *Comando*: `grep -E "test_clang_tidy_malformed_yaml_inputs" apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`

- [ ] TASK-003: Implementar pruebas de rechazo de ataques de Directory Traversal complejos y caracteres nulos en `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangTidyConfigurationTests.cpp` incluye la función de prueba `test_clang_tidy_complex_path_traversal` que comprueba que `ClangTidyValidator::isSafeConfigPath` detecta y rechaza vectores avanzados de escape de directorio (`./../..`, `foo/../../bar`, `..\\windows\\style`, `/etc/shadow`, `/usr/bin/clang-tidy`, `path\0with_null.yaml`), retornando `false` y garantizando contención estricta.
  - *Comando*: `grep -E "test_clang_tidy_complex_path_traversal" apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`

- [ ] TASK-004: Implementar pruebas de serialización roundtrip con variantes avanzadas de comprobaciones y opciones en `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangTidyConfigurationTests.cpp` incluye la función de prueba `test_clang_tidy_roundtrip_variants` que evalúa combinaciones no predeterminadas de checks (`+cert-*`, `-google-*`, `readability-*`), `WarningsAsErrors: '*'` y `FormatStyle: 'file'`, certificando la preservación bidireccional exacta de todos los campos serializados y deserializados.
  - *Comando*: `grep -E "test_clang_tidy_roundtrip_variants" apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`

- [ ] TASK-005: Implementar pruebas de concurrencia masiva multihilo (16 hilos) sobre inspección y corrección en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` incluye la función de prueba `test_tidy_service_massive_multithreading_concurrency` que ejecuta al menos 16 hilos (`std::thread`) concurrentes realizando simultáneamente operaciones de inspección en memoria pura, autofix en memoria e inspección dry-run en disco sobre una instancia compartida de `ClangTidyService`, comprobando ausencia total de carreras de datos (data races), contención sincronizada por `std::mutex` y finalización exitosa de todos los workers.
  - *Comando*: `grep -E "test_tidy_service_massive_multithreading_concurrency" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de estrés de cancelación cooperativa asíncrona multihilo y política Zero-Zombie en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` incluye la función de prueba `test_tidy_service_cooperative_cancellation_stress` que evalúa la activación concurrente de `CancellationToken` desde un hilo secundario durante el escaneo masivo de árboles de directorios, verificando el retorno inmediato con `TidyExecutionStatus::Cancelled`, sin procesos zombis ni fugas de descriptores de archivo.
  - *Comando*: `grep -E "test_tidy_service_cooperative_cancellation_stress" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de inyección de fallos de permisos de archivo y reversión atómica en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` incluye la función de prueba `test_tidy_service_permission_failure_injection` que remueve los permisos de escritura de un archivo temporal (`std::filesystem::perms::owner_read`), invoca `applyFixesToFile` y comprueba que se retorna limpiamente `TidyExecutionStatus::PermissionDenied` o `WriteError`, preservando íntegro el archivo original sin corrupción de datos.
  - *Comando*: `grep -E "test_tidy_service_permission_failure_injection" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de recorrido recursivo en jerarquía multinivel con exclusión de carpetas en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` incluye la función de prueba `test_tidy_service_recursive_directory_tree` que construye una jerarquía de directorios sintéticos con subdirectorios anidados (`a/b/c/d/`), archivos conformes, archivos no conformes, archivos no C++ (`.txt`, `.json`, `.png`) y carpetas excluidas (`_build`, `.git`, `generated`, `assets`), comprobando que `inspectDirectory` reporta el número exacto de archivos examinados e incidencias sin mutar los archivos en disco.
  - *Comando*: `grep -E "test_tidy_service_recursive_directory_tree" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de idempotencia de rollback y saneamiento seguro de copias de respaldo en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` incluye la función de prueba `test_tidy_service_backup_atomic_idempotence` que evalúa llamadas sucesivas a `rollbackFile()`, rollback sobre archivos sin backup previo y confirmación de eliminación física mediante `removeBackup()`, garantizando comportamiento seguro y predecible.
  - *Comando*: `grep -E "test_tidy_service_backup_atomic_idempotence" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de recuperación ante bloqueos transitorios y reintentos con backoff exponencial (`applyFixesWithRetry`) en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` amplía la cobertura de `applyFixesWithRetry` evaluando contención transitoria simulada y verificando que concluye con `TidyExecutionStatus::Success` una vez superada la contención, o `FileLocked` si excede `maxRetries`.
  - *Comando*: `grep -E "test_tidy_service_retry_backoff" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-011: Implementar pruebas de síntesis CLI con rutas complejas y opciones avanzadas en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` verifica la generación de líneas de comando de Clang-Tidy con rutas que contienen espacios, parámetros relativos y absolutos, y banderas `-fix` y `-warnings-as-errors`.
  - *Comando*: `grep -E "test_tidy_service_cli_synthesis" apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`

- [ ] TASK-012: Auditar y validar cumplimiento estricto de textos públicos y UX copy en español conforme a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`
  - *Criterio de verificación*: `ClangTidyServiceTests.cpp` audita programáticamente que la totalidad de los 12 estados de `TidyExecutionStatus` y las pistas de remediación devueltas por `executionStatusToString` y `generateRemediationHint` emplean lenguaje natural en español neutro y no contienen términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/ClangTidy*.cpp || echo "AUDIT_CLEAN"`

- [ ] TASK-013: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de suites de prueba en CTest aprobadas exitosamente (19/19 tests en < 3s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-014: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0142.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0142-module.foundations.task0048.propose.lock.json --project-root .`

- [ ] TASK-015: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0048-clang-tidy-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de clang-tidy configuration.
  - *Comando*: `grep -E "module.foundations.task0048" docs/rules/01-modules/foundations/micro/0048-clang-tidy-configuration/task.md`
