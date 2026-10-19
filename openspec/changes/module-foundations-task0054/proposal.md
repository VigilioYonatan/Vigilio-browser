# Change Proposal — module-foundations-task0054

Pack: `module.foundations.task0054`

Status: PROPOSED

## Outcome and scope

Habiéndose completado y certificado formalmente en las capacidades e hitos precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de diagnósticos y advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política integral de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la configuración canónica y servicio de Clang-Format (`task0041`-`task0044`), la configuración canónica y servicio de Clang-Tidy (`task0045`-`task0048`), el pipeline completo de CI Linux (`task0049`-`task0052`), y la formalización del modelo de dominio, tipos inmutables, validador estático de orden topológico y planes canónicos para CI Windows (`task0053`), esta propuesta formaliza la **implementación de la capa operativa de servicio propietaria (`vigilio::CiWindowsJobService`), el orquestador del pipeline de compilación y verificación en Windows, la ejecución determinista en modo simulación/dry-run y en vivo, la cancelación cooperativa multihilo bajo la política Zero-Zombie, el mecanismo de corte temprano ante fallos (fail-fast), la tolerancia selectiva a errores (`continueOnError`), la síntesis reproducible de líneas de comando CLI para toolchains de Windows (MSVC/Clang-cl, PowerShell, Ninja, CMake, CTest, CPack), la gestión y saneamiento seguro del directorio de compilación previniendo Directory Traversal y nombres reservados de DOS/Windows, y el control estricto de presupuestos de tiempo y memoria RAM en C++20**.

