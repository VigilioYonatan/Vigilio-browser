# Execution Memory — module-foundations-task0021

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **domain** de la capacidad **startup bootstrap** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0021 — Definir el comportamiento y las invariantes: startup bootstrap`), especificando la máquina de estados de ciclo de vida con 11 fases discretas, los tipos de valor inmutables (`StartupTypes.h`), el parseo y saneamiento de argumentos de línea de comandos (`StartupCommandLine`), la defensa activa contra banderas inseguras de Chromium (`--no-sandbox`), la resolución y saneamiento de rutas de perfiles locales contra Directory Traversal, el aislamiento estricto de sesiones efímeras de incógnito (`isStoragePersistent = false`), y el coordinador de dominio desacoplado (`StartupBootstrapCoordinator`), bajo compilación C++20 estricta y ejecución 100% headless sin requerir servidor gráfico ni procesos Chromium/WebEngine.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance de `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Estados visuales y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`fast`, `unit`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0021-startup-bootstrap/task.md`: Contrato vinculante de la microtarea 0021.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `4d1d62dd26115fb78ee507fd862cd05e0156382f8b71a1ca59c389263836b66f`
  - Lock SHA-256: `e109d878ac081ccc347f9660acc9d4b37ade6ede5eb64eb653870d5674f13722`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre el coordinador de arranque desacoplado (`StartupBootstrapCoordinator`), los validadores de subsistema (`QtDependencyValidator`, `WebEngineVersionValidator`, `ResourceInclusionService`), y el punto de entrada de la aplicación (`src/app.cpp`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas centrada en pruebas unitarias deterministas y de integración interna, con fixtures locales, sin sleeps arbitrarios, aserciones claras y clasificación explícita en CTest con reporte preciso de `PASS`, `FAIL` o `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `std::filesystem`, `std::optional`, `std::vector`), RAII para flujos de datos, const-correctness estricta, y desacoplamiento absoluto de la lógica de dominio respecto a `QApplication` y widgets de UI.
- `docs/skills/04-ux-ui-senior.md`: Protección de la identidad visual sobria y profesional; diseño de transiciones y degradaciones limpias sin estados intermitentes ni bloqueos de pantalla.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Respeto absoluto a la denominación pública obligatoria según `docs/UX-COPY-PUBLICO.md`: prohibición de exponer términos de ingeniería en la UI y uso obligado de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia computacional y huella de memoria: evaluación completa del coordinador de arranque en < 10 ms, huella de memoria en reposo < 50 KB, y prevención activa de spawn de procesos auxiliares o zombis en caso de fallo temprano.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first constitucional (decisión Q-001): cero peticiones de red o telemetría durante el inicio; aislamiento estricto de sesiones de incógnito (`isStoragePersistent = false`); defensa activa contra inyección de argumentos inseguros de Chromium (`--no-sandbox`) y contra Directory Traversal en rutas de perfil.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, compilación con `-Werror` bajo C++20 y pruebas de regresión automatizadas en CTest bajo CMake/Ninja.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la arquitectura de dominio y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («startup DAG», «QRC», «bootstrap», «milestone WebEngine», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (Q-001)*: Cero conexiones de red, pings de telemetría o consultas de actualización durante la secuencia de arranque.
  2. *Aislamiento estricto de incógnito*: Si `cmdLine.isIncognito == true`, el reporte resultante garantiza `isStoragePersistent == false`, forzando almacenamiento efímero en memoria y prohibiendo escrituras en `SessionFile` o base de datos de historial SQLite.
  3. *Defensa activa contra argumentos inseguros*: Se prohíbe explícitamente el paso de argumentos que degraden la seguridad de Chromium (`--no-sandbox`, `--disable-web-security`, `--remote-debugging-port`), provocando rechazo inmediato con código `InvalidCommandLineArgs` (exit code 10).
  4. *Saneamiento estricto de rutas de perfil*: Las rutas especificadas en `--profile-dir` deben pasar `isValidProfilePath`, bloqueando secuencias `../`, caracteres de control y directorios protegidos del sistema operativo.
  5. *Desacoplamiento headless y presupuesto de rendimiento*: La evaluación completa de `StartupBootstrapCoordinator::executeBootstrap` se ejecuta en menos de 10 ms en pruebas locales sin inicializar `QApplication` ni requerir servidor gráfico X11 o Wayland.
  6. *Determinismo en la máquina de estados*: Las 11 fases de `StartupPhase` se ejecutan en orden estricto, abortando inmediatamente en la primera fase que no satisfaga sus precondiciones.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/app.cpp`: Punto de entrada actual con validaciones preliminares.
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`: Validador de versión y entorno Qt.
  - `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `.cpp`: Validador de versión y modo degradado WebEngine.
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo canónico inmutable.
  - `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `.cpp`: Validador de arranque de recursos QRC.
  - `apps/vigilio-browser/src/storage/LocalRepository.h` y `.cpp`: Almacenamiento SQLite local.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Acoplamiento temprano de la validación de arranque a `QApplication` o clases gráficas de Qt Widgets, impidiendo ejecutar pruebas unitarias en servidores headless de CI sin servidor X11.
    *Mitigación*: Mantener `StartupBootstrapCoordinator` estrictamente en `vigilio_core` como una clase C++20 pura desacoplada de `QApplication`, recibiendo variables de entorno y parámetros como argumentos inyectables.
  - *Riesgo*: Inyección inadvertida de parámetros inseguros de línea de comandos en Chromium embebido.
    *Mitigación*: Implementación de un filtro de argumentos estricto con lista blanca explícita de opciones válidas y lista negra de banderas de riesgo que provocan fallo inmediato (`InvalidCommandLineArgs`).
  - *Riesgo*: Fuga de datos de incógnito a disco en caso de arranque en modo privado.
    *Mitigación*: Forzar invariante `isStoragePersistent = false` en `StartupDiagnosticReport` cada vez que `--incognito` esté activo, impidiendo la inicialización de archivos de sesión en disco.

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - 8/8 tests de CTest pasando exitosamente en 0.14s (exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.04s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.03s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.02s)
    8. `resource_qrc_inclusion_tests`: Passed (0.02s)
- **Validación documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 21 referencias validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0061-module.foundations.task0021.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a fase APPLY (`module.foundations.task0022`)**:
  - Se definieron formalmente las 10 tareas verificables en `tasks.md` con comandos explícitos de CTest y shell.
  - Los tipos de datos de valor y la interfaz del coordinador están listos para su codificación limpia en C++20 dentro de `vigilio_core` sin alterar las garantías de calidad y arquitectura existentes.
