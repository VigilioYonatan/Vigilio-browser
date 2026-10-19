# Change Proposal — module-foundations-task0056

Pack: `module.foundations.task0056`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **tests** de la capacidad **CI Windows job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0056 — Probar casos positivos, negativos y recuperación: CI Windows job`).

Habiéndose completado y certificado formalmente en las etapas precedentes la formalización del modelo de dominio, taxonomía de etapas, soporte de versiones Windows y validador topológico con defensas contra dispositivos reservados DOS (`task0053`, commit `14d0db3`), implementado el servicio operativo `vigilio::CiWindowsJobService` en C++20 con orquestación secuencial, modo simulación dry-run determinista, política fail-fast, tolerancia selectiva a errores, cancelación cooperativa multihilo bajo la política Zero-Zombie, saneamiento seguro de directorios con defensas Windows (dispositivos DOS, prefijos UNC, normalización de separadores y reintentos con retroceso exponencial ante contención de archivos) y síntesis CLI reproducible (`task0054`, commit `9c609a6`), y formalizado el arnés de pruebas de interfaz de usuario con verificación de cobertura de compilación y auditoría de textos públicos en `WindowTests.cpp` (`task0055`, commit `fb4f9f4`), el objetivo esencial de este pack es **diseñar y estructurar la suite exhaustiva, determinista y de alta resiliencia de pruebas automatizadas para CI Windows job**.

La suite de pruebas proyectada cubre sistemáticamente:
- **Casos positivos y ciclo de vida integral del pipeline de CI Windows**:
  - Verificación exhaustiva de planes canónicos de depuración y optimización (`canonicalDebugPlan`, `canonicalReleasePlan`), evaluando precondiciones, postcondiciones, tiempos límites (`totalTimeoutSeconds`), cuotas de memoria RAM (`maxMemoryBytes` = 4 GB), y la orquestación ordenada de las etapas canónicas (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`).
  - Ejecución en modo simulación y dry-run de planes personalizados y heterogéneos: subconjuntos de etapas (e.g. únicamente verificación estática y formato, o únicamente compilación y pruebas), comprobando que los archivos en disco permanecen estrictamente inmutables durante el modo `DryRun`.
  - Síntesis CLI determinista de comandos Windows complejos: validación de invocaciones (`cmake --preset ci-windows-debug`, `ctest --preset ci-windows-debug --output-on-failure`, banderas de sanitizers Clang-cl/MSVC `-fsanitize=address`, opciones con espacios o caracteres especiales, rutas de ejecutables de compilador con espacios tipo `C:\Program Files\Microsoft Visual Studio\2022`).
  - Generación y consistencia de resúmenes estructurados de etapas (`formatStepSummary`) con tiempos de ejecución, estados por etapa, conteo de etapas exitosas/fallidas y tiempos agregados.
