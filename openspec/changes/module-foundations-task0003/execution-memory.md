# Execution Memory — module-foundations-task0003

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0003` del módulo `foundations` (`module.foundations.task0003`), correspondiente a la etapa `ui` de la capacidad **cmake targets**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0003-cmake-targets/task.md`
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
  - Prompt SHA-256: `73680733fc840ad228a68129171f36748a479f2a3fc969cefff821e9e2c50aa5`
  - Lock SHA-256: `0a2592be3d0756a0c97178cc0c00562d7c35d0e86b13fe277a6e6b0336beb180`

## Skills and required references read

Se consultaron y aplicaron al 100% como norma vinculante las siguientes skills y especificaciones del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID (Single Responsibility e Interface Segregation aplicados a la separación de targets de UI frente a core/almacenamiento), DRY en la compartición de recursos compilados (`resources.qrc`), y KISS/YAGNI en la no introducción de dependencias innecesarias.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; categorización determinista de suites en CTest (`ui;fast`, `e2e;heavy`); registro honesto de estados de ejecución (`PASS`, `FAIL`, `NOT_EXECUTED`).
- `docs/skills/03-codigo-cpp-qt.md`: Código C++20 idiomático, ownership estricto de widgets Qt mediante jerarquía de padres (`QObject` parent-child), no bloquear el hilo de interfaz bajo ninguna circunstancia, y separación estricta entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Estilo visual sobrio, sin brillos excesivos ni estética artificial; azul oscuro como base (`#141D2B`), contraste controlado y empleo exclusivo de iconos Lucide SVG sin emojis.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; navegación completa por teclado, foco visible con alto contraste, etiquetado para tecnologías asistivas (`setAccessibleName`), y «Interfaz más fácil de usar» con escalado tipográfico a 14pt y controles con tap targets mínimos de 48px. Documentación obligatoria de ausencia de impacto visual directo para la capacidad `cmake targets`.
- `docs/skills/06-rendimiento-y-ram.md`: Carga diferida de pestañas restauradas (`deferredUrl`) para evitar instanciación masiva de WebEngine en arranque; contención de memoria RAM y serialización de pruebas gráficas (`RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`).
- `docs/skills/07-seguridad-y-privacidad.md`: Modelo local-first estricto; prohibición de cuentas de usuario, nube o sincronización de Vigilio; aislamiento de datos en SQLite local y exportación controlada sin exponer contraseñas ni cookies.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico riguroso de compilación; registros locales sin telemetría remota; evidencia de comandos y exit codes.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance; APPLY implementa únicamente el incremento del pack; REVIEW verifica de forma independiente).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado nativo (`WIN32_EXECUTABLE`, `MACOSX_BUNDLE`) e instalación estándar.
- `docs/skills/11-assets-svg-quality.md`: Integración de 36 recursos vectoriales SVG con viewBox `0 0 24 24`, trazo uniforme y referencias en `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Norma vinculante de lenguaje de producto público; prohibición absoluta de nombres de ingeniería («Senior Mode», «Modo Senior», «cmake», «targets», «DAG», «Prompt Builder», «OpenSpec»); uso exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de ventana (tabs 34-40dp, toolbar 44-48dp), estados de controles y accesibilidad WCAG 2.2 AA.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Reglas de gobernanza de la fase PROPOSE, verificación reproducible y preservación de restricciones de scope.
- `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`: Referencias visuales de la interfaz de usuario objetivo y del panel de accesibilidad con «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_core` y `vigilio_storage` son completamente independientes de bibliotecas de interfaz gráfica (cero dependencias de Qt Widgets o WebEngine).
  2. El target `vigilio-browser` compila exclusivamente bajo la guarda condicional `VIGILIO_BUILD_GUI=ON` y la detección exitosa de Qt6 (`if(Qt6_FOUND)`).
  3. Soporte headless garantizado con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) para compilación y paso íntegro de pruebas en entornos de CI sin interfaz gráfica.
  4. Ningún término técnico de ingeniería o sistema de construcción se expone en la interfaz de usuario pública; la capacidad `cmake targets` no introduce controles rotulados con su nombre en la GUI.
  5. Contención de memoria: sólo una instancia activa de `QWebEngineView` al restaurar sesión; pestañas secundarias diferidas.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
  - `apps/vigilio-browser/resources.qrc`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de paquetes de desarrollo de Qt6 en entornos ligeros de CI.
    *Mitigación*: Mantener la verificación condicional `find_package(Qt6 6.10 QUIET ...)` con mensaje de advertencia y registro de estado `NOT_EXECUTED` para pruebas de GUI cuando falten dependencias Qt, sin romper la compilación headless.
  - *Riesgo*: Fugas de memoria o acumulación de procesos huérfanos de WebEngine en pruebas automatizadas.
    *Mitigación*: Gobernanza estricta en CMake con `RUN_SERIAL TRUE` y `RESOURCE_LOCK browser_e2e`, limitando la ejecución concurrente y garantizando la liberación de perfiles temporales.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 3, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0007-module.foundations.task0003.propose.lock.json`
  - Salida: `{"lockfile": "...", "valid": true, "checks": [...]}`
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (solo bloquea `release.public-release`).
- **Verificación de suite headless**:
  - Comando: `ctest --preset core-debug`
  - Salida: 2/2 tests pasados (`core_units` y `storage_integration`) en 0.01s.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0003/proposal.md`: redactado y completado con el alcance de la etapa `ui`.
  - `openspec/changes/module-foundations-task0003/design.md`: redactado y completado con arquitectura de targets de UI, recursos compilados y accesibilidad.
  - `openspec/changes/module-foundations-task0003/tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-005.
  - `openspec/changes/module-foundations-task0003/execution-memory.md`: redactado y completado.
- **Handoff**:
  - La propuesta está completamente elaborada, coherente y lista para la revisión y aprobación humana antes de proceder a la fase APPLY de `module.foundations.task0003`.
