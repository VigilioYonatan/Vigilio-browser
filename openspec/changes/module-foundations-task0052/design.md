# Design — module-foundations-task0052

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Modelo de dominio y capa operativa de servicio de CI Linux completados (`task0049`, `task0050` y `task0051`)**:
   - `vigilio_core` cuenta con `src/core/CiLinuxJobTypes.h`, que establece enumeraciones fuertemente tipadas (`CiLinuxPipelineStep`, `CiLinuxJobStatus`, `CiLinuxStepStatus`, `CiLinuxExecutionMode`, `CiLinuxJobError`, `CiLinuxArchitecture`, `CiLinuxCompiler`, `CiLinuxBuildConfiguration`), estructuras inmutables de configuración y ejecución (`CiLinuxServiceOptions`, `CiLinuxEnvironment`, `CiLinuxStepOptions`, `CiLinuxStepDefinition`, `CiLinuxStepResult`, `CiLinuxJobPlan`, `CiLinuxJobReport`) y el token cooperativo de cancelación multihilo `CancellationToken`.
   - `src/core/CiLinuxJobValidator.h/.cpp` implementa la validación estática de planes (`validatePlan`), verificación de secuencias topológicas (`validateStepSequence`), validación de requisitos de entorno C++20 (`validateEnvironment`), comprobación de rutas seguras frente a Directory Traversal (`isSafeBuildPath`) y traducción de estados, errores, etapas y arquitecturas a texto descriptivo en español neutro (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`).
   - `src/core/CiLinuxJobConfiguration.h/.cpp` genera los planes canónicos del proyecto (`canonicalDebugPlan`, `canonicalReleasePlan`), verifica invariantes del sistema (`verifyInvariants`) y valida distribuciones Linux soportadas (Ubuntu, Debian, Fedora, Arch, Alpine).
   - `src/core/CiLinuxJobService.h/.cpp` proporciona la capa operativa desacoplada con orquestación secuencial de pipelines en modo simulación dry-run determinista (`executePlan`), ejecución granular de etapas (`executeStep`), corte temprano ante fallos (`failFast`), tolerancia selectiva a fallos (`continueOnError`), cancelación cooperativa multihilo Zero-Zombie, saneamiento seguro de directorios (`cleanBuildDirectory`), síntesis CLI reproducible (`buildCommandLine`) y diagnósticos públicos (`executionModeToString`, `formatStepSummary`).
2. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/CiLinuxJobConfigurationTests.cpp`: 11 pruebas unitarias enfocadas en plan canónico debug, plan canónico release, validación de plan, pasos vacíos, orden topológico de etapas, validación de entorno, rutas seguras y directory traversal, distribuciones soportadas, opciones y resultados de etapa, UX copy público y pistas de remediación.
   - `tests/unit/CiLinuxJobServiceTests.cpp`: 12 pruebas unitarias enfocadas en inicialización, dry-run pipeline, fail-fast, continue on error, cancelación básica, limpieza segura de directorio de build y traversal, ejecución individual de etapa, síntesis CLI, límites de recursos y memoria, manejo de timeout, reintentos con retroceso exponencial ante contención y cumplimiento de UX Copy.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario con verificación programática dry-run de cobertura de compilación de UI en el pipeline de CI Linux y auditoría de textos públicos de remediación.
3. **Estado de compilación y pruebas continuas**:
   - Compilación limpia bajo C++20 con `-Werror` y cero advertencias.
   - 21 de 21 pruebas en CTest pasando con éxito en ~3.96 segundos.
4. **Brecha técnica identificada para la etapa `tests` (GAP)**:
   - Las pruebas existentes cubren los flujos nominales y casos negativos estándar de forma secuencial y en escenarios controlados monohilo, pero carecen de:
     - Pruebas sistemáticas de estrés bajo concurrencia masiva multihilo (16 workers simultáneos ejecutando planes y pasos sobre una única instancia compartida de `CiLinuxJobService`).
     - Pruebas de estrés de cancelación cooperativa asíncrona multihilo en tiempo real bajo la política Zero-Zombie, donde la cancelación se dispara concurrentemente desde un hilo independiente mientras el pipeline está en ejecución.
     - Inyección profunda de fallos de permisos de archivo en disco sobre el directorio de compilación (`perms::owner_read`) y verificación de robustez sin excepciones no capturadas.
     - Verificación exhaustiva de fronteras en configuración de planes (timeouts extremos, cuotas de memoria anómalas) y vectores complejos de Directory Traversal (`./../..`, `build/../../etc`, `..\\`, rutas protegidas del sistema y caracteres nulos).
     - Comparativa determinista de las políticas de corte (`failFast` vs `continueOnError`) ante etapas que fallan en secuencia.

---

### Estado objetivo (TARGET)

1. **Suite exhaustiva de pruebas automatizadas y deterministas para CI Linux Job**:
   - **Pruebas de frontera de configuración y Directory Traversal avanzado (`CiLinuxJobConfigurationTests.cpp`)**:
     - Validación en límites de planes: timeouts iguales a cero o valores negativos simulados, cuotas extremas de memoria RAM ($0$ bytes, $> 64$ GB), nombres de preset vacíos y comandos truncados.
     - Detección y rechazo de secuencias desordenadas complejas: etapas que rompen las dependencias topológicas (e.g. `TestSuiteExecution` sin previa compilación, `ArtifactPackaging` sin tests previos, pasos duplicados).
     - Detección y rechazo riguroso de ataques complejos de Directory Traversal (`./../..`, `build/../../etc`, `..\\windows\\path`, `/etc/shadow`, `/var/run`, inyección de caracteres nulos embebidos `\0`).
     - Verificación de límites de versiones de compiladores y herramientas C++20 en `validateEnvironment`.
   - **Pruebas de concurrencia masiva y estrés multihilo (`CiLinuxJobServiceTests.cpp`)**:
     - 16 hilos (`std::thread`) concurrentes ejecutando simultáneamente sobre una única instancia compartida de `CiLinuxJobService`: 8 hilos ejecutando `executePlan` en modo `DryRun` con planes heterogéneos y 8 hilos ejecutando `executeStep` sobre distintas etapas del pipeline.
     - Certificación de ausencia total de data races, contención segura mediante `std::mutex` interno y convergencia determinista de todos los workers sin deadlocks.
   - **Pruebas de cancelación cooperativa multihilo y política Zero-Zombie (`CiLinuxJobServiceTests.cpp`)**:
     - Disparo asíncrono de `CancellationToken` desde un hilo secundario durante la ejecución secuencial del plan de 8 etapas, comprobando interrupción inmediata en la etapa activa, transición a `CiLinuxJobStatus::Cancelled`, marcado de etapas posteriores como `CiLinuxStepStatus::Omitted`, y ausencia de subprocesos colgados.
   - **Pruebas de inyección de fallos de permisos y recuperación (`CiLinuxJobServiceTests.cpp`)**:
     - Simulación de directorios con permisos restringidos de sólo lectura (`std::filesystem::perms::owner_read`), certificando que `cleanBuildDirectory` reporta limpiamente `false` con captura de error sin abortos ni excepciones no capturadas.
   - **Evaluación comparativa determinista de políticas de corte (`CiLinuxJobServiceTests.cpp`)**:
     - Evaluación directa y contrastada de `failFast == true` (corte inmediato en el primer fallo con etapas posteriores marcadas como `Omitted`) vs `failFast == false` / `continueOnError == true` (ejecución continuada de etapas no dependientes para diagnóstico completo).
   - **Síntesis CLI avanzada (`CiLinuxJobServiceTests.cpp`)**:
     - Verificación de síntesis CLI con rutas complejas que contienen espacios, parámetros relativos y absolutos, y banderas de compilación y sanitizers.
2. **Rendimiento, portabilidad y desacoplamiento**:
   - Tiempo de ejecución total de la suite < 500 ms en CTest.
   - Cero requerimientos de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) o inicialización de `QApplication` para las pruebas del núcleo.
   - Cumplimiento riguroso de la política Local-First (ADR-001 / Decisión Q-001).
