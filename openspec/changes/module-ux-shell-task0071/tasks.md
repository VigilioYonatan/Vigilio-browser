# Tasks — module-ux-shell-task0071

Status: PROPOSED

- [ ] TASK-001: Ampliar pruebas de fronteras dimensionales exactas y entradas negativas en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` incluye pruebas que evalúan las dimensiones en las fronteras exactas de fallo (ancho $399$ px, $200$ px, $0$ px y valores negativos patológicos), verificando el retorno estricto de `TabStripDensityError::WidthBelowMinimum`.
  - *Comando*: `grep -E "testSubMinimumDimensionsExhaustive" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-002: Ampliar pruebas de violación de altura en la «Interfaz más fácil de usar» y tap targets accesibles en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` contiene pruebas que verifican que alturas $< 48$ px en densidad Senior retornan `TabStripDensityError::SeniorModeMinimumHeightViolation` y que dimensiones insuficientes de tap target retornan `TabStripDensityError::SeniorModeTapTargetViolation`.
  - *Comando*: `grep -E "testSeniorModeHeightViolations" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-003: Ampliar pruebas de compresión elástica extrema (100 pestañas en 400 px) y desbordamiento horizontal en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` evalúa el comportamiento elástico ante 100 pestañas en un contenedor estrecho de $400$ px, certificando la activación determinista de `TabStripOverflowMode::Scrollable` y la invariante de no solapamiento sin colapso de geometrías.
  - *Comando*: `grep -E "testExtremeOverflowAndElasticCompression" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-004: Ampliar pruebas de saturación por pestañas fijadas y agotamiento de ancho disponible en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` valida escenarios donde múltiples pestañas fijadas saturan el ancho disponible, evaluando la detección analítica de `TabStripDensityError::AvailableWidthExhausted` y la preservación del botón de nueva pestaña.
  - *Comando*: `grep -E "testPinnedTabsSaturationAndSpaceExhaustion" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-005: Ampliar pruebas de hit-testing espacial cartesiano exhaustivo de esquinas, bordes y puntos exteriores en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` evalúa las coordenadas de las cuatro esquinas de cada pestaña, icono, título, botón de cierre e indicador de audio, así como puntos a $\pm 1$ px fuera de los límites y retorno de `TabStripHitTestZone::Nowhere`.
  - *Comando*: `grep -E "testExhaustiveHitTestingBoundaries" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-006: Ampliar pruebas de sanitización hostil de títulos y protección de privacidad en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` evalúa cadenas con credenciales embebidas (`user:password@host`), parámetros sensibles (`token=`, `key=`), caracteres de control (`\r`, `\n`, `\t`, bytes nulos `\0`) y cadenas de longitud extrema, verificando la sanitización limpia sin fuga de datos sensibles y el fallback seguro en cadenas vacías.
  - *Comando*: `grep -E "testHostileTitleSanitizationAndPrivacy" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-007: Ampliar auditoría de textos públicos y pistas de remediación frente a `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` audita programáticamente todas las cadenas de error, pistas de remediación, tooltips y etiquetas accesibles, certificando cero ocurrencias de términos internos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «toolchain»).
  - *Comando*: `grep -E "testPublicCopyExhaustiveAudit" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-008: Implementar prueba de estrés de concurrencia masiva con 16 hilos en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` contiene una prueba de concurrencia que lanza 16 hilos concurrentes compitiendo sobre una instancia compartida de `TabStripDensityService` (mutación de pestañas, redimensionamiento, conmutación de densidad y consultas simultáneas), completando $\ge 200$ iteraciones por hilo sin carreras de datos ni interbloqueos.
  - *Comando*: `grep -E "testMultiThreadedHighStressConcurrency16Threads" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-009: Implementar prueba de cancelación cooperativa asíncrona mediante `CancellationToken` en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` prueba la activación asíncrona de `CancellationToken` durante operaciones del servicio, verificando respuesta inmediata (< 10 ms), aborto seguro y preservación del plan activo.
  - *Comando*: `grep -E "testCooperativeCancellationStress" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de fuzzing y resiliencia ante archivos de persistencia corruptos en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` inyecta archivos JSON truncados, vacíos, con sintaxis rota, campos con tipos incongruentes o valores numéricos negativos en `loadStateFromFile`, certificando que el servicio retorna `false` y restablece de forma transparente la configuración canónica por defecto (`canonicalDefaultDensity()`) sin caídas.
  - *Comando*: `grep -E "testCorruptedPersistenceFuzzingAndRecovery" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-011: Implementar pruebas de simulación de fallos de disco y permisos denegados en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` simula operaciones de guardado en directorios con permisos de solo lectura (`perms::read_only`), verificando que `saveStateToFile` maneja el error de forma segura retornando `false` sin abortos ni excepciones no capturadas.
  - *Comando*: `grep -E "testDiskIoFailureHandling" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-012: Implementar pruebas de aislamiento estricto de sesión privada / modo incógnito en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` certifica que cuando `isIncognitoMode() == true`, las llamadas a `saveStateToFile` y `loadStateFromFile` retornan inmediatamente sin realizar operaciones de lectura o escritura en el sistema de archivos local.
  - *Comando*: `grep -E "testIncognitoStrictPrivacyIsolation" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-013: Implementar pruebas de ciclo de vida de pestañas y mutaciones masivas en ráfaga en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`
  - *Criterio de verificación*: `TabStripDensityServiceTests.cpp` evalúa un bucle de 50 operaciones alternadas de adición y eliminación de pestañas, validando la estabilidad de la selección adyacente y la coherencia del estado sin excepciones ni punteros colgantes.
  - *Comando*: `grep -E "testContinuousTabMutationsBurst" apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`

- [ ] TASK-014: Refinar aserciones de integración de tira de pestañas, eventos nativos de ventana, foco de teclado y tap targets $\ge 48\times 48$ px en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: `WindowTests.cpp` valida la presencia de `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()`, tap targets $\ge 48\times 48$ px en modo accesible, foco por teclado y tolerancia limpia en entornos headless (`NOT_EXECUTED` si display ausente).
  - *Comando*: `grep -E "tabStripDensityService|vigilio-tabs|tabNewButton" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-015: Verificar formato canónico con Clang-Format sobre archivos modificados
  - *Criterio de verificación*: `clang-format --dry-run --Werror` reporta cero discrepancias sobre todos los archivos de prueba modificados.
  - *Comando*: `clang-format --dry-run --Werror apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-016: Verificar análisis estático con Clang-Tidy sobre archivos modificados
  - *Criterio de verificación*: El análisis estático no arroja advertencias ni violaciones sobre las unidades de traducción modificadas.
  - *Comando*: `test -f build/compile_commands.json && clang-tidy -p build apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp || true`

- [ ] TASK-017: Compilar y ejecutar la suite completa de pruebas en CTest
  - *Criterio de verificación*: CMake compila limpiamente bajo C++20 con `-Werror` y CTest ejecuta el 100% de las pruebas con éxito (exit code 0).
  - *Comando*: `cmake --build build && ctest --test-dir build --output-on-failure -L fast`

- [ ] TASK-018: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `tests` y mantiene sincronizados los criterios de aceptación y verificación.
  - *Comando*: `grep -E "module.ux-shell.task0071" docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md`
