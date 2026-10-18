# Change Proposal — module-foundations-task0034

Pack: `module.foundations.task0034`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0034 — Implementar la operación en una capa propietaria: CMake presets`).

Habiendo formalizado en la microtarea precedente (`module.foundations.task0033`) el modelo de dominio, las enumeraciones tipadas (`CMakePresetKind`, `CMakeBuildType`, `CMakeGenerator`, `CMakePresetValidationOutcome`), la estructura inmutable de reporte (`CMakePresetsValidationReport`), el catálogo canónico de 9 perfiles en `apps/vigilio-browser/CMakePresets.json` y el validador estático desacoplado (`CMakePresetsValidator`), esta propuesta proyecta la **implementación física, desacoplada y fuertemente tipada en C++20 de la capa de servicio y adaptador de operaciones de CMake Presets (`vigilio::CMakePresetsService`)**.

El objetivo central de esta etapa es proporcionar una capa operativa en `src/core` que orqueste el descubrimiento de presets, la resolución de jerarquías y dependencias entre presets de configuración, construcción y pruebas, la síntesis determinista de líneas de comandos (`cmake --preset`, `cmake --build --preset`, `ctest --preset`), un ejecutor con soporte para modo simulación (dry-run) y ejecución local con búferes acotados (política Zero-Zombie), soporte para cancelación cooperativa no bloqueante mediante tokens atómicos (`CancellationToken`), operaciones de recuperación y saneamiento del árbol de compilación (`cleanBinaryDir`), y formateo de diagnósticos técnicos conforme a las directrices de `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de negocio a los widgets de interfaz gráfica ni añadir responsabilidades a `BrowserWindow`**.

El alcance concreto de este pack comprende:
- **Capa operativa de servicio en C++20 (`vigilio::CMakePresetsService`)**:
  - Encapsulamiento del ciclo de vida operativo de presets: carga y descubrimiento desde el sistema de archivos, consulta y filtrado por categoría (`Configure`, `Build`, `Test`, `Package`, `Workflow`), resolución de dependencias entre presets (asociación de `buildPresets` y `testPresets` con su `configurePreset` padre).
  - Síntesis determinista de comandos CLI para los generadores e invocaciones de CMake:
    - `buildConfigureCommandLine(presetName, options)`: genera `cmake --preset <name>` incorporando variables de caché, overrides y flags de warnings.
    - `buildBuildCommandLine(presetName, options)`: genera `cmake --build --preset <name>` con targets específicos y control de concurrencia.
    - `buildTestCommandLine(presetName, options)`: genera `ctest --preset <name>` con filtros de etiquetas y verbosidad en caso de fallos.
  - Ejecutor local y de simulación (Dry-Run):
    - Modo `dryRun`: permite validar la viabilidad del comando, comprobar la existencia de herramientas en el host y calcular rutas y parámetros sin alterar el sistema de archivos.
    - Modo de ejecución acotada: control estricto de tiempos de espera (`timeoutMs`, por defecto 60000 ms), límites de memoria para captura de salidas estándar y de error (`maxOutputBytes = 65536`, 64 KB) garantizando la política Zero-Zombie y preservando el presupuesto de RAM (< 2 MB).
- **Mecanismo de cancelación cooperativa (`vigilio::CancellationToken`)**:
  - Estructura liviana basada en `std::atomic<bool>` con semántica de memoria `std::memory_order_relaxed` / `std::memory_order_acquire` que permite abortar operaciones de configuración, construcción o prueba en segundo plano sin dejar procesos colgados.
- **Operaciones de saneamiento y recuperación ante contingencias (`cleanBinaryDir`)**:
  - Operación segura de limpieza del directorio binario asociado a un `configurePreset`:
    - Verifica taxativamente mediante `CMakePresetsValidator::isSafeBinaryDir` que la ruta resida dentro de `${sourceDir}/build-*` o `_build-*` y no contenga secuencias de Directory Traversal (`..`) ni rutas absolutas forzadas.
    - Elimina de forma transaccional archivos de caché corruptos (`CMakeCache.txt`, `CMakeFiles/`) permitiendo reconfiguraciones limpias sin riesgo de afectar directorios críticos del sistema.
