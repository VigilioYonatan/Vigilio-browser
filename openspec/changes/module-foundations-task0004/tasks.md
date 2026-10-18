# Tasks — module-foundations-task0004

Status: PROPOSED

- [ ] TASK-001: Refinar y robustecer la suite de pruebas unitarias de targets de núcleo (`vigilio_core_tests`) en `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - *Criterio de verificación*: Las pruebas unitarias cubren casos positivos de normalización de URLs y consultas, casos negativos de rechazo de esquemas maliciosos (`javascript:`, `file:///`, credenciales en URL), operaciones sobre `TabModel` (fijar, duplicar, cerrar otros) y serialización/deserialización de sesiones con limpieza determinista de archivos temporales.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R core_units`

- [ ] TASK-002: Refinar y robustecer la suite de pruebas de persistencia e integración (`vigilio_storage_tests`) en `apps/vigilio-browser/tests/integration/StorageTests.cpp`
  - *Criterio de verificación*: Las pruebas de integración validan la creación de esquema SQLite, inserción y actualización idempotente de marcadores, resistencia a inyecciones SQL en títulos, registro de visitas en historial, reapertura de base de datos con persistencia verificada y eliminación determinista de artefactos temporales (`.sqlite`, `-wal`, `-shm`).
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R storage_integration`

- [ ] TASK-003: Formalizar y auditar en `apps/vigilio-browser/CMakeLists.txt` la gobernanza de pruebas de targets (etiquetas CTest, timeouts y serialización)
  - *Criterio de verificación*: Los targets de prueba en `CMakeLists.txt` tienen asignadas etiquetas taxonómicas (`unit;fast`, `integration;fast`, `ui;fast`, `e2e;heavy`), timeouts estrictos (15s, 15s, 20s, 60s) y propiedades de serialización (`RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`), garantizando compilación y paso limpio de pruebas en modo headless (`VIGILIO_BUILD_GUI=OFF`).
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-004: Ejecutar la validación automatizada completa mediante el script de comprobación local del proyecto
  - *Criterio de verificación*: Compilación limpia con Ninja en modo headless, paso del 100% de las pruebas rápidas de CTest (`core_units` y `storage_integration`) y auditoría de formato con ClangFormat con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-005: Sincronizar la documentación canónica de la microtarea y validar referencias
  - *Criterio de verificación*: El archivo `docs/rules/01-modules/foundations/micro/0004-cmake-targets/task.md` actualiza su estado de ejecución y pasa la validación canónica de referencias de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
