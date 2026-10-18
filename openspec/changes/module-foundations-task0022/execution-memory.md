# Execution Memory — module-foundations-task0022

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **startup bootstrap** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0022 — Implementar la operación en una capa propietaria: startup bootstrap`), proyectando la implementación física en C++20 de la clase de servicio y orquestador desacoplado `vigilio::StartupBootstrapCoordinator`, los tipos de valor inmutables (`StartupTypes.h`), el parseo y saneamiento de línea de comandos (`StartupCommandLine`), la defensa activa contra banderas inseguras de Chromium (`--no-sandbox`, `--disable-web-security`), la defensa contra Directory Traversal en directorios de perfil (`isValidProfilePath`), el aislamiento estricto de sesiones privadas de incógnito (`isStoragePersistent = false`, `:memory:`), el modo degradado headless para entornos sin servidor gráfico, la delegación desde el punto de entrada `src/app.cpp`, y la suite de pruebas unitarias automatizadas en `tests/unit/StartupBootstrapTests.cpp`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance de `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura desacoplada frente a WebEngine y Widgets.
  - `docs/rules/01-modules/foundations/screens.md`: Estados de ciclo de vida y degradación controlada de UI.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, estados, fallos y códigos de salida.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`unit;fast`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad local, logging estructurado y recuperación.
  - `docs/rules/01-modules/foundations/micro/0022-startup-bootstrap/task.md`: Contrato vinculante de la microtarea 0022.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `2bc42c077251283a87fcda4edb3d8a249e42bc35e5a69da1b8cd22d1e136e931`
  - Lock SHA-256: `b94dfa8abf2cbdb34f31acb7d8ae4c1e2835755966b8858e42f1baf6c01e8745`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; encapsulación de la lógica de bootstrap en una capa propietaria (`StartupBootstrapCoordinator`), separada de los adaptadores de validación individual (`QtDependencyValidator`, `WebEngineVersionValidator`, `ResourceInclusionService`), y desacoplada del punto de entrada `src/app.cpp`.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: 11 casos de prueba unitarios deterministas con fixtures locales, sin esperas arbitrarias ni sleeps, aserciones claras bajo la macro `CHECK`, reporte preciso en CTest con `LABELS "unit;fast"` y timeout de 15 segundos.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `std::filesystem`, `std::optional`, `std::vector`), const-correctness estricta, semántica inmutable de tipos de valor, utilidades `constexpr`, y desacoplamiento absoluto de la lógica de dominio respecto a `QApplication` y `BrowserWindow`.
- `docs/skills/04-ux-ui-senior.md`: Estilo sobrio y profesional; manejo de mensajes de consola comprensibles y limpios sin caracteres de control espurios ni decoración innecesaria.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Respeto absoluto a la denominación pública obligatoria según `docs/UX-COPY-PUBLICO.md`: prohibición de exponer términos de ingeniería interna en textos públicos y uso obligado de «Interfaz más fácil de usar». Declaración explícita de **sin cambio visual directo** en esta etapa técnica de implementación.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia computacional y huella de memoria: evaluación completa del coordinador de arranque en < 10 ms (medición real en ~1 ms), memoria en reposo < 50 KB, y prevención de creación de procesos pesados WebEngine o auxiliares en caso de fallo temprano.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first constitucional (decisión Q-001): cero peticiones de red o telemetría durante el inicio; aislamiento estricto de sesiones de incógnito (`isStoragePersistent = false`, `:memory:`); defensa activa contra inyección de argumentos inseguros de Chromium (`--no-sandbox`, `--disable-web-security`) y contra Directory Traversal en rutas de perfil.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, compilación bajo `-Werror` con C++20 y pruebas de regresión automatizadas en CTest bajo CMake/Ninja.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la implementación y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («startup DAG», «QRC», «bootstrap», «milestone WebEngine», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».
- Referencias visuales en `resultado/`: Revisión del catálogo visual y `resultado/06-logo-instalador-oficial.png`; constatación de que no hay modificación de iconos ni de interfaz gráfica en esta etapa.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Cero conexiones de red, pings de telemetría o consultas de actualización durante la secuencia de arranque.
  2. *Aislamiento estricto de incógnito*: Si `cmdLine.isIncognito == true`, el reporte garantiza `isStoragePersistent == false` y `storagePath = ":memory:"`, prohibiendo cualquier escritura a disco de historial o cookies.
  3. *Defensa activa contra argumentos inseguros*: Se prohíben explícitamente banderas que degraden la seguridad de Chromium (`--no-sandbox`, `--disable-web-security`, `--remote-debugging-pipe`, `--allow-file-access-from-files`, `--remote-debugging-port`), provocando rechazo inmediato con código `InvalidCommandLineArgs` (exit code 10).
  4. *Saneamiento estricto de rutas de perfil*: Las rutas especificadas en `--profile-dir` deben pasar `isValidProfilePath`, bloqueando secuencias `..`, caracteres de control, metacaracteres de shell y rutas protegidas del sistema operativo.
  5. *Desacoplamiento headless*: La evaluación completa de `StartupBootstrapCoordinator::executeBootstrap` se ejecuta sin inicializar `QApplication` ni requerir servidor gráfico X11 o Wayland, degradando a modo headless de forma ordenada si no hay display.
  6. *Presupuesto de rendimiento*: Latencia de bootstrap inferior a 10 ms con medición precisa en `elapsedMicroseconds`.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/app.cpp`: Punto de entrada que orquesta el arranque de forma desacoplada.
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`: Validador de versión y entorno Qt.
  - `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `.cpp`: Validador de versión y modo degradado WebEngine.
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo canónico inmutable.
  - `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `.cpp`: Validador de arranque de recursos QRC.
  - `apps/vigilio-browser/src/storage/LocalRepository.h` y `.cpp`: Capa de persistencia SQLite local.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Acoplamiento temprano de la validación de arranque a `QApplication` o clases gráficas de Qt Widgets, impidiendo ejecutar pruebas unitarias en servidores headless de CI sin servidor X11.
    *Mitigación*: Mantener `StartupBootstrapCoordinator` estrictamente en `vigilio_core` como una clase C++20 pura desacoplada de `QApplication`, recibiendo variables de entorno y parámetros como argumentos inyectables.
  - *Riesgo*: Inyección inadvertida de parámetros inseguros de línea de comandos en Chromium embebido.
    *Mitigación*: Implementación de un filtro de argumentos estricto con lista blanca explícita de opciones válidas y lista negra de banderas de riesgo que provocan fallo inmediato (`InvalidCommandLineArgs`, exit code 10).
  - *Riesgo*: Fuga de datos de incógnito a disco en caso de arranque en modo privado.
    *Mitigación*: Forzar invariante `isStoragePersistent = false` y `:memory:` en `StartupDiagnosticReport` cada vez que `--incognito` esté activo, verificado mediante prueba unitaria automatizada.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 9/9 tests de CTest pasando exitosamente en 0.24s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.03s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.04s)
    8. `resource_qrc_inclusion_tests`: Passed (0.04s)
    9. `startup_bootstrap_tests`: Passed (0.05s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 22 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0064-module.foundations.task0022.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a fase APPLY (`module.foundations.task0022`)**:
  - Se definieron formalmente las 10 tareas verificables en `tasks.md` con comandos explícitos de CTest, grep y shell.
  - La implementación C++20 de la capa propietaria de bootstrap está completamente especificada en `proposal.md`, `design.md`, `tasks.md` y `execution-memory.md` dentro de `openspec/changes/module-foundations-task0022/`.
