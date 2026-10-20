# Tasks — module-ux-shell-task0066

Status: PROPOSED

- [ ] TASK-001: Declarar miembros, métodos de barra de título y cabeceras de integración en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: `BrowserWindow.h` incluye `core/TitleBarService.h`, `core/TitleBarTypes.h` y `core/TitleBarConfiguration.h`, declara el puntero `titleBarService_`, los widgets de barra de título `titleBarWidget_`, `titleBarIcon_`, `titleBarTitle_`, `titleBarDragArea_`, `titleBarControlsBox_`, los botones de control `titleBarMinimizeBtn_`, `titleBarMaximizeBtn_`, `titleBarCloseBtn_`, `titleBarSeniorToggleBtn_`, los métodos privados `setupTitleBar`, `applyTitleBarPlan`, `updateTitleBarWindowState`, sobrescribe `changeEvent(QEvent*)` y declara los métodos públicos de consulta `titleBarService()`, `titleBarWidget()`, `titleBarMinimizeButton()`, `titleBarMaximizeButton()`, `titleBarCloseButton()` y `titleBarSeniorToggleButton()`.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.h && grep -E "TitleBarService|titleBarWidget_|titleBarMinimizeBtn_|titleBarMaximizeBtn_|titleBarCloseBtn_|changeEvent" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar la inicialización y orquestación del servicio de barra de título en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setupTitleBar` instancia `titleBarService_` con el plan canónico apropiado (`canonicalDefaultTitleBar()` o `canonicalSeniorTitleBar()`), configura el modo incógnito (`setIncognitoMode(isIncognito_)`), define la ruta de persistencia local `title_bar_state.json` y registra el callback `setTitleBarChangedCallback`.
  - *Comando*: `grep -E "make_unique<vigilio::TitleBarService>|setTitleBarChangedCallback|canonicalDefaultTitleBar|canonicalSeniorTitleBar" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Construir la jerarquía visual de `vigilio-titlebar` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setupUi` crea el contenedor `QFrame#vigilio-titlebar`, incorpora el icono de marca `QLabel#vigilio-titlebar-icon` con `branding/vigilio-logo.svg`, el título sanitizado `QLabel#vigilio-titlebar-title`, el área de arrastre `QWidget#vigilio-titlebar-drag-area` y la caja de controles `QWidget#vigilio-titlebar-controls`, insertándolo en la posición superior de `mainContainer_`.
  - *Comando*: `grep -E "vigilio-titlebar|vigilio-titlebar-icon|vigilio-titlebar-title|vigilio-titlebar-drag-area" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar los botones de control de ventana y sus conexiones en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Se crean los botones `vigilio-titlebar-minimize`, `vigilio-titlebar-maximize`, `vigilio-titlebar-close` y `vigilio-titlebar-senior-toggle`, conectando sus señales `clicked` a `showMinimized()`, alternar `isMaximized() ? showNormal() : showMaximized()`, `close()` y `toggleSeniorMode()` respectivamente, con etiquetas y descripciones accesibles según `TitleBarValidator`.
  - *Comando*: `grep -E "vigilio-titlebar-minimize|vigilio-titlebar-maximize|vigilio-titlebar-close|vigilio-titlebar-senior-toggle" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Implementar el soporte de arrastre interactivo y doble clic en el área de arrastre de `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: El área de arrastre o su filtro de eventos maneja `mousePressEvent`, `mouseMoveEvent` para mover la ventana de forma fluida, y `mouseDoubleClickEvent` para alternar entre tamaño normal y maximizado.
  - *Comando*: `grep -A 20 "mouseMoveEvent\|mouseDoubleClickEvent" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-006: Implementar `applyTitleBarPlan` para sincronizar geometría y etiquetas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `applyTitleBarPlan` ajusta la altura de `titleBarWidget_` a `plan.height`, actualiza dimensiones y visibilidad de botones, refresca el icono según `plan.density` y actualiza tooltips y nombres accesibles sin términos proscritos.
  - *Comando*: `grep -A 25 "BrowserWindow::applyTitleBarPlan" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "setFixedHeight|buttonAccessibleLabel"`

