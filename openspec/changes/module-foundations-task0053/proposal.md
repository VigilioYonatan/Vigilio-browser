# Change Proposal — module-foundations-task0053

Pack: `module.foundations.task0053`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **CI Windows job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0053 — Definir el comportamiento y las invariantes: CI Windows job`).

Habiéndose completado y certificado formalmente en las capacidades precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la configuración canónica de Clang-Format (`task0041`-`task0044`), la configuración canónica de Clang-Tidy (`task0045`-`task0048`), y el ciclo integral de cuatro etapas de **CI Linux job** (dominio en `task0049`, implementación en `task0050`, interfaz en `task0051` y pruebas en `task0052`), esta propuesta formaliza el **modelo de dominio, tipos fuertemente tipados, estructuras inmutables de planificación de pipeline, validador de secuencia de etapas y dependencias, políticas de contención y seguridad de rutas con soporte específico para sistemas Windows, verificación de entorno y catálogo canónico para el CI Windows job en Vigilio Browser**.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de CI Windows job (`CiWindowsJobTypes.h`)**:
  - `CiWindowsPipelineStep`: Etapas canónicas y secuenciales del pipeline de integración continua en Windows (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`).
  - `CiWindowsJobStatus`: Estados de ciclo de vida del trabajo (`Pending`, `Running`, `Passed`, `Failed`, `Cancelled`, `TimedOut`).
  - `CiWindowsStepStatus`: Estados de ejecución por etapa (`NotStarted`, `InProgress`, `Success`, `Failure`, `Omitted`).
  - `CiWindowsExecutionMode`: Modos de ejecución operativa (`DryRun`, `Simulated`, `Live`).
  - `CiWindowsServiceOptions`: Opciones operativas del servicio (`mode`, `failFast`, `maxRetries`, `retryBackoffMs`, `captureOutput`).
  - `CiWindowsJobError`: Clasificación exhaustiva y tipada de errores de CI en Windows (`None`, `ToolchainMissing`, `WindowsSdkMissing`, `ConfigurationFailed`, `CompilationFailed`, `TestExecutionFailed`, `FormatDiscrepancy`, `StaticAnalysisIssue`, `UnsafePipelinePath`, `TimeoutExceeded`, `InvalidWorkflowSchema`, `DependencyOrderingViolation`, `MemoryLimitExceeded`).
  - `CiWindowsArchitecture`: Arquitecturas de CPU admitidas en el entorno Windows (`X86_64`, `ARM64`).
  - `CiWindowsCompiler`: Compiladores C++20 admitidos en Windows (`MSVC`, `ClangCl`, `MinGW`).
  - `CiWindowsBuildConfiguration`: Modos y configuraciones de construcción soportados (`Debug`, `Release`, `Sanitizer`).
- **Estructuras inmutables de entorno, etapas, plan y reporte**:
  - `CiWindowsEnvironment`: Metadatos del entorno de ejecución de Windows (versión del sistema operativo Windows 10/11/Server, número de build, arquitectura de CPU, tipo y versión del compilador MSVC/Clang-cl, versión del Windows SDK, versiones de CMake y Ninja, memoria física total y núcleos disponibles).
  - `CiWindowsStepOptions`: Opciones operativas por etapa individual (tiempo límite en milisegundos, política de tolerancia a fallos `continueOnError`, variables de entorno requeridas y directorio de trabajo relativo).
  - `CiWindowsStepDefinition`: Definición declarativa inmutable de etapa (identificador `CiWindowsPipelineStep`, nombre descriptivo en español, comando a sintetizar o ejecutar, obligatoriedad `isMandatory` y opciones operativas).
  - `CiWindowsStepResult`: Resultado individual de ejecución de etapa (identificador de etapa, estado `CiWindowsStepStatus`, código de salida del proceso, duración en milisegundos, resumen de salida textual y mensaje de error).
  - `CiWindowsJobPlan`: Plan inmutable de trabajo de CI Windows (nombre de preset canónico de CMake v. gr. `ci-windows-debug`, directorio de compilación relativo seguro `build-ci-win-debug`, vector ordenado de definiciones de etapas, tiempo límite global de ejecución, presupuesto máximo de memoria RAM acotado a 4 GB y bandera de corte ante primer fallo `failFast`).
  - `CiWindowsJobReport`: Reporte inmutable consolidado (estado global `CiWindowsJobStatus`, nombre del plan evaluado, duración total en milisegundos, vector ordenado de `CiWindowsStepResult`, conteo de etapas exitosas y fallidas, conformidad estricta y pista de remediación en lenguaje natural español).
  - `CancellationToken`: Token liviano y atómico para cancelación cooperativa multihilo bajo la política Zero-Zombie.
