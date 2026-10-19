# Design — module-foundations-task0049

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Presets estáticos en `CMakePresets.json` sin abstracción de dominio en C++20**:
   - `CMakePresets.json` define los perfiles de configuración `ci-linux-debug` y `ci-linux-release`:
     - Generador: `Ninja`.
     - Flags: `CMAKE_BUILD_TYPE=Debug` / `Release`, `VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `VIGILIO_WARNINGS_AS_ERRORS=ON`.
     - Directorios de salida: `build-ci-debug` y `build-ci-release`.
   - Sin embargo, no existe una entidad de dominio en `vigilio_core` que represente el concepto integral de trabajo de integración continua en Linux (`CI Linux job`), sus etapas constitutivas, los requisitos de entorno o el resultado consolidado de ejecución.
2. **Script imperativo en shell Bash (`apps/vigilio-browser/scripts/check.sh`)**:
   - La verificación local depende de un script lineal `check.sh` que ejecuta `cmake`, `ctest`, `clang-format` y `clang-tidy` de manera hardcodeada y sin tolerancia a fallos estructurada ni diagnósticos accesibles.
   - No es posible consultar, simular o validar un plan de CI de forma programática ni desde arneses de prueba unitaria sin invocar procesos del shell del host.
3. **Carencia de validación formal de orden de etapas (DAG / Precedencia)**:
   - No existe un mecanismo en el código C++20 que garantice que las etapas del pipeline se invoquen en el orden estricto de sus dependencias (v. gr., no permitir compilación sin configuración previa, no admitir ejecución de pruebas sin binarios construidos).
4. **Falta de contención y validación de rutas de compilación de CI**:
   - No existe un validador que impida ataques de Directory Traversal (`..`, `../`) o rutas absolutas arbitrarias del sistema operativo al definir directorios de trabajo o de compilación para los trabajos de CI.
5. **Ausencia de catálogo tipado de estados de pipeline y pistas de remediación**:
   - No existen tipos enumerados que segreguen con precisión el estado del trabajo (`CiLinuxJobStatus`), de las etapas (`CiLinuxStepStatus`) o los errores tipados (`CiLinuxJobError`), ni conversores a texto comprensible en español que cumplan con la política de comunicación pública de `docs/UX-COPY-PUBLICO.md`.

---

### Estado objetivo (TARGET)

1. **Modelo de dominio unificado y fuertemente tipado en `vigilio_core` (`CiLinuxJobTypes.h`)**:
   - `CiLinuxPipelineStep`: Enumeración fuertemente tipada que identifica las 8 etapas canónicas (`EnvironmentCheck`, `FormatVerification`, `StaticAnalysis`, `ConfigurePreset`, `BuildCompilation`, `TestSuiteExecution`, `SanitizerVerification`, `ArtifactPackaging`).
   - `CiLinuxJobStatus` y `CiLinuxStepStatus`: Estados exhaustivos del trabajo (`Pending`, `Running`, `Passed`, `Failed`, `Cancelled`, `TimedOut`) y de sus etapas (`NotStarted`, `InProgress`, `Success`, `Failure`, `Omitted`).
   - `CiLinuxJobError`: Catálogo exhaustivo de fallos tipados (`None`, `ToolchainMissing`, `ConfigurationFailed`, `CompilationFailed`, `TestExecutionFailed`, `FormatDiscrepancy`, `StaticAnalysisIssue`, `UnsafePipelinePath`, `TimeoutExceeded`, `InvalidWorkflowSchema`, `DependencyOrderingViolation`, `MemoryLimitExceeded`).
   - Estructuras inmutables con semántica de valor: `CiLinuxEnvironment`, `CiLinuxStepOptions`, `CiLinuxStepDefinition`, `CiLinuxStepResult`, `CiLinuxJobPlan` y `CiLinuxJobReport`.
   - Soporte para cancelación asíncrona mediante `CancellationToken`.
2. **Entidad de configuración canónica e invariantes de dominio (`CiLinuxJobConfiguration`)**:
   - `canonicalDebugPlan()`: Plan canónico predeterminado para integración continua rápida y depuración en Linux x64, asociando el preset `ci-linux-debug`.
   - `canonicalReleasePlan()`: Plan canónico para validación de optimización y empaquetado de producción en Linux x64, asociando el preset `ci-linux-release`.
   - `verifyInvariants(const CiLinuxJobPlan&)`: Verificación estricta de cumplimiento de todas las invariantes de calidad, orden, seguridad y recursos.
   - `isSupportedLinuxDistro(std::string_view)`: Catálogo oficial de distribuciones Linux compatibles (Ubuntu 22.04/24.04, Debian 12, Fedora 39/40, Arch Linux).
   - Presupuestos estandarizados: RAM máxima de 4 GB (`maxMemoryBytes`), timeout global de 600 s y timeout de prueba individual de 15 s.
3. **Validador estático desacoplado y seguro (`CiLinuxJobValidator`)**:
   - `validatePlan(const CiLinuxJobPlan&)`: Verificación formal de validez estructural del plan, tiempos límite y cuotas.
   - `validateStepSequence(const std::vector<CiLinuxPipelineStep>&)`: Comprobación estricta de precedencia lógica y ausencia de violaciones de dependencias entre etapas.
   - `validateEnvironment(const CiLinuxEnvironment&)`: Validación de toolchain mínimo (Linux x86_64/aarch64, CMake >= 3.24, GCC >= 14 o Clang >= 18).
   - `isSafeBuildPath(const std::filesystem::path&)`: Prevención rigurosa de escapes de directorio y rutas peligrosas del sistema.
   - `jobStatusToString`, `jobErrorToString`, `stepToString`, `stepStatusToString`: Generación de texto descriptivo en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
   - `generateRemediationHint(CiLinuxJobError, std::string_view)`: Pistas de remediación claras y orientadas a la acción sin jerga técnica prohibida.
4. **Batería de pruebas unitarias trazables en CTest (`CiLinuxJobConfigurationTests.cpp`)**:
   - Suite `vigilio_ci_linux_job_tests` con 11 pruebas unitarias deterministas en C++20 puro, ejecutables en < 50 ms en modo headless.

---

### Brechas y análisis de delta (GAPS)

| Aspecto | Estado Actual (CURRENT) | Brecha (GAP) | Estado Objetivo (TARGET) |
| --- | --- | --- | --- |
| **Modelo de datos** | Ninguno; scripts de shell aislados | Ausencia de tipos C++20 estructurados | `CiLinuxJobTypes.h` con tipos inmutables, enums y reportes |
| **Configuración canónica** | Presets JSON aislados | Falta de entidad canónica en código | `CiLinuxJobConfiguration` con planes debug y release |
| **Reglas de precedencia** | Implícitas en script Bash | Ausencia de validación lógica de orden | `CiLinuxJobValidator::validateStepSequence` |
| **Seguridad de rutas** | Sin validación de rutas | Vulnerabilidad potencial a path traversal | `isSafeBuildPath` con contención estricta |
| **Validación de entorno** | Búsqueda ad-hoc de binarios (`command -v`) | Sin verificación de versiones mínimas | `validateEnvironment` con comprobación de toolchain C++20 |
| **Mensajes y UX copy** | Ecos en inglés de herramientas | Sin localización ni adhesión a directrices | Funciones puras con copy accesible en español neutro |
| **Pruebas de especificación** | Ninguna para CI Linux | Ausencia de cobertura de dominio en CTest | 11 pruebas unitarias en `CiLinuxJobConfigurationTests.cpp` |

---

## Compatibility, migration and rollback

### Compatibilidad hacia atrás y portabilidad

- **Estándar C++20 estricto**: La implementación utiliza exclusivamente construcciones estándar de C++20 (`std::string_view`, `std::filesystem::path`, `std::vector`, `std::atomic`, `enum class`). No se emplean extensiones del compilador ni bibliotecas no portables.
- **Independencia absoluta de entorno gráfico (Headless)**: El modelo de dominio reside en `vigilio_core`, sin dependencias de `QApplication`, `QWidget`, `QWebEngineView`, ni requerimientos de servidores `DISPLAY` o `WAYLAND_DISPLAY`.
- **Compatibilidad con CMakePresets v6**: Los planes canónicos se sincronizan de manera exacta con las definiciones de `apps/vigilio-browser/CMakePresets.json`, garantizando interoperabilidad inmediata con las herramientas de construcción existentes.
- **Compatibilidad multiplataforma**: Aunque el trabajo modela específicamente la ejecución en entornos Linux, las estructuras y clases de validación compilan limpiamente en cualquier plataforma soportada (Linux, macOS, Windows) bajo GCC, Clang y MSVC.

### Estrategia de migración

- **Adición no destructiva**: Esta propuesta introduce nuevas clases desacopladas en `apps/vigilio-browser/src/core/` sin modificar ni alterar las APIs de las capacidades previas (`CMakePresets`, `ClangFormat`, `ClangTidy`, etc.).
- **Coexistencia pacífica con scripts existentes**: Los scripts actuales (`apps/vigilio-browser/scripts/check.sh`) continuarán funcionando sin interrupciones. En la etapa `implementation` subsiguiente (`task0050`), el nuevo `CiLinuxJobService` proporcionará la alternativa programática superior.

### Plan de Rollback

En caso de detectarse alguna incompatibilidad imprevista durante la fase APPLY:
1. Revertir el commit asociado (`git revert HEAD`).
2. Remover los archivos creados (`CiLinuxJobTypes.h`, `CiLinuxJobConfiguration.*`, `CiLinuxJobValidator.*`, `CiLinuxJobConfigurationTests.cpp`).
3. Restaurar `apps/vigilio-browser/CMakeLists.txt` a su estado previo.
4. Ejecutar `ctest` para confirmar que las 19 pruebas de referencia retornan al 100% de éxito.

---

## Arquitectura y modelo de dominio técnico detallado

### Diagrama de entidades y relaciones

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              vigilio::core                                  │
└─────────────────────────────────────────────────────────────────────────────┘

 ┌───────────────────────────┐         ┌───────────────────────────────┐
 │   CiLinuxPipelineStep     │         │       CiLinuxJobStatus        │
 ├───────────────────────────┤         ├───────────────────────────────┤
 │ EnvironmentCheck          │         │ Pending                       │
 │ FormatVerification        │         │ Running                       │
 │ StaticAnalysis            │         │ Passed                        │
 │ ConfigurePreset           │         │ Failed                        │
 │ BuildCompilation          │         │ Cancelled                     │
 │ TestSuiteExecution        │         │ TimedOut                      │
 │ SanitizerVerification     │         └───────────────────────────────┘
 │ ArtifactPackaging         │
 └─────────────┬─────────────┘
               │ 1..*
               ▼
 ┌───────────────────────────┐         ┌───────────────────────────────┐
 │   CiLinuxStepDefinition   │         │       CiLinuxJobPlan          │
 ├───────────────────────────┤ 1     * ├───────────────────────────────┤
 │ step: CiLinuxPipelineStep │◄────────┤ presetName: std::string       │
 │ name: std::string         │         │ buildDir: std::filesystem     │
 │ command: std::string      │         │ steps: vector<StepDefinition> │
 │ isMandatory: bool         │         │ totalTimeoutSeconds: uint32   │
 │ options: StepOptions      │         │ maxMemoryBytes: uint64        │
 └───────────────────────────┘         │ failFast: bool                │
                                       └───────────────┬───────────────┘
                                                       │ 1
                                                       ▼
 ┌───────────────────────────┐         ┌───────────────────────────────┐
 │   CiLinuxJobValidator     │         │       CiLinuxJobReport        │
 ├───────────────────────────┤         ├───────────────────────────────┤
 │ validatePlan()            │         │ status: CiLinuxJobStatus      │
 │ validateStepSequence()    │         │ planName: std::string         │
 │ validateEnvironment()     │         │ durationMs: uint64            │
 │ isSafeBuildPath()         │         │ stepResults: vector<Result>   │
 │ generateRemediationHint() │         │ isClean: bool                 │
 └───────────────────────────┘         │ remediationHint: std::string  │
                                       └───────────────────────────────┘
```

