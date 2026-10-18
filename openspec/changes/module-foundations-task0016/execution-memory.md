# Execution Memory — module-foundations-task0016

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `WebEngine version compatibility` en su etapa `tests` (especificación de la suite de pruebas automatizadas deterministas, focalizadas y exhaustivas en C++20 para casos positivos, entradas inválidas, desbordamiento, ordenamiento total, matrices de capacidades funcionales, perfiles de compatibilidad, permisos de proceso asistente sintético mediante RAII, pre-validación de arranque, degradación suave y estrés concurrente multihilo sin procesos zombis de Chromium ni dependencias de red), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0016-webengine-version-compatibility/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 plataformas x64/arm64)
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `f9c21835184b9e59483bc6f26ae38de157451874cbce279217edef9a602cfe7c`
  - Lock SHA-256: `a00afcc9bf68faa23bf1a575f29ba0c5ed12b6dbc1c492c6a2358eb9fa4acef5`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y directrices canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación rigurosa de SOLID, DRY y KISS en el diseño de pruebas; cada función de prueba evalúa una única responsabilidad sin acoplamiento colateral ni duplicación de fixtures sintéticos.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: pruebas unitarias puras y deterministas para lógica y contratos de compatibilidad; uso exclusivo de fixtures locales sintéticos; prohibición absoluta de sleeps fijos; reporte explícito de estados; pruebas E2E solo para flujos críticos y programadas de forma estrictamente serial (`RUN_SERIAL TRUE`).
- `docs/skills/03-codigo-cpp-qt.md`: Uso de C++20 moderno (`operator<=>`, `std::nullopt`, `std::string_view`, RAII estricto con `TempHelperFileGuard`), inmutabilidad de reportes (`const-correctness`), ausencia de llamadas bloqueantes en el hilo principal y separación de responsabilidades entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Ausencia total de cambio visual directo en la etapa `tests`; verificación de que los contratos visuales existentes de `BrowserWindow` y los estados de error no se degraden.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Garantía de que la infraestructura de validación preserve los principios de la «Interfaz más fácil de usar» y no exponga terminología técnica en interfaces visibles.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de ejecución extrema: suite unitaria ejecutada en < 50 ms en CTest, huella de memoria < 100 KB RAM, contención cero en concurrencia (latencia < 1 ms por validación) y cero procesos zombis de Chromium.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta (decisión Q-001): cero peticiones de red, cero consultas de telemetría remota, sin persistencia de URLs completas en reportes diagnósticos y limpieza segura de archivos sintéticos en `/tmp`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto C++20 verificado con Clang-Format, compilación con `-Werror`, diagnósticos estructurados y evidencia reproducible con comandos y códigos de salida explícitos.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta entre ciclo de vida PROPOSE (diseño completo sin modificar código de aplicación) y APPLY (implementación del incremento específico tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Respeto por la coherencia visual e identidad de assets del producto sin modificaciones en esta etapa.
- `docs/UX-COPY-PUBLICO.md`: Prohibición estricta de exponer conceptos de ingeniería interna («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «renderProcessTerminated», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec») en interfaces públicas.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales del producto objetivo y logotipo oficial.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Baseline canónico de Chromium: el hito mínimo soportado es 128 (`ChromiumVersion{128, 0, 0, 0}`), asociado a Qt >= 6.10.0 según la decisión Q-002.
  2. Pureza funcional y desacoplamiento: el núcleo de validación en `vigilio_core` no incluye cabeceras de Chromium ni enlaza con `Qt6::WebEngineWidgets`, ejecutándose 100% en modo headless sin entorno gráfico.
  3. Política Zero-Zombie: ninguna prueba unitaria iniciará procesos Chromium reales (`QtWebEngineProcess`). Los helpers ejecutables sintéticos se crean como scripts mínimos temporales y se destruyen inmediatamente mediante RAII (`TempHelperFileGuard`).
  4. Resiliencia ante entradas malformadas: cadenas vacías, caracteres alfanuméricos en posiciones numéricas o desbordamientos enteros retornan `std::nullopt` sin arrojar excepciones no controladas ni provocar fallos de segmentación.
  5. Concurrencia libre de bloqueos y carreras: los métodos de validación son funciones puras sin estado mutable global, permitiendo invocaciones simultáneas desde múltiples hilos con latencia < 1 ms por llamada.
  6. Filosofía local-first: cero peticiones de red externa y sin telemetría remota (decisión Q-001).
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Condiciones de carrera o inestabilidad en pruebas multihilo en entornos con pocos núcleos de CPU.
    *Mitigación*: Métodos de validación puramente inmutables; cada hilo opera con variables locales en pila, garantizando compatibilidad con TSan y cero contención.
  - *Riesgo*: Artefactos temporales huérfanos en `/tmp` tras aserciones fallidas o interrupciones de prueba.
    *Mitigación*: Uso obligatorio del patrón RAII `TempHelperFileGuard`, ejecutando `cleanup()` tanto en el constructor como en el destructor de forma no-except.
  - *Riesgo*: Falsa integración si las pruebas intentan conectarse a la red para verificar Chromium.
    *Mitigación*: Todo el catálogo de versiones y matrices de capacidades es sintético, inmutable y evaluado localmente en espacio de usuario.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Compilación local: Ninja configurado con éxito (`CMakeLists.txt`, C++20, `-Werror`, `VIGILIO_BUILD_GUI=OFF`).
  - Suite de pruebas automatizadas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (5/5 tests pasados en 0.05s, exit code 0):
    - `core_units`: Passed (0.00s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.04s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.00s)
  - Formato de código: Clang-Format verificado en todos los archivos de `src/` y `tests/` sin discrepancias (`exit code 0`).
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 16/16 referencias canónicas con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0046-module.foundations.task0016.propose.lock.json --project-root .` superó 14/14 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que ninguna decisión bloquea foundations ni webengine (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo bloquea release pública).
- **Handoff**:
  - La propuesta para `module.foundations.task0016` queda completa, coherente y lista en `openspec/changes/module-foundations-task0016/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY.
