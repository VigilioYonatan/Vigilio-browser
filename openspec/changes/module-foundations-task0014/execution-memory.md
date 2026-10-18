# Execution Memory — module-foundations-task0014

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `WebEngine version compatibility` en su etapa `implementation` (implementación de la capa C++20 desacoplada en `vigilio_core`: `WebEngineVersionValidator`, entidades inmutables `ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, `WebEngineCompatibilityReport`, integración en `CMakeLists.txt`, pre-validación en el punto de arranque `app.cpp` y suite de pruebas unitarias en CTest), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0014-webengine-version-compatibility/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 multiplataforma)
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `e21306c174f8aa576877161d7d3fcdef0497e6139bad42c18b7b4b5ca0f247f0`
  - Lock SHA-256: `6daffb15492fe56f289a75ef2687fbd6303b3cfc09c683ec1461d4992b17c99f`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y directrices canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación de principios SOLID: clase `ChromiumVersion` con responsabilidad única de representación y parsing de versión cuatripartita; clase `WebEngineVersionValidator` con funciones puras y cohesivas; desacoplamiento estricto del núcleo de dominio `vigilio_core` respecto a las cabeceras pesadas de Chromium y `Qt6::WebEngineWidgets`; observancia de principios DRY, KISS y YAGNI evitando dependencias superfluas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: especificación de casos unitarios puros para parsing, normalización, comparación, resolución de matrices de características y validación de reportes; aislamiento de ejecución headless sin requerir servidor gráfico X11/Wayland; registro honesto de estados `PASS`, `FAIL` y `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: Uso de C++20 moderno (operador de nave espacial `<=>`, `std::string_view`, `std::optional`, tipos estándar); inmutabilidad y const-correctness; separación clara de capas (`src/core`, `src/storage`, `src/ui`); validación estricta de rutas de archivos para el proceso asistente.
- `docs/skills/04-ux-ui-senior.md`: Coherencia visual sobria; formalización de ausencia total de cambio visual directo para la etapa `implementation`; aseguramiento de que la infraestructura no añade distorsiones visuales ni componentes no justificados.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; garantía de que la infraestructura interna no interfiere con lectores de pantalla ni navegación por teclado; preservación del principio de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de ejecución (< 1 ms por evaluación); consumo de memoria RAM inferior a 50 KB; prevención estricta de procesos zombies; cero instancias de Chromium o WebEngine lanzadas durante la validación de arranque.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta (decisión Q-001); ausencia total de consultas de red o servicios en la nube para comprobación de versiones; verificación del sandbox de Chromium; aislamiento de almacenamiento y protección de datos locales.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico legible y estructurado mediante `formatStatus` y `formatReport`; evidencia auditable con comandos reproducibles y códigos de salida explícitos.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Requisitos de validación de dependencias de empaquetado y verificación de componentes del sistema operativo.
- `docs/skills/11-assets-svg-quality.md`: Preservación de la iconografía vectorial SVG y referencias de `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de controles, estados de interacción y accesibilidad.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Contrato de gobernanza de fases; verificación reproducible y respeto absoluto a las restricciones de escritura en allowed paths.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Versión mínima requerida: Chromium milestone >= 128 asociado canónicamente a Qt >= 6.10.0 para perfiles de escritorio con navegación web.
  2. Desacoplamiento de Chromium: `vigilio_core` no incluye cabeceras de Chromium ni enlaza con librerías dinámicas de WebEngine, garantizando portabilidad y compilación 100% en entornos headless.
  3. Proceso asistente obligatorio: el binario ejecutable `QtWebEngineProcess` debe existir y tener permisos de ejecución antes de instanciar componentes web en perfiles con GUI.
  4. Degradación suave: si WebEngine está desactivado o la aplicación corre en modo consola, la arquitectura transiciona limpiamente a `DegradedHeadless` sin caídas anormales ni fugas de memoria.
  5. Ausencia formal de cambio visual directo: la capacidad es infraestructura de backend y arranque; no expone nombres técnicos a los usuarios finales (cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`).
  6. Filosofía local-first estricta: cero conexiones de red para verificación de versiones o telemetría.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Variabilidad en la ruta de instalación de `QtWebEngineProcess` entre diferentes distribuciones Linux.
    *Mitigación*: Soporte para búsqueda estándar en librerías del sistema, introspección vía `QLibraryInfo` cuando esté disponible y variable de entorno `QTWEBENGINEPROCESS_PATH`.
  - *Riesgo*: Indisponibilidad del motor WebEngine en entornos headless o contenedores mínimos de integración continua.
    *Mitigación*: Preservación estricta de la ruta de compilación headless (`VIGILIO_BUILD_GUI=OFF`), permitiendo que el núcleo y sus pruebas unitarias se ejecuten con éxito total.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Compilación local: Ninja configurado con éxito (`CMakeLists.txt`, C++20, `-Werror`).
  - Suite de pruebas automatizadas: `apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (4/4 tests pasados en 0.13s, exit code 0).
  - Validación de referencias OpenSpec: `vigilio-prompt validate-references` superó 14/14 comprobaciones con estado `valid: true`.
  - Verificación de lockfile: hashes de prompt y lock validados satisfactoriamente.
  - Registro de decisiones: `validate-decisions` confirmó que ninguna decisión bloquea foundations ni webengine.
- **Handoff**:
  - La propuesta para `module.foundations.task0014` queda completa, coherente y lista en `openspec/changes/module-foundations-task0014/` para revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY.