### Contratos de entidades

#### 1. `CiLinuxJobTypes.h`
- Define la taxonomía completa de enumeraciones y estructuras inmutables.
- Implementa `CancellationToken` para el control de interrupción asíncrona.
- Métodos puros de conversión de cadenas de texto en español neutro, diseñados para cumplir estrictamente con `docs/UX-COPY-PUBLICO.md`:
  - `jobStatusToString`: Transforma `CiLinuxJobStatus::Passed` a «Aprobado con éxito», `Failed` a «Falló con incidencias», `Cancelled` a «Cancelado por el usuario», `TimedOut` a «Tiempo límite excedido».
  - `jobErrorToString`: Transforma `CiLinuxJobError::CompilationFailed` a «Error en la compilación del código», `TestExecutionFailed` a «Una o más pruebas automáticas fallaron», `UnsafePipelinePath` a «Ruta de trabajo no autorizada o insegura», `DependencyOrderingViolation` a «Orden incorrecto en las etapas del trabajo».
  - `stepToString`: Asigna a cada etapa un nombre claro: «Verificación del entorno», «Comprobación de formato de código», «Análisis estático de calidad», «Configuración de compilación», «Compilación del proyecto», «Ejecución de pruebas automatizadas», «Verificación de sanitizadores de memoria», «Validación de empaquetado».

