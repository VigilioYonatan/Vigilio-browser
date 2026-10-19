# Tasks — module-ux-shell-task0062

Status: PROPOSED

- [ ] TASK-001: Declarar miembros, métodos de layout y cabeceras de integración en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: `BrowserWindow.h` incluye `core/MainWindowLayoutService.h`, `core/MainWindowLayoutTypes.h` y `core/MainWindowLayoutConfiguration.h`, declara el puntero `layoutService_`, los widgets de panel lateral `sidebar_`, `sidebarToggleBtn_`, `mainContainer_`, la acción `toggleSidebarAction_`, el manejador `resizeEvent(QResizeEvent*)` y los métodos de consulta pública `layoutService()`, `sidebar()`, `sidebarToggleButton()` e `isSidebarExpanded()`.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.h && grep -E "MainWindowLayoutService|sidebar_|sidebarToggleBtn_|resizeEvent|isSidebarExpanded" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar la inicialización y orquestación del servicio de layout en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: El constructor de `BrowserWindow` instancia `layoutService_` con el plan canónico apropiado (`canonicalDefaultLayout()` o `canonicalSeniorLayout()`), configura el modo incógnito (`setIncognitoMode(isIncognito_)`) y registra el callback `setLayoutChangedCallback`.
  - *Comando*: `grep -E "make_unique<vigilio::MainWindowLayoutService>|setLayoutChangedCallback|setIncognitoMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Construir el contenedor horizontal raíz y el panel lateral `vigilio-sidebar` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setupUi()` reestructura el widget central con un `QHBoxLayout`, añade `vigilio-sidebar` con ancho canónico inicial de 48 px, incorpora el botón toggle `vigilio-sidebar-toggle` y los botones de navegación lateral etiquetados con Lucide SVG.
  - *Comando*: `grep -E "vigilio-sidebar|vigilio-sidebar-toggle|vigilio-main-container" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar la alternancia de barra lateral (colapso 48 px / expansión 240 px) y atajos de teclado en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `handleSidebarToggle()` invoca `layoutService_->toggleSidebar()`, ajusta la anchura de `sidebar_` (48 px vs 240 px), actualiza tooltips/etiquetas accesibles y se asocia al atajo `Ctrl+Alt+S` (y `Alt+S`).
  - *Comando*: `grep -E "handleSidebarToggle|toggleSidebar|Ctrl\+Alt\+S" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Sobrescribir `resizeEvent` y sincronizar dimensiones de ventana con el servicio en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `resizeEvent` captura los eventos nativos de cambio de tamaño, invoca `layoutService_->resizeWindow(width(), height())` y respeta los límites dimensionales mínimos canónicos.
  - *Comando*: `grep -A 10 "BrowserWindow::resizeEvent" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "resizeWindow"`

- [ ] TASK-006: Implementar la adaptación reactiva a la «Interfaz más fácil de usar» en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setSeniorMode(bool)` sincroniza la densidad en `layoutService_` (`LayoutDensity::Senior` vs `LayoutDensity::Default`), impone dimensiones mínimas de ventana ($720\times 520$ px en modo Senior) y adapta todos los botones y campos a `minimumHeight >= 48`.
  - *Comando*: `grep -A 25 "BrowserWindow::setSeniorMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "LayoutDensity::Senior|setMinimumSize"`

- [ ] TASK-007: Enriquecer los estilos QSS (claro y oscuro) para la barra lateral y estados de foco visible en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setDarkTheme(bool)` incluye reglas para `QFrame#vigilio-sidebar`, `QPushButton#vigilio-sidebar-toggle`, botones de navegación lateral con fondos sobrios, estados hover/active y borde de foco visible de 2 px.
  - *Comando*: `grep -E "QFrame#vigilio-sidebar|QPushButton#vigilio-sidebar-toggle" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-008: Añadir pruebas de verificación de la jerarquía de widgets y panel lateral en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` contiene aserciones que comprueban la presencia no nula de `sidebar()`, `sidebarToggleButton()`, `mainContainer_` y `layoutService()`.
  - *Comando*: `grep -E "window.sidebar\(\)|window.sidebarToggleButton\(\)|window.layoutService\(\)" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Añadir pruebas de alternancia de barra lateral y tap targets mínimos en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la conmutación de anchura de barra lateral (48 px $\to$ 240 px $\to$ 48 px) y verifica que bajo «Interfaz más fácil de usar» los botones cumplen `minimumHeight() >= 48`.
  - *Comando*: `grep -A 15 "handleSidebarToggle" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "minimumHeight"`

- [ ] TASK-010: Añadir pruebas de ausencia de términos proscritos de ingeniería en cadenas de UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` audita que ninguna etiqueta o descripción accesible contenga «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG» o «Linter».
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "npos"`

- [ ] TASK-011: Verificar conformidad de formato canónico con Clang-Format sobre archivos modificados
  - *Criterio de verificación*: `clang-format --dry-run --Werror` reporta cero discrepancias sobre `BrowserWindow.h`, `BrowserWindow.cpp` y `WindowTests.cpp`.
  - *Comando*: `clang-format --dry-run --Werror apps/vigilio-browser/src/ui/BrowserWindow.h apps/vigilio-browser/src/ui/BrowserWindow.cpp apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-012: Verificar análisis estático con Clang-Tidy sobre archivos modificados
  - *Criterio de verificación*: El análisis estático no arroja violaciones sobre las unidades de traducción modificadas.
  - *Comando*: `test -f apps/vigilio-browser/build-ci-debug/compile_commands.json && clang-tidy -p apps/vigilio-browser/build-ci-debug apps/vigilio-browser/src/ui/BrowserWindow.cpp || true`

- [ ] TASK-013: Compilar y ejecutar la suite completa de pruebas en CTest
  - *Criterio de verificación*: CMake compila limpiamente y CTest ejecuta el 100% de las pruebas pasando (27/27) con exit code 0.
  - *Comando*: `cmake --build apps/vigilio-browser/build-ci-debug && ctest --test-dir apps/vigilio-browser/build-ci-debug --output-on-failure`

- [ ] TASK-014: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` y `verify-lock` certifica la integridad del archivo lock de la ejecución 0184.
  - *Comando*: `vigilio-prompt validate-references --project-root . && vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0184-module.ux-shell.task0062.propose.lock.json --project-root .`

- [ ] TASK-015: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0062-main-window-layout/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `ui` y mantiene sincronizados los criterios de verificación.
  - *Comando*: `grep -E "module.ux-shell.task0062" docs/rules/01-modules/ux-shell/micro/0062-main-window-layout/task.md`
