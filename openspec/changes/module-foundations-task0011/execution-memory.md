# Execution Memory — module-foundations-task0011

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `Qt dependency validation` en su etapa `ui` (interacción en Qt Widgets, contratos visuales, pre-validación de arranque en `app.cpp`, ausencia formal de cambio visual directo conforme a `docs/UX-COPY-PUBLICO.md`, estabilidad de la «Interfaz más fácil de usar» y gobernanza de pruebas de UI), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `d85a367da646cc3179147765a210cd0067075e4c99858afd93578e423fa612df`
  - Lock SHA-256: `47cdd265fcc52cd2200a7c679a6423e12394fc9dfa7c2ed6921d433cfed0fd06`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y especificaciones del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID: responsabilidad única en el validador de dependencias desacoplado de la interfaz gráfica; interfaces pequeñas y enfocadas; cero duplicación de estilos o lógica de validación (DRY); simplicidad y mantenimiento sin hilos ni procesos extra injustificados (KISS/YAGNI).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: pruebas unitarias puras en `CoreTests.cpp`; arnés de interfaz gráfica condicional en `WindowTests.cpp`; aislamiento headless; registro honesto y transparente de `NOT_EXECUTED` para pruebas de UI cuando falten paquetes de desarrollo Qt.
- `docs/skills/03-codigo-cpp-qt.md`: Buenas prácticas C++20 + Qt: núcleo de negocio en `src/core/` desacoplado de widgets; sin bloqueo del hilo de interfaz; uso estricto de RAII, const-correctness y tipos estándar C++20; propiedad (ownership) clara de objetos Qt.
- `docs/skills/04-ux-ui-senior.md`: Estilo visual moderno, sobrio y profesional sin brillos artificiales; declaración contractual de ausencia de cambio visual directo para la capacidad `Qt dependency validation`; acciones primarias visibles y mensajes de error comprensibles.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal: navegación por teclado completa (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`), foco visible, contraste adecuado, nombres accesibles (`setAccessibleName`) y controles ampliados (tap target >= 48px y tipografía 14pt) en la «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de ejecución: pre-validación de dependencias en < 2 ms; consumo de memoria < 100 KB RAM; cero procesos Chromium WebEngine iniciados durante la verificación; prevención estricta de procesos zombies.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta: cero peticiones de red externas, telemetría o servicios cloud; validación confinada a variables locales del sistema operativo; persistencia en SQLite local.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico riguroso de compilación; mensajes estructurados en stderr ante fallos de runtime; evidencia auditable de comandos ejecutados y códigos de salida; formateo conforme con ClangFormat.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado y validación de prerrequisitos de sistema mediante la opción `VIGILIO_REQUIRE_QT`.
- `docs/skills/11-assets-svg-quality.md`: Preservación de la iconografía vectorial SVG (viewBox 0 0 24 24, trazo 1.8 px, terminaciones redondeadas) y referencias en `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer nombres técnicos («Qt dependency validation», «Qt6», «QPA», «WebEngineWidgets», «Senior Mode», «Modo Senior», «CMake», «DAG», «Prompt Builder», «OpenSpec») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar».
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Versión mínima requerida: Qt >= 6.10.0 para todos los componentes gráficos (`Core`, `Widgets`, `WebEngineWidgets`).
  2. Desacoplamiento headless absoluto: `vigilio_core` (incluido `QtDependencyValidator`) y `vigilio_storage` tienen cero dependencias hacia Qt, garantizando compilación y pruebas 100% en entornos sin servidor gráfico.
  3. Gobernanza de configuración: conmutador `VIGILIO_REQUIRE_QT=OFF` permite degradación suave a build headless cuando falta Qt en desarrollo local; `VIGILIO_REQUIRE_QT=ON` fuerza error fatal en entornos de release/packaging.
  4. Inmunización de cabeceras externas: todos los directorios de inclusión de Qt se declaran como `SYSTEM` en CMake, impidiendo que advertencias de terceros interfieran con `-Werror`.
  5. Ausencia formal de cambio visual directo: la capacidad no añade componentes gráficos ni expone jerga técnica a los usuarios finales (cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`).
  6. Manejo seguro de anomalías de display: si falta display gráfico en Linux, la aplicación aborta antes de `QApplication` con código 1 y mensaje en `stderr`, evitando caídas por `SIGSEGV`.
  7. Filosofía local-first estricta: cero conexiones de red para descarga de binarios o telemetría.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h`
  - `apps/vigilio-browser/src/core/QtDependencyValidator.cpp`
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de bibliotecas de desarrollo de Qt6 en entornos CI mínimos o contenedores headless.
    *Mitigación*: Preservación estricta de la ruta de compilación headless (`VIGILIO_BUILD_GUI=OFF` y preset `core-debug`), asegurando que las suites de prueba de dominio y persistencia pasen al 100% sin requerir Qt.
  - *Riesgo*: Incompatibilidad binaria (ABI mismatch) entre la versión de cabeceras compilada y las bibliotecas dinámicas compartidas del sistema.
    *Mitigación*: Verificación en tiempo de ejecución de `qVersion()` frente a `QT_VERSION_STR` en `app.cpp` antes de instanciar ventanas, abortando con mensaje claro en `stderr` si existe divergencia mayor.
  - *Riesgo*: Lanzamiento accidental de la GUI en entornos Linux sin servidor gráfico (sin `DISPLAY`/`WAYLAND_DISPLAY`), provocando crash de Qt.
    *Mitigación*: Detección temprana en runtime de variables de display en `validateRuntimeEnvironment` y salida controlada por `stderr` con código 1.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 11, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0031-module.foundations.task0011.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0031-module.foundations.task0011.propose.lock.json", "valid": true, "checks": [...]}` (13/13 comprobaciones pasadas exitosamente)
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando exclusivamente el hito final de empaquetado `release.public-release`.
- **Verificación de construcción y pruebas headless**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: Compilación Ninja limpia sin advertencias bajo `-Werror`, 3/3 tests pasados (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.24s.
  - Código de salida: `0`
- **Verificación de calidad de formato de código (Clang-Format)**:
  - Comando: `find apps/vigilio-browser/src apps/vigilio-browser/tests -name '*.cpp' -o -name '*.h' | while read -r file; do clang-format --dry-run --Werror "$file"; done`
  - Salida: Cero diferencias, exit code 0.
- **Estado de artefactos OpenSpec (`openspec/changes/module-foundations-task0011/`)**:
  - `canonical-reference.yaml`: validado conforme al esquema v1 de Prompt Builder.
  - `proposal.md`: redactado y completado con alcance, out of scope, contexto, diseño, pasos APPLY, DoD y commit propuesto.
  - `design.md`: redactado y completado con CURRENT/TARGET/GAPS, diseño de interacción, contratos visuales, accesibilidad y matriz de pruebas trazables.
  - `tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-007 con criterios de verificación y comandos ejecutables.
  - `execution-memory.md`: redactado y completado con memoria técnica y handoff.
- **Handoff**:
  - La propuesta para `module.foundations.task0011` (etapa `ui` de `Qt dependency validation`) está completamente elaborada, es coherente, respeta todos los contratos canónicos y está lista para la revisión y aprobación humana antes de proceder a la fase APPLY.
