# Change Proposal — module-foundations-task0059

Pack: `module.foundations.task0059`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **CI macOS job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0059 — Implementar la interacción y estados visuales en Qt: CI macOS job`).

Habiéndose completado y certificado formalmente en las capacidades e hitos precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de advertencias y diagnósticos estrictos (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política de recuperación ante caídas (`task0029`-`task0032`), los CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures (`task0037`-`task0040`), la configuración canónica y servicio de Clang-Format (`task0041`-`task0044`), la configuración canónica y servicio de Clang-Tidy (`task0045`-`task0048`), la especificación, servicio y validación de UI para CI Linux job (`task0049`-`task0052`), la especificación, servicio y validación de UI para CI Windows job (`task0053`-`task0056`), y habiéndose formalizado el modelo de dominio de CI macOS (`task0057`, commit `c8287be`) e implementado el servicio operativo `vigilio::CiMacOsJobService` en C++20 con orquestación topológica de 8 etapas, modo simulación dry-run, política fail-fast, tolerancia selectiva a fallos (`continueOnError`), cancelación cooperativa multihilo bajo la política Zero-Zombie, saneamiento seguro de directorios con defensas específicas de macOS (confinamiento en rutas relativas autorizadas, rechazo de Directory Traversal `..` / `../`, restricción frente a rutas del sistema host como `/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`, y eliminación/rechazo de artefactos o bifurcaciones de Apple como `.DS_Store` o `._*`) y síntesis CLI reproducible para toolchains macOS (`task0058`, commit `6b25d9d`), esta propuesta proyecta la **gobernanza de interacción en Qt Widgets, contratos visuales, tolerancia headless, arnés de pruebas de interfaz de usuario (`vigilio_ui_tests` / `WindowTests.cpp`), validación de cobertura de construcción de UI y bundle nativo en el pipeline de CI macOS, y auditoría exhaustiva de textos públicos y diagnósticos para CI macOS job**.

El objetivo central de este pack es garantizar que la infraestructura de integración continua en macOS gobierne y certifique de manera determinista la integridad de compilación, empaquetado del bundle `.app` (`VigilioBrowser.app/Contents/MacOS` e `Info.plist`) y recursos de los componentes visuales de escritorio (`src/ui/BrowserWindow.h`, `src/ui/BrowserWindow.cpp`, `resources.qrc`, `branding/vigilio-logo.svg`) y de sus suites de pruebas (`tests/ui/WindowTests.cpp`), manteniendo la estricta política de lenguaje público de producto `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de desarrollo o ejecución de pipelines a la ventana principal de navegación (`BrowserWindow`), sin introducir botones, menús, consolas de compilación ni diálogos de depuración en la interfaz visible del usuario final, y asegurando tolerancia completa a entornos sin servidor gráfico (Zero-Zombie)**.

El alcance concreto comprende:
- **Declaración contractual de UX/UI y «sin cambio visual directo en la interfaz pública»**:
  - Declaración formal vinculante de que la capacidad **CI macOS job** es un subsistema de automatización de compilación, empaquetado del bundle nativo `.app`, pruebas continuas y gobernanza de calidad en entornos macOS (Apple Silicon ARM64, Intel x86_64 y binarios universales); por consiguiente, **no introduce widgets, botones de ejecución de pipelines, paneles de compilación, menús de CI/CD ni consolas de terminal en la interfaz visible de navegación de producción (`BrowserWindow`)**.
  - Cumplimiento riguroso de `docs/UX-COPY-PUBLICO.md`: proscripción absoluta de exponer términos de ingeniería interna como «CI macOS job», «Pipeline», «Build Step», «CMake Presets», «Compiler», «AppleClang», «HomebrewClang», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» en menús principales, barras de herramientas o ventanas visibles al usuario común.
  - Verificación contractual de que todos los controles de `apps/vigilio-browser/src/ui/BrowserWindow.cpp` (pestañas diferidas, barra de navegación, barra de direcciones, menús, accesibilidad y páginas nativas de inicio/error) respeten rigurosamente las directrices de diseño de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales oficiales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Integración de `CiMacOsJobService` y `CiMacOsJobValidator` en el arnés de pruebas de interfaz gráfica (`WindowTests.cpp`)**:
  - Incorporación en la suite de pruebas Qt Test (`vigilio_ui_tests` / `WindowTests.cpp`) de un bloque de verificación programática que utiliza `vigilio::CiMacOsJobService`, `vigilio::CiMacOsJobValidator`, `vigilio::CiMacOsJobConfiguration` y `vigilio::CiMacOsJobTypes.h`:
    - Ejecución en modo simulación/dry-run (`CiMacOsExecutionMode::DryRun`) del plan canónico de depuración (`canonicalDebugPlan()`), certificando que el pipeline de CI macOS evalúa y orquesta secuencialmente las etapas fundamentales (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`) sin errores y emite un `CiMacOsJobReport` en estado `Passed`, con `isClean == true`, duración $\ge 0$ ms y error `None`.
    - Evaluación de plan comprensivo con las 8 etapas canónicas completas de macOS (incluyendo `SanitizerVerification` y `BundleVerification`), certificando que la orquestación secuencial se ejecuta satisfactoriamente en modo dry-run.
    - Validación programática de que las etapas de compilación, pruebas y verificación de bundle del plan de CI macOS gobiernan y verifican los artefactos y componentes de interfaz de usuario (`vigilio_ui_tests`, `BrowserWindow.cpp`, recursos `resources.qrc`, layout del bundle nativo `.app`), asegurando que ningún cambio de UI quede fuera de la cobertura del pipeline en plataformas macOS.
    - Auditoría de lenguaje público de las funciones de diagnóstico y estado de CI macOS en `CiMacOsJobService` (`executionModeToString`, `formatStepSummary`) y `CiMacOsJobValidator` (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`), comprobando que emplean estrictamente lenguaje natural en español neutro comprensible para cualquier usuario, sin jerga de ingeniería interna ni términos prohibidos por `docs/UX-COPY-PUBLICO.md`.
    - Aislamiento total de las operaciones de validación de pipeline en UI: uso exclusivo de `DryRun` en memoria y lectura segura sin ejecutar llamadas externas destructivas ni alterar archivos durante la ejecución de las pruebas de UI.
