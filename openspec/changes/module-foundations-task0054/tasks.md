# Tasks — module-foundations-task0054

Status: PROPOSED

- [ ] TASK-001: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/CiWindowsJobService.h`
  - *Criterio de verificación*: `CiWindowsJobService.h` declara la clase `CiWindowsJobService` en el namespace `vigilio` con métodos de ciclo de vida (`setPlan`, `plan`, `setOptions`, `options`), ejecución (`executePlan`, `executeStep`), saneamiento (`cleanBuildDirectory`), síntesis CLI (`buildCommandLine`) y diagnósticos (`executionModeToString`, `formatStepSummary`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `grep -E "class CiWindowsJobService|executePlan|executeStep|cleanBuildDirectory|buildCommandLine" apps/vigilio-browser/src/core/CiWindowsJobService.h`

- [ ] TASK-002: Implementar constructores, asignación de plan y opciones con sincronización mediante mutex en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `CiWindowsJobService.cpp` implementa constructores (por defecto con `canonicalDebugPlan()` y parametrizado con plan explícito), `setPlan`, `plan`, `setOptions` y `options` con protección de concurrencia mediante `std::mutex`.
  - *Comando*: `grep -E "canonicalDebugPlan|setPlan|options|mutex" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-003: Implementar la orquestación secuencial de pipeline, modo DryRun y medición precisa de tiempos en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `executePlan` valida el plan con `CiWindowsJobValidator::validatePlan`, recorre secuencialmente las etapas definidas, mide la duración individual y total con `std::chrono::steady_clock` y genera un `CiWindowsJobReport` consolidado.
  - *Comando*: `grep -E "executePlan|validatePlan|steady_clock|CiWindowsJobReport" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-004: Implementar la política Fail-Fast y tolerancia a fallos selectiva (`continueOnError`) en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: Si una etapa obligatoria falla y `failFast == true`, las etapas dependientes subsiguientes se marcan como `CiWindowsStepStatus::Omitted` y se aborta el pipeline; si `continueOnError == true`, se registra el fallo pero se continúa ejecutando el resto de etapas.
  - *Comando*: `grep -E "failFast|continueOnError|CiWindowsStepStatus::Omitted" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-005: Implementar la cancelación cooperativa multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `executePlan` y `executeStep` evalúan `cancel->isCancelled()` antes y durante cada etapa, interrumpiendo de inmediato la ejecución, evitando subprocesos colgados y emitiendo el estado `CiWindowsJobStatus::Cancelled`.
  - *Comando*: `grep -E "isCancelled|CiWindowsJobStatus::Cancelled" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-006: Implementar el saneamiento seguro del directorio de compilación, control de Directory Traversal y defensas Windows en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `cleanBuildDirectory` evalúa la ruta mediante `CiWindowsJobValidator::isSafeBuildPath`, rechazando secuencias `..` o rutas absolutas de sistema/dispositivos DOS, y eliminando de manera recursiva el directorio válido aplicando reintentos exponenciales ($50\text{ ms} \times 2^i$) ante contención de archivos.
  - *Comando*: `grep -E "cleanBuildDirectory|isSafeBuildPath|retryBackoffMs" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-007: Implementar la ejecución granular por etapa (`executeStep`) y síntesis determinista de argumentos CLI para Windows en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `executeStep` ejecuta etapas individuales de manera aislada; `buildCommandLine` sintetiza comandos reproducibles para CMake, Ninja, MSVC/Clang-cl, CTest, PowerShell y CPack conforme a la etapa de pipeline en Windows.
  - *Comando*: `grep -E "executeStep|buildCommandLine|powershell" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-008: Implementar funciones de formato y diagnósticos accesibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`
  - *Criterio de verificación*: `executionModeToString` y `formatStepSummary` devuelven descripciones claras en español neutro sin jerga interna conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "executionModeToString|formatStepSummary" apps/vigilio-browser/src/core/CiWindowsJobService.cpp`

- [ ] TASK-009: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-CI-WIN-SRV-001` a `TEST-CI-WIN-SRV-012`), cubriendo inicialización, ejecución de pipeline en dry-run, política fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie, saneamiento seguro de directorios y rechazo de traversal/rutas host, ejecución de etapa individual, síntesis de línea de comandos CLI para Windows, límites de recursos y memoria, control de timeouts, reintentos con retroceso exponencial y auditoría de UX copy en español neutro.
  - *Comando*: `grep -E "test_ci_windows_service_initialization|test_ci_windows_service_dry_run_pipeline|test_ci_windows_service_fail_fast|test_ci_windows_service_cancellation" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-010: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_ci_windows_job_service_tests` y registrar `ci_windows_job_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/CiWindowsJobService.cpp` en `vigilio_core`, define `add_executable(vigilio_ci_windows_job_service_tests tests/unit/CiWindowsJobServiceTests.cpp)` y registra `add_test(NAME ci_windows_job_service_tests COMMAND vigilio_ci_windows_job_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "ci_windows_job_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-011: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (23/23 tests esperados) en menos de 4 segundos.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-012: Validar referencias canónicas y lockfile con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0160, y `task.md` formaliza el estado completado de la etapa `implementation`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && grep -E "module.foundations.task0054" docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md`
