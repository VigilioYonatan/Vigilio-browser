# Change Proposal — module-foundations-task0050

Pack: `module.foundations.task0050`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **CI Linux job** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0050 — Implementar la operación en una capa propietaria: CI Linux job`).

Habiéndose completado y certificado formalmente en las capacidades e hitos precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de diagnósticos y advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política integral de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la configuración canónica y servicio de Clang-Format (`task0041`-`task0044`), la configuración canónica y servicio de Clang-Tidy (`task0045`-`task0048`), y la formalización del modelo de dominio, tipos inmutables, validador estático de orden topológico y planes canónicos para CI Linux (`task0049`), esta propuesta formaliza la **implementación de la capa operativa de servicio propietaria (`vigilio::CiLinuxJobService`), el orquestador de pipeline de compilación y verificación en Linux, la ejecución determinista en modo simulación/dry-run y en vivo, la cancelación cooperativa multihilo bajo la política Zero-Zombie, el mecanismo de corte temprano ante fallos (fail-fast), la tolerancia selectiva a errores, la síntesis reproducible de líneas de comando CLI, la gestión y saneamiento seguro del directorio de compilación, y el control estricto de presupuestos de tiempo y memoria RAM en C++20**.

El objetivo central de esta etapa es dotar a la biblioteca `vigilio_core` de un servicio desacoplado, seguro y robusto en `src/core/` que orqueste la ejecución ordenada de las etapas canónicas del trabajo de CI en Linux (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`), garantice el cumplimiento del orden de dependencias definido por el dominio, soporte cancelación cooperativa inmediata sin procesos zombis colgados indefinidamente, permita la inspección y prueba en modo seco (dry-run) sin requerir llamadas externas destructivas en CTest, ofrezca saneamiento seguro del directorio de compilación previniendo Directory Traversal (`..`), y reporte métricas consolidadas en `CiLinuxJobReport` con recomendaciones en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de negocio a los widgets de interfaz gráfica ni añadir dependencias hacia WebEngine o `BrowserWindow`**.

El alcance concreto comprende:
- **Capa operativa de servicio desacoplada (`vigilio::CiLinuxJobService`)**:
  - Inicialización flexible y configuración: construcción con plan canónico debug por defecto (`canonicalDebugPlan`), plan release (`canonicalReleasePlan`) o inyección de un plan personalizado validado.
  - Gestión de opciones de servicio (`CiLinuxServiceOptions`): modo de ejecución (`DryRun`, `Simulated`, `Live`), reintentos para operaciones de disco, capturas de salida y política de corte inmediato (`failFast`).
  - Orquestación de pipeline completa (`executePlan`):
    - Verificación previa obligatoria mediante `CiLinuxJobValidator::validatePlan` y `CiLinuxJobValidator::isSafeBuildPath`.
    - Ejecución secuencial de etapas respetando el orden topológico de dependencias.
    - Medición de tiempos de ejecución de alta precisión con `std::chrono::steady_clock` para cada etapa individual y para el total del trabajo.
    - Manejo de fallos con política `failFast`: al fallar una etapa obligatoria, se omiten ordenadamente las subsiguientes (`CiLinuxStepStatus::Omitted`).
    - Soporte para `continueOnError`: etapas marcadas con esta opción permiten continuar el pipeline aun cuando fallen, acumulando el resultado.
    - Consolidación del resultado global en `CiLinuxJobReport` (estados `Passed`, `Failed`, `Cancelled`, `TimedOut`).
  - Ejecución granular por etapa individual (`executeStep`): evaluación aislada de una etapa específica para arneses de diagnóstico o reintentos dirigidos.
  - Soporte de **cancelación cooperativa multihilo (Zero-Zombie)** mediante `CancellationToken*`:
    - Comprobación atómica previa a cada etapa y durante operaciones prolongadas.
    - Interrupción segura e inmediata ante solicitud externa, marcando el estado global como `CiLinuxJobStatus::Cancelled`.
    - Garantía absoluta de cero subprocesos huérfanos o colgados esperando EOF.
  - Saneamiento y gestión segura de directorios (`cleanBuildDirectory`):
    - Eliminación recursiva y segura del directorio de construcción configurado.
    - Validación rigurosa de seguridad de ruta (`isSafeBuildPath`) impidiendo el borrado de rutas de sistema (`/`, `/usr`, `/etc`) o vectores de Directory Traversal (`..`).
    - Resiliencia con reintentos exponenciales ante contención de bloqueos de archivo transitorios.
  - Síntesis determinista y reproducible de comandos CLI (`buildCommandLine`):
    - Generación exacta de comandos para cada etapa del pipeline Linux (`cmake --preset ...`, `cmake --build ...`, `ctest --test-dir ...`, `clang-format ...`, `clang-tidy ...`, `cpack ...`).
  - Monitoreo de límites de recursos y presupuestos:
    - Verificación de presupuesto de memoria RAM (máximo 4 GB según `maxMemoryBytes`).
    - Control de tiempo límite global (`totalTimeoutSeconds`) y por etapa individual (`timeoutMs`).
