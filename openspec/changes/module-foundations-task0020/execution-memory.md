# Execution Memory — module-foundations-task0020

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **resource qrc inclusion** en el módulo `foundations` (microtarea `0020 — Probar casos positivos, negativos y recuperación: resource qrc inclusion`), definiendo una suite automatizada de pruebas exhaustivas (`ResourceQrcInclusionTests.cpp`) que verifique casos normales, integridad física y de contenido en disco, fuzzing de alias anómalos, defensa contra Directory Traversal, corrupción sintáctica de manifiestos, aislamiento de recursos críticos, fallbacks deterministas, cancelación cooperativa y estrés multihilo libre de data races, bajo compilación portable C++20 y ejecución 100% headless sin requerir servidor gráfico ni procesos Chromium/WebEngine.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`: Responsabilidad y alcance de `foundations`.
  - `docs/rules/01-modules/foundations/design.md`: Arquitectura y desacoplamiento del núcleo frente a WebEngine.
  - `docs/rules/01-modules/foundations/screens.md`: Estados visuales y degradación de interfaz.
  - `docs/rules/01-modules/foundations/business.md`: Reglas operativas, transiciones y manejo de errores.
  - `docs/rules/01-modules/foundations/tests.md`: Matriz de pruebas y clasificación taxonómica (`fast`, `unit`, `integration`).
  - `docs/rules/01-modules/foundations/operations.md`: Observabilidad y recuperación ante contingencias.
  - `docs/rules/01-modules/foundations/micro/0020-resource-qrc-inclusion/task.md`: Contrato vinculante de la microtarea 0020.
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first sin nube, Q-002 C++20/Qt6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64, Q-006 assets en `apps/vigilio-browser/assets`).
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `2139ad901764a789986d1dc92deeebc30c31d37804118477b414cb8d9105d2b3`
  - Lock SHA-256: `567900cfdbfeaabe25df08fd1cdf207fc428b32f91a0f2425fa02d57bc38398b`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY; separación de responsabilidades entre el catálogo inmutable (`ResourceQrcCatalog`), el servicio de inclusión (`ResourceInclusionService`) y el arnés de verificación (`ResourceQrcInclusionTests`), evitando duplicación de aserciones mediante macros deterministas.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas centrada en pruebas unitarias deterministas y de integración interna, con fixtures locales, sin sleeps arbitrarios, aserciones claras y clasificación explícita en CTest con reporte preciso de `PASS`, `FAIL` o `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, `std::filesystem`, `std::atomic`, `std::thread`, RAII para flujos de archivos), sin operaciones bloqueantes ni interferencia con hilos de interfaz, asegurando const-correctness estricta.
- `docs/skills/04-ux-ui-senior.md`: Protección de la identidad visual sobria y profesional; preservación de tokens de diseño limpios y consistencia en la iconografía SVG de Lucide sin emojis ni brillos artificiales.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Verificación de la integridad de los activos utilizados por «Interfaz más fácil de usar» (Modo Senior) y respeto absoluto a la denominación pública obligatoria según `docs/UX-COPY-PUBLICO.md`.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia computacional y huella de memoria: huella estática del catálogo de recursos < 30 KB, tiempo de verificación de arranque < 2 ms, pruebas multihilo con latencia < 1 ms por operación y cero procesos zombis en segundo plano.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first constitucional (decisión Q-001): todos los recursos se consumen desde el almacenamiento local compilado; defensa activa contra Path Traversal (`../`) y Local File Inclusion (LFI); cero conexiones a servicios en la nube o telemetría.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, compilación con `-Werror` bajo C++20 y pruebas de regresión automatizadas en CTest bajo CMake/Ninja.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la arquitectura de pruebas y contratos) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/skills/11-assets-svg-quality.md`: Contrato de calidad de assets SVG: iconografía vectorial Lucide con viewBox `0 0 24 24`, trazos de 1.8 px, terminaciones redondas, XML bien formado y ausencia de scripts maliciosos.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («QRC», «RCC», «viewBox», «DOM», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Correspondencia biunívoca 1:1*: Cada recurso en `ResourceQrcCatalog` cuenta con una entrada en `resources.qrc` y un archivo físico en `apps/vigilio-browser/assets/**`.
  2. *Resolución determinista de fallbacks*: Ante cualquier alias desconocido o dañado, el fallback a `icons/tools/tools.svg` (iconos) o `themes/light.json` (temas) es inmediato e infalible.
  3. *Local-First constitucional (Q-001)*: Cero peticiones de red para obtener o validar activos; todo dato es local.
  4. *Aislamiento Headless*: La ejecución de las pruebas unitarias y de integración no requiere servidor gráfico (X11/Wayland), GPU ni inicio de procesos Chromium/WebEngine.
  5. *Seguridad contra Path Traversal*: Ningún alias con secuencias de escape (`../`) o rutas absolutas de sistema es aceptado por el catálogo o el servicio de inclusión.
  6. *Thread-Safety absoluto*: Las consultas concurrentes sobre tablas estáticas inmutables son totalmente libres de bloqueos (`lock-free`) y no provocan condiciones de carrera.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo canónico inmutable y validador de sintaxis.
  - `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `.cpp`: Servicio de resolución y recuperación por fallback.
  - `apps/vigilio-browser/src/core/ResourceTypes.h`: Definiciones de descriptores y enumeraciones de error/estado.
  - `apps/vigilio-browser/resources.qrc`: Manifiesto canónico de 35 archivos.
  - `apps/vigilio-browser/assets/**`: 35 activos físicos (SVG, JSON, JPG).
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline continuo de compilación y pruebas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Flakiness o falsos negativos en pruebas multihilo por contención de recursos en entornos de CI limitados.
    *Mitigación*: Las consultas sobre el catálogo y los métodos de servicio operan sobre estructuras inmutables en memoria de solo lectura (`.rodata`), garantizando un comportamiento totalmente libre de bloqueos (`lock-free`).
  - *Riesgo*: Rutas relativas inconsistentes al acceder a `assets/` o `resources.qrc` desde diferentes directorios de trabajo de ejecución de pruebas.
    *Mitigación*: Inyección determinista de macros de compilación (`VIGILIO_ASSETS_DIR`, `VIGILIO_RESOURCES_QRC_PATH`) mediante CMake con rutas absolutas canónicas, complementadas con resolución fallback relativa controlada.
  - *Riesgo*: Dependencia inadvertida de servidores gráficos o display en entornos de integración continua headless.
    *Mitigación*: La suite `ResourceQrcInclusionTests.cpp` forma parte del núcleo desacoplado `vigilio_core`, operando puramente sobre la lógica de C++20 estándar sin instanciar componentes de ventana ni requerir X11/Wayland.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Pipeline de compilación y pruebas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (7/7 tests pasados en 0.15s, exit code 0):
    - `core_units`: Passed (0.01s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.06s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.04s)
    - `resource_qrc_catalog_tests`: Passed (0.01s)
    - `resource_inclusion_service_tests`: Passed (0.03s)
  - Validación de formato: Clang-Format verificado sin advertencias ni modificaciones pendientes.
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 20/20 referencias con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0058-module.foundations.task0020.propose.lock.json --project-root .` superó 13/13 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que no existen decisiones abiertas que bloqueen foundations ni resource qrc inclusion (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo aplica al hito final de lanzamiento público).
- **Handoff**:
  - La propuesta técnica para la etapa `tests` de `module.foundations.task0020` queda formalizada, coherente y lista en `openspec/changes/module-foundations-task0020/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY (`module.foundations.task0020.apply`).
