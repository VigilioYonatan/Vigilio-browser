# Tasks — module-ux-shell-task0064

Status: PROPOSED

- [ ] TASK-001: Modelar tipos de datos y taxonomía fuertemente tipada de barra de título en `apps/vigilio-browser/src/core/TitleBarTypes.h`
  - *Criterio de verificación*: `TitleBarTypes.h` define `enum class TitleBarMode : uint8_t`, `TitleBarControlsPlacement : uint8_t`, `TitleBarButtonType : uint8_t`, `TitleBarButtonState : uint8_t`, `TitleBarHitTestZone : uint8_t`, `TitleBarError : uint8_t` y las estructuras inmutables `TitleBarDimensions`, `TitleBarControlSpec`, `TitleBarState`, `TitleBarConfig`, `TitleBarLayoutPlan` y `TitleBarReport`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TitleBarTypes.h && grep -E "enum class TitleBarMode|enum class TitleBarHitTestZone|struct TitleBarLayoutPlan" apps/vigilio-browser/src/core/TitleBarTypes.h`

- [ ] TASK-002: Especificar la configuración canónica, constantes dimensionales e invariantes en `apps/vigilio-browser/src/core/TitleBarConfiguration.h` y `.cpp`
  - *Criterio de verificación*: `TitleBarConfiguration` define constantes canónicas para alturas estándar (32 px), Senior (48 px) y Compact (28 px), dimensiones de botones ($46\times 32$ px estándar, $\ge 48\times 48$ px Senior), factorías `canonicalDefaultTitleBar()`, `canonicalSeniorTitleBar()`, `canonicalCompactTitleBar()` y el método `verifyInvariants()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/TitleBarConfiguration.h && grep -E "canonicalDefaultTitleBar|canonicalSeniorTitleBar|verifyInvariants" apps/vigilio-browser/src/core/TitleBarConfiguration.h`

- [ ] TASK-003: Implementar el validador estático, algoritmo de distribución espacial y hit-testing en `apps/vigilio-browser/src/core/TitleBarValidator.h` y `.cpp`
  - *Criterio de verificación*: `TitleBarValidator` implementa métodos puros en C++20 `validateDimensions`, `validatePlan`, `calculateTitleBarLayout` y `hitTest`, garantizando la disyunción matemática de botones ($\forall i \ne j, C_i \cap C_j = \emptyset$) y la suficiencia de área de arrastre ($\ge 120$ px).
  - *Comando*: `test -f apps/vigilio-browser/src/core/TitleBarValidator.h && grep -E "calculateTitleBarLayout|hitTest|validateDimensions" apps/vigilio-browser/src/core/TitleBarValidator.h`

- [ ] TASK-004: Implementar las funciones de lenguaje público y textos accesibles en español neutro en `TitleBarValidator.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `titleBarErrorToString`, `titleBarModeToString`, `hitTestZoneToString`, `buttonTypeToString`, `buttonAccessibleLabel`, `buttonTooltip` y `generateRemediationHint` devuelven cadenas en lenguaje natural sin tecnicismos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).
  - *Comando*: `grep -E "titleBarErrorToString|buttonAccessibleLabel|generateRemediationHint" apps/vigilio-browser/src/core/TitleBarValidator.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/TitleBarValidator.cpp`

- [ ] TASK-005: Registrar los nuevos fuentes en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `TitleBarConfiguration.cpp` y `TitleBarValidator.cpp` quedan agregados a la lista de fuentes de `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
  - *Comando*: `grep -E "TitleBarConfiguration\.cpp" apps/vigilio-browser/CMakeLists.txt && grep -E "TitleBarValidator\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Declarar el target ejecutable de pruebas unitarias `vigilio_title_bar_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_title_bar_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_title_bar_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-007: Implementar la suite de 14 pruebas unitarias de especificación en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: `TitleBarIntegrationTests.cpp` compila limpiamente y ejecuta las 14 pruebas unitarias cubriendo planes canónicos, hit-testing, alternancia de estados, controles izquierda/derecha, Modo Senior y copy público.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp && grep -E "testCanonicalDefaultTitleBarInvariants|testCanonicalSeniorTitleBarInvariants|testTitleBarHitTestCaptionAndDrag" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`

- [ ] TASK-008: Verificar el comportamiento de hit-testing analítico de zonas y controles en `TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: Las pruebas `testTitleBarHitTestCaptionAndDrag` y `testTitleBarHitTestWindowControls` certifican que `hitTest(x, y)` clasifica con precisión de píxel las regiones `DraggableSpace`, `WindowIcon`, `MinimizeButton`, `MaximizeButton`, `CloseButton` y `SeniorModeToggle`.
  - *Comando*: `grep -A 25 "testTitleBarHitTestWindowControls" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp | grep -E "CloseButton|MinimizeButton|MaximizeButton"`

- [ ] TASK-009: Verificar el comportamiento y límites dimensionales de la «Interfaz más fácil de usar» (Modo Senior) en `TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: Las pruebas `testCanonicalSeniorTitleBarInvariants` y `testTitleBarSeniorMinimumHeightViolation` certifican que el plan accesible exige una altura mínima de barra $\ge 48$ px, botones $\ge 48\times 48$ px, arrastre $\ge 160$ px y rechaza configuraciones con altura $< 48$ px con `SeniorModeMinimumHeightViolation`.
  - *Comando*: `grep -A 20 "testCanonicalSeniorTitleBarInvariants" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp | grep -E "kSeniorTitleBarHeight.*>=.*48|kSeniorButtonWidth.*>=.*48"`

- [ ] TASK-010: Verificar el soporte de posicionamiento de controles a la izquierda (macOS / zurdos) en `TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: La prueba `testTitleBarLeftControlsPlacement` comprueba que configurar `TitleBarControlsPlacement::Left` posiciona la caja de controles en $x = 0$ con el orden canónico `Close`, `Minimize`, `MaximizeRestore` sin solapamiento con el área de arrastre.
  - *Comando*: `grep -A 20 "testTitleBarLeftControlsPlacement" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp | grep -E "TitleBarControlsPlacement::Left"`

- [ ] TASK-011: Auditar la ausencia absoluta de terminología técnica interna prohibida en todas las cadenas emitidas por el modelo de dominio en `TitleBarIntegrationTests.cpp`
  - *Criterio de verificación*: La prueba `testTitleBarUxCopySpanishCompliance` evalúa todas las funciones de formateo público y verifica que devuelven descripciones comprensibles en español neutro, sin contener «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» ni «OpenSpec».
  - *Comando*: `grep -A 25 "testTitleBarUxCopySpanishCompliance" apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp | grep -E "titleBarErrorToString|buttonAccessibleLabel"`

- [ ] TASK-012: Ejecutar el pipeline de compilación limpia y pruebas continuas con CTest
  - *Criterio de verificación*: El build compila con `-Werror`, ejecuta Clang-Format y corre exitosamente el 100% de las pruebas en CTest (28/28 pruebas pasando) con exit code 0 en menos de 10 segundos.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build --output-on-failure`

- [ ] TASK-013: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0064-title-bar-integration/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0190, y `task.md` formaliza el cumplimiento de la etapa `domain`.
  - *Comando*: `vigilio-prompt validate-references --project-root . && vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0190-module.ux-shell.task0064.propose.lock.json --project-root . && grep -E "module.ux-shell.task0064" docs/rules/01-modules/ux-shell/micro/0064-title-bar-integration/task.md`
