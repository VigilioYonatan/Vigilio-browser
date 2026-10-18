# Execution Memory — module-foundations-task0007

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0007` del módulo `foundations` (`module.foundations.task0007`), correspondiente a la etapa `ui` de la capacidad **compiler warnings**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0007-compiler-warnings/task.md`
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
  - Secuencia de ejecución: `0019`
  - Pack: `module.foundations.task0007`
  - Phase: `propose`
  - Generated artifact: `0019-module.foundations.task0007.propose.md`
  - Prompt SHA-256: `16b503ffc9165602e967cc144f22ec78a4593382c883cfa47f73bdac4c23a938`
  - Lock SHA-256: `b9edb29ee6911384221cbdb03ec90b4331f9055d3438dfdaefef7977df00a62b`
  - Prompt Builder version: `0.6.3`

## Skills and required references read

Se consultaron y aplicaron al 100% como norma vinculante las siguientes skills y especificaciones del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID aplicados a la separación de responsabilidades entre el sistema de advertencias y los widgets de UI; DRY en la centralización de flags de compilador; KISS y YAGNI en no inventar controles de interfaz inexistentes para capacidades de compilación.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; diferenciación entre suites headless rápidas (`unit;fast`, `integration;fast`), pruebas de interfaz (`ui;fast`) y pruebas de humo E2E seriales (`e2e;heavy`); registro honesto de `PASS`, `FAIL`, y `NOT_EXECUTED` (cuando falten paquetes de desarrollo de Qt6 en el entorno).
- `docs/skills/03-codigo-cpp-qt.md`: Código C++20 idiomático, ownership claro de objetos mediante jerarquía padre-hijo de `QObject`, no bloquear el hilo principal de la UI, y separación estricta entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Estilo visual sobrio, sin brillos exagerados ni apariencias artificiales; azul oscuro como base (`#141D2B`), contraste controlado y uso exclusivo de iconos Lucide SVG sin emojis.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; navegación completa por teclado, foco visible con alto contraste, etiquetado para tecnologías asistivas (`setAccessibleName`), y «Interfaz más fácil de usar» con escalado tipográfico a 14pt y controles con tap targets mínimos de 48px. Documentación obligatoria de ausencia de impacto visual directo para la capacidad `compiler warnings`.
- `docs/skills/06-rendimiento-y-ram.md`: Carga diferida de pestañas restauradas (`deferredUrl`) para evitar instanciación masiva de WebEngine en arranque; contención de memoria RAM y serialización de pruebas de navegador.
- `docs/skills/07-seguridad-y-privacidad.md`: Modelo local-first estricto; prohibición de cuentas de usuario, nube o sincronización remota de Vigilio; aislamiento de datos en SQLite local y exportación controlada sin exponer contraseñas ni cookies.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico riguroso de compilación; registros locales sin telemetría remota; evidencia de comandos y exit codes.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance; APPLY implementa únicamente el incremento del pack; REVIEW verifica de forma independiente).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado nativo (`WIN32_EXECUTABLE`, `MACOSX_BUNDLE`) e instalación estándar.
- `docs/skills/11-assets-svg-quality.md`: Integración de 36 recursos vectoriales SVG con viewBox `0 0 24 24`, trazo uniforme y referencias en `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Norma vinculante de lenguaje de producto público; prohibición absoluta de nombres de ingeniería («compiler warnings», «warnings», «-Werror», «Senior Mode», «Modo Senior», «CMake», «DAG», «Prompt Builder», «OpenSpec»); uso exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de ventana (tabs 34-40dp, toolbar 44-48dp), estados de controles y accesibilidad WCAG 2.2 AA.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Reglas de gobernanza de la fase PROPOSE, verificación reproducible y preservación de restricciones de scope.
- `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`: Referencias visuales de la interfaz de usuario objetivo y del panel de accesibilidad con «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_compiler_warnings` es una biblioteca `INTERFACE` en CMake vinculada de forma privada (`PRIVATE`) a todos los targets del proyecto, impidiendo la fuga transitiva hacia consumidores externos.
  2. Todos los targets de interfaz gráfica (`vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`) compilan con cero advertencias bajo `-Werror` / `/WX`.
  3. Ausencia formal de cambio visual directo: la capacidad `compiler warnings` no añade ningún widget, menú ni texto con nombres técnicos en la GUI pública del navegador.
  4. Cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`: prohibido el uso de «Senior Mode» / «Modo Senior»; empleo exclusivo de «Interfaz más fácil de usar».
  5. Soporte headless garantizado con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) para compilación y paso íntegro de pruebas en entornos de CI sin servidor gráfico.
  6. Aislamiento de código generado por Qt (`moc`, `rcc`) y cabeceras de terceros (`SQLite3`, `Qt6`) mediante inclusión `SYSTEM`.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/resources.qrc`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de paquetes de desarrollo de Qt6 en entornos ligeros de CI.
    *Mitigación*: Preservar la guarda condicional `find_package(Qt6 6.10 QUIET ...)` con mensaje de advertencia y registro honesto de estado `NOT_EXECUTED` para pruebas de GUI cuando falten dependencias Qt, sin romper la compilación headless.
  - *Riesgo*: Aparición de advertencias no controladas en macros internas de Qt (`Q_OBJECT`).
    *Mitigación*: Marcado de dependencias Qt6 como `SYSTEM` en CMake, evitando que diagnósticos de cabeceras de terceros detengan la compilación bajo `-Werror`.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 7, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0019-module.foundations.task0007.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0019-module.foundations.task0007.propose.lock.json", "valid": true, "checks": [...]}` (todos los checks passed)
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (solo bloquea `release.public-release`).
- **Verificación de suite headless**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: Ninja build exitoso, 2/2 tests pasados (`core_units` y `storage_integration`) en 0.02s con exit code 0.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0007/proposal.md`: redactado y completado con el alcance de la etapa `ui`.
  - `openspec/changes/module-foundations-task0007/design.md`: redactado y completado con arquitectura de UI, higiene de C++20 en widgets, accesibilidad y contratos de interacción.
  - `openspec/changes/module-foundations-task0007/tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-005.
  - `openspec/changes/module-foundations-task0007/execution-memory.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0007/canonical-reference.yaml`: validado.
- **Handoff**:
  - La propuesta está completamente elaborada, coherente y lista para la revisión y aprobación humana antes de proceder a la fase APPLY de `module.foundations.task0007`.
