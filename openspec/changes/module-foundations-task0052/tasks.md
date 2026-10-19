# Tasks — module-foundations-task0052

Status: PROPOSED

- [ ] TASK-001: Implementar pruebas de casos de frontera y límites de configuración de planes (`totalTimeoutSeconds`, `maxMemoryBytes`, nombres vacíos) en `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` incluye la función de prueba `test_ci_linux_plan_validation_boundaries` que evalúa exhaustivamente planes con tiempos límites extremos (0, valores negativos simulados), cuotas de memoria inválidas (0 bytes, valores mayores a 64 GB) y nombres de preset vacíos, comprobando que `validatePlan` retorna el código de error tipado correspondiente (`CiLinuxJobError::InvalidWorkflowSchema` o `CiLinuxJobError::MemoryLimitExceeded`).
  - *Comando*: `grep -E "test_ci_linux_plan_validation_boundaries" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`

- [ ] TASK-002: Implementar pruebas de violaciones de secuencias topológicas y dependencias entre etapas en `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` incluye la función de prueba `test_ci_linux_step_sequence_topological_violations` que evalúa secuencias arbitrariamente desordenadas donde `TestSuiteExecution` precede a `BuildCompilation`, `ArtifactPackaging` precede a pruebas, pasos obligatorios omitidos y etapas duplicadas, comprobando que `validateStepSequence` emite `CiLinuxJobError::DependencyOrderingViolation` o `CiLinuxJobError::InvalidWorkflowSchema`.
  - *Comando*: `grep -E "test_ci_linux_step_sequence_topological_violations" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`

- [ ] TASK-003: Implementar pruebas de rechazo de ataques avanzados de Directory Traversal y rutas inseguras en `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` incluye la función de prueba `test_ci_linux_advanced_path_traversal` que comprueba que `CiLinuxJobValidator::isSafeBuildPath` detecta y rechaza vectores como `./../..`, `build/../../etc`, `..\\windows\\path`, `/etc/shadow`, `/var/run`, rutas relativas con caracteres nulos embebidos y cadenas vacías, retornando `false` y garantizando contención estricta.
  - *Comando*: `grep -E "test_ci_linux_advanced_path_traversal" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`

- [ ] TASK-004: Implementar pruebas de límites de versiones de compiladores y herramientas C++20 en `apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` incluye la función de prueba `test_ci_linux_environment_boundaries` que evalúa entornos con versiones de compiladores obsoletas (GCC < 14, Clang < 18), herramientas esenciales faltantes (CMake < 3.28, Ninja < 1.12), arquitecturas no admitidas y distribuciones no catalogadas, comprobando que `validateEnvironment` retorna `CiLinuxJobError::ToolchainMissing`.
  - *Comando*: `grep -E "test_ci_linux_environment_boundaries" apps/vigilio-browser/tests/unit/CiLinuxJobConfigurationTests.cpp`

