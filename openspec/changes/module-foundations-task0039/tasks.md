# Tasks — module-foundations-task0039

Status: PROPOSED

- [ ] TASK-001: Auditar y validar los contratos de interfaz de usuario y la declaración formal de «sin cambio visual directo» para test fixture foundation conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Confirmación estática de que `BrowserWindow.h` y `BrowserWindow.cpp` no contienen widgets, botones, menús de depuración ni etiquetas con terminología interna («Test Fixture», «Fixture Foundation», «Senior Mode», «Prompt Builder», «DAG», «OpenSpec»), respetando la denominación aprobada «Interfaz más fácil de usar».
  - *Comando*: `! grep -E "TestFixture|FixtureFoundation|Senior Mode|Prompt Builder|OpenSpec" apps/vigilio-browser/src/ui/BrowserWindow.h apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-002: Integrar las cabeceras canónicas de `TestFixtureFoundation`, `TestFixtureValidator` y `TestFixtureService` en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` incluye formalmente `#include "core/TestFixtureFoundation.h"`, `#include "core/TestFixtureValidator.h"` y `#include "core/TestFixtureService.h"`, accediendo a la infraestructura de fixtures de `vigilio_core`.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/TestFixtureFoundation\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "#include \"core/TestFixtureValidator\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-003: Reemplazar la estructura ad-hoc `TempDirFixture` en `apps/vigilio-browser/tests/ui/WindowTests.cpp` por `vigilio::TestFixtureFoundation`
  - *Criterio de verificación*: Eliminación completa de `struct TempDirFixture` en `WindowTests.cpp` y adopción de `vigilio::TestFixtureFoundation` con gestión RAII limpia y prefijo canónico `vigilio_test_` en todas las pruebas de crash recovery y persistencia temporal.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "vigilio::TestFixtureFoundation" apps/vigilio-browser/tests/ui/WindowTests.cpp && ! grep -E "struct TempDirFixture" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-004: Incorporar la validación de seguridad de rutas `TestFixtureValidator::isSafeTestDirectory` en los bloques de prueba de UI de `WindowTests.cpp`
  - *Criterio de verificación*: Cada bloque de prueba de recuperación ante caídas valida formalmente que la ruta temporal generada resida bajo `temp_directory_path()` y comience con `vigilio_test_` mediante `UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()), ...)`.
  - *Comando*: `grep -E "TestFixtureValidator::isSafeTestDirectory" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-005: Revalidar las pruebas de recuperación ante caídas sobre fixtures seguros en `WindowTests.cpp`
  - *Criterio de verificación*: Las 5 pruebas de crash recovery (`vigilio_ui_crash_abnormal`, `vigilio_ui_crash_clean`, `vigilio_ui_crash_incognito`, `vigilio_ui_crash_loop` y `vigilio_ui_crash_dismiss`) ejecutan sobre instancias seguras de fixtures, validando la interacción con los botones «Restaurar pestañas», «Empezar de cero» y «Cerrar aviso».
  - *Comando*: `grep -E "vigilio_ui_crash_abnormal|vigilio_ui_crash_clean|vigilio_ui_crash_incognito|vigilio_ui_crash_loop|vigilio_ui_crash_dismiss" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-006: Auditar la tolerancia a entornos headless en `WindowTests.cpp` con reporte honesto de `NOT_EXECUTED`
  - *Criterio de verificación*: La detección preliminar de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) previene la inicialización de `QApplication` en entornos headless, retornando exitosamente código 0 y emitiendo el mensaje estructurado `WindowTests: GUI display not available (no DISPLAY / WAYLAND_DISPLAY) - NOT_EXECUTED`.
  - *Comando*: `grep -A 8 "main(int argc, char\*\* argv)" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "NOT_EXECUTED"`

- [ ] TASK-007: Auditar la compatibilidad y dimensionamiento para «Interfaz más fácil de usar» (Modo Senior) en `WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` verifica el escalado dinámico de controles y tipografías (altura mínima de botones $\ge 48$ px, toolbar $\ge 55$ px, banner $\ge 56$ px, escalado de iconos 22×22 $\to$ 32×32 px y reversión limpia a modo estándar).
  - *Comando*: `grep -E "setSeniorMode.*true|minimumHeight.*>=.*56|minimumHeight.*>=.*48" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-008: Auditar el enlace de bibliotecas y guardas condicionales en `apps/vigilio-browser/CMakeLists.txt` para `vigilio_ui_tests`
  - *Criterio de verificación*: `CMakeLists.txt` asegura que `vigilio_ui_tests` enlaza privadamente `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test` bajo la guarda `if(TARGET Qt6::Test)` con label `ui;fast`.
  - *Comando*: `grep -A 6 "add_executable(vigilio_ui_tests" apps/vigilio-browser/CMakeLists.txt | grep "vigilio_core"`

- [ ] TASK-009: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (15/15 tests) en menos de 1 segundo.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0115.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0115-module.foundations.task0039.propose.lock.json --project-root .`

- [ ] TASK-011: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0039-test-fixture-foundation/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `ui` y se mantiene alineado con los contratos y el arnés de servicio acordados.
  - *Comando*: `grep -E "module.foundations.task0039" docs/rules/01-modules/foundations/micro/0039-test-fixture-foundation/task.md`
