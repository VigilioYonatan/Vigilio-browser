# Tasks — module-foundations-task0038

Status: PROPOSED

- [ ] TASK-001: Extender el modelo de dominio con las enumeraciones fuertemente tipadas y estructuras inmutables operativas en `apps/vigilio-browser/src/core/TestFixtureTypes.h`
  - *Criterio de verificación*: `TestFixtureTypes.h` define `enum class FixtureOperationStatus`, `struct FixtureOperationResult`, `struct OrphanScanReport` y `struct BrowserProfileSeedOptions`, compilando limpiamente bajo C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureTypes.h && grep -E "enum class FixtureOperationStatus|struct FixtureOperationResult|struct OrphanScanReport|struct BrowserProfileSeedOptions" apps/vigilio-browser/src/core/TestFixtureTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/TestFixtureService.h`
  - *Criterio de verificación*: `TestFixtureService.h` declara la clase `TestFixtureService` en el namespace `vigilio` con métodos de gestión de fixtures (`createManagedFixture`, `releaseFixture`, `releaseAllFixtures`, `findFixture`), resiliencia (`cleanWithRetry`), huérfanos (`scanAndPurgeOrphanFixtures`), instantáneas (`createSnapshot`, `rollbackToSnapshot`, `removeSnapshot`), y sembrado (`seedBrowserProfile`, `seedMockDatabase`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.h && grep -E "class TestFixtureService|createManagedFixture|cleanWithRetry|scanAndPurgeOrphanFixtures|createSnapshot|rollbackToSnapshot" apps/vigilio-browser/src/core/TestFixtureService.h`

- [ ] TASK-003: Implementar la orquestación y el registro centralizado de ciclo de vida de fixtures en `apps/vigilio-browser/src/core/TestFixtureService.cpp`
  - *Criterio de verificación*: `TestFixtureService.cpp` implementa `createManagedFixture`, `releaseFixture`, `releaseAllFixtures`, `findFixture` y `activeFixtureCount` con protección de memoria y mapeo asociativo de punteros únicos.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.cpp && grep -E "createManagedFixture|releaseFixture|releaseAllFixtures|findFixture" apps/vigilio-browser/src/core/TestFixtureService.cpp`

- [ ] TASK-004: Implementar la cancelación cooperativa y el protocolo de reintentos ante bloqueos de archivos en `apps/vigilio-browser/src/core/TestFixtureService.cpp`
  - *Criterio de verificación*: `cleanWithRetry` implementa el bucle de reintentos con retroceso exponencial (*exponential backoff*) y comprobación continua de `CancellationToken*`, devolviendo `FixtureOperationStatus::Cancelled` o `FixtureOperationStatus::LockContention` según corresponda.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.cpp && grep -E "cleanWithRetry|exponential|sleep_for|isCancelled" apps/vigilio-browser/src/core/TestFixtureService.cpp`

- [ ] TASK-005: Implementar la detección y purga segura de fixtures huérfanos con validación canónica en `apps/vigilio-browser/src/core/TestFixtureService.cpp`
  - *Criterio de verificación*: `scanAndPurgeOrphanFixtures` filtra directorios `vigilio_test_*` en `temp_directory_path()`, valida cada uno con `TestFixtureValidator::isSafeTestDirectory`, respeta `CancellationToken` y genera un `OrphanScanReport` con el balance de carpetas y bytes recuperados.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.cpp && grep -E "scanAndPurgeOrphanFixtures|isSafeTestDirectory|OrphanScanReport" apps/vigilio-browser/src/core/TestFixtureService.cpp`

- [ ] TASK-006: Implementar puntos de control, instantáneas y rollback atómico en `apps/vigilio-browser/src/core/TestFixtureService.cpp`
  - *Criterio de verificación*: `createSnapshot` clona el árbol del fixture en un subdirectorio aislado `_snapshots/<id>`, y `rollbackToSnapshot` restaura el contenido a su estado original purgando mutaciones posteriores, respetando la cuota máxima de disco asignada.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.cpp && grep -E "createSnapshot|rollbackToSnapshot|removeSnapshot|_snapshots" apps/vigilio-browser/src/core/TestFixtureService.cpp`

- [ ] TASK-007: Implementar el sembrado sintético de perfil de navegador y base de datos SQLite en `apps/vigilio-browser/src/core/TestFixtureService.cpp`
  - *Criterio de verificación*: `seedBrowserProfile` genera la estructura `profiles/<name>`, `preferences.json` válido, y `seedMockDatabase` escribe una cabecera SQLite sintética con soporte de journaling WAL, cumpliendo estrictamente con `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureService.cpp && grep -E "seedBrowserProfile|seedMockDatabase|preferences.json" apps/vigilio-browser/src/core/TestFixtureService.cpp`

- [ ] TASK-008: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`
  - *Criterio de verificación*: `TestFixtureServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-FIXTURE-SRV-001` a `TEST-FIXTURE-SRV-012`), cubriendo registro, liberación en lote, cancelación, huérfanos, reintentos, snapshots, rollback, sembrado de perfiles, bases de datos SQLite, cuotas, concurrencia y auditoría de textos en español.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp && grep -E "test_fixture_service_lifecycle|test_fixture_service_cancellation|test_fixture_service_orphan_purge|test_fixture_service_snapshots" apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`

- [ ] TASK-009: Integrar los nuevos fuentes en `vigilio_core`, configurar el ejecutable `vigilio_fixture_service_tests` y registrar la prueba en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/TestFixtureService.cpp` en `vigilio_core`, define `add_executable(vigilio_fixture_service_tests tests/unit/TestFixtureServiceTests.cpp)` y registra `add_test(NAME test_fixture_service_tests COMMAND vigilio_fixture_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "test_fixture_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-010: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (15/15 tests) en menos de 1 segundo.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-011: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0112.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0112-module.foundations.task0038.propose.lock.json --project-root .`

- [ ] TASK-012: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0038-test-fixture-foundation/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `implementation` y se mantiene alineado con los contratos y el arnés de servicio acordados.
  - *Comando*: `grep -E "module.foundations.task0038" docs/rules/01-modules/foundations/micro/0038-test-fixture-foundation/task.md`
