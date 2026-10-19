# Tasks — module-foundations-task0057

Status: PROPOSED

- [ ] TASK-001: Definir tipos de dominio, enumeraciones y estructuras inmutables en `apps/vigilio-browser/src/core/CiMacOsJobTypes.h`
  - *Criterio de verificación*: `CiMacOsJobTypes.h` define las enumeraciones fuertemente tipadas `CiMacOsPipelineStep`, `CiMacOsJobStatus`, `CiMacOsStepStatus`, `CiMacOsExecutionMode`, `CiMacOsJobError`, `CiMacOsArchitecture`, `CiMacOsCompiler`, `CiMacOsBuildConfiguration`, así como las estructuras inmutables `CiMacOsServiceOptions`, `CiMacOsEnvironment`, `CiMacOsStepOptions`, `CiMacOsStepDefinition`, `CiMacOsStepResult`, `CiMacOsJobPlan`, `CiMacOsJobReport` y el token `CancellationToken` con salvaguarda de doble definición.
  - *Comando*: `grep -E "enum class CiMacOsPipelineStep|struct CiMacOsJobPlan" apps/vigilio-browser/src/core/CiMacOsJobTypes.h`

- [ ] TASK-002: Declarar la clase estática `CiMacOsJobConfiguration` en `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h`
  - *Criterio de verificación*: `CiMacOsJobConfiguration.h` declara la clase `CiMacOsJobConfiguration` con constructor eliminado, exponiendo los métodos estáticos puros `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants(const CiMacOsJobPlan&)` e `isSupportedMacOsVersion(std::string_view)`.
  - *Comando*: `grep -E "canonicalDebugPlan|canonicalReleasePlan" apps/vigilio-browser/src/core/CiMacOsJobConfiguration.h`

- [ ] TASK-003: Implementar la configuración canónica, planes debug/release y filtros de versiones de macOS en `apps/vigilio-browser/src/core/CiMacOsJobConfiguration.cpp`
  - *Criterio de verificación*: `CiMacOsJobConfiguration.cpp` implementa `canonicalDebugPlan()` vinculando el preset `ci-macos-debug` y el directorio `build-ci-macos-debug`, `canonicalReleasePlan()` vinculando el preset `ci-macos-release` y el directorio `build-ci-macos-release`, definiendo las etapas canónicas obligatorias, presupuestos de memoria de 4 GB y tiempos límite de 600 s y 900 s respectivamente.
  - *Comando*: `grep -E "ci-macos-debug|canonicalDebugPlan" apps/vigilio-browser/src/core/CiMacOsJobConfiguration.cpp`

- [ ] TASK-004: Declarar el motor estático de validación de pipeline y seguridad de rutas en `apps/vigilio-browser/src/core/CiMacOsJobValidator.h`
  - *Criterio de verificación*: `CiMacOsJobValidator.h` declara los métodos estáticos `validatePlan(const CiMacOsJobPlan&)`, `validateStepSequence(const std::vector<CiMacOsPipelineStep>&)`, `validateEnvironment(const CiMacOsEnvironment&)`, `isSafeBuildPath(const std::filesystem::path&)` y las funciones de traducción a texto accesible `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString` y `generateRemediationHint`.
  - *Comando*: `grep -E "validatePlan|validateStepSequence|isSafeBuildPath" apps/vigilio-browser/src/core/CiMacOsJobValidator.h`

- [ ] TASK-005: Implementar las reglas de validación topológica, seguridad de rutas macOS y UX copy en `apps/vigilio-browser/src/core/CiMacOsJobValidator.cpp`
  - *Criterio de verificación*: `CiMacOsJobValidator.cpp` implementa la verificación estricta de orden de precedencia entre etapas (impidiendo compilación sin previa configuración, e impidiendo pruebas sin previa compilación), detección y rechazo de Directory Traversal (`..`, `../`), rutas absolutas del host (`/System`, `/Library`, `/Applications`), artefactos macOS (`.DS_Store`, `._*`) y textos públicos descriptivos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "DependencyOrderingViolation|isSafeBuildPath" apps/vigilio-browser/src/core/CiMacOsJobValidator.cpp`

- [ ] TASK-006: Registrar las fuentes en la biblioteca `vigilio_core` y configurar el ejecutable de pruebas en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `apps/vigilio-browser/CMakeLists.txt` incluye `src/core/CiMacOsJobConfiguration.cpp` y `src/core/CiMacOsJobValidator.cpp` en `add_library(vigilio_core STATIC ...)` y declara el target ejecutable `vigilio_ci_macos_job_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`, registrado en CTest bajo `LABELS "unit;fast"` con timeout de 15 segundos.
  - *Comando*: `grep -E "vigilio_ci_macos_job_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Implementar la suite de pruebas unitarias de dominio en `apps/vigilio-browser/tests/unit/CiMacOsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiMacOsJobConfigurationTests.cpp` implementa 11 pruebas unitarias deterministas que evalúan: plan canónico debug, plan canónico release, validación de planes válidos, rechazo de plan con etapas vacías o límites erróneos, validación de orden topológico y precedencia de etapas, validación de toolchain macOS, SDK y herramientas Xcode, rechazo de Directory Traversal en macOS (`../`), rechazo de artefactos conflictivos de macOS (`.DS_Store`, `._*`), filtro de versiones de macOS compatibles (macOS 13, 14, 15 / Darwin 22, 23, 24), modelos de datos de opciones y resultados de etapas, y cumplimiento estricto de textos públicos y pistas de remediación.
  - *Comando*: `grep -E "test_ci_macos_canonical_debug_plan|test_ci_macos_safe_build_path_traversal" apps/vigilio-browser/tests/unit/CiMacOsJobConfigurationTests.cpp`

- [ ] TASK-008: Auditar y validar el cumplimiento estricto de textos públicos y ausencia de jerga interna conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: La prueba unitaria `test_ci_macos_ux_copy_public_compliance` verifica programáticamente que la totalidad de los valores devueltos por `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint` están redactados en español neutro y no contienen términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "test_ci_macos_ux_copy_public_compliance" apps/vigilio-browser/tests/unit/CiMacOsJobConfigurationTests.cpp`
