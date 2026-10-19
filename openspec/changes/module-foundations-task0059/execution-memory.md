# Execution Memory — module-foundations-task0059

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `ui` de la capacidad **CI macOS job** (`module.foundations.task0059`), diseñando la gobernanza de calidad en Qt Widgets, contratos visuales, tolerancia headless, integración en el arnés de pruebas de interfaz de usuario (`vigilio_ui_tests` / `WindowTests.cpp`), verificación de cobertura de compilación de UI y empaquetado del bundle nativo `.app` (`VigilioBrowser.app/Contents/MacOS` e `Info.plist`) en el pipeline de CI macOS, y auditoría exhaustiva de textos públicos y diagnósticos para CI macOS job de acuerdo con `docs/UX-COPY-PUBLICO.md`, sin acoplar lógica de CI a `BrowserWindow` y bajo estricta política Zero-Zombie.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo foundations.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/foundations/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/foundations/micro/0059-ci-macos-job/task.md`: Contrato vinculante de la microtarea canónica 0059.
  - `openspec/changes/module-foundations-task0059/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes GUI.
- `docs/skills/02-testing-senior.md`: Pruebas de UI deterministas, rápidas y reproducibles en Qt Test / CTest; aserciones precisas sin mocks engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados, RAII, inmutabilidad, metaobjetos Qt y exclusión de dependencias de GUI en el núcleo de servicios.
- `docs/skills/04-ux-ui-senior.md`: Principio de «sin cambio visual directo en la interfaz pública» para capacidades de infraestructura de CI; preservación de la sobriedad visual y de los 8 estados de interfaz (`idle`, `hover`, `focus`, `disabled`, `loading`, `empty`, `error`, `success`).
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en `BrowserWindow`, áreas de interacción táctiles y de ratón $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos (22×22 a 32×32 px), atajos universales y compatibilidad con lectores de pantalla (VoiceOver).
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, presupuestos de memoria RAM (máximo 4 GB), ejecución en memoria dry-run para tests de UI (< 5 ms) y pruebas globales < 10 segundos en CTest.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de perfiles y contención de rutas.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Gobernanza sobre el empaquetado del bundle nativo `.app` en macOS (`VigilioBrowser.app/Contents/MacOS` e `Info.plist`) y trazabilidad de binarios.
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e identidad vectorial Lucide SVG en QRC.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter», «CI macOS job») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de orquestación, validación y ejecución opera de manera estrictamente local en el equipo o runner de CI, sin dependencias cloud ni telemetría remota.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI macOS job` en la etapa `ui` es infraestructura de orquestación de compilación, empaquetado de bundle y pruebas; por consiguiente, no agrega widgets, botones ni consolas en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`. `WindowTests.cpp` en `tests/ui` consume ambas bibliotecas para certificar su integración y gobernanza.
  4. *Tolerancia headless y política Zero-Zombie*: Detección de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) con reporte transparente de `NOT_EXECUTED` (exit code 0) en entornos sin pantalla interactiva.
  5. *Preservación de «Interfaz más fácil de usar» (Modo Senior)*: Tap targets $\ge 48$ px, barra de herramientas $\ge 55$ px, banner $\ge 56$ px, escalado de iconos 22×22 $\to$ 32×32 px.
  6. *Gobernanza de bundle en macOS*: La etapa `BundleVerification` certifica que los ejecutables y recursos de UI se empaquetan en la estructura canónica `.app`.
  7. *Auditoría de lenguaje público*: Toda cadena o sugerencia de remediación emitida por `CiMacOsJobService` o `CiMacOsJobValidator` debe formularse en español neutro sin términos internos proscritos.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Arnés existente de pruebas de UI en Qt Test con bloques de prueba previos para Clang-Format, Clang-Tidy, CI Linux y CI Windows.
  - `apps/vigilio-browser/src/core/CiMacOsJobTypes.h`: Catálogo de tipos fuertemente tipados para CI macOS.
  - `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h/.cpp`: Planes canónicos oficiales (`canonicalDebugPlan`, `canonicalReleasePlan`).
  - `apps/vigilio-browser/src/core/CiMacOsJobValidator.h/.cpp`: Validador estático desacoplado y generador de copy en español neutro.
  - `apps/vigilio-browser/src/core/CiMacOsJobService.h/.cpp`: Capa operativa de servicio con orquestación, fail-fast y simulación dry-run.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Fallo de ejecución de pruebas en entornos de CI sin display.
    *Mitigación*: Preservar la guarda preliminar de `DISPLAY` / `WAYLAND_DISPLAY` en `WindowTests.cpp` reportando `NOT_EXECUTED` con exit code 0.
  - *Riesgo 2*: Fuga de terminología interna en cadenas de error devueltas al usuario.
    *Mitigación*: Aserciones exhaustivas en `WindowTests.cpp` que validan la ausencia de términos prohibidos en todas las enumeraciones y pistas de remediación.
  - *Riesgo 3*: Sobrecarga de tiempo en la suite de pruebas de UI.
    *Mitigación*: Validación en modo `CiMacOsExecutionMode::DryRun` en memoria con tiempo de ejecución < 5 ms.

---

## Checklist evidence and handoff

- **Pipeline continuo en CMake / CTest**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 25/25 pruebas pasando exitosamente en CTest en ~6.13s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.07s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.04s)
    6. `resource_qrc_catalog_tests`: Passed (0.06s)
    7. `resource_inclusion_service_tests`: Passed (0.13s)
    8. `resource_qrc_inclusion_tests`: Passed (0.27s)
    9. `startup_bootstrap_tests`: Passed (0.31s)
    10. `application_lifetime_tests`: Passed (0.21s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.11s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.11s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (2.90s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.16s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.14s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.77s)
    24. `ci_macos_job_configuration_tests`: Passed (0.01s)
    25. `ci_macos_job_service_tests`: Passed (0.68s)
- **Gobernanza con Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 59 referencias canónicas validadas (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0175-module.foundations.task0059.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
- **Handoff para la fase APPLY**:
  - La propuesta para la etapa `ui` de CI macOS job está completamente definida en `openspec/changes/module-foundations-task0059/`.
  - La implementación en APPLY consistirá en integrar el bloque de pruebas programáticas de CI macOS en `apps/vigilio-browser/tests/ui/WindowTests.cpp`, verificar compilación y CTest con `apps/vigilio-browser/scripts/check.sh`, validar referencias y lockfile con Prompt Builder CLI, y sincronizar `docs/rules/01-modules/foundations/micro/0059-ci-macos-job/task.md`.
