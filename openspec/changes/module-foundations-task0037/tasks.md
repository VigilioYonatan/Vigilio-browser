# Tasks — module-foundations-task0037

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos, enumeraciones fuertemente tipadas y estructuras inmutables del dominio en `apps/vigilio-browser/src/core/TestFixtureTypes.h`
  - *Criterio de verificación*: `TestFixtureTypes.h` declara formalmente las enumeraciones `enum class FixtureScope`, `enum class FixtureCleanPolicy`, `enum class TestEnvironmentState`, `enum class TempDirectoryMode`, `enum class FixtureLifecycleEvent`, `enum class FixtureError`, y las estructuras inmutables `TestEnvironmentConfig`, `TestFixtureReport` y `EnvironmentVariableOverride`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureTypes.h && grep -E "enum class FixtureScope|enum class TestEnvironmentState|struct TestEnvironmentConfig|struct TestFixtureReport" apps/vigilio-browser/src/core/TestFixtureTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del validador de seguridad de rutas e higiene en `apps/vigilio-browser/src/core/TestFixtureValidator.h`
  - *Criterio de verificación*: `TestFixtureValidator.h` declara la clase `TestFixtureValidator` en el namespace `vigilio` con los métodos estáticos `isSafeTestDirectory`, `validateConfig`, `isWithinTempRoot`, `fixtureStateToString` y `fixtureErrorToString`, sin incluir dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureValidator.h && grep -E "class TestFixtureValidator|isSafeTestDirectory|validateConfig|fixtureStateToString" apps/vigilio-browser/src/core/TestFixtureValidator.h`

- [ ] TASK-003: Implementar el motor estático de contención de rutas temporales, validación de cuotas y copy público en `apps/vigilio-browser/src/core/TestFixtureValidator.cpp`
  - *Criterio de verificación*: `TestFixtureValidator.cpp` implementa la verificación estricta de que la ruta resida dentro de `temp_directory_path()`, comience con `vigilio_test_`, no contenga `..`, no sea idéntica a `/tmp`, y genera mensajes legibles en español cumpliendo con `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureValidator.cpp && grep -E "isSafeTestDirectory|validateConfig|fixtureStateToString|fixtureErrorToString" apps/vigilio-browser/src/core/TestFixtureValidator.cpp`

- [ ] TASK-004: Declarar las clases administradoras del ciclo de vida RAII y el guardián de entorno en `apps/vigilio-browser/src/core/TestFixtureFoundation.h`
  - *Criterio de verificación*: `TestFixtureFoundation.h` declara `TestFixtureFoundation` con constructor/destructor RAII, métodos de gestión de archivos y bases de datos (`createSubdirectory`, `createTestFile`, `cleanupSqliteWalShm`, `checkQuota`, `clean`), y declara `TestEnvironmentGuard` para manejo seguro de variables de entorno.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureFoundation.h && grep -E "class TestFixtureFoundation|class TestEnvironmentGuard|cleanupSqliteWalShm" apps/vigilio-browser/src/core/TestFixtureFoundation.h`

- [ ] TASK-005: Implementar la gestión de directorios temporales libres de colisión, creación de archivos sintéticos, purga de SQLite y rollback de entorno en `apps/vigilio-browser/src/core/TestFixtureFoundation.cpp`
  - *Criterio de verificación*: `TestFixtureFoundation.cpp` genera rutas atómicas únicas (PID + nanosegundos + contador atómico), implementa la creación y eliminación segura de carpetas, cálculo de tamaño acumulado, límite de cuota (50 MB), purga de archivos `-wal` y `-shm`, y restauración de variables de entorno en orden LIFO.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TestFixtureFoundation.cpp && grep -E "cleanupSqliteWalShm|calculateTotalSizeBytes|restoreAll" apps/vigilio-browser/src/core/TestFixtureFoundation.cpp`

- [ ] TASK-006: Implementar la batería exhaustiva de 10 pruebas unitarias de especificación en `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`
  - *Criterio de verificación*: `TestFixtureFoundationTests.cpp` implementa las 10 pruebas trazables (`TEST-FIXTURE-001` a `TEST-FIXTURE-010`) que verifican configuración canónica, ciclo de vida RAII, rechazo de rutas no seguras, subcarpetas, cuota de disco, limpieza SQLite, guardián de variables de entorno, concurrencia, resiliencia ante errores de permisos y copy público.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp && grep -E "test_fixture_canonical_config|test_fixture_lifecycle_raii|test_fixture_unsafe_path_rejection" apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`

- [ ] TASK-007: Integrar los fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_fixture_tests` y registrar `test_fixture_foundation_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/TestFixtureValidator.cpp` y `src/core/TestFixtureFoundation.cpp` en la librería `vigilio_core`, define `add_executable(vigilio_fixture_tests tests/unit/TestFixtureFoundationTests.cpp)` y registra `add_test(NAME test_fixture_foundation_tests COMMAND vigilio_fixture_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "test_fixture_foundation_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-008: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente en menos de 1 segundo.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-009: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0109.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0109-module.foundations.task0037.propose.lock.json --project-root .`

- [ ] TASK-010: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0037-test-fixture-foundation/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `domain` y se mantiene alineado con los contratos y el modelo de dominio acordados.
  - *Comando*: `grep -E "module.foundations.task0037" docs/rules/01-modules/foundations/micro/0037-test-fixture-foundation/task.md`
