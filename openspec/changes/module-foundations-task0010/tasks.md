# Tasks — module-foundations-task0010

Status: PROPOSED

- [ ] TASK-001: Implementar las entidades C++20 de dominio y el servicio de validación de dependencias Qt (`QtDependencyValidator.h` y `QtDependencyValidator.cpp`) en `apps/vigilio-browser/src/core/` sin dependencias de Qt
  - *Criterio de verificación*: Las entidades `SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome`, `QtValidationReport` y los métodos estáticos de validación (`parseVersion`, `validateProfile`, `validateRuntimeEnvironment`, `formatReport`) están implementados en C++20 puro y sin acoplamiento a módulos de Qt ni a widgets.
  - *Comando*: `ls -la apps/vigilio-browser/src/core/QtDependencyValidator.*`

- [ ] TASK-002: Registrar `src/core/QtDependencyValidator.cpp` en el target estático `vigilio_core` dentro de `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `vigilio_core` incluye el archivo de implementación del validador y compila limpiamente en modo headless con C++20 bajo la política estricta `-Werror`.
  - *Comando*: `grep "src/core/QtDependencyValidator.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-003: Implementar la gobernanza de dependencias Qt en CMake con soporte para `VIGILIO_REQUIRE_QT`, detección granular de módulos y degradación elegante a headless
  - *Criterio de verificación*: `CMakeLists.txt` define la opción de configuración `VIGILIO_REQUIRE_QT`, evalúa independientemente los componentes `Core`, `Widgets` y `WebEngineWidgets` (y `Test` si aplica), e implementa los caminos de advertencia con degradación o error fatal con diagnóstico específico.
  - *Comando*: `grep -E "VIGILIO_REQUIRE_QT|QT_MINIMUM_VERSION" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-004: Integrar el protocolo de pre-validación de arranque en `apps/vigilio-browser/src/app.cpp` antes de instanciar `QApplication` y `BrowserWindow`
  - *Criterio de verificación*: `main()` invoca `QtDependencyValidator::validateRuntimeEnvironment` y aborta de forma controlada con mensaje a `std::cerr` y código de salida 1 si la validación no es satisfactoria, evitando crashes por ABI o ausencia de servidor gráfico.
  - *Comando*: `grep -A 10 "validateRuntimeEnvironment" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-005: Implementar pruebas unitarias deterministas en `apps/vigilio-browser/tests/unit/CoreTests.cpp` para validar la lógica de parsing de versiones, perfiles y resultados de validación
  - *Criterio de verificación*: Las pruebas unitarias cubren casos normales, errores de versión (< 6.10.0), perfiles con componentes faltantes y evaluación de entorno runtime en modo 100% headless.
  - *Comando*: `grep -A 20 "QtDependencyValidator" apps/vigilio-browser/tests/unit/CoreTests.cpp`

- [ ] TASK-006: Ejecutar la suite completa de calidad local (`apps/vigilio-browser/scripts/check.sh`), linters y compilación bajo `-Werror`
  - *Criterio de verificación*: Compilación limpia con Ninja bajo `-Werror`, cero advertencias y 100% de pruebas de CTest pasadas con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Sincronizar la documentación canónica de la microtarea en `docs/rules/01-modules/foundations/micro/0010-qt-dependency-validation/task.md` y verificar gobernanza con Prompt Builder
  - *Criterio de verificación*: La microtarea canónica 0010 actualiza su estado a `CURRENT; PASS` tras la aprobación de APPLY y pasa satisfactoriamente `validate-references`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
