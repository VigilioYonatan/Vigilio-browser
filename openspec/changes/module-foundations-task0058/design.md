# Design — module-foundations-task0058

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Modelo de dominio de CI macOS completado en la etapa previa (`task0057`)**:
   - `CiMacOsJobTypes.h` define las enumeraciones fuertemente tipadas `CiMacOsPipelineStep`, `CiMacOsJobStatus`, `CiMacOsStepStatus`, `CiMacOsExecutionMode`, `CiMacOsJobError`, `CiMacOsArchitecture`, `CiMacOsCompiler`, `CiMacOsBuildConfiguration`, así como las estructuras inmutables `CiMacOsServiceOptions`, `CiMacOsEnvironment`, `CiMacOsStepOptions`, `CiMacOsStepDefinition`, `CiMacOsStepResult`, `CiMacOsJobPlan`, `CiMacOsJobReport` y `CancellationToken`.
   - `CiMacOsJobConfiguration` provee los planes canónicos oficiales (`canonicalDebugPlan`, `canonicalReleasePlan`), verificación formal de invariantes (`verifyInvariants`) y filtros de versiones de macOS y Darwin compatibles (`isSupportedMacOsVersion`).
   - `CiMacOsJobValidator` provee validación de planes (`validatePlan`), verificación de orden topológico de etapas (`validateStepSequence`), validación de entorno de compilación macOS (`validateEnvironment`), validación de seguridad de rutas APFS/HFS+ con defensas ante traversal y rutas host (`isSafeBuildPath`) y funciones de conversión y textos en español neutro (`jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`, `architectureToString`, `compilerToString`, `generateRemediationHint`).
   - La suite `CiMacOsJobConfigurationTests.cpp` (11 pruebas unitarias) pasa limpiamente en CTest.
2. **Carencia de una capa de servicio operativo en tiempo de ejecución para macOS**:
   - No existe una clase coordinadora (`CiMacOsJobService`) que orqueste la ejecución ordenada de las etapas del pipeline de CI macOS sobre configuraciones reales o simuladas.
   - No hay soporte para ejecución en modo no destructivo o simulación (*dry-run*) a nivel de pipeline completo o etapas individuales de macOS.
3. **Ausencia de orquestación de etapas con políticas fail-fast y tolerancia a fallos**:
   - No existe un mecanismo que ejecute las etapas secuencialmente, mida tiempos de ejecución de alta precisión (`std::chrono::steady_clock`), aborte la ejecución de etapas dependientes si una etapa obligatoria falla (*fail-fast*), o permita continuar ante fallos no críticos (`continueOnError`).
4. **Falta de soporte de cancelación cooperativa multihilo (Zero-Zombie)**:
   - No se dispone de un mecanismo en la capa de servicio para interrumpir de manera segura e inmediata la ejecución de un trabajo de CI macOS cuando un hilo o usuario lo solicite, arriesgando subprocesos colgados o zombis esperando indefinidamente.
5. **Carencia de síntesis determinista de argumentos CLI para el entorno macOS**:
   - No existe un generador de comandos CLI reproducible que sintetice la invocación de CMake presets, Ninja, Apple Clang / Homebrew Clang, CTest y verificación del bundle nativo `.app` para cada etapa del trabajo.