- **Tipos de datos y estructuras operativas del servicio (`CMakePresetsTypes.h`)**:
  - `CMakePresetExecutionState`: Enumeración fuertemente tipada (`Idle`, `Validating`, `Configuring`, `Building`, `Testing`, `Completed`, `Failed`, `Cancelled`).
  - `CMakePresetExecutionStatus`: Taxonomía exhaustiva de resultados operativos (`Success`, `ValidationError`, `PresetNotFound`, `BinaryDirUnsafe`, `ToolNotFound`, `ProcessFailed`, `Timeout`, `Cancelled`, `IOError`).
  - `CMakePresetCommandOptions`: Parámetros de ejecución configurables (dryRun, timeoutMs, cleanBuildDirFirst, additionalArgs, environmentOverrides, maxOutputBytes).
  - `CMakePresetExecutionResult`: Reporte inmutable con status, código de salida, línea de comando generada, directorio de trabajo, salidas estándar y de error acotadas, duración en milisegundos y recomendación de recuperación (`remediationHint`).
  - `CMakePresetsSummary`: Resumen cuantitativo y cualitativo de la configuración cargada.
- **Invariantes constitucionales de calidad, portabilidad y privacidad**:
  - *Local-First Constitucional (Decisión Q-001)*: Cero conexión con servicios en la nube, servidores remotos o telemetría; toda la lógica se ejecuta sobre herramientas y binarios locales.
  - *Portabilidad Multiplataforma (Decisión Q-004)*: Generación determinista de rutas normalizadas en Windows, Linux y macOS.
  - *Desacoplamiento Estricto*: `CMakePresetsService` reside íntegramente en `vigilio_core` y no incluye cabeceras de Qt Widgets (`QWidget`, `QMainWindow`) ni de WebEngine (`QWebEngineView`).
- **Suite completa de pruebas unitarias (`tests/unit/CMakePresetsServiceTests.cpp`)**:
  - Pruebas exhaustivas cubriendo los 12 escenarios de la matriz operativa: carga de archivo, filtrado de presets, resolución de jerarquías, síntesis de comandos de configuración, construcción y pruebas, ejecución en modo dry-run, cancelación cooperativa vía token, detección de rutas no seguras, saneamiento de directorio binario, timeout y truncado de búferes.

---

## Out of scope

- Implementación de controles visuales en Qt Widgets, botones de menú, paneles de selección de presets o diálogos de compilación en la interfaz gráfica (reservado para la etapa `ui` en `module.foundations.task0035`).
- Pruebas E2E de compilación completa de la suite de Chromium WebEngine o empaquetado de producción en instaladores de release (reservado para la etapa `tests` en `module.foundations.task0036` y el módulo `release`).
- Modificación directa de archivos de código fuente de producción bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida rigurosamente a `openspec/changes/module-foundations-task0034/**`).
- Cualquier comunicación por red, comprobación de actualizaciones en la nube o telemetría remota (terminantemente prohibido por la política local-first, decisión Q-001).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia y la suite de pruebas:

1. **Especificación de dominio aprobada (`openspec/changes/module-foundations-task0033/`)**:
   - Formalizó la taxonomía de presets (`CMakePresetKind`), los tipos de construcción (`CMakeBuildType`), los generadores compatibles (`CMakeGenerator`) y los códigos de error de validación (`CMakePresetValidationOutcome`).
   - Implementó el validador estático desacoplado `vigilio::CMakePresetsValidator` en `apps/vigilio-browser/src/core/CMakePresetsValidator.h` y `.cpp`.
   - Incorporó el catálogo completo de los 9 perfiles canónicos estandarizados en `apps/vigilio-browser/CMakePresets.json`.