- **Auditoría y preservación de contratos visuales en `BrowserWindow`**:
  - Verificación exhaustiva de los 8 estados interactivos fundamentales en los componentes de la interfaz de usuario:
    - `idle`: presentación limpia y sobria según `/resultado/01-vigilio-home.png`, fondo azul oscuro sutil sin brillos artificiales.
    - `hover`: realce discreto en botones de acción y pestañas de navegación.
    - `focus`: contorno visible accesible de 2 px para navegación asistida por teclado.
    - `disabled`: atenuación visual accesible en acciones inactivas (e.g. historial atrás/adelante sin entradas).
    - `loading`: indicador textual comprensible en pestaña y barra de estado («Cargando…»).
    - `empty`: página de inicio nativa (`newHomePage` / widget `vigilio-home`) con logo vectorial oficial SVG (`branding/vigilio-logo.svg`), título y buscador centralizado, sin inicializar instancias innecesarias de `QWebEngineView`.
    - `error`: contenedor nativo accesible (`vigilio-page-error`) con icono Lucide de alerta, título «No se pudo cargar la página», mensaje descriptivo y botones de acción primaria («Reintentar») y secundaria («Ir al inicio»).
    - `success`: confirmación visual accesible en acciones completadas (e.g. persistencia de marcadores o restauración de sesión).
- **Preservación y escalado dinámico para «Interfaz más fácil de usar» (Modo Senior)**:
  - Verificación de la adaptación visual accesible en todos los controles de la ventana:
    - Escalado dinámico de iconos de barra de herramientas (de 22×22 px a 32×32 px).
    - Dimensiones mínimas de toque y clic (tap targets $\ge 48$ px en botones `QPushButton` y campos `QLineEdit`; barra de herramientas $\ge 55$ px; banner de recuperación $\ge 56$ px).
    - Aumento tipográfico (+4 pt) manteniendo legibilidad sin solapamientos ni truncamientos de texto.
  - Mapeo universal de atajos de teclado accesibles: `Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+B`, `Ctrl+Q`, `Alt+Home`, `Alt+F`.
  - Atributos completos `accessibleName` y `accessibleDescription` en lenguaje natural en español neutro para lectores de pantalla (VoiceOver en macOS, Orca en Linux, NVDA/Narrator en Windows).