#### 2. `CiLinuxJobConfiguration`
- Clase estática con constructor eliminado (`= delete`).
- `canonicalDebugPlan()`:
  - Preset: `"ci-linux-debug"`.
  - Directorio de compilación: `"build-ci-debug"`.
  - Etapas obligatorias incluidas:
    1. `EnvironmentCheck`
    2. `FormatVerification`
    3. `StaticAnalysis`
    4. `ConfigurePreset`
    5. `BuildCompilation`
    6. `TestSuiteExecution`
  - `totalTimeoutSeconds`: `600` (10 minutos).
  - `maxMemoryBytes`: `4294967296` (4 GB).
  - `failFast`: `true`.
- `canonicalReleasePlan()`:
  - Preset: `"ci-linux-release"`.
  - Directorio de compilación: `"build-ci-release"`.
  - Etapas obligatorias incluidas:
    1. `EnvironmentCheck`
    2. `FormatVerification`
    3. `StaticAnalysis`
    4. `ConfigurePreset`
    5. `BuildCompilation`
    6. `TestSuiteExecution`
    7. `ArtifactPackaging`
  - `totalTimeoutSeconds`: `900` (15 minutos).
  - `maxMemoryBytes`: `4294967296` (4 GB).
  - `failFast`: `true`.
- `verifyInvariants(const CiLinuxJobPlan&)`: Invoca `CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::None`.
- `isSupportedLinuxDistro(std::string_view distro)`: Valida cadenas reconocidas de distribuciones Linux (v. gr., `"ubuntu"`, `"debian"`, `"fedora"`, `"arch"`).

