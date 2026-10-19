# Change Proposal — module-foundations-task0049

Pack: `module.foundations.task0049`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **CI Linux job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0049 — Definir el comportamiento y las invariantes: CI Linux job`).

Habiéndose completado y certificado formalmente en las capacidades precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de diagnósticos y advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política integral de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la configuración canónica de Clang-Format (`task0041`-`task0044`), y la configuración canónica de Clang-Tidy (`task0045`-`task0048`), esta propuesta formaliza el **modelo de dominio, tipos fuertemente tipados, estructuras inmutables de planificación de pipeline, validador de secuencia de etapas y dependencias, políticas de contención y seguridad de rutas de compilación, verificación de entorno y catálogo canónico para el CI Linux job en Vigilio Browser**.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de CI Linux job (`CiLinuxJobTypes.h`)**:
  - `CiLinuxPipelineStep`: Etapas canónicas y secuenciales del pipeline de integración continua (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`).
  - `CiLinuxJobStatus`: Estados de ciclo de vida del trabajo (`Pending`, `Running`, `Passed`, `Failed`, `Cancelled`, `TimedOut`).
  - `CiLinuxStepStatus`: Estados de ejecución por etapa (`NotStarted`, `InProgress`, `Success`, `Failure`, `Omitted`).
  - `CiLinuxJobError`: Clasificación exhaustiva y tipada de errores de integración continua (`None`, `ToolchainMissing`, `ConfigurationFailed`, `CompilationFailed`, `TestExecutionFailed`, `FormatDiscrepancy`, `StaticAnalysisIssue`, `UnsafePipelinePath`, `TimeoutExceeded`, `InvalidWorkflowSchema`, `DependencyOrderingViolation`, `MemoryLimitExceeded`).
  - `CiLinuxArchitecture`: Arquitecturas de CPU admitidas en el entorno Linux (`X86_64`, `AArch64`).
  - `CiLinuxCompiler`: Compiladores C++20 admitidos (`GCC`, `Clang`).
  - `CiLinuxBuildConfiguration`: Modos de construcción soportados (`Debug`, `Release`, `Sanitizer`).
- **Estructuras inmutables de entorno, etapas, plan y reporte**:
  - `CiLinuxEnvironment`: Metadatos del entorno de ejecución (distribución Linux, versión de kernel, arquitectura de CPU, tipo y versión del compilador, versiones de CMake y Ninja, memoria física total y núcleos disponibles).
  - `CiLinuxStepOptions`: Opciones operativas por etapa (tiempo límite en milisegundos, política de tolerancia a fallos `continueOnError`, variables de entorno requeridas y directorio de trabajo relativo).
  - `CiLinuxStepDefinition`: Definición declarativa de etapa (identificador `CiLinuxPipelineStep`, nombre descriptivo en español, comando a sintetizar o ejecutar, opciones y obligatoriedad `isMandatory`).
  - `CiLinuxStepResult`: Resultado individual de ejecución de una etapa (identificador de etapa, estado `CiLinuxStepStatus`, código de salida del proceso, duración en milisegundos, resumen de salida textual y mensaje de error).
  - `CiLinuxJobPlan`: Plan inmutable de trabajo (nombre de preset canónico de CMake, directorio de compilación relativo seguro, lista ordenada de definiciones de etapas, tiempo límite global de ejecución, presupuesto máximo de memoria RAM acotado a 4 GB y bandera de corte ante primer fallo `failFast`).
  - `CiLinuxJobReport`: Reporte inmutable consolidado (estado global `CiLinuxJobStatus`, nombre del plan evaluado, duración total en milisegundos, vector ordenado de `CiLinuxStepResult`, conteo de etapas exitosas y fallidas, conformidad estricta y pista de remediación en lenguaje natural español).