6. **Ausencia de saneamiento seguro del directorio de compilación con defensas macOS**:
   - No existe una rutina segura que limpie el directorio de construcción antes o después de la compilación verificando previamente la ausencia de Directory Traversal (`..`, `../`), rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`), y artefactos específicos de macOS (`.DS_Store`, `._*`), aplicando reintentos exponenciales ante contención de bloqueos transitorios.

---

### Estado objetivo (TARGET)

1. **Capa operativa de servicio desacoplada (`vigilio::CiMacOsJobService`)**:
   - Integrada en `apps/vigilio-browser/src/core/` dentro del target `vigilio_core`.
   - Proporciona orquestación completa de pipeline (`executePlan`), ejecución granular de etapas (`executeStep`), modo simulación determinista (`DryRun`), saneamiento seguro de directorios (`cleanBuildDirectory`) y síntesis CLI reproducible para macOS (`buildCommandLine`).
   - Cero dependencias hacia `QApplication`, widgets de Qt o instancias de Chromium WebEngine.
2. **Orquestación robusta y tolerante a fallos**:
   - Ejecución secuencial respetando el orden topológico de dependencias de etapas macOS.
   - Evaluación previa obligatoria de invariantes y límites de recursos (RAM $\le 4\text{ GB}$).
   - Medición precisa de duración en milisegundos por etapa y global.
   - Marcado de etapas omitidas (`CiMacOsStepStatus::Omitted`) ante activación de `failFast`.
3. **Cancelación cooperativa y política Zero-Zombie**:
   - Verificación atómica periódica de `cancel->isCancelled()` antes y durante cada etapa.
   - Cese inmediato de procesamiento ante cancelación y emisión de reporte con estado `Cancelled`.
   - Garantía de liberación de recursos sin procesos zombis colgados.
4. **Saneamiento seguro de directorios con resiliencia macOS**:
   - Limpieza de directorio de compilación validando `isSafeBuildPath`.
   - Reintentos con retroceso exponencial ($50\text{ ms} \times 2^i$) ante bloqueos temporales de archivos.
5. **Síntesis CLI y UX Copy público**:
   - Generación determinista de líneas de comando CLI para las 8 etapas canónicas considerando herramientas nativas de macOS.
   - Conversión de estados y generación de sugerencias amigables en español neutro cumpliendo estrictamente `docs/UX-COPY-PUBLICO.md`.
6. **Batería de pruebas unitarias exhaustivas en CTest**:
   - Suite `vigilio_ci_macos_job_service_tests` en `tests/unit/CiMacOsJobServiceTests.cpp` con 12 pruebas trazables (`TEST-CI-MAC-SRV-001` a `TEST-CI-MAC-SRV-012`).

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| Servicio de CI macOS | Inexistente | `vigilio::CiMacOsJobService` declarado e implementado en `vigilio_core` | Crear `CiMacOsJobService.h` y `CiMacOsJobService.cpp` con orquestación, fail-fast, cancelación y saneamiento |
| Sincronización multihilo | Sin sincronización | `std::mutex` protegiendo `plan_` y `options_` | Implementar getters y setters seguros contra condiciones de carrera |
| Modo Dry-Run determinista | Sin simulación | Ejecución virtualizada de las 8 etapas midiendo tiempos reales | Implementar `executeStepInternal` con simulación libre de efectos colaterales destructivos |
| Cancelación Zero-Zombie | Sin soporte en servicio | Monitoreo cooperativo continuo de `CancellationToken` | Comprobar `isCancelled()` en cada etapa y abortar inmediatamente sin procesos zombis |
| Limpieza resiliente | Sin saneamiento | `cleanBuildDirectory` con validación APFS y reintentos exponenciales | Implementar eliminación segura con retroceso exponencial ante locks de archivo |
| Síntesis CLI macOS | Sin generador CLI | `buildCommandLine` determinista para herramientas macOS | Implementar mapeo de etapas a comandos de CMake, Ninja, Apple Clang, CTest y bundle `.app` |
| Pruebas unitarias operativas | 0 pruebas de servicio | 12 pruebas unitarias en `CiMacOsJobServiceTests.cpp` | Implementar `TEST-CI-MAC-SRV-001` a `TEST-CI-MAC-SRV-012` en CTest |
| Build System | Target de test no configurado | `vigilio_ci_macos_job_service_tests` en `CMakeLists.txt` | Registrar fuentes en `vigilio_core` y agregar test en `CMakeLists.txt` |

---

## Compatibility, migration and rollback

### Compatibilidad y estándares C++20

- **Compiladores soportados**:
  - Apple Clang 15.0 o superior en macOS con `-std=c++20`.
  - Homebrew Clang 18 o superior en macOS.
  - GCC 13+ y Clang 18+ en Linux para el arnés multiplataforma de `vigilio_core`.
  - MSVC 2022 v19.38 o superior en Windows.
- **Portabilidad de rutas y archivos**:
  - Utilización exclusiva de `std::filesystem::path` con normalización de separadores (`/`).
  - Detección y rechazo de rutas host de macOS (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`) y artefactos (`.DS_Store`, `._*`).

### Estrategia de migración

- La incorporación de `CiMacOsJobService` es puramente aditiva dentro de `src/core/`.
- No modifica APIs públicas preexistentes ni contratos de módulos vecinos.
- Las pruebas existentes (`24/24`) permanecen intactas y continúan ejecutándose exitosamente.

### Plan de contingencia y rollback