3. **Auditoría de UX Copy**:
   - 100% de los mensajes de diagnóstico, estados, errores y remediación auditados contra `docs/UX-COPY-PUBLICO.md`, confirmando ausencia absoluta de términos de ingeniería interna.

---

### Brechas identificadas a resolver en APPLY (GAPS)

| Componente bajo prueba | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| Fronteras de configuración de planes | Pruebas de planes canónicos válidos y paso vacío único | Cobertura sistemática de timeouts límites ($\le 0$), memoria anómala y nombres vacíos | Implementar `test_ci_linux_plan_validation_boundaries` en `CiLinuxJobConfigurationTests.cpp`. |
| Secuencias topológicas complejas | Prueba básica de orden `ConfigurePreset` -> `BuildCompilation` | Evaluación de secuencias arbitrariamente desordenadas, pasos duplicados y saltos de dependencia | Implementar `test_ci_linux_step_sequence_topological_violations` en `CiLinuxJobConfigurationTests.cpp`. |
| Directory Traversal avanzado | Prueba básica de `../../etc` y `/etc/shadow` | Prueba de ataques avanzados (`./../..`, `build/../../etc`, `..\\`, rutas con nulos embebidos) | Implementar `test_ci_linux_advanced_path_traversal` en `CiLinuxJobConfigurationTests.cpp`. |
| Límites de entorno C++20 | Prueba básica de GCC 14 y Ubuntu 24.04 | Evaluación de límites exactos (GCC < 14, Clang < 18, CMake < 3.28, Ninja < 1.12, Arch desconocida) | Implementar `test_ci_linux_environment_boundaries` en `CiLinuxJobConfigurationTests.cpp`. |
| Concurrencia masiva multihilo | Pruebas secuenciales monohilo en `CiLinuxJobServiceTests.cpp` | Prueba con 16 hilos simultáneos operando sobre la misma instancia | Implementar `test_ci_linux_service_massive_multithreading_concurrency` en `CiLinuxJobServiceTests.cpp`. |
| Estrés de cancelación cooperativa | Cancelación pre-activada en un solo hilo | Cancelación disparada concurrentemente desde hilo separado durante ejecución secuencial (Zero-Zombie) | Implementar `test_ci_linux_service_cooperative_cancellation_stress` en `CiLinuxJobServiceTests.cpp`. |
| Inyección de fallos de permisos | Solo se prueba traversal en limpieza | Fallo real de I/O forzando permisos de solo lectura (`perms::owner_read`) | Implementar `test_ci_linux_service_permission_failure_injection` en `CiLinuxJobServiceTests.cpp`. |
| Reintentos con retroceso exponencial | Prueba básica de retry en `CiLinuxJobServiceTests.cpp` | Verificación de retroceso exponencial ante contención transitoria simulada | Implementar `test_ci_linux_service_retry_backoff_transient` en `CiLinuxJobServiceTests.cpp`. |
| Comparativa de políticas de corte | Pruebas separadas de fail-fast y continue-on-error | Prueba comparativa determinista directa sobre el mismo plan con fallos inyectados | Implementar `test_ci_linux_service_fail_fast_vs_continue_on_error` en `CiLinuxJobServiceTests.cpp`. |
| Síntesis CLI con rutas complejas | Comandos con nombres de preset simples | Síntesis con rutas que contienen espacios, subdirectorios anidados y banderas avanzadas | Implementar `test_ci_linux_service_cli_synthesis_advanced` en `CiLinuxJobServiceTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad C++20 y Qt6
- **Estándar C++20**: Uso estricto de `std::filesystem`, `std::string_view`, `std::thread`, `std::atomic`, `enum class`, const-correctness y tipos estándar.
- **Independencia del núcleo**: `vigilio_core` permanece 100% independiente de Qt GUI, Widgets y WebEngine. Las pruebas unitarias se ejecutan directamente sobre la biblioteca del núcleo sin requerir enlaces a Qt Widgets o Chromium.
- **Portabilidad de rutas**: Manipulación exclusiva mediante `std::filesystem::path` con operadores de concatenación portables (`/`), garantizando soporte indistinto en plataformas Linux, macOS y Windows.

### Estrategia de migración
- No existen cambios destructivos en las APIs existentes ni modificaciones en las firmas de `CiLinuxJobService`, `CiLinuxJobValidator` o `CiLinuxJobConfiguration`.
- Las nuevas pruebas amplían la cobertura de regresión y resiliencia en los archivos de prueba existentes (`CiLinuxJobConfigurationTests.cpp` y `CiLinuxJobServiceTests.cpp`), manteniéndose plenamente compatibles con la infraestructura de CMake y CTest.

### Procedimiento de reversión (Rollback)
- Si alguna prueba introduce inestabilidad, fallo de compilación o flakiness en entornos con limitaciones de recursos:
  1. Identificar la función de prueba afectada mediante el reporte detallado de CTest (`ctest --output-on-failure`).
  2. Revertir los cambios en los archivos de prueba afectados a su estado consolidado anterior mediante `git checkout HEAD -- apps/vigilio-browser/tests/unit/`.
  3. Revalidar compilación y suites completas con `apps/vigilio-browser/scripts/check.sh`.

---

## Arquitectura técnica de pruebas y matriz de resiliencia

### 1. Desacoplamiento de componentes bajo prueba

La arquitectura de pruebas aísla con precisión las responsabilidades:
- **`CiLinuxJobValidator`**: Componente funcional puro y estático. No realiza operaciones de E/S en disco salvo validaciones puras en memoria. No posee estado mutable ni acoplamientos externos.
- **`CiLinuxJobConfiguration`**: Fábrica estática inmutable de configuración de planes, verificación de invariantes y catálogo de distribuciones. No realiza E/S ni depende de hilos.
- **`CiLinuxJobService`**: Componente operativo con estado interno protegido por `std::mutex`. Coordina operaciones en memoria pura (modo `DryRun`), ejecución granular de etapas (`executeStep`), cancelación cooperativa multihilo Zero-Zombie, y saneamiento seguro de directorios en disco (`cleanBuildDirectory`).

### 2. Jerarquía y cobertura exhaustiva de pruebas para CI Linux Job

#### Nivel 1: Validación estática, límites y Directory Traversal (`CiLinuxJobConfigurationTests.cpp`)
- Evalúa la coherencia de validación de planes ante valores de frontera: timeouts extremos ($\le 0$), cuotas de memoria inválidas ($0$ bytes, $> 64$ GB), nombres vacíos.
- Evalúa la detección de violaciones topológicas en secuencias de etapas (`TestSuiteExecution` antes de `BuildCompilation`, omisión de pasos obligatorios).
- Verifica la prevención estricta de Directory Traversal en `isSafeBuildPath` con vectores avanzados (`./../..`, `build/../../etc`, `..\\`, `/etc/shadow`, rutas con caracteres nulos embebidos).
- Comprueba los límites de versiones mínimas de compiladores y herramientas en `validateEnvironment`.

#### Nivel 2: Concurrencia masiva multihilo (16 hilos) y sincronización (`CiLinuxJobServiceTests.cpp`)
- Ejecuta 16 hilos trabajadores concurrentes sobre una única instancia compartida de `CiLinuxJobService`:
  - 8 hilos ejecutando concurrentemente `executePlan` en modo `DryRun` con planes heterogéneos.
  - 8 hilos ejecutando granularmente `executeStep` sobre distintas etapas del pipeline.
- Certifica que la protección por `std::mutex` en `plan_` y `options_` previene cualquier condición de carrera o corrupción de datos.

#### Nivel 3: Cancelación cooperativa multihilo y política Zero-Zombie (`CiLinuxJobServiceTests.cpp`)
- Dispara asíncronamente `CancellationToken` desde un hilo secundario en tiempo real durante la ejecución secuencial de un plan de 8 etapas.
- Comprueba que la etapa en curso finaliza limpiamente y las etapas posteriores son inmediatamente abortadas y marcadas como `CiLinuxStepStatus::Omitted`.
- Verifica la ausencia total de procesos zombis o colgados esperando EOF.

#### Nivel 4: Inyección de fallos de permisos y recuperación (`CiLinuxJobServiceTests.cpp`)
- Inyecta fallos de permisos en directorios de compilación restringiendo permisos a solo lectura (`std::filesystem::perms::owner_read`).
- Comprueba que `cleanBuildDirectory` maneja el fallo con elegancia, retornando `false` y capturando el mensaje descriptivo sin lanzar excepciones no controladas.

#### Nivel 5: Evaluación de políticas de corte (`failFast` vs `continueOnError`) (`CiLinuxJobServiceTests.cpp`)
- Compara directamente el comportamiento ante etapas que fallan:
  - En `failFast == true`, el primer fallo aborta inmediatamente el pipeline, omitiendo todas las etapas siguientes.
  - En `failFast == false` / `continueOnError == true`, el servicio continúa evaluando las etapas subsiguientes para recabar un diagnóstico integral.

#### Nivel 6: Síntesis CLI y auditoría de UX Copy (`CiLinuxJobServiceTests.cpp`)
- Valida la síntesis determinista de líneas de comando CLI para invocaciones complejas (rutas con espacios, argumentos de compilación y sanitizers).
- Audita programáticamente que todas las funciones de diagnóstico y estado emplean español neutro conforme a `docs/UX-COPY-PUBLICO.md`.

---

## Invariantes y políticas de resiliencia

1. **Invariante de inmutabilidad en modo DryRun**:
   - `CiLinuxJobService::executePlan` en modo `DryRun` jamás crea, modifica ni elimina archivos o directorios en disco. Toda la evaluación se realiza estrictamente en memoria.
2. **Invariante de contención y exclusión mutua**:
   - Toda lectura o mutación del estado interno de `CiLinuxJobService` está protegida por `std::mutex` (`mutex_`), garantizando seguridad multihilo absoluta en entornos de concurrencia masiva.
3. **Invariante de la política Zero-Zombie**:
   - Ninguna operación de ejecución, cancelación o simulación deja subprocesos huérfanos o procesos esperando EOF indefinidamente.
4. **Invariante de contención de rutas (Path Confinement)**:
   - `cleanBuildDirectory` nunca ejecutará `std::filesystem::remove_all` sobre rutas que no hayan sido previamente validadas como relativas y seguras por `isSafeBuildPath`.
5. **Invariante de determinismo temporal**:
   - La suite completa de pruebas unitarias debe ejecutarse en menos de 500 ms en CTest, manteniendo la etiqueta `fast`.