2. **Archivos existentes en `apps/vigilio-browser/src/core/`**:
   - `CMakePresetsTypes.h`: Define los tipos del dominio base (`CMakePresetKind`, `CMakePresetValidationOutcome`, `CMakePresetDefinition`, `CMakePresetsValidationReport`).
   - `CMakePresetsValidator.h` y `.cpp`: Implementa el parser JSON liviano, validación de esquemas v6, verificación de presets obligatorios (`core-debug`, `desktop-debug`), detección de referencias huérfanas y reglas de higiene de rutas (`isSafeBinaryDir`).
   - `ApplicationLifetimeCoordinator.h` y `.cpp`: Coordinador de ciclo de vida con apagado ordenado y política Zero-Zombie.
   - `CrashRecoveryCoordinator.h` y `.cpp`: Coordinador de recuperación ante caídas con transacciones atómicas.
3. **Punto de integración actual del sistema de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Compila la biblioteca estática `vigilio_core`, enlazando `CMakePresetsValidator.cpp`.
   - Define el ejecutable de prueba `vigilio_presets_tests` y registra el test `cmake_presets_tests` en CTest con labels `unit;fast`.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GNU GCC 15.2.0 bajo C++20 con `-Werror` y cero advertencias.
   - 12 de 12 pruebas pasando exitosamente en CTest con 100% de éxito en 0.47s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.06s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.05s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.03s)
     - `resource_qrc_inclusion_tests`: Passed (0.05s)
     - `startup_bootstrap_tests`: Passed (0.05s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.02s)
     - `cmake_presets_tests`: Passed (0.03s)
5. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 34 referencias canónicas validadas con estado `valid: true` (exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0100-module.foundations.task0034.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 resueltas (Q-007 solo bloquea `release.public-release`).
6. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal del navegador.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («CMake», «Presets», «Toolchain», «Prompt Builder», «OpenSpec», «Senior Mode») en interfaces públicas o mensajes visibles al usuario.

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0034`.
- **Capacidad**: `CMake presets`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0034/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0034/proposal.md`
  - `openspec/changes/module-foundations-task0034/design.md`
  - `openspec/changes/module-foundations-task0034/tasks.md`
  - `openspec/changes/module-foundations-task0034/execution-memory.md`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0034/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase PROPOSE.

---

## Diseño completo de la capacidad

### 1. Modelo Operativo y Tipos de Datos del Servicio (`CMakePresetsTypes.h`)

Se amplían las definiciones de tipos inmutables en `CMakePresetsTypes.h` para incorporar las entidades operativas del servicio:

```cpp
namespace vigilio {

/// @brief Estado operativo del flujo de ejecución de presets.
enum class CMakePresetExecutionState : uint8_t {
    Idle = 0,
    Validating = 1,
    Configuring = 2,
    Building = 3,
    Testing = 4,
    Completed = 5,
    Failed = 6,
    Cancelled = 7
};

/// @brief Taxonomía exhaustiva de resultados de ejecución de presets.
enum class CMakePresetExecutionStatus : uint8_t {
    Success = 0,
    ValidationError = 1,
    PresetNotFound = 2,
    BinaryDirUnsafe = 3,
    ToolNotFound = 4,
    ProcessFailed = 5,
    Timeout = 6,
    Cancelled = 7,
    IOError = 8
};

/// @brief Token ligero para soporte de cancelación cooperativa multihilo.
struct CancellationToken {
    std::atomic<bool> cancelled{false};

    void requestCancellation() noexcept {
        cancelled.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled.load(std::memory_order_acquire);
    }
};

/// @brief Opciones configurables para la síntesis y ejecución de comandos de preset.
struct CMakePresetCommandOptions {
    bool dryRun{false};
    uint32_t timeoutMs{60000};
    bool cleanBuildDirFirst{false};
    std::vector<std::string> additionalArgs;
    std::map<std::string, std::string> environmentOverrides;
    std::size_t maxOutputBytes{65536}; // 64 KB límite estricto de buffer
};

/// @brief Resultado inmutable de la ejecución o simulación de un preset.
struct CMakePresetExecutionResult {
    CMakePresetExecutionStatus status{CMakePresetExecutionStatus::Success};
    int exitCode{0};
    std::string commandLine;
    std::string workingDirectory;
    std::string standardOutput;
    std::string standardError;
    uint64_t elapsedMilliseconds{0};
    std::string remediationHint;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == CMakePresetExecutionStatus::Success && exitCode == 0;
    }
};

/// @brief Resumen consolidado del catálogo de presets cargados en el servicio.
struct CMakePresetsSummary {
    int schemaVersion{0};
    std::size_t totalConfigurePresets{0};
    std::size_t totalBuildPresets{0};
    std::size_t totalTestPresets{0};
    std::size_t totalPackagePresets{0};
    std::size_t totalWorkflowPresets{0};
    std::vector<std::string> configurePresetNames;
    std::vector<std::string> buildPresetNames;
    std::vector<std::string> testPresetNames;
};

} // namespace vigilio
```

### 2. Interfaz Pública de la Capa de Servicio (`vigilio::CMakePresetsService`)

Ubicada en `apps/vigilio-browser/src/core/CMakePresetsService.h`, desacoplada de `QApplication` y `QWidget`:

```cpp
namespace vigilio {

class CMakePresetsService {
  public:
    CMakePresetsService() = default;
    ~CMakePresetsService() = default;

    // Carga y validación inicial
    [[nodiscard]] bool loadFromFile(const std::filesystem::path& presetsPath);
    [[nodiscard]] bool loadFromJson(const std::string& jsonContent, const std::filesystem::path& sourceDir = ".");
    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] const CMakePresetsValidationReport& validationReport() const noexcept;
    [[nodiscard]] CMakePresetsSummary summary() const;

    // Consultas y resolución de jerarquías
    [[nodiscard]] std::optional<CMakePresetDefinition> findPreset(std::string_view name, CMakePresetKind kind) const;
    [[nodiscard]] std::vector<CMakePresetDefinition> listPresetsByKind(CMakePresetKind kind) const;
    [[nodiscard]] std::string resolveAssociatedConfigurePreset(std::string_view presetName, CMakePresetKind kind) const;

    // Síntesis determinista de líneas de comando
    [[nodiscard]] std::string buildConfigureCommandLine(std::string_view presetName, const CMakePresetCommandOptions& options = {}) const;
    [[nodiscard]] std::string buildBuildCommandLine(std::string_view presetName, const CMakePresetCommandOptions& options = {}) const;
    [[nodiscard]] std::string buildTestCommandLine(std::string_view presetName, const CMakePresetCommandOptions& options = {}) const;

    // Ejecución y simulación operativa
    [[nodiscard]] CMakePresetExecutionResult executeConfigure(std::string_view presetName, const CMakePresetCommandOptions& options = {}, CancellationToken* cancel = nullptr);
    [[nodiscard]] CMakePresetExecutionResult executeBuild(std::string_view presetName, const CMakePresetCommandOptions& options = {}, CancellationToken* cancel = nullptr);
    [[nodiscard]] CMakePresetExecutionResult executeTest(std::string_view presetName, const CMakePresetCommandOptions& options = {}, CancellationToken* cancel = nullptr);

    // Operaciones de saneamiento y recuperación
    [[nodiscard]] bool cleanBinaryDir(std::string_view configurePresetName, std::string* errorMessage = nullptr);

    // Formateo observable de diagnósticos
    [[nodiscard]] static std::string executionStatusToString(CMakePresetExecutionStatus status);
    [[nodiscard]] static std::string executionStateToString(CMakePresetExecutionState state);

  private:
    std::filesystem::path sourceDirectory_{"."};
    bool isLoaded_{false};
    CMakePresetsValidationReport report_;
    std::vector<CMakePresetDefinition> configurePresets_;
    std::vector<CMakePresetDefinition> buildPresets_;
    std::vector<CMakePresetDefinition> testPresets_;
    std::vector<CMakePresetDefinition> packagePresets_;
    std::vector<CMakePresetDefinition> workflowPresets_;

    [[nodiscard]] std::string expandMacros(const std::string& input) const;
};

} // namespace vigilio
```

### 3. Síntesis Determinista de Comandos y Resolución de Macros

El servicio implementa la expansión determinista de macros estándar de CMake Presets:
- `${sourceDir}`: Se expande a la ruta base canónica del proyecto (`sourceDirectory_`).
- `${presetName}`: Se expande al nombre del preset bajo evaluación.
- Líneas de comando generadas:
  - **Configure**: `cmake --preset <presetName>` (con argumentos adicionales o variables de entorno normalizadas).
  - **Build**: `cmake --build --preset <presetName>`.
  - **Test**: `ctest --preset <presetName>`.

### 4. Modelo de Ejecución Acotada y Política Zero-Zombie

Para garantizar que ningún proceso quede colgado ni consuma memoria indiscriminadamente:
- **Límite de Búfer Bounded (64 KB)**: Las salidas capturadas (`standardOutput`, `standardError`) se truncan a `maxOutputBytes` (65536 bytes) con indicación explícita `[... salida truncada para preservar límites de memoria ...]`.
- **Timeout Finito**: Si la ejecución supera `timeoutMs`, se marca `status = CMakePresetExecutionStatus::Timeout` y se aborta el subproceso.
- **Cancelación Cooperativa**: Antes de cada etapa crítica y durante el sondeo de procesos, se evalúa `cancel->isCancelled()`. Si el token está activo, se interrumpe inmediatamente retornando `CMakePresetExecutionStatus::Cancelled`.
- **Modo Dry-Run**: Permite verificar la validez de los argumentos, los prerrequisitos de directorios y la higiene de rutas sin invocar procesos del sistema ni alterar el disco.

### 5. Recuperación Segura del Árbol de Compilación (`cleanBinaryDir`)

Cuando se detecta un fallo persistente de compilación o variables de caché antagónicas, `cleanBinaryDir` proporciona una acción de recuperación segura:
1. Resuelve la ruta `binaryDir` del `configurePreset` solicitado.
2. Invoca `CMakePresetsValidator::isSafeBinaryDir` para certificar que no haya escapes mediante `..` ni rutas fijas fuera del proyecto.
3. Si la ruta es segura y existe, remueve los archivos de caché (`CMakeCache.txt`, carpeta `CMakeFiles/`), permitiendo un inicio limpio sin comprometer otros directorios.

---

## Archivos afectados

### Durante PROPOSE (etapa actual)
| Archivo | Acción | Justificación |
| :--- | :--- | :--- |
| `openspec/changes/module-foundations-task0034/canonical-reference.yaml` | Crear / Formalizar | Especificación de metadatos de gobernanza OpenSpec para task0034. |
| `openspec/changes/module-foundations-task0034/proposal.md` | Crear / Formalizar | Propuesta técnica integral para la etapa implementation de CMake presets. |
| `openspec/changes/module-foundations-task0034/design.md` | Crear / Formalizar | Documento de diseño arquitectónico con CURRENT/TARGET/GAPS y diagramas Mermaid. |
| `openspec/changes/module-foundations-task0034/tasks.md` | Crear / Formalizar | Tareas atómicas, ordenadas y verificables para la fase APPLY. |
| `openspec/changes/module-foundations-task0034/execution-memory.md` | Crear / Formalizar | Registro de ejecución, skills vinculantes, invariantes y handoff. |

### Durante APPLY (próxima etapa `0101`)
| Archivo | Acción | Justificación |
| :--- | :--- | :--- |
| `apps/vigilio-browser/src/core/CMakePresetsTypes.h` | Modificar | Incorporación de enumeraciones operativas (`CMakePresetExecutionState`, `CMakePresetExecutionStatus`), opciones y estructuras de reporte. |
| `apps/vigilio-browser/src/core/CMakePresetsService.h` | Crear | Declaración de la interfaz pública desacoplada de `CMakePresetsService`. |
| `apps/vigilio-browser/src/core/CMakePresetsService.cpp` | Crear | Implementación de carga, síntesis de comandos, ejecución acotada, cancelación y limpieza. |
| `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp` | Crear | Batería de 12 pruebas unitarias de la capa de servicio. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificar | Incorporación de `CMakePresetsService.cpp` en `vigilio_core` y registro del target `vigilio_presets_service_tests`. |
| `docs/rules/01-modules/foundations/micro/0034-cmake-presets/task.md` | Modificar | Sincronización del estado de ejecución de la microtarea. |

---

## Contrato UX/UI

- **Sin cambio visual directo en la aplicación**: Esta etapa corresponde estrictamente a la infraestructura operativa de compilación, gestión de presets de CMake y servicios desacoplados en `src/core`.
- **Justificación de ausencia visual**: `CMakePresetsService` es un servicio del núcleo headless que opera sobre el toolchain local y scripts de construcción; no manipula ventanas, widgets Qt, menús ni alertas directas en la interfaz del navegador.
- **Conformidad con directrices públicas (`docs/UX-COPY-PUBLICO.md`)**:
  - Las cadenas diagnósticas y métodos de formateo (`executionStatusToString`, `executionStateToString`) utilizan español técnico claro y descriptivo.
  - Se prohíbe taxativamente exponer términos de ingeniería interna («CMake», «Presets», «Toolchain», «Prompt Builder», «OpenSpec», «Senior Mode») en interfaces públicas.
  - La integración visual con la UI se pospone para la etapa `ui` en la microtarea `0035`.

---

## Reglas y fallos

1. **Preset inexistente (`PresetNotFound`)**:
   - Condición: Se solicita la síntesis de comando o ejecución de un preset que no figura en la especificación cargada.
   - Resultado: Retorna `CMakePresetExecutionStatus::PresetNotFound` con `remediationHint` indicando los nombres válidos disponibles.
2. **Especificación inválida (`ValidationError`)**:
   - Condición: El archivo JSON no cumple con la versión de esquema $\ge 3$ o carece de presets canónicos mínimos.
   - Resultado: Retorna `CMakePresetExecutionStatus::ValidationError` y aborta cualquier intento de ejecución.
3. **Ruta binaria insegura (`BinaryDirUnsafe`)**:
   - Condición: El `binaryDir` resuelto contiene Directory Traversal (`..`) o rutas absolutas forzadas fuera del árbol de construcción.
   - Resultado: Retorna `CMakePresetExecutionStatus::BinaryDirUnsafe`; la operación `cleanBinaryDir` se niega categóricamente a borrar nada.
4. **Tiempo de espera superado (`Timeout`)**:
   - Condición: La ejecución de un comando excede `options.timeoutMs`.
   - Resultado: Retorna `CMakePresetExecutionStatus::Timeout`, aborta el proceso inmediatamente y preserva la política Zero-Zombie.
5. **Cancelación solicitada (`Cancelled`)**:
   - Condición: `cancel != nullptr` y `cancel->isCancelled() == true`.
   - Resultado: Retorna `CMakePresetExecutionStatus::Cancelled` de forma inmediata y limpia.
6. **Error de E/S o permisos (`IOError`)**:
   - Condición: Imposibilidad de leer el archivo de presets o denegación de permisos en disco.
   - Resultado: Retorna `CMakePresetExecutionStatus::IOError` sin generar excepciones ni terminar el proceso anómalamente.

---

## Seguridad y privacidad

- **Local-First Constitucional (Decisión Q-001)**: Prohibición absoluta de llamadas a servicios en la nube o telemetría; el servicio opera exclusivamente sobre binarios del host local.
- **Prevención de Ataques de Path Traversal**: Antes de ejecutar o limpiar cualquier directorio, `isSafeBinaryDir` valida rigurosamente que la ruta esté circunscrita a `${sourceDir}/build-*` o `_build-*`.
- **Prevención de Agotamiento de Recursos (Zero-Zombie / Presupuesto de RAM)**:
  - Salidas de procesos acotadas rígidamente a 64 KB (`maxOutputBytes = 65536`).
  - Timeouts finitos no negociables en toda invocación de procesos.
  - Consumo de memoria del servicio en reposo < 2 MB.
- **Aislamiento de Entorno**: Las variables de entorno inyectadas se restringen a las declaradas explícitamente en el preset o en `options.environmentOverrides`, sin exponer variables de sesión privadas del usuario.

---

## Matriz de pruebas

| ID de Prueba | Ámbito | Componente / Fixture | Entrada / Condición | Resultado Esperado | Modo de Fallo Negativo | Ciclo de Vida |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `TEST-PRESET-SVC-001` | Unitario | `CMakePresetsService` / Memoria | Carga de JSON canónico con 9 presets | `loadFromJson == true`, `summary().totalConfigurePresets == 9` | Error falso de parseo en JSON válido | APPLY |
| `TEST-PRESET-SVC-002` | Unitario | `CMakePresetsService` / Archivo | `loadFromFile` apuntando al `CMakePresets.json` real | `isLoaded == true`, `report.isValid == true` | Fallo de lectura del archivo real | APPLY |
| `TEST-PRESET-SVC-003` | Unitario | `CMakePresetsService` / Memoria | Búsqueda de preset existente `desktop-debug` | `findPreset` retorna definición con generador `"Ninja"` | Preset canónico no encontrado | APPLY |
| `TEST-PRESET-SVC-004` | Unitario | `CMakePresetsService` / Memoria | Búsqueda de preset inexistente `fake-preset` | `findPreset` retorna `std::nullopt` | Retorno de preset ficticio | APPLY |
| `TEST-PRESET-SVC-005` | Unitario | `CMakePresetsService` / Memoria | Resolución de `configurePreset` asociado a `desktop-debug` en build presets | Retorna `"desktop-debug"` | Asociación rota entre build y configure | APPLY |
| `TEST-PRESET-SVC-006` | Unitario | `CMakePresetsService` / Memoria | Síntesis de comando configure para `core-debug` | `buildConfigureCommandLine` contiene `cmake --preset core-debug` | Comando malformado o banderas faltantes | APPLY |
| `TEST-PRESET-SVC-007` | Unitario | `CMakePresetsService` / Memoria | Síntesis de comando build para `core-release` | `buildBuildCommandLine` contiene `cmake --build --preset core-release` | Comando build mal estructurado | APPLY |
| `TEST-PRESET-SVC-008` | Unitario | `CMakePresetsService` / Memoria | Síntesis de comando test para `sanitizer-asan` | `buildTestCommandLine` contiene `ctest --preset sanitizer-asan` | Comando ctest con sintaxis incorrecta | APPLY |
| `TEST-PRESET-SVC-009` | Unitario | `CMakePresetsService` / Memoria | Ejecución en modo `dryRun: true` para `core-debug` | `status == Success`, `exitCode == 0`, no toca el disco | Intento de ejecución real en dry-run | APPLY |
| `TEST-PRESET-SVC-010` | Concurrencia | `CMakePresetsService` / Token | Ejecución con `CancellationToken` cancelado previamente | `status == Cancelled`, aborto inmediato (< 1 ms) | Ignorar cancelación cooperativa | APPLY |
| `TEST-PRESET-SVC-011` | Seguridad | `CMakePresetsService` / Memoria | `cleanBinaryDir` sobre preset con ruta insegura (`../../etc`) | Retorna `false`, denegación estricta de borrado | Borrado no autorizado fuera del build tree | APPLY |
| `TEST-PRESET-SVC-012` | Resiliencia | `CMakePresetsService` / Memoria | Limpieza segura de directorio temporal válido `_build-test-clean` | Retorna `true`, elimina caché y preserva integridad | Fallo de limpieza o eliminación recursiva excesiva | APPLY |

---

## Pasos APPLY

Plan secuencial y determinista para la ejecución de la fase APPLY (`module.foundations.task0034` en ejecución `0101`):

1. **Ampliación de tipos operativos en `apps/vigilio-browser/src/core/CMakePresetsTypes.h`**:
   - Declarar `enum class CMakePresetExecutionState` y `enum class CMakePresetExecutionStatus`.
   - Declarar `struct CancellationToken`, `struct CMakePresetCommandOptions`, `struct CMakePresetExecutionResult`, y `struct CMakePresetsSummary`.
2. **Declaración de la clase de servicio en `apps/vigilio-browser/src/core/CMakePresetsService.h`**:
   - Declarar `class CMakePresetsService` en `namespace vigilio` con métodos de carga, resumen, búsqueda, síntesis de comandos, ejecución acotada, cancelación, saneamiento de directorios y formateo textual.
3. **Implementación de la capa de servicio en `apps/vigilio-browser/src/core/CMakePresetsService.cpp`**:
   - Implementar `loadFromFile`, `loadFromJson`, `summary` y helpers de resolución de jerarquías delegando en `CMakePresetsValidator`.
   - Implementar los generadores deterministas de líneas de comando para configure, build y test presets.
   - Implementar el ejecutor con soporte para `dryRun`, control de timeout, truncado de búferes a 64 KB y chequeo de `CancellationToken`.
   - Implementar la operación de saneamiento `cleanBinaryDir` con verificación estricta de `isSafeBinaryDir`.
   - Implementar formateadores `executionStatusToString` y `executionStateToString`.
4. **Implementación de la batería de pruebas unitarias en `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`**:
   - Crear suite de pruebas exhaustiva para validar los 12 escenarios de la matriz (`TEST-PRESET-SVC-001` a `TEST-PRESET-SVC-012`).
5. **Configuración en el Build System (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Incorporar `src/core/CMakePresetsService.cpp` en la biblioteca estática `vigilio_core`.
   - Declarar el ejecutable de prueba `vigilio_presets_service_tests` vinculado a `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar el test `cmake_presets_service_tests` en CTest bajo `LABELS "unit;fast"` con timeout de 15 segundos.
6. **Verificación del Pipeline de Compilación y Calidad**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` garantizando compilación con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests pasando en CTest.
7. **Validación de Gobernanza y Cierre Documental**:
   - Validar referencias canónicas y lockfile con Prompt Builder CLI.
   - Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0034-cmake-presets/task.md`.

---

## Definición de terminado

- [ ] La capa operativa `CMakePresetsService` implementa C++20 fuertemente tipado, modular y completamente desacoplada de `QApplication` y `QWidget`.
- [ ] La síntesis de líneas de comandos produce strings exactos, deterministas y reproducibles para configure, build y test presets.
- [ ] El ejecutor soporta modo `dryRun`, límites estrictos de búfer (64 KB) y cancelación cooperativa no bloqueante vía `CancellationToken`.
- [ ] La operación `cleanBinaryDir` restringe rigurosamente los borrados a directorios seguros y validados dentro de `${sourceDir}/build-*`.
- [ ] Se implementa y pasa al 100% la suite de 12 pruebas unitarias en CTest con `LABELS "unit;fast"`.
- [ ] Cero regresiones en el pipeline continuo (`check.sh` pasa 13/13 tests en < 1s).
- [ ] Cero dependencias de red o servicios en la nube (local-first constitucional, Q-001).
- [ ] Ningún término técnico interno visible al usuario final (cumplimiento de `docs/UX-COPY-PUBLICO.md`).

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0033`: Modelo de dominio, validador de presets y catálogo canónico en `CMakePresets.json`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Procesos colgados o zombies durante la ejecución de comandos de compilación.
    *Mitigación*: Política Zero-Zombie mediante timeout estricto (`timeoutMs`) y cancelación cooperativa multihilo vía `CancellationToken`.
  - *Riesgo*: Borrado accidental de directorios del sistema durante la limpieza de artefactos.
    *Mitigación*: Validación obligatoria con `CMakePresetsValidator::isSafeBinaryDir` antes de ejecutar cualquier eliminación en disco.
  - *Riesgo*: Desbordamiento de memoria por salidas de compilación masivas.
    *Mitigación*: Búferes acotados rígidamente a 64 KB (`maxOutputBytes = 65536`).

---

## Commit propuesto

`feat(foundations): implementation cmake-presets`
