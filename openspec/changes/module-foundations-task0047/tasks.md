# Tasks — module-foundations-task0047

Status: PROPOSED

- [ ] TASK-001: Auditar y validar los contratos de interfaz de usuario y la declaración formal de «sin cambio visual directo» para clang-tidy configuration conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Confirmación estática de que `BrowserWindow.h` y `BrowserWindow.cpp` no contienen widgets, botones, menús de linteo ni etiquetas con terminología interna («Clang-Tidy», «Linter», «Static Analysis», «AST», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»), respetando la denominación aprobada «Interfaz más fácil de usar».
  - *Comando*: `! grep -E "ClangTidy|clang-tidy|CodeLinter|Linter|Senior Mode|Prompt Builder|OpenSpec" apps/vigilio-browser/src/ui/BrowserWindow.h apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-002: Integrar las cabeceras canónicas de `ClangTidyService`, `ClangTidyValidator`, `ClangTidyConfiguration` y `ClangTidyTypes.h` en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` incluye formalmente `#include "core/ClangTidyService.h"`, `#include "core/ClangTidyValidator.h"`, `#include "core/ClangTidyConfiguration.h"` y `#include "core/ClangTidyTypes.h"`, accediendo a la infraestructura de calidad y linteo estático de `vigilio_core`.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/ClangTidyService\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/ClangTidyTypes\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-003: Incorporar la inspección programática de conformidad estática (dry-run) sobre `BrowserWindow.h` en `WindowTests.cpp` mediante `ClangTidyService`
  - *Criterio de verificación*: `WindowTests.cpp` instancia `vigilio::ClangTidyService` y ejecuta `inspectFile` sobre la cabecera de la ventana principal, comprobando mediante `UI_CHECK` que la inspección devuelva `isClean == true` con cero incidencias (`issuesCount == 0`).
  - *Comando*: `grep -E "tidyService\.inspectFile.*BrowserWindow\.h" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-004: Incorporar la inspección programática de conformidad estática (dry-run) sobre `BrowserWindow.cpp` en `WindowTests.cpp` mediante `ClangTidyService`
  - *Criterio de verificación*: `WindowTests.cpp` ejecuta `inspectFile` sobre la implementación de la ventana principal, comprobando mediante `UI_CHECK` que la inspección devuelva `isClean == true` con cero incidencias estáticas respecto a la configuración canónica.
  - *Comando*: `grep -E "tidyService\.inspectFile.*BrowserWindow\.cpp" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-005: Incorporar la inspección programática de conformidad estática (dry-run) sobre `WindowTests.cpp` en `WindowTests.cpp` mediante `ClangTidyService`
  - *Criterio de verificación*: `WindowTests.cpp` evalúa su propio archivo fuente con `inspectFile`, asegurando que el arnés de pruebas de interfaz cumple rigurosamente las reglas de linteo estático sin desviaciones ni advertencias.
  - *Comando*: `grep -E "tidyService\.inspectFile.*WindowTests\.cpp" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-006: Incorporar la verificación de textos y cadenas de diagnóstico en español neutro de `ClangTidyService::executionStatusToString` y `generateRemediationHint` en `WindowTests.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `WindowTests.cpp` valida mediante `UI_CHECK` que los mensajes de estado y remediación para `Success`, `IssuesFound`, `Cancelled` y `FileLocked` devuelven texto en lenguaje natural en español neutro («Análisis de código completado correctamente», «Se detectaron posibles mejoras», «El análisis de código fue cancelado», etc.) y no contienen términos en inglés ni jerga técnica no autorizada.
  - *Comando*: `grep -E "ClangTidyService::executionStatusToString" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-007: Auditar la tolerancia a entornos headless en `WindowTests.cpp` con reporte honesto de `NOT_EXECUTED`
  - *Criterio de verificación*: La detección preliminar de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) previene la inicialización de `QApplication` en entornos headless, retornando exitosamente código 0 y emitiendo el mensaje estructurado `WindowTests: GUI display not available (no DISPLAY / WAYLAND_DISPLAY) - NOT_EXECUTED`.
  - *Comando*: `grep -A 8 "main(int argc, char\*\* argv)" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "NOT_EXECUTED"`

- [ ] TASK-008: Auditar la compatibilidad, controles accesibles y dimensionamiento para «Interfaz más fácil de usar» (Modo Senior) en `WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` verifica el escalado dinámico de controles y tipografías (altura mínima de botones $\ge 48$ px, toolbar $\ge 55$ px, banner $\ge 56$ px, escalado de iconos 22×22 $\to$ 32×32 px y reversión limpia a modo estándar).
  - *Comando*: `grep -E "setSeniorMode.*true|minimumHeight.*>=.*56|minimumHeight.*>=.*48" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Auditar el enlace de bibliotecas y guardas condicionales en `apps/vigilio-browser/CMakeLists.txt` para `vigilio_ui_tests`
  - *Criterio de verificación*: `CMakeLists.txt` asegura que `vigilio_ui_tests` enlaza privadamente `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test` bajo la guarda `if(TARGET Qt6::Test)` con label `ui;fast`.
  - *Comando*: `grep -A 6 "add_executable(vigilio_ui_tests" apps/vigilio-browser/CMakeLists.txt | grep "vigilio_core"`

- [ ] TASK-010: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (19/19 tests) en menos de 3 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-011: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las 47 referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0139.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0139-module.foundations.task0047.propose.lock.json --project-root .`

- [ ] TASK-012: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0047-clang-tidy-configuration/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `ui` y se mantiene alineado con los contratos y el arnés de servicio acordados.
  - *Comando*: `grep -E "module.foundations.task0047" docs/rules/01-modules/foundations/micro/0047-clang-tidy-configuration/task.md`
