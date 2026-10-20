# Tasks — module-ux-shell-task0069

Status: PROPOSED

- [ ] TASK-001: Declarar la clase `TabStripDensityService`, opciones y callbacks en `apps/vigilio-browser/src/core/TabStripDensityService.h`
  - *Criterio de verificación*: `TabStripDensityService.h` declara la clase `TabStripDensityService` con mutex interno para concurrencia, `TabStripDensityServiceOptions`, `OnTabStripChangedCallback` y métodos públicos de mutación, cálculo de layout, hit-testing y persistencia en C++20 sin dependencias de GUI.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TabStripDensityService.h && grep -E "class TabStripDensityService|struct TabStripDensityServiceOptions" apps/vigilio-browser/src/core/TabStripDensityService.h`

- [ ] TASK-002: Implementar constructores, ciclo de vida y transiciones de densidad en `apps/vigilio-browser/src/core/TabStripDensityService.cpp`
  - *Criterio de verificación*: `TabStripDensityService.cpp` implementa constructores (por defecto, canónico Senior, canónico Compact y personalizado), `setDensity` con ajuste automático de restricciones mínimas según densidad y recálculo determinista.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TabStripDensityService.cpp && grep -E "setDensity|canonicalDefaultDensity|canonicalSeniorDensity" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-003: Implementar la gestión de colección de pestañas (adición, inserción, eliminación con selección adyacente y reordenamiento) en `apps/vigilio-browser/src/core/TabStripDensityService.cpp`
  - *Criterio de verificación*: `addTab`, `insertTab`, `removeTab` y `moveTab` actualizan la colección de pestañas, manteniendo IDs únicos y reasignando la selección a la pestaña adyacente más próxima al cerrar la activa.
  - *Comando*: `grep -E "addTab|insertTab|removeTab|moveTab|selectTab" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-004: Implementar la alternancia de fijación (`setTabPinned`) con preservación de precedencia espacial y ancho invariante en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `setTabPinned` conmuta el estado de fijación, reubica la pestaña a la izquierda de todas las regulares ($x_{\text{pinned}} < x_{\text{regular}}$) y aplica el ancho canónico fijado sin compresión elástica.
  - *Comando*: `grep -E "setTabPinned" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-005: Implementar el control de reproducción/silenciamiento sonoro y ahorro de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo») en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `setTabMediaState`, `toggleTabAudioMute` y `setTabDiscardState` actualizan los atributos de la pestaña, reflejando el rectángulo de altavoz y las etiquetas accesibles aprobadas.
  - *Comando*: `grep -E "setTabMediaState|toggleTabAudioMute|setTabDiscardState" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-006: Implementar el redimensionamiento dinámico de ancho, compresión elástica y desbordamiento horizontal (`overflowMode`, scroll offset) en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `setAvailableWidth` valida umbrales mínimos ($W \ge 400$ px estándar / $500$ px Senior), comprime pestañas elásticamente hasta `minTabWidth` y activa `isOverflowing == true` con controles de desplazamiento.
  - *Comando*: `grep -E "setAvailableWidth|setOverflowMode|setScrollOffset" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-007: Implementar el hit-testing cartesiano $\mathcal{O}(1)$ e identificación de pestaña por coordenadas en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `hitTest(x, y)` y `tabAtPoint(x, y)` traducen coordenadas relativas a zonas de impacto (`TabStripHitTestZone`) y devuelven el ID de la pestaña correspondiente de forma thread-safe.
  - *Comando*: `grep -E "hitTest|tabAtPoint" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-008: Implementar la persistencia atómica en disco y recuperación ante archivos corruptos en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `saveStateToFile` escribe mediante archivo temporal `.tmp` con renombramiento atómico, y `loadStateFromFile` deserializa el estado de la tira, restaurando `canonicalDefaultDensity()` si el archivo está dañado o truncado.
  - *Comando*: `grep -E "saveStateToFile|loadStateFromFile" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-009: Implementar el aislamiento estricto de navegación en modo incógnito/privado en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `setIncognitoMode` y `isIncognitoMode` gestionan la bandera de sesión privada, y cualquier intento de persistencia a disco bajo modo incógnito aborta inmediatamente retornando `false` sin escribir archivos.
  - *Comando*: `grep -E "setIncognitoMode|isIncognitoMode" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-010: Implementar el soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `computeLayout`, `saveStateToFile` y `loadStateFromFile` aceptan un puntero opcional `CancellationToken*`, comprobando `isCancelled()` y abortando limpiamente si la cancelación es solicitada.
  - *Comando*: `grep -E "CancellationToken.*cancel" apps/vigilio-browser/src/core/TabStripDensityService.h && grep -E "cancel->isCancelled" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-011: Implementar observabilidad y formateadores de diagnóstico en español neutro conforme a `docs/UX-COPY-PUBLICO.md` en `TabStripDensityService.cpp`
  - *Criterio de verificación*: `formatTabStripSummary` y métodos afines emiten resúmenes utilizando los términos aprobados («Interfaz más fácil de usar», «Liberar memoria de pestañas inactivas», «Pestaña en reposo», «Ahorro de memoria»), proscribiendo términos de ingeniería interna.
  - *Comando*: `grep -E "formatTabStripSummary" apps/vigilio-browser/src/core/TabStripDensityService.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/TabStripDensityService.cpp`

- [ ] TASK-012: Registrar `TabStripDensityService.cpp` en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `src/core/TabStripDensityService.cpp` está presente en la lista de archivos fuente de `add_library(vigilio_core STATIC ...)`.
  - *Comando*: `grep -E "src/core/TabStripDensityService\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-013: Declarar el target ejecutable de pruebas unitarias `vigilio_tab_strip_density_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_tab_strip_density_service_tests`, lo vincula a `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_tab_strip_density_service_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-014: Implementar la suite de 17 pruebas unitarias de servicio en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` compila limpiamente y define los 17 casos de prueba cubriendo inicialización, mutaciones, transiciones, fijación, selección adyacente, audio, ahorro de memoria, redimensionamiento, desbordamiento, hit-testing, persistencia atómica, tolerancia a corrupción, incógnito y cancelación.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp && grep -E "test_service_initialization|test_service_density_transitions|test_service_persistence_roundtrip" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-015: Verificar las pruebas de inicialización, transiciones de densidad, mutaciones y ordenación en `TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-TSS-001` a `TEST-TSS-007` comprueban la conformidad con los planes canónicos, el recálculo correcto para la «Interfaz más fácil de usar» y el mantenimiento de la precedencia de pestañas fijadas.
  - *Comando*: `grep -A 20 "test_service_density_transitions" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp | grep -E "LayoutDensity::Senior"`

- [ ] TASK-016: Verificar las pruebas de audio, descarte de memoria, persistencia atómica, tolerancia a corrupción, incógnito y cancelación Zero-Zombie en `TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: Pruebas `TEST-TSS-008` a `TEST-TSS-018` comprueban el guardado y carga exactos, la recuperación segura ante archivos truncados, el bloqueo incondicional de escrituras bajo modo incógnito, el aborto inmediato ante `CancellationToken` y la ausencia de términos internos en los resúmenes generados.
  - *Comando*: `grep -A 25 "test_service_incognito_isolation" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp | grep -E "isIncognitoMode"`

- [ ] TASK-017: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: `check.sh` compila con `-Werror`, ejecuta Clang-Format y corre el 100% de las pruebas en CTest (31/31 pruebas pasando) con exit code 0 en menos de 10 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-018: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0069-tab-strip-density/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `implementation` y mantiene sincronizados los criterios de aceptación y verificación.
  - *Comando*: `grep -E "module.ux-shell.task0069" docs/rules/01-modules/ux-shell/micro/0069-tab-strip-density/task.md`
