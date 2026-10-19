# Tasks — module-ux-shell-task0061

Status: PROPOSED

- [ ] TASK-001: Declarar la clase `MainWindowLayoutService`, opciones y callbacks en `apps/vigilio-browser/src/core/MainWindowLayoutService.h`
  - *Criterio de verificación*: `MainWindowLayoutService.h` declara la clase `MainWindowLayoutService` con mutex interno para concurrencia, `MainWindowLayoutServiceOptions`, `OnLayoutChangedCallback` y métodos públicos de mutación, cálculo y persistencia en C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/MainWindowLayoutService.h && grep -E "class MainWindowLayoutService|struct MainWindowLayoutServiceOptions" apps/vigilio-browser/src/core/MainWindowLayoutService.h`

- [ ] TASK-002: Implementar el ciclo de vida, mutaciones de estado y transiciones de densidad en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `MainWindowLayoutService.cpp` implementa constructores (por defecto, canónico Senior, canónico Compact y personalizado), `setDensity` con ajuste automático de restricciones mínimas y `resizeWindow` validando umbrales antes de mutar el plan.
  - *Comando*: `test -f apps/vigilio-browser/src/core/MainWindowLayoutService.cpp && grep -E "setDensity|resizeWindow|canonicalDefaultLayout" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-003: Implementar el control desacoplado de la barra lateral (toggle, anchura, anclaje) y visibilidad de zonas en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `toggleSidebar`, `setSidebarState`, `setSidebarWidth` (restringido al rango $[200, 360]$ o 48 px), `setSidebarDockPosition`, `showNotificationBanner` y `showStatusBar` recalculan la distribución geométrica sin provocar solapamientos.
  - *Comando*: `grep -E "toggleSidebar|setSidebarWidth|setSidebarDockPosition|showNotificationBanner" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-004: Implementar la persistencia atómica en disco y recuperación ante fallos o archivos corruptos en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `saveLayoutStateToFile` escribe mediante archivo temporal `.tmp` con renombramiento atómico, y `loadLayoutStateFromFile` analiza el estado en disco, restaurando `canonicalDefaultLayout()` con código `LayoutPersistenceCorrupted` si el archivo está dañado o truncado.
  - *Comando*: `grep -E "saveLayoutStateToFile|loadLayoutStateFromFile|LayoutPersistenceCorrupted" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-005: Implementar el aislamiento estricto de navegación en modo privado/incógnito en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `setIncognitoMode` y `isIncognitoMode` gestionan la bandera de sesión privada, y cualquier intento de persistencia a disco bajo modo incógnito aborta inmediatamente retornando `false` sin escribir archivos.
  - *Comando*: `grep -E "setIncognitoMode|isIncognitoMode" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-006: Implementar el soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `computeLayout`, `saveLayoutStateToFile` y `loadLayoutStateFromFile` aceptan un puntero opcional `CancellationToken*`, comprobando `isCancelled()` y abortando limpiamente si la cancelación es solicitada.
  - *Comando*: `grep -E "CancellationToken.*cancel" apps/vigilio-browser/src/core/MainWindowLayoutService.h && grep -E "cancel->isCancelled" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-007: Implementar los formateadores de diagnóstico y observabilidad en español neutro conforme a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`
  - *Criterio de verificación*: `formatLayoutSummary` y métodos afines emiten resúmenes utilizando los términos aprobados («Interfaz más fácil de usar», «Estándar», «Compacta»), proscribiendo términos de ingeniería interna.
  - *Comando*: `grep -E "formatLayoutSummary" apps/vigilio-browser/src/core/MainWindowLayoutService.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`

- [ ] TASK-008: Registrar `MainWindowLayoutService.cpp` en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `src/core/MainWindowLayoutService.cpp` está presente en la lista de archivos fuente de `add_library(vigilio_core STATIC ...)`.
  - *Comando*: `grep -E "src/core/MainWindowLayoutService\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-009: Declarar el target ejecutable de pruebas unitarias `vigilio_main_window_layout_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_main_window_layout_service_tests`, lo vincula a `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_main_window_layout_service_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-010: Implementar la suite de 14 pruebas unitarias de servicio en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` compila limpiamente y define los 14 casos de prueba cubriendo inicialización, mutaciones, transiciones, barra lateral, redimensionamiento, persistencia atómica, tolerancia a corrupción, incógnito y cancelación.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp && grep -E "test_service_initialization|test_service_density_transitions|test_service_persistence_roundtrip" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-011: Verificar las pruebas de inicialización, transiciones de densidad y redimensionamiento en `MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-SRV-001` a `TEST-SRV-006` comprueban la conformidad con los planes canónicos, el recálculo correcto de tap targets para la «Interfaz más fácil de usar» y el rechazo de dimensiones sub-mínimas.
  - *Comando*: `grep -A 20 "test_service_density_transitions" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp | grep -E "LayoutDensity::Senior"`

- [ ] TASK-012: Verificar las pruebas de persistencia atómica, resiliencia ante archivos corruptos y aislamiento de modo incógnito en `MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-SRV-011` a `TEST-SRV-013` comprueban el guardado y carga exactos, la recuperación segura ante archivos truncados y el bloqueo incondicional de escrituras bajo modo incógnito.
  - *Comando*: `grep -A 25 "test_service_incognito_isolation" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp | grep -E "isIncognitoMode"`

- [ ] TASK-013: Verificar las pruebas de cancelación cooperativa Zero-Zombie y observabilidad con UX copy público en `MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-SRV-014` y afines comprueban el aborto inmediato ante `CancellationToken` y la ausencia de términos internos en los resúmenes generados.
  - *Comando*: `grep -A 20 "test_service_cancellation" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp | grep -E "requestCancellation"`

- [ ] TASK-014: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: `check.sh` compila con `-Werror`, ejecuta Clang-Format y corre el 100% de las pruebas en CTest (27/27 pruebas pasando) con exit code 0 en menos de 10 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-015: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0061-main-window-layout/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0181, y `task.md` formaliza el cumplimiento de la etapa `implementation`.
  - *Comando*: `vigilio-prompt validate-references --project-root . && vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0181-module.ux-shell.task0061.propose.lock.json --project-root . && grep -E "module.ux-shell.task0061" docs/rules/01-modules/ux-shell/micro/0061-main-window-layout/task.md`
