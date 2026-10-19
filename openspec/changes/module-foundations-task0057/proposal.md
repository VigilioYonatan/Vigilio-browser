# Change Proposal — module-foundations-task0057

Pack: `module.foundations.task0057`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **CI macOS job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0057 — Definir el comportamiento y las invariantes: CI macOS job`).

Habiéndose completado y certificado formalmente en las capacidades precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la configuración canónica de Clang-Format (`task0041`-`task0044`), la configuración canónica de Clang-Tidy (`task0045`-`task0048`), el ciclo de cuatro etapas de **CI Linux job** (`task0049`-`task0052`), y el ciclo de cuatro etapas de **CI Windows job** (`task0053`-`task0056`), esta propuesta formaliza el **modelo de dominio, tipos fuertemente tipados, estructuras inmutables de planificación de pipeline, validador de secuencia de etapas y dependencias, políticas de contención y seguridad de rutas con defensas específicas para sistemas macOS (Darwin), verificación de entorno (Xcode / Apple Clang, macOS SDK, arquitecturas ARM64 y x86_64) y catálogo canónico para el CI macOS job en Vigilio Browser**.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de CI macOS job (`CiMacOsJobTypes.h`)**:
  - `CiMacOsPipelineStep`: Etapas canónicas y secuenciales del pipeline de integración continua en macOS (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `BundleVerification`).
  - `CiMacOsJobStatus`: Estados de ciclo de vida del trabajo (`Pending`, `Running`, `Passed`, `Failed`, `Cancelled`, `TimedOut`).
  - `CiMacOsStepStatus`: Estados de ejecución por etapa (`NotStarted`, `InProgress`, `Success`, `Failure`, `Omitted`).
  - `CiMacOsExecutionMode`: Modos de ejecución operativa (`DryRun`, `Simulated`, `Live`).
  - `CiMacOsServiceOptions`: Opciones operativas del servicio (`mode`, `failFast`, `maxRetries`, `retryBackoffMs`, `captureOutput`).
  - `CiMacOsJobError`: Clasificación exhaustiva y tipada de errores de CI en macOS (`None`, `ToolchainMissing`, `MacOsSdkMissing`, `XcodeToolsMissing`, `ConfigurationFailed`, `CompilationFailed`, `TestExecutionFailed`, `FormatDiscrepancy`, `StaticAnalysisIssue`, `UnsafePipelinePath`, `TimeoutExceeded`, `InvalidWorkflowSchema`, `DependencyOrderingViolation`, `MemoryLimitExceeded`, `BundleVerificationFailed`).
  - `CiMacOsArchitecture`: Arquitecturas de CPU admitidas en el entorno macOS (`ARM64`, `X86_64`, `Universal`).
  - `CiMacOsCompiler`: Compiladores C++20 admitidos en macOS (`AppleClang`, `HomebrewClang`).
  - `CiMacOsBuildConfiguration`: Modos y configuraciones de construcción soportados (`Debug`, `Release`, `Sanitizer`).
- **Estructuras inmutables de entorno, etapas, plan y reporte**:
  - `CiMacOsEnvironment`: Metadatos del entorno de ejecución de macOS (versión del sistema operativo macOS 13 Ventura / macOS 14 Sonoma / macOS 15 Sequoia, versión de kernel Darwin 22/23/24, arquitectura de CPU, tipo y versión del compilador Apple Clang/Homebrew Clang, versión del macOS SDK, versiones de CMake y Ninja, memoria física total y núcleos disponibles).
  - `CiMacOsStepOptions`: Opciones operativas por etapa individual (tiempo límite en milisegundos, política de tolerancia a fallos `continueOnError`, variables de entorno requeridas y directorio de trabajo relativo).
  - `CiMacOsStepDefinition`: Definición declarativa inmutable de etapa (identificador `CiMacOsPipelineStep`, nombre descriptivo en español, comando a sintetizar o ejecutar, obligatoriedad `isMandatory` y opciones operativas).
  - `CiMacOsStepResult`: Resultado individual de ejecución de etapa (identificador de etapa, estado `CiMacOsStepStatus`, código de salida del proceso, duración en milisegundos, resumen de salida textual y mensaje de error).
  - `CiMacOsJobPlan`: Plan inmutable de trabajo de CI macOS (nombre de preset canónico de CMake v. gr. `ci-macos-debug`, directorio de compilación relativo seguro `build-ci-macos-debug`, vector ordenado de definiciones de etapas, tiempo límite global de ejecución, presupuesto máximo de memoria RAM acotado a 4 GB y bandera de corte ante primer fallo `failFast`).
  - `CiMacOsJobReport`: Reporte inmutable consolidado (estado global `CiMacOsJobStatus`, nombre del plan evaluado, duración total en milisegundos, vector ordenado de `CiMacOsStepResult`, conteo de etapas exitosas y fallidas, conformidad estricta y pista de remediación en lenguaje natural español).
  - `CancellationToken`: Token liviano y atómico para cancelación cooperativa multihilo bajo la política Zero-Zombie.
- **Funciones puras de conversión y UX copy público conforme a `docs/UX-COPY-PUBLICO.md`**:
  - `jobStatusToString(CiMacOsJobStatus)` y `jobErrorToString(CiMacOsJobError)`: Textos claros en español neutro, libres de jerga interna proscrita («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - `stepToString(CiMacOsPipelineStep)` y `stepStatusToString(CiMacOsStepStatus)`: Nombres accesibles y estados descriptivos para cada etapa del pipeline.
  - `architectureToString(CiMacOsArchitecture)` y `compilerToString(CiMacOsCompiler)`: Representaciones textuales normalizadas.
  - `generateRemediationHint(CiMacOsJobError, std::string_view stepName)`: Orientación clara y accionable para resolver incidencias de compilación, tests, análisis estático, herramientas Xcode o empaquetado bundle en macOS.
- **Validador de reglas, seguridad de rutas macOS y dependencias de etapas (`vigilio::CiMacOsJobValidator`)**:
  - Validador estático desacoplado en C++20 puro, ubicado en `vigilio_core`, libre de dependencias hacia `QApplication`, widgets de Qt o subprocesos de Chromium WebEngine.
  - `validatePlan(const CiMacOsJobPlan&)`: Verificación formal de coherencia del plan (lista de etapas no vacía, presencia de etapas obligatorias, ausencia de nombres duplicados, tiempo límite positivo y respeto riguroso del orden de dependencias: no compilar sin configurar, no ejecutar tests sin compilar, y comprobaciones estáticas previas a la compilación).
  - `validateStepSequence(const std::vector<CiMacOsPipelineStep>&)`: Comprobación estricta de orden topológico y prerequisitos entre etapas del pipeline.
  - `validateEnvironment(const CiMacOsEnvironment& env)`: Verificación de requisitos mínimos del entorno (macOS 13/14/15 ARM64 o x86_64, CMake >= 3.28, Ninja >= 1.12, Apple Clang >= 15.0 o Homebrew Clang >= 18.0, macOS SDK >= 13.0 y compatibilidad estándar C++20).
  - `isSafeBuildPath(const std::filesystem::path&)`: Prevención terminante de Directory Traversal (`..`, `../`), rechazo de rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`), prevención de artefactos ocultos problemáticos de macOS (`.DS_Store`, bifurcaciones AppleDouble `._*`), y restricción de la ejecución a subdirectorios autorizados de compilación (`build-ci-macos-debug`, `build-ci-macos-release`, `_build`).
