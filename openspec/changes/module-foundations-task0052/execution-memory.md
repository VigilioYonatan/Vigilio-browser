# Execution Memory — module-foundations-task0052

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **CI Linux job** en el módulo `foundations` (`module.foundations.task0052`, microtarea canónica `0052 — Probar casos positivos, negativos y recuperación: CI Linux job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0052-ci-linux-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0154`.
  - Pack: `module.foundations.task0052`.
  - Phase: `propose`.
  - Generated artifact: `0154-module.foundations.task0052.propose.md`.
  - Prompt SHA-256: `9abcb39d68bbafd60be5596f02af514f9aa64e7d375f095a9b463ce676f0f227`.
  - Lock SHA-256: `72a7c98efea61465de53f46dfa2fb08ba38706e204e40fc2457fb04cb89f98b6`.
  - Dependencias ejecutables: `module.foundations.task0051`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID, DRY, KISS y YAGNI; separación estricta entre la lógica de validación/servicio en `src/core/` y el arnés de pruebas automatizadas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; suites unitarias deterministas, aserciones directas, fixtures controlados y reporte estructurado sin sleeps ciegos ni flakiness.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`CiLinuxExecutionMode`, `CiLinuxJobStatus`), gestión RAII de recursos y ausencia de llamadas bloqueantes.
- `docs/skills/04-ux-ui-senior.md`: Interfaz sobria, moderna y profesional; preservación del principio de no impacto visual en el navegador de usuario final para tareas de CI.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de «Interfaz más fácil de usar» (Modo Senior), dimensiones de toque y texto legible.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por archivo, límites de memoria (RAM $\le 4\text{ GB}$) y tiempos de ejecución en CTest < 500 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas o servicios cloud de CI, contención de rutas y rechazo categórico de Directory Traversal (`..`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror` y verificación continua en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Políticas de empaquetado y compatibilidad de distribución.
- `docs/skills/11-assets-svg-quality.md`: Calidad de assets SVG, Lucide y branding oficial.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «CI Linux job») en cadenas públicas o de diagnóstico. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de CI Linux, validación y orquestación opera de manera estrictamente local en el equipo, sin dependencias de servicios externos ni telemetría remota.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Linux job` es infraestructura de automatización de compilación y calidad; no agrega botones, menús ni consolas en `BrowserWindow`.
  3. *Inmutabilidad en modo DryRun*: Las evaluaciones en modo `DryRun` jamás crean, modifican ni eliminan archivos en disco; operan exclusivamente en memoria.
  4. *Concurrencia masiva segura (16 hilos)*: Todo acceso a `plan_` y `options_` en `CiLinuxJobService` está gobernado por `std::mutex`, garantizando ausencia total de data races.
  5. *Política Zero-Zombie*: La cancelación cooperativa mediante `CancellationToken` aborta de inmediato las etapas subsiguientes, liberando recursos sin dejar procesos zombis o colgados esperando EOF.
  6. *Contención estricta de rutas (Path Confinement)*: `cleanBuildDirectory` previene operaciones destructivas fuera de la raíz del proyecto verificando la ruta mediante `isSafeBuildPath`.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`: Taxonomía y estructuras operativas del pipeline de Linux.
  - `apps/vigilio-browser/src/core/CiLinuxJobValidator.h/.cpp`: Validador estático, orden topológico y comprobador de rutas seguras.
  - `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h/.cpp`: Planes canónicos y filtros de distribución.
  - `apps/vigilio-browser/src/core/CiLinuxJobService.h/.cpp`: Capa operativa de servicio, simulación dry-run y textos públicos en español.
  - `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`: Arnés unitario de configuración y validación.
  - `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`: Arnés unitario de servicio operativo.
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
  - 21/21 pruebas pasando exitosamente en CTest en ~3.96s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.17s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.27s)
    6. `resource_qrc_catalog_tests`: Passed (0.09s)
    7. `resource_inclusion_service_tests`: Passed (0.10s)
    8. `resource_qrc_inclusion_tests`: Passed (0.12s)
    9. `startup_bootstrap_tests`: Passed (0.20s)
    10. `application_lifetime_tests`: Passed (0.20s)
    11. `crash_recovery_tests`: Passed (0.04s)
    12. `cmake_presets_tests`: Passed (0.05s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.02s)
    15. `test_fixture_service_tests`: Passed (0.11s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (2.24s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.14s)
    20. `ci_linux_job_configuration_tests`: Passed (0.01s)
    21. `ci_linux_job_service_tests`: Passed (0.05s)
- **Gobernanza con Prompt Builder CLI**:
  - `validate-references`: `valid: true` (52 referencias canónicas comprobadas, exit code 0).
  - `verify-lock`: `valid: true` (13 comprobaciones pasadas para 0154, exit code 0).
- **Handoff a la fase APPLY**:
  - La propuesta técnica OpenSpec bajo `openspec/changes/module-foundations-task0052/` queda completamente definida, coherente y lista para revisión humana y posterior ejecución incremental en APPLY.