- **Tolerancia a entornos headless y garantía Zero-Zombie**:
  - Detección determinista en tiempo de ejecución de la presencia de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY` en Linux/X11/Wayland, o sesión de ventanas de Quartz/WindowServer en macOS), reportando con transparencia y rigor técnico `NOT_EXECUTED` (exit code 0) en entornos de integración continua headless en lugar de provocar fallos en cascada o generar mocks que falseen la integración visual.
  - Tiempos de ejecución acotados (< 200 ms) para suites de interfaz en CTest, cancelación cooperativa inmediata con `CancellationToken` y garantía absoluta de cero procesos huérfanos o colgados esperando EOF.
- **Desacoplamiento arquitectónico estricto**:
  - La infraestructura de CI macOS (`CiMacOsJobService`, `CiMacOsJobValidator`, `CiMacOsJobConfiguration`, `CiMacOsJobTypes.h`) permanece 100% confinada en `src/core`, sin dependencias hacia `QWidget`, `QMainWindow` ni `QApplication`.
  - La capa gráfica (`BrowserWindow`) permanece en `src/ui`, dedicada exclusivamente a la experiencia de usuario y ajena a la orquestación de compilaciones o pipelines de CI.
  - El arnés de prueba `WindowTests.cpp` en `tests/ui` actúa como consumidor de ambas capas, orquestando `vigilio_core` para validar la calidad, robustez de pipeline y contratos de textos de `vigilio_ui`.

---

## Out of scope

- Implementación física de consolas de compilación, paneles de CI/CD, visores de logs de terminal o botones de ejecución de pipelines dentro de la ventana de navegación pública de Vigilio Browser (la capacidad es infraestructura de automatización de desarrollo y calidad, no una herramienta para desarrolladores o IDE).
- Modificación directa del código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante la fase APPLY tras aprobación humana).
- Creación de botones, barras de herramientas o diálogos de CI visibles al usuario común (terminantemente proscrito por `docs/UX-COPY-PUBLICO.md`).
- Baterías pesadas de pruebas de integración de compilación en vivo sobre hardware macOS físico distribuido, firma de código con certificados de Apple Developer (codesigning / notarization) o empaquetado masivo de instaladores `.dmg` (reservadas para la microtarea `0060`).
- Conexión con servicios externos en la nube (GitHub Actions, Azure DevOps, servidores remotos) o transmisión de telemetría (incompatible con la política Local-First constitucional, Decisión Q-001 / ADR-001).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Especificación de dominio e implementación de servicio de CI macOS completadas (`task0057` y `task0058`)**:
   - `src/core/CiMacOsJobTypes.h`: Taxonomía fuertemente tipada de etapas (`CiMacOsPipelineStep`: `EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `BundleVerification`), estados de trabajo (`CiMacOsJobStatus`), estados de etapa (`CiMacOsStepStatus`), códigos de error (`CiMacOsJobError`: `None`, `ToolchainMissing`, `MacOsSdkMissing`, `XcodeToolsMissing`, `ConfigurationFailed`, `CompilationFailed`, `TestExecutionFailed`, `FormatDiscrepancy`, `StaticAnalysisIssue`, `UnsafePipelinePath`, `TimeoutExceeded`, `InvalidWorkflowSchema`, `DependencyOrderingViolation`, `MemoryLimitExceeded`, `BundleVerificationFailed`), arquitecturas (`CiMacOsArchitecture`: `ARM64`, `X86_64`, `Universal`), compiladores (`CiMacOsCompiler`: `AppleClang`, `HomebrewClang`), modos de ejecución (`CiMacOsExecutionMode`: `DryRun`, `Simulated`, `Live`), opciones de servicio (`CiMacOsServiceOptions`), definiciones de etapa (`CiMacOsStepDefinition`), plan de trabajo (`CiMacOsJobPlan`), reporte inmutable consolidado (`CiMacOsJobReport`) y token cooperativo (`CancellationToken`).
   - `src/core/CiMacOsJobConfiguration.h` y `.cpp`: Planes canónicos oficiales (`canonicalDebugPlan`, `canonicalReleasePlan`), verificación de invariantes del sistema y listas de versiones de macOS soportadas (macOS 14 Sonoma, macOS 13 Ventura, macOS 15 Sequoia).
   - `src/core/CiMacOsJobValidator.h` y `.cpp`: Validación de planes y secuencias topológicas (`validatePlan`, `validateStepSequence`), validación de entorno (`validateEnvironment`), comprobación de seguridad de rutas APFS (`isSafeBuildPath` con defensas contra Directory Traversal `..`, caracteres nulos, rutas host `/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`, y artefactos `.DS_Store`, `._*`) y funciones de conversión y textos en español neutro (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`).
   - `src/core/CiMacOsJobService.h` y `.cpp`: Capa operativa de servicio desacoplada implementada en C++20 (commit `6b25d9d`) con orquestación secuencial (`executePlan`), ejecución granular (`executeStep`), simulación determinista (`DryRun`), cancelación cooperativa multihilo Zero-Zombie, saneamiento seguro de directorios (`cleanBuildDirectory` con reintentos exponenciales ante contención de bloqueos), síntesis CLI reproducible (`buildCommandLine`) y diagnósticos públicos (`executionModeToString`, `formatStepSummary`).
   - `tests/unit/CiMacOsJobConfigurationTests.cpp`: 11 pruebas unitarias de dominio aprobadas.
   - `tests/unit/CiMacOsJobServiceTests.cpp`: 12 pruebas unitarias operativas de servicio aprobadas.
2. **Arnés de pruebas de interfaz gráfica existente (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
   - 1304 líneas de código en Qt Test (`vigilio_ui_tests` / `window_ui_tests`).
   - Verifica: inicialización de ventana, recuento de pestañas diferidas (lazy loading), logo corporativo SVG compilado en QRC, acciones de navegación con iconos Lucide, metadatos de accesibilidad (`accessibleName`), atajos de teclado, «Interfaz más fácil de usar» con escalado de iconos (22×22 a 32×32 px) y controles ($\ge 48$ px), contenedor de error nativo (`vigilio-page-error`), banner de modo seguro, banner de recuperación ante caídas (`vigilio-crash-recovery-banner`), primitivas de fixtures temporales (`TestFixtureFoundation`), y gobernanza de Clang-Format (`task0043`), Clang-Tidy (`task0047`), CI Linux job (`task0051`) y CI Windows job (`task0055`) en UI.
   - *Oportunidad de integración identificada*: `WindowTests.cpp` no cuenta aún con una validación programática que certifique que el pipeline de CI macOS (`CiMacOsJobService`) incluye y gobierna adecuadamente la compilación de la capa de interfaz gráfica y la estructura del bundle nativo `.app` en entornos macOS, ni audita en el contexto de UI que las cadenas de diagnóstico y remediación pública de CI macOS respeten estrictamente `docs/UX-COPY-PUBLICO.md` en español neutro.
3. **Punto de integración actual del sistema de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_ui_tests` enlaza `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test`.
   - Dado que `vigilio_core` ya compila y exporta `CiMacOsJobService`, `CiMacOsJobValidator`, `CiMacOsJobConfiguration` y `CiMacOsJobTypes.h`, el ejecutable de pruebas de interfaz de usuario tiene acceso directo e inmediato a la infraestructura de CI macOS sin requerir dependencias adicionales ni reconfiguración de bibliotecas.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GNU GCC 15.2.0 bajo C++20 con `-Werror` y cero advertencias.
   - 25 de 25 pruebas pasando exitosamente en CTest con 100% de éxito en ~6.13s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.07s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.04s)
     - `resource_qrc_catalog_tests`: Passed (0.06s)
     - `resource_inclusion_service_tests`: Passed (0.13s)
     - `resource_qrc_inclusion_tests`: Passed (0.27s)
     - `startup_bootstrap_tests`: Passed (0.31s)
     - `application_lifetime_tests`: Passed (0.21s)
     - `crash_recovery_tests`: Passed (0.03s)
     - `cmake_presets_tests`: Passed (0.04s)
     - `cmake_presets_service_tests`: Passed (0.11s)
     - `test_fixture_foundation_tests`: Passed (0.01s)
     - `test_fixture_service_tests`: Passed (0.11s)
     - `clang_format_configuration_tests`: Passed (0.01s)
     - `clang_format_service_tests`: Passed (2.90s)
     - `clang_tidy_configuration_tests`: Passed (0.01s)
     - `clang_tidy_service_tests`: Passed (0.16s)
     - `ci_linux_job_configuration_tests`: Passed (0.01s)
     - `ci_linux_job_service_tests`: Passed (0.14s)
     - `ci_windows_job_configuration_tests`: Passed (0.01s)
     - `ci_windows_job_service_tests`: Passed (0.77s)
     - `ci_macos_job_configuration_tests`: Passed (0.01s)
     - `ci_macos_job_service_tests`: Passed (0.68s)

