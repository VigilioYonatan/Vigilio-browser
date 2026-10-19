# Execution Memory — module-foundations-task0046

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **clang-tidy configuration** en el módulo `foundations` (`module.foundations.task0046`, microtarea canónica `0046 — Implementar la operación en una capa propietaria: clang-tidy configuration`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0136`.
  - Pack: `module.foundations.task0046`.
  - Phase: `propose`.
  - Generated artifact: `0136-module.foundations.task0046.propose.md`.
  - Prompt SHA-256: `c7e30e14f38fd942c5bdbc5f43e4f0f3c8525270ae88e64eebe8c55b1f79a7fa`.
  - Lock SHA-256: `7b7e1afaad959415e41e24363069778ca92a939ec4ed07ad690d6867dbe1a27c`.
  - Dependencias ejecutables: `module.foundations.task0045`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad y separación estricta de responsabilidades en C++20; desacoplamiento total entre el servicio de linteo (`ClangTidyService`) y la capa gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas con tests deterministas aislados en directorios temporales (`TempDirGuard`); ausencia de sleeps rígidos y cobertura de fallos transitorios con retroceso exponencial.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), manejo robusto de excepciones de E/S y ausencia de dependencias hacia `QApplication` o `QWidget` en `vigilio_core`.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta y presupuestos acotados; cuota máxima de 10 MB por archivo (`FileTooLarge`), profundidad de escaneo de 32 niveles y ejecución de suite en CTest < 150 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención rigurosa en rutas (`isSafeConfigPath`) y prevención contra Directory Traversal.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad mediante ClangFormat y Clang-Tidy, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español neutro comprensible.
- `resultado/README.md`: Revisión de referencias visuales de la aplicación y preservación de la dirección artística del proyecto.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Todo el análisis estático, correcciones automáticas y validaciones operan exclusivamente en el equipo local del usuario, sin dependencias cloud ni telemetría.
  2. *Seguridad y contención de rutas (`isSafeConfigPath`)*: Cualquier ruta inspeccionada o modificada debe validar `isSafeConfigPath` y no contener secuencias de escape hacia directorios superiores (`..`).
  3. *Presupuestos estrictos de recursos*: Archivos que superen 10 MB son rechazados inmediatamente (`FileTooLarge`), y la recursión se detiene en 32 niveles de profundidad.
  4. *Garantía Zero-Zombie*: Cancelación cooperativa multihilo mediante `CancellationToken*`, comprobado en cada archivo y en cada iteración de reintento.
  5. *Preservación atómica de respaldos (.bak) y rollback*: Todo archivo físico modificado genera previamente una copia `.bak`, asegurando rollback transparente ante cualquier error de E/S.
  6. *Resiliencia ante bloqueos de archivo*: Reintentos con retroceso exponencial ($50\text{ ms} \times 2^i$) ante contención de bloqueos (`FileLocked`).
  7. *Aislamiento y ligereza*: Implementación en `vigilio_core` sin hilos zombis ni acoplamiento a WebEngine o interfaz gráfica.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/ClangFormatService.h/.cpp`: Patrón probado de servicio desacoplado, respaldos `.bak`, reintentos exponenciales y síntesis CLI.
  - `apps/vigilio-browser/src/core/ClangTidyTypes.h`: Modelo de dominio de tipos y estados establecido en `task0045`.
  - `apps/vigilio-browser/src/core/ClangTidyValidator.h/.cpp`: Validador estático, parser YAML y funciones de seguridad de rutas.
  - `apps/vigilio-browser/src/core/ClangTidyConfiguration.h/.cpp`: Configuración canónica y filtros de extensión/exclusión.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua de compilación, tests y linteo.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Modificación destructiva accidental durante correcciones físicas.
    *Mitigación*: Mecanismo obligatorio de respaldo `.bak` previo a cualquier escritura, con función garantizada de `rollbackFile`.
  - *Riesgo 2*: Hilos o subprocesos colgados en directorios masivos.
    *Mitigación*: Verificación periódica de `CancellationToken` en cada archivo procesado, límite de profundidad de 32 niveles y timeout de 15s.
  - *Riesgo 3*: Introducción involuntaria de jerga técnica en cadenas visibles.
    *Mitigación*: La prueba `TEST-TIDY-SRV-012` audita programáticamente que ninguna cadena generada contenga términos prohibidos por `docs/UX-COPY-PUBLICO.md`.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 18/18 pruebas pasando exitosamente en CTest en 2.86s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.13s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.19s)
    6. `resource_qrc_catalog_tests`: Passed (0.09s)
    7. `resource_inclusion_service_tests`: Passed (0.12s)
    8. `resource_qrc_inclusion_tests`: Passed (0.08s)
    9. `startup_bootstrap_tests`: Passed (0.09s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.12s)
    13. `cmake_presets_service_tests`: Passed (0.20s)
    14. `test_fixture_foundation_tests`: Passed (0.02s)
    15. `test_fixture_service_tests`: Passed (0.19s)
    16. `clang_format_configuration_tests`: Passed (0.01s)
    17. `clang_format_service_tests`: Passed (1.39s)
    18. `clang_tidy_configuration_tests`: Passed (0.00s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 46 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0136-module.foundations.task0046.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0046` en ejecución `0137`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0046/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 12 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