- **Casos negativos, validación de fronteras y defensas de seguridad Windows**:
  - Detección y rechazo de planes vacíos (`steps.empty()`), planes con tiempos límites no positivos ($\le 0$ o $> 3600\text{ s}$), o con cuotas de memoria RAM anómalas ($0$ bytes o $> 8\text{ GB}$) mediante `CiWindowsJobValidator::validatePlan`.
  - Detección y rechazo de violaciones de orden topológico y precedencia: etapas desordenadas (e.g. `BuildCompilation` antes de `ConfigurePreset`, `TestSuiteExecution` antes de `BuildCompilation`, o `ArtifactPackaging` antes de pruebas) mediante `CiWindowsJobValidator::validateStepSequence`, emitiendo el código tipado `CiWindowsJobError::DependencyOrderingViolation` o `InvalidWorkflowSchema`.
  - Detección de etapas obligatorias ausentes en planes de release (e.g. omisión de `BuildCompilation` o `TestSuiteExecution`).
  - Detección de comandos vacíos, nombres vacíos o descripciones truncadas en definiciones de etapa.
  - Detección y rechazo riguroso de ataques de Directory Traversal y vectores específicos de Windows en `buildDir`: rutas relativas maliciosas (`../`, `../../`, `..\\`, `build/../../etc`, `build\\..\\..\\windows\\system32`), rutas absolutas peligrosas fuera del alcance seguro (`/`, `C:\`, `D:/path`, `\\server\share`, `//server/share`), rutas con secuencias de escape o caracteres nulos embebidos (`\0`), y dispositivos reservados del sistema DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`, con o sin extensión como `aux.json`, `con.txt`), certificando que `isSafeBuildPath` retorna `false` y `cleanBuildDirectory` previene operaciones destructivas fuera de la raíz del proyecto.
  - Detección de entornos incompatibles en `validateEnvironment`: compiladores no soportados (MSVC < 19.38, Clang-cl < 18, MinGW < 13), SDKs de Windows obsoletos o malformados (< 10.0.19041), herramientas esenciales faltantes (CMake < 3.28, Ninja < 1.12), arquitecturas no admitidas (`Unknown`), y versiones de Windows no soportadas (Windows 7, 8, XP).
- **Concurrencia masiva multihilo (16 hilos) y sincronización**:
  - Ejecución paralela sobre `CiWindowsJobService` desde 16 hilos (`std::thread`) concurrentes:
    - 16 hilos ejecutando simultáneamente `executePlan` en modo `DryRun` con planes idénticos y divergentes sobre una instancia compartida.
    - 16 hilos ejecutando granularmente `executeStep` sobre distintas etapas del pipeline.
    - Acceso simultáneo a métodos de lectura (`plan()`, `options()`) y modificación protegida (`setPlan()`, `setOptions()`).
    - Certificación de ausencia absoluta de carreras de datos (data races), integridad del estado interno protegido por `std::mutex`, y terminación limpia de todos los hilos trabajadores sin interbloqueos (deadlocks).
- **Cancelación cooperativa asíncrona multihilo y política Zero-Zombie**:
  - Pruebas de estrés de cancelación: interrupción asíncrona inmediata mediante `CancellationToken` disparado desde hilos independientes en distintos instantes de la ejecución del pipeline (durante la primera etapa, etapa intermedia o penúltima etapa).
  - Verificación de que `CiWindowsJobService::executePlan` aborta de inmediato las etapas subsiguientes al detectar el token cancelado, establece el estado consolidado en `CiWindowsJobStatus::Cancelled`, asigna el error tipado correspondiente, calcula correctamente las etapas ejecutadas vs omitidas (`CiWindowsStepStatus::Omitted`), y emite la pista de remediación en lenguaje natural.
  - Garantía de ausencia total de procesos zombis: ningún subproceso de Windows colgado (`cmd.exe`, `powershell.exe`, `ninja.exe`), liberación inmediata de descriptores de archivo y saneamiento de copias temporales.
- **Recuperación ante fallos de disco, permisos y contención transitoria de archivos (File Locks)**:
  - Simulación de fallos de escritura por permisos denegados en directorios de construcción (`std::filesystem::perms::owner_read`), certificando el manejo limpio en `cleanBuildDirectory` sin abortos ni excepciones no capturadas.
  - Simulación de contención transitoria y bloqueos temporales de archivos típicos de Windows (indexador, antivirus) y verificación de reintentos con retroceso exponencial (`retryBackoffMs`): prueba de etapa con bloqueo transitorio simulado donde la etapa se recupera tras N reintentos, validando que el servicio alcanza el estado exitoso sin excepciones no controladas.
  - Evaluación comparativa determinista entre la política de corte rápido (`failFast == true`, donde el primer fallo aborta las etapas posteriores marcándolas como `Omitted`) y la política de continuación (`failFast == false` / `continueOnError == true`, donde se evalúan todas las etapas posibles para recabar diagnósticos completos).
- **Desacoplamiento arquitectónico y tolerancia headless**:
  - Certificación de ejecución 100% autónoma en CTest (< 500 ms en total) sin requerir `QApplication`, `DISPLAY`, `WAYLAND_DISPLAY` ni procesos WebEngine/Chromium.
  - Aislamiento absoluto y política Local-First constitucional (ADR-001 / Decisión Q-001): cero sockets de red externa, cero peticiones cloud y total privacidad de datos locales.
- **Auditoría exhaustiva de textos públicos (UX Copy)**:
  - Verificación programática de que todas las cadenas de diagnóstico, resumen y remediación generadas por `CiWindowsJobValidator::jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `generateRemediationHint`, `CiWindowsJobService::executionModeToString` y `formatStepSummary` emplean español neutro claro y accesible, libres de términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «CI Windows job»), conforme a `docs/UX-COPY-PUBLICO.md`.

---

## Out of scope

- Modificación de la interfaz gráfica de usuario de producción (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de controles visuales, botones de compilación o menús de ejecución de pipelines en la ventana de navegación (proscrito taxativamente por `docs/UX-COPY-PUBLICO.md` y por la responsabilidad del módulo `foundations`).
- Modificación de los contratos de dominio consolidados en `CiWindowsJobTypes.h`, `CiWindowsJobValidator.h`, `CiWindowsJobConfiguration.h` o `CiWindowsJobService.h` durante PROPOSE (la propuesta define pruebas sobre las APIs estables existentes).
- Invocación de procesos pesados de Qt WebEngine o Chromium durante la ejecución de las suites de pruebas unitarias o de integración de CI Windows.
- Conexión a redes externas, telemetría o servicios cloud de CI (GitHub Actions, Azure Pipelines, AppVeyor, servidores remotos) (estrictamente vetado por ADR-001 / Decisión Q-001).
- Modificación de código fuente fuera de las rutas autorizadas para esta etapa.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Dominio y servicio operativo desacoplado (`apps/vigilio-browser/src/core/`)**:
   - `CiWindowsJobTypes.h`: Define enumeraciones fuertemente tipadas `CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`. Incluye las estructuras inmutables `CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport` y el token ligero de cancelación cooperativa multihilo `CancellationToken`.
   - `CiWindowsJobValidator.h` / `.cpp`: Proporciona validación estática de planes (`validatePlan`), verificación de secuencias topológicas (`validateStepSequence`), validación de requisitos de entorno C++20 (`validateEnvironment` para MSVC 19.38+, Clang-cl 18+, MinGW-w64 13+ y Windows SDK 10.0.19041+), comprobación de rutas seguras frente a Directory Traversal y dispositivos reservados DOS (`isSafeBuildPath`) y traducción de estados, errores, etapas y arquitecturas a texto descriptivo en español neutro (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`).
   - `CiWindowsJobConfiguration.h` / `.cpp`: Genera los planes canónicos del proyecto (`canonicalDebugPlan`, `canonicalReleasePlan`), verifica invariantes del sistema (`verifyInvariants`) y valida versiones Windows soportadas (Windows 11 23H2/22H2, Windows 10 22H2, Windows Server 2022 vía `isSupportedWindowsVersion`).
   - `CiWindowsJobService.h` / `.cpp`: Proporciona la capa operativa completa con orquestación de plan (`executePlan`), ejecución granular de etapas (`executeStep`), simulación determinista en modo `DryRun`, saneamiento seguro de directorios (`cleanBuildDirectory` con reintentos exponenciales ante contención de archivos), síntesis CLI reproducible (`buildCommandLine`) y diagnósticos públicos (`executionModeToString`, `formatStepSummary`).
2. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/CiWindowsJobConfigurationTests.cpp`: 12 pruebas unitarias evaluando plan canónico debug, plan canónico release, validación de plan, pasos vacíos, orden topológico de etapas, validación de entorno, rutas seguras y directory traversal, dispositivos DOS reservados, versiones soportadas, opciones y resultados de etapa, UX copy público y pistas de remediación.
   - `tests/unit/CiWindowsJobServiceTests.cpp`: 12 pruebas unitarias evaluando inicialización, concurrencia y sincronización básica (8 hilos), dry-run pipeline, fail-fast, continue on error, cancelación, limpieza segura de directorio de build, traversal en limpieza, ejecución individual de etapa, síntesis CLI, límites de recursos y memoria, y cumplimiento de UX Copy.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario que integra `CiWindowsJobService` en modo dry-run para certificar la cobertura de compilación de UI y la ausencia de jerga interna en los diagnósticos y pistas de remediación.
3. **Integración en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Los ejecutables de pruebas `vigilio_ci_windows_job_tests` y `vigilio_ci_windows_job_service_tests` están plenamente integrados en el sistema de construcción de CTest, vinculando `vigilio_core` y `vigilio_compiler_warnings`.
   - Ambos ejecutables están catalogados bajo las propiedades `LABELS "unit;fast"` con un tiempo límite de ejecución de 15 segundos.
4. **Verificación continua existente (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - 23 de 23 pruebas pasando exitosamente en CTest con 100% de éxito en ~4.52 segundos:
     - `core_units`: Passed (0.01s)
     - `storage_integration`: Passed (0.01s)
     - `vigilio_warnings_tests`: Passed (0.16s)
     - `qt_dependency_validation_tests`: Passed (0.01s)
     - `webengine_version_validator_tests`: Passed (0.23s)
     - `resource_qrc_catalog_tests`: Passed (0.09s)
     - `resource_inclusion_service_tests`: Passed (0.12s)
     - `resource_qrc_inclusion_tests`: Passed (0.27s)
     - `startup_bootstrap_tests`: Passed (0.20s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.05s)
     - `cmake_presets_tests`: Passed (0.04s)
     - `cmake_presets_service_tests`: Passed (0.10s)
     - `test_fixture_foundation_tests`: Passed (0.01s)
     - `test_fixture_service_tests`: Passed (0.11s)
     - `clang_format_configuration_tests`: Passed (0.00s)
     - `clang_format_service_tests`: Passed (1.97s)
     - `clang_tidy_configuration_tests`: Passed (0.01s)
     - `clang_tidy_service_tests`: Passed (0.12s)
     - `ci_linux_job_configuration_tests`: Passed (0.01s)
     - `ci_linux_job_service_tests`: Passed (0.14s)
     - `ci_windows_job_configuration_tests`: Passed (0.01s)
     - `ci_windows_job_service_tests`: Passed (0.67s)
5. **Gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 56 referencias canónicas validadas con estado `valid: true` (exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0166-module.foundations.task0056.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).

---

## Alcance exacto

1. **Diseño de pruebas exhaustivas en `CiWindowsJobConfigurationTests.cpp`**:
   - `test_ci_windows_plan_validation_boundaries`: límites extremos de planes (timeouts $\le 0$, $> 3600\text{ s}$, memoria $0$ o $> 8\text{ GB}$, nombres de preset vacíos, comandos truncados).
   - `test_ci_windows_step_sequence_topological_violations`: violaciones complejas de orden topológico (compilación antes de preset, pruebas antes de compilación, empaquetado antes de pruebas, pasos duplicados).
   - `test_ci_windows_advanced_path_traversal`: vectores maliciosos complejos específicos de Windows (`build\\..\\..\\windows`, `..\\..\\system32`, prefijos UNC `//`, `\\\\`, caracteres nulos `\0`, nombres reservados DOS compuestos como `aux.json`, `con.txt`).
   - `test_ci_windows_environment_boundaries`: validación en límites de versiones Windows, compiladores obsoletos (MSVC < 19.38, Clang-cl < 18, MinGW < 13), Windows SDK inválido o < 10.0.19041, y CMake/Ninja obsoletos.
