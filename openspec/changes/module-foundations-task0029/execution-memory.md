# Execution Memory — module-foundations-task0029

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0029 — Definir el comportamiento y las invariantes: crash recovery policy`), proyectando el modelo de dominio en C++20, máquinas de estado de detección, archivo centinela de estado durable, protección contra bucles de caídas repetidas (crash loops), saneamiento estricto de URLs y garantía constitucional de privacidad de incógnito (Q-001) sin dependencias de red ni acoplamiento con widgets de UI o WebEngine.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `ui`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0029-crash-recovery-policy/task.md`: Contrato vinculante de la microtarea 0029.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 UI nativa Qt Widgets, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución: `0085`
  - Pack: `module.foundations.task0029`
  - Fase: `propose`
  - Prompt SHA-256: `a8307f153cae2559a80a8fcefdba26fda7e4bb1e3bed1d7c133c5b81630a6f5d`
  - Lock SHA-256: `3026f29725e76eb76423689ebb54841d54814bd0eea48b5a661ae35852bb1865`
  - Dependencias ejecutables: `module.foundations.task0028`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; encapsulamiento de la lógica de detección y centinela en una clase estática de dominio (`CrashRecoveryCoordinator`) desacoplada de la interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas centrada en pruebas unitarias deterministas y de alta velocidad en `vigilio_core`, sin dependencias de red ni levantamiento de procesos Chromium; aserciones claras y fixtures sintéticos en disco temporal.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, no bloqueo del hilo de interfaz, separación estricta entre `src/core` y `src/ui`.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en arranque; centinela JSON compacto (< 512 bytes) con lectura y parseo en < 0.5 ms; saneamiento acotado con tope de 1000 pestañas y 8192 bytes por URL.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero envío de reportes de caída a la red o servicios cloud; aislamiento total de incógnito (cero escrituras o lecturas de centinela o sesión en disco).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la detección de caídas y recuperación se realiza exclusivamente en almacenamiento local del perfil. Cero llamadas de red, telemetría o servicios cloud.
  2. *Aislamiento estricto de incógnito*: Si `isIncognito == true`, `CrashRecoveryCoordinator` garantiza que no se crea, lee ni modifica ningún archivo centinela o de sesión en disco, retornando siempre `NoPreviousSession` y `StartClean`.
  3. *Protección contra Crash Loop*: Ante $\ge 2$ caídas consecutivas, el sistema previene la reapertura automática de pestañas maliciosas o inestables, recomendando `SafeModeRecovery` o inicio limpio.
  4. *Persistencia atómica y resiliente*: Toda actualización del centinela utiliza archivo temporal `.tmp` y renombrado atómico (`std::filesystem::rename` en POSIX, `MoveFileExW` con `MOVEFILE_REPLACE_EXISTING` en Windows).
  5. *Saneamiento de URLs y Bounded Buffers*: Se validan esquemas permitidos (`http`, `https`, `vigilio://`) descartando esquemas peligrosos (`javascript:`, `data:`, `file:`), truncando a 1000 pestañas y 8192 bytes por URL.
  6. *Zero-Chromium y Zero-Zombie*: Las comprobaciones de dominio se ejecutan sin inicializar procesos Chromium ni WebEngine.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`: Persistencia atómica de sesiones de navegación (`saveSession`, `loadSession`).
  - `apps/vigilio-browser/src/core/LifetimeTypes.h` y `ApplicationLifetimeCoordinator.h` & `.cpp`: Tipos de ciclo de vida y reporte de cierre limpio (`isCleanExit`).
  - `apps/vigilio-browser/src/core/UrlInput.h` y `.cpp`: Motor de resolución de entradas de URL (`resolveInput`).
  - `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` y `.cpp`: Modos de arranque e inicialización.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Implementación de ventana principal.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Bucle infinito de caídas por páginas inestables que tumban el motor WebEngine.
    *Mitigación*: Mecanismo de detección de crash loop en el centinela (`consecutive_crashes >= threshold`), que escala automáticamente a `SafeModeRecovery`.
  - *Riesgo*: Corrupción de archivos de estado ante pérdidas repentinas de alimentación eléctrica.
    *Mitigación*: Escritura siempre atómica mediante archivo `.tmp` y renombrado transaccional en el sistema de archivos. Si el archivo resulta dañado, se archiva de forma segura como `.corrupt.bak` y el navegador continúa sin crashear.
  - *Riesgo*: Fuga de privacidad en modo incógnito.
    *Mitigación*: Validación de precondición estricta: si `config.isIncognito == true`, se cortocircuita cualquier acceso al sistema de archivos.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 10/10 tests de CTest pasando exitosamente en 0.54s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.09s)
    6. `resource_qrc_catalog_tests`: Passed (0.03s)
    7. `resource_inclusion_service_tests`: Passed (0.07s)
    8. `resource_qrc_inclusion_tests`: Passed (0.06s)
    9. `startup_bootstrap_tests`: Passed (0.07s)
    10. `application_lifetime_tests`: Passed (0.14s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 29 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0085-module.foundations.task0029.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0030`)**:
  - Se definieron formalmente las 10 tareas verificables en `tasks.md` con criterios precisos y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
