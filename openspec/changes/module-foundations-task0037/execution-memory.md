# Execution Memory — module-foundations-task0037

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **test fixture foundation** en el módulo `foundations` (`module.foundations.task0037`, microtarea canónica `0037 — Definir el comportamiento y las invariantes: test fixture foundation`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0037-test-fixture-foundation/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0109`.
  - Pack: `module.foundations.task0037`.
  - Fase: `propose`.
  - Generated artifact: `0109-module.foundations.task0037.propose.md`.
  - Prompt SHA-256: `0c06b4b0344c3694190ac09bcf1380f63966cad991fbd79963908b66b1c62644`.
  - Lock SHA-256: `0c93bc7bc0dc2d4179b07926570e24ba8999ecb68e26179c73fdc30051f3e017`.
  - Dependencias ejecutables: `module.foundations.task0036`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad y separación estricta de responsabilidades en C++20; desacoplamiento entre el núcleo de fixtures y la capa gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración deterministas en `TestFixtureFoundationTests.cpp`; uso de fixtures locales RAII en `/tmp`, ausencia de sleeps artificiales y cobertura de casos positivos, negativos, extremos y recuperación ante fallos.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), manejo seguro de errores sin excepciones no controladas y ausencia de dependencias hacia `QApplication` o `QWidget` en la infraestructura de prueba del núcleo.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Verificación de que la capacidad mantiene el contrato de «sin cambio visual directo» en la interfaz pública de usuario.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta y presupuestos acotados; cuota de disco limitada a 50 MB por fixture (`maxTempDirSizeBytes = 52428800`) y tiempos de ejecución en CTest inferiores a 100 ms sin lanzar instancias de Chromium.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, prevención contra Directory Traversal en nombres de fixtures y contención estricta en rutas de eliminación (`isSafeTestDirectory`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad mediante Clang-Format, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español comprensible.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de creación, gestión y eliminación de fixtures opera de manera estrictamente local en el equipo del usuario, sin componentes cloud ni telemetría.
  2. *Contención estricta de rutas temporales (`isSafeTestDirectory`)*: Cualquier carpeta administrada por el fixture debe residir obligatoriamente en `std::filesystem::temp_directory_path()`, comenzar con `vigilio_test_` y no coincidir con `/tmp` ni contener `..`. Queda terminantemente prohibido borrar cualquier ruta fuera de este confinamiento.
  3. *Garantía RAII y Zero-Zombie*: El destructor garantiza el saneamiento automático de carpetas temporales y la reversión de variables de entorno, evitando archivos o configuraciones huérfanas tras la prueba.
  4. *Cuota máxima de disco*: Se aplica un presupuesto máximo de 50 MB por fixture para impedir que bucles de prueba agoten el almacenamiento local.
  5. *Unicidad y concurrencia CTest*: Generación de rutas combinando PID + steady_clock nanosegundos + contador atómico, garantizando aislamiento total en ejecuciones paralelas (`ctest -jN`).
  6. *Saneamiento especializado de SQLite*: Capacidad de purgar de forma atómica y completa bases de datos SQLite temporales y sus diarios transaccionales (`-wal`, `-shm`).
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`: Estructura `TempDbGuard`.
  - `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`: Estructura `TempDirectoryFixture`.
  - `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`: Lógica de saneamiento seguro de directorios.
  - `apps/vigilio-browser/scripts/check.sh`: Arnés local de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Comportamiento de permisos en contenedores CI con usuario root donde los permisos de solo lectura no bloquean la eliminación.
    *Mitigación*: Validación condicional de permisos en `TEST-FIXTURE-009` para certificar el manejo de errores sin generar falsos positivos en entornos root.
  - *Riesgo 2*: Resolución temporal limitada en ciertas arquitecturas.
    *Mitigación*: Fusión determinista de marca de tiempo en nanosegundos con el PID del proceso y un contador atómico local.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 13/13 pruebas pasando exitosamente en CTest en 0.55s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.04s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.03s)
    9. `startup_bootstrap_tests`: Passed (0.05s)
    10. `application_lifetime_tests`: Passed (0.16s)
    11. `crash_recovery_tests`: Passed (0.02s)
    12. `cmake_presets_tests`: Passed (0.03s)
    13. `cmake_presets_service_tests`: Passed (0.09s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 37 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0109-module.foundations.task0037.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0037` en ejecución `0110`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0037/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 10 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