2. **Diseño de pruebas exhaustivas en `CiWindowsJobServiceTests.cpp`**:
   - `test_ci_windows_service_massive_multithreading_concurrency`: estrés multihilo con 16 workers simultáneos ejecutando planes en modo `DryRun` y pasos sobre una única instancia compartida sin carreras de datos.
   - `test_ci_windows_service_cooperative_cancellation_stress`: estrés de interrupción asíncrona en tiempo real con `CancellationToken` bajo la política Zero-Zombie.
   - `test_ci_windows_service_permission_failure_injection`: inyección de fallos de permisos en directorios de compilación (`perms::owner_read`) y verificación de reporte de error limpio sin excepciones no capturadas.
   - `test_ci_windows_service_retry_backoff_transient`: prueba de recuperación con retroceso exponencial ante contención transitoria simulada de archivos.
   - `test_ci_windows_service_fail_fast_vs_continue_on_error`: evaluación comparativa determinista entre la política de corte inmediato (`failFast`) y la de continuación (`continueOnError`).
   - `test_ci_windows_service_cli_synthesis_advanced`: síntesis determinista de CLI de Windows con rutas complejas (rutas con espacios, prefijos de compilador y banderas avanzadas).
3. **Auditoría continua de UX Copy**:
   - Verificación estricta de que todas las cadenas públicas de estado, error y remediación cumplan con `docs/UX-COPY-PUBLICO.md` y no expongan jerga interna de ingeniería.

