# Design — module-foundations-task0056

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Modelo de dominio y capa operativa de servicio de CI Windows completados (`task0053`, `task0054` y `task0055`)**:
   - `vigilio_core` cuenta con `src/core/CiWindowsJobTypes.h`, que establece enumeraciones fuertemente tipadas (`CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`), estructuras inmutables de configuración y ejecución (`CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport`) y el token cooperativo de cancelación multihilo `CancellationToken`.
   - `src/core/CiWindowsJobValidator.h/.cpp` implementa la validación estática de planes (`validatePlan`), verificación de secuencias topológicas (`validateStepSequence`), validación de requisitos de entorno C++20 (`validateEnvironment` para MSVC 19.38+, Clang-cl 18+, MinGW-w64 13+ y Windows SDK 10.0.19041+), comprobación de rutas seguras frente a Directory Traversal, prefijos UNC y dispositivos reservados DOS (`isSafeBuildPath`) y traducción de estados, errores, etapas y arquitecturas a texto descriptivo en español neutro (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`).
   - `src/core/CiWindowsJobConfiguration.h/.cpp` genera los planes canónicos del proyecto (`canonicalDebugPlan`, `canonicalReleasePlan`), verifica invariantes del sistema (`verifyInvariants`) y valida versiones Windows soportadas (Windows 11 23H2/22H2, Windows 10 22H2, Windows Server 2022).
   - `src/core/CiWindowsJobService.h/.cpp` proporciona la capa operativa desacoplada con orquestación secuencial de pipelines en modo simulación dry-run determinista (`executePlan`), ejecución granular de etapas (`executeStep`), corte temprano ante fallos (`failFast`), tolerancia selectiva a fallos (`continueOnError`), cancelación cooperativa multihilo Zero-Zombie, saneamiento seguro de directorios (`cleanBuildDirectory` con reintentos exponenciales ante contención de archivos), síntesis CLI reproducible (`buildCommandLine`) y diagnósticos públicos (`executionModeToString`, `formatStepSummary`).
2. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/CiWindowsJobConfigurationTests.cpp`: 12 pruebas unitarias enfocadas en plan canónico debug, plan canónico release, validación de plan, pasos vacíos, orden topológico de etapas, validación de entorno, rutas seguras y directory traversal, dispositivos DOS reservados, versiones soportadas, opciones y resultados de etapa, UX copy público y pistas de remediación.
   - `tests/unit/CiWindowsJobServiceTests.cpp`: 12 pruebas unitarias enfocadas en inicialización, concurrencia básica de configuración (8 hilos), dry-run pipeline, fail-fast, continue on error, cancelación básica, limpieza segura de directorio de build, traversal en limpieza, ejecución individual de etapa, síntesis CLI, límites de recursos y memoria, y cumplimiento de UX Copy.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario con verificación programática dry-run de cobertura de compilación de UI en el pipeline de CI Windows y auditoría de textos públicos de remediación.
3. **Estado de compilación y pruebas continuas**:
   - Compilación limpia bajo C++20 con `-Werror` y cero advertencias.
   - 23 de 23 pruebas en CTest pasando con éxito en ~4.52 segundos.
4. **Brecha técnica identificada para la etapa `tests` (GAP)**:
   - Las pruebas existentes cubren los flujos nominales y casos negativos estándar de forma secuencial y en escenarios controlados, pero carecen de:
     - Pruebas sistemáticas de estrés bajo concurrencia masiva multihilo (16 workers simultáneos ejecutando planes y pasos sobre una única instancia compartida de `CiWindowsJobService`).
     - Pruebas de estrés de cancelación cooperativa asíncrona multihilo en tiempo real bajo la política Zero-Zombie, donde la cancelación se dispara concurrentemente desde un hilo independiente mientras el pipeline está en ejecución.
     - Inyección profunda de fallos de permisos de archivo en disco sobre el directorio de compilación (`perms::owner_read`) y verificación de robustez sin excepciones no capturadas.
     - Verificación exhaustiva de fronteras en configuración de planes (timeouts extremos $\le 0$ o $> 3600$, cuotas de memoria anómalas $0$ o $> 8\text{ GB}$) y vectores complejos de Directory Traversal y dispositivos DOS (`build\\..\\..\\windows`, `..\\..\\system32`, prefijos UNC `//`, `\\\\`, inyección de caracteres nulos `\0`, nombres reservados DOS compuestos como `aux.json`, `con.txt`).
     - Evaluación de límites exactos de herramientas Windows (MSVC < 19.38, Clang-cl < 18, MinGW < 13, Windows SDK malformado o build < 19041).
     - Comparativa determinista de las políticas de corte (`failFast` vs `continueOnError`) ante etapas que fallan en secuencia.
     - Síntesis CLI avanzada con rutas que contienen espacios y caracteres especiales de Windows.

---

### Estado objetivo (TARGET)

1. **Suite exhaustiva de pruebas automatizadas y deterministas para CI Windows Job**:
   - **Pruebas de frontera de configuración, Directory Traversal y dispositivos DOS (`CiWindowsJobConfigurationTests.cpp`)**:
     - Validación en límites de planes: timeouts iguales a cero o valores mayores a 3600 segundos, cuotas extremas de memoria RAM ($0$ bytes, $> 8$ GB), nombres de preset vacíos y comandos truncados.
     - Detección y rechazo de secuencias desordenadas complejas: etapas que rompen las dependencias topológicas (e.g. `TestSuiteExecution` sin previa compilación, `ArtifactPackaging` sin tests previos, pasos duplicados).
     - Detección y rechazo riguroso de ataques complejos de Directory Traversal específicos de Windows (`build\\..\\..\\windows`, `..\\..\\system32`, prefijos UNC `//share/data`, `\\\\server\\share`, inyección de caracteres nulos embebidos `\0`, dispositivos DOS con extensiones arbitrarias como `aux.json`, `con.txt`).
     - Verificación de límites de versiones de compiladores y herramientas C++20 en `validateEnvironment` (MSVC, Clang-cl, MinGW-w64, Windows SDK, CMake, Ninja, y versiones de SO).
   - **Pruebas de concurrencia masiva y estrés multihilo (`CiWindowsJobServiceTests.cpp`)**:
     - 16 hilos (`std::thread`) concurrentes ejecutando simultáneamente sobre una única instancia compartida de `CiWindowsJobService`: 8 hilos ejecutando `executePlan` en modo `DryRun` con planes heterogéneos y 8 hilos ejecutando `executeStep` sobre distintas etapas del pipeline.
     - Certificación de ausencia total de data races, contención segura mediante `std::mutex` interno y convergencia determinista de todos los workers sin deadlocks.
   - **Pruebas de cancelación cooperativa multihilo y política Zero-Zombie (`CiWindowsJobServiceTests.cpp`)**:
     - Disparo asíncrono de `CancellationToken` desde un hilo secundario durante la ejecución secuencial del plan de 7 u 8 etapas, comprobando interrupción inmediata en la etapa activa, transición a `CiWindowsJobStatus::Cancelled`, marcado de etapas posteriores como `CiWindowsStepStatus::Omitted`, y ausencia de subprocesos colgados.
   - **Pruebas de inyección de fallos de permisos y recuperación (`CiWindowsJobServiceTests.cpp`)**:
     - Simulación de directorios con permisos restringidos de sólo lectura (`std::filesystem::perms::owner_read`), certificando que `cleanBuildDirectory` reporta limpiamente `false` con captura de error sin abortos ni excepciones no capturadas.
   - **Pruebas de reintentos con retroceso exponencial ante contención transitoria (`CiWindowsJobServiceTests.cpp`)**:
     - Verificación del comportamiento de retroceso exponencial (`maxRetries`, `retryBackoffMs`) ante fallos transitorios simulados de acceso a archivos.
   - **Evaluación comparativa determinista de políticas de corte (`CiWindowsJobServiceTests.cpp`)**:
     - Evaluación directa y contrastada de `failFast == true` (corte inmediato en el primer fallo con etapas posteriores marcadas como `Omitted`) vs `failFast == false` / `continueOnError == true` (ejecución continuada de etapas no dependientes para diagnóstico completo).
   - **Síntesis CLI avanzada para Windows (`CiWindowsJobServiceTests.cpp`)**:
     - Verificación de síntesis CLI con rutas complejas que contienen espacios, parámetros relativos y absolutos con diagonales invertidas, y banderas de compilación y sanitizers.
2. **Rendimiento, portabilidad y desacoplamiento**:
   - Tiempo de ejecución total de la suite < 500 ms en CTest.
   - Cero requerimientos de servidor gráfico o inicialización de `QApplication` para las pruebas del núcleo.
   - Cumplimiento riguroso de la política Local-First (ADR-001 / Decisión Q-001).
3. **Auditoría de UX Copy**:
   - 100% de los mensajes de diagnóstico, estados, errores y remediación auditados contra `docs/UX-COPY-PUBLICO.md`, confirmando ausencia absoluta de términos de ingeniería interna.

---

### Brechas identificadas a resolver en APPLY (GAPS)

| Componente bajo prueba | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| Fronteras de configuración de planes | Pruebas de planes canónicos válidos y paso vacío único | Cobertura sistemática de timeouts límites ($\le 0$, $> 3600$), memoria anómala ($0$, $> 8$ GB) y nombres vacíos | Implementar `test_ci_windows_plan_validation_boundaries` en `CiWindowsJobConfigurationTests.cpp`. |
| Secuencias topológicas complejas | Prueba básica de orden `ConfigurePreset` -> `BuildCompilation` | Evaluación de secuencias arbitrariamente desordenadas, pasos duplicados y saltos de dependencia | Implementar `test_ci_windows_step_sequence_topological_violations` en `CiWindowsJobConfigurationTests.cpp`. |
| Directory Traversal y dispositivos DOS avanzado | Prueba básica de `../../etc` y nombres DOS simples (`CON`) | Prueba de vectores avanzados (`build\\..\\..\\windows`, `..\\..\\system32`, UNC `//`, `\\\\`, caracteres nulos `\0`, nombres DOS compuestos `aux.json`) | Implementar `test_ci_windows_advanced_path_traversal` en `CiWindowsJobConfigurationTests.cpp`. |
| Límites de entorno Windows y toolchains C++20 | Prueba básica de MSVC 19.38 y Windows 11 | Evaluación de límites exactos (MSVC < 19.38, Clang-cl < 18, MinGW < 13, Windows SDK < 10.0.19041, SO no soportado) | Implementar `test_ci_windows_environment_boundaries` en `CiWindowsJobConfigurationTests.cpp`. |
| Concurrencia masiva multihilo | Prueba básica con 8 hilos en `CiWindowsJobServiceTests.cpp` | Prueba con 16 hilos simultáneos operando sobre ejecución de planes y etapas | Implementar `test_ci_windows_service_massive_multithreading_concurrency` en `CiWindowsJobServiceTests.cpp`. |
| Estrés de cancelación cooperativa | Cancelación pre-activada en un solo hilo | Cancelación disparada concurrentemente desde hilo separado durante ejecución secuencial (Zero-Zombie) | Implementar `test_ci_windows_service_cooperative_cancellation_stress` en `CiWindowsJobServiceTests.cpp`. |
| Inyección de fallos de permisos | Solo se prueba traversal en limpieza | Fallo real de I/O forzando permisos de solo lectura (`perms::owner_read`) | Implementar `test_ci_windows_service_permission_failure_injection` en `CiWindowsJobServiceTests.cpp`. |
| Reintentos con retroceso exponencial | Prueba básica de retry en servicio | Verificación de retroceso exponencial ante contención transitoria simulada de archivos (file locks) | Implementar `test_ci_windows_service_retry_backoff_transient` en `CiWindowsJobServiceTests.cpp`. |
| Comparativa de políticas de corte | Pruebas separadas de fail-fast y continue-on-error | Prueba comparativa determinista directa sobre el mismo plan con fallos inyectados | Implementar `test_ci_windows_service_fail_fast_vs_continue_on_error` en `CiWindowsJobServiceTests.cpp`. |
| Síntesis CLI con rutas complejas | Comandos con nombres de preset simples | Síntesis con rutas que contienen espacios (e.g. `Program Files`), subdirectorios anidados y banderas avanzadas | Implementar `test_ci_windows_service_cli_synthesis_advanced` en `CiWindowsJobServiceTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad C++20 y Qt6
- **Estándar C++20**: Uso estricto de `std::filesystem`, `std::string_view`, `std::thread`, `std::atomic`, `enum class`, const-correctness y tipos estándar.
- **Independencia del núcleo**: `vigilio_core` permanece 100% independiente de Qt GUI, Widgets y WebEngine. Las pruebas unitarias se ejecutan directamente sobre la biblioteca del núcleo sin requerir enlaces a Qt Widgets o Chromium.
- **Portabilidad de rutas y defensas Windows**: Manipulación mediante `std::filesystem::path` con normalización bidireccional (`/` y `\`), garantizando que las pruebas se ejecuten y validen idénticamente en Linux, macOS y Windows nativo.

### Estrategia de migración
- No existen cambios destructivos en las APIs existentes ni modificaciones en las firmas de `CiWindowsJobService`, `CiWindowsJobValidator` o `CiWindowsJobConfiguration`.
- Las nuevas pruebas amplían la cobertura de regresión y resiliencia en los archivos de prueba existentes (`CiWindowsJobConfigurationTests.cpp` y `CiWindowsJobServiceTests.cpp`), manteniéndose plenamente compatibles con la infraestructura de CMake y CTest.

### Procedimiento de reversión (Rollback)
- Si alguna prueba introduce inestabilidad, fallo de compilación o flakiness en entornos con limitaciones de recursos:
  1. Identificar la función de prueba afectada mediante el reporte detallado de CTest (`ctest --output-on-failure`).
  2. Revertir los cambios en los archivos de prueba afectados a su estado consolidado anterior mediante `git checkout HEAD -- apps/vigilio-browser/tests/unit/`.
  3. Revalidar compilación y suites completas con `apps/vigilio-browser/scripts/check.sh`.

---

## Arquitectura técnica de pruebas y matriz de resiliencia

### 1. Desacoplamiento de componentes bajo prueba

La arquitectura de pruebas aísla con precisión las responsabilidades:
- **`CiWindowsJobValidator`**: Componente funcional puro y estático. No realiza operaciones destructivas en disco. Valida esquemas, memoria, orden topológico, compatibilidad de toolchain y contención de rutas.
- **`CiWindowsJobConfiguration`**: Factoría inmutable de planes canónicos (`canonicalDebugPlan`, `canonicalReleasePlan`), verificador de invariantes y catálogo de versiones de Windows soportadas.
- **`CiWindowsJobService`**: Orquestador con estado interno thread-safe gobernado por `std::mutex`. Soporta modos `DryRun`, `Simulated` y `Live`, cancelación cooperativa multihilo con `CancellationToken` y políticas de tolerancia a fallos.

### 2. Defensas de seguridad de rutas en Windows

Las pruebas de seguridad validan exhaustivamente los vectores de ataque en sistemas Windows:
- **Directory Traversal**: Detección de cualquier ocurrencia de `..`, secuencias mixtas como `..\\..\\windows` o `build/../../etc`.
- **Rutas absolutas**: Detección de separadores iniciales (`/`, `\\`) y letras de unidad (`C:`, `D:/`).
- **Prefijos UNC y NT**: Detección de `//` y `\\\\`.
- **Dispositivos reservados DOS**: Validación no sensible a mayúsculas/minúsculas de nombres reservados del sistema operativo DOS (`CON`, `PRN`, `AUX`, `NUL`, `COM1`-`COM9`, `LPT1`-`LPT9`), incluyendo variantes con extensiones de archivo arbitrarias (`aux.json`, `con.txt`, `nul.log`).
- **Caracteres nulos embebidos**: Detección y rechazo de cadenas con byte `\0`.

### 3. Modelo de concurrencia masiva (16 hilos)

```
                       +-----------------------------+
                       |   CiWindowsJobService       |
                       |   - plan_ (mutex)           |
                       |   - options_ (mutex)        |
                       +-----------------------------+
                                      ^
                                      |
         +----------------------------+----------------------------+
         |                                                         |
[ 8 Hilos Workers: executePlan ]                          [ 8 Hilos Workers: executeStep ]
- Modo DryRun en memoria                                   - Ejecución granular por etapa
- Planes idénticos y heterogéneos                          - Lectura y ajuste de opciones
- Sin carreras de datos ni deadlocks                       - Verificación de consistencia
```

- Se inicializa una barrera de sincronización mediante `std::atomic<bool> startFlag{false}`.
- 16 hilos se crean y quedan en espera activa (`std::this_thread::yield()`).
- Al liberarse la barrera (`startFlag.store(true)`), los 16 hilos acceden simultáneamente al servicio:
  - 8 hilos invocan `executePlan()` en modo `DryRun`.
  - 8 hilos invocan `executeStep()` sobre diversas etapas del pipeline.
- Se comprueba que todas las operaciones concluyen exitosamente sin excepciones ni corrupción de memoria.

### 4. Cancelación cooperativa multihilo y política Zero-Zombie

- El hilo ejecutor lanza `executePlan()` con un plan secuencial de 7 etapas.
- Un hilo observador secundario espera un retardo mínimo (5 ms) y dispara `token.cancel()`.
- `executePlan()` comprueba `token.isCancelled()` antes de iniciar cada etapa y durante el ciclo de ejecución.
- Al detectar la cancelación:
  - Aborta de inmediato la etapa en curso o subsiguiente.
  - Asigna el estado general del reporte a `CiWindowsJobStatus::Cancelled`.
  - Marca las etapas pendientes como `CiWindowsStepStatus::Omitted`.
  - No genera ningún subproceso colgado o huérfano.

### 5. Recuperación ante bloqueos transitorios de archivos (File Locks)

- En Windows, los archivos abiertos por procesos concurrentes (antivirus, servicio de indexación de Windows Search, inspectores) quedan bloqueados temporalmente contra eliminación.
- `cleanBuildDirectory` implementa reintentos con retroceso exponencial (`retryBackoffMs`):
  $$\text{wait\_time} = \text{retryBackoffMs} \times 2^{\text{attempt}}$$
- Las pruebas simulan este escenario validando que tras N reintentos configurados, la operación concluye de forma limpia sin provocar caídas ni excepciones no controladas.

### 6. Comparativa failFast vs continueOnError

- Se prueba deterministamente un plan con 5 etapas sintéticas donde la etapa 2 genera un fallo inyectado.
- Escenario 1 (`failFast = true`):
  - Etapa 1: `Success`
  - Etapa 2: `Failed`
  - Etapas 3, 4, 5: `Omitted`
  - Estado global: `Failed`
- Escenario 2 (`failFast = false`, `continueOnError = true`):
  - Etapa 1: `Success`
  - Etapa 2: `Failed`
  - Etapas 3, 4, 5: `Success`
  - Estado global: `Failed` (reportando diagnósticos completos de todas las etapas evaluadas).

### 7. Síntesis CLI reproducible para herramientas de Windows

- Se valida la síntesis de líneas de comando para ejecutables ubicados en rutas con espacios típicas de Windows (`C:\Program Files\...`).
- Se certifica el entrecomillado adecuado de argumentos, rutas relativas con separadores normalizados y banderas específicas de toolchain (e.g. `/permissive-`, `-fsanitize=address`).

### 8. Gobernanza y auditoría de textos públicos (UX Copy)

- Verificación exhaustiva contra `docs/UX-COPY-PUBLICO.md`:
  - Los estados se reportan en español claro: «Exitoso», «Fallido», «Cancelado», «Omitido», «En ejecución».
  - Las pistas de remediación (`generateRemediationHint`) orientan con claridad al usuario sobre cómo corregir problemas de ruta, permisos o herramientas faltantes.
  - Ningún texto expone jerga técnica interna de ingeniería.