- **Ampliación de tipos y contratos en `CiLinuxJobTypes.h`**:
  - `CiLinuxExecutionMode`: Modos de ejecución (`DryRun`, `Simulated`, `Live`).
  - `CiLinuxServiceOptions`: Opciones operativas del servicio (`mode = DryRun`, `failFast = true`, `maxRetries = 3`, `retryBackoffMs = 50`, `captureOutput = true`).
- **Invariantes constitucionales de calidad, portabilidad y privacidad**:
  - *Local-First Constitucional (ADR-001 / Decisión Q-001)*: Cero conexiones de red, cero servicios cloud, cero telemetría remota.
  - *Aislamiento modular y cero acoplamiento de GUI*: La lógica reside exclusivamente en `vigilio_core`, ejecutable en entornos headless de CTest sin requerir servidor X11/Wayland ni `QApplication`.
  - *Seguridad de rutas*: Confinamiento en rutas relativas de compilación y validación previa de seguridad.
- **Suite completa de pruebas unitarias del servicio (`tests/unit/CiLinuxJobServiceTests.cpp`)**:
  - 12 pruebas unitarias deterministas en CTest (`TEST-CI-LINUX-SRV-001` a `TEST-CI-LINUX-SRV-012`) cubriendo inicialización, ejecución de pipeline en dry-run, política fail-fast, tolerancia con `continueOnError`, cancelación cooperativa Zero-Zombie, saneamiento seguro de directorios y rechazo de traversal, ejecución de etapa individual, síntesis de línea de comandos CLI, límites de recursos y memoria, control de timeouts, reintentos con retroceso exponencial y auditoría de UX copy en español neutro.

---

## Out of scope

- Implementación de widgets de interfaz gráfica, ventanas de visualización de CI, barras de progreso o botones en la interfaz de usuario de Vigilio Browser (reservado para la etapa `ui` en `module.foundations.task0051`).
- Pruebas pesadas de integración extremo a extremo multihilo y ejecución real de toolchains en pipelines distribuidos (reservado para la etapa `tests` en `module.foundations.task0052`).
- Modificación directa de código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante la fase APPLY tras aprobación humana).
- Descarga o instalación automatizada de paquetes del sistema operativo mediante `apt`, `dnf` o `pacman`.
- Conexión a servicios externos de CI/CD (GitHub Actions, GitLab CI, Jenkins) o almacenamiento de telemetría en la nube.

---

## Contexto y evidencia

1. **Estado del módulo foundations y capacidad CI Linux job**:
   - En la etapa anterior (`task0049`, commit `0cb3f59`), se formalizó el modelo de dominio en `CiLinuxJobTypes.h`, `CiLinuxJobConfiguration.h/.cpp`, `CiLinuxJobValidator.h/.cpp` y la suite de pruebas `CiLinuxJobConfigurationTests.cpp` (11 pruebas pasando exitosamente).
   - El pipeline actual de verificación continua (`apps/vigilio-browser/scripts/check.sh`) compila limpiamente con C++20 y `-Werror`, ejecutando 20/20 pruebas con 100% de éxito en 2.77 segundos.
