# Execution Memory — module-ux-shell-task0060

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `domain` de la capacidad **main window layout** (`module.ux-shell.task0060`), modelando de manera formal y desacoplada la taxonomía de zonas de ventana, configuraciones canónicas de densidad visual (estándar, compacta y «Interfaz más fácil de usar» / Modo Senior), invariantes dimensionales, presupuestos de área central de contenido, validador estático geométrico y suite de pruebas unitarias de especificación en CTest, manteniendo total independencia de widgets Qt y estricta conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md`: Contrato vinculante de la microtarea canónica 0060.
  - `openspec/changes/module-ux-shell-task0060/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes de interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pruebas de dominio deterministas, rápidas y reproducibles en CTest; aserciones precisas sin mocks engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados (`enum class : uint8_t`), semántica de valor inmutable, const-correctness y exclusión de dependencias de GUI en el núcleo de dominio.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración con barra lateral, omnibox y tarjetas.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), áreas de interacción táctiles y de ratón $\ge 48$ px, barra de navegación $\ge 56$ px, iconos escalados a $32\times 32$ px, navegación por teclado y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, presupuestos de memoria reducidos, cálculos geométricos analíticos en memoria (< 1 ms) y pruebas globales < 10 segundos en CTest.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cálculos sin retención de datos de navegación.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la ventana principal con barra lateral, barra de herramientas y buscador.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de cálculo geométrico y validación opera de forma 100% local en memoria, sin dependencias cloud ni telemetría remota.
  2. *Desacoplamiento estricto del núcleo de dominio*: `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration` y `MainWindowLayoutValidator` permanecen en `vigilio_core` sin dependencias hacia `QWidget`, `QMainWindow` ni `QApplication`.
  3. *Invariante de disyunción geométrica*: El algoritmo `calculateLayoutAreas` garantiza formalmente que todas las zonas visibles son mutuamente disjuntas ($\forall i \ne j, Z_i \cap Z_j = \emptyset$).
  4. *Invariante de suficiencia de contenido web*: El área asignada al contenido central debe disponer de al menos $400\times 250$ px netos; cualquier intento de distribución que colapse este presupuesto se rechaza con `ZeroContentAreaBudget`.
  5. *Invariante de «Interfaz más fácil de usar»*: Tap targets y controles $\ge 48$ px, barra de navegación $\ge 56$ px, dimensiones mínimas de ventana $\ge 720\times 520$ px.
  6. *Invariante de barra lateral*: Estado colapsado fijo en 48 px; estado expandido en el intervalo $[200, 360]$ px (valor por defecto 240 px).
  7. *Auditoría de lenguaje público*: Toda cadena o sugerencia de remediación emitida por `MainWindowLayoutValidator` debe formularse en español neutro sin términos internos proscritos.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/ui/BrowserWindow.h/.cpp`: Código actual de la ventana principal, analizado para definir los deltas arquitectónicos de zonificación y desacoplamiento.
  - `apps/vigilio-browser/src/core/StartupTypes.h`, `LifetimeTypes.h`, `CrashRecoveryTypes.h`: Patrones canónicos de tipos de dominio y reportes inmutables.
  - `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`: Referencias visuales de partida para dimensionamiento y proporciones de ventana.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Regresiones por acoplamiento de tipos GUI en el núcleo de dominio.
    - *Mitigación*: Implementación en C++20 puro usando tipos primitivos estándar y estructuras de valor cartesianas (`LayoutRect`), aplazando el enlace con Qt a la etapa `ui` (`task0062`).
  - *Riesgo 2*: Desbordamientos visuales en monitores de baja resolución al activar Modo Senior.
    - *Mitigación*: Validación preventiva mediante `validateDimensions` que rechaza ventanas de dimensiones insuficientes ($< 720\times 520$ px) antes de la renderización.
  - *Riesgo 3*: Conflicto de acople o solapamiento entre banners condicionales y barra lateral.
    - *Mitigación*: Orden topológico estricto que posiciona los banners sobre la sección central, reduciendo exclusivamente la altura disponible de la sección central sin perturbar el ancho lateral.

---

## Checklist evidence and handoff

- **Pipeline continuo en CMake / CTest**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 25/25 pruebas pasando exitosamente en CTest en ~5.36s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.14s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.22s)
    6. `resource_qrc_catalog_tests`: Passed (0.04s)
    7. `resource_inclusion_service_tests`: Passed (0.19s)
    8. `resource_qrc_inclusion_tests`: Passed (0.27s)
    9. `startup_bootstrap_tests`: Passed (0.12s)
    10. `application_lifetime_tests`: Passed (0.15s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.02s)
    15. `test_fixture_service_tests`: Passed (0.12s)
    16. `clang_format_configuration_tests`: Passed (0.00s)
    17. `clang_format_service_tests`: Passed (2.02s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.13s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.15s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.77s)
    24. `ci_macos_job_configuration_tests`: Passed (0.01s)
    25. `ci_macos_job_service_tests`: Passed (0.76s)
- **Gobernanza con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 60 referencias canónicas validadas (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0178-module.ux-shell.task0060.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate --pack module.ux-shell.task0060 --phase propose --project-root .`: Validación de pack OK (exit code 0).
