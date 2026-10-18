# Tasks — module-foundations-task0003

Status: PROPOSED

- [ ] TASK-001: Auditar y formalizar la configuración del target de UI `vigilio-browser` y recursos `resources.qrc` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: El target `vigilio-browser` enlaza de forma privada las bibliotecas necesarias (`vigilio_core`, `vigilio_storage`, `Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`) bajo la guarda `VIGILIO_BUILD_GUI=ON` y `Qt6_FOUND`, con `resources.qrc` compilado vía `CMAKE_AUTORCC ON` y propiedades de ejecutable nativo de escritorio (`WIN32_EXECUTABLE`, `MACOSX_BUNDLE`).
  - *Comando*: `cmake --preset core-debug`

- [ ] TASK-002: Estructurar la definición condicional del arnés de pruebas de interfaz de usuario (`vigilio_ui_tests`) en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: Bajo las guardas `BUILD_TESTING`, `VIGILIO_BUILD_GUI` y disponibilidad de `Qt6::Test`, se define un target de pruebas de interacción de ventana con etiquetas CTest `ui;fast` y timeout de 20s; en ausencia de paquetes de desarrollo de Qt6 se emite mensaje informativo sin interrumpir la compilación headless.
  - *Comando*: `cmake --preset desktop-debug`

- [ ] TASK-003: Auditar las cadenas públicas de la interfaz de usuario en `src/ui/BrowserWindow.cpp` contra `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Ninguna cadena visible al usuario contiene términos técnicos prohibidos («Modo Senior», «Senior Mode», «cmake», «targets», «DAG», «Prompt Builder», «OpenSpec»), empleando exclusivamente la etiqueta aprobada «Interfaz más fácil de usar» y lenguaje natural claro.
  - *Comando*: Inspección y verificación de cadenas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`.

- [ ] TASK-004: Ejecutar la validación automatizada del proyecto mediante el script de comprobación local
  - *Criterio de verificación*: Compilación limpia con Ninja en modo headless y paso del 100% de las pruebas rápidas registradas (`core_units` y `storage_integration`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-005: Sincronizar la documentación canónica de la microtarea y validar referencias
  - *Criterio de verificación*: El archivo `docs/rules/01-modules/foundations/micro/0003-cmake-targets/task.md` actualiza su estado de ejecución y pasa la validación canónica de referencias de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
