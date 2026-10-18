# Execution Memory — module-foundations-task0002

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0002` del módulo `foundations` (`module.foundations.task0002`), correspondiente a la etapa `implementation` de la capacidad **cmake targets**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0002-cmake-targets/task.md`
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
  - Prompt SHA-256: `6b5193ebcb6ac2162ad5d3b99e922c5f4911eb417f7fa96fa37257b410df55db`
  - Lock SHA-256: `983eb40357166a7d6e52be24cabed6bd1580f40377756a335d599445a7c70526`

## Skills and required references read

Se consultaron y aplicaron al 100% las siguientes skills normativas del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID (Single Responsibility aplicado a la separación de `vigilio_core` y `vigilio_storage`), DRY, KISS y YAGNI en la modularización de targets de CMake.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unit, integration, e2e); timeouts deterministas (15s para fast, 60s para e2e) y registro honesto de estados de ejecución.
- `docs/skills/03-codigo-cpp-qt.md`: Código C++20 estándar, const-correctness, núcleo desacoplado de la ventana principal y separación estricta entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Filosofía de diseño sobrio, contraste controlado, reuso de componentes y prohibición de jerga interna en interfaces visibles.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible, navegación por teclado, Modo Senior («Interfaz más fácil de usar») y documentación obligatoria de ausencia de impacto visual para tareas de construcción e infraestructura.
- `docs/skills/06-rendimiento-y-ram.md`: Contención estricta del uso de memoria, aislamiento serial de pruebas pesadas de navegador (`RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`) y no ejecutar procesos de renderizado innecesarios.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first, cero telemetría remota, sin sincronización ni servicios en la nube de Vigilio.
- `docs/skills/08-observabilidad-calidad-linters.md`: Calidad de código mediante formateo estricto con ClangFormat, diagnóstico riguroso de compilador y logs locales sin datos sensibles.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de las fases PROPOSE (diseño completo), APPLY (implementación del incremento específico) y REVIEW (verificación independiente); respeto estricto a las restricciones de escritura en PROPOSE.
- `docs/skills/10-release-instaladores.md`: Criterios para empaquetado nativo y desinstalación transparente y conservadora.
- `docs/skills/11-assets-svg-quality.md`: Reglas de renderizado SVG, inclusión de recursos en `resources.qrc` y coherencia visual con `/resultado/`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición total de exponer términos de ingeniería (`Senior Mode`, `Prompt Builder`, `OpenSpec`, `DAG`, `cmake`) en interfaces de usuario finales.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_core` no contiene referencias a SQLite, Qt Widgets ni Qt WebEngine, compilando exclusivamente lógica de dominio puro en C++20.
  2. `vigilio_storage` es el único target responsable de enlazar `SQLite::SQLite3`, dependiendo de `vigilio_core`.
  3. Soporte headless completo mediante `VIGILIO_BUILD_GUI=OFF` para permitir compilación y ejecución de pruebas en entornos de CI sin interfaz gráfica.
  4. Pruebas E2E de navegador restringidas con `RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE` y timeout máximo de 60 segundos.
  5. Ausencia total de cuentas, almacenamiento en la nube o telemetría remota de Vigilio.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/core/UrlInput.h`
  - `apps/vigilio-browser/src/storage/LocalRepository.h`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Modificación en include paths al separar targets.
  - *Mitigación*: Utilizar `target_include_directories(... PUBLIC src)` en ambos targets de librería, asegurando que los includes existentes como `core/UrlInput.h` y `storage/LocalRepository.h` sigan resolviéndose idénticamente.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 2, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0004-module.foundations.task0002.propose.lock.json`
  - Salida: `{"lockfile": "...", "valid": true, "checks": [...]}`
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (solo bloquea `release.public-release`).
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0002/proposal.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0002/design.md`: redactado y completado con arquitectura modular de targets.
  - `openspec/changes/module-foundations-task0002/tasks.md`: redactado y completado con tareas estables TASK-001 a TASK-005.
  - `openspec/changes/module-foundations-task0002/execution-memory.md`: redactado y completado.
- **Handoff**:
  - La propuesta está completamente elaborada, coherente y lista para la revisión y aprobación humana antes de proceder a la fase APPLY de `module.foundations.task0002`.
