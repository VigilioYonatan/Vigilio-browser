# Tasks — module-ux-shell-task0070

Status: PROPOSED

- [ ] TASK-001: Declarar miembros, métodos de tira de pestañas y cabeceras de integración en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: `BrowserWindow.h` incluye `core/TabStripDensityService.h`, `core/TabStripDensityTypes.h` y `core/TabStripDensityConfiguration.h`, declara el miembro `std::unique_ptr<vigilio::TabStripDensityService> tabStripDensityService_`, el botón `QPushButton* tabNewButton_`, los métodos privados `setupTabStrip`, `applyTabStripDensityPlan`, `syncTabsToService` y los métodos públicos de consulta `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()`.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.h && grep -E "TabStripDensityService|tabStripDensityService_|tabNewButton_" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar la inicialización y orquestación del servicio de tira de pestañas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setupTabStrip` instancia `tabStripDensityService_` con el plan canónico apropiado (`canonicalDefaultDensity()` o `canonicalSeniorDensity()`), configura el modo de aislamiento de incógnito (`setIncognitoMode(isIncognito_)`) y registra el callback de observabilidad `setTabStripChangedCallback`.
  - *Comando*: `grep -E "make_unique<vigilio::TabStripDensityService>|setTabStripChangedCallback|canonicalDefaultDensity|canonicalSeniorDensity" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Construir el botón de nueva pestaña (`vigilio-tab-new-button`) e integrarlo como corner widget en `tabs_` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Se crea `tabNewButton_` con `objectName` `"vigilio-tab-new-button"`, se conecta a `openTab()`, se asignan las etiquetas y tooltips accesibles generados por `TabStripDensityValidator::newTabButtonAccessibleLabel()` y se ubica mediante `tabs_->setCornerWidget(tabNewButton_, Qt::TopRightCorner)`.
  - *Comando*: `grep -E "vigilio-tab-new-button|setCornerWidget|newTabButtonAccessibleLabel" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar `applyTabStripDensityPlan` para sincronizar alturas, dimensiones y etiquetas accesibles en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `applyTabStripDensityPlan` actualiza la altura de la tira según `plan.height` (36 px en estándar, 48 px en accesible, 28 px en compacto), ajusta las dimensiones de `tabNewButton_` según densidad y actualiza tooltips sin términos proscritos.
  - *Comando*: `grep -A 20 "BrowserWindow::applyTabStripDensityPlan" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "plan.height|tabNewButton_"`

- [ ] TASK-005: Sincronizar eventos de redimensionamiento de ventana con el servicio de pestañas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::resizeEvent` invoca `tabStripDensityService_->setAvailableWidth(width())` junto con las notificaciones a los demás servicios de ventana.
  - *Comando*: `grep -A 10 "BrowserWindow::resizeEvent" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "tabStripDensityService_.*setAvailableWidth"`

- [ ] TASK-006: Sincronizar la conmutación de la «Interfaz más fácil de usar» en `setSeniorMode(bool)` con la densidad de tira de pestañas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setSeniorMode(bool)` invoca `tabStripDensityService_->setDensity(enabled ? LayoutDensity::Senior : LayoutDensity::Default)`, eleva la altura de tira a 48 px y dimensiona el botón de nueva pestaña a `minimumHeight >= 48` y `minimumWidth >= 48`.
  - *Comando*: `grep -A 35 "BrowserWindow::setSeniorMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp | grep -E "tabStripDensityService_.*setDensity"`

- [ ] TASK-007: Enriquecer los estilos QSS (claro y oscuro) para la tira de pestañas en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setDarkTheme(bool)` incorpora selectores detallados para `QTabWidget#vigilio-tabs`, `QTabBar::tab`, `:selected`, `:hover`, `:focus` con borde de 2 px `#3B82F6` y botón `QPushButton#vigilio-tab-new-button`.
  - *Comando*: `grep -E "QTabWidget#vigilio-tabs|vigilio-tab-new-button" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-008: Añadir pruebas de presencia e inicialización de la tira de pestañas y servicio en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` contiene aserciones que verifican la presencia no nula de `window.tabStripDensityService()`, `window.tabWidget()`, `window.tabBar()` y `window.tabNewButton()`.
  - *Comando*: `grep -E "window.tabStripDensityService\(\)|window.tabWidget\(\)|window.tabNewButton\(\)" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Añadir pruebas de altura canónica estándar (36 px) y accesible (48 px) en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida que la altura estándar de la tira de pestañas es 36 px y tras invocar `setSeniorMode(true)` asciende canónicamente a 48 px.
  - *Comando*: `grep -A 15 "setSeniorMode" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "tabStripDensityService.*plan.*height|36|48"`

- [ ] TASK-010: Añadir pruebas de dimensiones de tap target $\ge 48\times 48$ px para el botón de nueva pestaña en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida que bajo `setSeniorMode(true)`, `window.tabNewButton()->minimumHeight() >= 48` y `window.tabNewButton()->minimumWidth() >= 48`.
  - *Comando*: `grep -A 20 "setSeniorMode" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "tabNewButton.*minimumHeight"`

- [ ] TASK-011: Añadir pruebas de sincronización de redimensionamiento y auditoría de textos accesibles en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la sincronización de ancho en `tabStripDensityService()->availableWidth()` y audita que ninguna etiqueta o tooltip de pestañas contenga «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec».
  - *Comando*: `grep -E "tabNewButton|tabStripDensityService" apps/vigilio-browser/tests/ui/WindowTests.cpp | grep -E "Senior Mode|Modo Senior"`

- [ ] TASK-012: Compilar limpiamente y ejecutar la suite completa de pruebas en CTest mediante `check.sh`
  - *Criterio de verificación*: `apps/vigilio-browser/scripts/check.sh` compila sin advertencias con `-Werror`, Clang-Format valida formato limpio y CTest pasa el 100% de las pruebas con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-013: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0070-tab-strip-density/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `ui` y mantiene sincronizados los criterios de verificación.
  - *Comando*: `grep -E "module.ux-shell.task0070" docs/rules/01-modules/ux-shell/micro/0070-tab-strip-density/task.md`
