# Execution Memory — module-foundations-task0042

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **clang-format configuration** en el módulo `foundations` (`module.foundations.task0042`, microtarea canónica `0042 — Implementar la operación en una capa propietaria: clang-format configuration`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0042-clang-format-configuration/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0124`.
  - Pack: `module.foundations.task0042`.
  - Phase: `propose`.
  - Generated artifact: `0124-module.foundations.task0042.propose.md`.
  - Prompt SHA-256: `a344991ccab97030482cbfa67369056f92fee47b72669197b25523ed5df8b701`.
  - Lock SHA-256: `3e60cc3fb40898db013bab27c63911a2b4a3d6dd7486fd923f1c99b46715a582`.
  - Dependencias ejecutables: `module.foundations.task0041`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Separación estricta de responsabilidades (SOLID, KISS, YAGNI); diseño de la clase de servicio `ClangFormatService` desacoplada del subsistema gráfico y de Chromium.
- `docs/skills/02-testing-senior.md`: Pruebas unitarias deterministas en `ClangFormatServiceTests.cpp`, fixtures locales sin dependencias de red, esperas basadas en condiciones observables y ausencia de sleeps fijos arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class FormatExecutionStatus`), gestión RAII y ausencia de llamadas bloqueantes en el hilo principal.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por archivo (máximo 10 MB rechazado con `FileTooLarge`), límites de profundidad de directorios (máximo 32 niveles) y tiempos de ejecución en CTest < 150 ms para la suite completa.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español neutro comprensible.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de formateo, análisis de discrepancias e inspección opera de manera estrictamente local en el equipo del usuario, sin dependencias cloud ni telemetría.
  2. *Cancelación cooperativa y Zero-Zombie*: Cada operación de iteración en directorios o reintentos comprueba `CancellationToken::isCancelled()`, asegurando que no queden hilos zombis o bucles colgados.
  3. *Seguridad y contención de rutas (`isSafeConfigPath`)*: Cualquier ruta inspeccionada debe ser canónicamente segura y no contener secuencias de escape hacia directorios superiores (`..`).
  4. *Prevención de corrupción y reversibilidad*: Modificaciones destructivas en disco crean preventivamente una copia `.bak`, restaurable atómicamente mediante `rollbackFile` si la operación falla.
  5. *Límites estrictos de memoria*: Archivos que superen los 10 MB (`maxFileSizeBytes`) se rechazan de inmediato con `FormatExecutionStatus::FileTooLarge` sin cargarse a memoria.
  6. *Aislamiento y ligereza*: Operación 100% en `vigilio_core` sin widgets Qt ni dependencias hacia WebEngine.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/ClangFormatTypes.h`: Tipos de dominio de Clang-Format (ampliados con tipos operativos).
  - `apps/vigilio-browser/src/core/ClangFormatValidator.h/.cpp`: Validador de opciones, parser YAML y comprobador de rutas.
  - `apps/vigilio-browser/src/core/ClangFormatConfiguration.h/.cpp`: Configuración canónica y filtros de extensiones/carpetas.
  - `apps/vigilio-browser/src/core/CMakePresetsService.h/.cpp`: Patrón de servicio desacoplado y argumentos CLI.
  - `apps/vigilio-browser/src/core/TestFixtureService.h/.cpp`: Patrón de reintentos exponenciales y backups.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y formato.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Bloqueos transitorios de archivos en sistemas como Windows debido a procesos de escaneo o indexación.
    *Mitigación*: Implementación de `formatWithRetry` con retroceso exponencial ($50\text{ ms} \times 2^i$) y límite de 3 reintentos.
  - *Riesgo 2*: Interrupción abrupta de energía o fallo de disco durante la sobrescritura del archivo.
    *Mitigación*: Copia de seguridad atómica previa (`.bak`) que se conserva hasta que la escritura final se verifica como exitosa.
  - *Riesgo 3*: Escaneo de carpetas muy profundas o enlaces simbólicos recursivos.
    *Mitigación*: Control explícito de profundidad máxima (`maxScanDepth = 32`) y comprobación de `CancellationToken`.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 16/16 pruebas pasando exitosamente en CTest en 1.52s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.16s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.15s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.17s)
    8. `resource_qrc_inclusion_tests`: Passed (0.13s)
    9. `startup_bootstrap_tests`: Passed (0.29s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.06s)
    12. `cmake_presets_tests`: Passed (0.07s)
    13. `cmake_presets_service_tests`: Passed (0.14s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.13s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 42 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0124-module.foundations.task0042.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0042` en ejecución `0125`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0042/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 12 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