- [ ] TASK-005: Implementar pruebas de concurrencia masiva multihilo (16 hilos) sobre ejecución de planes y etapas en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_massive_multithreading_concurrency` que ejecuta al menos 16 hilos (`std::thread`) concurrentes realizando simultáneamente operaciones de `executePlan` en modo `DryRun` y `executeStep` sobre una instancia compartida de `CiLinuxJobService`, comprobando ausencia total de carreras de datos (data races), contención sincronizada por `std::mutex` y finalización exitosa de todos los workers.
  - *Comando*: `grep -E "test_ci_linux_service_massive_multithreading_concurrency" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-006: Implementar pruebas de estrés de cancelación cooperativa asíncrona multihilo bajo la política Zero-Zombie en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_cooperative_cancellation_stress` que evalúa la activación concurrente de `CancellationToken` desde un hilo secundario durante la ejecución de un plan de 8 etapas, verificando interrupción inmediata en la etapa en curso, estado consolidado `CiLinuxJobStatus::Cancelled`, etapas posteriores omitidas (`CiLinuxStepStatus::Omitted`), y ausencia de subprocesos colgados.
  - *Comando*: `grep -E "test_ci_linux_service_cooperative_cancellation_stress" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-007: Implementar pruebas de inyección de fallos de permisos en directorios de construcción en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_permission_failure_injection` que remueve los permisos de escritura de un directorio temporal (`std::filesystem::perms::owner_read`), invoca `cleanBuildDirectory`, comprobando que retorna limpiamente `false` con captura de mensaje de error descriptivo sin lanzar excepciones no controladas ni provocar abortos.
  - *Comando*: `grep -E "test_ci_linux_service_permission_failure_injection" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-008: Implementar pruebas de recuperación ante contención transitoria y reintentos con retroceso exponencial en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_retry_backoff_transient` que evalúa la configuración operativa de `CiLinuxServiceOptions` (`maxRetries`, `retryBackoffMs`) y comprueba que una etapa con contención transitoria simulada se reintenta y concluye con `CiLinuxStepStatus::Success`.
  - *Comando*: `grep -E "test_ci_linux_service_retry_backoff_transient" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-009: Implementar pruebas de evaluación comparativa directa de políticas de corte (`failFast` vs `continueOnError`) en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_fail_fast_vs_continue_on_error` que somete un plan con etapas fallidas simuladas a evaluación con `failFast == true` (verificando que aborta inmediatamente y marca el resto como `Omitted`) y con `continueOnError == true` (verificando que continúa ejecutando las etapas independientes para recabar diagnósticos completos).
  - *Comando*: `grep -E "test_ci_linux_service_fail_fast_vs_continue_on_error" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-010: Implementar pruebas de síntesis CLI con rutas complejas y parámetros avanzados en `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`
  - *Criterio de verificación*: `CiLinuxJobServiceTests.cpp` incluye la función de prueba `test_ci_linux_service_cli_synthesis_advanced` que comprueba la síntesis determinista de líneas de comando CLI para etapas con nombres de preset con espacios, subdirectorios anidados y banderas de sanitizers (`-fsanitize=address,undefined`).
  - *Comando*: `grep -E "test_ci_linux_service_cli_synthesis_advanced" apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`

- [ ] TASK-011: Auditar y validar cumplimiento estricto de textos públicos y UX copy en español conforme a `docs/UX-COPY-PUBLICO.md` en suites de pruebas de CI Linux
  - *Criterio de verificación*: `CiLinuxJobConfigurationTests.cpp` y `CiLinuxJobServiceTests.cpp` auditan programáticamente que el 100% de los estados de `CiLinuxJobStatus`, `CiLinuxStepStatus`, `CiLinuxJobError`, modos de ejecución y pistas de remediación emplean lenguaje natural en español neutro y carecen de terminología prohibida («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - *Comando*: `grep -E "Senior Mode|Modo Senior|Prompt Builder|OpenSpec" apps/vigilio-browser/src/core/CiLinuxJob*.cpp || echo "AUDIT_CLEAN"`

- [ ] TASK-012: Ejecutar el pipeline de compilación limpia y pruebas continuas con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia bajo C++20 con `-Werror`, cero advertencias de compilador, verificación Clang-Format aprobada y 100% de suites de prueba en CTest aprobadas exitosamente (21+ tests en < 5s).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-013: Validar referencias canónicas y lockfile de ejecución con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` sobre todas las referencias canónicas del proyecto, y `verify-lock` certifica la integridad del archivo lock correspondiente a la ejecución 0154.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0154-module.foundations.task0052.propose.lock.json --project-root .`

- [ ] TASK-014: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0052-ci-linux-job/task.md`
  - *Criterio de verificación*: El documento canónico de la microtarea formaliza el cumplimiento de la etapa `tests` y se mantiene alineado con la matriz de pruebas de CI Linux job.
  - *Comando*: `grep -E "module.foundations.task0052" docs/rules/01-modules/foundations/micro/0052-ci-linux-job/task.md`