---

## Diseño completo de la capacidad

El diseño de la capacidad **CI Windows job** en su etapa **tests** se estructura en torno a 4 pilares:

```
+-----------------------------------------------------------------------------------+
|                           Arquitectura de Pruebas CI Windows                      |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [ CiWindowsJobConfigurationTests ]                 [ CiWindowsJobServiceTests ]  |
|  - Casos de frontera en planes (timeout/RAM)        - Concurrencia masiva (16 T)  |
|  - Violaciones topológicas complejas                - Cancelación Zero-Zombie     |
|  - Traversal Windows y dispositivos DOS             - Inyección fallo permisos    |
|  - Límites de entorno C++20 y Windows SDK           - Reintentos con backoff      |
|                                                     - failFast vs continueOnError |
|                                                     - Síntesis CLI avanzada       |
|                                                                                   |
|                               [ Auditoría UX Copy ]                               |
|                     - Español neutro sin jerga técnica interna                    |
|                     - Cumplimiento estricto docs/UX-COPY-PUBLICO.md              |
|                                                                                   |
+-----------------------------------------------------------------------------------+
```

### 1. Pruebas de frontera y defensas contra vectores Windows (`CiWindowsJobConfigurationTests`)
- Evaluación de límites exactos:
  - `totalTimeoutSeconds == 0` y `totalTimeoutSeconds > 3600` retornan `CiWindowsJobError::InvalidWorkflowSchema`.
  - `maxMemoryBytes == 0` y `maxMemoryBytes > 8589934592ULL` retornan `CiWindowsJobError::MemoryLimitExceeded`.
  - Nombres de preset vacíos o comandos vacíos en etapas retornan `CiWindowsJobError::InvalidWorkflowSchema`.
- Violaciones de orden topológico:
  - `BuildCompilation` sin previo `ConfigurePreset` -> `DependencyOrderingViolation`.
  - `TestSuiteExecution` sin previo `BuildCompilation` -> `DependencyOrderingViolation`.
  - `ArtifactPackaging` sin previo `TestSuiteExecution` -> `DependencyOrderingViolation`.
  - Duplicación de cualquier etapa en el vector de pasos -> `DependencyOrderingViolation`.
- Traversal y dispositivos DOS de Windows:
  - Detección de `build/../../etc`, `..\\..\\windows\\system32`, prefijos UNC `//share/data`, `\\\\server\\share`.
  - Detección de dispositivos reservados DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`) incluso con extensiones (`aux.json`, `nul.txt`, `com1.log`).
  - Detección de caracteres nulos embebidos (`\0`).
- Validación de entorno:
  - MSVC con `compilerMajorVersion < 19` o `(19 && minor < 38)` -> `ToolchainMissing`.
  - Clang-cl con `compilerMajorVersion < 18` -> `ToolchainMissing`.
  - MinGW con `compilerMajorVersion < 13` -> `ToolchainMissing`.
  - Windows SDK vacío, sin prefijo `10.0.` o con build < `19041` -> `WindowsSdkMissing`.
  - Versiones de Windows no catalogadas (e.g. `Windows 7`, `Windows XP`) -> `ToolchainMissing`.

### 2. Pruebas operativas multihilo, resiliencia y recuperación (`CiWindowsJobServiceTests`)
- **Concurrencia masiva multihilo**:
  - Lanzamiento simultáneo de 16 hilos sobre la misma instancia de `CiWindowsJobService`.
  - Hilos pares ejecutan `executePlan` en modo `DryRun` con planes heterogéneos; hilos impares invocan `executeStep` y modifican opciones de servicio.
  - Sincronización determinista mediante `std::atomic<bool>` de arranque para garantizar máxima contención.
  - Verificación de convergencia sin bloqueos ni data races (`std::mutex`).
- **Cancelación cooperativa asíncrona (Zero-Zombie)**:
  - Ejecución de un plan de 7 u 8 etapas en hilo trabajador.
  - Hilo de control cancela el `CancellationToken` tras un breve intervalo (e.g. 5 ms) mientras la ejecución está activa.
  - Comprobación de que el pipeline se interrumpe inmediatamente, el reporte tiene estado `CiWindowsJobStatus::Cancelled`, las etapas posteriores se marcan como `CiWindowsStepStatus::Omitted`, y ningún proceso queda colgado.
- **Inyección de fallos de permisos en disco**:
  - Restricción de permisos sobre un directorio temporal (`std::filesystem::perms::owner_read`).
  - `cleanBuildDirectory` debe retornar limpiamente `false` con captura de error sin abortar ni lanzar excepciones no capturadas.
- **Recuperación ante contención y reintentos con retroceso exponencial**:
  - Simulación de etapa con contención transitoria que requiere múltiples reintentos (`maxRetries = 3`, `retryBackoffMs = 20`).
  - Verificación de que la etapa alcanza el éxito tras los reintentos y que el retroceso exponencial se respeta deterministamente.
- **Comparativa failFast vs continueOnError**:
  - Plan con etapas simuladas donde la etapa 2 falla.
  - Con `failFast = true`: la etapa 2 falla y las etapas 3..N se marcan como `Omitted`.
  - Con `continueOnError = true` y `failFast = false`: las etapas subsiguientes independientes continúan ejecutándose para proveer diagnósticos completos.
- **Síntesis CLI Windows avanzada**:
  - Validación de síntesis determinista de comandos para CMake, Ninja, CTest y Clang-cl con rutas complejas (rutas con espacios, prefijos de unidad `C:\...` y parámetros de sanitizers).

### 3. Auditoría de textos públicos (UX Copy)
- Verificación programática en las pruebas de que el 100% de las funciones de conversión de texto:
  - `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `generateRemediationHint`
  - `executionModeToString`, `formatStepSummary`
  emplean español neutro y carecen de términos técnicos prohibidos por `docs/UX-COPY-PUBLICO.md`.

