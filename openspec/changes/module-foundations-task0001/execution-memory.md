# Execution Memory — module-foundations-task0001

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0001` del módulo `foundations` (`module.foundations.task0001`), correspondiente a la etapa `domain` de la capacidad **cmake targets**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0001-cmake-targets/task.md`
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
  - Prompt SHA-256: `4aa654a2cb71d7a69e0eae9deffd422a93fa5d3503c3b104059266bd3f99fb31`
  - Lock SHA-256: `caa9ebe43cd928285750fd591c9bce6923b4cfcf44151bc2e6db7fe04f231d61`

## Skills and required references read

Se consultaron y aplicaron al 100% las siguientes skills normativas del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación de principios SOLID, DRY, KISS y YAGNI; modularidad de targets de CMake sin mezcla indebida de responsabilidades.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unit, integration, e2e); ejecución serial de tests pesados (`RESOURCE_LOCK`), timeouts deterministas y registro honesto de estados (`PASS`, `FAIL`, `NOT_EXECUTED`).
- `docs/skills/03-codigo-cpp-qt.md`: Código C++20 estándar, const-correctness, núcleo desacoplado de la ventana principal y exclusión de Widgets en la biblioteca de negocio.
- `docs/skills/04-ux-ui-senior.md`: Filosofía de diseño sobrio, contraste controlado, reuso de componentes y prohibición de jerga interna en interfaces visibles.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible, navegación por teclado, Modo Senior («Interfaz más fácil de usar») y declaración explícita de impacto para tareas sin interfaz gráfica.
- `docs/skills/06-rendimiento-y-ram.md`: Contención estricta del uso de memoria, no ejecutar pruebas E2E en paralelo y no cargar módulos pesados si están inactivos.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first, cero telemetría, sin sincronización ni servicios en la nube de Vigilio.
- `docs/skills/08-observabilidad-calidad-linters.md`: Calidad de código, formateo mediante ClangFormat/Clang-Tidy y logs locales protegidos sin registrar datos sensibles.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de las fases PROPOSE (diseño completo), APPLY (implementación del incremento específico) y REVIEW (verificación independiente).
- `docs/skills/10-release-instaladores.md`: Criterios para instaladores nativos, empaquetado de assets y desinstalación transparente.
- `docs/skills/11-assets-svg-quality.md`: Reglas de renderizado SVG, rutas dentro de `resources.qrc` y consistencia visual con `/resultado/`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición total de exponer términos de ingeniería (`Senior Mode`, `Prompt Builder`, `OpenSpec`, `DAG`) en interfaces de usuario finales.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_core` NUNCA enlaza a `Qt6::Widgets` ni `Qt6::WebEngineWidgets`.
  2. El estándar de compilación se fija estrictamente en C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`).
  3. Soporte headless completo mediante la opción `VIGILIO_BUILD_GUI=OFF` para entornos de integración continua y servidores.
  4. Pruebas E2E de navegador restringidas con `RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE` y timeout máximo de 60 segundos.
  5. Ausencia total de cuentas, almacenamiento en la nube o telemetría remota de Vigilio.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Ausencia de bibliotecas de desarrollo (`libsqlite3-dev`, `Qt6`) en máquinas de compilación limpias.
  - *Mitigación*: Mantener targets desacoplados con presets headless; en caso de ausencia de Qt6, registrar `NOT_EXECUTED` con advertencia clara en lugar de fallar o falsear el resultado.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 1, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0001-module.foundations.task0001.propose.lock.json`
  - Salida: `{"lockfile": "...", "valid": true, "checks": [...]}`
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001..Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (solo bloquea `release.public-release`).
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0001/proposal.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0001/design.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0001/tasks.md`: redactado y completado con tareas estables TASK-001 a TASK-004.
  - `openspec/changes/module-foundations-task0001/execution-memory.md`: redactado y completado.
- **Handoff**:
  - La propuesta está completamente elaborada, coherente y lista para la revisión y aprobación humana antes de proceder a la fase APPLY de este pack o a la siguiente microtarea (`module.foundations.task0002`).