- **Funciones puras de conversión y copy público conforme a `docs/UX-COPY-PUBLICO.md`**:
  - `jobStatusToString(CiLinuxJobStatus)` y `jobErrorToString(CiLinuxJobError)`: Textos claros en español neutro, libres de jerga interna proscrita («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - `stepToString(CiLinuxPipelineStep)` y `stepStatusToString(CiLinuxStepStatus)`: Nombres accesibles y estados descriptivos para cada etapa del pipeline.
  - `generateRemediationHint(CiLinuxJobError, std::string_view stepName)`: Orientación clara y accionable para resolver incidencias de compilación, tests o análisis estático.
- **Validador de reglas, seguridad de rutas y dependencias de etapas (`vigilio::CiLinuxJobValidator`)**:
  - Validador estático desacoplado en C++20 puro, ubicado en `vigilio_core`, libre de dependencias hacia `QApplication`, widgets de Qt o subprocesos de Chromium WebEngine.
  - `validatePlan(const CiLinuxJobPlan&)`: Verificación formal de coherencia del plan (lista de etapas no vacía, presencia de etapas obligatorias, ausencia de nombres duplicados, tiempo límite positivo y respeto riguroso del orden de dependencias: no compilar sin configurar, no ejecutar tests sin compilar, y comprobaciones estáticas previas a la compilación).
  - `validateStepSequence(const std::vector<CiLinuxPipelineStep>&)`: Comprobación estricta de orden topológico y prerequisitos entre etapas del pipeline.
  - `validateEnvironment(const CiLinuxEnvironment&)`: Verificación de requisitos mínimos del entorno (Linux x86_64 o aarch64, CMake >= 3.24, GCC >= 14 o Clang >= 18, y compatibilidad estándar C++20).
  - `isSafeBuildPath(const std::filesystem::path&)`: Prevención terminante de Directory Traversal (`..`, `../`, `..\\`), rechazo de rutas absolutas del host (`/`, `/etc`, `/usr/bin`) y restricción a subdirectorios autorizados de compilación (`build-ci-debug`, `build-ci-release`, `_build`).
- **Entidad de configuración canónica del CI Linux (`vigilio::CiLinuxJobConfiguration`)**:
  - Provisión de los planes canónicos oficiales: `canonicalDebugPlan()` (basado en el preset `ci-linux-debug`, generador Ninja, `CMAKE_BUILD_TYPE=Debug`, `VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `VIGILIO_WARNINGS_AS_ERRORS=ON`) y `canonicalReleasePlan()` (basado en `ci-linux-release`).
  - `verifyInvariants(const CiLinuxJobPlan&)`: Certificación de que el plan cumple con todas las invariantes de calidad y seguridad de Vigilio Browser.
  - Filtro oficial de distribuciones Linux compatibles (`isSupportedLinuxDistro`: Ubuntu 22.04/24.04 LTS, Debian 12, Fedora 39/40, Arch Linux).
  - Presupuestos estandarizados de recursos: límite de RAM <= 4 GB (`maxMemoryBytes`), tiempo límite por prueba individual <= 15 segundos y tiempo límite de job global <= 600 segundos (10 minutos).
- **Batería exhaustiva de pruebas unitarias de especificación (`CiLinuxJobConfigurationTests.cpp`)**:
  - 11 pruebas unitarias deterministas en CTest ejecutables en modo headless (< 50 ms), garantizando cero dependencias de red, sin instanciación de WebEngine y total aislamiento Local-First.

---

## Out of scope

- Implementación del servicio ejecutor de subprocesos o coordinador asíncrono (`CiLinuxJobService`), ejecución real de procesos Ninja/CMake o monitorización en tiempo de ejecución de procesos Linux en segundo plano (reservado estrictamente para la etapa `implementation` en `module.foundations.task0050`).
- Creación de widgets Qt, paneles visuales de diagnóstico de CI o diálogos de estado de integración continua en la interfaz de usuario (reservado para la etapa `ui` en `module.foundations.task0051`).
- Pruebas pesadas de estrés de ejecución paralela con simulación de timeouts forzados, inyección de fallos en procesos hijos y cancelación cooperativa multihilo (reservado para la etapa `tests` en `module.foundations.task0052`).
- Modificación directa de la ventana principal de navegación (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de elementos que saturen la experiencia visual del usuario final (proscrito por la política de diseño sobrio y `docs/UX-COPY-PUBLICO.md`).
- Conexión a servicios cloud remotos de CI (e.g. GitHub Actions API remota, GitLab CI remota, telemetría o webhooks externos), vetado taxativamente por la arquitectura constitucional Local-First (ADR-001 / Decisión Q-001).
- Modificación de código de aplicación o CMakeLists.txt fuera de las extensiones autorizadas para la fase PROPOSE (la implementación física ocurrirá en la fase APPLY una vez aprobada la propuesta).

---

## Contexto y evidencia

Se realizó una inspección exhaustiva de la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Presets canónicos existentes para CI Linux en `apps/vigilio-browser/CMakePresets.json`**:
   - `CMakePresets.json` define explícitamente los perfiles oficiales para Linux CI:
     ```json
     {
       "name": "ci-linux-debug",
       "displayName": "CI Linux Debug",
       "description": "Pipeline automatizado de integración continua en Linux con advertencias como errores",
       "generator": "Ninja",
       "binaryDir": "${sourceDir}/build-ci-debug",
       "cacheVariables": {
         "CMAKE_BUILD_TYPE": "Debug",
         "VIGILIO_BUILD_GUI": "OFF",
         "BUILD_TESTING": "ON",
         "VIGILIO_WARNINGS_AS_ERRORS": "ON"
       }
     },
     {
       "name": "ci-linux-release",
       "displayName": "CI Linux Release",
       "description": "Pipeline de validación de empaquetado y release en CI Linux",
       "generator": "Ninja",
       "binaryDir": "${sourceDir}/build-ci-release",
       "cacheVariables": {
         "CMAKE_BUILD_TYPE": "Release",
         "VIGILIO_BUILD_GUI": "OFF",
         "BUILD_TESTING": "ON",
         "VIGILIO_WARNINGS_AS_ERRORS": "ON"
       }
     }
     ```
   - No existía previamente en `apps/vigilio-browser/src/core/` una abstracción de dominio que modele, valide y orqueste programáticamente las etapas de este pipeline de CI en Linux.
2. **Script de verificación local actual (`apps/vigilio-browser/scripts/check.sh`)**:
   - `check.sh` realiza una secuencia lineal básica en shell:
     ```bash
     cmake -S "$ROOT" -B "$ROOT/_build" -G Ninja -DVIGILIO_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
     cmake --build "$ROOT/_build"
     ctest --test-dir "$ROOT/_build" --output-on-failure -L fast
     ```
   - Esta lógica está acoplada al shell Bash y carece de validación estructurada de etapas, modelos tipados de fallos, límites de memoria, reporte unificado o soporte para diagnósticos locales.
3. **Ausencia de tipos y validador de CI Linux en `vigilio_core`**:
   - Al igual que se modelaron `CMakePresets` (`CMakePresetsTypes.h`, `CMakePresetsValidator.h`), `ClangFormat` (`ClangFormatTypes.h`, `ClangFormatValidator.h`) y `ClangTidy` (`ClangTidyTypes.h`, `ClangTidyValidator.h`), se requiere un modelo de dominio fuertemente tipado para `CI Linux job` (`CiLinuxJobTypes.h`, `CiLinuxJobConfiguration.h`, `CiLinuxJobValidator.h`).
4. **Estado actual de construcción y pruebas**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - 19 de 19 pruebas unitarias e integración en CTest pasando exitosamente en 4.49s (100% pass, exit code 0).
5. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 49 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0145-module.foundations.task0049.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).

---

## Alcance exacto

La propuesta formaliza el incremento de la etapa **domain** de la capacidad **CI Linux job** en el módulo `foundations`:

1. **Definición del modelo de dominio y tipos inmutables (`CiLinuxJobTypes.h`)**:
   - Enumeraciones tipadas: `CiLinuxPipelineStep`, `CiLinuxJobStatus`, `CiLinuxStepStatus`, `CiLinuxJobError`, `CiLinuxArchitecture`, `CiLinuxCompiler`, `CiLinuxBuildConfiguration`.
   - Estructuras inmutables: `CiLinuxEnvironment`, `CiLinuxStepOptions`, `CiLinuxStepDefinition`, `CiLinuxStepResult`, `CiLinuxJobPlan`, `CiLinuxJobReport`.
   - Token ligero de cancelación cooperativa `CancellationToken` (con salvaguarda de inclusión múltiple).
   - Funciones de conversión a texto legible en español neutro conforme a `docs/UX-COPY-PUBLICO.md`: `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`.
2. **Motor de validación de reglas de pipeline, orden y seguridad (`CiLinuxJobValidator.h` / `.cpp`)**:
   - `CiLinuxJobError validatePlan(const CiLinuxJobPlan& plan)`: Comprobación de coherencia general e invariantes del plan.
   - `CiLinuxJobError validateStepSequence(const std::vector<CiLinuxPipelineStep>& steps)`: Comprobación de precedencia lógica y ausencia de violaciones de orden.
   - `CiLinuxJobError validateEnvironment(const CiLinuxEnvironment& env)`: Comprobación de herramientas mínimas de compilación y plataforma Linux.
   - `bool isSafeBuildPath(const std::filesystem::path& path)`: Detección y rechazo inmediato de escapes de directorio (`..`) y rutas absolutas no autorizadas.
   - `std::string generateRemediationHint(CiLinuxJobError error, std::string_view stepName)`: Diagnóstico descriptivo y guía de resolución en español claro.
3. **Entidad de configuración canónica (`CiLinuxJobConfiguration.h` / `.cpp`)**:
   - `CiLinuxJobPlan canonicalDebugPlan()`: Plan canónico de desarrollo y CI para depuración rápida.
   - `CiLinuxJobPlan canonicalReleasePlan()`: Plan canónico de validación de optimización y empaquetado.
   - `bool verifyInvariants(const CiLinuxJobPlan& plan)`: Verificación booleana estricta de cumplimiento de invariantes de dominio.
   - `bool isSupportedLinuxDistro(std::string_view distro)`: Validación de distribuciones Linux reconocidas.
4. **Integración en `vigilio_core` y suite de pruebas unitarias (`CiLinuxJobConfigurationTests.cpp`)**:
   - Adición de las fuentes al target estático `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt`.
   - Creación del ejecutable de pruebas `vigilio_ci_linux_job_tests` vinculado con `vigilio_compiler_warnings` y etiquetado con `LABELS "unit;fast"` en CTest.

---

## Diseño completo de la capacidad

El diseño de la capacidad **CI Linux job** se estructura en cuatro etapas ordenadas dentro del módulo `foundations`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       module.foundations (CI Linux job)                     │
└─────────────────────────────────────────────────────────────────────────────┘
       │
       ▼
[0049 — domain] (ESTA PROPUESTA)
  ├── CiLinuxJobTypes.h: Taxonomía tipada, etapas, estados, errores y reportes.
  ├── CiLinuxJobValidator.h/.cpp: Validación de dependencias, orden y seguridad.
  ├── CiLinuxJobConfiguration.h/.cpp: Planes canónicos e invariantes.
  └── CiLinuxJobConfigurationTests.cpp: 11 pruebas unitarias deterministas en CTest.
       │
       ▼
[0050 — implementation] (INCREMENTO SUBSIGUIENTE)
  ├── CiLinuxJobService.h/.cpp: Ejecución asíncrona, control de procesos y timeouts.
  └── Coordinación de presets CMake, CTest y captura controlada de salida.
       │
       ▼
[0051 — ui] (ETAPA VISUAL / DIAGNÓSTICOS)
  ├── Widgets Qt de estado de pipeline y diagnósticos de desarrollo en modo sobrio.
  └── Respeto a tokens visuales, escalado y «Interfaz más fácil de usar».
       │
       ▼
[0052 — tests] (RESILIENCIA Y ESTRÉS)
  ├── Simulación de fallos inyectados, contención, timeouts forzados y cancelación.
  └── Verificación exhaustiva de cero procesos zombis y estabilidad de memoria.
```

### Invariantes de dominio para CI Linux job

1. **Precedencia determinista de etapas**:
   - `EnvironmentCheck` debe ser la primera etapa del pipeline.
   - `FormatVerification` y `StaticAnalysis` deben preceder a la compilación para evitar compilar código no conforme.
   - `ConfigurePreset` debe ejecutarse antes de `BuildCompilation`.
   - `BuildCompilation` debe ejecutarse y ser exitosa antes de invocar `TestSuiteExecution` o `SanitizerVerification`.
   - `ArtifactPackaging` sólo puede ejecutarse si la compilación y los tests resultaron completamente exitosos.
2. **Aislamiento y seguridad de rutas**:
   - El directorio de compilación debe residir estrictamente bajo subcarpetas autorizadas del proyecto (`build-ci-debug`, `build-ci-release`, `_build`).
   - Se rechaza categóricamente cualquier intento de Directory Traversal (`..`, `../`, `..\\`) o rutas absolutas fijadas al host.
3. **Presupuesto estricto de memoria y CPU**:
   - El presupuesto de memoria RAM para la ejecución de integración continua en Linux está acotado a un máximo de 4 GB (`maxMemoryBytes`).
   - El tiempo de ejecución por prueba unitaria individual está limitado a 15 segundos en CTest.
   - El tiempo de ejecución global del trabajo no puede superar los 600 segundos (10 minutos).
4. **Política Zero-Zombie**:
   - Toda invocación de proceso debe contar con timeout explícito y canal de cancelación cooperativa (`CancellationToken`).
   - No se permiten procesos huérfanos o colgados a la espera indefinida de EOF.
5. **Local-First constitucional (ADR-001 / Decisión Q-001)**:
   - Toda la validación, análisis y ejecución de pruebas opera en modo 100% local sin requerir conexión a redes externas ni telemetría remota.

---

## Archivos afectados

| Archivo | Acción | Rol / Contenido |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/CiLinuxJobTypes.h` | Crear (fase APPLY) | Definición de enumeraciones tipadas (`CiLinuxPipelineStep`, `CiLinuxJobStatus`, `CiLinuxJobError`), estructuras inmutables (`CiLinuxEnvironment`, `CiLinuxJobPlan`, `CiLinuxJobReport`) y conversiones de UX copy. |
| `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h` | Crear (fase APPLY) | Declaración de la entidad canónica de configuración de CI Linux, planes oficiales debug/release e invariantes. |
| `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.cpp` | Crear (fase APPLY) | Implementación de planes canónicos, verificación de invariantes y filtrado de distribuciones Linux compatibles. |
| `apps/vigilio-browser/src/core/CiLinuxJobValidator.h` | Crear (fase APPLY) | Declaración del validador estático de planes, secuencias de etapas, entorno y contención de rutas. |
| `apps/vigilio-browser/src/core/CiLinuxJobValidator.cpp` | Crear (fase APPLY) | Implementación de las reglas de validación topológica de etapas, seguridad de rutas y generación de pistas de remediación. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificar (fase APPLY) | Inclusión de las fuentes en `vigilio_core` y registro del ejecutable de prueba `vigilio_ci_linux_job_tests` en CTest. |
| `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp` | Crear (fase APPLY) | Batería de 11 pruebas unitarias deterministas en C++20 para validar el modelo de dominio y sus invariantes. |
| `openspec/changes/module-foundations-task0049/proposal.md` | Modificar (fase PROPOSE) | Propuesta de cambio OpenSpec detallada (este documento). |
| `openspec/changes/module-foundations-task0049/design.md` | Modificar (fase PROPOSE) | Documento de diseño arquitectónico y contratos de integración. |
| `openspec/changes/module-foundations-task0049/tasks.md` | Modificar (fase PROPOSE) | Desglose ordenado y verificable de tareas de implementación. |
| `openspec/changes/module-foundations-task0049/execution-memory.md` | Modificar (fase PROPOSE) | Registro de memoria de ejecución, autoridades, invariantes y evidencia. |

---

## Contrato UX/UI

- **Sin cambio visual directo en la ventana de navegación**:
  - Al tratarse de una capacidad de infraestructura de integración continua y calidad de compilación en Linux perteneciente a `foundations`, esta etapa no altera la ventana principal (`BrowserWindow.cpp`), ni agrega barras de estado, botones o menús visibles al usuario final.
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Se prohíbe taxativamente la inclusión de términos técnicos internos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner» o «linter» en cualquier mensaje devuelto por las funciones de diagnóstico.
  - Toda referencia a la modalidad accesible de la interfaz utiliza exclusivamente la etiqueta pública aprobada: **«Interfaz más fácil de usar»**.
  - Los nombres de las etapas y los mensajes de estado se redactan en español neutro claro y accesible:
    - `EnvironmentCheck`: «Verificación del entorno de compilación».
    - `FormatVerification`: «Comprobación de formato de código».
    - `StaticAnalysis`: «Análisis estático de calidad de código».
    - `ConfigurePreset`: «Configuración de compilación del sistema».
    - `BuildCompilation`: «Compilación del proyecto».
    - `TestSuiteExecution`: «Ejecución de pruebas automatizadas».
    - `SanitizerVerification`: «Verificación de sanitizadores de memoria».
    - `ArtifactPackaging`: «Validación de empaquetado del navegador».

---

## Reglas y fallos

| Escenario de fallo / Condición anómala | Detección / Validación | Código de Error | Comportamiento del Sistema y Recuperación |
| --- | --- | --- | --- |
| Plan sin etapas definidas o lista vacía | `validatePlan` | `CiLinuxJobError::InvalidWorkflowSchema` | El plan es rechazado; se emite diagnóstico indicando que el pipeline requiere al menos las etapas de verificación, configuración, compilación y pruebas. |
| Etapa de compilación invocada antes de configuración | `validateStepSequence` | `CiLinuxJobError::DependencyOrderingViolation` | Se detecta violación de precedencia; se impide la compilación huérfana y se exige anteponer la etapa `ConfigurePreset`. |
| Etapa de pruebas invocada antes de compilar | `validateStepSequence` | `CiLinuxJobError::DependencyOrderingViolation` | Se detecta que no existen binarios construidos; se detiene la validación y se exige completar exitosamente la compilación previa. |
| Ruta de compilación con intento de Directory Traversal (`../..`) | `isSafeBuildPath` | `CiLinuxJobError::UnsafePipelinePath` | Rechazo inmediato de la ruta; no se accede al sistema de archivos y se confina la operación dentro del árbol de trabajo autorizado. |
| Ruta de compilación que apunta a directorio del sistema (`/etc`, `/usr`) | `isSafeBuildPath` | `CiLinuxJobError::UnsafePipelinePath` | Rechazo inmediato; se emite pista de remediación exigiendo el uso de subdirectorios locales estándar (`build-ci-debug`, `_build`). |
| Entorno con versión obsoleta de CMake (< 3.24) | `validateEnvironment` | `CiLinuxJobError::ToolchainMissing` | Se rechaza el entorno; se indica que Vigilio Browser requiere CMake 3.24 o superior para soporte nativo de CMakePresets versión 6. |
| Entorno sin compilador C++20 compatible (GCC < 14, Clang < 18) | `validateEnvironment` | `CiLinuxJobError::ToolchainMissing` | Se marca el entorno como no conforme; se reporta la necesidad de un compilador con soporte completo de C++20 estándar. |
| Tiempo límite de plan nulo o negativo | `validatePlan` | `CiLinuxJobError::InvalidWorkflowSchema` | Se rechaza la configuración del plan; se restablece el tiempo límite al valor canónico seguro (600 segundos). |
| Presupuesto de memoria excesivo (> 8 GB) o nulo | `validatePlan` | `CiLinuxJobError::MemoryLimitExceeded` | Se ajusta el límite al presupuesto canónico máximo estricto de 4 GB (`4294967296` bytes). |

---

## Seguridad y privacidad

1. **Aislamiento Local-First (ADR-001 / Decisión Q-001)**:
   - Todo el modelo de CI Linux opera en el entorno local del desarrollador o del runner de integración continua propio. No se envían trazas, telemetría, métricas ni metadatos a servidores externos o de Vigilio.
2. **Defensas contra Directory Traversal e inyección de rutas**:
   - `CiLinuxJobValidator::isSafeBuildPath` valida rigurosamente cualquier ruta de directorio de construcción, exigiendo que sea relativa y comience estrictamente por prefijos controlados (`build-ci-*`, `_build`), rechazando prefijos de escape (`..`) y caracteres nulos (`\0`).
3. **Límites de recursos y política Zero-Zombie**:
   - La estructura `CiLinuxJobPlan` establece un presupuesto inmutable de memoria de 4 GB (`maxMemoryBytes`) y tiempos máximos de ejecución por etapa y globales (600 s), impidiendo la degradación por consumo desmedido de RAM o procesos colgados indefinidamente.
4. **Protección de datos de navegación e incógnito**:
   - Las pruebas ejecutadas en el marco del CI Linux operan sobre perfiles temporales efímeros en directorios sintéticos aislados; ningún dato de pruebas persiste en el almacenamiento real del navegador.

---

## Matriz de pruebas

| Identificador de Prueba | Alcance | Fixture / Entrada | Comportamiento Esperado | Fallo Negativo / Verificación |
| --- | --- | --- | --- | --- |
| `test_ci_linux_canonical_debug_plan` | Unitario | `CiLinuxJobConfiguration::canonicalDebugPlan()` | Verifica preset `ci-linux-debug`, buildDir `build-ci-debug`, presencia de 6 etapas canónicas, timeout 600s, presupuesto 4 GB y `verifyInvariants == true`. | Si difiere en preset, etapas o invariantes, la prueba falla. |
| `test_ci_linux_canonical_release_plan` | Unitario | `CiLinuxJobConfiguration::canonicalReleasePlan()` | Verifica preset `ci-linux-release`, buildDir `build-ci-release`, presencia de etapas de verificación, empaquetado y validez de invariantes. | Si el preset de release no está presente o falla invariantes, la prueba falla. |
| `test_ci_linux_plan_validation_valid` | Unitario | Planes canónicos válidos | `CiLinuxJobValidator::validatePlan` retorna `CiLinuxJobError::None`. | Si retorna cualquier error sobre un plan canónico, falla. |
| `test_ci_linux_plan_validation_empty_steps` | Unitario | Plan con vector de etapas vacío | `validatePlan` retorna `CiLinuxJobError::InvalidWorkflowSchema`. | Si acepta un plan sin etapas, la prueba falla. |
| `test_ci_linux_step_sequence_ordering` | Unitario | Secuencia correcta vs. secuencia desordenada (compilar antes de configurar, tests antes de compilar) | Secuencia válida retorna `None`; secuencia con `BuildCompilation` antes de `ConfigurePreset` o `TestSuiteExecution` antes de `Build` retorna `DependencyOrderingViolation`. | Si no detecta transiciones ilegales de orden, la prueba falla. |
| `test_ci_linux_environment_validation` | Unitario | Entornos sintéticos (GCC 14, Clang 18, CMake 3.28 vs. GCC 9, CMake 3.10) | Entorno conforme retorna `None`; entorno con CMake < 3.24 o compilador obsoleto retorna `ToolchainMissing`. | Si acepta versiones incompatibles de compilador o CMake, falla. |
| `test_ci_linux_safe_build_path_traversal` | Unitario | Vectores de escape (`../build`, `foo/../../bar`, `..\\escaped`, `/etc`, `/usr/bin`) | `isSafeBuildPath` retorna `false` ante todo vector peligroso y `true` para `build-ci-debug`, `_build`, `build-ci-release`. | Si permite cualquier escape de directorio o ruta absoluta, falla. |
| `test_ci_linux_supported_distros` | Unitario | Cadenas de distribución: `Ubuntu`, `Debian`, `Fedora`, `Arch Linux`, `Solaris`, `Windows` | Retorna `true` para distribuciones soportadas y `false` para sistemas operativos o distribuciones no soportadas. | Si clasifica erróneamente una distribución, falla. |
| `test_ci_linux_step_options_and_results` | Unitario | Estructuras `CiLinuxStepOptions`, `CiLinuxStepResult`, `CiLinuxJobReport` | Valida asignación inmutable, cálculo de `isClean`, conteo de etapas exitosas y duraciones. | Si los reportes calculan erróneamente el éxito o duraciones, falla. |
| `test_ci_linux_ux_copy_public_compliance` | Unitario | Cadenas generadas por `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `generateRemediationHint` | Verifica programáticamente ausencia total de términos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») y presencia de español neutro accesible. | Si alguna cadena contiene jerga interna o texto en inglés no traducido, la prueba falla. |
| `test_ci_linux_remediation_hints` | Unitario | Todos los códigos de `CiLinuxJobError` | Cada código de error genera una pista de remediación no vacía, explicativa y accionable en español. | Si alguna pista es vacía o incomprensible, falla. |

---

## Pasos APPLY

1. **Creación de `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`**:
   - Declarar las enumeraciones `CiLinuxPipelineStep`, `CiLinuxJobStatus`, `CiLinuxStepStatus`, `CiLinuxJobError`, `CiLinuxArchitecture`, `CiLinuxCompiler`, `CiLinuxBuildConfiguration`.
   - Declarar las estructuras inmutables `CiLinuxEnvironment`, `CiLinuxStepOptions`, `CiLinuxStepDefinition`, `CiLinuxStepResult`, `CiLinuxJobPlan`, `CiLinuxJobReport`.
   - Incluir el token `CancellationToken` con salvaguarda `#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED`.
   - Declarar funciones puras de conversión textual conforme a `docs/UX-COPY-PUBLICO.md`.
2. **Creación de `apps/vigilio-browser/src/core/CiLinuxJobValidator.h` y `CiLinuxJobValidator.cpp`**:
   - Implementar los métodos estáticos puros `validatePlan`, `validateStepSequence`, `validateEnvironment`, `isSafeBuildPath` y `generateRemediationHint`.
   - Implementar las funciones de mapeo a cadenas en español neutro sin jerga técnica prohibida.
3. **Creación de `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h` y `CiLinuxJobConfiguration.cpp`**:
   - Implementar `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants()` e `isSupportedLinuxDistro()`.
4. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Agregar `src/core/CiLinuxJobValidator.cpp` y `src/core/CiLinuxJobConfiguration.cpp` a las fuentes de la biblioteca estática `vigilio_core`.
   - Declarar el ejecutable de pruebas `vigilio_ci_linux_job_tests` compilando `tests/unit/CiLinuxJobConfigurationTests.cpp`.
   - Vincular con `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar la prueba en CTest con `add_test(NAME ci_linux_job_configuration_tests COMMAND vigilio_ci_linux_job_tests)` y fijar sus propiedades a `LABELS "unit;fast" TIMEOUT 15`.
5. **Creación de `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`**:
   - Implementar la suite determinista de 11 pruebas unitarias cubriendo todos los casos de la matriz de pruebas.
6. **Ejecución y certificación de calidad**:
   - Ejecutar compilación completa con Ninja y verificar cero advertencias (`-Werror`).
   - Ejecutar la suite completa de pruebas en CTest verificando el 100% de éxito (20 de 20 pruebas aprobadas).
   - Ejecutar `check.sh` y certificar Clang-Format sobre los archivos modificados.

---

## Definición de terminado

- [ ] Las estructuras y tipos del modelo de dominio de CI Linux job están definidos en `CiLinuxJobTypes.h` con tipos estándar de C++20 y const-correctness.
- [ ] La entidad `CiLinuxJobConfiguration` provee los planes canónicos debug y release alineados exactamente con `apps/vigilio-browser/CMakePresets.json`.
- [ ] `CiLinuxJobValidator` valida el orden estricto de dependencias entre etapas, el entorno de compilación, la seguridad de rutas contra Directory Traversal y genera pistas de remediación claras.
- [ ] Todas las cadenas generadas cumplen con la directriz de lenguaje público de `docs/UX-COPY-PUBLICO.md`, sin jerga interna proscrita.
- [ ] Se añaden las fuentes a `vigilio_core` en `apps/vigilio-browser/CMakeLists.txt` y se registra la prueba `ci_linux_job_configuration_tests` en CTest.
- [ ] Las 11 pruebas unitarias de `CiLinuxJobConfigurationTests.cpp` pasan exitosamente en CTest en tiempo < 50 ms.
- [ ] La suite global de CTest reporta 20 de 20 pruebas aprobadas (100% pass) con código de salida 0.
- [ ] Cero regresiones en los componentes existentes de `foundations`.

---

## Dependencias y riesgos

- **Dependencias resueltas**:
  - `module.foundations.task0048` (pruebas exhaustivas de Clang-Tidy configuration) completada y certificada.
  - Perfiles de presets canónicos `ci-linux-debug` y `ci-linux-release` ya consolidados y verificados en `apps/vigilio-browser/CMakePresets.json`.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Posible ambigüedad en el orden de etapas entre formateo, análisis estático y compilación.
    *Mitigación*: `CiLinuxJobValidator::validateStepSequence` codifica de forma determinista la regla canónica: las verificaciones de calidad de código preceden obligatoriamente a la compilación para fail-fast inmediato.
  - *Riesgo 2*: Rutas absolutas o dependencias del host que impidan portabilidad en entornos de CI heterogéneos.
    *Mitigación*: Validación estricta en `isSafeBuildPath` exigiendo rutas relativas confinadas dentro de la raíz del proyecto.
  - *Riesgo 3*: Filtrado o fuga de términos técnicos en interfaces públicas.
    *Mitigación*: Prueba unitaria automatizada dedicada (`test_ci_linux_ux_copy_public_compliance`) que audita cada cadena devuelta contra una lista de términos prohibidos.

---

## Commit propuesto

`docs(foundations): domain ci-linux-job`
