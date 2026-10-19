# Tasks — module-foundations-task0045

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos, enumeraciones fuertemente tipadas y estructuras inmutables del dominio en `apps/vigilio-browser/src/core/ClangTidyTypes.h`
  - *Criterio de verificación*: `ClangTidyTypes.h` declara formalmente las enumeraciones `enum class ClangTidyCheckCategory`, `enum class ClangTidyValidationState`, `enum class ClangTidyError`, y las estructuras inmutables `ClangTidyOptions`, `ClangTidyValidationReport` y `ClangTidyIssueInspection`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyTypes.h && grep -E "enum class ClangTidyCheckCategory|enum class ClangTidyValidationState|enum class ClangTidyError|struct ClangTidyOptions|struct ClangTidyValidationReport" apps/vigilio-browser/src/core/ClangTidyTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del validador de configuración y parser YAML en `apps/vigilio-browser/src/core/ClangTidyValidator.h`
  - *Criterio de verificación*: `ClangTidyValidator.h` declara la clase `ClangTidyValidator` en el namespace `vigilio` con los métodos estáticos `validateOptions`, `validateConfigurationFile`, `validateConfigContent`, `parseConfig`, `parseCheckTokens`, `serializeConfig`, `isSafeConfigPath`, `isSafeRegex`, `tidyStateToString`, `tidyErrorToString` y `categoryToString`, sin dependencias hacia `QApplication` ni `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyValidator.h && grep -E "class ClangTidyValidator|validateOptions|validateConfigurationFile|parseConfig|parseCheckTokens|isSafeRegex" apps/vigilio-browser/src/core/ClangTidyValidator.h`

- [ ] TASK-003: Implementar el motor estático de validación, parser YAML determinista, analizador de tokens de comprobación, serializador y copy público en `apps/vigilio-browser/src/core/ClangTidyValidator.cpp`
  - *Criterio de verificación*: `ClangTidyValidator.cpp` implementa la validación de checks no vacíos, compilación de regex seguro con `std::regex`, parsing de pares clave-valor YAML sin bibliotecas externas, prevención de directory traversal y conversión de errores/estados a cadenas en español conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyValidator.cpp && grep -E "validateOptions|validateConfigurationFile|parseConfig|parseCheckTokens|tidyErrorToString" apps/vigilio-browser/src/core/ClangTidyValidator.cpp`

- [ ] TASK-004: Declarar e implementar la entidad de configuración canónica, extensiones y exclusiones en `apps/vigilio-browser/src/core/ClangTidyConfiguration.h` y `apps/vigilio-browser/src/core/ClangTidyConfiguration.cpp`
  - *Criterio de verificación*: `ClangTidyConfiguration` provee el método `canonicalConfig()` con los valores estándar del proyecto (`Checks: '-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements'`, `WarningsAsErrors: ''`, `HeaderFilterRegex: 'src/.*'`), el filtro `isSupportedSourceExtension` para `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`, y el filtro `isExcludedDirectory` para directorios `_build`, `build`, `generated`, `.git`, `.prompt-runner`, `assets`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangTidyConfiguration.h && test -f apps/vigilio-browser/src/core/ClangTidyConfiguration.cpp && grep -E "canonicalConfig|isSupportedSourceExtension|isExcludedDirectory|verifyInvariants" apps/vigilio-browser/src/core/ClangTidyConfiguration.h`

- [ ] TASK-005: Implementar la batería exhaustiva de 11 pruebas unitarias de especificación en `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangTidyConfigurationTests.cpp` implementa las 11 pruebas trazables (`TEST-TIDY-001` a `TEST-TIDY-011`) que verifican opciones canónicas, validación de opciones correctas, rechazo de checks vacíos, rechazo de regex de encabezados corrupto, parseo del archivo `.clang-tidy` del repositorio, serialización roundtrip, detección de claves obligatorias faltantes, rechazo de rutas con directory traversal, filtro de extensiones/exclusiones, particionado de tokens habilitados/deshabilitados, y cumplimiento del copy público en español sin jerga técnica interna.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp && grep -E "test_clang_tidy_canonical_options|test_clang_tidy_validator_valid_options|test_clang_tidy_parse_canonical_yaml" apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`

- [ ] TASK-006: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_clang_tidy_tests` y registrar `clang_tidy_configuration_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/ClangTidyValidator.cpp` y `src/core/ClangTidyConfiguration.cpp` en la librería `vigilio_core`, define `add_executable(vigilio_clang_tidy_tests tests/unit/ClangTidyConfigurationTests.cpp)`, define `VIGILIO_CLANG_TIDY_PATH` y registra `add_test(NAME clang_tidy_configuration_tests COMMAND vigilio_clang_tidy_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "clang_tidy_configuration_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente en menos de 3 segundos (18/18 tests).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-008: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las 45 referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0133.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0133-module.foundations.task0045.propose.lock.json --project-root .`

- [ ] TASK-009: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0045-clang-tidy-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `domain` y se mantiene alineado con los contratos y el modelo de dominio acordados.
  - *Comando*: `grep -E "module.foundations.task0045" docs/rules/01-modules/foundations/micro/0045-clang-tidy-configuration/task.md`
