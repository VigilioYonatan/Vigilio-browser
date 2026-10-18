# Tasks — module-foundations-task0035

Status: PROPOSED

- [ ] TASK-001: Auditar y verificar las propiedades de compilación de los targets de interfaz en `apps/vigilio-browser/CMakeLists.txt` bajo la guarda `VIGILIO_BUILD_GUI` y presets de escritorio (`desktop-debug`, `desktop-release`)
  - *Criterio de verificación*: `CMakeLists.txt` compila el ejecutable principal `vigilio-browser` y la suite `vigilio_ui_tests` vinculados a `vigilio_core` (que contiene `CMakePresetsService`), `vigilio_storage`, `vigilio_compiler_warnings` y las bibliotecas Qt6 correspondientes, con `CMAKE_AUTOMOC ON` y `CMAKE_AUTORCC ON` activos para componentes gráficos.
  - *Comando*: `grep -A 15 "if(VIGILIO_BUILD_GUI)" apps/vigilio-browser/CMakeLists.txt && grep -E "vigilio_core vigilio_storage vigilio_compiler_warnings" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-002: Verificar la correspondencia entre los perfiles de interfaz gráfica en `apps/vigilio-browser/CMakePresets.json` y la configuración de CMake
  - *Criterio de verificación*: `CMakePresets.json` define deterministamente los perfiles de escritorio `desktop-debug`, `desktop-release` y `desktop-e2e` con `VIGILIO_BUILD_GUI: "ON"`, y los perfiles headless (`core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`) con `VIGILIO_BUILD_GUI: "OFF"`, garantizando la compilación autónoma de ambos entornos.
  - *Comando*: `grep -E '"name": "(desktop-debug|desktop-release|desktop-e2e|core-debug)"' apps/vigilio-browser/CMakePresets.json`

- [ ] TASK-003: Auditar el cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md` en todas las cadenas de texto y componentes visuales de `apps/vigilio-browser/src/ui/`
  - *Criterio de verificación*: Cero ocurrencias de nombres de ingeniería interna como «CMake», «presets», «toolchain», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» en archivos bajo `src/ui/`, garantizando que la capacidad CMake Presets respete el contrato de «sin cambio visual directo» en la interfaz pública del navegador.
  - *Comando*: `! grep -inE "(Senior Mode|Modo Senior|Prompt Builder|OpenSpec|toolchain)" apps/vigilio-browser/src/ui/BrowserWindow.h apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Verificar la suite de pruebas de interacción de interfaz en `apps/vigilio-browser/tests/ui/WindowTests.cpp` y la tolerancia a entornos headless
  - *Criterio de verificación*: `WindowTests.cpp` implementa detección determinista de display gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) con reporte honesto de `NOT_EXECUTED` si falta servidor gráfico, verificando la inicialización de ventana, logo corporate, acciones QRC, accesibilidad asistiva y escalado de la «Interfaz más fácil de usar».
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "WAYLAND_DISPLAY|NOT_EXECUTED|setSeniorMode|vigilio-home" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-005: Comprobar el desacoplamiento arquitectónico de `vigilio::CMakePresetsService` respecto a la interfaz gráfica
  - *Criterio de verificación*: `CMakePresetsService.h` y `CMakePresetsTypes.h` no incluyen cabeceras de Qt Widgets (`QWidget`, `QMainWindow`, `QAction`, `QMenu`) ni bibliotecas gráficas, manteniendo `vigilio_core` desacoplado de la capa de interfaz de usuario.
  - *Comando*: `! grep -inE "(QWidget|QMainWindow|QAction|QMenu|QApplication)" apps/vigilio-browser/src/core/CMakePresetsService.h apps/vigilio-browser/src/core/CMakePresetsTypes.h`

- [ ] TASK-006: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests en CTest aprobados exitosamente (13/13 tests, exit code 0 en < 1s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0103.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0103-module.foundations.task0035.propose.lock.json --project-root .`

- [ ] TASK-008: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0035-cmake-presets/task.md`
  - *Criterio de verificación*: El documento de microtarea refleja el estado y alcance formalizados para la etapa UI de CMake Presets.
  - *Comando*: `grep -E "module.foundations.task0035" docs/rules/01-modules/foundations/micro/0035-cmake-presets/task.md`
