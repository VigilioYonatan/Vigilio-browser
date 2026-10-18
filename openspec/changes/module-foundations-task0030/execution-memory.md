# Execution Memory — module-foundations-task0030

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0030 — Implementar la operación en una capa propietaria: crash recovery policy`), proyectando la capa de servicio operativo en C++20, transiciones de decisión, soporte para cancelación y rollback de recuperación, latido periódico, y coordinación con `ApplicationLifetimeCoordinator` sin acoplar lógica de negocio a los widgets de interfaz ni añadir responsabilidades a `BrowserWindow`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `ui`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md`: Contrato vinculante de la microtarea 0030.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 UI nativa Qt Widgets, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución actual: `0088`
  - Pack: `module.foundations.task0030`
  - Fase: `propose`
  - Generated artifact: `0088-module.foundations.task0030.propose.md`
  - Prompt SHA-256: `71ce1c5ced76ebfacb9de6bfc90a44674aa1f58b86b89c0812c248755afcd660`
  - Lock SHA-256: `9efbf5bef74eb5fc1b45e555adc7b5aa9f59cfc446302a0e8703df7649dfea34`
  - Dependencias ejecutables: `module.foundations.task0029`
  - Paso de fase anterior: `none`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; encapsulamiento de la lógica de recuperación operativa en `src/core`, manteniendo una interfaz limpia y desacoplada de la interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias deterministas y de alta velocidad en `vigilio_core`, sin dependencias de red ni levantamiento de procesos Chromium; fixtures sintéticos en disco temporal.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, no bloqueo del hilo de interfaz, separación estricta entre `src/core` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Principios visuales y de accesibilidad para la futura integración UI (etapa 0031); navegación por teclado, foco visible y Modo Senior («Interfaz más fácil de usar»).
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en arranque; operaciones atómicas de disco < 1 ms; saneamiento de URLs acotado con tope de 1000 pestañas y 8192 bytes por URL.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero envío de reportes de caída a la red o servicios cloud; aislamiento total de incógnito (cero escrituras o lecturas en disco).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato Clang-Format, compilación con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».
- Referencias visuales en `/resultado/`:
  - `resultado/01-vigilio-home.png`: Ventana principal con pestañas y buscador.
  - `resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
  - `resultado/06-logo-instalador-oficial.png`: Logotipo oficial.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la detección de caídas, centinela y recuperación se realiza exclusivamente en almacenamiento local del perfil. Cero llamadas de red, telemetría o servicios cloud.
  2. *Aislamiento estricto de incógnito*: Si `config.isIncognito == true`, se garantiza que no se crea, lee ni modifica ningún archivo centinela o de sesión en disco, devolviendo siempre éxito en memoria y reporte limpio.
  3. *Protección contra Crash Loop*: Ante $\ge 2$ caídas consecutivas, el sistema previene la reapertura automática ciega de pestañas inestables, recomendando `SafeModeRecovery` o inicio limpio.
  4. *Persistencia atómica y resiliente*: Toda actualización del centinela utiliza archivo temporal `.tmp` y renombrado transaccional en el sistema de archivos (`std::filesystem::rename` en POSIX, `MoveFileExW` con `MOVEFILE_REPLACE_EXISTING` en Windows).
  5. *Saneamiento de URLs y Bounded Buffers*: Se validan esquemas permitidos (`http`, `https`, `vigilio://`) descartando esquemas peligrosos (`javascript:`, `data:`, `file:`), truncando a 1000 pestañas y 8192 bytes por URL.
  6. *Desacoplamiento de Widgets*: Ninguna clase en `src/core` incluye headers de Qt Widgets (`QWidget`, `QMainWindow`, `QTabWidget`) ni `QWebEngineView`.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Tipos y estructuras base de recuperación.
  - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` & `.cpp`: Métodos estáticos de dominio de recuperación.
  - `apps/vigilio-browser/src/core/SessionFile.h` & `.cpp`: Persistencia atómica de sesiones (`saveSession`, `loadSession`).
  - `apps/vigilio-browser/src/core/LifetimeTypes.h` y `ApplicationLifetimeCoordinator.h` & `.cpp`: Tipos de ciclo de vida y reporte de cierre limpio (`isCleanExit`).
  - `apps/vigilio-browser/src/core/UrlInput.h` & `.cpp`: Motor de resolución de entradas de URL (`resolveInput`).
  - `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` & `.cpp`: Modos de arranque e inicialización.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` & `.cpp`: Ventana principal del navegador.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Bucle infinito de caídas por páginas maliciosas que desbordan la memoria de WebEngine.
    *Mitigación*: Mecanismo de detección de crash loop en el centinela (`consecutive_crashes >= threshold`), que escala automáticamente a `SafeModeRecovery`.
  - *Riesgo*: Corrupción de archivos de estado ante pérdidas repentinas de alimentación eléctrica.
    *Mitigación*: Escritura siempre atómica mediante archivo `.tmp` y renombrado transaccional en el sistema de archivos. Si el archivo resulta dañado, se archiva de forma segura como `.corrupt.bak` y el navegador continúa sin crashear.
  - *Riesgo*: Fuga de privacidad en modo incógnito.
    *Mitigación*: Validación de precondición estricta: si `config.isIncognito == true`, se cortocircuita cualquier acceso al sistema de archivos.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 11/11 tests de CTest pasando exitosamente en 0.60s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.15s)
    9. `startup_bootstrap_tests`: Passed (0.13s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.00s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 30 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0088-module.foundations.task0030.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0030` en ejecución 0089)**:
  - Se definieron formalmente las 10 tareas verificables en `tasks.md` con criterios precisos y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