El objetivo central de esta etapa es dotar a la biblioteca `vigilio_core` de un servicio desacoplado, seguro y robusto en `src/core/` que orqueste la ejecución ordenada de las etapas canónicas del trabajo de CI en Windows (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`), garantice el cumplimiento del orden de dependencias definido por el dominio, soporte cancelación cooperativa inmediata sin subprocesos zombis colgados indefinidamente, permita la inspección y prueba en modo seco (dry-run) sin requerir llamadas externas destructivas en CTest, ofrezca saneamiento seguro del directorio de compilación previniendo Directory Traversal (`..`, `..\`), letras de unidad (`C:\`), rutas absolutas del host (`\Windows\System32`, `\\server\share`), prefijos de espacio de nombres NT (`\\?\`, `\\.\`) y dispositivos reservados DOS (`CON`, `PRN`, `AUX`, `NUL`), y reporte métricas consolidadas en `CiWindowsJobReport` con recomendaciones en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de negocio a los widgets de interfaz gráfica ni añadir dependencias hacia WebEngine o `BrowserWindow`**.

El alcance concreto comprende:
- **Capa operativa de servicio desacoplada (`vigilio::CiWindowsJobService`)**:
  - Inicialización flexible y configuración: construcción con plan canónico debug por defecto (`canonicalDebugPlan`), plan release (`canonicalReleasePlan`) o inyección de un plan personalizado validado.
  - Gestión de opciones de servicio (`CiWindowsServiceOptions`): modo de ejecución (`DryRun`, `Simulated`, `Live`), reintentos para operaciones de disco, capturas de salida y política de corte inmediato (`failFast`).
  - Orquestación de pipeline completa (`executePlan`):
    - Verificación previa obligatoria mediante `CiWindowsJobValidator::validatePlan` y `CiWindowsJobValidator::isSafeBuildPath`.
    - Ejecución secuencial de etapas respetando el orden topológico de dependencias.
    - Medición de tiempos de ejecución de alta precisión con `std::chrono::steady_clock` para cada etapa individual y para el total del trabajo.
    - Manejo de fallos con política `failFast`: al fallar una etapa obligatoria, se omiten ordenadamente las subsiguientes (`CiWindowsStepStatus::Omitted`).
    - Soporte para `continueOnError`: etapas marcadas con esta opción permiten continuar el pipeline aun cuando fallen, acumulando el resultado.
    - Consolidación del resultado global en `CiWindowsJobReport` (estados `Passed`, `Failed`, `Cancelled`, `TimedOut`).
  - Ejecución granular por etapa individual (`executeStep`): evaluación aislada de una etapa específica para arneses de diagnóstico o reintentos dirigidos.
  - Soporte de **cancelación cooperativa multihilo (Zero-Zombie)** mediante `CancellationToken*`:
    - Comprobación atómica previa a cada etapa y durante operaciones prolongadas.
    - Interrupción segura e inmediata ante solicitud externa, marcando el estado global como `CiWindowsJobStatus::Cancelled`.
    - Garantía absoluta de cero subprocesos huérfanos o colgados esperando EOF.
  - Saneamiento y gestión segura de directorios (`cleanBuildDirectory`):
    - Eliminación recursiva y segura del directorio de construcción configurado.
    - Validación rigurosa de seguridad de ruta (`isSafeBuildPath`) impidiendo el borrado de rutas de sistema (`C:\Windows`, `System32`), secuencias de Directory Traversal (`..`, `..\`) o dispositivos DOS reservados (`CON`, `PRN`, `AUX`, `NUL`).
    - Resiliencia con reintentos exponenciales ante contención de bloqueos de archivo transitorios típicos de Windows (antivirus, indexador de búsqueda).
  - Síntesis determinista y reproducible de comandos CLI (`buildCommandLine`):
    - Generación exacta de comandos para cada etapa del pipeline Windows considerando herramientas nativas (`cmake --preset ci-windows-debug ...`, `cmake --build ...`, `ctest --test-dir ...`, `powershell ... clang-format`, `clang-tidy ...`, `cpack ...`).
  - Monitoreo de límites de recursos y presupuestos:
    - Verificación de presupuesto de memoria RAM (máximo 4 GB según `maxMemoryBytes`).
    - Control de tiempo límite global (`totalTimeoutSeconds`) y por etapa individual (`timeoutMs`).
- **Invariantes constitucionales de calidad, portabilidad y privacidad**:
  - *Local-First Constitucional (ADR-001 / Decisión Q-001)*: Cero conexiones de red, cero servicios cloud, cero telemetría remota.
  - *Aislamiento modular y cero acoplamiento de GUI*: La lógica reside exclusivamente en `vigilio_core`, ejecutable en entornos headless de CTest sin requerir servidor gráfico ni `QApplication`.
  - *Seguridad de rutas Windows*: Confinamiento en rutas relativas de compilación y validación previa de seguridad frente a semánticas del sistema de archivos NTFS/FAT32.
- **Suite completa de pruebas unitarias del servicio (`tests/unit/CiWindowsJobServiceTests.cpp`)**:
  - 12 pruebas unitarias deterministas en CTest (`TEST-CI-WIN-SRV-001` a `TEST-CI-WIN-SRV-012`) cubriendo inicialización, ejecución de pipeline en dry-run, política fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie, saneamiento seguro de directorios y rechazo de traversal/rutas host, ejecución de etapa individual, síntesis de línea de comandos CLI para Windows, límites de recursos y memoria, control de timeouts, reintentos con retroceso exponencial y auditoría de UX copy en español neutro.

---

## Out of scope

- Implementación de componentes visuales de interfaz gráfica, ventanas de visualización de estado de CI, barras de progreso o botones en la interfaz de usuario de Vigilio Browser (reservado para la etapa `ui` en `module.foundations.task0055`).
- Pruebas pesadas de integración multihilo de larga duración con compilación real en máquinas Windows nativas o runners remotos (reservado para la etapa `tests` en `module.foundations.task0056`).
- Pipeline de integración continua para macOS (reservado para las microtareas `task0057` a `task0060`).
- Modificación directa de código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante la fase APPLY tras aprobación humana).
- Descarga o instalación automatizada de paquetes Windows mediante `winget`, `choco` o instaladores MSI externos.
- Conexión a servicios externos de CI/CD (GitHub Actions, Azure DevOps, Jenkins) o almacenamiento de telemetría en la nube.

---

## Contexto y evidencia

### Línea base del proyecto

El proyecto Vigilio Browser cuenta con 22 suites de pruebas unitarias e integración en CTest ejecutándose limpiamente con exit code 0 en aproximadamente 3.75 segundos mediante `apps/vigilio-browser/scripts/check.sh`.

La etapa previa `module.foundations.task0053` (`docs(foundations): domain ci-windows-job`) estableció el modelo de dominio inmutable en `src/core/`:
- `CiWindowsJobTypes.h`: enumeraciones (`CiWindowsPipelineStep`, `CiWindowsJobStatus`, `CiWindowsStepStatus`, `CiWindowsExecutionMode`, `CiWindowsJobError`, `CiWindowsArchitecture`, `CiWindowsCompiler`, `CiWindowsBuildConfiguration`) y estructuras (`CiWindowsServiceOptions`, `CiWindowsEnvironment`, `CiWindowsStepOptions`, `CiWindowsStepDefinition`, `CiWindowsStepResult`, `CiWindowsJobPlan`, `CiWindowsJobReport`, `CancellationToken`).
- `CiWindowsJobConfiguration.h/.cpp`: provisión de `canonicalDebugPlan()` (`ci-windows-debug`), `canonicalReleasePlan()` (`ci-windows-release`), verificación de invariantes (`verifyInvariants`) y catálogo de versiones de Windows compatibles (`isSupportedWindowsVersion`).
- `CiWindowsJobValidator.h/.cpp`: validación formal de planes (`validatePlan`), secuencia topológica (`validateStepSequence`), validación de entorno (`validateEnvironment`), seguridad estricta de rutas con rechazo de traversal, letras de unidad y dispositivos reservados DOS (`isSafeBuildPath`), y funciones de UX copy público en español neutro.
- `CiWindowsJobConfigurationTests.cpp`: 11 pruebas unitarias deterministas en CTest ejecutadas en modo headless con éxito rotundo.

### Evidencia de comandos ejecutados en la exploración

1. `bash apps/vigilio-browser/scripts/check.sh`:
   - 22/22 pruebas pasando limpiamente (100% pass, exit code 0).
   - Tiempo total: 3.75s.
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
2. `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`:
   - `references: 54`, `valid: true`.
3. `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0160-module.foundations.task0054.propose.lock.json --project-root .`:
   - `valid: true`, 13 checks aprobados.

---

## Alcance exacto

El alcance exacto de esta microtarea comprende la especificación y planificación de los siguientes entregables para la fase APPLY:

1. **Diseño e interfaz de la capa operativa de servicio (`CiWindowsJobService.h`)**:
   - Declaración de `vigilio::CiWindowsJobService` en el namespace `vigilio`.
   - Métodos de configuración y ciclo de vida: `CiWindowsJobService()`, `explicit CiWindowsJobService(CiWindowsJobPlan plan)`, `setPlan`, `plan`, `setOptions`, `options`.
   - Métodos de ejecución y orquestación: `executePlan(CancellationToken* cancel = nullptr)`, `executeStep(CiWindowsPipelineStep step, CancellationToken* cancel = nullptr)`.
   - Método de gestión de sistema de archivos: `cleanBuildDirectory(std::string* errorMessage = nullptr)`.
   - Método de síntesis CLI: `buildCommandLine(const CiWindowsStepDefinition& step) const`.
   - Métodos y funciones libres de diagnósticos y UX copy: `executionModeToString(CiWindowsExecutionMode mode)`, `formatStepSummary(const CiWindowsStepResult& result)`.
2. **Implementación de la capa operativa de servicio (`CiWindowsJobService.cpp`)**:
   - Sincronización multihilo mediante `std::mutex` en el acceso al plan y opciones.
   - Orquestación de pipeline con validación previa estricta (`validatePlan`, `isSafeBuildPath`).
   - Medición precisa de duración por etapa y acumulada (`std::chrono::steady_clock`).
   - Política Fail-Fast ante fallos en etapas obligatorias, marcando etapas restantes como `Omitted`.
   - Soporte para `continueOnError`.
   - Soporte de cancelación cooperativa (Zero-Zombie) con `CancellationToken`.
   - Saneamiento seguro de directorios con reintentos exponenciales ($50\text{ ms} \times 2^i$) y control de errores de disco.
   - Síntesis determinista de CLI para las 8 etapas en entorno Windows.
3. **Suite completa de pruebas unitarias (`tests/unit/CiWindowsJobServiceTests.cpp`)**:
   - 12 pruebas unitarias deterministas (`TEST-CI-WIN-SRV-001` a `TEST-CI-WIN-SRV-012`).
4. **Registro de target y pruebas en `CMakeLists.txt`**:
   - Incorporación de `src/core/CiWindowsJobService.cpp` a la librería `vigilio_core`.
   - Creación del ejecutable `vigilio_ci_windows_job_service_tests` enlazado con `vigilio_core`.
   - Registro de la prueba `ci_windows_job_service_tests` en CTest con labels `unit;fast` y timeout de 15 segundos.
5. **Sincronización de gobernanza y microtarea**:
   - Actualización de `docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md` documentando el estado de implementación completado.

---

## Diseño completo de la capacidad

### 1. Entidades de datos y responsabilidades

| Entidad | Capa | Responsabilidad principal |
| --- | --- | --- |
| `CiWindowsJobTypes.h` | Dominio (`vigilio_core`) | Define tipos, estados, opciones, definiciones de etapas, planes y reportes inmutables de CI Windows. |
| `CiWindowsJobConfiguration` | Dominio (`vigilio_core`) | Provee los planes canónicos oficiales (`canonicalDebugPlan`, `canonicalReleasePlan`) y valida invariantes. |
| `CiWindowsJobValidator` | Dominio (`vigilio_core`) | Validador estático desacoplado de coherencia de plan, orden topológico, seguridad de rutas NTFS y UX copy. |
| `CiWindowsJobService` | Operativa (`vigilio_core`) | Orquesta la ejecución ordenada, aplica fail-fast, cancelación cooperativa Zero-Zombie, saneamiento seguro y síntesis CLI. |
| `CancellationToken` | Concurrencia (`vigilio_core`) | Señalización atómica multihilo para cancelación cooperativa sin procesos huérfanos. |

### 2. Contrato funcional

- **Invocación**:
  - `CiWindowsJobReport executePlan(CancellationToken* cancel = nullptr)`:
    - Precondiciones: El plan configurado debe superar `validatePlan(plan_)` y `isSafeBuildPath(plan_.buildDir)`.
    - Postcondiciones: Devuelve un `CiWindowsJobReport` consolidado con estado (`Passed`, `Failed`, `Cancelled`), duración total en ms, resultados individuales por etapa y sugerencia de remediación si hubo fallos.
    - Invariantes: Si `cancel->isCancelled()` es true en cualquier punto, la ejecución se interrumpe inmediatamente y el estado final es `CiWindowsJobStatus::Cancelled`.
  - `CiWindowsStepResult executeStep(CiWindowsPipelineStep step, CancellationToken* cancel = nullptr)`:
    - Precondiciones: La etapa debe estar definida en el plan o resolver una definición canónica por defecto.
    - Postcondiciones: Devuelve un `CiWindowsStepResult` inmutable con estado (`Success`, `Failure`), código de salida y resumen.
  - `bool cleanBuildDirectory(std::string* errorMessage = nullptr)`:
    - Precondiciones: La ruta `plan_.buildDir` debe ser validada como segura por `CiWindowsJobValidator::isSafeBuildPath`.
    - Postcondiciones: Si la ruta es segura, se elimina recursivamente mediante `std::filesystem::remove_all` con reintentos exponenciales ante contención de bloqueos; si es insegura, devuelve `false` y un mensaje de error descriptivo en español neutro sin tocar el disco.
  - `std::string buildCommandLine(const CiWindowsStepDefinition& step) const`:
    - Genera la cadena de invocación CLI para Windows correspondiente al paso (`cmake`, `ninja`, `cl.exe`, `ctest`, `powershell clang-format`, `clang-tidy`, `cpack`).

---

## Archivos afectados

| Archivo | Acción | Justificación técnica |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/CiWindowsJobService.h` | Crear | Declaración de la clase de servicio propietaria `CiWindowsJobService` y funciones libres auxiliares. |
| `apps/vigilio-browser/src/core/CiWindowsJobService.cpp` | Crear | Implementación del orquestador, sincronización con mutex, fail-fast, cancelación, saneamiento y síntesis CLI. |
| `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp` | Crear | Suite exhaustiva de 12 pruebas unitarias deterministas en CTest. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificar | Agregar `CiWindowsJobService.cpp` a `vigilio_core` y registrar el target ejecutable de pruebas y test en CTest. |
| `docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md` | Modificar | Registrar el estado completado de la etapa implementation en la microtarea canónica. |

---

## Contrato UX/UI

- **Sin cambio visual directo**: Esta microtarea pertenece a la capa de infraestructura del núcleo C++20 (`vigilio_core`) para la orquestación y automatización de integración continua en Windows. No añade ventanas, botones, paneles ni menús a `BrowserWindow`.
- **Compatibilidad con Modo Senior («Interfaz más fácil de usar»)**:
  - Preserva íntegramente la estabilidad de los componentes existentes de interfaz gráfica.
  - La ejecución headless no altera tamaños de botones, contraste WCAG ni áreas de clic.
- **Textos públicos y neutralidad (conforme a `docs/UX-COPY-PUBLICO.md`)**:
  - Queda terminantemente proscrito el uso de jerga interna como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain» o «runner» en cualquier mensaje de error, resumen de etapa o sugerencia devuelta por el servicio.
  - Todos los textos de estado, resumen de ejecución y pistas de remediación se formatean en español neutro comprensible.

---

## Reglas y fallos

1. **Entrada inválida o plan malformado**:
   - Si `validatePlan` detecta etapas faltantes, orden topológico invertido, tiempo límite cero o rutas inseguras, `executePlan` aborta antes de ejecutar cualquier subproceso y retorna `CiWindowsJobStatus::Failed` con el error de validación correspondiente y una pista de remediación generada por `generateRemediationHint`.
2. **Fallo en etapa obligatoria con Fail-Fast**:
   - Si una etapa obligatoria (`isMandatory = true`) falla (código de salida distinto de 0 o excepción) y `options.failFast` está activo, las etapas posteriores se marcan como `CiWindowsStepStatus::Omitted` y el reporte global se fija en `CiWindowsJobStatus::Failed`.
3. **Fallo en etapa con tolerancia (`continueOnError = true`)**:
   - Si la etapa tiene activa la bandera `continueOnError`, el fallo se registra en el `CiWindowsStepResult`, pero el servicio prosigue ejecutando las etapas restantes.
4. **Cancelación cooperativa (Zero-Zombie)**:
   - Si `cancelToken->isCancelled()` se evalúa verdadero antes de iniciar una etapa o durante su espera, el bucle se detiene inmediatamente, las etapas subsiguientes se marcan como `Omitted`, el estado global se establece en `Cancelled`, y no se deja ningún proceso en segundo plano colgado.
5. **Intento de saneamiento de ruta insegura**:
   - Si `cleanBuildDirectory` recibe una ruta que contiene `..`, `..\`, letras de unidad (`C:\`), rutas de host (`\Windows\System32`), prefijos NT (`\\?\`) o nombres DOS reservados (`CON`, `NUL`), la operación se rechaza de inmediato con retorno `false`, preservando la integridad del sistema.
6. **Bloqueos de archivo transitorios en Windows**:
   - La eliminación de archivos aplica retroceso exponencial ($50\text{ ms} \times 2^i$) hasta `maxRetries` (por defecto 3 reintentos) para superar contenciones temporales de manejadores abiertas por antivirus o indexadores del sistema.

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Cero dependencias de servicios en la nube, telemetría remota, analíticas o servidores externos.
  - Toda la orquestación, validación y ejecución ocurre exclusivamente en la máquina local o runner de CI.
- **Aislamiento de procesos y memoria**:
  - El presupuesto de memoria RAM está estrictamente acotado a 4 GB (`maxMemoryBytes = 4294967296ULL`).
  - No se instancian contextos de Chromium WebEngine ni llamadas a MCP durante la ejecución del servicio.
- **Defensas NTFS y sistema de archivos Windows**:
  - Restricción terminante a subdirectorios locales de compilación (`build-ci-win-debug`, `build-ci-win-release`, `_build`).
  - Verificación léxica previa para impedir el acceso o borrado de archivos del sistema operativo Windows.

---

## Matriz de pruebas

| ID Prueba | Tipo | Fixture / Método | Comportamiento verificado | Criterio de éxito |
| --- | --- | --- | --- | --- |
| `TEST-CI-WIN-SRV-001` | Unitario | `CiWindowsJobService` | Inicialización por defecto con plan debug canónico y con plan personalizado | Plan y opciones configurados correctamente |
| `TEST-CI-WIN-SRV-002` | Unitario | `CiWindowsJobService` | Concurrencia y sincronización de `setPlan`, `plan`, `setOptions`, `options` | Cero condiciones de carrera, coherencia bajo mutex |
| `TEST-CI-WIN-SRV-003` | Unitario | `executePlan` | Ejecución completa del pipeline en modo `DryRun` | 8/8 etapas completadas con éxito, estado `Passed` y métricas de tiempo |
| `TEST-CI-WIN-SRV-004` | Unitario | `executePlan` | Política Fail-Fast ante fallo en etapa obligatoria | Etapa fallida registrada, etapas subsiguientes `Omitted`, estado `Failed` |
| `TEST-CI-WIN-SRV-005` | Unitario | `executePlan` | Tolerancia a fallos con `continueOnError = true` | Etapa fallida registrada, pero las etapas posteriores sí se ejecutan |
| `TEST-CI-WIN-SRV-006` | Unitario | `executePlan` | Cancelación cooperativa con `CancellationToken` (Zero-Zombie) | Interrupción inmediata, pasos omitidos, estado `Cancelled` |
| `TEST-CI-WIN-SRV-007` | Unitario | `cleanBuildDirectory` | Saneamiento seguro de directorio de construcción válido | Limpieza exitosa y retorno `true` |
| `TEST-CI-WIN-SRV-008` | Unitario | `cleanBuildDirectory` | Rechazo categórico de rutas inseguras (`..`, `C:\Windows`, `CON`, `NUL`) | Operación denegada, disco intacto, mensaje descriptivo |
| `TEST-CI-WIN-SRV-009` | Unitario | `executeStep` | Ejecución granular de una etapa individual aislada | `CiWindowsStepResult` con estado `Success`, duración y salida |
| `TEST-CI-WIN-SRV-010` | Unitario | `buildCommandLine` | Síntesis determinista de argumentos CLI para Windows para las 8 etapas | Comandos exactos para MSVC, Ninja, CMake, CTest, PowerShell, CPack |
| `TEST-CI-WIN-SRV-011` | Unitario | `executePlan` | Verificación de límites de presupuesto (memoria 4 GB y timeouts) | Rechazo temprano si el presupuesto excede los límites |
| `TEST-CI-WIN-SRV-012` | Unitario | Diagnósticos / Copy | Auditoría de cadenas devueltas contra `docs/UX-COPY-PUBLICO.md` | Español neutro, cero términos prohibidos |

---

## Pasos APPLY

1. **Crear `apps/vigilio-browser/src/core/CiWindowsJobService.h`**:
   - Declarar la interfaz pública de la clase `CiWindowsJobService` con soporte multihilo, métodos de ciclo de vida, orquestación, saneamiento, síntesis CLI y funciones libres de diagnósticos.
2. **Crear `apps/vigilio-browser/src/core/CiWindowsJobService.cpp`**:
   - Implementar la lógica del servicio, validación previa con `CiWindowsJobValidator`, bucle de ejecución secuencial, medición con `std::chrono::steady_clock`, fail-fast, `continueOnError`, evaluación de `CancellationToken`, limpieza recursiva segura con reintentos y síntesis determinista de comandos CLI para Windows.
3. **Crear `apps/vigilio-browser/tests/unit/CiWindowsJobServiceTests.cpp`**:
   - Implementar los 12 casos de prueba unitarios deterministas (`TEST-CI-WIN-SRV-001` a `TEST-CI-WIN-SRV-012`).
4. **Modificar `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/CiWindowsJobService.cpp` al target `vigilio_core`.
   - Definir el ejecutable de pruebas `vigilio_ci_windows_job_service_tests` y registrar el test `ci_windows_job_service_tests` en CTest con labels `unit;fast` y timeout de 15 segundos.
5. **Ejecutar verificación y gates de calidad**:
   - Correr `apps/vigilio-browser/scripts/check.sh` asegurando compilación limpia con `-Werror`, formato Clang-Format aprobado y 100% de pruebas en CTest pasando (23/23 tests esperados).
6. **Actualizar la microtarea canónica**:
   - Actualizar `docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md` documentando el estado de la implementación.

---

## Definición de terminado

La etapa **implementation** de **CI Windows job** se considerará terminada al cumplirse la totalidad de los siguientes criterios:
1. `CiWindowsJobService.h` y `CiWindowsJobService.cpp` implementados en `src/core/` con arquitectura desacoplada, orquestación determinista, fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie y saneamiento seguro.
2. 12 pruebas unitarias operativas en `CiWindowsJobServiceTests.cpp` ejecutándose y aprobadas al 100% en CTest (`TEST-CI-WIN-SRV-001` a `TEST-CI-WIN-SRV-012`).
3. El target ejecutable `vigilio_ci_windows_job_service_tests` y el test `ci_windows_job_service_tests` integrados en `CMakeLists.txt` con labels `unit;fast`.
4. El pipeline continuo `apps/vigilio-browser/scripts/check.sh` pasa limpiamente con `-Werror`, cero advertencias y 23/23 pruebas aprobadas.
5. Saneamiento de directorios verifica rigurosamente `isSafeBuildPath`, previniendo Directory Traversal y dispositivos DOS reservados de Windows con reintentos exponenciales.
6. Cero acoplamiento hacia `QApplication`, widgets de interfaz gráfica o dependencias de WebEngine.
7. Textos públicos, resúmenes y sugerencias de remediación auditados en español neutro conforme a `docs/UX-COPY-PUBLICO.md`.
8. Comprobaciones de `validate-references` y `verify-lock` certificadas con resultado `valid: true`.
9. Documento de microtarea canónica `docs/rules/01-modules/foundations/micro/0054-ci-windows-job/task.md` formalizado y sincronizado.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0053`: Modelo de dominio, validador de invariantes y configuración canónica de CI Windows (completada y certificada).
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Contención de manejadores de archivos y bloqueos transitorios en sistemas Windows durante el saneamiento del directorio de compilación.
    *Mitigación*: Implementación de retroceso exponencial con hasta 3 reintentos separados por intervalos de $50\text{ ms} \times 2^i$.
  - *Riesgo 2*: Subprocesos colgados o zombis esperando indefinidamente durante la cancelación.
    *Mitigación*: Adopción estricta del patrón `CancellationToken` (Zero-Zombie), evaluado atómicamente antes y durante cada etapa.
  - *Riesgo 3*: Filtrado de jerga técnica interna en textos visibles.
    *Mitigación*: Prueba unitaria automatizada `TEST-CI-WIN-SRV-012` que audita programáticamente cada cadena producida contra el catálogo de `docs/UX-COPY-PUBLICO.md`.

---

## Commit propuesto

Una vez aprobada esta propuesta por el usuario e implementada en la fase APPLY, el commit resultante será:

```bash
feat(foundations): implementation ci-windows-job
```