---

## Alcance exacto

El alcance exacto de esta microtarea en la fase APPLY posterior comprenderá:
1. **Modificación de `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Inclusión de cabeceras canónicas:
     ```cpp
     #include "core/CiMacOsJobConfiguration.h"
     #include "core/CiMacOsJobService.h"
     #include "core/CiMacOsJobTypes.h"
     #include "core/CiMacOsJobValidator.h"
     ```
   - Adición del bloque de verificación programática para CI macOS job:
     - Instanciación de `CiMacOsJobService` con `canonicalDebugPlan()` en modo `CiMacOsExecutionMode::DryRun`.
     - Ejecución de `executePlan()` y comprobación de `status == CiMacOsJobStatus::Passed`, `isClean == true`, duración $\ge 0$ ms, y correspondencia de etapas evaluadas.
     - Definición y ejecución de plan comprensivo con las 8 etapas canónicas completas (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `BundleVerification`) comprobando que alcanza estado `Passed`.
     - Comprobación de que las etapas `BuildCompilation`, `TestSuiteExecution` y `BundleVerification` gobiernan y verifican los componentes visuales de la aplicación y el layout del bundle `.app` nativo.
     - Auditoría de lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`):
       - Modos de ejecución (`DryRun`, `Simulated`, `Live`) en español neutro.
       - Resumen estructurado de etapa (`formatStepSummary`).
       - Todos los estados de trabajo (`CiMacOsJobStatus`).
       - Todos los errores fuertemente tipados (`CiMacOsJobError`) y sus sugerencias de remediación (`generateRemediationHint`).
       - Todas las etapas del pipeline (`CiMacOsPipelineStep`).
       - Todos los estados de etapa (`CiMacOsStepStatus`).
       - Proscripción estricta de términos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «CI macOS job»).
