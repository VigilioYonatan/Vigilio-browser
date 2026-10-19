# Execution Memory — module-foundations-task0058

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `implementation` de la capacidad **CI macOS job** (`module.foundations.task0058`), diseñando la capa operativa de servicio desacoplada (`vigilio::CiMacOsJobService`), la orquestación secuencial de las ocho etapas canónicas del pipeline de macOS (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `BundleVerification`), la ejecución determinista en modo simulación/dry-run y en vivo, la cancelación cooperativa multihilo bajo la política Zero-Zombie, el mecanismo de corte temprano ante fallos (fail-fast), la tolerancia selectiva a errores (`continueOnError`), el saneamiento seguro del directorio de compilación previniendo Directory Traversal (`..`, `../`), rutas host de macOS (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`) y artefactos (`.DS_Store`, `._*`), la síntesis reproducible de comandos CLI para macOS (incluyendo la verificación del bundle `.app` nativo), y el control estricto de presupuestos de tiempo y memoria RAM en C++20.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo foundations.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/foundations/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/foundations/micro/0058-ci-macos-job/task.md`: Contrato vinculante de la microtarea canónica 0058.
  - `openspec/changes/module-foundations-task0058/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes GUI.
- `docs/skills/02-testing-senior.md`: Pruebas unitarias deterministas, rápidas y reproducibles en CTest; aserciones precisas sin mocks de integración engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados, inmutabilidad, RAII, ausencia de memoria dinámica descontrolada y exclusión de dependencias de Qt GUI en el núcleo de servicios.
- `docs/skills/04-ux-ui-senior.md`: Contrato de «sin cambio visual directo en la interfaz pública» para subsistemas de infraestructura de integración continua y calidad de compilación; preservación de la estabilidad visual y de accesibilidad.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en los componentes de interfaz, áreas de interacción $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos y compatibilidad con lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por trabajo (`maxMemoryBytes`, 4 GB de presupuesto máximo), límites de tiempo (`totalTimeoutSeconds`, 600 s en Debug y 900 s en Release) y tiempos de ejecución en CTest < 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`, `../`), rutas del sistema host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`) y artefactos macOS (`.DS_Store`, `._*`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Consideraciones sobre empaquetado del instalador y bundle de aplicación nativa en macOS (`VigilioBrowser.app/Contents/MacOS` y `Info.plist`) y trazabilidad de binarios.
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e identidad.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de orquestación, validación y ejecución opera de manera estrictamente local en el equipo o runner de CI, sin dependencias cloud ni telemetría remota.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI macOS job` en la etapa `implementation` es infraestructura de orquestación de compilación y pruebas en macOS; por consiguiente, no agrega widgets, botones ni menús en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`, ejecutándose limpiamente en CTest sin necesidad de servidor gráfico ni entorno macOS nativo durante las pruebas unitarias.
  4. *Tolerancia headless y política Zero-Zombie*: La ejecución opera con timeouts finitos explícitos, sin procesos zombis colgados indefinidamente, liberando recursos inmediatamente ante cancelación cooperativa (`CancellationToken`).
  5. *Precedencia estricta de etapas (DAG)*: No se permite compilar sin previa configuración, ni ejecutar pruebas sin compilación previa exitosa. Las comprobaciones estáticas preceden a la compilación.
  6. *Seguridad de rutas y confinamiento macOS*: Toda ruta de directorio de construcción debe ser relativa, rechazar componentes de escape (`..`, `../`), rutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`) y artefactos específicos de macOS (`.DS_Store`, `._*`).
  7. *Presupuesto de recursos*: Límite estricto de memoria de 4 GB (`maxMemoryBytes`) por trabajo y límite de 15 segundos por prueba unitaria en CTest.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiLinuxJobService.h/.cpp` y `CiWindowsJobService.h/.cpp`: Patrón simétrico de servicio desacoplado, orquestación, fail-fast, cancelación Zero-Zombie y saneamiento de directorios.
  - `apps/vigilio-browser/src/core/CiMacOsJobTypes.h`: Catálogo completo de enumeraciones, estructuras inmutables y opciones operativas de CI macOS (`CiMacOsServiceOptions`, `CiMacOsExecutionMode`).
  - `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h/.cpp`: Provisión de planes canónicos oficiales (`canonicalDebugPlan`, `canonicalReleasePlan`).
  - `apps/vigilio-browser/src/core/CiMacOsJobValidator.h/.cpp`: Validador estático desacoplado de planes, orden topológico, seguridad APFS y UX copy.
  - `apps/vigilio-browser/CMakePresets.json`: Definición oficial de presets del proyecto.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Contención de bloqueos transitorios de archivo en sistemas de archivos APFS en ejecuciones concurrentes.
    *Mitigación*: Implementación de retroceso exponencial ($50\text{ ms} \times 2^i$) con hasta 3 reintentos antes de reportar error en `cleanBuildDirectory`.
  - *Riesgo 2*: Subprocesos colgados o zombis esperando indefinidamente durante la cancelación.
    *Mitigación*: Evaluación atómica de `CancellationToken::isCancelled()` antes de despachar cada etapa y en rutinas de espera.
  - *Riesgo 3*: Filtrado de jerga interna en textos de diagnóstico devueltos al usuario.
    *Mitigación*: Prueba unitaria automatizada (`TEST-CI-MAC-SRV-012`) que audita programáticamente cada cadena producida contra el catálogo de `docs/UX-COPY-PUBLICO.md`.

---

## Checklist evidence and handoff

- **Pipeline continuo en CMake / CTest**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 24/24 pruebas pasando exitosamente en CTest en ~6.20s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.24s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.23s)
    6. `resource_qrc_catalog_tests`: Passed (0.11s)
    7. `resource_inclusion_service_tests`: Passed (0.19s)
    8. `resource_qrc_inclusion_tests`: Passed (0.21s)
    9. `startup_bootstrap_tests`: Passed (0.27s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.05s)
    12. `cmake_presets_tests`: Passed (0.05s)
    13. `cmake_presets_service_tests`: Passed (0.12s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.14s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (3.15s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.20s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.18s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.79s)
    24. `ci_macos_job_configuration_tests`: Passed (0.01s)
- **Gobernanza con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 58 referencias canónicas validadas (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0172-module.foundations.task0058.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
- **Handoff para la fase APPLY**:
  - La propuesta OpenSpec queda completamente formalizada en `openspec/changes/module-foundations-task0058/` con `proposal.md`, `design.md`, `tasks.md` y `execution-memory.md`.
  - El siguiente paso es la revisión y aprobación humana de la propuesta antes de proceder a la ejecución de la fase APPLY (`module.foundations.task0058.apply`), cuyo commit sugerido es:
    `feat(foundations): implementation ci-macos-job`.