- **Entidad de configuración canónica del CI macOS (`vigilio::CiMacOsJobConfiguration`)**:
  - Provisión de los planes canónicos oficiales: `canonicalDebugPlan()` (basado en el preset `ci-macos-debug`, generador Ninja, compilador Apple Clang / Homebrew Clang, `CMAKE_BUILD_TYPE=Debug`, `VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `VIGILIO_WARNINGS_AS_ERRORS=ON`) y `canonicalReleasePlan()` (basado en `ci-macos-release`, `CMAKE_BUILD_TYPE=Release`, incluyendo verificación de empaquetado de bundle `.app` nativo `BundleVerification`).
  - `verifyInvariants(const CiMacOsJobPlan&)`: Certificación booleana estricta de que el plan cumple con todas las invariantes de calidad y seguridad de Vigilio Browser.
  - Filtro oficial de versiones de macOS compatibles (`isSupportedMacOsVersion`: macOS 13 Ventura / Darwin 22, macOS 14 Sonoma / Darwin 23, macOS 15 Sequoia / Darwin 24).
  - Presupuestos estandarizados de recursos: límite de RAM <= 4 GB (`maxMemoryBytes`), tiempo límite por prueba individual <= 15 segundos y tiempo límite de job global <= 600 segundos (10 minutos) para Debug y <= 900 segundos (15 minutos) para Release.
- **Batería exhaustiva de pruebas unitarias de especificación (`CiMacOsJobConfigurationTests.cpp`)**:
  - 11 pruebas unitarias deterministas en CTest ejecutables en modo headless (< 50 ms), garantizando cero dependencias de red, sin instanciación de WebEngine y total aislamiento Local-First.

---

## Out of scope

- Implementación del servicio ejecutor de subprocesos, monitorización de procesos en macOS o invocación asíncrona (`CiMacOsJobService`) (reservado estrictamente para la etapa `implementation` en `module.foundations.task0058`).
- Creación de widgets Qt, diálogos o controles de interfaz de usuario de integración continua en la ventana de navegación (reservado para la etapa `ui` en `module.foundations.task0059`).
- Pruebas masivas de estrés, concurrencia multihilo y simulación de inyección de fallos de permisos en macOS (reservado para la etapa `tests` en `module.foundations.task0060`).
- Modificación directa de la interfaz gráfica principal de producción (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de controles que saturen la experiencia visual del usuario final (proscrito por la política de diseño sobrio y `docs/UX-COPY-PUBLICO.md`).
- Conexión a servicios cloud remotos de CI (GitHub Actions API remota, Xcode Cloud, Azure Pipelines, telemetría o webhooks externos), vetado taxativamente por la arquitectura constitucional Local-First (ADR-001 / Decisión Q-001).
- Firma de código oficial (`codesign` con certificados Apple Developer ID de producción) o notarización remota con Apple Notary Service (reservado a pipelines de release final e infraestructura de distribución con credenciales protegidas).
- Modificación de código de aplicación o CMakeLists.txt fuera de las extensiones autorizadas para la fase PROPOSE (la implementación física ocurrirá en la fase APPLY una vez aprobada la propuesta).

---

## Contexto y evidencia

1. **Estado del módulo `foundations` y dependencias previas**:
   - `module.foundations.task0056` ha concluido exitosamente la suite de pruebas y validación de `CI Windows job`, dejando el árbol de trabajo limpio con 23 pruebas de CTest pasando al 100%.
   - Previamente, se diseñó, implementó y testeó la suite de `CI Linux job` (`task0049`-`task0052`) y `CI Windows job` (`task0053`-`task0056`).
2. **Definición de presets en `apps/vigilio-browser/CMakePresets.json`**:
   - `CMakePresets.json` define actualmente perfiles para `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan` y `sanitizer-ubsan`.
   - Se requiere establecer la infraestructura conceptual y de validación para los presets de macOS (`ci-macos-debug`, `ci-macos-release`) en `apps/vigilio-browser/src/core/`.
3. **Estado de construcción y pruebas de verificación (`scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - 23 de 23 pruebas unitarias e integración en CTest pasando exitosamente al 100% en ~5.07 segundos:
     - `core_units`: Passed (0.01s)
     - `storage_integration`: Passed (0.01s)
     - `vigilio_warnings_tests`: Passed (0.20s)
     - `qt_dependency_validation_tests`: Passed (0.01s)
     - `webengine_version_validator_tests`: Passed (0.20s)
     - `resource_qrc_catalog_tests`: Passed (0.08s)
     - `resource_inclusion_service_tests`: Passed (0.10s)
     - `resource_qrc_inclusion_tests`: Passed (0.09s)
     - `startup_bootstrap_tests`: Passed (0.11s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.05s)
     - `cmake_presets_tests`: Passed (0.06s)
     - `cmake_presets_service_tests`: Passed (0.13s)
     - `test_fixture_foundation_tests`: Passed (0.01s)
     - `test_fixture_service_tests`: Passed (0.12s)
     - `clang_format_configuration_tests`: Passed (0.01s)
     - `clang_format_service_tests`: Passed (2.58s)
     - `clang_tidy_configuration_tests`: Passed (0.01s)
     - `clang_tidy_service_tests`: Passed (0.20s)
     - `ci_linux_job_configuration_tests`: Passed (0.01s)
     - `ci_linux_job_service_tests`: Passed (0.15s)
     - `ci_windows_job_configuration_tests`: Passed (0.01s)
     - `ci_windows_job_service_tests`: Passed (0.77s)
4. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 57 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0169-module.foundations.task0057.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
5. **Oportunidad y brecha técnica para la etapa `domain` (task0057)**:
   - Vigilio Browser es un navegador multiplataforma cuyo despliegue en macOS requiere un tratamiento riguroso de particularidades de dicha plataforma:
     - Detección de compiladores nativos (`Apple Clang` vía Xcode Command Line Tools o Xcode.app, y `Homebrew Clang`).
     - Soporte para arquitecturas ARM64 (Apple Silicon M1/M2/M3/M4) e Intel x86_64, así como binarios universales (`Universal`).
     - Validación del macOS SDK (`MacOSX.sdk` >= 13.0) para enlaces nativos con Cocoa y frameworks de Qt.
     - Estructura de bundle macOS `.app` (`VigilioBrowser.app/Contents/MacOS`, `Contents/Resources`, `Contents/Info.plist`).
     - Validación de seguridad de rutas frente a peculiaridades de macOS / Darwin / APFS: contención frente a traversal (`..`), rechazo de rutas absolutas del host (`/System`, `/Library`, `/Applications`), y prevención frente a artefactos ocultos de macOS (`.DS_Store`, archivos AppleDouble `._*`).
     - Planes canónicos para macOS: `canonicalDebugPlan` (preset `ci-macos-debug`, directorio `build-ci-macos-debug`) y `canonicalReleasePlan` (preset `ci-macos-release`, directorio `build-ci-macos-release`, validación de bundle `BundleVerification`).

---

## Alcance exacto

La propuesta formaliza el incremento de la etapa **domain** de la capacidad **CI macOS job** en el módulo `foundations`:

1. **Definición del modelo de dominio y tipos inmutables (`CiMacOsJobTypes.h`)**:
   - Enumeraciones fuertemente tipadas: `CiMacOsPipelineStep`, `CiMacOsJobStatus`, `CiMacOsStepStatus`, `CiMacOsExecutionMode`, `CiMacOsJobError`, `CiMacOsArchitecture`, `CiMacOsCompiler`, `CiMacOsBuildConfiguration`.
   - Estructuras inmutables: `CiMacOsServiceOptions`, `CiMacOsEnvironment`, `CiMacOsStepOptions`, `CiMacOsStepDefinition`, `CiMacOsStepResult`, `CiMacOsJobPlan`, `CiMacOsJobReport`.
   - Token ligero de cancelación cooperativa `CancellationToken` (con salvaguarda de inclusión múltiple).
   - Funciones libres y métodos estáticos de conversión a texto en español neutro conforme a `docs/UX-COPY-PUBLICO.md`: `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`.
2. **Motor de validación de reglas de pipeline, orden y seguridad macOS (`CiMacOsJobValidator.h` / `.cpp`)**:
   - `CiMacOsJobError validatePlan(const CiMacOsJobPlan& plan)`: Comprobación de coherencia general e invariantes del plan.
   - `CiMacOsJobError validateStepSequence(const std::vector<CiMacOsPipelineStep>& steps)`: Comprobación de precedencia lógica y ausencia de violaciones de orden.
   - `CiMacOsJobError validateEnvironment(const CiMacOsEnvironment& env)`: Comprobación de herramientas mínimas de compilación, macOS SDK y versiones de macOS / Darwin compatibles.
   - `bool isSafeBuildPath(const std::filesystem::path& path)`: Detección y rechazo inmediato de escapes de directorio (`..`, `../`), rutas absolutas del host, artefactos macOS (`.DS_Store`, `._*`) y rutas no relativas al árbol de construcción.
   - `std::string generateRemediationHint(CiMacOsJobError error, std::string_view stepName)`: Diagnóstico descriptivo y guía de resolución en español claro.
3. **Entidad de configuración canónica (`CiMacOsJobConfiguration.h` / `.cpp`)**:
   - `CiMacOsJobPlan canonicalDebugPlan()`: Plan canónico de desarrollo y CI en macOS para depuración rápida (`ci-macos-debug`, `build-ci-macos-debug`, 600 s).
   - `CiMacOsJobPlan canonicalReleasePlan()`: Plan canónico de validación de optimización y bundle (.app) (`ci-macos-release`, `build-ci-macos-release`, 900 s).
   - `bool verifyInvariants(const CiMacOsJobPlan& plan)`: Verificación booleana estricta de cumplimiento de invariantes de dominio.
   - `bool isSupportedMacOsVersion(std::string_view version)`: Validación de versiones de macOS reconocidas y soportadas (macOS 13, 14, 15 y equivalentes Darwin 22, 23, 24).
4. **Integración en `vigilio_core` y suite de pruebas unitarias (`CiMacOsJobConfigurationTests.cpp`)**:
   - Adición de las fuentes al target estático `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
   - Creación del ejecutable de pruebas `vigilio_ci_macos_job_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`, registrado con `LABELS "unit;fast"` en CTest.

---

## Diseño completo de la capacidad

El diseño de la capacidad **CI macOS job** se estructura en cuatro etapas ordenadas dentro del módulo `foundations`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       module.foundations (CI macOS job)                     │
└─────────────────────────────────────────────────────────────────────────────┘
       │
       ▼
[0057 — domain] (ESTA PROPUESTA)
  ├── CiMacOsJobTypes.h: Taxonomía tipada, etapas, estados, errores y reportes.
  ├── CiMacOsJobValidator.h/.cpp: Validación de dependencias, orden y seguridad macOS.
  ├── CiMacOsJobConfiguration.h/.cpp: Planes canónicos e invariantes.
  └── CiMacOsJobConfigurationTests.cpp: 11 pruebas unitarias deterministas en CTest.
       │
       ▼
[0058 — implementation] (INCREMENTO SUBSIGUIENTE)
  ├── CiMacOsJobService.h/.cpp: Ejecución asíncrona, control de procesos y timeouts.
  └── Coordinación de presets CMake, CTest y captura controlada de salida en macOS.
       │
       ▼
[0059 — ui] (ETAPA VISUAL / DIAGNÓSTICOS)
  ├── Integración de diagnósticos con interfaz sobria y textos públicos accesibles.
  └── Respeto a «Interfaz más fácil de usar» y directrices de diseño en macOS.
       │
       ▼
[0060 — tests] (PRUEBAS EXHAUSTIVAS Y RECUPERACIÓN)
  └── Casos negativos, cancelación forzada, timeouts y recuperación en macOS.
```

### Invariantes de dominio para CI macOS job

1. **Invariante de Local-First y Privacidad Absoluta**:
   - Ninguna comprobación de entorno, invocación de herramientas ni reporte de etapas envía datos, telemetría o paquetes de red a servidores remotos. Toda la verificación se ejecuta localmente en el runner o estación de trabajo.
2. **Invariante de Independencia del Núcleo (`vigilio_core`)**:
   - Las clases de dominio `CiMacOsJobConfiguration`, `CiMacOsJobValidator` y los tipos en `CiMacOsJobTypes.h` se compilan en `vigilio_core` sin incluir cabeceras de `QWidget`, `QApplication` ni subprocesos de Chromium WebEngine.
3. **Invariante de Precedencia Topológica de Etapas (DAG)**:
   - Toda etapa de compilación (`BuildCompilation`) requiere obligatoriamente una configuración previa exitosa (`ConfigurePreset`).
   - Toda ejecución de pruebas (`TestSuiteExecution`) requiere una compilación previa exitosa (`BuildCompilation`).
   - La comprobación de entorno (`EnvironmentCheck`) debe situarse al inicio del pipeline.
   - Las verificaciones estáticas (`FormatVerification`, `StaticAnalysis`) deben validarse antes o junto con la compilación para corte temprano (`failFast`).
   - La validación de bundle (.app) (`BundleVerification`) se ejecuta tras la compilación y pruebas.
4. **Invariante de Confinamiento y Seguridad de Rutas en macOS**:
   - El directorio de construcción `buildDir` debe ser estrictamente relativo y descender de rutas seguras aprobadas (`build-ci-macos-debug`, `build-ci-macos-release`, `_build`).
   - Se prohíbe de manera terminante cualquier componente de escape (`..`, `../`), rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, etc.) y artefactos conflictivos de macOS (`.DS_Store`, archivos AppleDouble `._*`).
