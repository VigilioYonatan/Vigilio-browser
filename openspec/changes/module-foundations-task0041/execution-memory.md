# Execution Memory — module-foundations-task0041

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **clang-format configuration** en el módulo `foundations` (`module.foundations.task0041`, microtarea canónica `0041 — Definir el comportamiento y las invariantes: clang-format configuration`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0041-clang-format-configuration/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0121`.
  - Pack: `module.foundations.task0041`.
  - Phase: `propose`.
  - Generated artifact: `0121-module.foundations.task0041.propose.md`.
  - Prompt SHA-256: `063f65e2d5ae666290f4f3c7ef62d037445a77462987877089adeaf88a2787f1`.
  - Lock SHA-256: `77d867fe548d08e7a7068e3e424b484ff55a00f7fd72cba2a85c95bee3f1f139`.
  - Dependencias ejecutables: `module.foundations.task0040`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad y separación estricta de responsabilidades en C++20; desacoplamiento entre el núcleo de configuración de formato y la capa gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración deterministas en `ClangFormatConfigurationTests.cpp`; ausencia de sleeps fijos, fixtures deterministas y cobertura de casos positivos, negativos y extremos.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), manejo seguro de errores sin excepciones no controladas y ausencia de dependencias hacia `QApplication` o `QWidget` en el modelo de dominio.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta y presupuestos acotados; validación en memoria < 5 ms, parsing determinista sin bucles no acotada y tiempos de ejecución en CTest inferiores a 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, prevención contra Directory Traversal en nombres de archivos y contención estricta en rutas (`isSafeConfigPath`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad mediante Clang-Format, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español neutro comprensible.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de análisis, serialización y validación de Clang-Format opera de manera estrictamente local en el equipo del usuario, sin dependencias cloud ni telemetría.
  2. *Seguridad y contención de rutas (`isSafeConfigPath`)*: Cualquier ruta inspeccionada debe ser canónicamente segura y no contener secuencias de escape hacia directorios superiores (`..`).
  3. *Determinismo YAML*: La serialización de `ClangFormatOptions` a YAML y su posterior deserialización mediante `parseConfig` es idempotente y libre de pérdidas de información.
  4. *Coherencia canónica*: `ClangFormatConfiguration::canonicalConfig()` refleja con total exactitud las opciones declaradas en el archivo físico `apps/vigilio-browser/.clang-format` (`BasedOnStyle: LLVM`, `IndentWidth: 4`, `ColumnLimit: 105`, `PointerAlignment: Left`, `AllowShortFunctionsOnASingleLine: None`).
  5. *Aislamiento y ligereza*: Operación 100% en `vigilio_core` sin hilos adicionales, sin procesos externos zombis y sin dependencias de interfaces gráficas.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/.clang-format`: Archivo fuente de la configuración canónica en el repositorio.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y dry-run de formato.
  - `apps/vigilio-browser/src/core/CMakePresetsValidator.h/.cpp`: Patrón de validador desacoplado en C++20.
  - `apps/vigilio-browser/src/core/TestFixtureValidator.h/.cpp`: Patrón de validador con copy público en español y contención de rutas.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Desincronización entre el archivo `.clang-format` físico y la entidad en C++.
    *Mitigación*: La prueba `TEST-FORMAT-005` analiza directamente el archivo físico del árbol del proyecto contra las opciones canónicas.
  - *Riesgo 2*: Parseo de directivas YAML complejas no contempladas.
    *Mitigación*: El parser está acotado al subconjunto de opciones y directivas oficialmente soportadas por Vigilio Browser, ignorando comentarios y espacios sin fallar catastróficamente.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 15/15 pruebas pasando exitosamente en CTest en 1.10s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.11s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.07s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.08s)
    8. `resource_qrc_inclusion_tests`: Passed (0.10s)
    9. `startup_bootstrap_tests`: Passed (0.19s)
    10. `application_lifetime_tests`: Passed (0.17s)
    11. `crash_recovery_tests`: Passed (0.06s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.12s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 41 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0121-module.foundations.task0041.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0041` en ejecución `0122`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0041/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 9 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
