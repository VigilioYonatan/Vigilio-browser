# Tasks — module-foundations-task0040

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos negativos extremos y path traversal profundo en `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`
  - *Criterio de verificación*: `TestFixtureFoundationTests.cpp` incluye la función de prueba `test_fixture_deep_path_traversal_rejection` que evalúa secuencias de directory traversal complejas (`a/../../b`, `..\\escape`, `../../../etc/shadow`), rutas absolutas no autorizadas y nombres con componentes vacíos, comprobando que retornan rutas vacías o error sin alterar el estado del fixture.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp && grep -E "test_fixture_deep_path_traversal_rejection" apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`

- [ ] TASK-002: Implementar pruebas de precisión de cuota en límites de frontera y transición a `QuotaExceeded` en `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`
  - *Criterio de verificación*: `TestFixtureFoundationTests.cpp` incluye la función de prueba `test_fixture_exact_boundary_quota` que evalúa la escritura en el límite exacto de la cuota de almacenamiento (escritura de 1024 bytes con éxito) e intento de escribir 1 solo byte adicional, comprobando el rechazo con `FixtureError::QuotaExceeded`, la transición a `TestEnvironmentState::Corrupted` y `checkQuota() == false`.
  - *Comando*: `grep -E "test_fixture_exact_boundary_quota" apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`

- [ ] TASK-003: Implementar pruebas de anidamiento y robustez del guardián de variables de entorno (`TestEnvironmentGuard`) en `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`
  - *Criterio de verificación*: `TestFixtureFoundationTests.cpp` incluye la función de prueba `test_fixture_nested_environment_guards` que evalúa múltiples instancias anidadas de `TestEnvironmentGuard` modificando la misma variable de entorno, comprobando la restauración exacta de los valores previos en orden LIFO y la preservación del estado inicial.
  - *Comando*: `grep -E "test_fixture_nested_environment_guards" apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`

- [ ] TASK-004: Implementar pruebas de robustez en el saneamiento de ficheros SQLite WAL/SHM (`cleanupSqliteWalShm`) en `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`
  - *Criterio de verificación*: `TestFixtureFoundationTests.cpp` incluye la función de prueba `test_fixture_sqlite_cleanup_edge_cases` que comprueba que `cleanupSqliteWalShm` maneja de forma segura ficheros que no existen, nombres con secuencias relativas y rutas que corresponden a directorios, retornando `false` de forma limpia y sin generar excepciones no capturadas.
  - *Comando*: `grep -E "test_fixture_sqlite_cleanup_edge_cases" apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`

- [ ] TASK-005: Implementar pruebas del ciclo de vida integral y persistencia SQLite sintética E2E en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_full_lifecycle_e2e` que ejecuta en una secuencia determinista: creación de fixture gestionado $\to$ sembrado de perfil sintético $\to$ sembrado de base de datos SQLite con datos simulados $\to$ creación de snapshot $\to$ mutación destructiva $\to$ rollback atómico a snapshot $\to$ verificación de consistencia $\to$ limpieza completa sin residuos en disco.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp && grep -E "test_fixture_service_full_lifecycle_e2e" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de validación de entradas inválidas en instantáneas (snapshots) y perfiles en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_invalid_snapshot_and_profile_inputs` que evalúa intentos de rollback a snapshots inexistentes (`NotFound`), creación de snapshots con nombres vacíos o que contienen caracteres de escape (`PathUnsafe`), e intento de rollback con token cancelado.
  - *Comando*: `grep -E "test_fixture_service_invalid_snapshot_and_profile_inputs" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de concurrencia masiva multihilo (16 hilos) e inmunidad a colisiones de IDs en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_massive_concurrency_stress` que ejecuta al menos 16 hilos (`std::thread`) concurrentes creando, escribiendo y liberando fixtures simultáneamente, comprobando unicidad absoluta de `fixtureId`, ausencia de carreras de datos y convergencia de `activeFixtureCount()` a 0.
  - *Comando*: `grep -E "test_fixture_service_massive_concurrency_stress" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de estrés de cancelación cooperativa multihilo en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_cancellation_token_stress` que evalúa la respuesta inmediata ante cancelaciones en todas las operaciones del servicio (`seedBrowserProfile`, `scanAndPurgeOrphanFixtures`, `cleanWithRetry`), certificando el retorno con `FixtureOperationStatus::Cancelled`.
  - *Comando*: `grep -E "test_fixture_service_cancellation_token_stress" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de recuperación ante bloqueos transitorios e inyección de fallos en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_transient_lock_and_retry_recovery` que comprueba que `cleanWithRetry` ejecuta retrocesos exponenciales ante contención de bloqueos temporales, reportando el diagnóstico de forma limpia.
  - *Comando*: `grep -E "test_fixture_service_transient_lock_and_retry_recovery" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas del escáner de huérfanos con casos de frontera y límites temporales en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` incluye la función de prueba `test_fixture_service_orphan_purge_boundary_conditions` que comprueba que `scanAndPurgeOrphanFixtures` con umbral positivo no elimina fixtures recién creados y preserva incondicionalmente cualquier directorio ajeno al prefijo `vigilio_test_` en el directorio temporal.
  - *Comando*: `grep -E "test_fixture_service_orphan_purge_boundary_conditions" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-011: Auditar y validar cumplimiento estricto de textos públicos y UX copy en español conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `TestFixtureValidator` y `TestFixtureService` son auditados para comprobar que todas las cadenas de texto devueltas por `fixtureStateToString`, `fixtureErrorToString` y `operationStatusToString` estén en español neutro y no contengan términos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/TestFixture*.cpp || echo "AUDIT_CLEAN"`

- [ ] TASK-012: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de suites de prueba en CTest aprobadas exitosamente (15/15 tests en < 1s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-013: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0118.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0118-module.foundations.task0040.propose.lock.json --project-root .`

- [ ] TASK-014: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0040-test-fixture-foundation/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de test fixture foundation.
  - *Comando*: `grep -E "module.foundations.task0040" docs/rules/01-modules/foundations/micro/0040-test-fixture-foundation/task.md`
