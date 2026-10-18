# Execution Memory — module-foundations-task0006

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta formal (fase PROPOSE) para la microtarea `0006` del módulo `foundations` (`module.foundations.task0006`), correspondiente a la etapa `implementation` de la capacidad **compiler warnings**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0006-compiler-warnings/task.md`
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
  - Prompt SHA-256: `44365337fed32cdf63d4f67802c5c676a1f239eaa25c6482582e4e90fa454241`
  - Lock SHA-256: `479b160e948023e416d7473b4af34aed226d244d1b9f1683d51952d14b23cf91`
  - Dependencias ejecutables: `module.foundations.task0005`
  - Versión de Prompt Builder: `0.6.3`

## Skills and required references read

Se consultaron y aplicaron al 100% las siguientes skills normativas del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID (Single Responsibility e Interface Segregation aplicados a la centralización de diagnósticos en `vigilio_compiler_warnings`), DRY (eliminación de bloques repetidos de `target_compile_options`), KISS y YAGNI.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas, timeouts deterministas y verificación en modo headless con Ninja y CTest.
- `docs/skills/03-codigo-cpp-qt.md`: Código C++20 estándar, const-correctness, RAII y desacoplamiento modular entre lógica de negocio, persistencia y UI.
- `docs/skills/04-ux-ui-senior.md`: Filosofía de diseño sobrio y coherente; documentación explícita de ausencia de cambio visual directo en capacidades de infraestructura de compilación.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible, protección de la experiencia de usuario y Modo Senior («Interfaz más fácil de usar») al prevenir caídas por corrupción de memoria o tipos.
- `docs/skills/06-rendimiento-y-ram.md`: Cero impacto en tiempo de ejecución, cero sobrecarga en binario y contención estricta de memoria RAM.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first, cero telemetría y prevención activa de vulnerabilidades de seguridad (integer overflows, punteros nulos, alineación de memoria) mediante diagnósticos del compilador.
- `docs/skills/08-observabilidad-calidad-linters.md`: Rigor de calidad de código mediante advertencias tratadas como errores (`VIGILIO_WARNINGS_AS_ERRORS`) y diagnósticos limpios.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases PROPOSE (diseño completo), APPLY (incremento acotado) y REVIEW (verificación independiente), respetando los límites de escritura de PROPOSE.
- `docs/skills/10-release-instaladores.md`: Criterios de reproducibilidad en construcción para empaquetado nativo.
- `docs/skills/11-assets-svg-quality.md`: Respeto a los assets gráficos y verificación con `/resultado/` (`01-vigilio-home.png` a `06-logo-instalador-oficial.png`).
- `docs/UX-COPY-PUBLICO.md`: Prohibición total de exponer términos de ingeniería (`compiler`, `warnings`, `-Werror`, `/WX`, `INTERFACE`, `CMake`) en interfaces públicas.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_compiler_warnings` es un target de tipo `INTERFACE` en CMake que centraliza todas las directivas de advertencia por compilador (GNU, Clang, MSVC).
  2. Todo target del proyecto enlaza `vigilio_compiler_warnings` con visibilidad `PRIVATE`, impidiendo la fuga transitiva hacia consumidores downstream.
  3. Opción `VIGILIO_WARNINGS_AS_ERRORS` activa por defecto (`ON`) en pipelines de CI y configurable a `OFF` para desarrollo exploratorio local.
  4. Inclusión de dependencias externas (`SQLite3`, `Qt6`) con directivas `SYSTEM` para suprimir advertencias ajenas al proyecto.
  5. Soporte continuo para construcción headless con `VIGILIO_BUILD_GUI=OFF` y ejecución limpia de suites de pruebas con CTest.
  6. Ausencia total de cuentas, telemetría o servicios en la nube de Vigilio.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
  - `apps/vigilio-browser/src/core/SessionFile.cpp`, `TabModel.cpp`, `UrlInput.cpp`
  - `apps/vigilio-browser/src/storage/LocalRepository.cpp`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Advertencias espurias originadas en macros de bibliotecas externas (SQLite3, Qt6).
    *Mitigación*: Marcar directorios externos como `SYSTEM` en CMake, evitando que diagnósticos ajenos aborten la compilación bajo `-Werror`.
  - *Riesgo*: Divergencia de flags entre diferentes versiones de compiladores (GCC vs Clang vs MSVC).
    *Mitigación*: Desacoplar listas de flags por `CMAKE_CXX_COMPILER_ID` y definir un fallback seguro `-Wall -Wextra` ante compiladores no reconocidos.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 6, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0016-module.foundations.task0006.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0016-module.foundations.task0006.propose.lock.json", "valid": true, "checks": [...]}`
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (solo bloquea `release.public-release`).
- **Simulación de compilación limpia**:
  - Simulación aislada del target `vigilio_compiler_warnings` en CMake con Ninja y GCC 15.2.0 bajo `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wformat=2 -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast -Werror`.
  - Resultado: 10/10 objetos construidos limpiamente, 100% pruebas de CTest superadas con código de salida 0.
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0006/proposal.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0006/design.md`: redactado y completado con arquitectura modular de advertencias de compilador.
  - `openspec/changes/module-foundations-task0006/tasks.md`: redactado y completado con tareas estables TASK-001 a TASK-007.
  - `openspec/changes/module-foundations-task0006/execution-memory.md`: redactado y completado.
  - `openspec/changes/module-foundations-task0006/canonical-reference.yaml`: presente y validado.
- **Handoff**:
  - La propuesta para `module.foundations.task0006` está completamente elaborada, coherente, técnicamente validada y lista para la revisión y aprobación humana antes de proceder a la fase APPLY.