2. **Sincronización documental de `docs/rules/01-modules/foundations/micro/0059-ci-macos-job/task.md`**:
   - Actualización de estado de microtarea y registro de trazabilidad de la etapa `ui`.

---

## Diseño completo de la capacidad

### Desacoplamiento arquitectónico y gobernanza de UI

```
┌────────────────────────────────────────────────────────────────────────┐
│                   apps/vigilio-browser/tests/ui                        │
│                         WindowTests.cpp                                │
│                                                                        │
│  ┌─────────────────────────────────┐  ┌─────────────────────────────┐  │
│  │   UI Interaction & Visual       │  │  CI Quality & Copy Audit    │  │
│  │   Contracts (BrowserWindow)     │  │  - CiLinuxJobService        │  │
│  │   - 8 Estados interactivos      │  │  - CiWindowsJobService      │  │
│  │   - «Interfaz más fácil de usar»│  │  - CiMacOsJobService        │  │
│  │   - Accesibilidad & Senior Mode │  │  - DryRun / Headless / Copy │  │
│  └────────────────┬────────────────┘  └──────────────┬──────────────┘  │
└───────────────────┼──────────────────────────────────┼─────────────────┘
                    │                                  │
                    ▼                                  ▼
┌─────────────────────────────────┐   ┌──────────────────────────────────┐
│           vigilio_ui            │   │           vigilio_core           │
│       src/ui/BrowserWindow      │   │     src/core/CiMacOsJobService   │
│  - Sin botones/consolas de CI   │   │  - Plan canónico 8 etapas        │
│  - Sin jerga de ingeniería      │   │  - Zero-Zombie cooperativo       │
│  - Interfaz pública limpia      │   │  - Copy español neutro           │
└─────────────────────────────────┘   └──────────────────────────────────┘
```

1. **Principio de no contaminación visual**:
   - `BrowserWindow` es la interfaz del navegador para el usuario final. No contiene ningún elemento relacionado con CI/CD, compiladores o ejecución de tests.
   - Las pruebas de UI actúan como el punto de convergencia donde se verifica tanto la experiencia de usuario de `BrowserWindow` como la gobernanza del pipeline de CI sobre los artefactos que conforman dicha interfaz.
2. **Gobernanza del pipeline sobre artefactos visuales**:
   - El pipeline canónico de macOS incluye `BuildCompilation` (compila `BrowserWindow.cpp` y recursos `resources.qrc`), `TestSuiteExecution` (ejecuta `vigilio_ui_tests`) y `BundleVerification` (certifica que el ejecutable y los recursos empaquetados en `VigilioBrowser.app` conservan la estructura esperada de macOS).
3. **Validación de lenguaje público en UI**:
   - Todo diagnóstico generado por el subsistema de CI macOS que pudiera presentarse ante una incidencia debe formularse en español neutro de acuerdo a `docs/UX-COPY-PUBLICO.md`, evitando jerga técnica o tecnicismos que confundan al usuario.

---

## Archivos afectados

| Archivo | Rol | Operación prevista en APPLY |
| --- | --- | --- |
| `openspec/changes/module-foundations-task0059/proposal.md` | OpenSpec Change Proposal | Autoría y refinamiento completo en PROPOSE |
| `openspec/changes/module-foundations-task0059/design.md` | OpenSpec Change Design | Autoría y refinamiento completo en PROPOSE |
| `openspec/changes/module-foundations-task0059/tasks.md` | OpenSpec Change Tasks | Tareas ordenadas y verificables en PROPOSE |
| `openspec/changes/module-foundations-task0059/execution-memory.md` | OpenSpec Execution Memory | Registro de invariantes, skills y evidencia |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Arnés de pruebas de UI | Integración de bloque de prueba de CI macOS job (APPLY) |
| `docs/rules/01-modules/foundations/micro/0059-ci-macos-job/task.md` | Especificación de microtarea | Sincronización de estado tras ejecución (APPLY) |

---

## Contrato UX/UI