2. **Preset canónico de CI Linux en el repositorio**:
   - `apps/vigilio-browser/CMakePresets.json` define formalmente el preset canónico `ci-linux-debug`:
     - Generador: `Ninja`.
     - Tipo de construcción: `Debug`.
     - Flags: `VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `VIGILIO_WARNINGS_AS_ERRORS=ON`.
     - Directorio de construcción: `${sourceDir}/build-ci-debug`.
3. **Patrón de servicio consolidado en el proyecto**:
   - Los servicios propietarios precedentes (`CMakePresetsService`, `TestFixtureService`, `ClangFormatService` y `ClangTidyService`) han demostrado una arquitectura de alta fiabilidad, basada en C++20 estándar, mutex para protección concurrente, tokens de cancelación cooperativa (`CancellationToken`), soporte para simulación dry-run determinista en CTest, y adhesión estricta a `docs/UX-COPY-PUBLICO.md`.

---

## Alcance exacto

El incremento técnico para la etapa `implementation` de `module.foundations.task0050` abarca:
1. **Modelo de tipos ampliado (`apps/vigilio-browser/src/core/CiLinuxJobTypes.h`)**:
   - Incorporar `enum class CiLinuxExecutionMode : uint8_t { DryRun, Simulated, Live }`.
   - Incorporar `struct CiLinuxServiceOptions` con parámetros de modo, corte rápido (`failFast`), reintentos y capturas.
2. **Declaración del servicio (`apps/vigilio-browser/src/core/CiLinuxJobService.h`)**:
   - Declarar la clase `CiLinuxJobService` en el namespace `vigilio`.
   - Métodos públicos de ciclo de vida: constructores (por defecto con plan canónico debug y con plan explícito), `setPlan`, `plan`, `setOptions`, `options`.
   - Métodos de ejecución y simulación: `executePlan`, `executeStep`.
   - Métodos de gestión de directorio: `cleanBuildDirectory`.
   - Síntesis de comandos: `buildCommandLine`.
   - Métodos de diagnóstico y copy público: `executionModeToString`, `formatStepSummary`.
3. **Implementación del servicio (`apps/vigilio-browser/src/core/CiLinuxJobService.cpp`)**:
   - Implementación en C++20 puro empleando `<chrono>`, `<filesystem>`, `<sstream>`, `<thread>`, `<mutex>`, `<algorithm>`.
   - Lógica de orquestación secuencial evaluando precondiciones mediante `CiLinuxJobValidator`.
   - Simulación y ejecución de etapas con temporización `std::chrono::steady_clock`.
   - Detección inmediata de cancelación cooperativa (`isCancelled`) para cumplimiento de la política Zero-Zombie.
   - Algoritmo de corte temprano (`failFast`) y omisión de etapas dependientes.
   - Saneamiento seguro de directorios con verificación de rutas y reintentos ante bloqueos.
   - Síntesis determinista de argumentos CLI para cada etapa del pipeline.
4. **Batería de pruebas unitarias del servicio (`apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp`)**:
   - 12 pruebas unitarias deterministas (`TEST-CI-LINUX-SRV-001` a `TEST-CI-LINUX-SRV-012`).
5. **Configuración de CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Añadir `src/core/CiLinuxJobService.cpp` a la biblioteca estática `vigilio_core`.
   - Declarar ejecutable `vigilio_ci_linux_job_service_tests` vinculado con `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar test `ci_linux_job_service_tests` en CTest con labels `unit;fast` y timeout de 15 segundos.
6. **Sincronización documental**:
   - Actualizar `docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md` para reflejar el estado completado de la etapa `implementation`.

---

## Diseño completo de la capacidad

### 1. Entidades de datos y responsabilidades

- **`CiLinuxJobService`**: Entidad operativa propietaria responsable de coordinar la planificación, validación previa, ejecución acotada de etapas de CI en Linux, cancelación cooperativa multihilo, saneamiento seguro de directorios y síntesis CLI.
- **`CiLinuxExecutionMode`**: Enumeración fuertemente tipada que distingue entre ejecución simulada/seca (`DryRun`, `Simulated`) para arneses de pruebas y ejecución real (`Live`).
- **`CiLinuxServiceOptions`**: Parámetros inmutables que configuran el modo operativo, tolerancia a fallos, reintentos y capturas de salida.
- **`CiLinuxJobPlan`**: Estructura inmutable que encapsula el conjunto ordenado de etapas, directivas de compilación, preset de CMake y restricciones de recursos.
- **`CiLinuxJobReport`**: Estructura inmutable consolidada que contiene los resultados individuales de cada etapa, estado final y recomendaciones de remediación.
- **`CancellationToken`**: Token atómico multihilo que permite abortar la ejecución del trabajo de manera cooperativa y determinista.

