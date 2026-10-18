# Execution Memory — module-foundations-task0027

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **ui** de la capacidad **application lifetime** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0027 — Implementar la interacción y estados visuales en Qt: application lifetime`), proyectando la integración física en Qt Widgets dentro de `BrowserWindow` con `ApplicationLifetimeCoordinator`, la acción global `quitAction_` con atajo `QKeySequence::Quit` (`Ctrl+Q`), el diálogo accesible de confirmación ante múltiples pestañas abiertas, el respeto absoluto a la denominación pública obligatoria según `docs/UX-COPY-PUBLICO.md` («Interfaz más fácil de usar»), el aislamiento estricto de sesiones de incógnito (Q-001) y la suite de pruebas de interfaz en CTest (`WindowTests.cpp`), sin acoplar lógica no visual a los widgets.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance de `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`fast`, `ui`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0027-application-lifetime/task.md`: Contrato vinculante de la microtarea 0027.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 UI nativa Qt Widgets, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Ejecución: `0079`
  - Pack: `module.foundations.task0027`
  - Fase: `propose`
  - Prompt SHA-256: `6eab1f529c83aa0dfe743db9bb6bf869aa56a5965f533922a8decc73438cbb82`
  - Lock SHA-256: `306034f6db560f95ef1e6905ee8919ee51fac11e6d066ced30547f060d896819`
  - Dependencias ejecutables: `module.foundations.task0026`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre la capa de presentación Qt (`BrowserWindow`) y la orquestación atómica del ciclo de vida (`ApplicationLifetimeCoordinator`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas deterministas de UI; verificación de estados de interfaz, navegación por teclado, y manejo determinista de `NOT_EXECUTED` ante la ausencia de display gráfico.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::unique_ptr`, `std::function`), ownership RAII riguroso en Qt, y no bloqueo del hilo de interfaz durante el apagado.
- `docs/skills/04-ux-ui-senior.md`: Estética sobria y profesional; tokens de color en temas claro y oscuro; diálogos modales limpios sin adornos artificiales ni look de IA genérica.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Accesibilidad universal WCAG AAA; escalado de botones a >= 48 px y tipografía a 14 pt en la «Interfaz más fácil de usar»; foco visible y navegación fluida con `Tab`, `Enter` y `Esc`.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en apagado; límite de tiempo estricto de 3000 ms garantizado por el watchdog para evitar bloqueos del proceso.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero peticiones de red o telemetría durante la salida; aislamiento total de incógnito (cero escrituras en disco).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto Clang-Format, compilación bajo C++20 con `-Werror`, y validación automatizada en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la interacción y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («application lifetime», «graceful teardown», «kill signal», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».
- Referencias visuales en `/resultado/`: Coherencia visual con `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Cero conexiones de red, pings de telemetría o consultas de actualización durante el ciclo de vida o salida de la aplicación.
  2. *Aislamiento estricto de incógnito*: Si `isIncognito_ == true`, `BrowserWindow` y `ApplicationLifetimeCoordinator` garantizan que no se escriba ningún dato en disco ni se desplieguen diálogos de persistencia de sesión al salir.
  3. *Persistencia atómica y resiliente*: El vaciado de sesión se delega en `saveSession` y `SessionFile.h`, garantizando integridad de datos y reemplazo seguro.
  4. *Cierre determinista orquestado*: `BrowserWindow::closeEvent` no destruye la ventana sin invocar primero `lifetimeCoordinator_->requestShutdown()` y `executeShutdown()`.
  5. *Soporte integral de accesibilidad*: En la «Interfaz más fácil de usar», los diálogos escalan controles a >= 48 px y fuentes a >= 14 pt, con etiquetas descriptivas para lectores de pantalla.
  6. *Detección determinista de entorno gráfico*: En entornos headless o sin servidor X11/Wayland, las pruebas de interfaz registran limpiamente `NOT_EXECUTED` sin fallar espuriamente el pipeline.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/LifetimeTypes.h` y `ApplicationLifetimeCoordinator.h` & `.cpp`: Tipos de ciclo de vida y coordinador desacoplado.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Implementación actual de ventana principal, gestión de pestañas y estilos.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Suite de pruebas de interfaz de usuario en Qt.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Congelamiento de la interfaz si el vaciado de estado se bloquea en disco durante `closeEvent`.
    *Mitigación*: La persistencia es atómica y local sobre JSON compacto; el temporizador de 3000 ms del coordinador aborta de emergencia si se excede el tiempo límite.
  - *Riesgo*: Cierre accidental de múltiples pestañas de trabajo por un clic involuntario en `[X]`.
    *Mitigación*: Diálogo modal accesible `confirmCloseMultipleTabs` que previene el cierre no intencionado, cancelable de forma inmediata con la tecla `Esc`.
  - *Riesgo*: Violación de privacidad en sesiones de incógnito al salir.
    *Mitigación*: En modo incógnito, se omite taxativamente `saveSession` y se destruye la base de datos `:memory:` por RAII sin tocar el disco.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 10/10 tests de CTest pasando exitosamente en 0.45s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.08s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.02s)
    7. `resource_inclusion_service_tests`: Passed (0.05s)
    8. `resource_qrc_inclusion_tests`: Passed (0.04s)
    9. `startup_bootstrap_tests`: Passed (0.08s)
    10. `application_lifetime_tests`: Passed (0.12s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 27 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0079-module.foundations.task0027.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirmó que todas las decisiones de foundations están resueltas (Q-001 a Q-006, Q-008 resueltas; Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0027`)**:
  - Se definieron formalmente las 11 tareas verificables en `tasks.md` con criterios precisos y comandos ejecutables.
  - La propuesta técnica integral está completamente redactada y validada para revisión humana.
