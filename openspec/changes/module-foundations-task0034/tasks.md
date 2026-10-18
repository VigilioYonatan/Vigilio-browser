# Tasks — module-foundations-task0034

Status: PROPOSED

- [ ] TASK-001: Ampliar `apps/vigilio-browser/src/core/CMakePresetsTypes.h` con enumeraciones y estructuras operativas del servicio
  - *Criterio de verificación*: `CMakePresetsTypes.h` declara formalmente las enumeraciones fuertemente tipadas `CMakePresetExecutionState` (`Idle`, `Validating`, `Configuring`, `Building`, `Testing`, `Completed`, `Failed`, `Cancelled`), `CMakePresetExecutionStatus` (`Success`, `ValidationError`, `PresetNotFound`, `BinaryDirUnsafe`, `ToolNotFound`, `ProcessFailed`, `Timeout`, `Cancelled`, `IOError`), la estructura de sincronización atómica `CancellationToken`, las opciones `CMakePresetCommandOptions`, el resultado inmutable `CMakePresetExecutionResult` y el consolidado `CMakePresetsSummary`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsTypes.h && grep -E "enum class CMakePresetExecutionState|enum class CMakePresetExecutionStatus|struct CMakePresetCommandOptions|struct CMakePresetExecutionResult|struct CMakePresetsSummary|struct CancellationToken" apps/vigilio-browser/src/core/CMakePresetsTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada de `CMakePresetsService` en `apps/vigilio-browser/src/core/CMakePresetsService.h`
  - *Criterio de verificación*: `CMakePresetsService.h` declara la clase `CMakePresetsService` en el espacio de nombres `vigilio` con métodos `loadFromFile`, `loadFromJson`, `isLoaded`, `summary`, `findPreset`, `listPresetsByKind`, `resolveAssociatedConfigurePreset`, `buildConfigureCommandLine`, `buildBuildCommandLine`, `buildTestCommandLine`, `executeConfigure`, `executeBuild`, `executeTest`, `cleanBinaryDir`, `executionStatusToString` y `executionStateToString`, completamente desacoplada de `QApplication` y `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsService.h && grep -E "class CMakePresetsService|loadFromFile|findPreset|buildConfigureCommandLine|executeConfigure|cleanBinaryDir" apps/vigilio-browser/src/core/CMakePresetsService.h`

- [ ] TASK-003: Implementar la carga, parseo, indexación y resumen de presets en `apps/vigilio-browser/src/core/CMakePresetsService.cpp`
  - *Criterio de verificación*: `CMakePresetsService.cpp` implementa `loadFromFile` y `loadFromJson` delegando en `CMakePresetsValidator::validatePresetsJson`, indexa los vectores internos de presets de configuración, construcción y pruebas, y calcula de forma determinista el resumen inmutable `CMakePresetsSummary`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/CMakePresetsService.cpp && grep -E "loadFromFile|loadFromJson|summary|findPreset" apps/vigilio-browser/src/core/CMakePresetsService.cpp`

- [ ] TASK-004: Implementar la síntesis determinista de líneas de comando CLI para configure, build y test presets en `apps/vigilio-browser/src/core/CMakePresetsService.cpp`
  - *Criterio de verificación*: `buildConfigureCommandLine` sintetiza `cmake --preset <name>`, `buildBuildCommandLine` genera `cmake --build --preset <name>` y `buildTestCommandLine` genera `ctest --preset <name>`, incorporando argumentos adicionales de `options.additionalArgs` y resolviendo la macro `${sourceDir}`.
  - *Comando*: `grep -E "buildConfigureCommandLine|buildBuildCommandLine|buildTestCommandLine" apps/vigilio-browser/src/core/CMakePresetsService.cpp`

- [ ] TASK-005: Implementar el ejecutor acotado con soporte de dry-run, timeouts, búferes acotados a 64 KB y cancelación cooperativa en `apps/vigilio-browser/src/core/CMakePresetsService.cpp`
  - *Criterio de verificación*: `executeConfigure`, `executeBuild` y `executeTest` evalúan el modo `dryRun` retornando éxito inmediato sin tocar el disco, comprueban `CancellationToken::isCancelled` abortando limpiamente si se solicita, aplican timeout acotado (`timeoutMs`) y truncan la captura de salida a `maxOutputBytes` (64 KB) con aviso explícito de truncado.
  - *Comando*: `grep -E "executeConfigure|executeBuild|executeTest|dryRun|maxOutputBytes" apps/vigilio-browser/src/core/CMakePresetsService.cpp`

- [ ] TASK-006: Implementar la operación de saneamiento `cleanBinaryDir` y formateo observable de diagnósticos en `apps/vigilio-browser/src/core/CMakePresetsService.cpp`
  - *Criterio de verificación*: `cleanBinaryDir` valida la ruta mediante `CMakePresetsValidator::isSafeBinaryDir`, denegando categóricamente el borrado si la ruta contiene `..` o no pertenece a `${sourceDir}/build-*`, y eliminando de forma segura los archivos de caché si es válida; `executionStatusToString` y `executionStateToString` devuelven descripciones en español técnico conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "cleanBinaryDir|isSafeBinaryDir|executionStatusToString|executionStateToString" apps/vigilio-browser/src/core/CMakePresetsService.cpp`

- [ ] TASK-007: Implementar la batería de 12 pruebas unitarias de la capa de servicio en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`
  - *Criterio de verificación*: `CMakePresetsServiceTests.cpp` cubre los 12 escenarios de la matriz (`TEST-PRESET-SVC-001` a `TEST-PRESET-SVC-012`): carga canónica, carga de archivo real, búsqueda válida e inválida, resolución de jerarquía build/configure, síntesis de configure, build y test CLI, ejecución dry-run, cancelación cooperativa con token, rechazo de saneamiento de ruta insegura y limpieza exitosa de directorio temporal seguro.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp && grep -E "test_service_load_canonical|test_service_command_line_synthesis|test_service_dry_run|test_service_cancellation|test_service_clean_binary_dir" apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`

- [ ] TASK-008: Integrar `CMakePresetsService.cpp` en `vigilio_core`, configurar el target ejecutable `vigilio_presets_service_tests` y registrar el test `cmake_presets_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incorpora `src/core/CMakePresetsService.cpp` en la biblioteca estática `vigilio_core`, define `add_executable(vigilio_presets_service_tests tests/unit/CMakePresetsServiceTests.cpp)` enlazado a `vigilio_core` y `vigilio_compiler_warnings`, y registra `add_test(NAME cmake_presets_service_tests COMMAND vigilio_presets_service_tests)` con `LABELS "unit;fast"` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "cmake_presets_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-009: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests de CTest pasando exitosamente (13/13 tests, exit code 0 en < 1s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile con Prompt Builder CLI y sincronizar documentación en `docs/rules/01-modules/foundations/micro/0034-cmake-presets/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas, `verify-lock` certifica la integridad del archivo lock correspondiente a 0100, y el documento de microtarea refleja la formalización de la etapa implementation.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0100-module.foundations.task0034.propose.lock.json --project-root . && grep -E "module.foundations.task0034" docs/rules/01-modules/foundations/micro/0034-cmake-presets/task.md`
