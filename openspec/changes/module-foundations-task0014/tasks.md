# Tasks — module-foundations-task0014

Status: PROPOSED

- [ ] TASK-001: Implementar la cabecera de tipos de dominio y API de servicio en `apps/vigilio-browser/src/core/WebEngineVersionValidator.h`
  - *Criterio de verificación*: Estructuración inmutable en C++20 de `ChromiumVersion` (con `operator<=>`, `milestone()`, `isCompatibleWith`), enum classes `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, estructura de reporte `WebEngineCompatibilityReport` y declaraciones estáticas de `WebEngineVersionValidator`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/WebEngineVersionValidator.h && grep -E "struct ChromiumVersion|enum class WebEngineFeature|class WebEngineVersionValidator" apps/vigilio-browser/src/core/WebEngineVersionValidator.h`

- [ ] TASK-002: Implementar la lógica del servicio desacoplado en `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp`
  - *Criterio de verificación*: Implementación completa de parsing cuatripartito, matriz de capacidades para Chromium milestone >= 128, requerimientos por perfil, comprobación de existencia y permisos del ejecutable asistente `QtWebEngineProcess`, y formateo legible de diagnósticos y remediaciones.
  - *Comando*: `test -f apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp && grep -E "parseChromiumVersion|supportedFeaturesForMilestone|validateCompatibility" apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp`

- [ ] TASK-003: Registrar `WebEngineVersionValidator.cpp` en el target estático `vigilio_core` de `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: El archivo `CMakeLists.txt` incluye `src/core/WebEngineVersionValidator.cpp` dentro de la definición de `vigilio_core`, compilando con `-Werror` bajo el estándar C++20 sin incluir cabeceras de `Qt6::WebEngineWidgets`.
  - *Comando*: `grep -A 8 "add_library(vigilio_core STATIC" apps/vigilio-browser/CMakeLists.txt | grep "WebEngineVersionValidator.cpp"`

- [ ] TASK-004: Integrar la comprobación de compatibilidad de WebEngine en el punto de arranque `apps/vigilio-browser/src/app.cpp`
  - *Criterio de verificación*: `app.cpp` invoca `WebEngineVersionValidator::validateStartup` tras la validación de dependencias Qt, emitiendo diagnósticos claros en `stderr` con código de salida 1 en caso de fallo crítico en modo desktop, o permitiendo degradación controlada.
  - *Comando*: `grep -E "WebEngineVersionValidator::validateStartup" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-005: Implementar suite de pruebas unitarias `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` y registrarla en CTest
  - *Criterio de verificación*: Suite de pruebas unitarias que cubre parsing (casos válidos e inválidos), comparaciones de versión, matriz de capacidades, perfiles, verificación de proceso asistente y generación de reportes, registrada como `webengine_version_validator_tests` con etiquetas `unit;fast`.
  - *Comando*: `grep "webengine_version_validator_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Ejecutar la suite de pruebas local automatizada y verificar compilación limpia bajo opciones estrictas `-Werror`
  - *Criterio de verificación*: Compilación exitosa con Ninja y 100% de pruebas pasadas en CTest sin advertencias de compilación y con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Sincronizar la documentación canónica de la microtarea y validar las referencias canónicas del árbol OpenSpec con Prompt Builder
  - *Criterio de verificación*: El archivo de referencia canónica `canonical-reference.yaml` y la propuesta OpenSpec pasan satisfactoriamente la validación automatizada de referencias canónicas mediante la herramienta Prompt Builder sin discrepancias.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
