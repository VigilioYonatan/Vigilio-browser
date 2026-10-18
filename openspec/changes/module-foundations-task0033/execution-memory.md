# Execution Memory — module-foundations-task0033

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **CMake presets** en el módulo propietario `foundations` (`module.foundations.task0033`, microtarea `0033 — Definir el comportamiento y las invariantes: CMake presets`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0097`.
  - Pack: `module.foundations.task0033`.
  - Fase: `propose`.
  - Generated artifact: `0097-module.foundations.task0033.propose.md`.
  - Prompt SHA-256: `e9cce35269261c396b4c3cd77a57b4065ba37aee2f05ce7c26f6bba4582eb71e`.
  - Lock SHA-256: `b1a3998715f01c4061087e10520e98611d2826047ff54c6b4e49101393165016`.
  - Dependencias ejecutables: `module.foundations.task0032`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre el catálogo declarativo de presets y el validador estático; interfaces pequeñas y enfocadas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas centrada en pruebas unitarias deterministas en `CMakePresetsTests.cpp`, fixtures locales en memoria y CTest rápido (< 50 ms).
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, tipos estándar (`<string>`, `<vector>`, `<map>`, `<filesystem>`), RAII, separación modular y desacoplamiento absoluto de Qt Widgets.
- `docs/skills/06-rendimiento-y-ram.md`: Análisis instantáneo en memoria (< 2 ms) con presupuesto de memoria acotado (< 2 MB) y cero procesos en segundo plano.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero descargas remotas o telemetría; rechazo riguroso de Directory Traversal (`..`) y rutas absolutas fijadas al host en `binaryDir`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato ClangFormat, compilación con `-Werror` y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta entre PROPOSE (especificación integral de dominio y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición de tecnicismos de ingeniería («CMake», «Presets», «Toolchain», «Prompt Builder») en interfaces públicas. Constatación formal de ausencia de cambio visual directo.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Configuración y resolución de presets 100% local, sin peticiones remotas ni servicios cloud.
  2. *Independencia del Núcleo*: La capacidad de presets pertenece al build system e infraestructura del núcleo (`vigilio_core`), compilable sin Qt ni WebEngine.
  3. *Seguridad de Rutas (`binaryDir`)*: Prohibición terminante de secuencias de escape `..` o rutas absolutas forzadas; toda compilación debe ubicarse bajo `${sourceDir}/build-*`.
  4. *Esquema Mínimo y Presets Críticos*: Se exige esquema CMake Presets v6 (mínimo v3) y la presencia obligatoria de los perfiles `core-debug` y `desktop-debug`.
  5. *Integridad Referencial*: Todo `buildPreset` y `testPreset` debe apuntar a un `configurePreset` existente en el mismo archivo.
  6. *Ausencia de Impacto Visual*: La capacidad es puramente de infraestructura de desarrollo y construcción; no altera la interfaz gráfica de usuario.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakePresets.json`: Archivo existente con esquema v6 y presets base `core-debug` y `desktop-debug`.
  - `apps/vigilio-browser/CMakeLists.txt`: Configuración principal del proyecto y conmutadores `VIGILIO_BUILD_GUI`, `BUILD_TESTING`, etc.
  - `apps/vigilio-browser/scripts/check.sh`: Script de compilación y pruebas continuas con Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Discrepancias de formato en rutas relativas de `binaryDir` entre Windows y Linux.
    *Mitigación*: Uso universal de barras `/` soportadas nativamente por CMake en todas las plataformas.
  - *Riesgo*: Presets huérfanos generados durante el desarrollo que provoquen fallos silenciosos en CI.
    *Mitigación*: Validación exhaustiva en tiempo de pruebas unitarias certificando que no existan referencias rotas.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 11/11 pruebas de CTest pasando exitosamente en 0.68s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.07s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.08s)
    8. `resource_qrc_inclusion_tests`: Passed (0.12s)
    9. `startup_bootstrap_tests`: Passed (0.16s)
    10. `application_lifetime_tests`: Passed (0.15s)
    11. `crash_recovery_tests`: Passed (0.02s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 33 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0097-module.foundations.task0033.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 resueltas (Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0033` en ejecución `0098`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0033/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se redactaron 9 tareas atómicas, secuenciales y verificables en `tasks.md` con criterios concretos y comandos de comprobación.
  - La propuesta está completamente lista y es apta para revisión humana previa a APPLY.