### 2. Contrato funcional

- **Actor / Invocación**:
  - Hilo principal de la aplicación, hilos trabajadores en segundo plano (*workers*) o el arnés de pruebas de CTest.
- **Entradas**:
  - Plan de trabajo (`CiLinuxJobPlan`) o identificador de etapa individual (`CiLinuxPipelineStep`).
  - Opciones de servicio (`CiLinuxServiceOptions`) y puntero a token de cancelación cooperativa (`CancellationToken*`).
- **Salidas**:
  - `CiLinuxJobReport` conteniendo el veredicto global, duración, desglose por etapa (`CiLinuxStepResult`) y pistas de remediación en caso de fallo.
  - Comandos CLI formateados mediante `buildCommandLine`.
  - Directorios de compilación saneados de manera segura en el sistema de archivos local.
- **Transiciones de estado**:
  - `Pending` $\rightarrow$ `Running` $\rightarrow$ `Passed` / `Failed` / `Cancelled` / `TimedOut`.
- **Precondiciones**:
  - El plan debe ser válido según `CiLinuxJobValidator::validatePlan` (no vacío, orden topológico correcto, ruta segura).
  - El presupuesto de memoria RAM debe ser $\le 4\text{ GB}$.
- **Postcondiciones**:
  - El reporte consolidado refleja el estado final de todas las etapas procesadas u omitidas.
  - Ningún subproceso zombi permanece en ejecución (política Zero-Zombie).
  - El directorio de compilación queda en estado limpio o preparado según las opciones solicitadas.

---

## Archivos afectados

