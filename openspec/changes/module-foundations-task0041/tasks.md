# Tasks — module-foundations-task0041

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos, enumeraciones fuertemente tipadas y estructuras inmutables del dominio en `apps/vigilio-browser/src/core/ClangFormatTypes.h`
  - *Criterio de verificación*: `ClangFormatTypes.h` declara formalmente las enumeraciones `enum class ClangFormatStylePreset`, `enum class PointerAlignmentPolicy`, `enum class ShortFunctionPolicy`, `enum class ClangFormatValidationState`, `enum class ClangFormatError`, y las estructuras inmutables `ClangFormatOptions`, `ClangFormatValidationReport` y `SourceFileFormatInspection`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatTypes.h && grep -E "enum class ClangFormatStylePreset|enum class ClangFormatValidationState|struct ClangFormatOptions|struct ClangFormatValidationReport" apps/vigilio-browser/src/core/ClangFormatTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del validador de configuración y parser YAML en `apps/vigilio-browser/src/core/ClangFormatValidator.h`
  - *Criterio de verificación*: `ClangFormatValidator.h` declara la clase `ClangFormatValidator` en el namespace `vigilio` con los métodos estáticos `validateOptions`, `validateConfigurationFile`, `parseConfig`, `serializeConfig`, `isSafeConfigPath`, `formatStateToString`, `formatErrorToString`, `stylePresetToString` y `pointerAlignmentToString`, sin dependencias hacia `QApplication` ni `QWidget`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatValidator.h && grep -E "class ClangFormatValidator|validateOptions|validateConfigurationFile|parseConfig|serializeConfig" apps/vigilio-browser/src/core/ClangFormatValidator.h`

- [ ] TASK-003: Implementar el motor estático de validación, parser YAML determinista, serializador y copy público en `apps/vigilio-browser/src/core/ClangFormatValidator.cpp`
  - *Criterio de verificación*: `ClangFormatValidator.cpp` implementa la validación de rangos (`indentWidth` entre 2 y 8, `columnLimit` entre 80 y 140), parsing seguro de pares clave-valor YAML sin bibliotecas externas, prevención de directory traversal y conversión de errores/estados a cadenas en español conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatValidator.cpp && grep -E "validateOptions|validateConfigurationFile|parseConfig|serializeConfig|formatErrorToString" apps/vigilio-browser/src/core/ClangFormatValidator.cpp`

- [ ] TASK-004: Declarar e implementar la entidad de configuración canónica, extensiones y exclusiones en `apps/vigilio-browser/src/core/ClangFormatConfiguration.h` y `apps/vigilio-browser/src/core/ClangFormatConfiguration.cpp`
  - *Criterio de verificación*: `ClangFormatConfiguration` provee el método `canonicalConfig()` con los valores estándar del proyecto (LLVM, 4, 105, Left, None), el filtro `isSupportedSourceExtension` para `.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`, y el filtro `isExcludedDirectory` para directorios `_build`, `build`, `generated`, `.git`, `.prompt-runner`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ClangFormatConfiguration.h && test -f apps/vigilio-browser/src/core/ClangFormatConfiguration.cpp && grep -E "canonicalConfig|isSupportedSourceExtension|isExcludedDirectory" apps/vigilio-browser/src/core/ClangFormatConfiguration.h`

- [ ] TASK-005: Implementar la batería exhaustiva de 10 pruebas unitarias de especificación en `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`
  - *Criterio de verificación*: `ClangFormatConfigurationTests.cpp` implementa las 10 pruebas trazables (`TEST-FORMAT-001` a `TEST-FORMAT-010`) que verifican opciones canónicas, validación de opciones correctas, rechazo de sangría inválida, rechazo de límite de columna anómalo, parseo del archivo `.clang-format` del repositorio, serialización roundtrip, detección de claves obligatorias faltantes, rechazo de rutas con directory traversal, filtro de extensiones/exclusiones, y cumplimiento del copy público en español sin jerga técnica interna.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp && grep -E "test_clang_format_canonical_options|test_clang_format_validator_valid_options|test_clang_format_parse_canonical_yaml" apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp`

- [ ] TASK-006: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_clang_format_tests` y registrar `clang_format_configuration_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/ClangFormatValidator.cpp` y `src/core/ClangFormatConfiguration.cpp` en la librería `vigilio_core`, define `add_executable(vigilio_clang_format_tests tests/unit/ClangFormatConfigurationTests.cpp)` y registra `add_test(NAME clang_format_configuration_tests COMMAND vigilio_clang_format_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "clang_format_configuration_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente en menos de 2 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-008: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las 41 referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0121.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0121-module.foundations.task0041.propose.lock.json --project-root .`

- [ ] TASK-009: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0041-clang-format-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `domain` y se mantiene alineado con los contratos y el modelo de dominio acordados.
  - *Comando*: `grep -E "module.foundations.task0041" docs/rules/01-modules/foundations/micro/0041-clang-format-configuration/task.md`