5. **Invariante de Presupuesto Acotado de Recursos**:
   - Límite máximo de memoria asignada de 4 GB (`4294967296` bytes).
   - Tiempo límite determinista global de 600 segundos (10 minutos) en Debug y 900 segundos (15 minutos) en Release.
   - Tiempo límite por prueba individual acotado a 15 segundos en CTest.

---

## Archivos afectados

### Creados en la etapa domain (APPLY subsiguiente)
1. `apps/vigilio-browser/src/core/CiMacOsJobTypes.h`: Definición de enums, estructuras inmutables y reporte.
2. `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h`: Declaración de la clase de configuración canónica.
3. `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.cpp`: Implementación de planes canónicos e invariantes.
4. `apps/vigilio-browser/src/core/CiMacOsJobValidator.h`: Declaración del validador de etapas, entorno y rutas.
5. `apps/vigilio-browser/src/core/CiMacOsJobValidator.cpp`: Implementación de validaciones y UX copy.
6. `apps/vigilio-browser/tests/unit/CiMacOsJobConfigurationTests.cpp`: Batería de 11 pruebas unitarias.

### Modificados en la etapa domain (APPLY subsiguiente)
1. `apps/vigilio-browser/CMakeLists.txt`: Adición de archivos a `vigilio_core` y declaración de `vigilio_ci_macos_job_tests`.

