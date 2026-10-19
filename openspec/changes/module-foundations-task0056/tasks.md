# Tasks — module-foundations-task0056

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos de frontera y límites de configuración de planes de Windows (`totalTimeoutSeconds`, `maxMemoryBytes`, nombres vacíos) en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` incluye la función de prueba `test_ci_windows_plan_validation_boundaries` que evalúa exhaustivamente planes con tiempos límites extremos (0, valores negativos simulados, mayores a 3600), cuotas de memoria inválidas (0 bytes, valores mayores a 8 GB) y nombres de preset vacíos, comprobando que `validatePlan` retorna el código de error tipado correspondiente (`CiWindowsJobError::InvalidWorkflowSchema` o `CiWindowsJobError::MemoryLimitExceeded`).
  - *Comando*: `grep -E "test_ci_windows_plan_validation_boundaries" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`

- [ ] TASK-002: Implementar pruebas de violaciones de secuencias topológicas y dependencias entre etapas en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` incluye la función de prueba `test_ci_windows_step_sequence_topological_violations` que evalúa secuencias arbitrariamente desordenadas donde `TestSuiteExecution` precede a `BuildCompilation`, `ArtifactPackaging` precede a pruebas, pasos obligatorios omitidos y etapas duplicadas, comprobando que `validateStepSequence` emite `CiWindowsJobError::DependencyOrderingViolation` o `CiWindowsJobError::InvalidWorkflowSchema`.
  - *Comando*: `grep -E "test_ci_windows_step_sequence_topological_violations" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`

- [ ] TASK-003: Implementar pruebas de rechazo de ataques avanzados de Directory Traversal, dispositivos reservados DOS y prefijos UNC en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` incluye la función de prueba `test_ci_windows_advanced_path_traversal` que comprueba que `CiWindowsJobValidator::isSafeBuildPath` detecta y rechaza vectores como `build\\..\\..\\windows`, `..\\..\\system32`, prefijos UNC `//`, `\\\\`, rutas con caracteres nulos embebidos `\0` y nombres reservados DOS compuestos con extensiones arbitrarias (`aux.json`, `con.txt`, `nul.log`), retornando `false` y garantizando contención estricta.
  - *Comando*: `grep -E "test_ci_windows_advanced_path_traversal" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`

- [ ] TASK-004: Implementar pruebas de límites de versiones de Windows y toolchains C++20 en `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` incluye la función de prueba `test_ci_windows_environment_boundaries` que evalúa entornos con versiones de compiladores obsoletas (MSVC < 19.38, Clang-cl < 18, MinGW < 13), Windows SDK ausente o con build < 19041, herramientas esenciales faltantes (CMake < 3.28, Ninja < 1.12), arquitecturas no admitidas y versiones de Windows no soportadas (Windows 7, Windows 8, Windows XP), comprobando que `validateEnvironment` retorna el error tipado correspondiente (`CiWindowsJobError::ToolchainMissing` o `CiWindowsJobError::WindowsSdkMissing`).
  - *Comando*: `grep -E "test_ci_windows_environment_boundaries" apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`

- [ ] TASK-005: Implementar pruebas de concurrencia masiva multihilo (16 hilos) sobre ejecución de planes y etapas en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_massive_multithreading_concurrency` que ejecuta al menos 16 hilos (`std::thread`) concurrentes realizando simultáneamente operaciones de `executePlan` en modo `DryRun` y `executeStep` sobre una instancia compartida de `CiWindowsJobService`, comprobando ausencia total de carreras de datos (data races), contención sincronizada por `std::mutex` y finalización exitosa de todos los workers.
  - *Comando*: `grep -E "test_ci_windows_service_massive_multithreading_concurrency" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de estrés de cancelación cooperativa asíncrona multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_cooperative_cancellation_stress` que evalúa la activación concurrente de `CancellationToken` desde un hilo secundario durante la ejecución de un plan de 7 u 8 etapas, verificando interrupción inmediata en la etapa en curso, estado consolidado `CiWindowsJobStatus::Cancelled`, etapas posteriores omitidas (`CiWindowsStepStatus::Omitted`), y ausencia de subprocesos colgados.
  - *Comando*: `grep -E "test_ci_windows_service_cooperative_cancellation_stress" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de inyección de fallos de permisos en directorios de construcción en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_permission_failure_injection` que remueve los permisos de escritura de un directorio temporal (`std::filesystem::perms::owner_read`), invoca `cleanBuildDirectory`, comprobando que retorna limpiamente `false` con captura de mensaje de error descriptivo sin lanzar excepciones no controladas ni provocar abortos.
  - *Comando*: `grep -E "test_ci_windows_service_permission_failure_injection" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de recuperación ante contención transitoria y reintentos con retroceso exponencial en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_retry_backoff_transient` que evalúa la configuración operativa de `CiWindowsServiceOptions` (`maxRetries`, `retryBackoffMs`) y comprueba que una etapa con contención transitoria simulada de archivos se reintenta y concluye con `CiWindowsStepStatus::Success`.
  - *Comando*: `grep -E "test_ci_windows_service_retry_backoff_transient" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de evaluación comparativa directa de políticas de corte (`failFast` vs `continueOnError`) en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_fail_fast_vs_continue_on_error` que somete un plan con etapas fallidas simuladas a evaluación con `failFast == true` (verificando que aborta inmediatamente y marca el resto como `Omitted`) y con `continueOnError == true` (verificando que continúa ejecutando las etapas independientes para recabar diagnósticos completos).
  - *Comando*: `grep -E "test_ci_windows_service_fail_fast_vs_continue_on_error" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de síntesis CLI de Windows con rutas complejas y parámetros avanzados en `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`
  - *Criterio de verificación*: `CiWindowsJobServiceTests.cpp` incluye la función de prueba `test_ci_windows_service_cli_synthesis_advanced` que comprueba la síntesis determinista de líneas de comando CLI para etapas con nombres de preset con espacios, subdirectorios anidados, rutas de ejecutables de compilador con espacios (`C:\Program Files\...`) y banderas de sanitizers (`-fsanitize=address`).
  - *Comando*: `grep -E "test_ci_windows_service_cli_synthesis_advanced" apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`

- [ ] TASK-011: Auditar y validar cumplimiento estricto de textos públicos y UX copy en español conforme a `docs/UX-COPY-PUBLICO.md` en suites de pruebas de CI Windows
  - *Criterio de verificación*: `CiWindowsJobConfigurationTests.cpp` y `CiWindowsJobServiceTests.cpp` auditan programáticamente que el 100% de los estados de `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsJobError`, modos de ejecución y pistas de remediación emplean lenguaje natural en español neutro y carecen de terminología prohibida («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/CiWindowsJob*.cpp || echo "AUDIT_CLEAN"`

- [ ] TASK-012: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de suites de prueba en CTest aprobadas exitosamente (23+ tests en < 5s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-013: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0166.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0166-module.foundations.task0056.propose.lock.json --project-root .`

- [ ] TASK-014: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0056-ci-windows-job/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de CI Windows job.
  - *Comando*: `grep -E "module.foundations.task0056" docs/rules/01-modules/foundations/micro/0056-ci-windows-job/task.md`