- **Funciones puras de conversión y UX copy público conforme a `docs/UX-COPY-PUBLICO.md`**:
  - `jobStatusToString(CiWindowsJobStatus)` y `jobErrorToString(CiWindowsJobError)`: Textos claros en español neutro, libres de jerga interna proscrita («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - `stepToString(CiWindowsPipelineStep)` y `stepStatusToString(CiWindowsStepStatus)`: Nombres accesibles y estados descriptivos para cada etapa del pipeline.
  - `architectureToString(CiWindowsArchitecture)` y `compilerToString(CiWindowsCompiler)`: Representaciones textuales normalizadas.
  - `generateRemediationHint(CiWindowsJobError, std::string_view stepName)`: Orientación clara y accionable para resolver incidencias de compilación, tests, análisis estático o configuración en Windows.
- **Validador de reglas, seguridad de rutas Windows y dependencias de etapas (`vigilio::CiWindowsJobValidator`)**:
  - Validador estático desacoplado en C++20 puro, ubicado en `vigilio_core`, libre de dependencias hacia `QApplication`, widgets de Qt o subprocesos de Chromium WebEngine.
  - `validatePlan(const CiWindowsJobPlan&)`: Verificación formal de coherencia del plan (lista de etapas no vacía, presencia de etapas obligatorias, ausencia de nombres duplicados, tiempo límite positivo y respeto riguroso del orden de dependencias: no compilar sin configurar, no ejecutar tests sin compilar, y comprobaciones estáticas previas a la compilación).
  - `validateStepSequence(const std::vector<CiWindowsPipelineStep>&)`: Comprobación estricta de orden topológico y prerequisitos entre etapas del pipeline.
  - `validateEnvironment(const CiWindowsEnvironment& env)`: Verificación de requisitos mínimos del entorno (Windows 10/11 x64 o ARM64, CMake >= 3.28, Ninja >= 1.12, MSVC >= 19.38 o Clang-cl >= 18, Windows SDK >= 10.0.22621.0 y compatibilidad estándar C++20).
  - `isSafeBuildPath(const std::filesystem::path&)`: Prevención terminante de Directory Traversal tanto con separador Unix (`/`) como Windows (`\`), rechazo categórico de letras de unidad (`C:\`, `D:/`), rutas absolutas del host (`\Windows\System32`, `\\server\share`), prefijos de espacio de nombres NT (`\\?\`, `\\.\`), y nombres de dispositivos reservados en DOS/Windows (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`), restringiendo la ejecución a subdirectorios autorizados de compilación (`build-ci-win-debug`, `build-ci-win-release`, `_build`).
- **Entidad de configuración canónica del CI Windows (`vigilio::CiWindowsJobConfiguration`)**:
  - Provisión de los planes canónicos oficiales: `canonicalDebugPlan()` (basado en el preset `ci-windows-debug`, generador Ninja, compilador MSVC / Clang-cl, `CMAKE_BUILD_TYPE=Debug`, `VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `VIGILIO_WARNINGS_AS_ERRORS=ON`) y `canonicalReleasePlan()` (basado en `ci-windows-release`, `CMAKE_BUILD_TYPE=Release`, incluyendo empaquetado del instalador nativo).
  - `verifyInvariants(const CiWindowsJobPlan&)`: Certificación booleana estricta de que el plan cumple con todas las invariantes de calidad y seguridad de Vigilio Browser.
  - Filtro oficial de versiones de Windows compatibles (`isSupportedWindowsVersion`: Windows 10 Versión 22H2 / Build 19045+, Windows 11 Versión 22H2/23H2 / Build 22621+, Windows Server 2022).
  - Presupuestos estandarizados de recursos: límite de RAM <= 4 GB (`maxMemoryBytes`), tiempo límite por prueba individual <= 15 segundos y tiempo límite de job global <= 600 segundos (10 minutos) para Debug y <= 900 segundos (15 minutos) para Release.
- **Batería exhaustiva de pruebas unitarias de especificación (`CiWindowsJobConfigurationTests.cpp`)**:
  - 11 pruebas unitarias deterministas en CTest ejecutables en modo headless (< 50 ms), garantizando cero dependencias de red, sin instanciación de WebEngine y total aislamiento Local-First.

---

## Out of scope

- Implementación del servicio ejecutor de subprocesos, monitorización de procesos en Windows o invocación asíncrona (`CiWindowsJobService`) (reservado estrictamente para la etapa `implementation` en `module.foundations.task0054`).
- Creación de widgets Qt, diálogos o controles de interfaz de usuario de integración continua en la ventana de navegación (reservado para la etapa `ui` en `module.foundations.task0055`).
- Pruebas masivas de estrés, concurrencia de 16 hilos y simulación de inyección de fallos de permisos en Windows (reservado para la etapa `tests` en `module.foundations.task0056`).
- Modificación directa de la interfaz gráfica principal de producción (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de controles que saturen la experiencia visual del usuario final (proscrito por la política de diseño sobrio y `docs/UX-COPY-PUBLICO.md`).
- Conexión a servicios cloud remotos de CI (Azure DevOps, GitHub Actions API remota, GitLab CI remota, telemetría o webhooks externos), vetado taxativamente por la arquitectura constitucional Local-First (ADR-001 / Decisión Q-001).
- Modificación de código de aplicación o CMakeLists.txt fuera de las extensiones autorizadas para la fase PROPOSE (la implementación física ocurrirá en la fase APPLY una vez aprobada la propuesta).

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Estado de capacidades previas en `apps/vigilio-browser/src/core/`**:
   - `CiLinuxJobTypes.h`, `CiLinuxJobConfiguration.h/.cpp`, `CiLinuxJobValidator.h/.cpp`, `CiLinuxJobService.h/.cpp`: Implementaron y certificaron el ciclo completo de CI en entornos Linux.
   - `SessionFile.cpp` y `CrashRecoveryCoordinator.cpp`: Contienen guardas condicionales `#include <windows.h>` demostrando compatibilidad de compilación con la plataforma Windows.
   - `CMakePresetsTypes.h`, `CMakePresetsValidator.h`: Soportan la abstracción de perfiles de CMakePresets v6.
2. **Definición de presets en `apps/vigilio-browser/CMakePresets.json`**:
   - `CMakePresets.json` define actualmente perfiles para `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan` y `sanitizer-ubsan`.
   - No existían previamente en `apps/vigilio-browser/src/core/` los tipos ni validadores para modelar los trabajos de CI específicos para Windows (`ci-windows-debug`, `ci-windows-release`).
3. **Estado de construcción y pruebas de verificación (`scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - 21 de 21 pruebas unitarias e integración en CTest pasando exitosamente al 100% en ~3.78 segundos:
     - `core_units`: Passed (0.01s)
     - `storage_integration`: Passed (0.02s)
     - `vigilio_warnings_tests`: Passed (0.34s)
     - `qt_dependency_validation_tests`: Passed (0.01s)
     - `webengine_version_validator_tests`: Passed (0.14s)
     - `resource_qrc_catalog_tests`: Passed (0.02s)
     - `resource_inclusion_service_tests`: Passed (0.08s)
     - `resource_qrc_inclusion_tests`: Passed (0.10s)
     - `startup_bootstrap_tests`: Passed (0.20s)
     - `application_lifetime_tests`: Passed (0.20s)
     - `crash_recovery_tests`: Passed (0.03s)
     - `cmake_presets_tests`: Passed (0.04s)
     - `cmake_presets_service_tests`: Passed (0.10s)
     - `test_fixture_foundation_tests`: Passed (0.01s)
     - `test_fixture_service_tests`: Passed (0.12s)
     - `clang_format_configuration_tests`: Passed (0.01s)
     - `clang_format_service_tests`: Passed (2.04s)
     - `clang_tidy_configuration_tests`: Passed (0.01s)
     - `clang_tidy_service_tests`: Passed (0.13s)
     - `ci_linux_job_configuration_tests`: Passed (0.01s)
     - `ci_linux_job_service_tests`: Passed (0.15s)
4. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 53 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0157-module.foundations.task0053.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
5. **Oportunidad y brecha técnica para la etapa `domain` (task0053)**:
   - Vigilio Browser es un navegador multiplataforma cuyo despliegue en Windows requiere un tratamiento riguroso de particularidades de dicha plataforma:
     - Detección de compiladores nativos (`MSVC` v143 / `cl.exe`, `Clang-cl`, `MinGW`).
     - Validación del Windows SDK (headers y bibliotecas necesarias para Qt y componentes nativos de escritorio).
     - Validación específica de seguridad de rutas frente a peculiaridades de Windows: rutas con contrabarra (`\`), letras de unidad (`C:`, `D:`), rutas UNC de red (`\\server\share`), espacios de nombres de dispositivo NT (`\\?\`), y nombres de archivo reservados en DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`).
     - Planes canónicos para Windows: `canonicalDebugPlan` (preset `ci-windows-debug`, directorio `build-ci-win-debug`) y `canonicalReleasePlan` (preset `ci-windows-release`, directorio `build-ci-win-release`, empaquetado del instalador).

---

## Alcance exacto

La propuesta formaliza el incremento de la etapa **domain** de la capacidad **CI Windows job** en el módulo `foundations`:

1. **Definición del modelo de dominio y tipos inmutables (`CiWindowsJobTypes.h`)**:
   - Enumeraciones fuertemente tipadas: `CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`.
   - Estructuras inmutables: `CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport`.
   - Token ligero de cancelación cooperativa `CancellationToken` (con salvaguarda de inclusión múltiple).
   - Funciones libres y métodos estáticos de conversión a texto en español neutro conforme a `docs/UX-COPY-PUBLICO.md`: `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`.
2. **Motor de validación de reglas de pipeline, orden y seguridad Windows (`CiWindowsJobValidator.h` / `.cpp`)**:
   - `CiWindowsJobError validatePlan(const CiWindowsJobPlan& plan)`: Comprobación de coherencia general e invariantes del plan.
   - `CiWindowsJobError validateStepSequence(const std::vector<CiWindowsPipelineStep>& steps)`: Comprobación de precedencia lógica y ausencia de violaciones de orden.
   - `CiWindowsJobError validateEnvironment(const CiWindowsEnvironment& env)`: Comprobación de herramientas mínimas de compilación, Windows SDK y versiones de Windows compatibles.
   - `bool isSafeBuildPath(const std::filesystem::path& path)`: Detección y rechazo inmediato de escapes de directorio (`..`, `../`, `..\`), letras de unidad (`C:\`), rutas absolutas del host, dispositivos reservados DOS (`CON`, `NUL`, etc.) y prefijos de espacio de nombres.
   - `std::string generateRemediationHint(CiWindowsJobError error, std::string_view stepName)`: Diagnóstico descriptivo y guía de resolución en español claro.
3. **Entidad de configuración canónica (`CiWindowsJobConfiguration.h` / `.cpp`)**:
   - `CiWindowsJobPlan canonicalDebugPlan()`: Plan canónico de desarrollo y CI en Windows para depuración rápida (`ci-windows-debug`, `build-ci-win-debug`, 600 s).
   - `CiWindowsJobPlan canonicalReleasePlan()`: Plan canónico de validación de optimización y empaquetado (`ci-windows-release`, `build-ci-win-release`, 900 s).
   - `bool verifyInvariants(const CiWindowsJobPlan& plan)`: Verificación booleana estricta de cumplimiento de invariantes de dominio.
   - `bool isSupportedWindowsVersion(std::string_view version)`: Validación de versiones de Windows reconocidas y soportadas.
4. **Integración en `vigilio_core` y suite de pruebas unitarias (`CiWindowsJobConfigurationTests.cpp`)**:
   - Adición de las fuentes al target estático `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
   - Creación del ejecutable de pruebas `vigilio_ci_windows_job_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`, registrado con `LABELS "unit;fast"` en CTest.

---

## Diseño completo de la capacidad

El diseño de la capacidad **CI Windows job** se estructura en cuatro etapas ordenadas dentro del módulo `foundations`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      module.foundations (CI Windows job)                    │
└─────────────────────────────────────────────────────────────────────────────┘
       │
       ▼
[0053 — domain] (ESTA PROPUESTA)
  ├── CiWindowsJobTypes.h: Taxonomía tipada, etapas, estados, errores y reportes.
  ├── CiWindowsJobValidator.h/.cpp: Validación de dependencias, orden y seguridad Windows.
  ├── CiWindowsJobConfiguration.h/.cpp: Planes canónicos e invariantes.
  └── CiWindowsJobConfigurationTests.cpp: 11 pruebas unitarias deterministas en CTest.
       │
       ▼
[0054 — implementation] (INCREMENTO SUBSIGUIENTE)
  ├── CiWindowsJobService.h/.cpp: Ejecución asíncrona, control de procesos y timeouts.
  └── Coordinación de presets CMake, CTest y captura controlada de salida en Windows.
       │
       ▼
[0055 — ui] (ETAPA VISUAL / DIAGNÓSTICOS)
  ├── Integración de diagnósticos con interfaz sobria y textos públicos accesibles.
  └── Cumplimiento de «Interfaz más fácil de usar» y directrices de Modo Senior.
       │
       ▼
[0056 — tests] (RESILIENCIA Y ESTRÉS)
  ├── Simulación de fallos inyectados, contención, timeouts forzados y cancelación multihilo.
  └── Verificación exhaustiva de cero procesos zombis y estabilidad de memoria en Windows.
```

### Invariantes de dominio para CI Windows job

1. **Precedencia determinista de etapas (DAG)**:
   - `EnvironmentCheck` debe ser la primera etapa del pipeline.
   - `FormatVerification` y `StaticAnalysis` deben preceder a la compilación para evitar compilar código no conforme.
   - `ConfigurePreset` debe ejecutarse antes de `BuildCompilation`.
   - `BuildCompilation` debe ejecutarse y ser exitosa antes de invocar `TestSuiteExecution` o `SanitizerVerification`.
   - `ArtifactPackaging` sólo puede ejecutarse si la compilación y los tests resultaron completamente exitosos.
2. **Aislamiento y seguridad de rutas en Windows**:
   - El directorio de compilación debe residir estrictamente bajo subcarpetas autorizadas del proyecto (`build-ci-win-debug`, `build-ci-win-release`, `_build`).
   - Se rechaza categóricamente cualquier intento de Directory Traversal (`..`, `../`, `..\`, `..\\`), rutas con letra de volumen (`C:`, `D:`), rutas UNC de red (`\\server\share`), prefijos de espacio de nombres NT (`\\?\`, `\\.\`), y nombres reservados de dispositivo en Windows (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`).
3. **Presupuesto estricto de memoria y CPU**:
   - El presupuesto de memoria RAM para la ejecución de integración continua en Windows está acotado a un máximo de 4 GB (`maxMemoryBytes`).
   - El tiempo de ejecución por prueba unitaria individual está limitado a 15 segundos en CTest.
   - El tiempo de ejecución global del trabajo no puede superar los 600 segundos (10 minutos) en Debug y 900 segundos (15 minutos) en Release.
4. **Política Zero-Zombie**:
   - Toda invocación de proceso debe contar con timeout explícito y canal de cancelación cooperativa (`CancellationToken`).
   - No se permiten procesos huérfanos o colgados a la espera indefinida de EOF.
5. **Local-First constitucional (ADR-001 / Decisión Q-001)**:
   - Toda la validación, análisis y ejecución de pruebas opera en modo 100% local sin requerir conexión a redes externas ni telemetría remota.

---

## Archivos afectados

| Archivo | Acción | Rol / Contenido |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/CiWindowsJobTypes.h` | Crear (fase APPLY) | Definición de tipos enumerados (`CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsJobError`), estructuras inmutables (`CiWindowsEnvironment`, `CiWindowsJobPlan`, `CiWindowsJobReport`) y conversiones de texto. |
| `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.h` | Crear (fase APPLY) | Declaración de la entidad canónica de configuración de CI Windows, planes oficiales debug/release e invariantes. |
| `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.cpp` | Crear (fase APPLY) | Implementación de planes canónicos, verificación de invariantes y filtrado de versiones de Windows compatibles. |
| `apps/vigilio-browser/src/core/CiWindowsJobValidator.h` | Crear (fase APPLY) | Declaración del validador estático de planes, secuencias de etapas, entorno y contención de rutas Windows. |
| `apps/vigilio-browser/src/core/CiWindowsJobValidator.cpp` | Crear (fase APPLY) | Implementación de las reglas de validación topológica de etapas, seguridad de rutas Windows y pistas de remediación. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificar (fase APPLY) | Inclusión de las fuentes en `vigilio_core` y registro del ejecutable de prueba `vigilio_ci_windows_job_tests` en CTest. |
| `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp` | Crear (fase APPLY) | Batería de 11 pruebas unitarias deterministas en C++20 para validar el modelo de dominio y sus invariantes. |
| `openspec/changes/module-foundations-task0053/proposal.md` | Modificar (fase PROPOSE) | Propuesta de cambio OpenSpec detallada (este documento). |
| `openspec/changes/module-foundations-task0053/design.md` | Modificar (fase PROPOSE) | Documento de diseño arquitectónico y contratos de integración. |
| `openspec/changes/module-foundations-task0053/tasks.md` | Modificar (fase PROPOSE) | Desglose ordenado y verificable de tareas de implementación. |
| `openspec/changes/module-foundations-task0053/execution-memory.md` | Modificar (fase PROPOSE) | Registro de memoria de ejecución, autoridades, invariantes y evidencia. |

---

## Contrato UX/UI

- **Sin cambio visual directo en la ventana de navegación**:
  - Al tratarse de una capacidad de infraestructura de integración continua y calidad de compilación en Windows perteneciente a `foundations`, esta etapa no altera la ventana principal (`BrowserWindow.cpp`), ni agrega barras de estado, botones o menús visibles al usuario final.
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Se prohíbe taxativamente la inclusión de términos técnicos internos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner» o «linter» en cualquier mensaje devuelto por las funciones de diagnóstico.
  - Toda referencia a la modalidad accesible de la interfaz utiliza exclusivamente la etiqueta pública aprobada: **«Interfaz más fácil de usar»**.
  - Los nombres de las etapas y los mensajes de estado se redactan en español neutro claro y accesible:
    - `EnvironmentCheck`: «Verificación del entorno de compilación en Windows».
    - `FormatVerification`: «Comprobación de formato de código».
    - `StaticAnalysis`: «Análisis estático de calidad de código».
    - `ConfigurePreset`: «Configuración de compilación del sistema».
    - `BuildCompilation`: «Compilación del proyecto».
    - `TestSuiteExecution`: «Ejecución de pruebas automatizadas».
    - `SanitizerVerification`: «Verificación de sanitizadores de memoria».
    - `ArtifactPackaging`: «Validación de empaquetado del instalador».

---

## Reglas y fallos

| Escenario de fallo / Condición anómala | Detección / Validación | Código de Error | Comportamiento del Sistema y Recuperación |
| --- | --- | --- | --- |
| Plan sin etapas definidas o lista vacía | `validatePlan` | `CiWindowsJobError::InvalidWorkflowSchema` | El plan es rechazado; se emite diagnóstico indicando que el pipeline requiere al menos las etapas de verificación, configuración, compilación y pruebas. |
| Etapa de compilación invocada antes de configuración | `validateStepSequence` | `CiWindowsJobError::DependencyOrderingViolation` | Se detecta violación de precedencia; se impide la compilación huérfana y se exige anteponer la etapa `ConfigurePreset`. |
| Etapa de pruebas invocada antes de compilar | `validateStepSequence` | `CiWindowsJobError::DependencyOrderingViolation` | Se detecta que no existen binarios construidos; se detiene la validación y se exige completar exitosamente la compilación previa. |
| Ruta de compilación con intento de Directory Traversal (`..\..` o `../..`) | `isSafeBuildPath` | `CiWindowsJobError::UnsafePipelinePath` | Rechazo inmediato de la ruta; no se accede al sistema de archivos y se confina la operación dentro del árbol de trabajo autorizado. |
| Ruta con letra de unidad absoluta (`C:\`, `D:/`) | `isSafeBuildPath` | `CiWindowsJobError::UnsafePipelinePath` | Rechazo inmediato; se emite pista de remediación exigiendo el uso de subdirectorios locales relativos (`build-ci-win-debug`, `_build`). |
| Ruta que utiliza un nombre de dispositivo DOS reservado (`CON`, `PRN`, `AUX`, `NUL`) | `isSafeBuildPath` | `CiWindowsJobError::UnsafePipelinePath` | Rechazo inmediato; previene bloqueos del kernel de Windows ante nombres reservados del subsistema DOS. |
| Entorno sin versión compatible de Windows (build anterior a Windows 10 19045) | `validateEnvironment` | `CiWindowsJobError::ToolchainMissing` | Se marca el entorno como no conforme; se reporta la necesidad de Windows 10 Versión 22H2 o superior. |
| Entorno sin compilador C++20 compatible (MSVC < 19.38 o Clang-cl < 18) | `validateEnvironment` | `CiWindowsJobError::ToolchainMissing` | Se rechaza el entorno; se indica que Vigilio Browser requiere MSVC v143 (Visual Studio 2022) o Clang-cl 18+. |
| Entorno sin Windows SDK instalado o versión obsoleta (< 10.0.22621.0) | `validateEnvironment` | `CiWindowsJobError::WindowsSdkMissing` | Se reporta la ausencia de Windows SDK necesario para los contratos de compilación y enlaces nativos. |
| Tiempo límite de plan nulo, negativo o excesivo (> 3600 s) | `validatePlan` | `CiWindowsJobError::InvalidWorkflowSchema` | Se rechaza la configuración del plan; se restablece el tiempo límite al valor canónico seguro (600 s en Debug, 900 s en Release). |
| Presupuesto de memoria excesivo (> 8 GB) o nulo | `validatePlan` | `CiWindowsJobError::MemoryLimitExceeded` | Se ajusta el límite al presupuesto canónico máximo estricto de 4 GB (`4294967296` bytes). |

---

## Seguridad y privacidad

1. **Aislamiento Local-First (ADR-001 / Decisión Q-001)**:
   - Toda la definición, validación y planificación de CI Windows opera exclusivamente de forma local sin invocar APIs en la nube, servidores remotos de compilación ni enviar telemetría.
2. **Defensa en profundidad en rutas Windows**:
   - `isSafeBuildPath` normaliza y sanitiza separadores de ruta tanto directos (`/`) como inversos (`\`).
   - Rechaza explícitamente:
     - Componentes de directorio padre (`..`).
     - Rutas absolutas con letra de disco (`C:`, `c:`, etc.).
     - Prefijos UNC (`\\server\share`).
     - Espacios de nombres de dispositivo NT (`\\?\`, `\\.\`).
     - Nombres de dispositivos reservados heredados de DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1` a `COM9`, `LPT1` a `LPT9`), independientemente de si llevan extensiones (`NUL.txt`).
3. **Presupuesto de recursos y límites estrictos**:
   - Memoria RAM limitada a 4 GB por defecto para evitar agotamiento de recursos en el sistema anfitrión.
   - Timeouts deterministas con corte estricto ante bloqueo o contención prolongada.

---

## Matriz de pruebas

| Identificador | Criterio de Verificación | Tipo de Prueba | Fixture / Entrada | Comando / Ejecución | Resultado Esperado | Fallo Negativo / Caso Inválido |
| --- | --- | --- | --- | --- | --- | --- |
| `TEST-001` | Plan canónico Debug válido y conforme | Unitario (APPLY) | `canonicalDebugPlan()` | `ctest -R ci_windows_job -V` | Preset `ci-windows-debug`, 6 etapas, timeout 600 s, memoria 4 GB, failFast activado | Falla si el preset no coincide o etapas < 6 |
| `TEST-002` | Plan canónico Release válido con empaquetado | Unitario (APPLY) | `canonicalReleasePlan()` | `ctest -R ci_windows_job -V` | Preset `ci-windows-release`, 7 etapas incluyendo `ArtifactPackaging`, timeout 900 s | Falla si omite empaquetado o preset incorrecto |
| `TEST-003` | Validación exitosa de planes canónicos | Unitario (APPLY) | Planes canónicos | `ctest -R ci_windows_job -V` | `validatePlan` retorna `CiWindowsJobError::None` | Falla si emite error ante plan canónico |
| `TEST-004` | Rechazo de planes con esquemas inválidos | Unitario (APPLY) | Plan con etapas vacías, timeout 0, preset vacío | `ctest -R ci_windows_job -V` | Retorna `CiWindowsJobError::InvalidWorkflowSchema` | Falla si acepta plan vacío o timeout no positivo |
| `TEST-005` | Validación de precedencia y orden topológico | Unitario (APPLY) | Etapas desordenadas (build antes de config, test antes de build) | `ctest -R ci_windows_job -V` | Retorna `CiWindowsJobError::DependencyOrderingViolation` | Falla si admite compilar sin previa configuración |
| `TEST-006` | Validación de entorno y toolchain Windows | Unitario (APPLY) | Entorno válido vs entorno con MSVC obsoleto o SDK ausente | `ctest -R ci_windows_job -V` | Retorna `None` para válido; `ToolchainMissing` o `WindowsSdkMissing` para inválido | Falla si tolera entorno sin Windows SDK o C++20 |
| `TEST-007` | Detección y rechazo de Directory Traversal en Windows | Unitario (APPLY) | Rutas `..\build`, `build/../../etc`, `..\\windows\\sys` | `ctest -R ci_windows_job -V` | `isSafeBuildPath` retorna `false` | Falla si permite rutas que escapan de la raíz |
| `TEST-008` | Rechazo de letras de unidad y nombres reservados DOS | Unitario (APPLY) | Rutas `C:\build`, `D:/temp`, `CON`, `NUL.txt`, `AUX` | `ctest -R ci_windows_job -V` | `isSafeBuildPath` retorna `false` | Falla si acepta letras de volumen o dispositivos DOS |
| `TEST-009` | Filtro oficial de versiones de Windows compatibles | Unitario (APPLY) | Win 10 22H2, Win 11 23H2, Win Server 2022 vs Win 7, Win XP | `ctest -R ci_windows_job -V` | Retorna `true` para versiones modernas, `false` para obsoletas | Falla si admite Windows 7 o versiones no mantenidas |
| `TEST-010` | Modelo de datos de opciones, resultados y reportes | Unitario (APPLY) | Estructuras inmutables instanciadas | `ctest -R ci_windows_job -V` | Comparación por defecto `operator==`, conteo de exitosos/fallidos correcto | Falla si los contadores o estados difieren |
| `TEST-011` | Cumplimiento estricto de UX Copy público | Unitario (APPLY) | Todas las cadenas de estado, error, etapa y remediación | `ctest -R ci_windows_job -V` | Cadenas en español neutro; cero términos proscritos de `docs/UX-COPY-PUBLICO.md` | Falla si contiene «Senior Mode», «DAG», «toolchain» |
| `TEST-LATER` | Servicio ejecutor de subprocesos y timeouts en Windows | Integración (LATER) | `CiWindowsJobService` (task0054) | CTest en etapa implementation | Coordinación asíncrona de procesos y presets | N/A en fase domain |
| `TEST-NA` | Proceso Chromium WebEngine o red externa | N/A | Entornos cloud o GUI compleja | Proscrito por política Local-First | Totalmente desacoplado de WebEngine y red | N/A en todas las etapas |

---

## Pasos APPLY

El incremento para la etapa **domain** se implementará de forma determinista y modular en la fase APPLY subsiguiente:

1. **Paso 1: Definir tipos de dominio en `apps/vigilio-browser/src/core/CiWindowsJobTypes.h`**:
   - Crear enumeraciones `CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`.
   - Definir estructuras inmutables `CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport` y `CancellationToken`.
2. **Paso 2: Declarar la clase `CiWindowsJobConfiguration` en `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.h`**:
   - Declarar métodos estáticos `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants(const CiWindowsJobPlan&)` e `isSupportedWindowsVersion(std::string_view)`.
3. **Paso 3: Implementar la configuración canónica en `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.cpp`**:
   - Definir planes oficiales para Debug (`ci-windows-debug`, `build-ci-win-debug`) y Release (`ci-windows-release`, `build-ci-win-release`), verificando invariantes y catálogo de versiones de Windows.
4. **Paso 4: Declarar el validador en `apps/vigilio-browser/src/core/CiWindowsJobValidator.h`**:
   - Declarar métodos estáticos `validatePlan`, `validateStepSequence`, `validateEnvironment`, `isSafeBuildPath` y conversores de UX copy.
5. **Paso 5: Implementar las reglas de validación en `apps/vigilio-browser/src/core/CiWindowsJobValidator.cpp`**:
   - Implementar verificación topológica de orden de etapas, sanitización de rutas para Windows (rechazo de `..`, `C:\`, `CON`, `NUL`) y generación de pistas de remediación en español neutro.
6. **Paso 6: Registrar las fuentes en `apps/vigilio-browser/CMakeLists.txt`**:
   - Incorporar los nuevos archivos en la biblioteca estática `vigilio_core` y definir el target de prueba `vigilio_ci_windows_job_tests` en CTest con `LABELS "unit;fast"`.
7. **Paso 7: Implementar la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`**:
   - Implementar 11 pruebas unitarias deterministas que cubran casos válidos, inválidos, fronteras de ruta, orden de etapas, versiones de Windows y auditoría de UX Copy.
8. **Paso 8: Verificación continua y certificación**:
   - Ejecutar `apps/vigilio-browser/scripts/check.sh`, validar formato con Clang-Format, verificar con Clang-Tidy y asegurar 100% de éxito en CTest (22 de 22 pruebas pasando).

---

## Definición de terminado

La etapa **domain** de **CI Windows job** se considerará terminada al cumplirse la totalidad de los siguientes criterios:
1. Las cabeceras e implementaciones de dominio compilan limpiamente en C++20 con `-Werror` y cero advertencias.
2. El target `vigilio_ci_windows_job_tests` ejecuta y aprueba al 100% las 11 pruebas unitarias en CTest en menos de 50 milisegundos.
3. Se verifica el rechazo absoluto de Directory Traversal y nombres de dispositivos DOS reservados en Windows.
4. No se introduce ninguna dependencia hacia `QApplication`, `QWidget`, `QWebEngineView` ni sockets de red externa.
5. Se audita programáticamente el 100% de los textos públicos devueltos, garantizando la total ausencia de términos técnicos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
6. Se preservan intactas las 21 pruebas unitarias e integración preexistentes en CTest (total 22 de 22 pruebas aprobadas).

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0052`: Pruebas de CI Linux job completadas y certificadas en `tests/unit/CiLinuxJobServiceTests.cpp` (commit `85c3567`).
  - CMake 3.28+ y compilador C++20 estándar en el entorno de desarrollo.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Diferencias de representación de rutas entre Linux (anfitrión de desarrollo) y Windows (plataforma objetivo).
    *Mitigación*: `isSafeBuildPath` implementa una normalización léxica exhaustiva que evalúa tanto separadores directos (`/`) como inversos (`\`), analizando los componentes independientemente de la plataforma en la que se ejecute la prueba.
  - *Riesgo 2*: Nombres de dispositivos reservados en Windows que bloquean procesos (`CON`, `PRN`, `AUX`, `NUL`, etc.).
    *Mitigación*: `isSafeBuildPath` comprueba expresamente el nombre base de cada segmento de ruta contra la lista de identificadores reservados DOS antes de cualquier evaluación de sistema de archivos.
  - *Riesgo 3*: Filtrado de jerga interna en diagnósticos o mensajes de error.
    *Mitigación*: Prueba unitaria dedicada (`test_ci_windows_ux_copy_public_compliance`) que audita programáticamente cada cadena de texto contra una lista de términos no permitidos.

---

## Commit propuesto

`docs(foundations): domain ci-windows-job`
