# Execution Memory — module-foundations-task0032

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **crash recovery policy** en el módulo `foundations` (`module.foundations.task0032`, microtarea `0032 — Probar casos positivos, negativos y recuperación: crash recovery policy`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0094`.
  - Pack: `module.foundations.task0032`.
  - Fase: `propose`.
  - Generated artifact: `0094-module.foundations.task0032.propose.md`.
  - Prompt SHA-256: `9743a86f9b656802641ec34b6f0614d3e10e06b36be4ddaa7b176764118dd1fa`.
  - Lock SHA-256: `19f3bb879098c48d03e47518080ecaaf6ad7f6ae795b24f9714e1f725479f2e8`.
  - Dependencias ejecutables: `module.foundations.task0031`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; encapsulamiento modular y tests independientes.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración deterministas en `CrashRecoveryTests.cpp`, uso de fixtures sintéticos RAII, eliminación de sleeps fijos, y cobertura exhaustiva de caminos felices, negativos y de fallo inyectado.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, tipos estándar (`<filesystem>`, `<chrono>`, `<thread>`), RAII, separación de capas y no bloqueo de hilos.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Preservación de contratos de UI existentes para el banner de recuperación tras caídas, escalado en «Interfaz más fácil de usar» (Modo Senior: botones $\ge 48$ px, contenedor $\ge 56$ px) y foco por teclado.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta de memoria mediante bounded buffers (límite de 1000 pestañas, longitud máxima de URLs de 8192 bytes) y pruebas rápidas (< 500 ms) sin lanzar procesos Chromium.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero telemetría remota o envío de reportes de fallos; aislamiento absoluto de incógnito (el centinela y las sesiones nunca tocan disco en modo privado).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación estricta con ClangFormat, compilación con `-Werror` y validación en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos OpenSpec) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición absoluta de términos de ingeniería («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas. Uso obligatorio de «Interfaz más fácil de usar».

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la lógica de detección, análisis de fallos y recuperación se ejecuta exclusivamente en el equipo local del usuario. Cero llamadas de red, telemetría o servicios cloud.
  2. *Aislamiento estricto de Incógnito*: Si `config.isIncognito == true`, ninguna operación lee o escribe archivos en disco; todas retornan éxito inmediato en memoria.
  3. *Bounded Buffers y Presupuesto de Memoria*: Máximo de 1000 pestañas recuperables y 8192 bytes por URL; esquemas restringidos a `http://`, `https://` y `vigilio://`.
  4. *Atomicidad y Limpieza de Disco*: Escritura siempre mediante archivos temporales `.tmp` seguidos de renombrado atómico (`writeSentinelAtomic`), con limpieza garantizada de archivos `.tmp` ante errores.
  5. *Resiliencia ante Fallos de E/S*: En caso de directorios de solo lectura o rutas inaccesibles, las operaciones retornan `false` de forma segura sin abortar ni arrojar excepciones no capturadas.
  6. *Pruebas Rápidas sin Chromium*: 100% de la suite de pruebas unitarias de crash recovery se ejecuta en < 500 ms sin instanciar instancias de Chromium/WebEngine.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Tipos del dominio (`CrashDetectionState`, `RecoveryAction`, `CrashSentinel`, `CrashRecoveryReport`).
  - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` & `.cpp`: Coordinador estático y servicio `CrashRecoveryService`.
  - `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`: Suite base de pruebas unitarias existente.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Suite de pruebas de interfaz de usuario.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Flakiness en pruebas de concurrencia multihilo.
    *Mitigación*: Uso de sincronización determinista (`std::thread::join`), marcas de tiempo controladas y verificación de integridad de archivo final sin depender de sleeps artificiales.
  - *Riesgo*: Problemas de portabilidad en pruebas de permisos de disco en plataformas Windows vs POSIX.
    *Mitigación*: Empleo de `std::filesystem::permissions` portable y verificación de rutas deliberadamente no existentes o inaccesibles, con fallback adecuado por plataforma.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 11/11 pruebas de CTest pasando exitosamente en 0.41s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.07s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
    8. `resource_qrc_inclusion_tests`: Passed (0.03s)
    9. `startup_bootstrap_tests`: Passed (0.06s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 32 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0094-module.foundations.task0032.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 resueltas (Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0032` en ejecución `0095`)**:
  - Se crearon y formalizaron en `openspec/changes/module-foundations-task0032/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se especificaron 10 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente lista y es apta para revisión humana previa a APPLY.
