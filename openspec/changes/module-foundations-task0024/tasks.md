# Tasks — module-foundations-task0024

Status: PROPOSED

- [ ] TASK-001: Consolidar la verificación de casos positivos (StandardGui con URLs iniciales y `--headless` explícito) en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_normal_gui` y `test_bootstrap_headless_explicit` validan `isSuccess() == true`, códigos de salida 0 y correcta asignación de modos operativos tanto con display presente como ausente.
  - *Comando*: `grep -E "test_bootstrap_normal_gui|test_bootstrap_headless_explicit" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-002: Consolidar la verificación de degradación elegante sin servidor gráfico (`DegradedHeadless`) en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_degraded_headless_missing_display` evalúa `DISPLAY=nullptr` y `WAYLAND_DISPLAY=nullptr` retornando `StartupStatusCode::DegradedHeadless`, exit code 1, `isSuccess() == true` y acción de remediación no vacía.
  - *Comando*: `grep -E "test_bootstrap_degraded_headless_missing_display" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-003: Consolidar la verificación de aislamiento en modo incógnito y modo seguro en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_incognito_volatile_storage` y `test_bootstrap_safe_mode` certifican `storagePath == ":memory:"`, `isStoragePersistent == false`, ausencia absoluta de creación de carpetas o archivos en disco, y correcta configuración de `SafeMode`.
  - *Comando*: `grep -E "test_bootstrap_incognito_volatile_storage|test_bootstrap_safe_mode" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-004: Consolidar la verificación de bloqueo de banderas prohibidas de seguridad de Chromium en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_forbidden_arguments` rechaza deterministamente `--no-sandbox`, `--disable-web-security`, `--remote-debugging-pipe`, `--allow-file-access-from-files` y `--remote-debugging-port` (individual y con puerto asignado), retornando `InvalidCommandLineArgs` y exit code 10.
  - *Comando*: `grep -E "test_bootstrap_forbidden_arguments" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-005: Consolidar la verificación de defensa activa contra Directory Traversal e inyección de rutas en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_directory_traversal_defense` somete a `isValidProfilePath` a secuencias `../`, rutas Unix protegidas (`/etc`, `/boot`, `/sys`, `/proc`, `/root`), rutas Windows (`\\server\share`, `C:\Windows\System32`), caracteres de control y metacaracteres shell.
  - *Comando*: `grep -E "test_bootstrap_directory_traversal_defense" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-006: Consolidar la verificación de solicitudes informativas de ayuda y versión en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_help_and_version_requests` evalúa `--help`, `-h`, `--version` y `-v` retornando `StartupStatusCode::UserHelpOrVersionRequested`, fase `Aborted` y exit code 0 sin inicializar recursos pesados.
  - *Comando*: `grep -E "test_bootstrap_help_and_version_requests" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-007: Consolidar las pruebas de inyección de fallos (Qt incompatible, QRC corrupto, almacenamiento denegado) en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_incompatible_qt_simulation`, `test_bootstrap_corrupt_resources_simulation` y `test_bootstrap_storage_access_denied` verifican la detención en fases correspondientes con códigos tipados 12, 14 y 15, y mensajes de remediación estructurados.
  - *Comando*: `grep -E "test_bootstrap_incompatible_qt_simulation|test_bootstrap_corrupt_resources_simulation|test_bootstrap_storage_access_denied" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-008: Consolidar la prueba de estrés concurrente masivo (16 hilos × 500 iteraciones) y conversiones de cadenas en `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`
  - *Criterio de verificación*: Comprobación de que `test_bootstrap_concurrency_stress` y `test_bootstrap_string_conversions` evalúan 8,000 ciclos concurrentes sin data races, midiendo latencia acotada y validando las 11 fases, 10 estados y 4 modos.
  - *Comando*: `grep -E "test_bootstrap_concurrency_stress|test_bootstrap_string_conversions" apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`

- [ ] TASK-009: Ejecutar el pipeline de compilación y pruebas automatizadas con `check.sh` certificando el paso de CTest
  - *Criterio de verificación*: Ejecución de `bash apps/vigilio-browser/scripts/check.sh` finalizando con exit code 0 y 100% de tests aprobados en CTest bajo C++20 y `-Werror`.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Certificar la gobernanza documental y validez del lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` reporta estado `valid: true` y `verify-lock` aprueba las 13 comprobaciones del lockfile `0070-module.foundations.task0024.propose.lock.json`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0070-module.foundations.task0024.propose.lock.json --project-root .`
