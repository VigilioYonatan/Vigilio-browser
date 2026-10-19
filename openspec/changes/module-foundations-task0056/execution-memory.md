# Execution Memory — module-foundations-task0056

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **CI Windows job** en el módulo `foundations` (`module.foundations.task0056`, microtarea canónica `0056 — Probar casos positivos, negativos y recuperación: CI Windows job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0056-ci-windows-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0166`.
  - Pack: `module.foundations.task0056`.
  - Phase: `propose`.
  - Generated artifact: `0166-module.foundations.task0056.propose.md`.
  - Prompt SHA-256: `19b6a8eb0881156def3ead7c6a780caccd0ea922b131085eded6bc30a8069d1e`.
  - Lock SHA-256: `3e1a4249921f706cd1e3a9462b80332688b309e36993396a1e7bea79b437868e`.
  - Dependencias ejecutables: `module.foundations.task0055`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID, DRY, KISS y YAGNI; separación estricta entre la lógica de validación/servicio en `src/core/` y el arnés de pruebas automatizadas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; suites unitarias deterministas, aserciones directas, fixtures controlados y reporte estructurado sin sleeps ciegos ni flakiness.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`CiWindowsExecutionMode`, `CiWindowsJobStatus`), gestión RAII de recursos y ausencia de llamadas bloqueantes.
- `docs/skills/04-ux-ui-senior.md`: Interfaz sobria, moderna y profesional; preservación del principio de no impacto visual en el navegador de usuario final para tareas de automatización/CI.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de «Interfaz más fácil de usar» (Modo Senior), dimensiones de toque y texto legible.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por archivo, límites de memoria (RAM $\le 4\text{ GB}$) y tiempos de ejecución en CTest < 500 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas o servicios cloud de CI, contención de rutas y rechazo categórico de Directory Traversal (`..`, `..\\`), dispositivos reservados DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`) y prefijos UNC (`//`, `\\\\`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror` y verificación continua en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Políticas de empaquetado y compatibilidad con instaladores Windows.
- `docs/skills/11-assets-svg-quality.md`: Calidad de assets SVG, Lucide y branding oficial.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «CI Windows job») en cadenas públicas o de diagnóstico. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de CI Windows, validación y orquestación opera de manera estrictamente local en el equipo, sin dependencias de servicios externos ni telemetría remota.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Windows job` es infraestructura de automatización de compilación y calidad; no agrega botones, menús ni consolas en `BrowserWindow`.
  3. *Inmutabilidad en modo DryRun*: Las evaluaciones en modo `DryRun` jamás crean, modifican ni eliminan archivos en disco; operan exclusivamente en memoria.
  4. *Concurrencia masiva segura (16 hilos)*: Todo acceso a `plan_` y `options_` en `CiWindowsJobService` está gobernado por `std::mutex`, garantizando ausencia total de data races.
  5. *Política Zero-Zombie*: La cancelación cooperativa mediante `CancellationToken` aborta de inmediato las etapas subsiguientes, liberando recursos sin dejar procesos zombis o colgados esperando EOF.
  6. *Contención estricta de rutas y defensas Windows (Path Confinement)*: `cleanBuildDirectory` previene operaciones destructivas fuera de la raíz del proyecto verificando la ruta mediante `isSafeBuildPath`, rechazando traversal (`..`, `..\\`), rutas absolutas, prefijos UNC y dispositivos reservados DOS.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiWindowsJobTypes.h`: Taxonomía y estructuras operativas del pipeline de Windows.
  - `apps/vigilio-browser/src/core/CiWindowsJobValidator.h/.cpp`: Validador estático, orden topológico y comprobador de rutas seguras.
  - `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.h/.cpp`: Planes canónicos y filtros de versiones de Windows.
  - `apps/vigilio-browser/src/core/CiWindowsJobService.h/.cpp`: Capa operativa de servicio, simulación dry-run y textos públicos en español.
  - `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`: Arnés unitario de configuración y validación.
  - `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`: Arnés unitario de servicio operativo.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Arnés de pruebas de UI en Qt Test.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y formato.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Flakiness en pruebas multihilo debido a planificación impredecible del sistema operativo.
    *Mitigación*: Uso estricto de sincronización determinista (`std::atomic`, `std::mutex`, `join`), sin temporizadores ciegos (`sleep`).
  - *Riesgo 2*: Falsos positivos de permisos en entornos de ejecución con privilegios de root (`geteuid() == 0`).
    *Mitigación*: Detección de privilegios para seleccionar rutas protegidas del sistema o inyección controlada en `cleanBuildDirectory`.
  - *Riesgo 3*: Incremento excesivo del tiempo de ejecución de la suite en CTest.
    *Mitigación*: Uso del modo `DryRun` en memoria para las pruebas de concurrencia masiva, acotando el tiempo total a < 500 ms.

---

## Checklist evidence and handoff

- **Pipeline continuo actual (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 23/23 pruebas pasando exitosamente en CTest en ~4.52s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.16s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.23s)
    6. `resource_qrc_catalog_tests`: Passed (0.09s)
    7. `resource_inclusion_service_tests`: Passed (0.12s)
    8. `resource_qrc_inclusion_tests`: Passed (0.27s)
    9. `startup_bootstrap_tests`: Passed (0.20s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.05s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.11s)
    16. `clang_format_configuration_tests`: Passed (0.00s)
    17. `clang_format_service_tests`: Passed (1.97s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.12s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.14s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.67s)
- **Gobernanza de referencias canónicas y lockfile**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 56 referencias validadas con éxito (`valid: true`, exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0166-module.foundations.task0056.propose.lock.json --project-root .`: 13/13 comprobaciones exitosas (`valid: true`, exit code 0).
- **Handoff hacia APPLY**:
  - La propuesta técnica, el diseño detallado, la matriz de pruebas trazable y el plan de tareas numeradas están completamente especificados bajo `openspec/changes/module-foundations-task0056/`.
  - La base de código y la suite de pruebas se encuentran en estado verde, sin bloqueos técnicos ni dependencias insatisfechas.
