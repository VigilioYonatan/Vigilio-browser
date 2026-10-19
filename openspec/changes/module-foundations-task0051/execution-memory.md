# Execution Memory — module-foundations-task0051

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **ui** de la capacidad **CI Linux job** en el módulo `foundations` (`module.foundations.task0051`, microtarea canónica `0051 — Implementar la interacción y estados visuales en Qt: CI Linux job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0051-ci-linux-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0151`.
  - Pack: `module.foundations.task0051`.
  - Phase: `propose`.
  - Generated artifact: `0151-module.foundations.task0051.propose.md`.
  - Prompt SHA-256: `dbed4f63fac4d4db20c3ac726dc983581fe431018f39bc3fe1934cd6ca40dfc5`.
  - Lock SHA-256: `e83512421ed42f5ffa084a850d6208851cc76f601772e1736c44351a5ce3cda9`.
  - Dependencias ejecutables: `module.foundations.task0050`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Separación estricta de responsabilidades (SOLID, KISS, YAGNI); desacoplamiento total entre la infraestructura de automatización de compilación/CI en `src/core/` y la interfaz gráfica de usuario en `src/ui/`.
- `docs/skills/02-testing-senior.md`: Pruebas unitarias y de integración deterministas en `WindowTests.cpp`, aserciones directas, fixtures controlados y reporte honesto `NOT_EXECUTED` en entornos sin servidor gráfico.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`CiLinuxExecutionMode`, `CiLinuxJobStatus`), jerarquía Qt Widgets limpia y ausencia de llamadas bloqueantes en el hilo principal de la GUI.
- `docs/skills/04-ux-ui-senior.md`: Interfaz sobria, moderna y profesional; preservación de los 8 estados visuales (`idle`, `hover`, `focus`, `disabled`, `loading`, `empty`, `error`, `success`); iconografía Lucide SVG en QRC sin emojis ni brillos artificiales.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices vinculantes de «Interfaz más fácil de usar» (Modo Senior): tap targets mínimos $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos (22×22 a 32×32 px), tipografía aumentada (+4 pt), atajos universales y metadatos accesibles.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por archivo, límites de memoria (RAM $\le 4\text{ GB}$) y tiempos de ejecución en CTest < 200 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas o servicios cloud de CI, contención en rutas y rechazo categórico de Directory Traversal (`..`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Política de empaquetado, dependencias y consistencia con assets.
- `docs/skills/11-assets-svg-quality.md`: Calidad de assets SVG, Lucide y branding oficial.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («CI Linux job», «Pipeline», «Build Step», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec») en pantallas visibles al usuario común. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de CI Linux, orquestación de compilación y evaluación opera de manera estrictamente local en el equipo, sin dependencias de servicios externos ni telemetría remota.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Linux job` es infraestructura de automatización de compilación y calidad; por consiguiente, no agrega widgets, botones de ejecución de pipelines, paneles de compilación ni consolas en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece libre de dependencias hacia `QWidget` y `QApplication`, mientras que `BrowserWindow` permanece enfocado en la experiencia web de usuario final sin asumir tareas de compilación o ejecución de pipelines.
  4. *Tolerancia headless y política Zero-Zombie*: El arnés de prueba `WindowTests.cpp` detecta la presencia de display (`DISPLAY` / `WAYLAND_DISPLAY`) y reporta limpiamente `NOT_EXECUTED` (código 0) si no hay servidor gráfico, evitando caídas o procesos colgados.
  5. *Inmutabilidad en arnés de UI (modo DryRun)*: Las pruebas de interfaz gráfica que consumen `CiLinuxJobService::executePlan` utilizan exclusivamente el modo `DryRun` en memoria, garantizando que no se alteran archivos ni se ejecutan comandos destructivos durante la ejecución de las pruebas.
  6. *Preservación de «Interfaz más fácil de usar» (Modo Senior)*: Se mantienen las dimensiones accesibles ($\ge 48$ px de tap target, $\ge 55$ px toolbar, escalado de iconos a 32×32 px, atajos de teclado y metadatos accesibles).
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`: Taxonomía y estructuras operativas del pipeline de Linux.
  - `apps/vigilio-browser/src/core/CiLinuxJobValidator.h/.cpp`: Validador estático, orden topológico y comprobador de rutas seguras.
  - `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h/.cpp`: Planes canónicos y filtros de distribución.
  - `apps/vigilio-browser/src/core/CiLinuxJobService.h/.cpp`: Capa operativa de servicio, simulación dry-run y textos públicos en español.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h/.cpp`: Ventana principal de navegación de Vigilio Browser.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario en Qt Test / CTest.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y formato.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Falsos fallos en entornos de integración continua sin servidor X11/Wayland.
    *Mitigación*: Guarda preliminar determinista en `WindowTests.cpp` que detecta la ausencia de variables de display y finaliza con código 0 emitiendo el mensaje `NOT_EXECUTED`.
  - *Riesgo 2*: Polución visual en el navegador o exposición de jerga de ingeniería.
    *Mitigación*: Regla estricta de `docs/UX-COPY-PUBLICO.md` y prueba de verificación estática (`TEST-UI-CILINUX-001`) que valida la ausencia de widgets o menús de CI en `BrowserWindow`.
  - *Riesgo 3*: Cuelgues o procesos huérfanos durante la simulación de pipeline en UI.
    *Mitigación*: Operación acotada en memoria (< 200 ms) y verificación de `CancellationToken` bajo la política Zero-Zombie.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 21/21 pruebas pasando exitosamente en CTest en ~4.29s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.14s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.13s)
    6. `resource_qrc_catalog_tests`: Passed (0.09s)
    7. `resource_inclusion_service_tests`: Passed (0.19s)
    8. `resource_qrc_inclusion_tests`: Passed (0.11s)
    9. `startup_bootstrap_tests`: Passed (0.19s)
    10. `application_lifetime_tests`: Passed (0.22s)
    11. `crash_recovery_tests`: Passed (0.29s)
    12. `cmake_presets_tests`: Passed (0.14s)
    13. `cmake_presets_service_tests`: Passed (0.19s)
    14. `test_fixture_foundation_tests`: Passed (0.02s)
    15. `test_fixture_service_tests`: Passed (0.14s)
    16. `clang_format_configuration_tests`: Passed (0.00s)
    17. `clang_format_service_tests`: Passed (2.21s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.13s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.04s)
- **Verificación con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` $\to$ `references: 51, valid: true`.
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0151-module.foundations.task0051.propose.lock.json --project-root .` $\to$ `valid: true, checks: 14/14 passed`.
- **Handoff hacia APPLY**:
  - La propuesta formal en `openspec/changes/module-foundations-task0051/` está completamente redactada, substantiva, trazable y lista para la revisión y aprobación humana antes de pasar a la fase de implementación (APPLY).