| Archivo | Acción | Propósito |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/CiLinuxJobTypes.h` | Modificación | Incorporar `CiLinuxExecutionMode` y `CiLinuxServiceOptions`. |
| `apps/vigilio-browser/src/core/CiLinuxJobService.h` | Creación | Declaración de la clase `CiLinuxJobService`, firmas públicas, métodos de ejecución, saneamiento y síntesis CLI. |
| `apps/vigilio-browser/src/core/CiLinuxJobService.cpp` | Creación | Implementación en C++20 de la orquestación de etapas, fail-fast, simulación dry-run, cancelación cooperativa, reintentos y UX copy. |
| `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp` | Creación | Batería exhaustiva de 12 pruebas unitarias deterministas en CTest (`TEST-CI-LINUX-SRV-001` a `TEST-CI-LINUX-SRV-012`). |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Incorporar `CiLinuxJobService.cpp` en `vigilio_core`, definir ejecutable de prueba `vigilio_ci_linux_job_service_tests` y registrar test `ci_linux_job_service_tests`. |
| `docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md` | Modificación | Actualizar el estado y evidencia de la microtarea a etapa `implementation` completada. |
| `openspec/changes/module-foundations-task0050/**` | Creación / Refinamiento | Artefactos OpenSpec de la propuesta (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`). |

---

## Contrato UX/UI

- **Impacto visual directo**: **Sin cambio visual directo**. Esta etapa corresponde estrictamente a la infraestructura operativa en `src/core/` (biblioteca headless `vigilio_core`).
- **Preparación para Senior Mode y accesibilidad**:
  - Los textos de diagnóstico, estados de etapas y sugerencias de remediación generados por el servicio cumplen rigurosamente con `docs/UX-COPY-PUBLICO.md`.
  - Prohibición absoluta de jerga interna técnica: no se incluyen términos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain» ni mensajes de depuración crípticos.
  - Todos los textos están redactados en español neutro comprensible para cualquier usuario.
- **Preparación para la etapa UI (`task0051`)**:
  - La interfaz gráfica podrá consumir `CiLinuxJobService` en un hilo de trabajo secundario mediante callbacks o polling periódico, visualizando el avance de cada etapa con iconos SVG Lucide y etiquetas accesibles sin congelar el hilo principal.

---

## Reglas y fallos

- **Matriz de fallos y recuperación**:
  1. *Plan inválido o desordenado (`DependencyOrderingViolation`, `InvalidWorkflowSchema`)*: Se rechaza la ejecución antes de iniciar cualquier etapa; se retorna inmediatamente un reporte con estado `Failed` y la pista de remediación correspondiente.
  2. *Ruta de compilación insegura (`UnsafePipelinePath`)*: Si `isSafeBuildPath` detecta `..` o rutas absolutas peligrosas, se cancela la operación y no se interactúa con el disco.
  3. *Cancelación por el usuario (`Cancelled`)*: Al detectarse `isCancelled() == true`, el servicio aborta de inmediato la ejecución del paso actual, marca las etapas restantes como `CiLinuxStepStatus::Omitted` y retorna `CiLinuxJobStatus::Cancelled`.
  4. *Fallo en etapa obligatoria con `failFast == true`*: La etapa se marca como `Failure`, se interrumpe la secuencia y las etapas siguientes se marcan como `Omitted`.
  5. *Fallo en etapa con `continueOnError == true`*: La etapa se marca como `Failure`, pero el servicio continúa ejecutando las etapas subsiguientes.
  6. *Tiempo límite excedido (`TimeoutExceeded`)*: Si la duración acumulada supera `totalTimeoutSeconds`, el trabajo se interrumpe y se marca como `CiLinuxJobStatus::TimedOut`.
  7. *Bloqueo transitorio al limpiar directorio*: Se aplican hasta `maxRetries` (3) con retroceso exponencial ($50\text{ ms} \times 2^i$); si persiste el bloqueo, se reporta error de disco sin colgar el hilo.

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Todas las operaciones del servicio de CI Linux se ejecutan localmente en la máquina del usuario o del runner.
  - Cero dependencias hacia la nube, cero telemetría y cero comunicación de red.
- **Seguridad en el sistema de archivos**:
  - Verificación sistemática contra Directory Traversal (`..`) antes de cualquier invocación o eliminación de directorios.
  - Operaciones de borrado limitadas estrictamente al subdirectorio relativo de compilación configurado.
- **Aislamiento de memoria y control de recursos**:
  - Verificación del presupuesto de memoria RAM (máximo 4 GB).
  - No se instancian subprocesos de Chromium WebEngine ni se interactúa con el perfil de usuario o datos privados.

---

## Matriz de pruebas

| ID Prueba | Caso evaluado | Fixture / Entrada | Comportamiento esperado | Fase |
| --- | --- | --- | --- | --- |
| `TEST-CI-LINUX-SRV-001` | Inicialización y configuración del servicio | Plan canónico debug por defecto y plan explícito | Inicializa con estado coherente, plan accesible y opciones predeterminadas | APPLY |
| `TEST-CI-LINUX-SRV-002` | Ejecución de pipeline en modo DryRun | Plan canónico debug de 8 etapas | Ejecuta todas las etapas secuencialmente, genera reporte exitoso y `isPassed() == true` | APPLY |
| `TEST-CI-LINUX-SRV-003` | Política Fail-Fast ante fallo en etapa obligatoria | Plan con etapa `FormatVerification` fallida simulada | Se detiene inmediatamente, etapas posteriores marcadas como `Omitted`, reporte `Failed` | APPLY |
| `TEST-CI-LINUX-SRV-004` | Tolerancia a fallos con `continueOnError` | Plan con etapa opcional configurada con `continueOnError` | Registra fallo en la etapa pero ejecuta exitosamente las etapas siguientes | APPLY |
| `TEST-CI-LINUX-SRV-005` | Cancelación cooperativa multihilo (Zero-Zombie) | Hilo de trabajo con `CancellationToken` activado a mitad de ejecución | Aborta de inmediato, no deja procesos colgados y reporta `CiLinuxJobStatus::Cancelled` | APPLY |
| `TEST-CI-LINUX-SRV-006` | Saneamiento seguro de directorio de compilación | Directorio temporal válido y ruta insegura con `../` | Limpia directorio válido; rechaza terminantemente ruta con traversal sin alterar disco | APPLY |
| `TEST-CI-LINUX-SRV-007` | Ejecución de etapa individual aislada | Invocación de `executeStep` para `ConfigurePreset` | Ejecuta únicamente la etapa solicitada y retorna `CiLinuxStepResult` preciso | APPLY |
| `TEST-CI-LINUX-SRV-008` | Síntesis determinista de argumentos CLI | Opciones de plan canónico y etapas del pipeline | Sintetiza comandos reproducibles para CMake, Ninja, CTest y herramientas C++ | APPLY |
| `TEST-CI-LINUX-SRV-009` | Control de límites de recursos y memoria | Plan con memoria solicitada > 4 GB o etapas vacías | Rechazo previo de ejecución y reporte de error `MemoryLimitExceeded` o esquema inválido | APPLY |
| `TEST-CI-LINUX-SRV-010` | Detección y manejo de tiempo límite (Timeout) | Simulación de tiempo límite global excedido | Interrumpe la orquestación y asigna estado `CiLinuxJobStatus::TimedOut` | APPLY |
| `TEST-CI-LINUX-SRV-011` | Reintentos con retroceso exponencial en operaciones de E/S | Directorio con contención temporal simulada | Aplica reintentos exponenciales ($50\text{ ms} \times 2^i$) y completa la operación | APPLY |
| `TEST-CI-LINUX-SRV-012` | Auditoría de textos públicos y copy en español neutro | Resúmenes de etapas, diagnósticos y sugerencias | Textos 100% en español sin jerga interna conforme a `docs/UX-COPY-PUBLICO.md` | APPLY |

---

## Pasos APPLY

1. **Paso 1**: Extender `apps/vigilio-browser/src/core/CiLinuxJobTypes.h` incorporando `CiLinuxExecutionMode` y `CiLinuxServiceOptions`.
2. **Paso 2**: Crear `apps/vigilio-browser/src/core/CiLinuxJobService.h` declarando la clase `CiLinuxJobService` y sus métodos de ciclo de vida, ejecución, saneamiento y síntesis CLI.
3. **Paso 3**: Crear `apps/vigilio-browser/src/core/CiLinuxJobService.cpp` implementando la lógica C++20 de orquestación, fail-fast, cancelación cooperativa, reintentos con backoff y copy público.
4. **Paso 4**: Crear `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp` con las 12 pruebas unitarias trazables.
5. **Paso 5**: Actualizar `apps/vigilio-browser/CMakeLists.txt` registrando `CiLinuxJobService.cpp` en `vigilio_core`, creando el target `vigilio_ci_linux_job_service_tests` y registrando el test `ci_linux_job_service_tests` en CTest.
6. **Paso 6**: Ejecutar el pipeline de compilación y pruebas con `bash apps/vigilio-browser/scripts/check.sh`, certificando 21/21 pruebas en verde bajo C++20 con `-Werror`.
7. **Paso 7**: Validar referencias canónicas y lockfile mediante Prompt Builder CLI.
8. **Paso 8**: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md`.

---

## Definición de terminado

- [ ] `CiLinuxJobTypes.h` ampliado con tipos operativos y compilando limpiamente sin dependencias de GUI ni WebEngine.
- [ ] `CiLinuxJobService.h` y `CiLinuxJobService.cpp` implementados con arquitectura desacoplada, resiliencia y Zero-Zombie.
- [ ] 12 pruebas unitarias operativas en `CiLinuxJobServiceTests.cpp` ejecutándose y pasando en CTest en menos de 2 segundos.
- [ ] Pipeline `apps/vigilio-browser/scripts/check.sh` pasando al 100% (21/21 tests) con `-Werror` y cero advertencias.
- [ ] `validate-references` y `verify-lock` aprobados con `valid: true`.
- [ ] Documento de microtarea sincronizado y formalizado.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0049` completado y certificado en commit `0cb3f59`.
  - Herramientas locales: compilador C++20 (GCC/Clang), CMake 3.24+, Ninja.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Modificación o borrado accidental de archivos fuera del directorio de construcción durante el saneamiento.
    *Mitigación*: Validación obligatoria con `CiLinuxJobValidator::isSafeBuildPath`, impidiendo terminantemente `..` o rutas absolutas peligrosas.
  - *Riesgo*: Procesos zombis o bucles colgados durante la orquestación.
    *Mitigación*: Verificación periódica obligatoria de `CancellationToken*` antes de cada etapa e iteración, y timeouts explícitos por paso y globales.
  - *Riesgo*: Desbordamiento de memoria por asignaciones masivas en reportes.
    *Mitigación*: Límite estricto de 4 GB (`maxMemoryBytes`) verificado antes de iniciar la ejecución del plan.

---

## Commit propuesto

```
feat(foundations): implementation ci-linux-job
```
