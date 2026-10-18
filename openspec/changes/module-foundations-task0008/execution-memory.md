# Execution Memory — module-foundations-task0008

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta completa y rigurosa para la capacidad `compiler warnings` en su etapa `tests` (pruebas automatizadas deterministas de casos normales, negativos de inducción de advertencias, recuperación y no regresión), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0008-compiler-warnings/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `389aec0c1d215062440079cac1edeb7ddd0587167f0001153a340a4f470a67c2`
  - Lock SHA-256: `f52ed6519056bed9b8b589549271ee6c55913dad25651ba12b6576d0d0f4f2b4`
  - Versión de Prompt Builder: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y especificaciones del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación rigurosa de principios SOLID; responsabilidad única en el target de advertencias; DRY en la centralización de flags de compilación para GCC, Clang y MSVC; KISS en la formulación de fixtures de prueba compactos; YAGNI en evitar código innecesario.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; aislamiento headless de suites rápidas (`unit;fast`, `integration;fast`); pruebas negativas con aserciones precisas; timeouts estrictos para prevenir bloqueos; reporte honesto de `PASS`, `FAIL` y `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: Buenas prácticas de C++20; const-correctness; tipos estándar; inicializaciones con llaves para prevenir estrechamiento; destructores virtuales explícitos en clases base polimórficas; separación de capas de dominio y presentación.
- `docs/skills/04-ux-ui-senior.md`: Coherencia visual sobria; ausencia formal de cambio visual directo para la capacidad `compiler warnings`; constatación de que la infraestructura de compilación no altera la apariencia ni los controles de la aplicación.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; documentación explícita de impacto cero en la interfaz gráfica; garantía de que no se exponen términos técnicos en la UI del navegador; preservación de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de construcción y pruebas; contención estricta de consumo de memoria (< 50 MB); cero procesos Chromium durante la ejecución de pruebas de compilador.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first; ausencia absoluta de telemetría remota, conexiones cloud o credenciales; pruebas ejecutadas estrictamente en entorno local aislado.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico riguroso de compilación; compilación limpia con cero advertencias bajo `-Werror`; evidencia auditable de comandos ejecutados y códigos de retorno.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado e instalación estándar.
- `docs/skills/11-assets-svg-quality.md`: Preservación de la iconografía vectorial SVG y referencias de `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer nombres técnicos («compiler warnings», «warnings», «-Werror», «Senior Mode», «Modo Senior», «CMake», «DAG», «Prompt Builder», «OpenSpec») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de controles, estados de interacción y accesibilidad.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Contrato de gobernanza de fases; verificación reproducible y respeto absoluto a las restricciones de escritura en allowed paths.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. `vigilio_compiler_warnings` es una biblioteca `INTERFACE` en CMake enlazada de forma privada (`PRIVATE`) a todos los targets del proyecto (`vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, `vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`).
  2. Con `VIGILIO_WARNINGS_AS_ERRORS=ON` (valor predeterminado), cualquier advertencia en código propio detiene la compilación con código de error.
  3. Las cabeceras de terceros (`SQLite3`, `Qt6`) se aíslan como `SYSTEM`, impidiendo que advertencias externas rompan la construcción.
  4. Ausencia formal de cambio visual directo: la capacidad de advertencias no añade componentes gráficos ni expone jerga técnica a usuarios finales.
  5. Soporte headless garantizado con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) para compilación y paso íntegro de pruebas en entornos de CI sin servidor gráfico.
  6. Cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`: prohibido el uso de «Senior Mode» / «Modo Senior»; empleo exclusivo de «Interfaz más fácil de usar».
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/src/core/**`
  - `apps/vigilio-browser/src/storage/**`
  - `apps/vigilio-browser/src/ui/**`
  - `apps/vigilio-browser/tests/unit/CoreTests.cpp`
  - `apps/vigilio-browser/tests/integration/StorageTests.cpp`
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Discrepancias entre compiladores (GCC vs Clang vs MSVC) en advertencias semánticas avanzadas.
    *Mitigación*: Listas de banderas separadas y validadas por familia de compilador en `CMakeLists.txt`.
  - *Riesgo*: Sobrecarga en CI por pruebas negativas de compilación.
    *Mitigación*: Snippets sintéticos mínimos (< 15 líneas) ejecutados con timeout estricto de 15 segundos en CTest.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 8, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0022-module.foundations.task0008.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0022-module.foundations.task0008.propose.lock.json", "valid": true, "checks": [...]}` (13/13 comprobaciones pasadas exitosamente)
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito `release.public-release`.
- **Verificación de construcción y pruebas headless**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: Compilación Ninja limpia sin advertencias bajo `-Werror`, 2/2 tests pasados (`core_units` y `storage_integration`) en 0.01s.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec (`openspec/changes/module-foundations-task0008/`)**:
  - `canonical-reference.yaml`: validado conforme al esquema v1 de Prompt Builder.
  - `proposal.md`: redactado y completado con alcance, out of scope, contexto, pasos APPLY, DoD y commit propuesto.
  - `design.md`: redactado y completado con CURRENT/TARGET/GAPS, diseño de suite de pruebas, reglas y matriz de pruebas trazables.
  - `tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-006 con criterios de verificación y comandos.
  - `execution-memory.md`: redactado y completado con memoria técnica y handoff.
- **Handoff**:
  - La propuesta está completamente elaborada, coherente y lista para la revisión y aprobación humana antes de proceder a la fase APPLY de `module.foundations.task0008`.
