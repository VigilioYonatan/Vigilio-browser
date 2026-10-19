# Tasks — module-foundations-task0058

Status: PROPOSED

- [ ] TASK-001: Declarar la interfaz pública desacoplada de la capa operativa de servicio en `apps/vigilio-browser/src/core/CiMacOsJobService.h`
  - *Criterio de verificación*: `CiMacOsJobService.h` declara la clase `CiMacOsJobService` en el espacio de nombres `vigilio` con métodos de ciclo de vida (`setPlan`, `plan`, `setOptions`, `options`), ejecución (`executePlan`, `executeStep`), saneamiento (`cleanBuildDirectory`), síntesis CLI (`buildCommandLine`) y diagnósticos (`executionModeToString`, `formatStepSummary`), libre de dependencias hacia `QApplication` o `QWidget`.
  - *Comando*: `grep -E "class CiMacOsJobService|executePlan|executeStep|cleanBuildDirectory|buildCommandLine" apps/vigilio-browser/src/core/CiMacOsJobService.h`

- [ ] TASK-002: Implementar constructores, asignación de plan y opciones con sincronización mediante mutex en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `CiMacOsJobService.cpp` implementa constructores (por defecto con `canonicalDebugPlan()` y parametrizado con plan explícito), `setPlan`, `plan`, `setOptions` y `options` con protección de concurrencia mediante `std::mutex`.
  - *Comando*: `grep -E "canonicalDebugPlan|setPlan|options|mutex" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-003: Implementar la orquestación secuencial de pipeline, modo DryRun y medición precisa de tiempos en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `executePlan` valida el plan con `CiMacOsJobValidator::validatePlan`, comprueba cuota de memoria ($\le 4\text{ GB}$), recorre secuencialmente las etapas definidas, mide la duración individual y total con `std::chrono::steady_clock` y genera un `CiMacOsJobReport` consolidado.
  - *Comando*: `grep -E "executePlan|validatePlan|steady_clock|CiMacOsJobReport" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-004: Implementar la política Fail-Fast y tolerancia a fallos selectiva (`continueOnError`) en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: Si una etapa obligatoria falla y `failFast == true`, las etapas dependientes subsiguientes se marcan como `CiMacOsStepStatus::Omitted` y se aborta el pipeline; si `continueOnError == true`, se registra el fallo pero se continúa ejecutando el resto de etapas.
  - *Comando*: `grep -E "failFast|continueOnError|CiMacOsStepStatus::Omitted" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-005: Implementar la cancelación cooperativa multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `executePlan` y `executeStep` evalúan `cancel->isCancelled()` antes y durante cada etapa, interrumpiendo de inmediato la ejecución, evitando subprocesos colgados y emitiendo el estado `CiMacOsJobStatus::Cancelled`.
  - *Comando*: `grep -E "isCancelled|CiMacOsJobStatus::Cancelled" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-006: Implementar el saneamiento seguro del directorio de compilación, control de Directory Traversal y defensas macOS en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `cleanBuildDirectory` evalúa la ruta mediante `CiMacOsJobValidator::isSafeBuildPath`, rechazando secuencias `..` o rutas absolutas de host (`/System`, `/Library`), y eliminando de manera recursiva el directorio válido aplicando reintentos exponenciales ($50\text{ ms} \times 2^i$) ante contención de archivos.
  - *Comando*: `grep -E "cleanBuildDirectory|isSafeBuildPath|retryBackoffMs" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-007: Implementar la ejecución granular por etapa (`executeStep`) y síntesis determinista de argumentos CLI para macOS en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `executeStep` ejecuta etapas individuales de manera aislada; `buildCommandLine` sintetiza comandos reproducibles para CMake presets, Ninja, Apple Clang, CTest y verificación de estructura del bundle `.app` nativo conforme a la etapa de pipeline en macOS.
  - *Comando*: `grep -E "executeStep|buildCommandLine|BundleVerification" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-008: Implementar funciones de formato y diagnósticos accesibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md` en `apps/vigilio-browser/src/core/CiMacOsJobService.cpp`
  - *Criterio de verificación*: `executionModeToString` y `formatStepSummary` devuelven descripciones claras en español neutro sin jerga técnica proscrita conforme a `docs/UX-COPY-PUBLICO.md`.
  - *Comando*: `grep -E "executionModeToString|formatStepSummary" apps/vigilio-browser/src/core/CiMacOsJobService.cpp`

- [ ] TASK-009: Implementar la batería exhaustiva de 12 pruebas unitarias operativas en `apps/vigilio-browser/tests/unit/CiMacOsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiMacOsJobServiceTests.cpp` implementa las 12 pruebas trazables (`TEST-CI-MAC-SRV-001` a `TEST-CI-MAC-SRV-012`), cubriendo inicialización, concurrencia de hilos, ejecución de pipeline en dry-run, política fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie, saneamiento seguro de directorios y rechazo de traversal/rutas host, ejecución de etapa individual, síntesis de línea de comandos CLI para macOS, límites de recursos y memoria, control de timeouts, reintentos con retroceso exponencial y auditoría de UX copy en español neutro.
  - *Comando*: `grep -E "test_ci_macos_service_initialization|test_ci_macos_service_dry_run_pipeline|test_ci_macos_service_fail_fast|test_ci_macos_service_cancellation" apps/vigilio-browser/tests/unit/CiMacOsJobServiceTests.cpp`

- [ ] TASK-010: Integrar los nuevos fuentes en `vigilio_core`, configurar el target ejecutable `vigilio_ci_macos_job_service_tests` y registrar `ci_macos_job_service_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: `CMakeLists.txt` incluye `src/core/CiMacOsJobService.cpp` en `vigilio_core`, define `add_executable(vigilio_ci_macos_job_service_tests tests/unit/CiMacOsJobServiceTests.cpp)` y registra `add_test(NAME ci_macos_job_service_tests COMMAND vigilio_ci_macos_job_service_tests)` con labels `unit;fast` y timeout de 15 segundos.
  - *Comando*: `grep -A 8 "ci_macos_job_service_tests" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-011: Ejecutar el pipeline de compilación limpia y pruebas continuas con CMake presets y CTest
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de pruebas en CTest pasando exitosamente (25/25 tests esperados) en modo headless.
  - *Comando*: `cd apps/vigilio-browser && cmake --preset ci-linux-debug && cmake --build --preset ci-linux-debug && ctest --preset ci-linux-debug`

- [ ] TASK-012: Validar referencias canónicas y lockfile con Prompt Builder CLI y sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0058-ci-macos-job/task.md`
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre las referencias canónicas del proyecto, `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0172, y `task.md` formaliza el estado completado de la etapa `implementation`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && grep -E "module.foundations.task0058" docs/rules/01-modules/foundations/micro/0058-ci-macos-job/task.md`
