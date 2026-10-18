# Execution Memory — module-foundations-task0017

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa, rigurosa y formal para la capacidad `resource qrc inclusion` en su etapa `domain` dentro del módulo propietario `foundations` de Vigilio Browser. El alcance define el modelo conceptual, los tipos de valor (`ResourceCategory`, `ResourceInclusionStatus`, `ResourceDescriptor`), el catálogo canónico inmutable de los 35 recursos aprobados (`ResourceQrcCatalog`), las reglas de normalización y validación sintáctica de alias (rechazo estricto de Directory Traversal `../`), las políticas de exclusión de rutas no autorizadas (`reference/`, `installer/`, `fonts/`), la especificación de consistencia para el archivo XML `.qrc` y el desacoplamiento arquitectónico en C++20 dentro de la biblioteca estática `vigilio_core`, preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0017-resource-qrc-inclusion/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64)
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `0ec6102307489fb8dac1f6df45ae7b4f1601bd6139388912a3bea1afe4b96d0b`
  - Lock SHA-256: `b213c8101487952f6fa0efc6974777aa08e704511f7e67b6089e1aac1449a4f2`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación de principios SOLID, DRY y KISS al catálogo de dominio; modelo inmutable con tipos fuertemente tipados, sin duplicación de definiciones y con responsabilidades acotadas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: pruebas unitarias puras y deterministas en C++20 para contratos de catálogo, validación de alias y consistencia XML; pruebas sin sleeps y con fixtures sintéticos reproducibles.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `constexpr`, `std::optional`, `operator<=>`), separación estricta entre el núcleo de lógica (`vigilio_core`) y la presentación Qt Widgets, y prevención de bloqueos en hilo de UI.
- `docs/skills/04-ux-ui-senior.md`: Ausencia total de cambio visual directo en la etapa `domain`; aseguramiento de que el catálogo contenga los recursos necesarios para la interfaz moderna, sobria y profesional definida en las referencias.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Integración con «Interfaz más fácil de usar»: el catálogo expone el icono específico `icons/tools/senior.svg` y los tokens de diseño (`tokens.json`) que regulan tipografías y áreas de clic accesibles.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia extrema: consumo de memoria estática < 20 KB en `vigilio_core`, consultas por alias en O(1) o logarítmicas deterministas, y cero procesos zombis o workers en segundo plano.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first (decisión Q-001): todos los recursos se empaquetan en el binario sin depender de CDNs externas ni peticiones de telemetría; rechazo de Directory Traversal y prevención de vectores XML/XXE.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto C++20 verificado con Clang-Format, compilación con `-Werror`, introspección de categorías y estados para logs de diagnóstico.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad para la etapa domain) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/skills/11-assets-svg-quality.md`: Contrato de calidad de assets SVG: viewBox `0 0 24 24`, trazos de 1.8 px, terminaciones redondeadas, coherencia de identidad con el globo terráqueo y correspondencia biunívoca con `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («QRC», «RCC», «viewBox», «DOM», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces visibles al usuario general.
- `/resultado/01-vigilio-home.png`, `/resultado/02-vigilio-tools-productivity.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Prefijo canónico inmutable*: Todo recurso del navegador se expone exclusivamente bajo el prefijo virtual `/vigilio` (ruta `:/vigilio/...`).
  2. *Catálogo canónico cerrado*: Consta de exactamente 35 recursos aprobados (6 temas, 6 wallpapers, 1 branding, 1 tokens, 10 iconos de navegación, 11 iconos de herramientas).
  3. *Reglas de exclusión de rutas no autorizadas*: Queda prohibida la inclusión de `reference/` (maquetas), `installer/` (instalador) y `fonts/` (fuentes sin licencia auditada).
  4. *Rechazo de Directory Traversal*: Cualquier alias que contenga secuencias `..` o componentes que intenten salir del bundle virtual es rechazado como `InvalidAliasFormat`.
  5. *Local-First constitucional (Decisión Q-001)*: Cero peticiones de red o resolución externa para activos de interfaz; todo recurso se incluye compilado en el binario.
  6. *Desacoplamiento de C++20 en `vigilio_core`*: El modelo de dominio no depende de Qt Widgets ni WebEngine, permitiendo su compilación y testeo en entornos headless.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/resources.qrc`: 35 elementos `<file alias="...">` existentes.
  - `apps/vigilio-browser/assets/**`: 35 archivos físicos validados en disco.
  - `apps/vigilio-browser/assets/README.md`: Reglas canónicas de activos y exclusiones.
  - `apps/vigilio-browser/CMakeLists.txt`: Configuración de `CMAKE_AUTORCC ON`.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline de integración y pruebas automatizadas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Discrepancia entre la definición en `resources.qrc` y el catálogo en C++.
    *Mitigación*: Se formaliza el método `validateQrcConsistency` para comprobar programáticamente en pruebas unitarias que la correspondencia sea exacta y bidireccional.
  - *Riesgo*: Contaminación del bundle binario con maquetas pesadas de `assets/reference/`.
    *Mitigación*: Regla estricta de exclusión R-004 validada sintácticamente por `isExcludedPath` tanto en runtime como en CTest.
  - *Riesgo*: Acoplamiento con bibliotecas gráficas en entornos sin display.
    *Mitigación*: Tipos de dominio puramente estándar en C++20 (`std::string_view`, `std::optional`, `std::array`).

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Pipeline de compilación y pruebas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (5/5 tests pasados en 0.13s, exit code 0):
    - `core_units`: Passed (0.00s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.07s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.05s)
  - Validación de formato: Clang-Format verificado sin advertencias ni modificaciones pendientes.
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 17/17 referencias con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0049-module.foundations.task0017.propose.lock.json --project-root .` superó 14/14 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que no existen decisiones abiertas que bloqueen foundations ni resource qrc inclusion (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo aplica al hito final de lanzamiento).
- **Handoff**:
  - La propuesta técnica para la etapa `domain` de `module.foundations.task0017` queda formalizada, coherente y lista en `openspec/changes/module-foundations-task0017/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY (`module.foundations.task0018`).