---

## Contrato UX/UI

- **Impacto visual directo**: **Sin cambio visual directo en la ventana de navegación** durante la etapa de dominio.
- **Razón**: La capacidad `CI macOS job` en su etapa de dominio corresponde a infraestructura de soporte de calidad y pruebas de compilación en sistemas macOS.
- **Cumplimiento de `docs/UX-COPY-PUBLICO.md`**:
  - Todos los textos generados por `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint` están redactados en español neutro comprensible.
  - Se prohíbe de manera estricta la inclusión de tecnicismos internos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - En su lugar, se emplean denominaciones claras: «Herramientas de compilación», «Comprobación del entorno de macOS», «Configuración del proyecto», «Compilación del código», «Ejecución de pruebas», «Verificación del paquete de la aplicación», e «Interfaz más fácil de usar».

---

## Reglas y fallos

| Condición de Error | Componente Responsable | Código de Error Tipado | Comportamiento y Recuperación |
| --- | --- | --- | --- |
| Plan con lista de etapas vacía | `validatePlan` | `CiMacOsJobError::InvalidWorkflowSchema` | Se rechaza el plan inmediatamente; el sistema mantiene el plan por defecto canónico. |
| Plan con directorio de compilación inseguro (ej. `/System`, `../../etc`, `.DS_Store`) | `isSafeBuildPath` / `validatePlan` | `CiMacOsJobError::UnsafePipelinePath` | Se rechaza la ruta; se previene cualquier operación sobre el sistema de archivos del host. |
| Dependencia de compilación antes de configuración en el pipeline | `validateStepSequence` | `CiMacOsJobError::DependencyOrderingViolation` | Se detecta la violación de precedencia; se rechaza la ejecución y se emite pista de ordenamiento. |
| Dependencia de pruebas antes de compilación en el pipeline | `validateStepSequence` | `CiMacOsJobError::DependencyOrderingViolation` | Se aborta la validación; se exige que la etapa de compilación preceda a las pruebas. |
| Entorno sin versión compatible de macOS (versión anterior a macOS 13 Ventura / Darwin 22) | `validateEnvironment` | `CiMacOsJobError::ToolchainMissing` | Se marca el entorno como no conforme; se reporta la necesidad de macOS 13 o superior. |
| Entorno sin compilador C++20 compatible (Apple Clang < 15.0 o Homebrew Clang < 18.0) | `validateEnvironment` | `CiMacOsJobError::ToolchainMissing` | Se rechaza el entorno; se indica que Vigilio Browser requiere Apple Clang 15+ o Homebrew Clang 18+. |
| Entorno sin macOS SDK instalado o versión obsoleta (< 13.0) | `validateEnvironment` | `CiMacOsJobError::MacOsSdkMissing` | Se reporta la ausencia del macOS SDK necesario para los contratos de compilación y enlaces nativos. |
| Entorno sin herramientas Xcode instaladas (`CommandLineTools` ausente) | `validateEnvironment` | `CiMacOsJobError::XcodeToolsMissing` | Se reporta la necesidad de ejecutar `xcode-select --install` o instalar Xcode Command Line Tools. |
| Tiempo límite de plan nulo, negativo o excesivo (> 3600 s) | `validatePlan` | `CiMacOsJobError::InvalidWorkflowSchema` | Se rechaza la configuración del plan; se restablece el tiempo límite al valor canónico seguro (600 s en Debug, 900 s en Release). |
| Presupuesto de memoria excesivo (> 8 GB) o nulo | `validatePlan` | `CiMacOsJobError::MemoryLimitExceeded` | Se ajusta el límite al presupuesto canónico máximo estricto de 4 GB (`4294967296` bytes). |