### Principio de «Sin cambio visual directo en la interfaz pública»
- La capacidad **CI macOS job** no altera el árbol visual ni los menús de `BrowserWindow`.
- Se prohíbe la inserción de acciones de CI en la barra de herramientas, barra de menús o menú contextual de navegación.

### Preservación de los 8 estados interactivos de `BrowserWindow`
1. `idle`: Ventana limpia, fondo azul oscuro sutil sin brillos artificiales, barra de navegación visible con botones estándar.
2. `hover`: Efecto hover discreto en botones `QPushButton` y pestañas `QTabBar` (cambio de fondo de 10% de opacidad sin alterar dimensiones).
3. `focus`: Contorno visible accesible de 2 px en color de contraste para navegación secuencial con tabulador.
4. `disabled`: Deshabilitación visual accesible con opacidad reducida en acciones no disponibles (adelante/atrás en historial vacío).
5. `loading`: Indicador accesible de carga en pestañas y barra de estado («Cargando…») sin bloquear el hilo principal.
6. `empty`: Página de inicio nativa (`newHomePage` / widget `vigilio-home`) con isotipo Lucide/SVG oficial y buscador centralizado.
7. `error`: Contenedor nativo `vigilio-page-error` con icono SVG Lucide de alerta, título explicativo en español («No se pudo cargar la página») y acciones de recuperación («Reintentar», «Ir al inicio»).
8. `success`: Notificación visual no invasiva ante acciones completadas (guardado de marcadores, restauración de pestañas).

### Preservación de «Interfaz más fácil de usar» (Modo Senior)
- **Dimensiones táctiles y de ratón mínimas**:
  - Botones y campos de texto con altura $\ge 48$ px.
  - Barra de herramientas con altura $\ge 55$ px.
  - Banner de recuperación ante caídas con altura $\ge 56$ px.
  - Escalado de iconos vectoriales SVG de 22×22 px a 32×32 px.
- **Tipografía y contraste**:
  - Incremento tipográfico de +4 pt manteniendo proporciones y sin truncamientos.
  - Contraste visual conforme a WCAG AA en todas las superficies.
- **Navegación asistida y atajos**:
  - Atajos universales: `Ctrl+T` (nueva pestaña), `Ctrl+W` (cerrar pestaña), `Ctrl+L` (foco en barra de direcciones), `Ctrl+D` (marcador), `Ctrl+B` (barra de marcadores), `Ctrl+Q` (salir), `Alt+Home` (página de inicio), `Alt+F` (menú principal).
  - Atributos `accessibleName` y `accessibleDescription` en todos los controles para VoiceOver en macOS.

---

## Reglas y fallos

1. **Invariante Local-First (ADR-001 / Decisión Q-001)**:
   - Ninguna operación de validación de pipeline en UI o servicio realiza peticiones HTTP/HTTPS ni conecta a la nube.
2. **Invariante Zero-Zombie**:
   - Todo bucle o proceso en pruebas cuenta con un timeout explícito. La cancelación cooperativa con `CancellationToken` detiene inmediatamente la ejecución.
3. **Tolerancia Headless**:
   - Si no hay servidor gráfico disponible (`DISPLAY` o `WAYLAND_DISPLAY` ausentes), las pruebas de UI retornan código 0 emitiendo el mensaje honesto `NOT_EXECUTED`, sin fingir ejecuciones gráficas con mocks vacíos.
4. **Manejo de condiciones anómalas**:
   - *Plan vacío o parámetros inválidos*: `CiMacOsJobValidator::validatePlan` retorna `InvalidWorkflowSchema`.
   - *Exceso de memoria*: Retorna `MemoryLimitExceeded` si supera los 8 GB.
   - *Ruta insegura*: Retorna `UnsafePipelinePath` si detecta secuencias `..`, rutas del host macOS (`/System`, `/Library`, etc.) o artefactos (`.DS_Store`, `._*`).
   - *Orden topológico incorrecto*: Retorna `DependencyOrderingViolation` si se intenta compilar antes de configurar o probar antes de compilar.
   - *Fallo de verificación de bundle*: Retorna `BundleVerificationFailed` si el layout del paquete `.app` no satisface los requisitos nativos de macOS.

---

## Seguridad y privacidad

- **Límites de confianza y aislamiento**:
  - `vigilio_core` no manipula credenciales, tokens ni datos personales.
  - El arnés de prueba `WindowTests.cpp` opera con fixtures en memoria o directorios temporales aislados creados con `TestFixtureFoundation`.
- **Confinamiento de rutas**:
  - Todas las operaciones de compilación y empaquetado en macOS se restringen a rutas de compilación seguras relativas (`build-ci-macos-*`, `_build*`).
