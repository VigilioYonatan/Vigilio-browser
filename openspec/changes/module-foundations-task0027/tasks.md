# Tasks — module-foundations-task0027

Status: PROPOSED

- [ ] TASK-001: Declarar la integración de `ApplicationLifetimeCoordinator`, la acción `quitAction_` y métodos de consulta en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: `BrowserWindow.h` incluye `core/ApplicationLifetimeCoordinator.h`, declara `std::unique_ptr<vigilio::ApplicationLifetimeCoordinator> lifetimeCoordinator_`, `QAction* quitAction_`, métodos de consulta `lifetimeCoordinator()`, `quitAction()` y el método auxiliar `bool confirmCloseMultipleTabs()`.
  - *Comando*: `grep -E "ApplicationLifetimeCoordinator|quitAction_|confirmCloseMultipleTabs" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar la instanciación y transición de estado de `ApplicationLifetimeCoordinator` en el constructor de `BrowserWindow.cpp`
  - *Criterio de verificación*: El constructor inicializa `lifetimeCoordinator_` con `LifetimeConfig` propagando `isIncognito_`, `isSafeMode_`, timeout de 3000 ms y ruta de sesión, e invoca `lifetimeCoordinator_->transitionTo(vigilio::AppLifetimeState::Running)`.
  - *Comando*: `grep -E "lifetimeCoordinator_ = std::make_unique<vigilio::ApplicationLifetimeCoordinator>|transitionTo\(vigilio::AppLifetimeState::Running\)" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Implementar la acción global de salida `quitAction_` con atajo `Ctrl+Q` e integración en la interfaz en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `quitAction_` se crea con texto «Cerrar navegador», atajo `QKeySequence::Quit` (`Ctrl+Q`), icono Lucide `icons/tools/log-out.svg` (o fallback canónico), `accessibleName`, y se conecta a `close()`.
  - *Comando*: `grep -E "quitAction_ = new QAction|QKeySequence::Quit|Cerrar navegador" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar el diálogo de confirmación accesible de cierre para múltiples pestañas (`confirmCloseMultipleTabs`) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `confirmCloseMultipleTabs` despliega un diálogo modal accesible cuando `tabCount() > 1`, consultando «¿Quieres cerrar las %1 pestañas abiertas?» con botones «Cerrar todas» y «Cancelar», navegable con `Tab`, `Enter` y `Esc`, y con soporte completo de escalado para la «Interfaz más fácil de usar».
  - *Comando*: `grep -E "confirmCloseMultipleTabs|Cerrar ventana|Cerrar todas" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Orquestar el evento de cierre (`BrowserWindow::closeEvent`) integrando `requestShutdown`, confirmación y `executeShutdown` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `closeEvent` solicita apagado al coordinador, evalúa la confirmación para múltiples pestañas (cancelando con `event->ignore()` si el usuario rechaza), e invoca `lifetimeCoordinator_->executeShutdown(...)` con callbacks para guardar sesión y cerrar `LocalRepository` (`db_`), aceptando finalmente `event->accept()`.
  - *Comando*: `grep -E "requestShutdown|executeShutdown|event->ignore\(\)|event->accept\(\)" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-006: Garantizar el aislamiento estricto de sesiones de incógnito al cerrar la ventana (Q-001) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Si `isIncognito_ == true`, al cerrar la ventana no se guarda la sesión en disco, no se despliega diálogo de guardar sesión y el coordinador no realiza llamadas a `sessionSaver`.
  - *Comando*: `grep -A 10 "void BrowserWindow::saveSession" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-007: Implementar el escalado de dimensiones y fuentes del diálogo de cierre bajo la «Interfaz más fácil de usar» (Senior Mode) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setSeniorMode(bool enabled)` escala botones a `minimumHeight >= 48` y tipografía a 14 pt en los componentes y diálogos de ciclo de vida.
  - *Comando*: `grep -A 25 "void BrowserWindow::setSeniorMode" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-008: Extender la suite de pruebas de UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp` con casos de prueba para `ApplicationLifetime` en UI
  - *Criterio de verificación*: `WindowTests.cpp` valida la presencia y estado `Running` de `lifetimeCoordinator()`, la configuración y atajo `Ctrl+Q` de `quitAction()`, el cierre sin confirmación para 1 pestaña, el cierre con múltiples pestañas y el aislamiento en modo incógnito.
  - *Comando*: `grep -E "test_ui_lifetime_coordinator|quitAction|warnOnCloseMultipleTabs" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Ejecutar el pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`) y verificar compilación limpia bajo C++20 con `-Werror`, Clang-Format y 100% de tests pasando en CTest
  - *Criterio de verificación*: Compilación limpia sin advertencias bajo Ninja y C++20; 100% de pruebas pasando en CTest con exit code 0; y conformidad absoluta con Clang-Format.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0079-module.foundations.task0027.propose.lock.json --project-root .`

- [ ] TASK-011: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0027-application-lifetime/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase y se mantiene coherente con la implementación de interfaz en Qt.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0027-application-lifetime/task.md && grep -E "module.foundations.task0027" docs/rules/01-modules/foundations/micro/0027-application-lifetime/task.md`
