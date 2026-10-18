# Tasks — module-foundations-task0012

Status: PROPOSED

- [ ] TASK-001: Implementar suite unitaria de parsing y aritmética de versiones semánticas en `apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`
  - *Criterio de verificación*: `SemanticVersion::parse` evalúa correctamente versiones canónicas de 3 partes (`6.10.0`), normalizaciones de 2 partes (`6.10`), versiones con espacios perimetrales y prefijos ('v'), rechazando deterministamente cadenas vacías, entradas no numéricas, puntuación errónea y desbordamientos. El operador `<=>` y el predicado `isCompatibleWith` garantizan compatibilidad estricta para Qt >= 6.10.0 dentro de la misma versión mayor (major 6).
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R qt_dependency_validation_tests --output-on-failure`

- [ ] TASK-002: Implementar casos de prueba para perfiles de requerimientos y componentes ausentes en `apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`
  - *Criterio de verificación*: `QtDependencyValidator::validateProfile` reporta `Satisfied` para los perfiles `HeadlessCore`, `MinimalGui`, `FullBrowser` y `DeveloperTest` cuando los módulos requeridos están presentes y la versión es compatible. Ante módulos faltantes, retorna `MissingComponent` identificando la lista exacta de componentes ausentes; ante versiones obsoletas (< 6.10.0), retorna `IncompatibleVersion`.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R qt_dependency_validation_tests --output-on-failure`

- [ ] TASK-003: Implementar pruebas de validación de entorno runtime, servidores de display y desajustes de versión ABI en `apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`
  - *Criterio de verificación*: `QtDependencyValidator::validateRuntimeEnvironment` valida correctamente entornos con servidor X11, Wayland o duales; detecta deterministamente la ausencia de servidor de display en Linux retornando `EnvironmentError`; detecta desajustes mayores de versión ABI entre compilación y runtime (major mismatch) retornando `EnvironmentError`; y detecta versiones de runtime obsoletas retornando `IncompatibleVersion`.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R qt_dependency_validation_tests --output-on-failure`

- [ ] TASK-004: Implementar pruebas de seguridad concurrente multihilo y formateo diagnóstico en `apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`
  - *Criterio de verificación*: Múltiples hilos (`std::jthread`) invocan simultáneamente y de forma segura los métodos puros de validación sin carreras de datos ni fugas de memoria. Las funciones `formatOutcome`, `formatReport` y `moduleName` generan salidas de texto completas, legibles y descriptivas sin cadenas vacías ni valores indefinidos.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R qt_dependency_validation_tests --output-on-failure`

- [ ] TASK-005: Registrar target `vigilio_qt_validator_tests` y test de CTest `qt_dependency_validation_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` declara el ejecutable `vigilio_qt_validator_tests` con enlace privado a `vigilio_core` y `vigilio_compiler_warnings`. Registra la prueba en CTest con `add_test(NAME qt_dependency_validation_tests COMMAND vigilio_qt_validator_tests)` bajo etiquetas `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 5 "qt_dependency_validation_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Ejecutar la suite completa de calidad local (`apps/vigilio-browser/scripts/check.sh`), linters y compilación bajo `-Werror`
  - *Criterio de verificación*: Compilación Ninja limpia sin ninguna advertencia bajo flags hiperestrictos y `-Werror`; 100% de las pruebas rápidas de CTest pasadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`) con exit code 0; y conformidad de estilo con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0012-qt-dependency-validation/task.md` y validar referencias canónicas y decisiones con Prompt Builder
  - *Criterio de verificación*: El documento canónico de la microtarea actualiza su estado a `EXECUTED` tras la fase APPLY, y el árbol de dependencias canónicas pasa satisfactoriamente las herramientas de validación de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
