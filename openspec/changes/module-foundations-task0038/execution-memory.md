# Execution Memory — module-foundations-task0038

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **test fixture foundation** en el módulo `foundations` (`module.foundations.task0038`, microtarea canónica `0038 — Implementar la operación en una capa propietaria: test fixture foundation`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0038-test-fixture-foundation/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0112`.
  - Pack: `module.foundations.task0038`.
  - Phase: `propose`.
  - Generated artifact: `0112-module.foundations.task0038.propose.md`.
  - Prompt SHA-256: `b0db2b86c5709ad8eac98a81fb5d0723fb07208c064e853534ff9f1ee1a481cc`.
  - Lock SHA-256: `c97ce9d34a761a8ea7e967b5f7f357da7f4814c7136215bf287c83916b55d3a2`.
  - Dependencias ejecutables: `module.foundations.task0037`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad y separación estricta de responsabilidades en C++20; desacoplamiento estricto entre la capa de servicio de fixtures y la interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración deterministas en `TestFixtureServiceTests.cpp`; uso de fixtures locales RAII en `/tmp`, ausencia de sleeps artificiales y cobertura de casos positivos, negativos, extremos y recuperación ante fallos.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), manejo seguro de errores sin excepciones no controladas y ausencia total de dependencias hacia `QApplication`, `QWidget` o `BrowserWindow` en la infraestructura de prueba del núcleo.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Verificación de que la capacidad mantiene el contrato de «sin cambio visual directo» en la interfaz visible de usuario.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta y presupuestos acotados; cuota de disco limitada a 50 MB por fixture (`maxTempDirSizeBytes = 52428800`), tiempos de ejecución en CTest inferiores a 150 ms para la suite del servicio y cero procesos colgados (política Zero-Zombie con `CancellationToken`).
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, prevención absoluta contra Directory Traversal en nombres de fixtures y contención estricta en rutas de eliminación (`isSafeTestDirectory`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad mediante Clang-Format, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación física en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Preservación de la integridad del empaquetado de producción sin filtración de artefactos de prueba temporales.
- `docs/skills/11-assets-svg-quality.md`: Constatación de ausencia de impacto sobre recursos vectoriales o iconos de la aplicación.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español neutro comprensible.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de creación, gestión, snapshots y purga de fixtures opera de manera estrictamente local en el equipo del usuario, sin componentes cloud ni telemetría.
  2. *Contención estricta de rutas temporales (`isSafeTestDirectory`)*: Cualquier carpeta administrada o purgada por `TestFixtureService` debe residir obligatoriamente en `std::filesystem::temp_directory_path()`, comenzar con `vigilio_test_` y no coincidir con `/tmp` ni contener `..`. Queda terminantemente prohibido borrar cualquier ruta fuera de este confinamiento.
  3. *Garantía Zero-Zombie y Cancelación Cooperativa*: Todas las operaciones intensivas de I/O de disco (escaneo de huérfanos, copiado de snapshots, purga masiva) aceptan un `CancellationToken*` y abortan inmediatamente sin dejar procesos o hilos bloqueados.
  4. *Cuota máxima de disco*: Se aplica un presupuesto acumulativo máximo de 50 MB por fixture para impedir que bucles de prueba agoten el almacenamiento local.
  5. *Unicidad y concurrencia CTest*: Generación de rutas combinando PID + steady_clock nanosegundos + contador atómico, garantizando aislamiento total en ejecuciones paralelas (`ctest -jN`).
  6. *Recuperación resiliente ante bloqueos*: Soporte para reintentos con retroceso exponencial (`cleanWithRetry`) para mitigar contención transitoria de descriptores de archivos en Windows o entornos con antivirus.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CMakePresetsService.h` y `.cpp`: Modelo de cancelación cooperativa con `CancellationToken` y ejecución acotada.
  - `apps/vigilio-browser/src/core/TestFixtureFoundation.h` y `.cpp`: Primitivas RAII del dominio base.
  - `apps/vigilio-browser/src/core/TestFixtureValidator.h` y `.cpp`: Motor estático de validación de seguridad de rutas y copy público.
  - `apps/vigilio-browser/scripts/check.sh`: Arnés local de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Contención de bloqueos transitorios en Windows por antivirus o indexadores.
    *Mitigación*: `cleanWithRetry` con retroceso exponencial configurable (hasta 3 reintentos con backoff).
  - *Riesgo 2*: Consumo excesivo de disco por creación de múltiples instantáneas en suites largas.
    *Mitigación*: Validación de cuota acumulativa antes de generar instantáneas (`FixtureOperationStatus::QuotaExceeded`).
  - *Riesgo 3*: Eliminación accidental de carpetas del sistema en la purga de huérfanos.
    *Mitigación*: Validación obligatoria de triple nivel con `TestFixtureValidator::isSafeTestDirectory` antes de cualquier llamada a `remove_all`.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 14/14 pruebas pasando exitosamente en CTest en 0.80s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.13s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.09s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.05s)
    8. `resource_qrc_inclusion_tests`: Passed (0.06s)
    9. `startup_bootstrap_tests`: Passed (0.09s)
    10. `application_lifetime_tests`: Passed (0.15s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.02s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 38 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0112-module.foundations.task0038.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0038` en ejecución `0113`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0038/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 12 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
