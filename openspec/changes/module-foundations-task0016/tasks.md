# Tasks — module-foundations-task0016

Status: PROPOSED

- [ ] TASK-001: Auditar y formalizar los casos de prueba de parsing positivo y normalización de versiones Chromium en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Verificación exhaustiva de versiones estándar (`128.0.6613.119`), versiones con ceros en subcampos, prefijos `v`/`V`, espaciado perimetral recortado, hito extraído (`milestone() == 128`) y serialización canónica idéntica a la entrada normalizada.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp && grep -E "test_chromium_version_parsing_positive" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-002: Implementar y verificar los casos de prueba de parsing negativo, cadenas malformadas y protección contra desbordamiento entero en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Rechazo determinista con `std::nullopt` para cadenas vacías, espacios, campos numéricos insuficientes (< 4), campos excedentes (> 4), sufijos textuales, caracteres no numéricos, puntuación rota, números negativos y números que desbordan `uint32_t`.
  - *Comando*: `grep -E "test_chromium_version_parsing_negative" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-003: Formalizar las pruebas de ordenamiento total, comparación estricta (`operator<=>`) e invariantes de baseline en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Comprobación de las propiedades de reflexividad, antisimetría y transitividad del operador `<=>`, predicado `isCompatibleWith` frente a Chromium 128.0.0.0, e invariante de `baselineChromiumVersion()`.
  - *Comando*: `grep -E "test_chromium_version_comparison_and_compatibility" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-004: Verificar la matriz de capacidades funcionales y perfiles de requerimientos en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Validación de las 8 capacidades en hito >= 128, 6 capacidades en hito 122, 4 capacidades en hito 118, y 0 en hito 0; comprobación de requerimientos para los 4 perfiles canónicos (`FullDesktopBrowser`, `RestrictedSecureProfile`, `HeadlessInspection`, `MinimalCore`).
  - *Comando*: `grep -E "test_feature_matrix_and_profiles" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-005: Implementar la validación exhaustiva de permisos de archivo (`0755` vs `0644`) y symlinks del proceso asistente sintético mediante RAII en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Comprobación de rechazo de rutas vacías, inexistentes y directorios; discriminación de permisos ejecutables (`0755` aceptado, `0644` rechazado en POSIX); y limpieza determinista de archivos sintéticos temporales mediante `TempHelperFileGuard`.
  - *Comando*: `grep -E "test_helper_executable_check|TempHelperFileGuard" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-006: Auditar la taxonomía completa de estados de compatibilidad e incompatibilidad y pre-validación de arranque en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Cobertura completa de estados `Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing` y `EnvironmentMisconfigured` en `validateCompatibility` y `validateStartup`.
  - *Comando*: `grep -E "test_compatibility_validation|test_startup_validation" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-007: Fortalecer las pruebas de concurrencia y estrés multihilo con 16 hilos simultáneos y 1000 iteraciones por hilo en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - *Criterio de verificación*: Ejecución concurrente masiva libre de data races comprobable con ThreadSanitizer; ausencia de estado mutable global; latencia por llamada de validación < 1 ms y finalización limpia con `atomic<bool> failure{false}`.
  - *Comando*: `grep -E "test_concurrency_and_thread_safety|numThreads" apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`

- [ ] TASK-008: Verificar la gobernanza en `apps/vigilio-browser/CMakeLists.txt` y ejecutar la verificación continua con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Target `vigilio_webengine_validator_tests` y test `webengine_version_validator_tests` registrados con `LABELS "unit;fast"`; ejecución exitosa de los 5 tests de CTest con exit code 0 y validación de formato con Clang-Format sin advertencias.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh && ctest --test-dir apps/vigilio-browser/_build -R webengine_version_validator_tests --verbose`

- [ ] TASK-009: Validar las referencias canónicas OpenSpec y lockfiles con la herramienta Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` para todas las referencias OpenSpec, y `verify-lock` certifica la integridad del lockfile `0046-module.foundations.task0016.propose.lock.json`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0046-module.foundations.task0016.propose.lock.json --project-root .`
