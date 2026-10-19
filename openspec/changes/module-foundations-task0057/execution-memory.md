# Execution Memory — module-foundations-task0057

Status: PROPOSED

## Objective and authority

- **Identificador de pack**: `module.foundations.task0057`
- **Capacidad**: CI macOS job
- **Etapa**: domain (`0057 — Definir el comportamiento y las invariantes: CI macOS job`)
- **Módulo propietario**: `foundations`
- **Fase del runner**: `propose`
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0057-ci-macos-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Objetivo concreto**: Definir formalmente el modelo de dominio, tipos inmutables, enums fuertemente tipados, estructuras de pipeline, reglas de precedencia (DAG), validación de toolchain (Apple Clang, Xcode Command Line Tools, macOS SDK, arquitecturas ARM64 y x86_64), políticas de contención y seguridad de rutas con defensas específicas de macOS (bloqueo de traversal, rutas absolutas del host `/System`, `/Library`, y artefactos como `.DS_Store` o `._*`), catálogo de versiones de macOS soportadas y mapeo estricto de textos públicos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, desacoplamiento absoluto de capas, código limpio en C++20 y ausencia de atajos o hacks.
- `docs/skills/02-testing-senior.md`: Pruebas unitarias independientes, reproducibles, deterministas, ejecutables en modo headless con CTest y con aserciones rigurosas de casos válidos e inválidos.
- `docs/skills/03-codigo-cpp-qt.md`: Semántica de valor inmutable, `[[nodiscard]]`, métodos `const`, ausencia de macros innecesarias y núcleo desacoplado de dependencias pesadas de GUI.
- `docs/skills/04-ux-ui-senior.md`: Contrato de «sin cambio visual directo en la interfaz pública» para subsistemas de infraestructura de integración continua y calidad de compilación; preservación de la estabilidad visual y de accesibilidad.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en los componentes de interfaz, áreas de interacción $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos y compatibilidad con lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por trabajo (`maxMemoryBytes`, 4 GB de presupuesto máximo), límites de tiempo (`totalTimeoutSeconds`, 600 s en Debug y 900 s en Release) y tiempos de ejecución en CTest < 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`, `../`), rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`) y artefactos del sistema de archivos macOS (`.DS_Store`, `._*`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Consideraciones sobre empaquetado de aplicaciones nativas en macOS (bundle `.app`, estructura de directorios `Contents/MacOS`, `Contents/Resources`, `Info.plist`, `BundleVerification`) y trazabilidad de binarios.
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e identidad.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de planificación, validación de etapas y ejecución de CI macOS opera de manera estrictamente local en el equipo o runner propio, sin dependencias cloud ni telemetría.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI macOS job` en la etapa `domain` es infraestructura de automatización de pruebas y compilación en macOS; por consiguiente, no agrega widgets, botones ni menús en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`, ejecutándose limpiamente en CTest sin necesidad de servidor gráfico ni entorno macOS nativo durante la prueba de dominio.
  4. *Tolerancia headless y política Zero-Zombie*: El arnés de pruebas opera con timeouts finitos explícitos, sin procesos zombis, sin colgarse indefinidamente y liberando recursos inmediatamente al completarse o cancelarse.
  5. *Precedencia estricta de etapas (DAG)*: No se permite compilar sin previa configuración, ni ejecutar pruebas sin compilación previa exitosa. Las comprobaciones de formato y análisis estático preceden a la compilación. La verificación de bundle `.app` sucede a las pruebas.
  6. *Seguridad de rutas y confinamiento macOS*: Toda ruta de directorio de construcción debe ser relativa, normalizada frente a separadores `/`, rechazar componentes de escape (`..`), rutas absolutas del host (`/System`, `/Library`, `/Applications`, etc.) y artefactos conflictivos de macOS (`.DS_Store`, archivos AppleDouble `._*`).
  7. *Presupuesto de recursos*: Límite estricto de memoria de 4 GB (`maxMemoryBytes`) por trabajo y límite de 15 segundos por prueba unitaria en CTest.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiLinuxJobTypes.h` y `CiWindowsJobTypes.h`: Patrón simétrico de tipos, estados y reportes de CI.
  - `apps/vigilio-browser/src/core/CiLinuxJobValidator.h/.cpp` y `CiWindowsJobValidator.h/.cpp`: Patrón de validador estático desacoplado.
  - `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h/.cpp` y `CiWindowsJobConfiguration.h/.cpp`: Patrón de entidad de configuración canónica.
  - `apps/vigilio-browser/CMakePresets.json`: Definición de presets del proyecto.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua con 23 pruebas pasando.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Confusión entre versiones de marketing de macOS (macOS 13, 14, 15) y versiones del kernel Darwin (Darwin 22, 23, 24).
    *Mitigación*: `isSupportedMacOsVersion` evalúa ambas nomenclaturas para máxima compatibilidad y robustez en runners heterogéneos.
  - *Riesgo 2*: Contaminación y falsos positivos causados por artefactos ocultos de macOS (`.DS_Store`, archivos `._*`).
    *Mitigación*: `isSafeBuildPath` detecta léxicamente y bloquea terminantemente la presencia de estos nombres.
  - *Riesgo 3*: Filtrado de jerga interna en textos de diagnóstico visibles.
    *Mitigación*: Prueba unitaria dedicada que audita programáticamente cada cadena de texto devuelta contra una lista de términos prohibidos de `docs/UX-COPY-PUBLICO.md`.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 23/23 pruebas pasando exitosamente en CTest en ~5.07s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.20s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.20s)
    6. `resource_qrc_catalog_tests`: Passed (0.08s)
    7. `resource_inclusion_service_tests`: Passed (0.10s)
    8. `resource_qrc_inclusion_tests`: Passed (0.09s)
    9. `startup_bootstrap_tests`: Passed (0.11s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.05s)
    12. `cmake_presets_tests`: Passed (0.06s)
    13. `cmake_presets_service_tests`: Passed (0.13s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.12s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (2.58s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.20s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.15s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.77s)
- **Gobernanza con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 57 referencias canónicas validadas (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0169-module.foundations.task0057.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
- **Handoff para la fase APPLY**:
  - La propuesta OpenSpec queda completamente formalizada en `openspec/changes/module-foundations-task0057/` con `proposal.md`, `design.md`, `tasks.md` y `execution-memory.md`.
  - El siguiente paso es la aprobación humana de la propuesta antes de proceder a la ejecución de la fase APPLY (`module.foundations.task0057.apply`), cuyo commit sugerido es:
    `docs(foundations): domain ci-macos-job`
