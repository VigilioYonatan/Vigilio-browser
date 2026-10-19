# Tasks — module-ux-shell-task0065

Status: PROPOSED

- [ ] TASK-001: Declarar la clase `TitleBarService`, opciones y callbacks en `apps/vigilio-browser/src/core/TitleBarService.h`
  - *Criterio de verificación*: `TitleBarService.h` declara la clase `TitleBarService` con mutex interno para concurrencia, `TitleBarServiceOptions`, `OnTitleBarChangedCallback` y métodos públicos de mutación, cálculo, hit-testing y persistencia en C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TitleBarService.h && grep -E "class TitleBarService|struct TitleBarServiceOptions" apps/vigilio-browser/src/core/TitleBarService.h`

- [ ] TASK-002: Implementar el ciclo de vida, mutaciones de estado y transiciones de densidad en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `TitleBarService.cpp` implementa constructores (por defecto, canónico Senior, canónico Compact y personalizado), `setDensity` con ajuste automático de restricciones mínimas y `setTitle` sanitizando la cadena entrante.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TitleBarService.cpp && grep -E "setDensity|setTitle|canonicalDefaultTitleBar" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-003: Implementar la conmutación de anclaje de controles (derecha/izquierda), visibilidad de icono y toggle Senior en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `setControlsPlacement`, `setShowIcon`, `setShowSeniorToggle` y `updateButtonState` recalculan la distribución geométrica sin solapamientos ni desbordamientos.
  - *Comando*: `grep -E "setControlsPlacement|setShowIcon|setShowSeniorToggle|updateButtonState" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-004: Implementar el redimensionamiento dinámico de ventana, mutación de estados y hit-testing en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `setWindowWidth` valida umbrales mínimos ($W \ge 400$ px estándar / $500$ px Senior), `setWindowState` sincroniza banderas de ventana, y `hitTest` clasifica coordenadas cartesianas de manera thread-safe.
  - *Comando*: `grep -E "setWindowWidth|setWindowState|hitTest" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-005: Implementar la persistencia atómica en disco y recuperación ante archivos corruptos en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `saveStateToFile` escribe mediante archivo temporal `.tmp` con renombramiento atómico, y `loadStateFromFile` deserializa el estado, restaurando `canonicalDefaultTitleBar()` si el archivo está dañado o truncado.
  - *Comando*: `grep -E "saveStateToFile|loadStateFromFile" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-006: Implementar el aislamiento estricto de navegación en modo incógnito/privado en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `setIncognitoMode` y `isIncognitoMode` gestionan la bandera de sesión privada, y cualquier intento de persistencia a disco bajo modo incógnito aborta inmediatamente retornando `false` sin escribir archivos.
  - *Comando*: `grep -E "setIncognitoMode|isIncognitoMode" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-007: Implementar el soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `computeLayout`, `saveStateToFile` y `loadStateFromFile` aceptan un puntero opcional `CancellationToken*`, comprobando `isCancelled()` y abortando limpiamente si la cancelación es solicitada.
  - *Comando*: `grep -E "CancellationToken.*cancel" apps/vigilio-browser/src/core/TitleBarService.h && grep -E "cancel->isCancelled" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-008: Implementar los formateadores de diagnóstico y observabilidad en español neutro conforme a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/src/core/TitleBarService.cpp`
  - *Criterio de verificación*: `formatTitleBarSummary` y métodos afines emiten resúmenes utilizando los términos aprobados («Interfaz más fácil de usar», «Estándar», «Compacta»), proscribiendo términos de ingeniería interna.
  - *Comando*: `grep -E "formatTitleBarSummary" apps/vigilio-browser/src/core/TitleBarService.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/TitleBarService.cpp`

- [ ] TASK-009: Registrar `TitleBarService.cpp` en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `src/core/TitleBarService.cpp` está presente en la lista de archivos fuente de `add_library(vigilio_core STATIC ...)`.
  - *Comando*: `grep -E "src/core/TitleBarService\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-010: Declarar el target ejecutable de pruebas unitarias `vigilio_title_bar_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_title_bar_service_tests`, lo vincula a `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_title_bar_service_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-011: Implementar la suite de 14 pruebas unitarias de servicio en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` compila limpiamente y define los 14 casos de prueba cubriendo inicialización, mutaciones, transiciones, placement, redimensionamiento, persistencia atómica, tolerancia a corrupción, incógnito y cancelación.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp && grep -E "test_service_initialization|test_service_density_transitions|test_service_persistence_roundtrip" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-012: Verificar las pruebas de inicialización, transiciones de densidad, placement y redimensionamiento en `TitleBarServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-TBS-001` a `TEST-TBS-006` comprueban la conformidad con los planes canónicos, el recálculo correcto para la «Interfaz más fácil de usar» y el rechazo de dimensiones sub-mínimas.
  - *Comando*: `grep -A 20 "test_service_density_transitions" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp | grep -E "LayoutDensity::Senior"`

- [ ] TASK-013: Verificar las pruebas de persistencia atómica, resiliencia ante corrupción y aislamiento de modo incógnito en `TitleBarServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-TBS-011` a `TEST-TBS-013` comprueban el guardado y carga exactos, la recuperación segura ante archivos truncados y el bloqueo incondicional de escrituras bajo modo incógnito.
  - *Comando*: `grep -A 25 "test_service_incognito_isolation" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp | grep -E "isIncognitoMode"`

- [ ] TASK-014: Verificar las pruebas de hit-testing, cancelación Zero-Zombie y observabilidad con copy público en `TitleBarServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-TBS-010`, `TEST-TBS-014` y afines comprueban la clasificación cartesiana, el aborto inmediato ante `CancellationToken` y la ausencia de términos internos en los resúmenes generados.
  - *Comando*: `grep -A 20 "test_service_cancellation" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp | grep -E "requestCancellation"`

- [ ] TASK-015: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: `check.sh` compila con `-Werror`, ejecuta Clang-Format y corre el 100% de las pruebas en CTest (29/29 pruebas pasando) con exit code 0 en menos de 10 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-016: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0065-title-bar-integration/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0193, y `task.md` formaliza el cumplimiento de la etapa `implementation`.
  - *Comando*: `vigilio-prompt validate-references --project-root . && vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0193-module.ux-shell.task0065.propose.lock.json --project-root . && grep -E "module.ux-shell.task0065" docs/rules/01-modules/ux-shell/micro/0065-title-bar-integration/task.md`