- Si la implementación introduce inestabilidad o fallos en CTest, el rollback se realiza de manera atómica revirtiendo los archivos nuevos en `src/core/` y `tests/unit/`, y desregistrando el target en `CMakeLists.txt`.
- No hay persistencia de datos en bases de datos ni migraciones de esquema asociadas a esta capacidad de CI.

---

## Arquitectura técnica de CI macOS job (implementation)

### Diagrama de componentes y flujo operativo

```mermaid
flowchart TD
    subgraph Dominio macOS CI
        Types["CiMacOsJobTypes.h<br/>(Enums, structs inmutables, Token)"]
        Config["CiMacOsJobConfiguration<br/>(Planes canónicos, Invariantes)"]
        Validator["CiMacOsJobValidator<br/>(Validación plan, rutas y UX copy)"]
    end

    subgraph Capa Operativa de Servicio (Target: vigilio_core)
        Service["CiMacOsJobService<br/>(Orquestador de pipeline multihilo)"]
        Mutex["std::mutex<br/>(Protección de estado interno)"]
        Executor["executeStepInternal<br/>(Simulación / Dry-Run / Live)"]
        Cleaner["cleanBuildDirectory<br/>(Saneamiento APFS con reintentos)"]
        CLISynth["buildCommandLine<br/>(Síntesis determinista para macOS)"]
    end

    subgraph Clientes y Consumidores
        TestRunner["Arnés de CTest<br/>(CiMacOsJobServiceTests.cpp)"]
        FutureUI["CiMacOsJobWidget<br/>(Etapa ui - task0059)"]
        FutureE2E["Tests de integración E2E<br/>(Etapa tests - task0060)"]
    end

    Types --> Service
    Config --> Service
    Validator --> Service
    Service --- Mutex
    Service --> Executor
    Service --> Cleaner
    Service --> CLISynth

    TestRunner --> Service
    FutureUI -.-> Service
    FutureE2E -.-> Service
```

### Modelo de servicio y ciclo de vida de CiMacOsJobService

`CiMacOsJobService` encapsula el estado mutable (`plan_` y `options_`) detrás de una interfaz protegida por un cerrojo `std::mutex`. Su ciclo de vida es el siguiente:
1. **Construcción**: Se instancia con el plan canónico de depuración `canonicalDebugPlan()` o con un plan inyectado explícitamente. Las opciones por defecto establecen `mode = DryRun`, `failFast = true`, `maxRetries = 3`, `retryBackoffMs = 50` y `captureOutput = true`.
2. **Reconfiguración**: Mediante `setPlan` y `setOptions`, el cliente puede alterar el plan o los parámetros operativos con garantía de seguridad de hilos.
3. **Ejecución del pipeline**: `executePlan(cancelToken)` ejecuta el flujo completo, generando un `CiMacOsJobReport` inmutable con el resultado acumulado.
4. **Ejecución granular**: `executeStep(step, cancelToken)` evalúa una etapa aislada para pruebas dirigidas o diagnósticos.
5. **Limpieza de disco**: `cleanBuildDirectory(errorMessage)` purga el directorio de compilación validando previamente la seguridad de la ruta.

### Orquestación de pipeline y política fail-fast

El algoritmo de orquestación en `executePlan` opera en cinco fases:
1. **Inspección de cancelación inicial**: Si `cancelToken` ya está cancelado, retorna inmediatamente con estado `Cancelled`.
2. **Validación previa de invariantes**: Ejecuta `CiMacOsJobValidator::validatePlan(plan)`. Si el plan no es válido o si la memoria supera los 4 GB (`maxMemoryBytes > 4294967296ULL`), aborta el pipeline retornando estado `Failed` con la pista de remediación generada por `generateRemediationHint`.
3. **Comprobación de tiempo límite global**: Evalúa si `totalTimeoutSeconds` fue excedido. Si es así, marca el reporte como `TimedOut` y omite todas las etapas restantes.
4. **Recorrido secuencial de etapas**:
   - Para cada etapa definida en el plan, evalúa si `cancelToken->isCancelled()`. Si se cancela en vuelo, omite las etapas restantes y marca el trabajo como `Cancelled`.
   - Ejecuta la etapa llamando a `executeStepInternal`.
   - Si la etapa resulta en `Failure`:
     - Si la etapa es mandatoria y `failFast == true`, interrumpe el pipeline y marca todas las etapas siguientes como `CiMacOsStepStatus::Omitted`.
     - Si la etapa tiene `continueOnError == true` o no es mandatoria, acumula el fallo pero permite continuar con las etapas posteriores.