- **Privacidad y ausencia de telemetría**:
  - Cero recopilación de métricas de usuario o telemetría remota.

---

## Matriz de pruebas

| ID | Criterio de aceptación | Fixture / Vector de prueba | Comando de ejecución | Resultado esperado | Fallo negativo / Condición de rechazo | Categoría |
| --- | --- | --- | --- | --- | --- | --- |
| `TEST-001` | Declaración formal de «sin cambio visual directo» | Inspección de `BrowserWindow.h` y `BrowserWindow.cpp` | `! grep -E "CiMacOs|ci-macos|CiPipeline|BuildStep|Senior Mode|Prompt Builder|OpenSpec" apps/vigilio-browser/src/ui/BrowserWindow.*` | Cero coincidencias de widgets o terminología de CI | Existencia de botones o paneles de CI en la ventana pública | `APPLY` |
| `TEST-002` | Inclusión de cabeceras de CI macOS en arnés de UI | `tests/ui/WindowTests.cpp` | `grep -E "#include \"core/CiMacOsJobService\.h\"" apps/vigilio-browser/tests/ui/WindowTests.cpp` | Cabecera incluida y accesible | Cabecera ausente o error de inclusión | `APPLY` |
| `TEST-003` | Validación programática en dry-run del plan de depuración | `canonicalDebugPlan()` en `CiMacOsExecutionMode::DryRun` | `vigilio_ui_tests` (o CTest) | Reporte con status `Passed`, `isClean == true`, duración $\ge 0$ ms | Estado distinto de `Passed` o fallo de ejecución | `APPLY` |
| `TEST-004` | Validación de plan comprensivo de 8 etapas | Plan con 8 etapas incluyendo `SanitizerVerification` y `BundleVerification` | `vigilio_ui_tests` (o CTest) | Reporte con status `Passed`, 8 etapas evaluadas | Fallo en orquestación o etapas omitidas indebidamente | `APPLY` |
| `TEST-005` | Cobertura del pipeline sobre la capa de UI | Inspección de `debugPlan.steps` en `WindowTests.cpp` | `vigilio_ui_tests` (o CTest) | Presencia confirmada de `BuildCompilation` y `TestSuiteExecution` | Pipeline que no gobierna compilación ni pruebas de UI | `APPLY` |
| `TEST-006` | Auditoría de textos de servicio en español neutro | `executionModeToString` y `formatStepSummary` | `vigilio_ui_tests` (o CTest) | Cadenas no vacías, mención de «simulada/simulación» y «ejecución» | Cadenas vacías o con términos en inglés | `APPLY` |
| `TEST-007` | Auditoría de estados y errores de validador | `jobStatusToString`, `jobErrorToString`, `generateRemediationHint` | `vigilio_ui_tests` (o CTest) | Mensajes descriptivos en español neutro | Cadenas vacías o mensajes genéricos sin contexto | `APPLY` |
| `TEST-008` | Proscripción de términos internos prohibidos en UI | Recorrido de todas las cadenas públicas de CI macOS | `vigilio_ui_tests` (o CTest) | Cero ocurrencias de «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «CI macOS job» | Detección de algún término prohibido | `APPLY` |
| `TEST-009` | Tolerancia a entornos headless | Ejecución sin `DISPLAY` ni `WAYLAND_DISPLAY` | `WindowTests` en consola | Emisión de mensaje honesto y exit code 0 (`NOT_EXECUTED`) | Crash con `SIGSEGV` o fallo por falta de display | `APPLY` |
| `TEST-010` | Escalado y dimensiones de «Interfaz más fácil de usar» | Activación programática de modo senior en `BrowserWindow` | `vigilio_ui_tests` | Altura de botones $\ge 48$ px, toolbar $\ge 55$ px, iconos $32\times 32$ px | Controles pequeños o solapamiento tipográfico | `APPLY` |
| `TEST-011` | Navegación por teclado y accesibilidad de interfaz | Simulación de eventos de teclado (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`) | `vigilio_ui_tests` | Foco visible y disparo de acciones correspondientes | Atajos inactivos o foco invisible | `APPLY` |
| `TEST-012` | Verificación de construcción y CTest global | Ejecución de suite completa de pruebas del proyecto | `bash apps/vigilio-browser/scripts/check.sh` | 100% pruebas aprobadas (25+ tests) con compilación limpia `-Werror` | Fallo de compilación o test en rojo | `APPLY` |

---

## Pasos APPLY

1. **Paso 1: Modificar `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Añadir los includes canónicos de `CiMacOsJobService.h`, `CiMacOsJobValidator.h`, `CiMacOsJobConfiguration.h` y `CiMacOsJobTypes.h`.
   - Incorporar el bloque de prueba para CI macOS job:
     - Instanciación de `CiMacOsJobService` con `canonicalDebugPlan()` en modo `DryRun`.
     - Ejecución de `executePlan()` y comprobación de `status == CiMacOsJobStatus::Passed`, `isClean == true`, duración $\ge 0$ ms, y correspondencia de etapas.
     - Evaluación de plan comprensivo con las 8 etapas canónicas completas (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `BundleVerification`) comprobando que alcanza estado `Passed`.
     - Comprobación de que las etapas `BuildCompilation`, `TestSuiteExecution` y `BundleVerification` gobiernan la compilación y pruebas de la UI y el empaquetado del bundle nativo `.app`.
     - Auditoría de lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`):
       - Modos de ejecución (`DryRun`, `Simulated`, `Live`) en español neutro.
       - Resumen estructurado de etapa (`formatStepSummary`).
       - Todos los estados de trabajo (`CiMacOsJobStatus`).
       - Todos los errores (`CiMacOsJobError`) y sugerencias de remediación (`generateRemediationHint`).
       - Todas las etapas del pipeline (`CiMacOsPipelineStep`).
       - Todos los estados de etapa (`CiMacOsStepStatus`).
       - Proscripción estricta de términos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «CI macOS job»).
2. **Paso 2: Ejecutar compilación y suite de pruebas continuas con `scripts/check.sh`**:
   - Verificar compilación limpia sin advertencias bajo C++20 con `-Werror`.
   - Certificar que el 100% de las pruebas en CTest pasan exitosamente en tiempo acotado.
3. **Paso 3: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI**:
   - Ejecutar `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`.
   - Ejecutar `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0175-module.foundations.task0059.propose.lock.json --project-root .`.
4. **Paso 4: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0059-ci-macos-job/task.md`**:
   - Actualizar el estado de la tarea y registrar la evidencia verificable de la etapa `ui`.

---

## Definición de terminado

- [ ] Declaración formal de «sin cambio visual directo en la interfaz pública» documentada y verificada estáticamente en `BrowserWindow`.
- [ ] `apps/vigilio-browser/tests/ui/WindowTests.cpp` integra las cabeceras canónicas de `CiMacOsJobService`, `CiMacOsJobValidator`, `CiMacOsJobConfiguration` y `CiMacOsJobTypes.h`.
- [ ] `WindowTests.cpp` valida programáticamente la ejecución en modo dry-run del plan canónico de depuración de CI macOS con resultado `Passed`.
- [ ] `WindowTests.cpp` valida el plan comprensivo de 8 etapas canónicas de macOS incluyendo `BundleVerification`.
- [ ] Se verifica que las etapas de compilación, pruebas y verificación de bundle del pipeline gobiernan los artefactos y componentes de UI (`BrowserWindow.cpp`, `vigilio_ui_tests`, `resources.qrc`, layout del bundle `.app`).
- [ ] Se audita exhaustivamente que todas las funciones de diagnóstico y estado de CI macOS emplean español neutro y no contienen términos proscritos por `docs/UX-COPY-PUBLICO.md`.
- [ ] Se preservan los 8 estados interactivos y el escalado accesible de «Interfaz más fácil de usar» (Modo Senior).
- [ ] Se mantiene la tolerancia a entornos headless con reporte de `NOT_EXECUTED` (exit code 0).
- [ ] El script `apps/vigilio-browser/scripts/check.sh` compila limpiamente bajo C++20 con `-Werror` y supera el 100% de pruebas en CTest.
- [ ] Prompt Builder CLI valida referencias y lockfile exitosamente (`valid: true`).

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0058`: Etapa `implementation` de CI macOS job completada y certificada en `CiMacOsJobServiceTests.cpp` (commit `6b25d9d`).
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Fallos por ausencia de servidor gráfico en entornos de CI sin display.
    *Mitigación*: Preservación estricta de la guarda preliminar de `DISPLAY` / `WAYLAND_DISPLAY` en `WindowTests.cpp`, emitiendo `NOT_EXECUTED` con exit code 0.
  - *Riesgo 2*: Fugas de jerga técnica en mensajes de error visibles.
    *Mitigación*: Aserciones exhaustivas en el test de UI que verifican la proscripción de términos internos en todas las enumeraciones y pistas de remediación.
  - *Riesgo 3*: Incremento en los tiempos de ejecución de las pruebas de UI.
    *Mitigación*: Uso exclusivo de `CiMacOsExecutionMode::DryRun` en memoria, completando la validación del pipeline en < 5 ms.

---

## Commit propuesto

```
feat(foundations): ui ci-macos-job
```