---

## Archivos afectados

Durante esta fase PROPOSE, la autoría se confina exclusivamente a la especificación OpenSpec:
- `openspec/changes/module-foundations-task0056/proposal.md`: Este documento de propuesta técnica completa.
- `openspec/changes/module-foundations-task0056/design.md`: Diseño detallado, estado actual/objetivo/brechas y compatibilidad.
- `openspec/changes/module-foundations-task0056/tasks.md`: Plan de tareas numeradas y verificables para la fase APPLY.
- `openspec/changes/module-foundations-task0056/execution-memory.md`: Memoria de ejecución, referencias y gobernanza.

Para la subsiguiente fase APPLY (tras aprobación humana), los archivos a modificar son:
- `apps/vigilio-browser/tests/unit/CiWindowsJobConfigurationTests.cpp`: Adición de pruebas de frontera de planes, violaciones topológicas, traversal avanzado y límites de entorno Windows.
- `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`: Adición de pruebas de concurrencia masiva (16 hilos), estrés de cancelación Zero-Zombie, inyección de fallos de permisos, reintentos con retroceso exponencial, comparativa fail-fast vs continue-on-error y síntesis CLI avanzada.
- `docs/rules/01-modules/foundations/micro/0056-ci-windows-job/task.md`: Sincronización del estado de la microtarea canónica.

---

## Contrato UX/UI

- **Declaración contractual de «sin cambio visual directo»**:
  - La capacidad **CI Windows job** es infraestructura de backend y automatización de calidad en entornos Windows; por consiguiente, **no introduce ningún componente visual, botón, menú o ventana en la interfaz gráfica pública de producción (`BrowserWindow`)**.
- **Cumplimiento de `docs/UX-COPY-PUBLICO.md`**:
  - Proscripción estricta de términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «CI Windows job») en cadenas visibles al usuario.
  - Uso exclusivo de **«Interfaz más fácil de usar»** para cualquier referencia de accesibilidad.
  - Todos los mensajes de estado y remediación emitidos por las APIs de CI Windows emplean lenguaje natural en español neutro comprensible para cualquier usuario.

---

## Reglas y fallos

1. **Determinismo y ausencia de flakiness**:
   - Todas las pruebas multihilo y de cancelación utilizan primitivas de sincronización deterministas (`std::atomic`, `std::mutex`, `join`). Prohibidos los temporizadores ciegos (`sleep`) que provoquen intermitencia.
