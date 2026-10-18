# Execution Memory — module-foundations-task0018

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa, rigurosa y formal para la capacidad `resource qrc inclusion` en su etapa `implementation` dentro del módulo propietario `foundations` de Vigilio Browser. El alcance define la capa de servicio C++20 desacoplada (`ResourceInclusionService`), el modelo de errores (`ResourceReadError`), resultados con semántica de valor (`ResourceReadResult<T>`), reporte de diagnóstico en arranque (`ResourceStartupReport`), token de cancelación cooperativa (`CancellationToken`), pre-validación de arranque en `src/app.cpp`, estrategias de recuperación local con fallbacks deterministas (`themes/light.json` y `icons/tools/tools.svg`) y suite de pruebas unitarias automatizadas en CTest, preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0018-resource-qrc-inclusion/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64)
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `4d1bfa628c0737795e95e828c9676f93290be7c9386d9255646f9aac4eec73c2`
  - Lock SHA-256: `8921052112c713a1a7df309f764a02592bf5f9e685e4dde229994fe94cce0066`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID, DRY y KISS en el diseño de `ResourceInclusionService`; responsabilidad única de provisión de recursos, interfaces acotadas sin acoplamiento a widgets de presentación.
- `docs/skills/02-testing-senior.md`: Pruebas unitarias puras y deterministas en C++20 para contratos de servicio, pre-validación de arranque, fallbacks y cancelación cooperativa; pruebas sin sleeps y con fixtures locales.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `constexpr`, `std::optional`, `std::atomic<bool>`), separación estricta entre el núcleo de lógica (`vigilio_core`) y la presentación Qt Widgets, evitando operaciones bloqueantes en el hilo de UI.
- `docs/skills/04-ux-ui-senior.md`: Ausencia total de cambio visual directo en la etapa `implementation`; aseguramiento de que el servicio provea los recursos necesarios para la interfaz sobria, moderna y profesional definida en las referencias.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Integración con «Interfaz más fácil de usar»: el servicio asegura la resolución confiable de `icons/tools/senior.svg` y los tokens de diseño que regulan tipografías y áreas de interacción accesibles.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia extrema: huella de memoria < 30 KB en `vigilio_core`, tiempo de pre-validación en arranque < 2 ms y cancelación no destructiva sin consumo de CPU en reposo.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first (decisión Q-001): todos los recursos se empaquetan en el binario sin depender de CDNs externas ni peticiones de telemetría; rechazo de Directory Traversal y prevención de rutas no autorizadas.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto C++20 verificado con Clang-Format, compilación con `-Werror`, introspección de estados y formateadores para diagnósticos legibles.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la capacidad para la etapa implementation) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/skills/11-assets-svg-quality.md`: Contrato de calidad de assets SVG: preservación de iconografía vectorial Lucide, viewBox `0 0 24 24`, trazos de 1.8 px y resolución garantizada mediante fallback a `icons/tools/tools.svg`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («QRC», «RCC», «viewBox», «DOM», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces visibles al usuario general.
- `/resultado/01-vigilio-home.png`, `/resultado/02-vigilio-tools-productivity.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Prefijo canónico inmutable*: Todo recurso del navegador se expone exclusivamente bajo el prefijo virtual `/vigilio` (ruta `:/vigilio/...`).
  2. *Validación obligatoria de recursos críticos*: El arranque requiere la presencia íntegra de los 8 recursos críticos (`branding/vigilio-logo.svg`, `design-tokens/tokens.json`, temas `light`/`dark`, 4 iconos de navegación esenciales).
  3. *Recuperación determinista con fallbacks*: Peticiones anómalas o recursos ausentes no críticos deben sustituirse automáticamente por `themes/light.json` o `icons/tools/tools.svg` con indicación en `hasFallback`.
  4. *Cancelación cooperativa*: Operaciones asíncronas de lectura pueden abortarse limpiamente vía `CancellationToken` sin fugas de recursos.
  5. *Aislamiento y rechazo de rutas prohibidas*: Denegación inmediata de accesos a `reference/`, `installer/`, `fonts/` o rutas absolutas.
  6. *Local-First constitucional (Decisión Q-001)*: Cero peticiones de red o dependencias de servidores remotos.
  7. *Desacoplamiento de C++20 en `vigilio_core`*: El servicio no depende de `QApplication` ni de `WebEngine`, garantizando su compilación y testeo en entornos headless.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/ResourceTypes.h`: Tipos taxonómicos de recursos.
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo canónico de 35 recursos y validadores estáticos.
  - `apps/vigilio-browser/src/app.cpp`: Pipeline de pre-validación de arranque existente.
  - `apps/vigilio-browser/resources.qrc` y `apps/vigilio-browser/assets/**`: 35 recursos existentes y probados.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline de integración y pruebas automatizadas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Demora en el arranque por verificación de recursos.
    *Mitigación*: Validación pura sobre el catálogo estático en O(1), completada en < 2 ms.
  - *Riesgo*: Crashes en la interfaz visual por referencias a iconos inexistentes.
    *Mitigación*: `resolveIconWithFallback` sustituye automáticamente cualquier alias anómalo por el icono genérico seguro `":/vigilio/icons/tools/tools.svg"`.
  - *Riesgo*: Operaciones de lectura colgadas en hilos secundarios.
    *Mitigación*: `CancellationToken` permite cancelación cooperativa inmediata y no bloqueante.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Pipeline de compilación y pruebas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (6/6 tests pasados en 0.13s, exit code 0):
    - `core_units`: Passed (0.00s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.06s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.05s)
    - `resource_qrc_catalog_tests`: Passed (0.01s)
  - Validación de formato: Clang-Format verificado sin advertencias ni modificaciones pendientes.
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 18/18 referencias con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0052-module.foundations.task0018.propose.lock.json --project-root .` superó 13/13 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que no existen decisiones abiertas que bloqueen foundations ni resource qrc inclusion (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo aplica al hito final de lanzamiento).
- **Handoff**:
  - La propuesta técnica para la etapa `implementation` de `module.foundations.task0018` queda formalizada, coherente y lista en `openspec/changes/module-foundations-task0018/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY (`module.foundations.task0018.apply`).
