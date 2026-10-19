# Execution Memory — module-foundations-task0049

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **CI Linux job** en el módulo `foundations` (`module.foundations.task0049`, microtarea canónica `0049 — Definir el comportamiento y las invariantes: CI Linux job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0049-ci-linux-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0145`.
  - Pack: `module.foundations.task0049`.
  - Phase: `propose`.
  - Generated artifact: `0145-module.foundations.task0049.propose.md`.
  - Prompt SHA-256: `a3f9e068f54f1eec5e65fcdc39a82a698c1a4001a6809078e17be05caaa78069`.
  - Lock SHA-256: `88a6c550acd5c44da639473121c9d488e89058046e281cb69785e41371e9b8a7`.
  - Dependencias ejecutables: `module.foundations.task0048`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Separación estricta de responsabilidades (SOLID, KISS, YAGNI); desacoplamiento total entre la lógica del núcleo (`src/core/`), el almacenamiento y la interfaz gráfica; pruebas deterministas y estructuradas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para contratos de configuración y servicios en memoria, integración para sistema de archivos y respaldos, CTest headless), aserciones deterministas, eliminación de retardos fijos (`sleep`), fixtures con datos locales y reporte transparente.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar (`std::filesystem`, `std::thread`, `std::atomic`), const-correctness, tipado fuerte (`CiLinuxJobStatus`, `CiLinuxJobError`, `CiLinuxPipelineStep`), encapsulamiento de datos inmutables y ausencia de dependencias hacia GUI en el núcleo de negocio.
- `docs/skills/04-ux-ui-senior.md`: Contrato de «sin cambio visual directo en la interfaz pública» para subsistemas de infraestructura de integración continua y calidad de compilación; preservación de los estados visuales en capas de diagnóstico.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en los componentes de interfaz, tap targets $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos (22×22 a 32×32 px) y compatibilidad con lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por trabajo (`maxMemoryBytes`, 4 GB de presupuesto máximo), límites de tiempo (`totalTimeoutSeconds`, 600 s) y tiempos de ejecución en CTest < 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de planificación, validación de etapas y ejecución de CI Linux opera de manera estrictamente local en el equipo o runner propio, sin dependencias cloud ni telemetría.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Linux job` en la etapa `domain` es infraestructura de automatización de pruebas y compilación en Linux; por consiguiente, no agrega widgets, botones ni menús en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`, ejecutándose limpiamente en CTest sin necesidad de servidor gráfico (`DISPLAY`).
  4. *Tolerancia headless y política Zero-Zombie*: El arnés de pruebas y los trabajos de CI operan con timeouts finitos explícitos, sin procesos zombis, sin colgarse indefinidamente y liberando recursos inmediatamente al completarse o cancelarse.
  5. *Precedencia estricta de etapas (DAG)*: No se permite compilar sin previa configuración, ni ejecutar pruebas sin compilación previa exitosa. Las comprobaciones de formato y análisis estático preceden a la compilación.
  6. *Seguridad de rutas y confinamiento*: Toda ruta de directorio de construcción debe ser relativa y validarse contra Directory Traversal (`..`) antes de cualquier acceso al sistema de archivos.
  7. *Presupuesto de recursos*: Límite estricto de memoria de 4 GB (`maxMemoryBytes`) por trabajo y límite de 15 segundos por prueba unitaria en CTest.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakePresets.json`: Definición oficial de los presets `ci-linux-debug` y `ci-linux-release`.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y formato.
  - `apps/vigilio-browser/src/core/CMakePresetsTypes.h`: Patrón de tipos y estructuras de presets de CMake.
  - `apps/vigilio-browser/src/core/CMakePresetsValidator.h/.cpp`: Patrón de validador desacoplado de configuración.
  - `apps/vigilio-browser/src/core/ClangTidyTypes.h`: Patrón de tipado fuerte y token de cancelación cooperativa.
  - `apps/vigilio-browser/tests/unit/`: Batería existente de 19 pruebas unitarias que pasan al 100% en CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Violaciones de orden de dependencias en pipelines personalizados.
    *Mitigación*: `CiLinuxJobValidator::validateStepSequence` realiza análisis topológico estricto rechazando combinaciones ilegales (`DependencyOrderingViolation`).
  - *Riesgo 2*: Escapes de directorio hacia rutas del sistema anfitrión.
    *Mitigación*: `isSafeBuildPath` exige rutas relativas que inicien por subdirectorios autorizados de compilación (`build-ci-*`, `_build`).
  - *Riesgo 3*: Filtrado de jerga interna en textos de diagnóstico visibles.
    *Mitigación*: Prueba unitaria dedicada que audita programáticamente cada cadena de texto devuelta contra una lista de términos prohibidos.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 19/19 pruebas pasando exitosamente en CTest en ~4.49s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.20s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.28s)
    6. `resource_qrc_catalog_tests`: Passed (0.17s)
    7. `resource_inclusion_service_tests`: Passed (0.22s)
    8. `resource_qrc_inclusion_tests`: Passed (0.11s)
    9. `startup_bootstrap_tests`: Passed (0.08s)
    10. `application_lifetime_tests`: Passed (0.16s)
    11. `crash_recovery_tests`: Passed (0.07s)
    12. `cmake_presets_tests`: Passed (0.06s)
    13. `cmake_presets_service_tests`: Passed (0.16s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.15s)
    16. `clang_format_configuration_tests`: Passed (0.02s)
    17. `clang_format_service_tests`: Passed (2.49s)
    18. `clang_tidy_configuration_tests`: Passed (0.02s)
    19. `clang_tidy_service_tests`: Passed (0.26s)
- **Gobernanza con Prompt Builder CLI 0.6.3**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 49 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0145-module.foundations.task0049.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
- **Handoff**:
  - El OpenSpec change `openspec/changes/module-foundations-task0049` contiene `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - La propuesta está completamente definida, autocontenida y lista para revisión humana y posterior aprobación antes de la fase APPLY.