2. **Política Zero-Zombie**:
   - Todo subproceso invocado debe tener timeout finito acotado y cancelarse limpiamente ante la activación de `CancellationToken`. Ningún proceso de compilación debe quedar huérfano.
3. **Manejo de permisos y fallos de E/S**:
   - Las operaciones de limpieza de directorio (`cleanBuildDirectory`) deben capturar cualquier `std::filesystem_error` y retornar `false` con mensaje descriptivo, sin abortos ni excepciones no controladas.
4. **Respeto a la política Local-First**:
   - Prohibido cualquier acceso a sockets de red externa, telemetría o servicios cloud. Toda verificación se realiza localmente.

---

## Seguridad y privacidad

- **Límites de confianza y Path Confinement**:
  - `CiWindowsJobValidator::isSafeBuildPath` garantiza que el directorio de construcción resida estrictamente dentro de la raíz del proyecto.
  - Detección rigurosa de Directory Traversal (`..`, `..\\`), rutas absolutas POSIX (`/`) y Windows (`C:\`, `D:/`), prefijos UNC (`//`, `\\\\`), y dispositivos reservados DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`).
- **Aislamiento de datos**:
  - Las pruebas operan en modo `DryRun` en memoria o sobre directorios temporales aislados (`std::filesystem::temp_directory_path`), que se sanean automáticamente al finalizar.
- **Privacidad Local-First (ADR-001 / Decisión Q-001)**:
  - Cero dependencias de cuentas o servicios remotos. Total soberanía del usuario.

---

## Matriz de pruebas

| Identificador | Componente / Función | Tipo | Alcance y Aserción Clave | Estado |
| --- | --- | --- | --- | --- |
| `TEST-013` | `test_ci_windows_plan_validation_boundaries` | Unitario | Límites extremos de plan (timeout $\le 0$, $> 3600$, memoria $0$ o $> 8\text{ GB}$, nombres vacíos) retornan error tipado. | APPLY |
| `TEST-014` | `test_ci_windows_step_sequence_topological_violations` | Unitario | Violaciones de precedencia (compilación antes de preset, tests antes de build, pasos duplicados) emiten `DependencyOrderingViolation`. | APPLY |
| `TEST-015` | `test_ci_windows_advanced_path_traversal` | Unitario / Seguridad | Rechazo de `build\\..\\..\\windows`, prefijos UNC `//`, `\\\\`, caracteres nulos `\0` y dispositivos DOS compuestos (`aux.json`). | APPLY |
| `TEST-016` | `test_ci_windows_environment_boundaries` | Unitario | Detección de versiones incompatibles de MSVC, Clang-cl, MinGW, Windows SDK < 10.0.19041 y versiones de Windows no soportadas. | APPLY |
| `TEST-CI-WIN-SRV-013` | `test_ci_windows_service_massive_multithreading_concurrency` | Integración / Concurrencia | 16 hilos ejecutando concurrentemente `executePlan` en `DryRun` y `executeStep` sin carreras de datos ni deadlocks. | APPLY |
| `TEST-CI-WIN-SRV-014` | `test_ci_windows_service_cooperative_cancellation_stress` | Integración / Resiliencia | Cancelación asíncrona en tiempo real con `CancellationToken` bajo política Zero-Zombie; estado `Cancelled` y etapas omitidas. | APPLY |
| `TEST-CI-WIN-SRV-015` | `test_ci_windows_service_permission_failure_injection` | Integración / Recuperación | Fallo simulado de permisos en disco (`perms::owner_read`) manejado limpiamente en `cleanBuildDirectory` sin excepciones. | APPLY |
| `TEST-CI-WIN-SRV-016` | `test_ci_windows_service_retry_backoff_transient` | Integración / Resiliencia | Etapa con contención transitoria simulada se recupera tras N reintentos con retroceso exponencial (`retryBackoffMs`). | APPLY |
| `TEST-CI-WIN-SRV-017` | `test_ci_windows_service_fail_fast_vs_continue_on_error` | Unitario / Lógica | Comparativa directa entre corte inmediato (`failFast`) y continuación de etapas independientes (`continueOnError`). | APPLY |
| `TEST-CI-WIN-SRV-018` | `test_ci_windows_service_cli_synthesis_advanced` | Unitario | Síntesis determinista de comandos Windows complejos con espacios, prefijos de herramientas y banderas de compilación. | APPLY |
| `TEST-CI-WIN-AUDIT` | Auditoría de UX Copy público | Unitario / Calidad | 100% de cadenas de diagnóstico y remediación libres de jerga técnica interna conforme a `docs/UX-COPY-PUBLICO.md`. | APPLY |

---

## Pasos APPLY

1. **Paso 1: Implementación de pruebas en `CiWindowsJobConfigurationTests.cpp`**:
   - Incorporar `test_ci_windows_plan_validation_boundaries` (`TEST-013`).
   - Incorporar `test_ci_windows_step_sequence_topological_violations` (`TEST-014`).
   - Incorporar `test_ci_windows_advanced_path_traversal` (`TEST-015`).
   - Incorporar `test_ci_windows_environment_boundaries` (`TEST-016`).
   - Actualizar la función `main()` del arnés para invocar los nuevos tests.
2. **Paso 2: Implementación de pruebas en `CiWindowsJobServiceTests.cpp`**:
   - Incorporar `test_ci_windows_service_massive_multithreading_concurrency` (`TEST-CI-WIN-SRV-013`).
   - Incorporar `test_ci_windows_service_cooperative_cancellation_stress` (`TEST-CI-WIN-SRV-014`).
   - Incorporar `test_ci_windows_service_permission_failure_injection` (`TEST-CI-WIN-SRV-015`).
   - Incorporar `test_ci_windows_service_retry_backoff_transient` (`TEST-CI-WIN-SRV-016`).
   - Incorporar `test_ci_windows_service_fail_fast_vs_continue_on_error` (`TEST-CI-WIN-SRV-017`).
   - Incorporar `test_ci_windows_service_cli_synthesis_advanced` (`TEST-CI-WIN-SRV-018`).
   - Actualizar la función `main()` del arnés para invocar los nuevos tests.
3. **Paso 3: Verificación de calidad y compilación limpia**:
   - Ejecutar `apps/vigilio-browser/scripts/check.sh` asegurando compilación sin advertencias y 100% de pruebas aprobadas en CTest.
   - Ejecutar formateo Clang-Format sobre los archivos de prueba modificados.
4. **Paso 4: Verificación de gobernanza y sincronización de microtarea**:
   - Ejecutar `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`.
   - Sincronizar `docs/rules/01-modules/foundations/micro/0056-ci-windows-job/task.md` documentando el estado de la capacidad.

---

## Definición de terminado

- [x] Propuesta técnica OpenSpec completa y exhaustiva redactada en `openspec/changes/module-foundations-task0056/`.
- [x] Contratos funcionales, de seguridad, UX copy y arquitectura formalizados.
- [x] Matriz de pruebas trazable definida con casos positivos, negativos, de concurrencia y recuperación.
- [x] Tareas de ejecución numeradas y reproducibles en `tasks.md`.
- [x] Memoria de ejecución alineada con autoridades canónicas y metadatos de runner en `execution-memory.md`.
- [x] Verificación de gobernanza con Prompt Builder CLI (`validate-references` y `verify-lock`) aprobada.
- [x] Cero modificaciones de código de aplicación durante la fase PROPOSE.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0055` (Completada exitosamente, commit `fb4f9f4`).
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Flakiness en pruebas multihilo debido a planificación del SO.
    *Mitigación*: Uso exclusivo de sincronización determinista (`std::atomic`, `std::mutex`, `join`), sin sleeps fijos ni condiciones de carrera.
  - *Riesgo 2*: Falsos positivos de permisos en entornos de ejecución con privilegios de root (`geteuid() == 0`).
    *Mitigación*: Detección de privilegios para seleccionar rutas protegidas del sistema o inyección controlada en `cleanBuildDirectory`.
  - *Riesgo 3*: Incremento en el tiempo de ejecución de la suite de pruebas.
    *Mitigación*: Las pruebas operativas de concurrencia se ejecutan en modo `DryRun` en memoria, garantizando ejecución en < 500 ms.

---

## Commit propuesto

- **Mensaje base**: `test(foundations): tests ci-windows-job`
- **Alcance del commit**: Exclusivamente las suites de pruebas automatizadas y la documentación canónica de la microtarea en la subsiguiente fase APPLY.
