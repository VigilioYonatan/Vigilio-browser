# Execution Memory — module-foundations-task0034

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0034 — Implementar la operación en una capa propietaria: CMake presets`), proyectando la capa de servicio operativo en C++20 (`vigilio::CMakePresetsService`), síntesis determinista de líneas de comando CLI (`cmake --preset`, `cmake --build --preset`, `ctest --preset`), ejecución acotada con soporte para modo simulación (`dryRun`), cancelación cooperativa multihilo vía `CancellationToken`, saneamiento seguro de directorios de compilación (`cleanBinaryDir`) y suite de pruebas unitarias, sin acoplar lógica de negocio a los widgets de interfaz ni añadir responsabilidades a `BrowserWindow`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura, independencia del núcleo C++20 respecto a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, contratos de fallos y recuperación local.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas locales y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0034-cmake-presets/task.md`: Contrato vinculante de la microtarea 0034.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 código abierto, Q-004 soporte multiplataforma Win/Linux/macOS, Q-005 actualizaciones manuales, Q-006 assets en `apps/vigilio-browser/assets`, Q-007 blocks only `release.public-release`, Q-008 firma builds).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución actual: `0100`
  - Pack: `module.foundations.task0034`
  - Phase: `propose`
  - Generated artifact: `0100-module.foundations.task0034.propose.md`
  - Prompt SHA-256: `788f68cad0cf4c0e362523a92c0d5eaa0dc61fc9cbc97cfe402af425cb878c3e`
  - Lock SHA-256: `8948045a695a5116f2a1bc6463effc242944cadf48017f78c16e178b8906dd4b`
  - Dependencias ejecutables: `module.foundations.task0033`
  - Paso de fase anterior: `none`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; encapsulamiento de la lógica de presets en la capa operativa `CMakePresetsService` en `src/core`, manteniendo una interfaz limpia y desacoplada de la interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias deterministas y de alta velocidad en `vigilio_core`, sin dependencias de red ni procesos pesados de Chromium; fixtures en memoria y en disco temporal.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, no bloqueo del hilo de interfaz, separación estricta entre `src/core` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Principios visuales y de accesibilidad para la futura integración UI (etapa 0035); navegación por teclado, foco visible y Modo Senior («Interfaz más fácil de usar»).
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en arranque; operaciones de parseo y síntesis < 2 ms; límites de memoria de salida a 64 KB y consumo de RAM en reposo < 2 MB.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero envío de telemetría a la red o servicios cloud; validación rigurosa de rutas y prevención de Path Traversal mediante `isSafeBinaryDir`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («CMake», «Presets», «Toolchain», «Prompt Builder», «OpenSpec», «Senior Mode») en interfaces públicas o mensajes visibles al usuario.
- Referencias visuales en `/resultado/`:
  - `resultado/01-vigilio-home.png`: Ventana principal del navegador.
  - `resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
  - `resultado/06-logo-instalador-oficial.png`: Logotipo oficial.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la gestión y ejecución de presets se realiza exclusivamente con herramientas del sistema local. Cero llamadas de red, telemetría o servicios remotos.
  2. *Higiene de Rutas y Sandboxing*: Toda operación de saneamiento (`cleanBinaryDir`) exige validación previa con `CMakePresetsValidator::isSafeBinaryDir`, denegando Directory Traversal (`..`) y rutas fuera del build tree (`${sourceDir}/build-*`).
  3. *Zero-Zombie y Bounded Buffers*: Toda ejecución impone timeout finito (`timeoutMs`) y truncado estricto de captura de salidas estándar y de error a 64 KB (`maxOutputBytes = 65536`).
  4. *Cancelación Cooperativa Thread-Safe*: `CancellationToken` provee un mecanismo seguro para abortar tareas en segundo plano sin generar procesos zombies ni fugas de descriptores de archivo.
  5. *Desacoplamiento Estricto*: `CMakePresetsService` reside íntegramente en `vigilio_core`, sin referencias a `QWidget`, `QApplication` ni `QWebEngineView`.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CMakePresetsTypes.h`: Tipos y estructuras base del dominio de presets.
  - `apps/vigilio-browser/src/core/CMakePresetsValidator.h` & `.cpp`: Validador estático, parser JSON y reglas de higiene de rutas (`isSafeBinaryDir`).
  - `apps/vigilio-browser/CMakePresets.json`: Catálogo canónico con los 9 perfiles estándar versión 6.
  - `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`: Batería de pruebas unitarias de dominio.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Procesos de compilación colgados o consumiendo CPU indefinidamente.
    *Mitigación*: Implementación de la política Zero-Zombie mediante timeout configurable y comprobación periódica de `CancellationToken::isCancelled`.
  - *Riesgo*: Borrado accidental de directorios de usuario durante la limpieza de artefactos.
    *Mitigación*: Validación mandatoria de `CMakePresetsValidator::isSafeBinaryDir` antes de realizar cualquier llamada a `std::filesystem::remove_all`.
  - *Riesgo*: Agotamiento de memoria por salidas de compilador masivas.
    *Mitigación*: Truncado de búfer a 64 KB con mensaje explícito de preservación de memoria.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 12/12 tests de CTest pasando exitosamente en 0.47s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.05s)
    9. `startup_bootstrap_tests`: Passed (0.05s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.02s)
    12. `cmake_presets_tests`: Passed (0.03s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 34 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0100-module.foundations.task0034.propose.lock.json --project-root .`: 13 de 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0034` en ejecución 0101)**:
  - Se definieron formalmente las 10 tareas atómicas y verificables en `tasks.md` con criterios observables y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
