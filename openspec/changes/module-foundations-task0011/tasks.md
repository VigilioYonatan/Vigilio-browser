# Tasks — module-foundations-task0011

Status: PROPOSED

- [ ] TASK-001: Auditar y validar la integración de dependencias Qt y aislamiento de cabeceras como `SYSTEM` en los targets de interfaz gráfica (`vigilio-browser` y `vigilio_ui_tests`) en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` mantiene vinculados `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y las bibliotecas Qt6 validadas (`Core`, `Widgets`, `WebEngineWidgets`) bajo la guarda condicional `VIGILIO_BUILD_GUI=ON` y `Qt6_FOUND`, con directorios de inclusión catalogados como `SYSTEM`.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-002: Auditar el protocolo de pre-validación de arranque en `apps/vigilio-browser/src/app.cpp` y el manejo estructurado de fallos de entorno
  - *Criterio de verificación*: `main()` invoca `QtDependencyValidator::validateRuntimeEnvironment` antes de inicializar `QApplication` y, ante cualquier resultado no satisfactorio (falta de display en Linux o desajuste de versión ABI), termina con mensaje estructurado a `std::cerr` y código de salida 1 sin crear widgets ni provocar fallos de segmentación.
  - *Comando*: `grep -A 10 "validateRuntimeEnvironment" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-003: Auditar la estabilidad de widgets y la preservación de los contratos de la «Interfaz más fácil de usar» en `apps/vigilio-browser/src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: La ventana principal no expone componentes de depuración técnica de Qt; `setSeniorMode()` aplica correctamente tipografías legibles de 14pt y controles con alturas mínimas de 48px en campos de entrada y 55px en barras de herramientas, preservando la accesibilidad y los atajos de teclado estándar (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`).
  - *Comando*: `grep -A 10 "setSeniorMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Auditar la conformidad de textos y etiquetas públicas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` contra `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Ninguna cadena visible al usuario contiene términos técnicos de ingeniería («Qt dependency validation», «Qt6», «QPA», «Senior Mode», «Modo Senior», «CMake», «DAG», «OpenSpec»), empleando exclusivamente la etiqueta aprobada «Interfaz más fácil de usar» y lenguaje natural claro.
  - *Comando*: `! grep -E -i "Senior Mode|Modo Senior|compiler warnings" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Verificar el arnés de pruebas de UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp` y el reporte transparente de dependencias
  - *Criterio de verificación*: La suite de pruebas de interfaz gráfica compila y ejecuta cuando `Qt6::Test` está presente; en entornos de compilación headless sin bibliotecas de desarrollo de Qt6, CMake omite la suite registrando honestamente `NOT_EXECUTED` sin generar falsos positivos con mocks artificiales.
  - *Comando*: `grep -A 5 "find_package(Qt6 QUIET COMPONENTS Test)" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Ejecutar la suite completa de calidad local (`apps/vigilio-browser/scripts/check.sh`), linters y compilación bajo `-Werror`
  - *Criterio de verificación*: Compilación limpia con Ninja bajo la política estricta `-Werror`, cero advertencias y 100% de pruebas de CTest pasadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Sincronizar la documentación canónica de la microtarea en `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md` y validar el árbol de referencias con Prompt Builder
  - *Criterio de verificación*: El archivo canónico `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md` actualiza su estado de ejecución tras la aprobación de APPLY y pasa satisfactoriamente `validate-references`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
