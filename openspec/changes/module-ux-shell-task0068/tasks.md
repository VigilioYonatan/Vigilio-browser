# Tasks — module-ux-shell-task0068

Status: PROPOSED

- [ ] TASK-001: Modelar tipos de datos y taxonomía fuertemente tipada de tira de pestañas en `apps/vigilio-browser/src/core/TabStripDensityTypes.h`
  - *Criterio de verificación*: `TabStripDensityTypes.h` define `enum class TabPinState : uint8_t`, `TabMediaState : uint8_t`, `TabDiscardState : uint8_t`, `TabLoadingState : uint8_t`, `TabInteractiveState : uint8_t`, `TabStripHitTestZone : uint8_t`, `TabStripOverflowMode : uint8_t`, `TabStripDensityError : uint8_t` y las estructuras inmutables `TabStripDimensions`, `TabSpec`, `TabItemLayout`, `TabStripDensityConfig`, `TabStripDensityPlan` y `TabStripDensityReport`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TabStripDensityTypes.h && grep -E "enum class TabStripHitTestZone|enum class TabStripDensityError|struct TabStripDensityPlan" apps/vigilio-browser/src/core/TabStripDensityTypes.h`

- [ ] TASK-002: Especificar la configuración canónica, constantes dimensionales e invariantes en `apps/vigilio-browser/src/core/TabStripDensityConfiguration.h` y `.cpp`
  - *Criterio de verificación*: `TabStripDensityConfiguration` define constantes canónicas para alturas estándar (36 px), Senior (48 px) y Compact (28 px), anchos preferidos, mínimos y máximos de pestañas regulares, anchos de pestañas fijadas, factorías `canonicalDefaultDensity()`, `canonicalSeniorDensity()`, `canonicalCompactDensity()` y el método estático de verificación geométrica `verifyInvariants()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TabStripDensityConfiguration.h && grep -E "canonicalDefaultDensity|canonicalSeniorDensity|verifyInvariants" apps/vigilio-browser/src/core/TabStripDensityConfiguration.h`

- [ ] TASK-003: Implementar el validador estático, algoritmo de compresión elástica y hit-testing en `apps/vigilio-browser/src/core/TabStripDensityValidator.h` y `.cpp`
  - *Criterio de verificación*: `TabStripDensityValidator` implementa métodos puros en C++20 `validateDimensions`, `validatePlan`, `calculateTabStripLayout` y `hitTest`, garantizando la disyunción matemática entre pestañas ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$), el dimensionamiento elástico acotado y la detección de desbordamiento horizontal.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TabStripDensityValidator.h && grep -E "calculateTabStripLayout|hitTest|validateDimensions" apps/vigilio-browser/src/core/TabStripDensityValidator.h`