5. **Consolidación final**: Computa la duración total mediante `std::chrono::steady_clock`, determina si el estado final es `Passed` o `Failed`, y retorna el `CiMacOsJobReport` inmutable.

### Cancelación cooperativa multihilo Zero-Zombie

Bajo la política Zero-Zombie, el servicio implementa cancelación cooperativa sin recurrir a terminación forzosa de hilos:
- Estructura atómica `CancellationToken` con `std::atomic<bool> cancelled{false}`.
- Comprobación previa a cada etapa: antes de iniciar cualquier operación que consuma recursos o invoque herramientas externas, se inspecciona `isCancelled()`.
- Comprobación durante operaciones simuladas: en modo `Simulated`, se realiza una espera fraccionada (e.g. bucle de 4 iteraciones de 5 ms) evaluando `isCancelled()` en cada ciclo para permitir interrupción en vuelo inmediata.
- Marcado determinista: ante cancelación, las etapas no iniciadas se marcan con `status = Omitted`, `exitCode = -1`, `errorMessage = "Operación cancelada."`, garantizando que ningún subproceso quede desatendido esperando EOF.

### Saneamiento seguro de directorios y defensas macOS

La operación `cleanBuildDirectory` salvaguarda la integridad del sistema operativo anfitrión:
1. **Validación estricta de seguridad**: Invocación de `CiMacOsJobValidator::isSafeBuildPath(targetDir)`:
   - Rechazo de secuencias `..` o componentes vacíos.
   - Rechazo de rutas absolutas del host (`/System`, `/Library`, `/Applications`, `/private`, `/usr`, `/Volumes`).
   - Rechazo de nombres de archivo ocultos del sistema de archivos macOS (`.DS_Store`, bifurcaciones AppleDouble `._*`).
2. **Resiliencia ante contención**: Si el directorio existe, ejecuta `std::filesystem::remove_all(targetDir, ec)` dentro de un bucle de reintentos (`maxRetries`). En caso de error de contención de archivo, aplica una pausa exponencial ($50\text{ ms} \times 2^i$) antes del siguiente intento.

### Síntesis determinista de argumentos CLI para macOS

El método `buildCommandLine` genera líneas de comandos canónicas para las ocho etapas del pipeline en macOS:
- `EnvironmentCheck`: `"cmake --version && ninja --version && clang --version"`
- `FormatVerification`: `"find apps/vigilio-browser/src apps/vigilio-browser/tests -name '*.cpp' -o -name '*.h' | while read -r f; do clang-format --dry-run --Werror \"$f\"; done"`
- `StaticAnalysis`: `"clang-tidy -p " + buildDirStr + " apps/vigilio-browser/src/**/*.cpp"`
- `ConfigurePreset`: `"cmake --preset " + presetNameStr + " -B " + buildDirStr`
- `BuildCompilation`: `"cmake --build " + buildDirStr + " --parallel"`
- `TestSuiteExecution`: `"ctest --test-dir " + buildDirStr + " --output-on-failure -L fast --timeout 15"`
- `SanitizerVerification`: `"ctest --test-dir " + buildDirStr + " --output-on-failure -L sanitizer"`
- `BundleVerification`: `"test -d " + buildDirStr + "/VigilioBrowser.app/Contents/MacOS && test -f " + buildDirStr + "/VigilioBrowser.app/Contents/Info.plist"`

### Contrato de UX Copy público conforme a docs/UX-COPY-PUBLICO.md

- Los métodos `executionModeToString` y `formatStepSummary` generan textos claros y accesibles en español neutro.
- Se prohíbe terminantemente la presencia de jerga técnica («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en salidas visibles o mensajes de error.
- Las sugerencias de remediación generadas por `CiMacOsJobValidator::generateRemediationHint` guían al usuario hacia acciones correctivas comprensibles sin tecnicismos innecesarios.

### Concurrencia y sincronización de estado

- El estado interno de `CiMacOsJobService` está compuesto por `CiMacOsJobPlan plan_` y `CiMacOsServiceOptions options_`.
- El acceso concurrente se serializa mediante `mutable std::mutex mutex_`.
- Todos los métodos públicos que leen o mutan este estado adquieren un cerrojo `std::lock_guard<std::mutex>`, garantizando ausencia de carreras de datos (`data races`) en arneses multihilo de CTest.
