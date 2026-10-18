# Tasks — module-foundations-task0020

Status: PROPOSED

- [ ] TASK-001: Implementar suite de integridad física de activos y estructura de contenidos en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`
  - *Criterio de verificación*: `test_physical_assets_integrity` y `test_asset_content_structure` verifican que los 35 recursos existan físicamente en `assets/**` con tamaño > 0 bytes; que `tokens.json` y los 6 temas sean JSON válidos; que los 22 SVG contengan `viewBox="0 0 24 24"` y XML bien formado sin `<script>`; y que los 6 fondos JPG presenten número mágico JPEG (`0xFF, 0xD8, 0xFF`).
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_inclusion_tests --output-on-failure`

- [ ] TASK-002: Implementar pruebas de fuzzing negativo de alias, prevención de Directory Traversal y exclusión de rutas en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`
  - *Criterio de verificación*: `test_negative_alias_syntax_fuzzing` y `test_excluded_paths_defense` verifican el rechazo estricto de rutas de escape (`../`, `..\`), cadenas vacías, mayúsculas, caracteres especiales y extensiones ejecutables en `isValidAlias`, así como el bloqueo infalible de `reference/`, `installer/`, `fonts/` y rutas del sistema operativo con código `AccessDenied`.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_inclusion_tests --output-on-failure`

- [ ] TASK-003: Implementar pruebas de corrupción de manifiestos QRC y diagnóstico estructurado de recursos críticos en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`
  - *Criterio de verificación*: `test_synthetic_qrc_manifest_corruption` y `test_critical_resources_isolation_and_remediation` verifican la detección determinista de prefijos erróneos, alias duplicados y no inventariados en XML sintético; y comprueban que la omisión individual de cualquiera de los 8 recursos críticos resulte en `MissingCritical` con listado explícito del recurso y acción de mitigación no vacía.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_inclusion_tests --output-on-failure`

- [ ] TASK-004: Implementar pruebas de recuperación determinista por fallback, resolución resiliente y cancelación cooperativa en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`
  - *Criterio de verificación*: `test_fallback_recovery_mechanisms` y `test_cancellation_cooperative` verifican la resolución infalible de iconos desconocidos a `":/vigilio/icons/tools/tools.svg"`, temas desconocidos a `"themes/light.json"`, carga con `allowFallback = true` retornando datos de reemplazo y `allowFallback = false` retornando `NotFound`; así como la cancelación inmediata y limpia de lecturas binarias con `CancellationToken` sin fugas de descriptores de archivo.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_inclusion_tests --output-on-failure`

- [ ] TASK-005: Implementar pruebas de estrés de alta concurrencia multihilo y seguridad sin cerrojos en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`
  - *Criterio de verificación*: `test_high_concurrency_stress_and_thread_safety` ejecuta 16 hilos simultáneos realizando 2000 iteraciones consecutivas (32,000 operaciones en total) de consultas, resolución de rutas y validación de manifiestos, certificando 100% de éxito, latencia media < 1 ms y cero condiciones de carrera bajo ThreadSanitizer.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_inclusion_tests --output-on-failure`

- [ ] TASK-006: Registrar el target ejecutable `vigilio_resource_qrc_inclusion_tests` y el test de CTest `resource_qrc_inclusion_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` declara `add_executable(vigilio_resource_qrc_inclusion_tests tests/unit/ResourceQrcInclusionTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, con macros `VIGILIO_ASSETS_DIR` y `VIGILIO_RESOURCES_QRC_PATH`, y registra `add_test(NAME resource_qrc_inclusion_tests COMMAND vigilio_resource_qrc_inclusion_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "resource_qrc_inclusion_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Ejecutar el pipeline de comprobación continua local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror` y Clang-Format
  - *Criterio de verificación*: Compilación Ninja limpia sin advertencias bajo flags estrictos y `-Werror`; 100% de las pruebas rápidas de CTest pasadas (8/8 pruebas con `resource_qrc_inclusion_tests`) con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-008: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0020-resource-qrc-inclusion/task.md` y validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: El documento canónico de la microtarea actualiza su estado a `EXECUTED` tras la fase APPLY, y las herramientas de validación de Prompt Builder certifican `valid: true`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0058-module.foundations.task0020.propose.lock.json --project-root .`
