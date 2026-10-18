# Execution Memory — module-foundations-task0013

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `WebEngine version compatibility` en su etapa `domain` (modelo de dominio de versiones cuatripartitas de Chromium en C++20, versión mínima canónica Chromium milestone >= 128 / Qt >= 6.10.0, taxonomía de capacidades funcionales, perfiles de compatibilidad, reglas de validación en tiempo de arranque y protocolo de degradación suave a modo headless), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0013-webengine-version-compatibility/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `8ae2896b1e6762c6b67d0fe6d86bb8fc76bb43212ec0e13997a5404a9e884f99`
  - Lock SHA-256: `0cb17d4d04a3599cf7fc7a283e69e0a8e3e1f5063f770e1ac7639e522055e0e4`
  - Versión de Prompt Builder: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y directrices canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación rigurosa de principios SOLID: clase `ChromiumVersion` con responsabilidad única de representación y parsing de versión cuatripartita; segregación de interfaces mediante perfiles de requerimiento; inversión de dependencias desacoplando el núcleo de dominio de `vigilio_core` de las cabeceras pesadas de Chromium y `Qt6::WebEngineWidgets`; observancia de principios DRY, KISS y YAGNI.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: especificación de casos unitarios puros para parsing, normalización, comparación y predicados de compatibilidad; aislamiento de ejecución headless sin requerir servidor X11/Wayland; registro honesto de estados `PASS`, `FAIL` y `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: Uso de C++20 moderno (operador de nave espacial `<=>`, `std::string_view`, `std::optional`, tipos estándar); inmutabilidad y const-correctness; separación clara de capas (`src/core`, `src/storage`, `src/ui`); validación estricta de rutas de archivos para el proceso asistente.
- `docs/skills/04-ux-ui-senior.md`: Coherencia visual sobria; formalización de ausencia total de cambio visual directo para la etapa `domain`; aseguramiento de que la infraestructura no añade distorsiones visuales ni componentes no justificados.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; garantía de que la infraestructura interna no interfiere con lectores de pantalla ni navegación por teclado; preservación del principio de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de ejecución (< 1 ms por evaluación); consumo de memoria RAM inferior a 50 KB; prevención estricta de procesos zombies; cero instancias de Chromium o WebEngine lanzadas durante la validación de dominio.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta (decisión Q-001); ausencia total de consultas de red o servicios en la nube para comprobación de versiones; verificación del sandbox de Chromium; aislamiento de almacenamiento y protección de perfiles privados.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico legible y estructurado mediante `formatStatus` y `formatReport`; evidencia auditable con comandos reproducibles y códigos de salida explícitos.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Requisitos de validación de dependencias de empaquetado y prerrequisitos del sistema operativo.
- `docs/skills/11-assets-svg-quality.md`: Preservación de la iconografía vectorial SVG y referencias de `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de controles, estados de interacción y accesibilidad.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Contrato de gobernanza de fases; verificación reproducible y respeto absoluto a las restricciones de escritura en allowed paths.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Versión mínima requerida: Chromium milestone >= 128 asociado a Qt >= 6.10.0 para perfiles de escritorio con navegación web.
  2. Desacoplamiento de Chromium: `vigilio_core` no incluye cabeceras de Chromium ni enlaza con librerías dinámicas de WebEngine, garantizando portabilidad y compilación 100% en entornos headless.
  3. Proceso asistente obligatorio: el binario ejecutable `QtWebEngineProcess` debe existir y tener permisos de ejecución antes de instanciar componentes web.
  4. Degradación suave: si WebEngine está desactivado o ausente, la aplicación transiciona limpiamente a `DegradedHeadless` sin abortos anormales ni fugas de memoria.
  5. Ausencia formal de cambio visual directo: la capacidad es infraestructura de dominio y no expone nombres técnicos a los usuarios finales (cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`).
  6. Filosofía local-first estricta: cero conexiones de red para verificación de versiones o telemetría.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad del motor WebEngine en entornos headless o contenedores mínimos de integración continua.
    *Mitigación*: Preservación estricta de la ruta de compilación headless (`VIGILIO_BUILD_GUI=OFF`), permitiendo que el núcleo y sus pruebas unitarias se ejecuten con éxito total.
  - *Riesgo*: Falsos positivos en la verificación de `QtWebEngineProcess` en distribuciones que ubican los asistentes en directorios no estándar.
    *Mitigación*: Consulta dinámica a través de `QLibraryInfo::LibraryExecutablesPath` y soporte de rutas configurables para empaquetados específicos.
  - *Riesgo*: Fallos de inicialización del sandbox de Chromium en Linux si los espacios de nombres de usuario están deshabilitados en el kernel.
    *Mitigación*: Diagnóstico estructurado mediante `EnvironmentMisconfigured` con instrucciones precisas de configuración del sistema en logs locales.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 13, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0037-module.foundations.task0013.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0037-module.foundations.task0013.propose.lock.json", "valid": true, "checks": [...]}` (13/13 comprobaciones pasadas exitosamente)
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito final `release.public-release`.
- **Verificación de construcción y pruebas locales**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: Compilación Ninja limpia bajo `-Werror` y 4/4 pruebas pasadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`) en 0.18s.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec (`openspec/changes/module-foundations-task0013/`)**:
  - `canonical-reference.yaml`: validado conforme al esquema v1 de Prompt Builder.
  - `proposal.md`: redactado y completado con alcance, out of scope, contexto, pasos APPLY, DoD y commit propuesto.
  - `design.md`: redactado y completado con CURRENT/TARGET/GAPS, diseño de dominio en C++20, reglas, fallos y matriz de pruebas trazables.
  - `tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-006 con criterios de verificación y comandos.
  - `execution-memory.md`: redactado y completado con memoria técnica y handoff.
- **Handoff**:
  - La propuesta para `module.foundations.task0013` (etapa `domain` de `WebEngine version compatibility`) está completamente elaborada, es coherente, respeta todos los contratos canónicos y está lista para la revisión y aprobación humana antes de proceder a la fase APPLY.
