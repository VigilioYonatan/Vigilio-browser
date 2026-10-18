# Execution Memory — module-foundations-task0028

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **application lifetime** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0028 — Probar casos positivos, negativos y recuperación: application lifetime`), proyectando una suite exhaustiva de pruebas automatizadas deterministas en C++20 para validar el apagado normal, entradas y estados inválidos, concurrencia multi-hilo, cancelación de apagado, manejo de señales, watchdog de seguridad, fallos inyectados en callbacks y garantía constitucional de incógnito (Q-001) sin dependencias de red ni levantamiento de procesos Chromium.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `ui`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md`: Contrato vinculante de la microtarea 0028.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 UI nativa Qt Widgets, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución: `0082`
  - Pack: `module.foundations.task0028`
  - Fase: `propose`
  - Prompt SHA-256: `ca9b2d5966c5889f25027009ea186de1a9b12c41d6e6cb1fed1cd61d098b0129`
  - Lock SHA-256: `7aa73430d970e14f7926a3499aabc31ec8155728c0ce3b50a17eb35b8a6a71a4`
  - Dependencias ejecutables: `module.foundations.task0027`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre la capa de orquestación atómica del ciclo de vida (`ApplicationLifetimeCoordinator`) y los contratos de prueba unitaria.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración de alta velocidad y determinismo; fixtures locales, ausencia de sleeps arbitrarios para sincronización lógica, y registro claro de estados.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::atomic`, `std::unique_ptr`, `std::function`), ownership RAII riguroso, no bloqueo del hilo principal y const-correctness.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en apagado; límite de tiempo estricto garantizado por el watchdog (3000 ms), pruebas ejecutables en < 250 ms, y consumo mínimo de memoria (< 15 MB).
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero peticiones de red o telemetría durante la salida; aislamiento total de incógnito (cero escrituras en disco).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto Clang-Format, compilación bajo C++20 con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la interacción y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («application lifetime», «graceful teardown», «kill signal», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Cero conexiones de red, pings de telemetría o consultas de actualización durante el ciclo de vida o salida de la aplicación.
  2. *Aislamiento estricto de incógnito*: Si `isIncognito == true`, `ApplicationLifetimeCoordinator` garantiza que `sessionSaver` jamás es ejecutado y no se escribe ningún dato en el sistema de archivos.
  3. *Persistencia atómica y resiliente*: El vaciado de sesión utiliza sustitución atómica mediante archivos temporales `.tmp` y renombrado atómico (`MoveFileExW` / `std::filesystem::rename`).
  4. *Watchdog acotado*: Todo vaciado de estado está acotado por un límite temporal estricto (`shutdownTimeoutMs`), abortando el proceso para evitar cuelgues indefinidos.
  5. *Zero-Chromium y Zero-Zombie*: Las pruebas del ciclo de vida se ejecutan sin inicializar procesos de renderizado de Chromium ni widgets gráficos de WebEngine, y recolectan limpiamente todos los hilos (`join()`).
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/LifetimeTypes.h` y `ApplicationLifetimeCoordinator.h` & `.cpp`: Tipos de ciclo de vida y coordinador desacoplado.
  - `apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`: Persistencia atómica de sesiones de navegación.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Implementación de ventana principal integrada con el coordinador.
  - `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`: Suite de pruebas unitarias existente.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Condiciones de carrera o flakiness en pruebas concurrentes.
    *Mitigación*: Uso de tipos atómicos de C++20 (`std::atomic<bool>`, `std::atomic<AppLifetimeState>`) con barreras de memoria explícitas (`memory_order_acquire`, `memory_order_release`) y sincronización determinista de hilos (`thread.join()`).
  - *Riesgo*: Pruebas de timeout lentas que degraden la velocidad de CTest.
    *Mitigación*: Uso de valores de timeout sintéticos pequeños (10-25 ms) para simular el watchdog en milisegundos, manteniendo el tiempo total de la suite por debajo de 250 ms.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 10/10 tests de CTest pasando exitosamente en 0.30s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.05s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.03s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.03s)
    9. `startup_bootstrap_tests`: Passed (0.02s)
    10. `application_lifetime_tests`: Passed (0.11s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 28 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0082-module.foundations.task0028.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006, Q-008 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0028`)**:
  - Se definieron formalmente las 11 tareas verificables en `tasks.md` con criterios precisos y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
