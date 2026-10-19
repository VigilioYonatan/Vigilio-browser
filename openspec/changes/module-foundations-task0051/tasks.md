# Tasks — module-foundations-task0051

Status: PROPOSED

- [ ] TASK-001: Auditar y validar los contratos de interfaz de usuario y la declaración formal de «sin cambio visual directo» para CI Linux job conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Confirmación estática de que `BrowserWindow.h` y `BrowserWindow.cpp` no contienen widgets, botones, barras de progreso de compilación, consolas de terminal ni etiquetas con terminología interna («CI Linux job», «Pipeline», «Build Step», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»), respetando la denominación aprobada «Interfaz más fácil de usar».
  - *Comando*: `! grep -E "CiLinux|ci-linux|CiPipeline|BuildStep|Senior Mode|Prompt Builder|OpenSpec" apps/vigilio-browser/src/ui/BrowserWindow.h apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-002: Integrar las cabeceras canónicas de `CiLinuxJobService.h`, `CiLinuxJobValidator.h`, `CiLinuxJobConfiguration.h` y `CiLinuxJobTypes.h` en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` incluye formalmente `#include "core/CiLinuxJobService.h"`, `#include "core/CiLinuxJobValidator.h"`, `#include "core/CiLinuxJobConfiguration.h"` y `#include "core/CiLinuxJobTypes.h"`, accediendo a la infraestructura de automatización de compilación y calidad de `vigilio_core`.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/CiLinuxJobService\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/CiLinuxJobTypes\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-003: Incorporar la validación programática en modo simulación dry-run del pipeline canónico de CI Linux en `WindowTests.cpp` mediante `CiLinuxJobService`
  - *Criterio de verificación*: `WindowTests.cpp` instancia `vigilio::CiLinuxJobService` con `canonicalDebugPlan()` en modo `CiLinuxExecutionMode::DryRun` y ejecuta `executePlan()`, comprobando mediante `UI_CHECK` que el reporte inmutable devuelve `status == CiLinuxJobStatus::Passed`, 8 etapas evaluadas satisfactoriamente (`steps.size() == 8`), duración total $\ge 0$ ms y error `None`.
  - *Comando*: `grep -E "ciLinuxService\.executePlan\(\)|CiLinuxJobStatus::Passed" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-004: Incorporar la verificación de cobertura de compilación y artefactos de interfaz gráfica en el pipeline canónico de CI Linux en `WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` verifica programáticamente que el plan canónico de CI Linux contiene las etapas obligatorias de compilación (`BuildCompilation`) y pruebas (`TestSuiteExecution`), asegurando que la capa gráfica (`BrowserWindow.cpp`, `vigilio_ui_tests`, `resources.qrc`) está gobernada y protegida contra regresiones por el pipeline.
  - *Comando*: `grep -E "CiLinuxPipelineStep::BuildCompilation|CiLinuxPipelineStep::TestSuiteExecution" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-005: Incorporar la auditoría de textos y descripciones en español neutro de `CiLinuxJobService::executionModeToString` y `formatStepSummary` en `WindowTests.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `WindowTests.cpp` valida mediante `UI_CHECK` que los modos de ejecución (`DryRun`, `Simulated`, `Live`) devuelven descripciones claras en español neutro («Simulación previa sin cambios», «Simulación de ejecución», «Ejecución real en el sistema») y que el resumen estructurado de etapas no está vacío y no contiene jerga en inglés.
  - *Comando*: `grep -E "CiLinuxJobService::executionModeToString|CiLinuxJobService::formatStepSummary" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-006: Incorporar la auditoría de mensajes públicos de diagnóstico, estados y remediación de `CiLinuxJobValidator` en `WindowTests.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `WindowTests.cpp` valida que `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint` devuelven mensajes en lenguaje natural comprensible en español («Trabajo completado con éxito», «El trabajo fue cancelado», «No se pudo validar el entorno», etc.).
  - *Comando*: `grep -E "CiLinuxJobValidator::jobStatusToString|CiLinuxJobValidator::generateRemediationHint" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-007: Incorporar la auditoría de ausencia total de terminología de ingeniería interna prohibida en todas las cadenas públicas de CI Linux en `WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` recorre todas las enumeraciones de estado de trabajo, errores y modos de ejecución de CI Linux, certificando que ninguna cadena ni sugerencia pública contiene términos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «CI Linux job»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -v UI_CHECK`

- [ ] TASK-008: Auditar la tolerancia a entornos headless en `WindowTests.cpp` con reporte honesto de `NOT_EXECUTED`
  - *Criterio de verificación*: La detección preliminar de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) previene la inicialización de `QApplication` en entornos headless, retornando exitosamente código 0 y emitiendo el mensaje estructurado `WindowTests: GUI display not available (no DISPLAY / WAYLAND_DISPLAY) - NOT_EXECUTED`.
  - *Comando*: `grep -A 8 "main(int argc, char\*\* argv)" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "NOT_EXECUTED"`

- [ ] TASK-009: Auditar la compatibilidad, controles accesibles y dimensionamiento para «Interfaz más fácil de usar» (Modo Senior) en `WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` verifica el escalado dinámico de controles y tipografías (altura mínima de botones $\ge 48$ px, toolbar $\ge 55$ px, banner $\ge 56$ px, escalado de iconos 22×22 $\to$ 32×32 px y reversión limpia a modo estándar).
  - *Comando*: `grep -E "setSeniorMode.*true|minimumHeight.*>=.*56|minimumHeight.*>=.*48" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-010: Auditar el enlace de bibliotecas y configuración en `apps/vigilio-browser/CMakeLists.txt` para `vigilio_ui_tests`
  - *Criterio de verificación*: `CMakeLists.txt` asegura que `vigilio_ui_tests` enlaza privadamente `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test` bajo la guarda `if(TARGET Qt6::Test)` con label `ui;fast`.
  - *Comando*: `grep -A 6 "add_executable(vigilio_ui_tests" apps/vigilio-browser/CMakeLists.txt | grep "vigilio_core"`

- [ ] TASK-011: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (21/21 tests) en menos de 5 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-012: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0051-ci-linux-job/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0151, y `task.md` formaliza el cumplimiento de la etapa `ui`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0151-module.foundations.task0051.propose.lock.json --project-root . && grep -E "module.foundations.task0051" docs/rules/01-modules/foundations/micro/0051-ci-linux-job/task.md`