- [ ] TASK-007: Sobrescribir `changeEvent` para sincronizar cambios de estado de ventana en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `changeEvent` intercepta `QEvent::WindowStateChange` y `QEvent::ActivationChange`, actualiza `TitleBarState` (`isMaximized`, `isActive`, `isFullscreen`) en `titleBarService_` y actualiza el icono/tooltip del botón maximizar/restaurar.
  - *Comando*: `grep -A 15 "BrowserWindow::changeEvent" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "WindowStateChange|ActivationChange"`

- [ ] TASK-008: Sincronizar eventos de tamaño de ventana con el servicio de barra de título en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `resizeEvent` invoca `titleBarService_->setWindowWidth(width())` junto con el redimensionamiento del servicio de layout.
  - *Comando*: `grep -A 10 "BrowserWindow::resizeEvent" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "titleBarService_.*setWindowWidth"`

- [ ] TASK-009: Implementar la adaptación reactiva a la «Interfaz más fácil de usar» en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setSeniorMode(bool)` sincroniza la densidad en `titleBarService_` (`LayoutDensity::Senior` vs `LayoutDensity::Default`), eleva la altura de la barra a 48 px y dimensiona los botones de la barra a `minimumHeight >= 48` y `minimumWidth >= 48`.
  - *Comando*: `grep -A 30 "BrowserWindow::setSeniorMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "titleBarService_.*setDensity"`

- [ ] TASK-010: Enriquecer los estilos QSS (claro y oscuro) para la barra de título en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setDarkTheme(bool)` añade reglas completas para `QFrame#vigilio-titlebar`, botones de control, estados `:hover`, `:pressed`, botón de cierre con hover rojo `#DC2626` y borde de foco accesible de 2 px.
  - *Comando*: `grep -E "QFrame#vigilio-titlebar|vigilio-titlebar-close" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-011: Añadir pruebas de presencia e inicialización de la barra de título en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` contiene aserciones que verifican la presencia no nula de `window.titleBarService()`, `window.titleBarWidget()`, `window.titleBarMinimizeButton()`, `window.titleBarMaximizeButton()` y `window.titleBarCloseButton()`.
  - *Comando*: `grep -E "window.titleBarService\(\)|window.titleBarWidget\(\)|window.titleBarCloseButton\(\)" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-012: Añadir pruebas de dimensiones canónicas y adaptación a la «Interfaz más fácil de usar» en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida que la altura estándar de `titleBarWidget()` es 32 px y tras activar `setSeniorMode(true)` asciende a 48 px con botones cumpliendo `minimumHeight() >= 48`.
  - *Comando*: `grep -A 15 "setSeniorMode" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "titleBarWidget.*height|minimumHeight"`

- [ ] TASK-013: Añadir pruebas de sincronización de estado de ventana y auditoría de textos accesibles en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la alternancia de etiquetas al maximizar/restaurar y audita que ninguna etiqueta o tooltip de la barra de título contenga «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG» o «Linter».
  - *Comando*: `grep -E "vigilio-titlebar|titleBarMaximizeButton" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "Senior Mode|Modo Senior"`

- [ ] TASK-014: Compilar limpiamente y ejecutar la suite completa de pruebas en CTest mediante `check.sh`
  - *Criterio de verificación*: `apps/vigilio-browser/scripts/check.sh` compila sin advertencias con `-Werror`, Clang-Format valida formato limpio y CTest pasa el 100% de las pruebas con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-015: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0066-title-bar-integration/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `ui` y mantiene sincronizados los criterios de verificación.
  - *Comando*: `grep -E "module.ux-shell.task0066" docs/rules/01-modules/ux-shell/micro/0066-title-bar-integration/task.md`
