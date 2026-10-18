# Execution Memory — module-foundations-task0025

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **application lifetime** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0025 — Definir el comportamiento y las invariantes: application lifetime`), especificando la máquina de estados de ciclo de vida con 9 estados discretos, los tipos de valor inmutables (`LifetimeTypes.h`), las 8 causas de terminación tipadas (`ShutdownReason`), las 4 acciones de resolución (`ShutdownAction`), el coordinador de dominio desacoplado (`ApplicationLifetimeCoordinator`), las políticas de persistencia atómica mediante `SessionFile.h`, el manejo seguro de señales del sistema operativo (`SIGINT`, `SIGTERM`), el temporizador de seguridad de 3000 ms (watchdog Zero-Zombie), y el invariante constitucional de aislamiento estricto de incógnito, bajo compilación estricta C++20 sin dependencias de `QApplication` ni WebEngine.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance de `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Estados visuales y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`fast`, `unit`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0025-application-lifetime/task.md`: Contrato vinculante de la microtarea 0025.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `22d695311db420b391ae59e6ecc7b37b3903c4aec771947b7095b0ed2b26969e`
  - Lock SHA-256: `bece9d1a68ecd742e7cff5ced63bf520220106082630703362f48c6b24af3a7b`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación estricta de responsabilidades entre el coordinador de ciclo de vida (`ApplicationLifetimeCoordinator`), los componentes de persistencia (`SessionFile`, `LocalRepository`), y el punto de entrada de la aplicación (`src/app.cpp`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias deterministas y de integración interna, con fixtures locales, sin esperas arbitrarias, aserciones claras y clasificación explícita en CTest con reporte preciso de `PASS`, `FAIL` o `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `std::filesystem`, `std::atomic`, `std::chrono`, `std::function`), RAII para flujos de datos, const-correctness estricta, y desacoplamiento absoluto de la lógica de dominio respecto a `QApplication` y widgets de UI.
- `docs/skills/04-ux-ui-senior.md`: Protección de la identidad visual sobria y profesional; diseño de transiciones y degradaciones limpias sin estados intermitentes ni bloqueos de pantalla.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Respeto absoluto a la denominación pública obligatoria según `docs/UX-COPY-PUBLICO.md`: prohibición de exponer términos de ingeniería en la UI y uso obligado de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia computacional y huella de memoria: evaluación de transiciones de estado en microsegundos, huella de memoria en reposo < 16 KB, y prevención activa de spawn de procesos auxiliares o zombis en caso de fallo temprano.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first constitucional (decisión Q-001): cero peticiones de red o telemetría durante el apagado; aislamiento estricto de sesiones de incógnito (`isIncognito == true`); persistencia atómica mediante `.tmp` y reemplazo seguro; y manejo seguro de señales POSIX sin llamadas no reentrantes.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, compilación con `-Werror` bajo C++20 y pruebas de regresión automatizadas en CTest bajo CMake/Ninja.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la arquitectura de dominio y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («application lifetime», «graceful teardown», «kill signal», «flush WAL», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Cero conexiones de red, pings de telemetría o consultas de actualización durante el ciclo de vida o apagado de la aplicación.
  2. *Aislamiento estricto de incógnito*: Si `config.isIncognito == true`, el coordinador garantiza que `sessionSaver` jamás sea invocado en disco, reportando `sessionSaved = false` y asegurando que no se cree ni modifique ningún archivo de sesión ni historial.
  3. *Persistencia atómica y resiliente*: El guardado de sesión se delega en `vigilio::saveSession` (`SessionFile.h`), garantizando que se escriba primero a un archivo temporal `.tmp` y se reemplace atómicamente, preservando la sesión anterior si la escritura falla.
  4. *Política Zero-Zombie y temporizador de seguridad*: Se impone un temporizador estricto de 3000 ms (`shutdownTimeoutMs`). Si el vaciado de datos o desmontaje se bloquea, se fuerza la transición a `EmergencyAbort` para evitar procesos zombis.
  5. *Deduplicación e idempotencia de apagado*: Múltiples solicitudes de apagado concurrentes o repetidas son deduplicadas; la primera ejecuta la secuencia de apagado y las subsiguientes retornan `IgnoreRequest` o aceleran a `EmergencyAbort` si se trata de señales repetidas del SO.
  6. *Determinismo en la máquina de estados*: Las transiciones entre los 9 estados de `AppLifetimeState` son validadas por `canTransition`, abortando cualquier transición anómala.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/app.cpp`: Punto de entrada actual con inicialización y llamada a `app.exec()`.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Manejo actual de `closeEvent` y `saveSession`.
  - `apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`: Implementación atómica de guardado y carga de sesiones.
  - `apps/vigilio-browser/src/storage/LocalRepository.h` y `.cpp`: Almacenamiento SQLite local.
  - `apps/vigilio-browser/src/core/StartupTypes.h` y `StartupBootstrapCoordinator.h`: Coordinador de arranque previo.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Bloqueo del proceso al salir debido a operaciones de disco lentas o conexiones SQLite abiertas, dejando procesos colgados (zombies).
    *Mitigación*: Implementación de un temporizador de seguridad de 3000 ms en el coordinador de ciclo de vida que fuerza el paso a `EmergencyAbort` ante retardos excesivos.
  - *Riesgo*: Invocación de funciones no seguras (reentrantes) dentro de manejadores de señales POSIX (`SIGINT`, `SIGTERM`).
    *Mitigación*: El coordinador desacopla la detección de señal del flujo de apagado; los manejadores de señal únicamente activan banderas atómicas estándar (`std::atomic<bool>`), permitiendo que el hilo principal procese el apagado dentro de su flujo determinista.
  - *Riesgo*: Filtración de datos de navegación en modo incógnito durante la secuencia de apagado.
    *Mitigación*: Invariante estricto en el coordinador: si `config.isIncognito == true`, `sessionSaver` jamás es ejecutado y no se escribe ningún archivo en disco.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 9/9 tests de CTest pasando exitosamente en 0.34s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.05s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.03s)
    7. `resource_inclusion_service_tests`: Passed (0.08s)
    8. `resource_qrc_inclusion_tests`: Passed (0.05s)
    9. `startup_bootstrap_tests`: Passed (0.06s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 25 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0073-module.foundations.task0025.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006, Q-008 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0026`)**:
  - Se definieron formalmente las 10 tareas verificables en `tasks.md` con comandos explícitos de CTest y shell.
  - Los tipos de datos de valor y la interfaz del coordinador están completamente especificados y listos para su implementación en C++20 dentro de `vigilio_core` durante la siguiente fase.
