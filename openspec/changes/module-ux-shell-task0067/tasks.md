# Tasks — module-ux-shell-task0067

Status: PROPOSED

- [ ] TASK-001: Ampliar pruebas de fronteras dimensionales exactas y entradas negativas en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` incluye pruebas que evalúan las dimensiones en las fronteras exactas de fallo (ancho $399$ px, $0$ px y valores negativos), verificando el retorno estricto de `TitleBarError::DimensionsBelowMinimum`.
  - *Comando*: `grep -E "testSubMinimumDimensionsExhaustive" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-002: Ampliar pruebas de violación de altura en la «Interfaz más fácil de usar» y área de arrastre insuficiente en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` contiene pruebas que verifican que alturas $< 48$ px en densidad Senior retornan `TitleBarError::SeniorModeMinimumHeightViolation` y que ventanas estrechas con controles expandidos retornan `TitleBarError::DraggableAreaInsufficient`.
  - *Comando*: `grep -E "testSeniorModeHeightViolations|testDraggableAreaExhaustion" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-003: Ampliar pruebas de disyunción espacial analítica y geometrías extremas en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` valida el cálculo analítico de áreas en resoluciones extremas ($3840\times 2160$, $3840\times 1080$, $1080\times 1920$), comprobando que no existen solapamientos espaciales entre controles ni con el área de arrastre ($\forall i \ne j, \text{control}_i \cap \text{control}_j = \emptyset$).
  - *Comando*: `grep -E "testSpatialDisjointnessAndExtremeResolutions" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-004: Ampliar pruebas de hit-testing espacial de esquinas, bordes y puntos exteriores en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` evalúa las coordenadas de las cuatro esquinas de cada botón de control y coordenadas exteriores a la ventana, verificando la detección precisa de zonas y el retorno de `TitleBarHitTestZone::Nowhere` fuera de los límites.
  - *Comando*: `grep -E "testExhaustiveHitTestingBoundaries" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-005: Ampliar pruebas de sanitización hostil de títulos y protección de privacidad en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` evalúa cadenas con credenciales embebidas (`user:password@host`), saltos de línea, caracteres de control y cadenas de longitud extrema, verificando la sanitización limpia sin fuga de datos sensibles y el fallback seguro en cadenas vacías.
  - *Comando*: `grep -E "testTitleSanitizationEdgeCases" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-006: Ampliar auditoría de textos públicos y pistas de remediación frente a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` audita programáticamente todas las cadenas de error, pistas de remediación, tooltips y etiquetas accesibles, certificando cero ocurrencias de términos internos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «toolchain»).
  - *Comando*: `grep -E "testPublicCopyExhaustiveAudit" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-007: Implementar prueba de estrés de concurrencia masiva con 16 hilos en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` contiene una prueba de concurrencia que lanza 16 hilos concurrentes compitiendo sobre una instancia compartida de `TitleBarService` (redimensionamiento, conmutación de densidad, mutación de colocación y consultas simultáneas), completando $\ge 200$ iteraciones por hilo sin carreras de datos ni interbloqueos.
  - *Comando*: `grep -E "testMultiThreadedHighStressConcurrency16Threads" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-008: Implementar prueba de cancelación cooperativa asíncrona mediante `CancellationToken` en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` prueba la activación asíncrona de `CancellationToken` durante operaciones del servicio, verificando respuesta inmediata (< 10 ms), aborto seguro y preservación del plan activo.
  - *Comando*: `grep -E "testCooperativeCancellationStress" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de fuzzing y resiliencia ante archivos de persistencia corruptos en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` inyecta archivos JSON truncados, vacíos, con sintaxis rota o con valores geométricos inválidos en `loadStateFromFile`, certificando que el servicio retorna `false` y restablece de forma transparente la configuración canónica por defecto (`canonicalDefaultTitleBar()`) sin caídas.
  - *Comando*: `grep -E "testCorruptedPersistenceFuzzingAndRecovery" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de simulación de fallos de disco y permisos denegados en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` simula operaciones de guardado en directorios con permisos de solo lectura (`perms::read_only`), verificando que `saveStateToFile` maneja el error de forma segura retornando `false` sin abortos ni excepciones no capturadas.
  - *Comando*: `grep -E "testDiskIoFailureHandling" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-011: Implementar pruebas de aislamiento estricto de sesión privada / modo incógnito en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` certifica que cuando `isIncognitoMode() == true`, las llamadas a `saveStateToFile` y `loadStateFromFile` retornan inmediatamente sin realizar operaciones de lectura o escritura en el sistema de archivos local.
  - *Comando*: `grep -E "testIncognitoStrictPrivacyIsolation" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-012: Implementar pruebas de transiciones continuas de colocación de controles y mutaciones de estado de ventana en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`
  - *Criterio de verificación*: `TitleBarServiceTests.cpp` evalúa la conmutación alternada `TitleBarControlsPlacement::Left` $\leftrightarrow$ `Right` y mutaciones continuas de `TitleBarState` (`isMaximized`, `isFullscreen`, `isActive`), comprobando la coherencia geométrica de la caja de controles y el orden canónico de botones.
  - *Comando*: `grep -E "testControlsPlacementAndStateTransitionsExhaustive" apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`

- [ ] TASK-013: Refinar aserciones de integración de barra de título, eventos nativos de ventana, foco de teclado y tap targets $\ge 48\times 48$ px en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la integración de `BrowserWindow` con `TitleBarService`, la reacción ante eventos `resizeEvent` y `WindowStateChange`, las dimensiones de tap target $\ge 48\times 48$ px en modo accesible, foco por teclado y tolerancia limpia en entornos headless (`NOT_EXECUTED` si display ausente).
  - *Comando*: `grep -E "titleBarService|vigilio-titlebar|titleBarMaximizeButton" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-014: Verificar formato canónico con Clang-Format sobre archivos modificados
  - *Criterio de verificación*: `clang-format --dry-run --Werror` reporta cero discrepancias sobre todos los archivos de prueba modificados.
  - *Comando*: `clang-format --dry-run --Werror apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-015: Verificar análisis estático con Clang-Tidy sobre archivos modificados
  - *Criterio de verificación*: El análisis estático no arroja advertencias ni violaciones sobre las unidades de traducción modificadas.
  - *Comando*: `test -f apps/vigilio-browser/_build/compile_commands.json && clang-tidy -p apps/vigilio-browser/_build apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp || true`

- [ ] TASK-016: Compilar y ejecutar la suite completa de pruebas en CTest
  - *Criterio de verificación*: CMake compila limpiamente bajo C++20 con `-Werror` y CTest ejecuta el 100% de las pruebas con éxito (exit code 0).
  - *Comando*: `cmake --build apps/vigilio-browser/_build && ctest --test-dir apps/vigilio-browser/_build --output-on-failure -L fast`

- [ ] TASK-017: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `tests` y mantiene sincronizados los criterios de aceptación y verificación.
  - *Comando*: `grep -E "module.ux-shell.task0067" docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md`
