# Execution Memory — module-foundations-task0004

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0004` del módulo `foundations` (`module.foundations.task0004`), correspondiente a la etapa `tests` de la capacidad **cmake targets**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0004-cmake-targets/task.md`
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/00-global/constitution.md`
  - `docs/rules/00-global/delivery.md`
  - `docs/rules/00-global/error-handling.md`
  - `docs/rules/00-global/observability.md`
  - `docs/rules/00-global/security.md`
  - `docs/rules/00-global/testing.md`
  - `docs/rules/00-global/ux-ui.md`
  - `docs/rules/answers.md`
- **Identificadores criptográficos del prompt**:
  - Ejecución actual: `0010`
  - Pack: `module.foundations.task0004`
  - Fase: `propose`
  - Generated artifact: `0010-module.foundations.task0004.propose.md`
  - Prompt SHA-256: `88770cf7760b3ef5684d7f67af073b921fd14dbaf56ff9c97a14f122eed74052`
  - Lock SHA-256: `7d1232428fef1156acc69dfce019e33defd893b30490c61f2294aae5af2e7438`
  - Dependencias ejecutables: `module.foundations.task0003`

## Skills and required references read

Se consultaron y aplicaron al 100% como norma vinculante las siguientes directrices y habilidades del proyecto:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID (Single Responsibility e Interface Segregation aplicados a targets de prueba específicos para cada capa), DRY en la reutilización de macros y fixtures de prueba, y KISS/YAGNI en el diseño minimalista de casos de prueba sin sobreingeniería.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para contratos de core, integración para SQLite y persistencia, E2E pesado serializado bajo demanda); pruebas negativas obligatorias; cero esperas con sleeps fijos; registro honesto de estados (`PASS`, `FAIL`, `NOT_EXECUTED`).
- `docs/skills/03-codigo-cpp-qt.md`: Código idiomático C++20 con RAII estricto para limpieza de archivos temporales de prueba, const-correctness, no bloquear el hilo de interfaz bajo ninguna circunstancia, y separación física de responsabilidades entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Estilo sobrio, fondos limpios con contraste controlado, iconografía vectorial Lucide SVG sin emojis decorativos y estricto apego al lenguaje de producto público.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal, navegación completa por teclado, nombres accesibles explícitos (`setAccessibleName`), y «Interfaz más fácil de usar» con escalado de tipografía y controles de 48px mínimos. Declaración explícita de ausencia de cambio visual directo para la capacidad `cmake targets`.
- `docs/skills/06-rendimiento-y-ram.md`: Ejecución serial de pruebas pesadas de navegador (`RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`), contención de RAM por debajo de 50MB en suites de consola, y prevención estricta de fugas de memoria o procesos huérfanos.
- `docs/skills/07-seguridad-y-privacidad.md`: Modelo local-first estricto, sin nube ni telemetría remota obligatoria, rechazo estricto de esquemas no permitidos (`javascript:`, `file:///`, credenciales embebidas) y resistencia a inyecciones SQL en persistencia local.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato verificado mediante ClangFormat, preparación para Clang-Tidy, registros de logs locales por categoría sin exponer datos confidenciales, y trazabilidad de comandos con códigos de salida.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de responsabilidades entre fases (PROPOSE diseña la solución completa del incremento sin escribir código de aplicación; APPLY ejecuta el incremento atómico; REVIEW verifica de forma independiente).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado nativo (`WIN32_EXECUTABLE`, `MACOSX_BUNDLE`) y directrices de instalación estándar.
- `docs/skills/11-assets-svg-quality.md`: Integración y verificación de recursos embebidos mediante `resources.qrc`, asegurando que `rcc` detecte cualquier discrepancia de assets en tiempo de compilación.
- `docs/UX-COPY-PUBLICO.md`: Prohibición absoluta de exhibir términos de ingeniería interna (`cmake`, `targets`, `Senior Mode`, `DAG`, `Prompt Builder`, `OpenSpec`) en la interfaz visible; uso exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de ventana, controles de navegación y estados accesibles WCAG 2.2 AA.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales conceptuales de la interfaz de usuario objetivo y del panel de accesibilidad.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. El grafo de dependencias entre targets en CMake es estrictamente unidireccional y acíclico (`vigilio_core` -> `vigilio_storage` -> `vigilio-browser`). `vigilio_core` desconoce por completo la existencia de Qt Widgets y SQLite.
  2. Compatibilidad headless absoluta: el preset `core-debug` (`VIGILIO_BUILD_GUI=OFF`) compila y ejecuta el 100% de las pruebas unitarias y de integración en entornos de CI sin display X11/Wayland.
  3. Gobernanza de pruebas en CTest: las pruebas rápidas de consola (`unit;fast`, `integration;fast`) son altamente paralelizables, mientras que las pruebas gráficas pesadas (`e2e;heavy`) están estrictamente serializadas mediante `RUN_SERIAL TRUE` y `RESOURCE_LOCK browser_e2e`.
  4. Ausencia de retardos ciegos (`sleep`): toda sincronización temporal se basa en eventos observables o condiciones deterministas con timeouts explícitos (15s para unit/integration, 20s para UI, 60s para E2E).
  5. Limpieza garantizada mediante RAII: todos los fixtures temporales (`.sqlite`, `-wal`, `-shm`, `.txt`) creados durante la ejecución de las pruebas se eliminan de forma segura al finalizar.
  6. Cumplimiento de producto público: la capacidad `cmake targets` no introduce ningún botón ni diálogo en la interfaz pública de usuario.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp`
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de display gráfico en máquinas de integración continua que ejecutan pruebas de interfaz.
    *Mitigación*: Mantener las suites de core y almacenamiento desacopladas de componentes gráficos para que corran en cualquier runner headless; registrar `NOT_EXECUTED` honesto si falta el entorno para pruebas de GUI.
  - *Riesgo*: Procesos zombies o subprocesos de Chromium colgados tras la interrupción inesperada de un test E2E.
    *Mitigación*: Gobernanza estricta en CMake con `TIMEOUT 60` y serialización forzada `RUN_SERIAL TRUE`, junto con limpieza de perfiles temporales mediante `QTemporaryDir`.
  - *Riesgo*: Regresiones no detectadas en targets existentes al refinar casos de prueba.
    *Mitigación*: Validación cruzada continua mediante `apps/vigilio-browser/scripts/check.sh` verificando que todas las aserciones preexistentes continúen en estado `PASS`.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 4, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad del lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0010-module.foundations.task0004.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0010-module.foundations.task0004.propose.lock.json", "valid": true, "checks": [...]}`
  - Código de salida: `0`
- **Validación de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (afecta exclusivamente el hito final de publicación `release.public-release`).
- **Verificación de suite headless existente**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: 100% de tests aprobados (`core_units` y `storage_integration`) en 0.01s con Ninja; formato verificado.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0004/canonical-reference.yaml`: válido y verificado.
  - `openspec/changes/module-foundations-task0004/proposal.md`: completado formalmente con el alcance exhaustivo de la etapa `tests`.
  - `openspec/changes/module-foundations-task0004/design.md`: completado formalmente con la arquitectura de pruebas, matriz de trazabilidad y gobernanza de procesos.
  - `openspec/changes/module-foundations-task0004/tasks.md`: completado formalmente con tareas atómicas y verificables TASK-001 a TASK-005.
  - `openspec/changes/module-foundations-task0004/execution-memory.md`: completado formalmente con registro de autoridad, skills, evidencias y handoff.
- **Handoff**:
  - La propuesta para la etapa `tests` de `cmake targets` está completamente elaborada, alineada con la autoridad canónica y lista para revisión y aprobación humana antes de proceder a la fase APPLY de `module.foundations.task0004`.
