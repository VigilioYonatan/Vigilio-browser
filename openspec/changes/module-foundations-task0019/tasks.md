# Tasks — module-foundations-task0019

Status: PROPOSED

- [ ] TASK-001: Declarar el método utilitario `makeResourceIcon` y punteros a acciones de navegación y barra de herramientas en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: Declaración de `QIcon makeResourceIcon(std::string_view alias) const;`, `QToolBar* toolbar_`, y punteros `backAction_`, `forwardAction_`, `reloadAction_`, `homeAction_`, `bookmarkAction_`, `favoritesAction_`, `newTabAction_`, `menuAction_`.
  - *Comando*: `grep -E "makeResourceIcon|toolbar_|backAction_" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar el método `makeResourceIcon` con fallback seguro en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `makeResourceIcon` invoca `vigilio::ResourceInclusionService::resolveIconWithFallback(alias)` y retorna un `QIcon` respaldado por la ruta QRC resuelta.
  - *Comando*: `grep -E "ResourceInclusionService::resolveIconWithFallback" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Asignar el icono oficial de marca (`branding/vigilio-logo.svg`) a la ventana principal en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setupUi()` invoca `setWindowIcon(makeResourceIcon("branding/vigilio-logo.svg"));`.
  - *Comando*: `grep -E "setWindowIcon.*branding/vigilio-logo.svg" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Equipar todas las acciones de navegación de la barra con sus correspondientes iconos vectoriales Lucide SVG en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Cada acción (`Atrás`, `Adelante`, `Recargar`, `Inicio`, `Favorito`, `Favoritos`, `Nueva pestaña`, `Menú`) se crea con su icono SVG correspondiente (`icons/navigation/*.svg`).
  - *Comando*: `grep -E "makeResourceIcon\(\"icons/navigation/" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Asignar iconos vectoriales a las acciones del menú principal en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `seniorAction_` se equipa con `makeResourceIcon("icons/tools/senior.svg")` y `themeAction_` con icono de ajustes/tema.
  - *Comando*: `grep -E "seniorAction_.*makeResourceIcon" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-006: Implementar el escalado dinámico de iconos en «Interfaz más fácil de usar» en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow::setSeniorMode(bool enabled)` ajusta `toolbar_->setIconSize(enabled ? QSize(32, 32) : QSize(22, 22))` y preserva alturas mínimas.
  - *Comando*: `grep -E "setIconSize.*32.*22" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-007: Configurar atributos de accesibilidad asistiva (`setAccessibleName`, `setAccessibleDescription`) y atajos de teclado en las acciones en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Todas las acciones con iconos disponen de nombres de accesibilidad en español natural conforme a `docs/UX-COPY-PUBLICO.md` y atajos universales.
  - *Comando*: `grep -E "setAccessibleName" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-008: Incorporar activos vectoriales de branding y aviso en `newHomePage` y `createErrorPageWidget` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `newHomePage()` despliega el icono de marca de Vigilio y `createErrorPageWidget()` despliega el icono vectorial de advertencia/escudo.
  - *Comando*: `grep -E "newHomePage|createErrorPageWidget" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-009: Ampliar la suite de pruebas Qt UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp` para validar la presencia de iconos, escalado en Senior Mode, accesibilidad y fallback determinista
  - *Criterio de verificación*: Aserciones `UI_CHECK` para iconos de acciones no nulos, icono de ventana no nulo, conmutación de `iconSize` entre (22, 22) y (32, 32), nombres accesibles no vacíos y fallback ante alias desconocido.
  - *Comando*: `grep -E "windowIcon|iconSize|accessibleName" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-010: Ejecutar la verificación completa del pipeline (`check.sh`) y certificar con Prompt Builder CLI (`validate-references`, `verify-lock`)
  - *Criterio de verificación*: `check.sh` aprueba 100% de pruebas en CTest con exit code 0; `validate-references` y `verify-lock` validan estado `valid: true`.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh && node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0055-module.foundations.task0019.propose.lock.json --project-root .`
