# Tasks — module-foundations-task0050

Status: PROPOSED

- [ ] TASK-001: Extender el modelo de tipos en `apps/vigilio-browser/src/core/CiLinuxJobTypes.h` incorporando `CiLinuxExecutionMode` y `CiLinuxServiceOptions`
  - *Criterio de verificación*: `CiLinuxJobTypes.h` define `enum class CiLinuxExecutionMode : uint8_t { DryRun, Simulated, Live };` y `struct CiLinuxServiceOptions` con parámetros inmutables y valores por defecto (`mode = DryRun`, `failFast = true`, `maxRetries = 3`, `retryBackoffMs = 50`, `captureOutput = true`), compilando limpiamente bajo C++20.
  - *Comando*: `grep -E "enum class CiLinuxExecutionMode|struct CiLinuxServiceOptions" apps/vigilio-browser/src/core/CiLinuxJobTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/CiLinuxJobService.h`
  - *Criterio de verificación*: `CiLinuxJobService.h` declara la clase `CiLinuxJobService` en el namespace `vigilio` con métodos de ciclo de vida (`setPlan`, `plan`, `setOptions`, `options`), ejecución (`executePlan`, `executeStep`), saneamiento (`cleanBuildDirectory`), síntesis CLI (`buildCommandLine`) y diagnósticos (`executionModeToString`, `formatStepSummary`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `grep -E "class CiLinuxJobService|executePlan|executeStep|cleanBuildDirectory|buildCommandLine" apps/vigilio-browser/src/core/CiLinuxJobService.h`

- [ ] TASK-003: Implementar la inicialización, configuración, gestión de estado y sincronización multihilo en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: `CiLinuxJobService.cpp` implementa constructores (por defecto con `canonicalDebugPlan()` y parametrizado con plan explícito), `setPlan`, `plan`, `setOptions` y `options` con protección de concurrencia mediante `std::mutex`.
  - *Comando*: `grep -E "canonicalDebugPlan|setPlan|options|mutex" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-004: Implementar la orquestación secuencial de pipeline, modo DryRun y medición precisa de tiempos en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: `executePlan` valida el plan con `CiLinuxJobValidator::validatePlan`, recorre secuencialmente las etapas definidas, mide la duración individual y total con `std::chrono::steady_clock` y genera un `CiLinuxJobReport` consolidado.
  - *Comando*: `grep -E "executePlan|validatePlan|steady_clock|CiLinuxJobReport" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-005: Implementar la política Fail-Fast y tolerancia a fallos selectiva (`continueOnError`) en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: Si una etapa obligatoria falla y `failFast == true`, las etapas dependientes subsiguientes se marcan como `CiLinuxStepStatus::Omitted` y se aborta el pipeline; si `continueOnError == true`, se registra el fallo pero se continúa ejecutando el resto de etapas.
  - *Comando*: `grep -E "failFast|continueOnError|CiLinuxStepStatus::Omitted" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-006: Implementar la cancelación cooperativa multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: `executePlan` y `executeStep` evalúan `cancel->isCancelled()` antes y durante cada etapa, interrumpiendo de inmediato la ejecución, evitando subprocesos colgados y emitiendo el estado `CiLinuxJobStatus::Cancelled`.
  - *Comando*: `grep -E "isCancelled|CiLinuxJobStatus::Cancelled" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-007: Implementar el saneamiento seguro del directorio de compilación y control de Directory Traversal en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: `cleanBuildDirectory` evalúa la ruta mediante `CiLinuxJobValidator::isSafeBuildPath`, rechazando secuencias `..` o rutas absolutas de sistema, y eliminando de manera recursiva el directorio válido aplicando reintentos exponenciales ($50\text{ ms} \times 2^i$) ante contención de archivos.
  - *Comando*: `grep -E "cleanBuildDirectory|isSafeBuildPath|retryBackoffMs" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-008: Implementar la síntesis determinista de argumentos CLI y funciones de copy público en español neutro en `apps/vigilio-browser/src/core/CiLinuxJobService.cpp`
  - *Criterio de verificación*: `buildCommandLine` sintetiza comandos reproducibles para CMake, Ninja, CTest y herramientas C++ conforme a la etapa de pipeline; `executionModeToString` y `formatStepSummary` devuelven descripciones claras en español neutro sin jerga interna conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "buildCommandLine|executionModeToString|formatStepSummary" apps/vigilio-browser/src/core/CiLinuxJobService.cpp`

- [ ] TASK-009: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-CI-LINUX-SRV-001` a `TEST-CI-LINUX-SRV-012`), cubriendo inicialización, ejecución de pipeline en dry-run, política fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie, saneamiento seguro de directorios y rechazo de traversal, ejecución de etapa individual, síntesis de línea de comandos CLI, límites de recursos y memoria, control de timeouts, reintentos con retroceso exponencial y auditoría de UX copy en español neutro.
  - *Comando*: `grep -E "test_ci_linux_service_initialization|test_ci_linux_service_dry_run_pipeline|test_ci_linux_service_fail_fast|test_ci_linux_service_cancellation" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-010: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_ci_linux_job_service_tests` y registrar `ci_linux_job_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/CiLinuxJobService.cpp` en `vigilio_core`, define `add_executable(vigilio_ci_linux_job_service_tests tests/unit/CiLinuxJobServiceTests.cpp)` y registra `add_test(NAME ci_linux_job_service_tests COMMAND vigilio_ci_linux_job_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "ci_linux_job_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-011: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (21/21 tests) en menos de 3 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-012: Validar referencias canónicas y lockfile con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0148, y `task.md` formaliza el estado completado de la etapa `implementation`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && grep -E "module.foundations.task0050" docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md`