- [ ] TASK-004: Implementar las funciones de lenguaje público y textos accesibles en español neutro en `TabStripDensityValidator.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `tabStripErrorToString`, `densityToString`, `hitTestZoneToString`, `tabAccessibleLabel`, `tabTooltip`, `closeButtonAccessibleLabel`, `audioButtonAccessibleLabel`, `newTabButtonAccessibleLabel` y `generateRemediationHint` devuelven cadenas en lenguaje natural sin tecnicismos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner»).
  - *Comando*: `grep -E "tabStripErrorToString|tabAccessibleLabel|generateRemediationHint" apps/vigilio-browser/src/core/TabStripDensityValidator.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/TabStripDensityValidator.cpp`

- [ ] TASK-005: Registrar los nuevos fuentes en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `TabStripDensityConfiguration.cpp` y `TabStripDensityValidator.cpp` quedan agregados a la lista de fuentes de `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
  - *Comando*: `grep -E "TabStripDensityConfiguration\.cpp" apps/vigilio-browser/CMakeLists.txt && grep -E "TabStripDensityValidator\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Declarar el target ejecutable de pruebas unitarias `vigilio_tab_strip_density_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_tab_strip_density_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_tab_strip_density_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-007: Implementar las pruebas de invariantes canónicas para densidades Estándar, Senior y Compacta en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`
  - *Criterio de verificación*: `TabStripDensityTests.cpp` compila limpiamente y ejecuta pruebas unitarias que verifican que los planes por defecto, Senior y Compacto satisfacen todas las invariantes geométricas sin solapamiento espacial.
  - *Comando*: `grep -E "testCanonicalDefaultInvariants|testCanonicalSeniorInvariants|testCanonicalCompactInvariants" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-008: Implementar las pruebas de separación geométrica y tamaño invariante de pestañas fijadas (*pinned tabs*) en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas verifican que las pestañas fijadas se ubican en el extremo izquierdo ($x_{\text{pinned}} < x_{\text{regular}}$), preservan su ancho canónico ($36$ px estándar, $\ge 48$ px Senior) y no sufren compresión ante el aumento de pestañas regulares.
  - *Comando*: `grep -E "testPinnedTabsGeometricSeparation" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-009: Implementar las pruebas de compresión elástica uniforme bajo número creciente de pestañas en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas validan que a medida que se agregan pestañas regulares en un ancho constante de tira, el ancho de cada pestaña disminuye de forma monotónica y uniforme entre `preferredTabWidth` y `minTabWidth`.
  - *Comando*: `grep -E "testElasticShrinkingUnderHighTabCount" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-010: Implementar las pruebas de detección de desbordamiento horizontal (*overflow*) y activación de modo desplazamiento en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas validan que cuando la suma de anchos de pestañas mínimas excede el espacio disponible, el plan activa `isOverflowing == true` y fija `overflowMode = TabStripOverflowMode::Scrollable`.
  - *Comando*: `grep -E "testOverflowDetectionAndModeActivation" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-011: Implementar las pruebas de ocultación condicional del botón de cierre por compresión y su permanencia obligatoria en la «Interfaz más fácil de usar» en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas certifican que en densidades Estándar y Compacta las pestañas inactivas comprimidas por debajo de 90 px ocultan el botón de cierre, mientras que en Modo Senior el botón de cierre permanece visible con hit target $\ge 48\times 48$ px.
  - *Comando*: `grep -E "testCloseButtonVisibilityThreshold|testSeniorModeCloseButtonPermanence" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-012: Implementar las pruebas de hit-testing espacial analítico de todas las zonas de la tira en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas verifican que `hitTest(x, y)` clasifica con precisión de píxel las coordenadas dentro de `TabBody`, `TabFavicon`, `TabTitle`, `TabCloseButton`, `TabAudioIndicator`, `NewTabButton`, `EmptySpace` y devuelve `Nowhere` fuera de los límites.
  - *Comando*: `grep -E "testExhaustiveHitTestingZones|testHitTestingNowhereAndBoundaries" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-013: Implementar las pruebas de representación de estados de audio/silencio y ahorro de memoria («Pestaña en reposo») en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: Las pruebas evalúan las transiciones a `TabMediaState::AudioPlaying`/`AudioMuted` y `TabDiscardState::Discarded`, comprobando la asignación del rectángulo del indicador de audio y la etiqueta accesible de «Pestaña en reposo».
  - *Comando*: `grep -E "testMediaIndicatorStateAndHitTest|testTabDiscardStateRepresentation" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-014: Implementar las pruebas de sanitización de títulos de pestañas para protección de privacidad en modo incógnito en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: La prueba valida que URLs y títulos con credenciales embebidas (`user:pass@host`), parámetros sensibles o identificadores de sesión son filtrados de forma limpia sin fuga de datos sensibles.
  - *Comando*: `grep -E "testTitleSanitizationForPrivacy" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-015: Auditar la ausencia absoluta de terminología técnica interna proscrita en todas las cadenas emitidas por el modelo de dominio en `TabStripDensityTests.cpp`
  - *Criterio de verificación*: La prueba audita programáticamente todas las cadenas devueltas por las funciones públicas del validador y certifica cero ocurrencias de términos internos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).
  - *Comando*: `grep -E "testUxCopySpanishCompliance" apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-016: Verificar formato canónico con Clang-Format sobre archivos modificados y nuevos
  - *Criterio de verificación*: `clang-format --dry-run --Werror` reporta cero discrepancias sobre todos los archivos de cabecera, implementación y pruebas creados.
  - *Comando*: `clang-format --dry-run --Werror apps/vigilio-browser/src/core/TabStripDensityTypes.h apps/vigilio-browser/src/core/TabStripDensityConfiguration.h apps/vigilio-browser/src/core/TabStripDensityConfiguration.cpp apps/vigilio-browser/src/core/TabStripDensityValidator.h apps/vigilio-browser/src/core/TabStripDensityValidator.cpp apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`

- [ ] TASK-017: Verificar análisis estático con Clang-Tidy sobre archivos modificados y nuevos
  - *Criterio de verificación*: El análisis estático no arroja advertencias ni violaciones sobre las unidades de traducción creadas.
  - *Comando*: `test -f apps/vigilio-browser/_build/compile_commands.json && clang-tidy -p apps/vigilio-browser/_build apps/vigilio-browser/src/core/TabStripDensityConfiguration.cpp apps/vigilio-browser/src/core/TabStripDensityValidator.cpp apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp || true`

- [ ] TASK-018: Compilar y ejecutar la suite completa de pruebas en CTest
  - *Criterio de verificación*: CMake compila limpiamente bajo C++20 con `-Werror` y CTest ejecuta el 100% de las pruebas (30/30 pruebas) con éxito (exit code 0).
  - *Comando*: `cmake --build apps/vigilio-browser/_build && ctest --test-dir apps/vigilio-browser/_build --output-on-failure -L fast`

- [ ] TASK-019: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0068-tab-strip-density/task.md`
  - *Criterio de verificación*: `task.md` documenta la finalización de la etapa `domain` y mantiene sincronizados los criterios de aceptación y verificación.
  - *Comando*: `grep -E "module.ux-shell.task0068" docs/rules/01-modules/ux-shell/micro/0068-tab-strip-density/task.md`
