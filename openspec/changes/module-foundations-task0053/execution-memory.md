# Execution Memory — module-foundations-task0053

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **CI Windows job** en el módulo `foundations` (`module.foundations.task0053`, microtarea canónica `0053 — Definir el comportamiento y las invariantes: CI Windows job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0053-ci-windows-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0157`.
  - Pack: `module.foundations.task0053`.
  - Phase: `propose`.
  - Generated artifact: `0157-module.foundations.task0053.propose.md`.
  - Prompt SHA-256: `3e67de2352e66d8f340606971e98663414dff9c7e35ae24425973bc3dc6a3363`.
  - Lock SHA-256: `b758e547c9381eb8a7928a58348d8aaf5c4f9354d74302facae8bdfb70a136fd`.
  - Dependencias ejecutables: `module.foundations.task0052`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Separación estricta de responsabilidades (SOLID, KISS, YAGNI); desacoplamiento total entre la lógica del núcleo (`src/core/`), el almacenamiento y la interfaz gráfica; contratos inmutables y pruebas deterministas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para contratos de configuración y validadores en memoria, integración para sistema de archivos y compilación, CTest headless), aserciones deterministas sin temporizadores frágiles (`sleep`), fixtures locales y reporte transparente.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar (`std::filesystem`, `std::thread`, `std::atomic`), const-correctness, tipado fuerte (`CiWindowsJobStatus`, `CiWindowsJobError`, `CiWindowsPipelineStep`), encapsulamiento de datos inmutables y ausencia absoluta de dependencias GUI en el núcleo de negocio.
- `docs/skills/04-ux-ui-senior.md`: Contrato de «sin cambio visual directo en la interfaz pública» para subsistemas de infraestructura de integración continua y calidad de compilación; preservación de la estabilidad visual y de accesibilidad.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en los componentes de interfaz, áreas de interacción $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos y compatibilidad con lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por trabajo (`maxMemoryBytes`, 4 GB de presupuesto máximo), límites de tiempo (`totalTimeoutSeconds`, 600 s en Debug y 900 s en Release) y tiempos de ejecución en CTest < 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`, `..\`), letras de unidad (`C:\`), rutas UNC y dispositivos DOS reservados (`CON`, `PRN`, `AUX`, `NUL`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Consideraciones sobre empaquetado del instalador oficial en Windows (`ArtifactPackaging`) y trazabilidad de binarios.
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e identidad.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de planificación, validación de etapas y ejecución de CI Windows opera de manera estrictamente local en el equipo o runner propio, sin dependencias cloud ni telemetría.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Windows job` en la etapa `domain` es infraestructura de automatización de pruebas y compilación en Windows; por consiguiente, no agrega widgets, botones ni menús en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`, ejecutándose limpiamente en CTest sin necesidad de servidor gráfico ni entorno Windows nativo durante la prueba de dominio.
  4. *Tolerancia headless y política Zero-Zombie*: El arnés de pruebas opera con timeouts finitos explícitos, sin procesos zombis, sin colgarse indefinidamente y liberando recursos inmediatamente al completarse o cancelarse.
  5. *Precedencia estricta de etapas (DAG)*: No se permite compilar sin previa configuración, ni ejecutar pruebas sin compilación previa exitosa. Las comprobaciones de formato y análisis estático preceden a la compilación.
  6. *Seguridad de rutas y confinamiento Windows*: Toda ruta de directorio de construcción debe ser relativa, normalizada frente a separadores duales (`/` y `\`), rechazar componentes de escape (`..`), letras de volumen (`C:`), prefijos de dispositivo y nombres reservados de DOS (`CON`, `PRN`, `AUX`, `NUL`).
  7. *Presupuesto de recursos*: Límite estricto de memoria de 4 GB (`maxMemoryBytes`) por trabajo y límite de 15 segundos por prueba unitaria en CTest.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`: Patrón simétrico de tipos, estados y reportes de CI.
  - `apps/vigilio-browser/src/core/CiLinuxJobValidator.h/.cpp`: Patrón de validador estático desacoplado.
  - `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h/.cpp`: Patrón de entidad de configuración canónica.
  - `apps/vigilio-browser/src/core/SessionFile.cpp` y `CrashRecoveryCoordinator.cpp`: Referencias de código portable con `#include <windows.h>`.
  - `apps/vigilio-browser/CMakePresets.json`: Definición de presets del proyecto.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua con 21 pruebas pasando.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Manipulación incorrecta de separadores de ruta mixtos (`\` vs `/`) en Windows.
    *Mitigación*: `isSafeBuildPath` analiza los componentes de la ruta de manera independiente del separador, reconociendo tanto `/` como `\`.
  - *Riesgo 2*: Bloqueos del sistema operativo Windows ante nombres de archivos especiales (`CON`, `NUL`, `AUX`, `PRN`).
    *Mitigación*: Validación léxica previa que rechaza terminantemente estos nombres antes de invocar operaciones de `std::filesystem`.
  - *Riesgo 3*: Filtrado de jerga interna en textos de diagnóstico visibles.
    *Mitigación*: Prueba unitaria dedicada que audita programáticamente cada cadena de texto devuelta contra una lista de términos prohibidos.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 21/21 pruebas pasando exitosamente en CTest en ~3.78s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.02s)
    3. `vigilio_warnings_tests`: Passed (0.34s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.14s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.08s)
    8. `resource_qrc_inclusion_tests`: Passed (0.10s)
    9. `startup_bootstrap_tests`: Passed (0.20s)
    10. `application_lifetime_tests`: Passed (0.20s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.12s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (2.04s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.13s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.15s)
- **Gobernanza con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 53 referencias canónicas validadas (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0157-module.foundations.task0053.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
- **Handoff para la fase APPLY**:
  - La propuesta OpenSpec queda completamente formalizada en `openspec/changes/module-foundations-task0053/` con `proposal.md`, `design.md`, `tasks.md` y `execution-memory.md`.
  - El siguiente paso es la aprobación humana de la propuesta antes de proceder a la ejecución de la fase APPLY (`module.foundations.task0053.apply`), cuyo commit sugerido es:
    `docs(foundations): domain ci-windows-job`
