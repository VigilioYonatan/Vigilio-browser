# Tasks — module-foundations-task0053

Status: PROPOSED

- [ ] TASK-001: Definir tipos de dominio, enumeraciones y estructuras inmutables en `apps/vigilio-browser/src/core/CiWindowsJobTypes.h`
  - *Criterio de verificación*: `CiWindowsJobTypes.h` define las enumeraciones fuertemente tipadas `CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`, así como las estructuras inmutables `CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport` y el token `CancellationToken` con salvaguarda de doble definición.
  - *Comando*: `grep -E "enum class CiWindowsPipelineStep|struct CiWindowsJobPlan" apps/vigilio-browser/src/core/CiWindowsJobTypes.h`

- [ ] TASK-002: Declarar la clase estática `CiWindowsJobConfiguration` en `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.h`
  - *Criterio de verificación*: `CiWindowsJobConfiguration.h` declara la clase `CiWindowsJobConfiguration` con constructor eliminado, exponiendo los métodos estáticos puros `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants(const CiWindowsJobPlan&)` e `isSupportedWindowsVersion(std::string_view)`.
  - *Comando*: `grep -E "canonicalDebugPlan|canonicalReleasePlan" apps/vigilio-browser/src/core/CiWindowsJobConfiguration.h`

- [ ] TASK-003: Implementar la configuración canónica, planes debug/release y filtros de versiones de Windows en `apps/vigilio-browser/src/core/CiWindowsJobConfiguration.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfiguration.cpp` implementa `canonicalDebugPlan()` vinculando el preset `ci-windows-debug` y el directorio `build-ci-win-debug`, `canonicalReleasePlan()` vinculando el preset `ci-windows-release` y el directorio `build-ci-win-release`, definiendo las etapas canónicas obligatorias, presupuestos de memoria de 4 GB y tiempos límite de 600 s y 900 s respectivamente.
  - *Comando*: `grep -E "ci-windows-debug|canonicalDebugPlan" apps/vigilio-browser/src/core/CiWindowsJobConfiguration.cpp`

- [ ] TASK-004: Declarar el motor estático de validación de pipeline y seguridad de rutas en `apps/vigilio-browser/src/core/CiWindowsJobValidator.h`
  - *Criterio de verificación*: `CiWindowsJobValidator.h` declara los métodos estáticos `validatePlan(const CiWindowsJobPlan&)`, `validateStepSequence(const std::vector<CiWindowsPipelineStep>&)`, `validateEnvironment(const CiWindowsEnvironment&)`, `isSafeBuildPath(const std::filesystem::path&)` y las funciones de traducción a texto accesible `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint`.
  - *Comando*: `grep -E "validatePlan|validateStepSequence|isSafeBuildPath" apps/vigilio-browser/src/core/CiWindowsJobValidator.h`

- [ ] TASK-005: Implementar las reglas de validación topológica, seguridad de rutas Windows y UX copy en `apps/vigilio-browser/src/core/CiWindowsJobValidator.cpp`
  - *Criterio de verificación*: `CiWindowsJobValidator.cpp` implementa la verificación estricta de orden de precedencia entre etapas (impidiendo compilación sin previa configuración, e impidiendo pruebas sin previa compilación), detección y rechazo de Directory Traversal (`..`, `..\`), letras de disco (`C:\`), dispositivos reservados DOS (`CON`, `PRN`, `AUX`, `NUL`) y textos públicos descriptivos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "DependencyOrderingViolation|isSafeBuildPath" apps/vigilio-browser/src/core/CiWindowsJobValidator.cpp`

- [ ] TASK-006: Registrar las fuentes en la biblioteca `vigilio_core` y configurar el ejecutable de pruebas en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `apps/vigilio-browser/CMakeLists.txt` incluye `src/core/CiWindowsJobConfiguration.cpp` y `src/core/CiWindowsJobValidator.cpp` en `add_library(vigilio_core STATIC ...)` y declara el target ejecutable `vigilio_ci_windows_job_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`, registrado en CTest bajo `LABELS "unit;fast"` con timeout de 15 segundos.
  - *Comando*: `grep -E "vigilio_ci_windows_job_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Implementar la suite de pruebas unitarias de dominio en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` implementa 11 pruebas unitarias deterministas que evalúan: plan canónico debug, plan canónico release, validación de planes válidos, rechazo de plan con etapas vacías, validación de orden topológico y precedencia de etapas, validación de toolchain Windows y SDK, rechazo de Directory Traversal en Windows (`..\`, `../`), rechazo de letras de unidad y dispositivos reservados DOS (`CON`, `NUL`), filtro de versiones de Windows compatibles, modelos de datos de opciones y resultados de etapas, y cumplimiento estricto de textos públicos y pistas de remediación.
  - *Comando*: `grep -E "test_ci_windows_canonical_debug_plan|test_ci_windows_safe_build_path_traversal" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`

- [ ] TASK-008: Auditar y validar el cumplimiento estricto de textos públicos y ausencia de jerga interna conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: La prueba unitaria `test_ci_windows_ux_copy_public_compliance` verifica programáticamente que la totalidad de los valores devueltos por `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint` están redactados en español neutro y no contienen términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "test_ci_windows_ux_copy_public_compliance" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
