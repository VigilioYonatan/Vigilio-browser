# Execution Memory — module-foundations-task0031

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **ui** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0031 — Implementar la interacción y estados visuales en Qt: crash recovery policy`), proyectando el banner de recuperación accesible en Qt Widgets (`vigilio-crash-recovery-banner`), sus acciones (`Restaurar pestañas`, `Empezar de cero`, `Cerrar aviso`), la navegación completa por teclado, la integración con la «Interfaz más fácil de usar» (Modo Senior), la carga diferida de pestañas restauradas para eficiencia de RAM y la orquestación del latido y apagado limpio sin bloquear la UI ni violar el aislamiento de incógnito.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance del módulo `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura, contratos y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Pantallas, jerarquía visual y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`, `ui`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad, métricas y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0031-crash-recovery-policy/task.md`: Contrato vinculante de la microtarea 0031.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`.
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`.
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube ni login, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-003 UI nativa Qt Widgets, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`.
  - Ejecución actual: `0091`.
  - Pack: `module.foundations.task0031`.
  - Fase: `propose`.
  - Generated artifact: `0091-module.foundations.task0031.propose.md`.
  - Prompt SHA-256: `7d61b63c9d83ac36399354e9185e8b43e7f2eb8b34f5e35820cc366a0610ec02`.
  - Lock SHA-256: `255ffca17e1c4ed5c9331385131aa50cf6385335b1d013fc4c19e9e642a99866`.
  - Dependencias ejecutables: `module.foundations.task0030`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre el servicio de recuperación en `core` y la presentación visual en `BrowserWindow`.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas de interfaz Qt deterministas en `WindowTests.cpp`, verificando estados, navegación por teclado y accesibilidad.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno, const-correctness, propiedad clara de widgets y objetos en Qt (`parent-child`), no bloqueo del hilo de UI mediante operaciones asíncronas y temporizadores.
- `docs/skills/04-ux-ui-senior.md`: Estilo moderno, sobrio, azul oscuro base, superficies limpias, iconos SVG consistentes de Lucide (`icons/tools/shield.svg`, `icons/navigation/reload.svg`), sin emojis y jerarquía visual evidente.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Accesibilidad base (foco visible con anillo de 2 px, contraste adecuado, etiquetas descriptivas para lector de pantalla) y «Interfaz más fácil de usar» (Modo Senior: botones $\ge 48$ px, contenedor $\ge 56$ px, iconos $28\times 28$, espaciado ampliado).
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de RAM mediante carga diferida de pestañas (lazy loading); solo la primera pestaña asigna un proceso WebEngine, mientras que las restantes permanecen como marcadores ligeros hasta ser activadas por el usuario.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-first constitucional (decisión Q-001): cero telemetría remota o envío de reportes de fallos; aislamiento absoluto de incógnito (el banner nunca se muestra ni se guardan datos de navegación privada).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación estricta con ClangFormat, compilación con `-Werror` y validación en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad y contratos OpenSpec) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/skills/11-assets-svg-quality.md`: Calidad vectorial de iconos SVG de Lucide, viewBox $0\ 0\ 24\ 24$, renderizado limpio en tamaños estándar (20 px) y Senior (28 px).
- `docs/UX-COPY-PUBLICO.md`: Prohibición absoluta de términos de ingeniería («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas. Uso obligatorio de «Interfaz más fácil de usar».
- Referencias visuales en `/resultado/`:
  - `resultado/01-vigilio-home.png`: Layout general de ventana, barra de navegación, pestañas y tarjetas.
  - `resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar, tipografía escalada y controles amplios.
  - `resultado/06-logo-instalador-oficial.png`: Logotipo oficial.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Toda la detección e interacción de recuperación se realiza exclusivamente en el equipo local del usuario. Cero llamadas de red, telemetría o servicios cloud.
  2. *Aislamiento estricto de Incógnito*: Si `isIncognito_ == true`, el banner de crash recovery nunca se construye ni se muestra en pantalla. Cero persistencia en disco.
  3. *Protección contra Crash Loops*: Ante $\ge 2$ caídas consecutivas, el texto del banner alerta sobre la estabilidad y prioriza el botón «Empezar de cero» sobre la restauración.
  4. *Eficiencia de RAM por carga diferida*: Las pestañas restauradas se crean como marcadores ligeros diferidos (`deferredUrl`), evitando instanciar múltiples visores Chromium en paralelo.
  5. *Accesibilidad universal*: Toda acción interactiva posee `accessibleName`, `accessibleDescription`, atajos de teclado (`Escape` para cerrar aviso) y foco visible.
  6. *No bloqueo de UI*: Toda operación de recuperación y latido se ejecuta de forma asíncrona o con tiempos de ejecución menores a 1 ms.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Definiciones de `CrashDetectionState`, `RecoveryAction`, `RecoveryServiceState`, `CrashRecoveryReport`.
  - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` & `.cpp`: Servicio `CrashRecoveryService` y métodos de persistencia atómica.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` & `.cpp`: Ventana principal, métodos de banner de modo seguro (`setupSafeModeBanner`), gestión de pestañas diferidas (`activateDeferredTab`) y escalado de Modo Senior (`setSeniorMode`).
  - `apps/vigilio-browser/resources.qrc`: Iconos vectoriales de Lucide disponibles (`icons/tools/shield.svg`, `icons/navigation/reload.svg`).
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Suite de pruebas de UI para BrowserWindow.
  - `apps/vigilio-browser/scripts/check.sh`: Script de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Apertura simultánea de docenas de pestañas recuperadas provocando bloqueo o colapso de memoria del sistema.
    *Mitigación*: Implementación estricta de carga diferida (lazy loading): solo la primera pestaña instanciará WebEngine.
  - *Riesgo*: Confusión de usuario al coincidir banner de modo seguro y banner de recuperación tras caída.
    *Mitigación*: Jerarquía vertical ordenada y clara en `centralLayout`: modo seguro arriba (índice 0), recuperación debajo (índice 1), con estilos diferenciados y atajos independientes.
  - *Riesgo*: Fuga involuntaria de pestañas privadas en sesiones normales tras un crash.
    *Mitigación*: Validación arquitectónica garantizada en `CrashRecoveryService`: las sesiones de incógnito jamás escriben en disco, impidiendo cualquier rastro recuperable.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 11/11 pruebas de CTest pasando exitosamente en 0.50s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.14s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.05s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.04s)
    8. `resource_qrc_inclusion_tests`: Passed (0.06s)
    9. `startup_bootstrap_tests`: Passed (0.04s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 31 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0091-module.foundations.task0031.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 resueltas (Q-007 solo bloquea `release.public-release`).
- **Handoff a fase APPLY (`module.foundations.task0031` en ejecución `0092`)**:
  - Se crearon y formalizaron en `openspec/changes/module-foundations-task0031/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se especificaron 11 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente lista y es apta para revisión humana previa a APPLY.
