# Tasks — module-ux-shell-task0063

Status: PROPOSED

- [ ] TASK-001: Ampliar pruebas de fronteras dimensionales y entradas negativas en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` incluye pruebas que evalúan las dimensiones en las fronteras exactas de fallo para Default ($639\times 480$, $640\times 479$), Senior ($719\times 520$, $720\times 519$), Compact ($499\times 380$, $500\times 379$), así como dimensiones cero y negativas, verificando el retorno estricto de `MainWindowLayoutError::DimensionsBelowMinimum`.
  - *Comando*: `grep -E "testSubMinimumDimensionsExhaustive|testZeroAndNegativeDimensions" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-002: Ampliar pruebas de fronteras de anchura de barra lateral y regiones prohibidas en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` contiene pruebas exhaustivas que verifican que anchuras de barra lateral en los intervalos prohibidos ($0..47$ px, $49..199$ px, $> 360$ px) retornan `MainWindowLayoutError::InvalidSidebarWidth` y generan una pista de remediación válida.
  - *Comando*: `grep -E "testInvalidSidebarWidthBoundaries" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-003: Ampliar pruebas de presupuestos corruptos y sobreasignación de zonas fijas en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` evalúa presupuestos de zonas con alturas negativas, márgenes negativos (`NegativeMarginOrPadding`), asignaciones contradictorias (`ConflictingZoneAnchors`) y situaciones donde la suma de zonas fijas consume la totalidad de la altura de la ventana, verificando la emisión de `ZeroContentAreaBudget`.
  - *Comando*: `grep -E "testZoneBudgetValidationFailures|ZeroContentAreaBudget" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-004: Ampliar pruebas de distribución analítica en geometrías de ventana extremas en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` valida el cálculo analítico de áreas de layout (`calculateLayoutAreas`) en resoluciones ultrapanorámicas ($3840\times 1080$), verticales ($1080\times 1920$) y de alta densidad ($2560\times 1440$), comprobando que no existen solapamientos espaciales entre zonas ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y que el área de contenido es positiva.
  - *Comando*: `grep -E "testExtremeWindowGeometries" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-005: Ampliar auditoría de textos públicos y pistas de remediación frente a UX Copy en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` audita programáticamente todas las funciones de conversión a texto y pistas de remediación (`layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `sidebarDockPositionToString`, `layoutErrorToString`, `generateRemediationHint`), verificando la ausencia total de términos de ingeniería proscritos por `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "testPublicCopyAndRemediationTextAudit" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-006: Implementar prueba de estrés de concurrencia masiva con 16 hilos en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` contiene una prueba de concurrencia que lanza 16 hilos concurrentes compitiendo sobre una instancia compartida de `MainWindowLayoutService` (redimensionamiento, conmutación de densidad, mutación de sidebar y consultas simultáneas), finalizando de forma limpia sin carreras de datos ni interbloqueos.
  - *Comando*: `grep -E "testMultiThreadedHighStressConcurrency16Threads" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-007: Implementar prueba de cancelación cooperativa multihilo mediante `CancellationToken` en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` prueba la activación asíncrona de `CancellationToken` durante operaciones del servicio, verificando respuesta inmediata (< 10 ms), aborto seguro y consistencia inalterada del plan de layout activo.
  - *Comando*: `grep -E "testCooperativeCancellationStress" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de resiliencia y recuperación ante JSON truncado y corrupto en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` inyecta archivos JSON truncados, vacíos o con sintaxis rota en `loadLayoutStateFromFile`, certificando que el servicio retorna `MainWindowLayoutError::LayoutPersistenceCorrupted` y restablece de forma transparente el plan canónico por defecto (`canonicalDefaultLayout()`) sin caídas.
  - *Comando*: `grep -E "testCorruptedLayoutPersistenceRecovery" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de recuperación ante violaciones geométricas en archivos de layout en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` inyecta archivos JSON válidos en sintaxis pero que contienen dimensiones sub-mínimas o anchuras de sidebar inválidas, certificando que el validador detecta la anomalía y el servicio restaura el plan seguro canónico.
  - *Comando*: `grep -A 20 "testCorruptedLayoutPersistenceRecovery" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp | grep -E "canonicalDefaultLayout"`

- [ ] TASK-010: Implementar pruebas de simulación de fallos de disco y permisos en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` simula operaciones de escritura en directorios con permisos de solo lectura (`perms::read_only`), verificando que `saveLayoutStateToFile` maneja el error de forma segura sin abortos ni excepciones no capturadas.
  - *Comando*: `grep -E "testDiskFailureHandling" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-011: Implementar pruebas de aislamiento estricto de incógnito en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` certifica que cuando `isIncognitoMode() == true`, las llamadas a `saveLayoutStateToFile` y `loadLayoutStateFromFile` retornan inmediatamente sin realizar operaciones de lectura o escritura en el sistema de archivos local.
  - *Comando*: `grep -E "testIncognitoModeStrictIsolation" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-012: Implementar pruebas de anclaje de barra lateral a la derecha y visibilidad de zonas en `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutServiceTests.cpp` evalúa la conmutación de `SidebarDockPosition::Right` y la alternancia de visibilidad de zonas opcionales (`NotificationBanner`, `StatusBar`), comprobando que las geometrías resultantes reflejan la posición derecha sin solapamientos.
  - *Comando*: `grep -E "testSidebarDockPositionAndVisibilityTransitions" apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`

- [ ] TASK-013: Refinar aserciones de integración de layout, eventos nativos de tamaño y accesibilidad en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la integración de `BrowserWindow` con `MainWindowLayoutService`, la reacción ante eventos `resizeEvent`, las dimensiones de tap target $\ge 48$ px en modo accesible y la tolerancia limpia en entornos headless (`NOT_EXECUTED` si display ausente).
  - *Comando*: `grep -E "layoutService|setSeniorMode|minimumHeight" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-014: Verificar formato canónico con Clang-Format sobre archivos modificados
  - *Criterio de verificación*: `clang-format --dry-run --Werror` reporta cero discrepancias sobre todos los archivos de prueba modificados.
  - *Comando*: `clang-format --dry-run --Werror apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-015: Verificar análisis estático con Clang-Tidy sobre archivos modificados
  - *Criterio de verificación*: El análisis estático no arroja advertencias ni violaciones sobre las unidades de traducción modificadas.
  - *Comando*: `test -f apps/vigilio-browser/build-ci-debug/compile_commands.json && clang-tidy -p apps/vigilio-browser/build-ci-debug apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp || true`

- [ ] TASK-016: Compilar y ejecutar la suite completa de pruebas en CTest
  - *Criterio de verificación*: CMake compila limpiamente bajo C++20 con `-Werror` y CTest ejecuta el 100% de las pruebas con éxito (exit code 0).
  - *Comando*: `cmake --build apps/vigilio-browser/build-ci-debug && ctest --test-dir apps/vigilio-browser/build-ci-debug --output-on-failure`

- [ ] TASK-017: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` y `verify-lock` certifica la integridad del archivo lock de la ejecución 0187.
  - *Comando*: `vigilio-prompt validate-references --project-root . && vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0187-module.ux-shell.task0063.propose.lock.json --project-root .`

- [ ] TASK-018: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0063-main-window-layout/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `tests` y mantiene sincronizados los criterios de aceptación y verificación.
  - *Comando*: `grep -E "module.ux-shell.task0063" docs/rules/01-modules/ux-shell/micro/0063-main-window-layout/task.md`
