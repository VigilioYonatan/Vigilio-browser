# Execution Memory — module-foundations-task0035

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **ui** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0035 — Implementar la interacción y estados visuales en Qt: CMake presets`), proyectando la gobernanza de perfiles de compilación con interfaz de usuario en `CMakePresets.json` y `CMakeLists.txt`, la formalización contractual de «sin cambio visual directo» en la interfaz pública conforme a `docs/UX-COPY-PUBLICO.md`, el desacoplamiento estricto entre el servicio `vigilio::CMakePresetsService` y la ventana `BrowserWindow`, y la verificación de accesibilidad y tolerancia headless en `WindowTests.cpp`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura, independencia del núcleo C++20 respecto a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, contratos de fallos y recuperación local.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `integration`, `ui;fast`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas locales y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0035-cmake-presets/task.md`: Contrato vinculante de la microtarea 0035.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 código abierto, Q-004 soporte multiplataforma Win/Linux/macOS, Q-005 actualizaciones manuales, Q-006 assets en `apps/vigilio-browser/assets`, Q-007 blocks only `release.public-release`, Q-008 firma builds).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución actual: `0103`
  - Pack: `module.foundations.task0035`
  - Phase: `propose`
  - Generated artifact: `0103-module.foundations.task0035.propose.md`
  - Prompt SHA-256: `273cbead5341df7977a8401d9cb944b362c5fd3aea61d4f487163664244a93ee`
  - Lock SHA-256: `8f1673d5c294b5ce9e95f31a58389f66c73c9d97ef6d117ba43ed19a8ddde028`
  - Dependencias ejecutables: `module.foundations.task0034`
  - Paso de fase anterior: `none`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre la orquestación de compilación de presets en `src/core` y los componentes de visualización e interacción en `src/ui`.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas, aserciones deterministas de interfaz en `vigilio_ui_tests` (`WindowTests.cpp`), y tolerancia a entornos de CI headless reportando honestamente `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, no bloqueo del hilo de interfaz, desacoplamiento estricto entre el servicio `CMakePresetsService` y `BrowserWindow`.
- `docs/skills/04-ux-ui-senior.md`: Estilo visual sobrio, sin estridencias, iconos SVG consistentes, coherencia con `/resultado/01-vigilio-home.png`.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Adaptabilidad dinámica en «Interfaz más fácil de usar» (Modo Senior), iconos de 22x22 escalando a 32x32 px, tap targets mínimos $\ge 48$ px, barra de herramientas $\ge 55$ px, fuentes +4 pt, navegación accesible por teclado y soporte para lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de memoria; no inicializar procesos Chromium innecesarios en la validación de presets; carga diferida de pestañas (lazy loading).
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero telemetría remota; aislamiento absoluto en sesiones de incógnito sin persistencia en disco ni banners de recuperación.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Proscripción de jerga de ingeniería interna («CMake», «Presets», «Toolchain», «Prompt Builder», «OpenSpec», «Senior Mode») en interfaces públicas.
- Referencias visuales en `/resultado/`:
  - `resultado/01-vigilio-home.png`: Ventana principal de navegación.
  - `resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
  - `resultado/06-logo-instalador-oficial.png`: Logotipo oficial.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la gestión y ejecución de presets se realiza exclusivamente con herramientas del sistema local. Cero llamadas de red, telemetría o servicios remotos.
  2. *Sin cambio visual directo en la UI pública*: Conforme a `docs/UX-COPY-PUBLICO.md`, la capacidad CMake Presets es un componente de infraestructura de compilación técnica; no introduce widgets ni menús en la UI pública de usuario general.
  3. *Desacoplamiento Estricto*: `vigilio::CMakePresetsService` reside íntegramente en `vigilio_core`, sin referencias a `QWidget`, `QApplication` ni `QWebEngineView`. `BrowserWindow` no almacena ni ejecuta procesos de compilación.
  4. *Gobernanza de Presets de UI*: Los perfiles `desktop-debug`, `desktop-release` y `desktop-e2e` configuran y compilan los artefactos visuales bajo `VIGILIO_BUILD_GUI=ON`, mientras que los perfiles `core-debug`, `core-release` y `ci-linux-*` garantizan la compatibilidad headless con `VIGILIO_BUILD_GUI=OFF`.
  5. *Tolerancia Headless*: `WindowTests.cpp` inspecciona `DISPLAY` y `WAYLAND_DISPLAY`, retornando limpiamente `NOT_EXECUTED` si falta display gráfico, evitando falsos positivos o bloqueos de CI.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CMakePresetsTypes.h`: Tipos y estructuras base del dominio y servicio de presets.
  - `apps/vigilio-browser/src/core/CMakePresetsValidator.h` & `.cpp`: Validador estático, parser JSON y reglas de higiene de rutas (`isSafeBinaryDir`).
  - `apps/vigilio-browser/src/core/CMakePresetsService.h` & `.cpp`: Servicio operativo de presets en `src/core`.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` & `.cpp`: Ventana gráfica principal y controles de usuario.
  - `apps/vigilio-browser/CMakePresets.json`: Catálogo canónico con los 9 perfiles estándar versión 6.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Suite de pruebas Qt Test para contratos de interfaz.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Acoplamiento indebido de lógica de CMake dentro de los widgets de navegación.
    - *Mitigación*: Desacoplamiento estricto a nivel de biblioteca; `CMakePresetsService` permanece en `vigilio_core` sin incluir cabeceras de `QWidget`.
  - *Riesgo*: Exposición de nombres de ingeniería interna en menús visibles.
    - *Mitigación*: Aplicación taxativa de `docs/UX-COPY-PUBLICO.md`; CMake Presets se declara formalmente como «sin cambio visual directo» en la UI pública.
  - *Riesgo*: Fallo de pruebas de UI en entornos de integración continua headless.
    - *Mitigación*: Detección de servidor gráfico en tiempo de ejecución en `WindowTests.cpp`, emitiendo `NOT_EXECUTED` de manera transparente y honesta.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 13/13 tests de CTest pasando exitosamente en 0.45s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.05s)
    9. `startup_bootstrap_tests`: Passed (0.03s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.02s)
    12. `cmake_presets_tests`: Passed (0.03s)
    13. `cmake_presets_service_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 35 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0103-module.foundations.task0035.propose.lock.json --project-root .`: 13 de 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0035` en ejecución 0104)**:
  - Se definieron formalmente las 8 tareas atómicas y verificables en `tasks.md` con criterios observables y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
