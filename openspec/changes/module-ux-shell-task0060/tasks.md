# Tasks — module-ux-shell-task0060

Status: PROPOSED

- [ ] TASK-001: Modelar tipos de datos y taxonomía fuertemente tipada de zonas de ventana en `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`
  - *Criterio de verificación*: `MainWindowLayoutTypes.h` define `enum class LayoutZone : uint8_t`, `LayoutDensity : uint8_t`, `SidebarState : uint8_t`, `SidebarDockPosition : uint8_t`, `WindowChromeMode : uint8_t`, `MainWindowLayoutError : uint8_t` y las estructuras inmutables `WindowDimensions`, `LayoutMargins`, `LayoutRect`, `ZoneBudget`, `SidebarConfig`, `MainWindowLayoutPlan`, `ZoneGeometry` y `MainWindowLayoutReport`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/MainWindowLayoutTypes.h && grep -E "enum class LayoutZone|enum class MainWindowLayoutError|struct MainWindowLayoutPlan" apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`

- [ ] TASK-002: Especificar la configuración canónica e invariantes de layout en `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h` y `.cpp`
  - *Criterio de verificación*: `MainWindowLayoutConfiguration` define constantes canónicas para dimensiones estándar ($1200\times 780$, mín $640\times 480$), Senior (mín $720\times 520$, toolbar $\ge 56$, tap targets $\ge 48$) y Compact (mín $500\times 380$), factorías `canonicalDefaultLayout()`, `canonicalSeniorLayout()`, `canonicalCompactLayout()` y la función `verifyInvariants()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h && grep -E "canonicalDefaultLayout|canonicalSeniorLayout|verifyInvariants" apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h`

- [ ] TASK-003: Implementar el validador estático desacoplado y el algoritmo espacial en `apps/vigilio-browser/src/core/MainWindowLayoutValidator.h` y `.cpp`
  - *Criterio de verificación*: `MainWindowLayoutValidator` implementa métodos puros en C++20 `validateDimensions`, `validateSidebar`, `validatePlan` y `calculateLayoutAreas`, garantizando la disyunción matemática de zonas ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y la suficiencia de área central ($W \ge 400$, $H \ge 250$).
  - *Comando*: `test -f apps/vigilio-browser/src/core/MainWindowLayoutValidator.h && grep -E "calculateLayoutAreas|validateDimensions|validatePlan" apps/vigilio-browser/src/core/MainWindowLayoutValidator.h`

- [ ] TASK-004: Implementar las funciones de lenguaje público y descripciones accesibles en español neutro en `MainWindowLayoutValidator.cpp` conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: `layoutErrorToString`, `layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `sidebarDockPositionToString`, `chromeModeToString` y `generateRemediationHint` devuelven cadenas en lenguaje natural sin tecnicismos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).
  - *Comando*: `grep -E "layoutErrorToString|layoutZoneToString|generateRemediationHint" apps/vigilio-browser/src/core/MainWindowLayoutValidator.h && ! grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/MainWindowLayoutValidator.cpp`

- [ ] TASK-005: Registrar los nuevos componentes en la biblioteca `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `MainWindowLayoutConfiguration.cpp` y `MainWindowLayoutValidator.cpp` quedan agregados a la lista de fuentes de `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
  - *Comando*: `grep -E "MainWindowLayoutConfiguration\.cpp" apps/vigilio-browser/CMakeLists.txt && grep -E "MainWindowLayoutValidator\.cpp" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-006: Declarar el target ejecutable de pruebas unitarias `vigilio_main_window_layout_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` define el target ejecutable `vigilio_main_window_layout_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`, y lo registra en CTest con las etiquetas `unit;fast`.
  - *Comando*: `grep -A 8 "add_executable(vigilio_main_window_layout_tests" apps/vigilio-browser/CMakeLists.txt | grep -E "vigilio_core|unit;fast"`

- [ ] TASK-007: Implementar la suite de 12 pruebas unitarias de especificación en `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: `MainWindowLayoutTests.cpp` compila limpiamente y ejecuta las 12 pruebas unitarias cubriendo planes canónicos, validación de dimensiones, rangos de barra lateral, disyunción de áreas, acople lateral y copy público.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp && grep -E "testCanonicalDefaultLayoutInvariants|testCanonicalSeniorLayoutInvariants|testDimensionsValidationStandard" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`

- [ ] TASK-008: Verificar el comportamiento de cálculo de áreas y no solapamiento espacial en `MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: La prueba `testLayoutCalculationDisjointAreas` calcula analíticamente la geometría de una ventana $1200\times 780$ px y certifica mediante aserciones formales que ningún par de zonas colisiona espacialmente.
  - *Comando*: `grep -A 20 "testLayoutCalculationDisjointAreas" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp | grep -E "intersects"`

- [ ] TASK-009: Verificar el comportamiento y límites dimensionales de la «Interfaz más fácil de usar» (Modo Senior) en `MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: La prueba `testCanonicalSeniorLayoutInvariants` certifica que el plan accesible exige una altura mínima de toolbar $\ge 56$ px, tap targets $\ge 48$ px y rechaza ventanas de dimensiones inferiores a $720\times 520$ px.
  - *Comando*: `grep -A 15 "testCanonicalSeniorLayoutInvariants" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp | grep -E "minWidth.*>=.*720|minHeight.*>=.*520"`

- [ ] TASK-010: Verificar la gestión dinámica de avisos y desplazamiento del área de contenido con `NotificationBannerZone` en `MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: La prueba `testLayoutCalculationWithBanner` comprueba que la activación del banner disminuye exactamente en su altura ($44$ px estándar / $56$ px Senior) el espacio disponible para el contenido central sin desbordamiento.
  - *Comando*: `grep -A 15 "testLayoutCalculationWithBanner" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp | grep -E "NotificationBanner"`

- [ ] TASK-011: Auditar la ausencia absoluta de terminología técnica interna prohibida en todas las cadenas emitidas por el modelo de dominio en `MainWindowLayoutTests.cpp`
  - *Criterio de verificación*: La prueba `testUxCopySpanishCompliance` evalúa todas las funciones de formateo público y verifica que devuelven descripciones comprensibles en español neutro, sin contener «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» ni «OpenSpec».
  - *Comando*: `grep -A 25 "testUxCopySpanishCompliance" apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp | grep -E "layoutErrorToString|layoutDensityToString"`

- [ ] TASK-012: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: El script `check.sh` compila con `-Werror`, ejecuta Clang-Format y corre exitosamente el 100% de las pruebas en CTest (26/26 pruebas pasando) con exit code 0 en menos de 10 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-013: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0178, y `task.md` formaliza el cumplimiento de la etapa `domain`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0178-module.ux-shell.task0060.propose.lock.json --project-root . && grep -E "module.ux-shell.task0060" docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md`