#### 3. `CiLinuxJobValidator`
- Clase estática con constructor eliminado (`= delete`).
- `validatePlan(const CiLinuxJobPlan& plan)`:
  - Comprueba que `plan.steps` no esté vacío (si está vacío, retorna `InvalidWorkflowSchema`).
  - Comprueba que `plan.totalTimeoutSeconds > 0` y `plan.totalTimeoutSeconds <= 3600` (si no, `InvalidWorkflowSchema`).
  - Comprueba que `plan.maxMemoryBytes > 0` y `plan.maxMemoryBytes <= 8589934592ULL` (si no, `MemoryLimitExceeded`).
  - Comprueba que `isSafeBuildPath(plan.buildDir)` retorne `true` (si no, `UnsafePipelinePath`).
  - Extrae el vector ordenado de `CiLinuxPipelineStep` e invoca `validateStepSequence`.
- `validateStepSequence(const std::vector<CiLinuxPipelineStep>& steps)`:
  - Verifica que las etapas aparezcan en un orden topológicamente válido:
    - Si existe `BuildCompilation`, debe estar precedida en algún punto por `ConfigurePreset`.
    - Si existe `TestSuiteExecution`, debe estar precedida por `BuildCompilation`.
    - Si existe `SanitizerVerification`, debe estar precedida por `BuildCompilation`.
    - Si existe `ArtifactPackaging`, debe estar precedida por `TestSuiteExecution` o `BuildCompilation`.
    - Si existen `FormatVerification` o `StaticAnalysis`, deben ubicarse antes de `BuildCompilation` para permitir detección temprana.
  - Si alguna regla se viola, retorna `CiLinuxJobError::DependencyOrderingViolation`.
  - Si no hay violaciones, retorna `CiLinuxJobError::None`.
- `validateEnvironment(const CiLinuxEnvironment& env)`:
  - Exige que `env.architecture == CiLinuxArchitecture::X86_64` o `AArch64`.
  - Exige que si `env.compiler == CiLinuxCompiler::GCC`, su versión principal sea $\ge 14$.
  - Exige que si `env.compiler == CiLinuxCompiler::Clang`, su versión principal sea $\ge 18$.
  - Exige que `env.cmakeMajorVersion >= 3` y `(env.cmakeMajorVersion > 3 || env.cmakeMinorVersion >= 24)`.
  - Exige que `env.totalMemoryBytes >= 2147483648ULL` (al menos 2 GB de RAM para compilación de navegador).
  - Si alguna condición no se cumple, retorna `CiLinuxJobError::ToolchainMissing`.
