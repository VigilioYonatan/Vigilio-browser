# Tasks — module-foundations-task0049

Status: PROPOSED

- [ ] TASK-001: Definir tipos de dominio, enumeraciones y estructuras inmutables en `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`
  - *Criterio de verificación*: `CiLinuxJobTypes.h` define las enumeraciones fuertemente tipadas `CiLinuxPipelineStep`, `CiLinuxJobStatus`, `CiLinuxStepStatus`, `CiLinuxJobError`, `CiLinuxArchitecture`, `CiLinuxCompiler`, `CiLinuxBuildConfiguration`, así como las estructuras inmutables `CiLinuxEnvironment`, `CiLinuxStepOptions`, `CiLinuxStepDefinition`, `CiLinuxStepResult`, `CiLinuxJobPlan`, `CiLinuxJobReport` y el token `CancellationToken` con salvaguarda de doble definición.
  - *Comando*: `grep -E "enum class CiLinuxPipelineStep|struct CiLinuxJobPlan" apps/vigilio-browser/src/core/CiLinuxJobTypes.h`

- [ ] TASK-002: Declarar la clase estática `CiLinuxJobConfiguration` en `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h`
  - *Criterio de verificación*: `CiLinuxJobConfiguration.h` declara la clase `CiLinuxJobConfiguration` con constructor eliminado, exponiendo los métodos estáticos puros `canonicalDebugPlan()`, `canonicalReleasePlan()`, `verifyInvariants(const CiLinuxJobPlan&)` e `isSupportedLinuxDistro(std::string_view)`.
  - *Comando*: `grep -E "canonicalDebugPlan|canonicalReleasePlan" apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h`

- [ ] TASK-003: Implementar la configuración canónica, planes debug/release y filtros de distribución en `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfiguration.cpp` implementa `canonicalDebugPlan()` vinculando el preset `ci-linux-debug` y el directorio `build-ci-debug`, `canonicalReleasePlan()` vinculando el preset `ci-linux-release` y el directorio `build-ci-release`, definiendo las etapas canónicas obligatorias, presupuestos de memoria de 4 GB y tiempos límite de 600 s y 900 s respectivamente.
  - *Comando*: `grep -E "ci-linux-debug|canonicalDebugPlan" apps/vigilio-browser/src/core/CiLinuxJobConfiguration.cpp`

- [ ] TASK-004: Declarar el motor estático de validación de pipeline y seguridad de rutas en `apps/vigilio-browser/src/core/CiLinuxJobValidator.h`
  - *Criterio de verificación*: `CiLinuxJobValidator.h` declara los métodos estáticos `validatePlan(const CiLinuxJobPlan&)`, `validateStepSequence(const std::vector<CiLinuxPipelineStep>&)`, `validateEnvironment(const CiLinuxEnvironment&)`, `isSafeBuildPath(const std::filesystem::path&)` y las funciones de traducción a texto accesible `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint`.
  - *Comando*: `grep -E "validatePlan|validateStepSequence|isSafeBuildPath" apps/vigilio-browser/src/core/CiLinuxJobValidator.h`

- [ ] TASK-005: Implementar las reglas de validación topológica, seguridad de rutas y UX copy en `apps/vigilio-browser/src/core/CiLinuxJobValidator.cpp`
  - *Criterio de verificación*: `CiLinuxJobValidator.cpp` implementa la verificación estricta de orden de precedencia entre etapas (impidiendo compilación sin previa configuración, e impidiendo pruebas sin previa compilación), detección y rechazo de Directory Traversal (`..`) y rutas absolutas no autorizadas, y textos públicos descriptivos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "DependencyOrderingViolation|isSafeBuildPath" apps/vigilio-browser/src/core/CiLinuxJobValidator.cpp`

- [ ] TASK-006: Registrar las fuentes en la biblioteca `vigilio_core` y configurar el ejecutable de pruebas en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `apps/vigilio-browser/CMakeLists.txt` incluye `src/core/CiLinuxJobConfiguration.cpp` y `src/core/CiLinuxJobValidator.cpp` en `add_library(vigilio_core STATIC ...)` y declara el target ejecutable `vigilio_ci_linux_job_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`, registrado en CTest bajo `LABELS "unit;fast"` con timeout de 15 segundos.
  - *Comando*: `grep -E "vigilio_ci_linux_job_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-007: Implementar la suite de pruebas unitarias de dominio en `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` implementa 11 pruebas unitarias deterministas que evalúan: plan canónico debug, plan canónico release, validación de planes válidos, rechazo de plan con etapas vacías, validación de orden topológico y precedencia de etapas, validación de toolchain y entorno, rechazo de vectores de Directory Traversal en rutas de compilación, filtro de distribuciones Linux compatibles, modelos de datos de opciones y resultados de etapas, y cumplimiento estricto de textos públicos y pistas de remediación.
  - *Comando*: `grep -E "test_ci_linux_canonical_debug_plan|test_ci_linux_safe_build_path_traversal" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`

- [ ] TASK-008: Auditar y validar el cumplimiento estricto de textos públicos y ausencia de jerga interna conforme a `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: La prueba unitaria `test_ci_linux_ux_copy_public_compliance` verifica programáticamente que la totalidad de los valores devueltos por `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString` y `generateRemediationHint` están redactados en español neutro y no contienen términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "test_ci_linux_ux_copy_public_compliance" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
