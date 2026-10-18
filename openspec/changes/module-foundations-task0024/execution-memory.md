# Execution Memory — module-foundations-task0024

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa, rigurosa y formal para la capacidad `startup bootstrap` en su etapa `tests` dentro del módulo propietario `foundations` de Vigilio Browser (microtarea `0024 — Probar casos positivos, negativos y recuperación: startup bootstrap`). El alcance define la construcción y certificación de la suite de pruebas automatizadas deterministas en C++20 (`apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`), cubriendo casos normales (StandardGui con URLs, Headless explícito, SafeMode, Incognito), entradas inválidas (flags prohibidos de Chromium, Directory Traversal, metacaracteres shell, rutas de sistema protegidas), inyección de fallos y recuperación (display ausente con degradación controlada a `DegradedHeadless`, Qt incompatible, recursos QRC corruptos, almacenamiento denegado), estrés concurrente multihilo (16 hilos × 500 iteraciones = 8,000 ciclos), presupuestos de latencia (< 50 ms) y garantías de aislamiento de datos en memoria, preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0024-startup-bootstrap/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64)
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `fc9dfea78042864de7094a4d9f9af3e81cd6e9b66f307a4fefea8013d9b0ddde`
  - Lock SHA-256: `ff70172bd9bcb13dd4acf635f34f115358772ea0600ce317459afa14d78140dc`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY en la infraestructura de testing; desacoplamiento estricto entre la lógica de prueba y la presentación de interfaz.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; enfoque en pruebas unitarias y de estrés deterministas, reproducibles, sin sleeps fijos y con aislamiento estricto de fixtures.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, punteros inteligentes, funciones puras, const-correctness), evitando operaciones bloqueantes y asegurando compilación estricta con `-Werror`.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en tiempo de inicio: validación de presupuestos de latencia en la secuencia de bootstrap (< 50 ms por ciclo) y ausencia de sobrecargas de RAM.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first (decisión Q-001): aislamiento de sesiones de incógnito en memoria (`":memory:"`), cero escrituras en disco durante navegación privada y cero telemetría remota.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, linters y comprobaciones de CTest continuas.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la suite de pruebas) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «bootstrap», «CLI», «DAG», «OpenSpec») en los mensajes diagnósticos o de usuario.
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Coherencia visual, atajos y accesibilidad asistiva.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales del producto final.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Aislamiento estricto en modo incógnito*: Cuando `cmdLine.isIncognito` es verdadero, la base de datos `LocalRepository` opera exclusivamente en memoria (`":memory:"`), sin escribir historial, cookies ni datos de navegación en disco.
  2. *Bloqueo de banderas prohibidas*: Argumentos como `--no-sandbox`, `--disable-web-security`, `--remote-debugging-pipe`, `--allow-file-access-from-files` y `--remote-debugging-port` son rechazados incondicionalmente con código `InvalidCommandLineArgs` (10).
  3. *Defensa contra Directory Traversal*: Toda ruta de perfil que contenga `..`, caracteres de control, metacaracteres shell o rutas protegidas del sistema operativo es invalidada antes de acceder al disco.
  4. *Degradación predecible*: En ausencia de servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`), la aplicación no aborta abruptamente con crash, sino que degrada a `DegradedHeadless` con exit code 1.
  5. *Política Zero-Zombie*: Ninguna prueba unitaria crea subprocesos Chromium reales (`QtWebEngineProcess`), ni abre conexiones de red, ni deja procesos o archivos colgados.
  6. *Thread-Safety absoluta*: Cero variables mutables globales o estáticas; 16 hilos concurrentes ejecutan ciclos de bootstrap sin condiciones de carrera.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/StartupBootstrapCoordinator.h` y `.cpp`: Coordinador desacoplado de bootstrap.
  - `apps/vigilio-browser/src/core/StartupTypes.h`: Entidades inmutables (`StartupCommandLine`, `StartupDiagnosticReport`, enums).
  - `apps/vigilio-browser/tests/unit/StartupBootstrapTests.cpp`: Arnés de pruebas unitarias y de estrés.
  - `apps/vigilio-browser/CMakeLists.txt`: Configuración de compilación CMake y registro de tests en CTest.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de integración y verificación.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Falsos positivos o caídas en entornos de CI sin display disponible.
    *Mitigación*: Las pruebas evalúan variables de display nulas y verifican la degradación a `DegradedHeadless`, garantizando ejecución 100% determinista en headless CI.
  - *Riesgo*: Conflictos en el sistema de archivos por carpetas temporales compartidas en `/tmp`.
    *Mitigación*: Cada caso de prueba genera rutas únicas con prefijo determinista y realiza limpieza exhaustiva antes y después de su ejecución.
  - *Riesgo*: Condiciones de carrera o bloqueos en la prueba de estrés concurrente.
    *Mitigación*: Se utiliza diseño puramente funcional con estructuras inmutables y variables atómicas para la contabilización de resultados.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Pipeline de compilación y pruebas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (9/9 tests pasados en 0.25s, exit code 0):
    - `core_units`: Passed (0.00s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.07s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.04s)
    - `resource_qrc_catalog_tests`: Passed (0.01s)
    - `resource_inclusion_service_tests`: Passed (0.03s)
    - `resource_qrc_inclusion_tests`: Passed (0.04s)
    - `startup_bootstrap_tests`: Passed (0.06s)
  - Validación de formato: Clang-Format verificado sin advertencias ni modificaciones pendientes.
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 24/24 referencias con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0070-module.foundations.task0024.propose.lock.json --project-root .` superó 13/13 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que no existen decisiones abiertas que bloqueen foundations ni startup bootstrap (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo aplica al hito final de lanzamiento público).
- **Handoff**:
  - La propuesta técnica para la etapa `tests` de `module.foundations.task0024` queda formalizada, coherente y lista en `openspec/changes/module-foundations-task0024/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY (`module.foundations.task0024.apply`).