- `isSafeBuildPath(const std::filesystem::path& path)`:
  - Rechaza rutas vacías.
  - Rechaza rutas que contengan componentes de escape de directorio `..`.
  - Rechaza rutas absolutas que comiencen por la raíz del sistema (`/`) o apunten a rutas críticas (`/etc`, `/usr`, `/var`, `/home`).
  - Exige que la ruta sea relativa y que su primer componente comience por un prefijo de compilación autorizado (`build-ci-`, `_build`, `build-`).
  - Si es segura, retorna `true`; en cualquier otro caso, retorna `false`.
- `generateRemediationHint(CiLinuxJobError error, std::string_view stepName)`:
  - Genera orientación clara en lenguaje natural español para solucionar cada categoría de error (v. gr., actualizar el compilador a GCC 14+, ejecutar primero la configuración de CMakePresets, revisar la sintaxis de rutas de compilación).

---

## Contratos de integración y seguridad

### Límites de confianza y contención

1. **Aislamiento Local-First (ADR-001 / Decisión Q-001)**:
   - Todo el flujo de CI Linux se ejecuta en máquinas de integración continua propias o localmente en el equipo del desarrollador.
   - Queda terminantemente prohibida cualquier comunicación externa no autenticada, subida de telemetría, conexión a servicios analíticos remotos o integración en la nube no aprobada.
2. **Defensas contra inyección de rutas (Path Traversal)**:
   - Todo directorio de compilación y trabajo se normaliza y valida mediante `CiLinuxJobValidator::isSafeBuildPath`.
   - Se mitigan riesgos de sobreescritura accidental o maliciosa del sistema anfitrión restringiendo la salida a carpetas aisladas del proyecto.
3. **Política Zero-Zombie y límites de ejecución**:
   - Cada etapa posee un límite de tiempo finito (`timeoutMs`), y el plan total está acotado por `totalTimeoutSeconds`.
   - Se implementa el canal de cancelación `CancellationToken` para que cualquier señal de aborto detenga inmediatamente los procesos hijos y evite la persistencia de procesos colgados.
4. **Presupuesto de recursos**:
   - Se impone un límite de memoria de 4 GB (`4294967296` bytes) por trabajo de CI Linux.
   - Las pruebas unitarias se ejecutan de manera desacoplada en CTest con un límite de 15 segundos por prueba.

---

## Decisiones técnicas y trade-offs

1. **Decisión: Modelo de dominio desacoplado en C++20 puro en lugar de configuración exclusiva en archivos YAML de CI (e.g. GitHub Actions workflows)**.
   - *Justificación*: Definir el modelo de dominio en C++20 permite validar y certificar los planes de integración continua directamente desde el código del navegador y desde la suite de CTest de forma reproducible en cualquier máquina (desarrollador local, contenedor Docker, runner CI institucional), sin depender de un proveedor de nube específico.
   - *Trade-off*: Requiere mantener sincronizados los archivos de workflow externos con las definiciones de los planes canónicos del código. Esto se resuelve garantizando que los workflows invoquen directamente `CMakePresets.json` y los tests de validación.
2. **Decisión: Precedencia estricta de verificaciones de calidad (Formato y Análisis Estático) antes de la compilación**.
   - *Justificación*: Adoptar la filosofía Fail-Fast. Comprobar formato y análisis estático toma fracciones de segundo (< 2 s), mientras que compilar puede tomar varios minutos. Detectar incidencias tempranas ahorra tiempo y ciclos de CPU.
   - *Trade-off*: Si un desarrollador solo desea probar un cambio rápido sin formatear, el plan estricto de CI lo rechazará. No obstante, el desarrollador puede usar el preset local `core-debug` para iteraciones informales.
3. **Decisión: Uso de español neutro y cumplimiento riguroso de `docs/UX-COPY-PUBLICO.md` en todos los mensajes de diagnóstico**.
   - *Justificación*: El sistema prohíbe exponer conceptos y jerga interna de ingeniería en interfaces o reportes accesibles al usuario. El uso de términos normalizados garantiza consistencia en toda la plataforma.
