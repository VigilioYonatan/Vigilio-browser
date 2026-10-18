# Tasks — module-foundations-task0023

Status: PROPOSED

- [ ] TASK-001: Extender la firma e interfaz de `BrowserWindow` en `apps/vigilio-browser/src/ui/BrowserWindow.h` para soportar contexto de arranque (`StartupCommandLine` y `StartupDiagnosticReport`)
  - *Criterio de verificación*: Declaración de `#include "core/StartupTypes.h"`, constructor `explicit BrowserWindow(const vigilio::StartupCommandLine& cmdLine = {}, const vigilio::StartupDiagnosticReport& report = {});`, y miembros privados `safeModeBanner_`, `incognitoBadge_`, `isIncognito_`, `isSafeMode_`.
  - *Comando*: `grep -E "StartupCommandLine|StartupDiagnosticReport|safeModeBanner_|incognitoBadge_" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar el constructor parametrizado y la resolución de persistencia (perfil personalizado e incógnito en `:memory:`) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `BrowserWindow` evalúa `cmdLine.isIncognito` asignando la persistencia a `":memory:"`, o utiliza `report.storagePath` si no está vacío, evitando escrituras en disco durante sesiones privadas.
  - *Comando*: `grep -E "isIncognito.*:memory:|report.storagePath" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Implementar el banner visual accesible de modo seguro (`safeModeBanner_`) con botón de descarte en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Creación de `vigilio-safe-mode-banner` con icono Lucide `icons/tools/shield.svg`, texto en lenguaje público y botón `vigilio-safe-mode-dismiss` con texto «Entendido» que oculta el banner y devuelve el foco.
  - *Comando*: `grep -E "vigilio-safe-mode-banner|vigilio-safe-mode-dismiss" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar el distintivo visual accesible de sesión privada de incógnito (`incognitoBadge_`) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Creación de `vigilio-incognito-badge` con icono `icons/tools/shield.svg`, texto «Navegación privada», actualización del título de ventana y atributos accesibles para tecnologías de lectura de pantalla.
  - *Comando*: `grep -E "vigilio-incognito-badge|Navegación privada" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Implementar la apertura automática de URLs pasadas en la línea de comandos en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Si `cmdLine.urls` contiene direcciones web, `BrowserWindow` abre una pestaña por cada URL válida de forma no bloqueante, asignando el foco a la primera pestaña.
  - *Comando*: `grep -E "cmdLine.urls|openTab\(QUrl" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-006: Adaptar el dimensionamiento y accesibilidad asistiva en `BrowserWindow::setSeniorMode(bool enabled)` para los componentes de arranque en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setSeniorMode(true)` escala el banner de modo seguro y el botón de descarte asegurando alturas mínimas >= 48 px y tipografía 14pt, conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "safeModeBanner_.*minimumHeight" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-007: Actualizar `apps/vigilio-browser/src/app.cpp` para transmitir `cmdLine` y `report` al instanciar `BrowserWindow` y presentar el diálogo accesible de error en GUI
  - *Criterio de verificación*: `app.cpp` invoca `BrowserWindow window(cmdLine, report);` y, en caso de fallo no recuperable con display disponible, presenta un diálogo modal accesible informando al usuario en lenguaje comprensible.
  - *Comando*: `grep -E "BrowserWindow window\(cmdLine, report\)|showStartupErrorDialog" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-008: Ampliar la suite de pruebas Qt UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp` con casos de prueba para modo seguro, incógnito, URLs CLI y perfiles
  - *Criterio de verificación*: Inclusión de aserciones `UI_CHECK` verificando la presencia del banner de modo seguro, el botón de descarte, el distintivo de incógnito, la apertura de URLs iniciales y el escalado en Senior Mode.
  - *Comando*: `grep -E "testStartupSafeModeBanner|testStartupIncognitoMode|testStartupWithInitialUrls" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Ejecutar la verificación completa del pipeline de compilación y pruebas con `check.sh`
  - *Criterio de verificación*: Compilación limpia con Ninja bajo C++20 y `-Werror`, y aprobación del 100% de pruebas en CTest con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Certificar la propuesta documental y gobernanza con Prompt Builder CLI (`validate-references`, `verify-lock`, `validate-decisions`)
  - *Criterio de verificación*: `validate-references` reporta estado `valid: true`, `verify-lock` aprueba las 13 comprobaciones del lockfile 0067, y `validate-decisions` confirma que las decisiones de foundations están resueltas.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0067-module.foundations.task0023.propose.lock.json --project-root .`