---

## Seguridad y privacidad

1. **Aislamiento Local-First (ADR-001 / Decisión Q-001)**:
   - Toda la definición, validación y planificación de CI macOS opera exclusivamente de forma local sin invocar APIs en la nube, servidores remotos de compilación ni enviar telemetría.
2. **Defensa en profundidad en rutas macOS**:
   - `isSafeBuildPath` normaliza y sanitiza separadores de ruta directos (`/`).
   - Rechaza explícitamente:
     - Componentes de directorio padre (`..`, `../`).
     - Rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`, `/Users`).
     - Artefactos y metadatos conflictivos del sistema de archivos de macOS (`.DS_Store`, bifurcaciones AppleDouble `._*`).
     - Caracteres nulos o secuencias de control no imprimibles.
3. **Presupuesto de recursos y límites estrictos**:
   - Memoria RAM limitada a 4 GB por defecto para evitar agotamiento de recursos en el sistema anfitrión.
   - Timeouts deterministas con corte estricto ante bloqueo o contención prolongada.

---

## Matriz de pruebas

| Identificador | Criterio de Verificación | Tipo de Prueba | Fixture / Entrada | Comando / Ejecución | Resultado Esperado | Fallo Negativo / Caso Inválido |
| --- | --- | --- | --- | --- | --- | --- |
| `TEST-001` | Plan canónico Debug válido y conforme | Unitario (APPLY) | `canonicalDebugPlan()` | `ctest -R ci_macos_job -V` | Preset `ci-macos-debug`, 6 etapas, timeout 600 s, memoria 4 GB, failFast activado | Falla si el preset no coincide o etapas < 6 |
| `TEST-002` | Plan canónico Release válido con bundle | Unitario (APPLY) | `canonicalReleasePlan()` | `ctest -R ci_macos_job -V` | Preset `ci-macos-release`, 7 etapas incluyendo `BundleVerification`, timeout 900 s | Falla si omite verificación de bundle o preset incorrecto |
| `TEST-003` | Validación exitosa de planes canónicos | Unitario (APPLY) | Planes canónicos | `ctest -R ci_macos_job -V` | `validatePlan` retorna `CiMacOsJobError::None` | Falla si emite error ante plan canónico |
| `TEST-004` | Rechazo de planes con esquemas inválidos y límites erróneos | Unitario (APPLY) | Plan con etapas vacías, timeout 0, preset vacío, memoria excesiva | `ctest -R ci_macos_job -V` | Retorna `CiMacOsJobError::InvalidWorkflowSchema` o `MemoryLimitExceeded` | Falla si acepta plan vacío o timeout no positivo |
| `TEST-005` | Validación de precedencia y orden topológico | Unitario (APPLY) | Etapas desordenadas (build antes de config, test antes de build) | `ctest -R ci_macos_job -V` | Retorna `CiMacOsJobError::DependencyOrderingViolation` | Falla si admite compilar sin previa configuración |
| `TEST-006` | Validación de entorno y toolchain macOS | Unitario (APPLY) | Entorno válido vs entorno con Apple Clang obsoleto, SDK ausente o Xcode faltante | `ctest -R ci_macos_job -V` | Retorna `None` para válido; `ToolchainMissing`, `MacOsSdkMissing` o `XcodeToolsMissing` para inválido | Falla si tolera entorno sin SDK o C++20 |
| `TEST-007` | Detección y rechazo de Directory Traversal en macOS | Unitario (APPLY) | Rutas `../build`, `build/../../etc`, `/System`, `/Library`, `/private/tmp` | `ctest -R ci_macos_job -V` | `isSafeBuildPath` retorna `false` | Falla si permite rutas que escapan de la raíz |
| `TEST-008` | Rechazo de artefactos conflictivos de macOS | Unitario (APPLY) | Rutas `.DS_Store`, `build/._icon`, `._metadata` | `ctest -R ci_macos_job -V` | `isSafeBuildPath` retorna `false` | Falla si acepta `.DS_Store` o bifurcaciones AppleDouble |
| `TEST-009` | Filtro oficial de versiones de macOS compatibles | Unitario (APPLY) | macOS 13, macOS 14, macOS 15, Darwin 22/23/24 vs macOS 10.15, macOS 11, macOS 12 | `ctest -R ci_macos_job -V` | Retorna `true` para versiones modernas, `false` para obsoletas | Falla si admite macOS 10.15 o versiones no mantenidas |
| `TEST-010` | Modelo de datos de opciones, resultados y reportes | Unitario (APPLY) | Estructuras inmutables instanciadas | `ctest -R ci_macos_job -V` | Comparación por defecto `operator==`, conteo de exitosos/fallidos correcto | Falla si los contadores o estados difieren |
| `TEST-011` | Cumplimiento estricto de UX Copy público | Unitario (APPLY) | Todas las cadenas de estado, error, etapa y remediación | `ctest -R ci_macos_job -V` | Cadenas en español neutro; cero términos proscritos de `docs/UX-COPY-PUBLICO.md` | Falla si contiene «Senior Mode», «DAG», «toolchain» |
| `TEST-LATER` | Servicio ejecutor de subprocesos y timeouts en macOS | Integración (LATER) | `CiMacOsJobService` (task0058) | CTest en etapa implementation | Coordinación asíncrona de procesos y presets | N/A en fase domain |
| `TEST-NA` | Proceso Chromium WebEngine o red externa | N/A | Entornos cloud o GUI compleja | Proscrito por política Local-First | Totalmente desacoplado de WebEngine y red | N/A en todas las etapas |

---

## Pasos APPLY

El incremento para la etapa **domain** se implementará de forma determinista y modular en la fase APPLY subsiguiente:

1. **Paso 1: Definir tipos de dominio en `apps/vigilio-browser/src/core/CiMacOsJobTypes.h`**:
   - Crear enumeraciones `CiMacOsPipelineStep`, `CiMacOsJobStatus`, `CiMacOsStepStatus`, `CiMacOsExecutionMode`, `CiMacOsJobError`, `CiMacOsArchitecture`, `CiMacOsCompiler`, `CiMacOsBuildConfiguration`.
   - Definir estructuras inmutables `CiMacOsServiceOptions`, `CiMacOsEnvironment`, `CiMacOsStepOptions`, `CiMacOsStepDefinition`, `CiMacOsStepResult`, `CiMacOsJobPlan`, `CiMacOsJobReport` y `CancellationToken`.
2. **Paso 2: Declarar la clase `CiMacOsJobConfiguration` en `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h`**:
   - Declarar métodos estáticos `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants(const CiMacOsJobPlan&)` e `isSupportedMacOsVersion(std::string_view)`.
3. **Paso 3: Implementar la configuración canónica en `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.cpp`**:
   - Definir planes oficiales para Debug (`ci-macos-debug`, `build-ci-macos-debug`) y Release (`ci-macos-release`, `build-ci-macos-release`), verificando invariantes y catálogo de versiones de macOS.
4. **Paso 4: Declarar el validador en `apps/vigilio-browser/src/core/CiMacOsJobValidator.h`**:
   - Declarar métodos estáticos `validatePlan`, `validateStepSequence`, `validateEnvironment`, `isSafeBuildPath` y conversores de UX copy.
5. **Paso 5: Implementar las reglas de validación en `apps/vigilio-browser/src/core/CiMacOsJobValidator.cpp`**:
   - Implementar verificación topológica de orden de etapas, sanitización de rutas para macOS (rechazo de `..`, `/System`, `.DS_Store`, `._*`) y generación de pistas de remediación en español neutro.
6. **Paso 6: Registrar las fuentes en `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/CiMacOsJobConfiguration.cpp` y `src/core/CiMacOsJobValidator.cpp` a la biblioteca `vigilio_core`.
   - Declarar el target `vigilio_ci_macos_job_tests` y añadirlo a CTest con `LABELS "unit;fast"`.
7. **Paso 7: Implementar la batería de pruebas en `apps/vigilio-browser/tests/unit/CiMacOsJobConfigurationTests.cpp`**:
   - Escribir las 11 pruebas unitarias deterministas (`TEST-001` a `TEST-011`) sin dependencias de red ni servidor gráfico.
8. **Paso 8: Verificar calidad y suite continua con `scripts/check.sh`**:
   - Ejecutar compilación y suite completa de pruebas en CTest verificando 24/24 pruebas pasando al 100%.

---

## Definición de terminado

La etapa `domain` de CI macOS job se considerará completamente terminada tras la ejecución de APPLY cuando:
1. `CiMacOsJobTypes.h` declare todas las enumeraciones y estructuras inmutables del dominio.
2. `CiMacOsJobConfiguration` exponga planes canónicos debug/release e invariantes con límite de RAM de 4 GB y tiempos de 600 s / 900 s.
3. `CiMacOsJobValidator` valide planes, precedencia de etapas, entorno de macOS y seguridad de rutas rechazando Directory Traversal y artefactos de macOS (`.DS_Store`, `._*`).
4. Las pruebas unitarias en `CiMacOsJobConfigurationTests.cpp` se compilen limpiamente sin advertencias bajo C++20 con `-Werror`.
5. Los 11 tests unitarios de CTest pasen al 100% en tiempo inferior a 50 ms en modo headless.
6. La suite completa del proyecto en `scripts/check.sh` mantenga el 100% de éxito.
7. Los textos públicos cumplan con `docs/UX-COPY-PUBLICO.md` sin jerga técnica proscrita.

---

## Dependencias y riesgos

- **Dependencia resuelta**: `module.foundations.task0056` completada satisfactoriamente con todas las pruebas pasando.
- **Riesgo 1**: Incompatibilidad léxica en versiones de macOS (Darwin 22/23/24 vs macOS 13/14/15).
  - *Mitigación*: `isSupportedMacOsVersion` soporta indistintamente nombres comerciales («macOS 13», «macOS 14», «macOS 15») y versiones del kernel Darwin («Darwin 22», «Darwin 23», «Darwin 24»).
- **Riesgo 2**: Artefactos de sistema de archivos propios de macOS (`.DS_Store`, `._*`) que contaminen los directorios de trabajo o generen falsos positivos en comprobaciones de limpieza.
  - *Mitigación*: `isSafeBuildPath` detecta y rechaza expresamente nombres que comiencen con `._` o se denominen `.DS_Store`.
- **Riesgo 3**: Presupuesto de memoria excesivo en runners con memoria compartida.
  - *Mitigación*: Límite estricto de 4 GB (`maxMemoryBytes`) validado en `validatePlan` y `verifyInvariants`.

---

## Commit propuesto

Tras la aprobación humana y ejecución de la fase APPLY:
`docs(foundations): domain ci-macos-job`
