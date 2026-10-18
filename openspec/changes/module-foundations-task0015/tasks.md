# Tasks — module-foundations-task0015

Status: PROPOSED

- [ ] TASK-001: Auditar y estructurar la API privada de estados y recuperación de WebEngine en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: Declaración de métodos auxiliares `createErrorPageWidget(const QUrl& failedUrl, QWebEngineView* view)` y gestión de estados visuales de pestañas (`empty`, `loading`, `success`, `error`, `deferred`) preservando la encapsulación de `BrowserWindow`.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.h && grep -E "createErrorPage|openTab|activateDeferredTab" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar el contenedor nativo accesible de recuperación ante error en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Creación de un widget nativo `QWidget` con `setObjectName("vigilio-page-error")`, título `"No se pudo cargar la página"`, botón primario `"Reintentar"` (que invoca recarga) y botón secundario `"Ir al inicio"`, con soporte para foco de teclado y nombres accesibles.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.cpp && grep -E "vigilio-page-error|No se pudo cargar la página|Reintentar" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Cablear la máquina de estados visuales y atajos de recarga (`Ctrl+R`, `F5`) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: La señal `loadFinished(bool ok)` conmuta limpiamente entre la vista web exitosa y el contenedor de error nativo; registro en historial SQLite condicionado a `ok == true`; atajos `Ctrl+R` y `F5` conectados a la recarga de la pestaña activa.
  - *Comando*: `grep -E "loadFinished|Ctrl\+R|QKeySequence::Refresh" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Garantizar el escalado y accesibilidad en la «Interfaz más fácil de usar» para todos los controles de WebEngine
  - *Criterio de verificación*: Al invocar `setSeniorMode(true)`, los campos de texto y botones interactivos del visor y del contenedor de error escalan a una altura mínima de 48px, las barras a 55px, y las fuentes a 14pt, cumpliendo `docs/UX-COPY-PUBLICO.md` sin mostrar etiquetas prohibidas.
  - *Comando*: `grep -E "setMinimumHeight\(enabled \? 48 : 32\)|accessibility/senior" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Ampliar y verificar la suite de pruebas de interfaz en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: Incorporación de pruebas con `QTest` que validen la creación de pestañas vacías, el estado diferido (`deferredUrl`), el escalado de controles en la «Interfaz más fácil de usar» y el reporte honesto de `NOT_EXECUTED` ante ausencia de display.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "BrowserWindow|tabCount" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-006: Ejecutar la verificación local completa con Ninja, Clang-Format y suite de pruebas en CTest
  - *Criterio de verificación*: Compilación limpia con `-Werror` bajo el estándar C++20, validación de formato sin diferencias y 100% de pruebas unitarias/integración aprobadas con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh && clang-format --dry-run --Werror apps/vigilio-browser/src/ui/BrowserWindow.*`

- [ ] TASK-007: Sincronizar la documentación canónica de la microtarea y validar las referencias canónicas del árbol OpenSpec con Prompt Builder
  - *Criterio de verificación*: `canonical-reference.yaml` y la propuesta OpenSpec pasan satisfactoriamente la validación automatizada de referencias canónicas mediante la herramienta Prompt Builder sin discrepancias.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
