# Tasks — module-foundations-task0033

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos de valor, enumeraciones y estructuras inmutables del dominio de CMake Presets en `apps/vigilio-browser/src/core/CMakePresetsTypes.h`
  - *Criterio de verificación*: `CMakePresetsTypes.h` declara formalmente las enumeraciones fuertemente tipadas `CMakePresetKind` (`Configure`, `Build`, `Test`, `Package`, `Workflow`), `CMakeBuildType` (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`), `CMakeGenerator` (`Ninja`, `UnixMakefiles`, `NinjaMultiConfig`, `VisualStudiov17`), `CMakePresetValidationOutcome` (`Valid`, `InvalidSchemaVersion`, `MissingRequiredPreset`, `OrphanConfigurePresetReference`, `DuplicatePresetName`, `UnsafeBinaryDir`, `ConflictingVariables`, `MalformedJson`), y las estructuras inmutables `CMakePresetDefinition` y `CMakePresetsValidationReport` con método `isValid()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsTypes.h && grep -E "enum class CMakePresetKind|enum class CMakePresetValidationOutcome|struct CMakePresetDefinition|struct CMakePresetsValidationReport" apps/vigilio-browser/src/core/CMakePresetsTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del validador de presets en `apps/vigilio-browser/src/core/CMakePresetsValidator.h`
  - *Criterio de verificación*: `CMakePresetsValidator.h` declara la clase `CMakePresetsValidator` en el espacio de nombres `vigilio` con métodos estáticos puros `validatePresetsJson`, `validatePresetsFile`, `isSafeBinaryDir`, `canonicalRequiredPresets` y `outcomeToString`, completamente desacoplada de `QApplication`, `QWidget` y `QWebEngineView`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsValidator.h && grep -E "class CMakePresetsValidator|validatePresetsJson|validatePresetsFile|isSafeBinaryDir" apps/vigilio-browser/src/core/CMakePresetsValidator.h`

- [ ] TASK-003: Implementar el motor de análisis estático, reglas de validación de esquemas y rechazo de rutas no seguras en `apps/vigilio-browser/src/core/CMakePresetsValidator.cpp`
  - *Criterio de verificación*: `CMakePresetsValidator.cpp` implementa el parsing determinista de JSON, verifica que la versión sea $\ge 3$, comprueba la existencia de los presets mínimos obligatorios (`core-debug` y `desktop-debug`), valida que todo `buildPreset` y `testPreset` referencie un `configurePreset` existente, detecta duplicados, rechaza rutas con Directory Traversal (`..`) o rutas absolutas fijas en `binaryDir`, y detecta combinaciones conflictivas de variables de caché.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsValidator.cpp && grep -E "validatePresetsJson|isSafeBinaryDir|canonicalRequiredPresets" apps/vigilio-browser/src/core/CMakePresetsValidator.cpp`

- [ ] TASK-004: Estandarizar y ampliar el catálogo declarativo de perfiles en `apps/vigilio-browser/CMakePresets.json`
  - *Criterio de verificación*: `CMakePresets.json` incorpora formalmente los 9 perfiles canónicos bajo el esquema versión 6: `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan` y `sanitizer-ubsan`, junto con sus correspondientes presets de compilación y prueba vinculados.
  - *Comando*: `grep -E "\"name\": \"core-debug\"|\"name\": \"core-release\"|\"name\": \"desktop-debug\"|\"name\": \"desktop-release\"|\"name\": \"desktop-e2e\"|\"name\": \"ci-linux-debug\"|\"name\": \"sanitizer-asan\"" apps/vigilio-browser/CMakePresets.json`

- [ ] TASK-005: Implementar la batería exhaustiva de pruebas unitarias y de resiliencia en `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`
  - *Criterio de verificación*: `CMakePresetsTests.cpp` cubre los 12 escenarios de la matriz de pruebas (`TEST-PRESET-001` a `TEST-PRESET-012`): preset válido canónico, esquema de versión inferior no compatible, preset obligatorio faltante, referencias huérfanas en build/test presets, nombres duplicados, rutas de binarios no seguras (traversal y absolutas), variables conflictivas, JSON malformado, validación del archivo real del proyecto y prueba de rendimiento en bucle (< 50 ms).
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp && grep -E "test_presets_canonical_valid|test_presets_orphan_configure_reference|test_presets_unsafe_binary_dir" apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`

- [ ] TASK-006: Integrar `CMakePresetsValidator.cpp` en `vigilio_core`, configurar el target ejecutable `vigilio_presets_tests` y registrar el test `cmake_presets_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incorpora `src/core/CMakePresetsValidator.cpp` a la biblioteca estática `vigilio_core`, define `add_executable(vigilio_presets_tests tests/unit/CMakePresetsTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, y registra `add_test(NAME cmake_presets_tests COMMAND vigilio_presets_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "cmake_presets_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests de CTest pasando exitosamente (exit code 0 en < 1s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-008: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0097.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0097-module.foundations.task0033.propose.lock.json --project-root .`

- [ ] TASK-009: Sincronizar la documentación canónica en `docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase domain y se mantiene coherente con el modelo y taxonomía de presets implementados.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md && grep -E "module.foundations.task0033" docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md`
