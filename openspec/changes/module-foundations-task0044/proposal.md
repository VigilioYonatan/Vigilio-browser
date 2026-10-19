# Change Proposal — module-foundations-task0044

Pack: `module.foundations.task0044`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **clang-format configuration** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0044 — Probar casos positivos, negativos y recuperación: clang-format configuration`).

Habiéndose formalizado y certificado en las etapas precedentes la especificación del modelo de dominio, invariantes de estilo y validador estático YAML (`task0041`), implementado el servicio operativo `vigilio::ClangFormatService` en C++20 con formateo en memoria, inspección dry-run, respaldos atómicos (`.bak`), reintentos y cancelación cooperativa (`task0042`), e integrado la auditoría de conformidad de estilo y UX copy en el arnés de pruebas de interfaz de usuario (`task0043`), el objetivo esencial de este pack es **diseñar y estructurar la suite exhaustiva, determinista y de alta resiliencia de pruebas automatizadas para clang-format configuration**.

La suite de pruebas proyectada cubre sistemáticamente:
- **Casos positivos y ciclo de vida de configuración y formateo**:
  - Verificación exhaustiva de paridad entre la configuración canónica en memoria (`ClangFormatConfiguration::canonicalConfig()`) y el archivo físico `.clang-format` del repositorio raíz.
  - Validación de formateo en memoria pura sobre múltiples dialectos y extensiones C++ (`.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`), certificando el cumplimiento de las reglas fundamentales (sangría de 4 espacios, límite de columna de 105 caracteres, punteros alineados a la izquierda `Type* var`, prohibición de funciones cortas en una sola línea `AllowShortFunctionsOnASingleLine: None`, estándar C++20).
  - Inspección en modo dry-run de archivos limpios y archivos con discrepancias estilísticas, comprobando que el archivo en disco permanece estrictamente inmutable durante la inspección.
  - Formateo físico sobre archivos temporales aislados, verificando la corrección de indentación, espaciado de operadores y llaves de bloques.
  - Formateo e inspección recursiva de árboles de directorios con múltiples niveles de anidamiento, verificando la discriminación correcta de extensiones soportadas y la exclusión de carpetas ignoradas (`_build`, `build`, `generated`, `.git`, `.prompt-runner`).
  - Serialización y deserialización bidireccional determinista (roundtrip) entre YAML y la estructura fuertemente tipada `ClangFormatOptions`.
- **Casos negativos, entradas inválidas y defensas de seguridad**:
  - Detección y rechazo de documentos YAML malformados, vacíos, truncados o con caracteres nulos/binarios mediante `ClangFormatValidator::validateConfigContent`.
  - Validación de límites estrictos en parámetros numéricos (`indentWidth` en rango $[1, 16]$, `columnLimit` en rango $[20, 500]$), rechazando valores nulos, negativos o desmedidos con códigos específicos `ClangFormatError::InvalidIndentWidth` y `ClangFormatError::InvalidColumnLimit`.
  - Detección y rechazo inmediato de ataques de Directory Traversal (`../`, `../../`, `..\\`) y rutas absolutas peligrosas fuera de la raíz del proyecto (`/`, `/etc`, `/usr`) mediante `ClangFormatValidator::isSafeConfigPath`.
  - Verificación de cuotas de protección de memoria (`maxFileSizeBytes` en `FormatServiceOptions`): rechazo de archivos que superen la cuota configurada (e.g. 10 MB) con estado `FormatExecutionStatus::FileTooLarge` sin agotamiento de memoria del sistema.
  - Manejo robusto de archivos inexistentes o rutas que corresponden a directorios cuando se espera un archivo, retornando limpiamente `FormatExecutionStatus::FileNotFound` o `FormatExecutionStatus::ReadError`.
- **Concurrencia masiva multihilo y sincronización**:
  - Ejecución paralela sobre `ClangFormatService` desde 8 a 16 hilos (`std::thread`) concurrentes:
    - Formateo simultáneo en memoria de múltiples buffers de código con contenidos divergentes, verificando ausencia de interferencias y const-correctness.
    - Inspección concurrente en modo dry-run sobre un mismo archivo compartido y sobre archivos distintos.
    - Formateo físico concurrente sobre archivos aislados en directorios temporales, comprobando la sincronización de acceso interno mediante `std::mutex` y la integridad del estado `isConfigured()`.
- **Cancelación cooperativa inmediata y política Zero-Zombie**:
  - Pruebas de interrupción inmediata de operaciones de inspección y formateo masivo de directorios ante la activación de `CancellationToken`.
  - Verificación de retorno atómico con estado `FormatExecutionStatus::Cancelled` y emisión del mensaje de remediación en lenguaje natural.
  - Garantía de ausencia total de procesos zombis, liberación inmediata de descriptores de archivo y saneamiento de copias temporales.
- **Recuperación ante fallos de disco, permisos y contención transitoria**:
  - Simulación de fallos de escritura por permisos denegados (archivos con permisos de sólo lectura `std::filesystem::perms::owner_read`), certificando la transición limpia a `FormatExecutionStatus::PermissionDenied` o `WriteError`.
  - Verificación del mecanismo atómico de respaldo (`createBackup`) y reversión (`rollbackFile`): ante fallo de escritura o formateo abortado, el archivo original es restaurado fielmente desde su copia `.bak` sin corrupción.
  - Validación del mecanismo de reintentos con retroceso exponencial (`formatWithRetry`): prueba con bloqueo transitorio simulado donde el archivo se libera tras reintentos, logrando éxito sin excepciones no capturadas.
  - Idempotencia de rollback y limpieza segura de copias de seguridad (`removeBackup`).
- **Desacoplamiento arquitectónico y tolerancia headless**:
  - Certificación de ejecución 100% autónoma en CTest (< 500 ms en total) sin requerir `QApplication`, `DISPLAY`, `WAYLAND_DISPLAY` ni procesos WebEngine/Chromium.
  - Aislamiento absoluto y política Local-First constitucional (ADR-001 / Decisión Q-001): cero sockets de red externa, cero peticiones cloud y total privacidad de datos.
- **Auditoría exhaustiva de textos públicos (UX Copy)**:
  - Verificación programática de que todas las cadenas de diagnóstico y remediación generadas por `ClangFormatService::executionStatusToString` y `ClangFormatValidator::formatErrorToString` emplean español neutro claro y accesible, libres de términos técnicos prohibidos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»), conforme a `docs/UX-COPY-PUBLICO.md`.

---

## Out of scope

- Modificación de la interfaz gráfica de usuario de producción (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de controles visuales, botones de formateo o menús de linters en la ventana de navegación (proscrito taxativamente por `docs/UX-COPY-PUBLICO.md` y por la responsabilidad del módulo `foundations`).
- Modificación de los contratos de dominio consolidados en `ClangFormatTypes.h`, `ClangFormatValidator.h` o `ClangFormatService.h` durante PROPOSE (la propuesta define pruebas sobre las APIs estables existentes).
- Invocación de procesos pesados de Qt WebEngine o Chromium durante la ejecución de las suites de pruebas unitarias o de integración de formateo.
- Conexión a redes externas, telemetría o servicios cloud de análisis de código (estrictamente vetado por ADR-001 / Decisión Q-001).
- Modificación de código fuente fuera de las rutas autorizadas para esta etapa.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Dominio desacoplado (`apps/vigilio-browser/src/core/`)**:
   - `ClangFormatTypes.h`: Define enumeraciones `ClangFormatStylePreset`, `PointerAlignmentPolicy`, `ShortFunctionPolicy`, `ClangFormatValidationState`, `ClangFormatError`, y `FormatExecutionStatus` fuertemente tipado. Incluye las estructuras `ClangFormatOptions`, `ClangFormatValidationReport`, `SourceFileFormatInspection`, `FormatServiceOptions`, `FormatOperationResult` y el token ligero de cancelación cooperativa multihilo `CancellationToken`.
   - `ClangFormatValidator.h` / `.cpp`: Proporciona validación estática de opciones numéricas, prevención de Directory Traversal (`isSafeConfigPath`), parser de sintaxis YAML, serialización a formato Clang-Format YAML y conversión de errores a texto descriptivo (`formatErrorToString`).
   - `ClangFormatConfiguration.h` / `.cpp`: Genera la configuración canónica inmutable del proyecto (LLVM, 4 espacios, 105 columnas, punteros a la izquierda, C++20), valida extensiones soportadas (`.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`) y exclusiones de carpetas (`_build`, `build`, `generated`, `.git`, `.prompt-runner`).
   - `ClangFormatService.h` / `.cpp`: Proporciona la capa operativa completa con formateo en memoria pura (`formatSourceCode`), inspección de cadenas (`inspectSource`), inspección dry-run en disco (`inspectFile`, `inspectDirectory`), formateo físico con respaldo (`formatFile`, `formatDirectory`), formateo con retroceso exponencial (`formatWithRetry`), gestión de backups (`createBackup`, `rollbackFile`, `removeBackup`), síntesis CLI (`buildClangFormatCommandLine`) y mensajes públicos de estado (`executionStatusToString`).
2. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/ClangFormatConfigurationTests.cpp`: 10 pruebas unitarias evaluando configuración canónica, límites básicos de indentación y columnas, parser YAML canónico, roundtrip básico, omisión de claves obligatorias, rechazo de rutas inseguras, extensiones/exclusiones y textos públicos.
   - `tests/unit/ClangFormatServiceTests.cpp`: 12 pruebas unitarias evaluando inicialización, formateo en memoria, dry-run conforme, dry-run con discrepancia, formateo físico, backup y rollback, retry con backoff, cancelación cooperativa, rechazo por cuota de archivo, rechazo por ruta insegura, síntesis CLI y cumplimiento de UX Copy.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario que integra `ClangFormatService` en modo dry-run para certificar la conformidad estilística de los archivos de UI y la ausencia de jerga interna en los diagnósticos.
3. **Integración en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Los ejecutables de pruebas `vigilio_clang_format_tests` y `vigilio_clang_format_service_tests` están plenamente integrados en el sistema de construcción de CTest, vinculando `vigilio_core` y `vigilio_compiler_warnings`.
   - Ambos ejecutables están catalogados bajo las propiedades `LABELS "unit;fast"` con un tiempo límite de ejecución de 15 segundos.
4. **Verificación continua existente (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - 17 de 17 pruebas pasando exitosamente en CTest con 100% de éxito en ~1.06 segundos:
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.05s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.03s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.16s)
     - `resource_qrc_inclusion_tests`: Passed (0.18s)
     - `startup_bootstrap_tests`: Passed (0.12s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.02s)
     - `cmake_presets_tests`: Passed (0.02s)
     - `cmake_presets_service_tests`: Passed (0.08s)
     - `test_fixture_foundation_tests`: Passed (0.00s)
     - `test_fixture_service_tests`: Passed (0.09s)
     - `clang_format_configuration_tests`: Passed (0.00s)
     - `clang_format_service_tests`: Passed (0.13s)
5. **Gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 44 referencias canónicas validadas con estado `valid: true` (exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0130-module.foundations.task0044.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
6. **Oportunidad y brecha para la etapa `tests` (task0044)**:
   - Si bien las pruebas unitarias básicas fueron creadas durante `task0041` y `task0042`, la etapa `tests` exige profundizar de manera sistemática en la resiliencia integral:
     - Pruebas de estrés bajo concurrencia masiva (16 hilos simultáneos) evaluando contención en el mutex interno y formateo paralelo.
     - Pruebas de cancelación cooperativa en múltiples fases de la operación.
     - Pruebas de inyección de fallos de permisos de archivo en disco y validación de idempotencia en rollback.
     - Pruebas exhaustivas de casos de frontera y límites numéricos de configuración YAML.
     - Pruebas de recorrido recursivo en árboles profundos de directorios temporales con exclusión de archivos y límites de profundidad.

---

## Alcance exacto

El alcance exacto de este pack comprende:
1. **Ampliación exhaustiva de pruebas en `tests/unit/ClangFormatConfigurationTests.cpp`**:
   - Casos de frontera en validación de parámetros (`indentWidth = 1`, `indentWidth = 16`, `columnLimit = 20`, `columnLimit = 500`).
   - Casos de rechazo numérico estricto (`indentWidth = 0`, `indentWidth = 17`, `columnLimit = 19`, `columnLimit = 501`, negativos).
   - Pruebas de rechazo de documentos YAML truncados, vacíos o con secuencias de escape corruptas.
   - Pruebas de rechazo de ataques de Directory Traversal complejos (`./../..`, `foo/../../bar`, `..\\windows\\style`).
   - Pruebas de roundtrip con variantes no predeterminadas (`PointerAlignmentPolicy::Right`, `ShortFunctionPolicy::Inline`).
2. **Ampliación exhaustiva de pruebas en `tests/unit/ClangFormatServiceTests.cpp`**:
   - **Concurrencia masiva multihilo**: 16 hilos concurrentes formateando en memoria e inspeccionando simultáneamente una instancia compartida de `ClangFormatService`, certificando ausencia de data races y convergencia determinista.
   - **Estrés de cancelación cooperativa multihilo**: Disparo asíncrono de `CancellationToken` durante la inspección y formateo de árboles de directorios con múltiples archivos, comprobando interrupción inmediata con estado `Cancelled`.
   - **Inyección de fallos de permisos y recuperación**: Simulación de archivos sin permisos de escritura mediante `std::filesystem::permissions`, verificando reporte de `PermissionDenied` o `WriteError`, preservación del archivo original y rollback limpio.
   - **Inspección y formateo recursivo en directorios multinivel**: Creación de jerarquías de directorios sintéticos con múltiples archivos conformes, no conformes, archivos no C++ (`.txt`, `.json`) y carpetas excluidas (`_build`, `.git`), comprobando el recuento exacto de `filesExamined`, `filesFormatted` y `discrepanciesCount`.
   - **Idempotencia de rollback y saneamiento de copias de seguridad**: Múltiples rollbacks sucesivos, rollback sin backup y confirmación de eliminación física con `removeBackup`.
   - **Síntesis CLI con rutas complejas**: Verificación de generación de comandos con rutas relativas, absolutas y con espacios.
3. **Auditoría de textos públicos**:
   - Verificación estricta de que todos los mensajes de diagnóstico y remediación cumplen al 100% con `docs/UX-COPY-PUBLICO.md`.
4. **Verificación del pipeline continuo y gobernanza**:
   - Ejecución limpia de `apps/vigilio-browser/scripts/check.sh`, validación de referencias canónicas y comprobación de integridad del lockfile con Prompt Builder CLI.

---

## Diseño completo de la capacidad

### 1. Topología y Arquitectura de Pruebas de Clang-Format

```mermaid
flowchart TD
    subgraph Harness["Arnés de Pruebas de Foundations (CTest)"]
        TCFG["vigilio_clang_format_tests<br/>(ClangFormatConfigurationTests.cpp)"]
        TSRV["vigilio_clang_format_service_tests<br/>(ClangFormatServiceTests.cpp)"]
        TUI["vigilio_ui_tests<br/>(WindowTests.cpp)"]
    end

    subgraph UnderTest["Módulos Bajo Prueba (vigilio_core)"]
        CFT[ClangFormatTypes.h<br/>Enums, Structs, CancellationToken]
        CFV[ClangFormatValidator<br/>Validación estática, YAML parser, Rutas seguras]
        CFC[ClangFormatConfiguration<br/>Configuración canónica LLVM C++20]
        CFS[ClangFormatService<br/>Formateo en memoria, Dry-Run, Backups, Rollback]
    end

    subgraph TestVectors["Vectores y Escenarios de Prueba"]
        POS["Casos Positivos:<br/>Formateo memoria/disco, Roundtrip YAML, Canon"]
        NEG["Casos Negativos:<br/>YAML inválido, Bounds numéricos, Path Traversal, Quota"]
        CONC["Concurrencia y Sincronización:<br/>16 hilos simultáneos, Thread-Safety, Mutex"]
        CANC["Cancelación Cooperativa:<br/>CancellationToken multihilo, Zero-Zombie"]
        FAIL["Inyección de Fallos y Recuperación:<br/>Read-only perms, Retry backoff, Atomic .bak"]
        COPY["Auditoría UX Copy:<br/>docs/UX-COPY-PUBLICO.md, Cero jerga interna"]
    end

    TCFG --> CFV
    TCFG --> CFC
    TCFG --> CFT
    TSRV --> CFS
    TSRV --> CFT
    TSRV --> CFC
    TUI --> CFS

    TCFG -.-> POS
    TCFG -.-> NEG
    TCFG -.-> COPY

    TSRV -.-> POS
    TSRV -.-> NEG
    TSRV -.-> CONC
    TSRV -.-> CANC
    TSRV -.-> FAIL
    TSRV -.-> COPY
```

### 2. Diagrama de Secuencia: Concurrencia Masiva Multihilo (16 Hilos)

```mermaid
sequenceDiagram
    autonumber
    participant Harness as Test Harness (16 Threads)
    participant ThreadPool as std::vector<std::thread> (16 Workers)
    participant Service as vigilio::ClangFormatService (Shared Instance)
    participant Mutex as std::mutex (Internal Guard)

    Harness->>Service: Instanciar con configuración canónica
    Harness->>ThreadPool: Lanzar 16 hilos simultáneos
    par Hilo 1..8: Formateo en Memoria
        ThreadPool->>Service: formatSourceCode(unformatted_buffer_i, options)
        Service->>Service: Formateo puro en memoria (Thread-safe)
        Service-->>ThreadPool: Código formateado determinista
    and Hilo 9..16: Inspección Dry-Run
        ThreadPool->>Service: inspectSource(code_sample_j)
        Service->>Mutex: std::lock_guard (Protección de estado interno)
        Mutex-->>Service: Bloqueo adquirido
        Service->>Service: Análisis de discrepancias
        Service->>Mutex: Liberación de bloqueo
        Service-->>ThreadPool: SourceFileFormatInspection
    end
    ThreadPool-->>Harness: Unir todos los hilos (join)
    Harness->>Harness: CHECK(cero excepciones, resultados consistentes, sin carreras)
```

### 3. Diagrama de Secuencia: Cancelación Cooperativa Inmediata (Zero-Zombie)

```mermaid
sequenceDiagram
    autonumber
    participant Harness as Test Runner
    participant CancelToken as vigilio::CancellationToken
    participant Service as vigilio::ClangFormatService
    participant TempTree as Directorio Temporal Sintético (Niveles Múltiples)

    Harness->>TempTree: Crear árbol de prueba con múltiples archivos C++
    Harness->>CancelToken: requestCancellation() (Estado atómico = true)
    Harness->>Service: inspectDirectory(tempDir, options, &cancelToken)
    Service->>CancelToken: isCancelled() -> true
    Service->>Service: Abortar inmediatamente sin descender en el árbol
    Service-->>Harness: FormatOperationResult(status=Cancelled, remediationHint="Operación cancelada...")
    Harness->>Harness: CHECK(res.status == FormatExecutionStatus::Cancelled)
    Harness->>Harness: CHECK(cero procesos zombis, cero descriptores abiertos)
```

### 4. Diagrama de Estado: Ciclo de Vida de Formateo, Respaldo y Rollback

```mermaid
stateDiagram-v2
    [*] --> OriginalPristine: Archivo existente en disco
    OriginalPristine --> BackupCreated: createBackup(filePath, .bak)
    BackupCreated --> FormattingInProgress: formatFile()
    
    state FormattingDecision <<choice>>
    FormattingInProgress --> FormattingDecision: ¿Escritura exitosa?
    
    FormattingDecision --> SuccessFormatted: Sí (Sin errores de disco/permisos)
    FormattingDecision --> FailureDetected: No (Permiso denegado / Disco lleno)
    
    SuccessFormatted --> CleanupBackup: removeBackup()
    CleanupBackup --> [*]: Archivo formateado limpio, .bak eliminado
    
    FailureDetected --> RollbackExecuted: rollbackFile()
    RollbackExecuted --> OriginalRestored: Restauración desde .bak
    OriginalRestored --> [*]: Archivo original intacto, sin corrupción
```

### 5. Diagrama de Clases: Arnés de Pruebas y Capas Desacopladas

```mermaid
classDiagram
    class ClangFormatConfigurationTests {
        <<test_suite>>
        +test_clang_format_canonical_options() void
        +test_clang_format_validator_bounds() void
        +test_clang_format_validator_malformed_yaml() void
        +test_clang_format_unsafe_path_traversal() void
        +test_clang_format_roundtrip_variants() void
        +test_clang_format_public_copy() void
    }

    class ClangFormatServiceTests {
        <<test_suite>>
        +test_format_service_in_memory_dialects() void
        +test_format_service_recursive_tree() void
        +test_format_service_backup_atomic_rollback() void
        +test_format_service_massive_concurrency() void
        +test_format_service_cooperative_cancellation() void
        +test_format_service_transient_retry_backoff() void
        +test_format_service_permission_fault_injection() void
        +test_format_service_quota_limit_enforcement() void
        +test_format_service_public_copy_compliance() void
    }

    class ClangFormatService {
        +loadConfiguration(path) bool
        +formatSourceCode(code, opts) string
        +inspectFile(path, cancel) SourceFileFormatInspection
        +inspectDirectory(dir, opts, cancel) FormatOperationResult
        +formatFile(path, opts, cancel) FormatOperationResult
        +formatWithRetry(path, opts, cancel) FormatOperationResult
        +createBackup(path, outBackup) bool
        +rollbackFile(path) bool
        +removeBackup(path) bool
        +executionStatusToString(status) string
    }

    class ClangFormatValidator {
        +validateConfigContent(yaml) ClangFormatValidationReport
        +isSafeConfigPath(path) bool
        +parseYaml(yaml, outOpts) bool
        +serializeYaml(opts) string
        +formatErrorToString(err) string
    }

    ClangFormatConfigurationTests ..> ClangFormatValidator : evalúa
    ClangFormatServiceTests ..> ClangFormatService : evalúa
    ClangFormatServiceTests ..> ClangFormatValidator : evalúa
```

---

## Archivos afectados

| Ruta de archivo | Acción propuesta | Justificación y responsabilidad |
| --- | --- | --- |
| `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp` | Modificar | Incorporar pruebas exhaustivas de casos de frontera numéricos, YAML malformado/truncado, ataques complejos de Directory Traversal y variantes no estándar. |
| `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp` | Modificar | Incorporar pruebas de concurrencia masiva multihilo (16 hilos), estrés de cancelación cooperativa, inyección de fallos de permisos, recorrido recursivo multinivel y cuotas estrictas. |
| `docs/rules/01-modules/foundations/micro/0044-clang-format-configuration/task.md` | Modificar (sync) | Sincronizar el documento canónico de la microtarea formalizando la aprobación de la etapa `tests`. |
| `openspec/changes/module-foundations-task0044/canonical-reference.yaml` | Creado | Metadatos canónicos del OpenSpec change para trazabilidad del runner. |
| `openspec/changes/module-foundations-task0044/proposal.md` | Creado | Propuesta técnica formal exhaustiva (este documento). |
| `openspec/changes/module-foundations-task0044/design.md` | Creado | Especificación técnica de diseño, gaps, compatibilidad y arquitectura de resiliencia. |
| `openspec/changes/module-foundations-task0044/tasks.md` | Creado | Lista ordenada y verificable de tareas atómicas para la fase APPLY. |
| `openspec/changes/module-foundations-task0044/execution-memory.md` | Creado | Memoria de ejecución, registro de skills leídas, invariantes y evidencia de gates. |

---

## Contrato UX/UI

- **Declaración formal de «sin cambio visual directo en la interfaz pública»**:
  - La capacidad **clang-format configuration** en la etapa **tests** es una suite de verificación de infraestructura, calidad de software y resiliencia de código; por consiguiente, **no introduce widgets, botones de linters, menús contextuales de formateo ni diálogos visibles en la ventana principal del navegador (`BrowserWindow`)**.
- **Preservación incondicional de los contratos visuales existentes**:
  - `BrowserWindow` permanece 100% enfocado en la navegación web del usuario, manteniendo intactos los 8 estados visuales fundamentales (`idle`, `hover`, `focus`, `disabled`, `loading`, `empty`, `error`, `success`).
  - Se preservan las dimensiones y reglas ergonómicas de «Interfaz más fácil de usar» (Modo Senior): tap targets mínimos $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos (22×22 a 32×32 px), aumento tipográfico (+4 pt) y atajos de teclado accesibles (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, etc.).
- **Auditoría vinculante de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Todas las cadenas de diagnóstico y remediación generadas por `ClangFormatService::executionStatusToString` y `ClangFormatValidator::formatErrorToString` son auditadas sistemáticamente en las pruebas automatizadas para comprobar que emplean exclusivamente español neutro comprensible para cualquier usuario, sin exponer términos internos de ingeniería («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).

---

## Reglas y fallos

| Condición de error / Escenario de fallo | Detección / Mecanismo de control | Estado observable / Transición | Recuperación y remediación |
| --- | --- | --- | --- |
| Documento YAML malformado o truncado | Parser de `ClangFormatValidator::validateConfigContent` | Retorna `isValid = false`, `error = ClangFormatError::ParseError` | Diagnóstico descriptivo en español sin excepciones no capturadas. |
| Valor numérico fuera de rango (`indentWidth = 0` o `> 16`) | Validación en `ClangFormatValidator` | Retorna `error = ClangFormatError::InvalidIndentWidth` | Mensaje orientativo en español indicando el rango permitido ($[1, 16]$). |
| Límite de columna inválido (`columnLimit = 10` o `> 500`) | Validación en `ClangFormatValidator` | Retorna `error = ClangFormatError::InvalidColumnLimit` | Mensaje orientativo indicando el rango permitido ($[20, 500]$). |
| Intento de Directory Traversal (`../../etc/passwd`) | Comprobación `ClangFormatValidator::isSafeConfigPath` | Retorna `isCompliant = false`, estado `UnsafePath` | Bloqueo preventivo de la operación sin acceso a disco. |
| Archivo que excede cuota máxima (`> maxFileSizeBytes`) | Verificación de tamaño previo a lectura en `ClangFormatService` | Retorna `status = FormatExecutionStatus::FileTooLarge` | Omisión segura del archivo sin agotamiento de memoria. |
| Archivo inexistente en inspección/formateo | Comprobación de existencia con `std::filesystem::exists` | Retorna `status = FormatExecutionStatus::FileNotFound` | Mensaje de remediación en español neutro indicando archivo no encontrado. |
| Fallo de escritura por permisos denegados (`perms::owner_read`) | Captura de excepción de I/O en `ClangFormatService::formatFile` | Retorna `status = FormatExecutionStatus::PermissionDenied` | Preservación de la copia original mediante rollback desde `.bak`. |
| Bloqueo de archivo transitorio durante formateo | Detección de contención en `formatWithRetry` | Reintentos configurados con retroceso exponencial | Si el archivo se libera, la operación concluye exitosamente. |
| Cancelación solicitada durante recorrido masivo | Comprobación periódica de `CancellationToken::isCancelled()` | Interrupción inmediata con `status = Cancelled` | Liberación inmediata de memoria y descriptores sin procesos zombi. |

---

## Seguridad y privacidad

- **Política Local-First constitucional (ADR-001 / Decisión Q-001)**:
  - Todo el proceso de inspección, análisis de estilo y formateo se ejecuta de forma estrictamente local y aislada en la máquina del usuario.
  - Cero dependencias en la nube, cero telemetría, cero llamadas a redes remotas y cero recolección de código fuente.
- **Defensa estricta contra Directory Traversal e inyección de rutas**:
  - `ClangFormatValidator::isSafeConfigPath` valida rigurosamente que las rutas inspeccionadas no contengan secuencias de escape de directorio (`..`), secuencias relativas ambiguas ni accesos fuera de la raíz autorizada.
- **Protección contra agotamiento de recursos (RAM / CPU)**:
  - Cuotas estrictas por archivo (`maxFileSizeBytes`, por defecto 10 MB). Archivos mayores se rechazan limpiamente con `FileTooLarge`.
  - Límite de profundidad máxima de escaneo en directorios (`maxScanDepth`, 32 niveles) para evitar bucles causados por enlaces simbólicos o jerarquías circulares.
- **Aislamiento de datos y privacidad en pruebas**:
  - Todos los fixtures de prueba se instancian en subdirectorios temporales aislados (`std::filesystem::temp_directory_path()`) con identificadores únicos basados en PID y marcas temporales en nanosegundos, garantizando eliminación completa tras la ejecución mediante RAII.

---

## Matriz de pruebas

| Identificador de prueba | Tipo de prueba | Archivo de prueba | Escenario evaluado | Resultado esperado | Criterio de aceptación |
| --- | --- | --- | --- | --- | --- |
| `TEST-CFG-001` | Unitario | `ClangFormatConfigurationTests.cpp` | Configuración canónica vs archivo `.clang-format` | Coincidencia idéntica de todas las opciones canónicas | Opciones equivalentes a LLVM, 4 espacios, 105 cols, C++20 |
| `TEST-CFG-002` | Unitario | `ClangFormatConfigurationTests.cpp` | Valores numéricos límite en fronteras exactas ($1, 16, 20, 500$) | Validación exitosa en frontera | `isValid == true`, `error == ClangFormatError::None` |
| `TEST-CFG-003` | Unitario | `ClangFormatConfigurationTests.cpp` | Valores numéricos fuera de rango ($0, 17, 19, 501$, negativos) | Rechazo tipado estricto | `error == InvalidIndentWidth` o `InvalidColumnLimit` |
| `TEST-CFG-004` | Unitario | `ClangFormatConfigurationTests.cpp` | Documentos YAML truncados, vacíos o malformados | Rechazo seguro sin crash | `error == ParseError`, mensaje en español neutro |
| `TEST-CFG-005` | Unitario | `ClangFormatConfigurationTests.cpp` | Ataques complejos de Directory Traversal (`../../`, `..\\`) | Detección y rechazo de ruta insegura | `isSafeConfigPath == false`, `error == UnsafeFilePath` |
| `TEST-CFG-006` | Unitario | `ClangFormatConfigurationTests.cpp` | Serialización roundtrip con políticas no estándar | Preservación de todos los campos serializados | YAML parseado idéntico a las opciones originales |
| `TEST-SRV-001` | Integración | `ClangFormatServiceTests.cpp` | Formateo en memoria pura para múltiples extensiones C++ | Código formateado determinista | Sangría de 4 espacios, punteros `*` a la izquierda |
| `TEST-SRV-002` | Integración | `ClangFormatServiceTests.cpp` | Inspección dry-run sobre árbol de directorios multinivel | Detección exacta de archivos y discrepancias | Archivos inmutables en disco, recuento exacto |
| `TEST-SRV-003` | Integración | `ClangFormatServiceTests.cpp` | Formateo físico sobre jerarquía con carpetas excluidas | Solo archivos válidos formateados | Carpetas `_build`, `.git` ignoradas, recuento exacto |
| `TEST-SRV-004` | Integración | `ClangFormatServiceTests.cpp` | Gestión de respaldo atómico (`.bak`) y rollback | Restauración fiel ante mutación | Archivo restaurado bit a bit idéntico al original |
| `TEST-SRV-005` | Concurrencia | `ClangFormatServiceTests.cpp` | Concurrencia masiva con 16 hilos simultáneos | Cero carreras de datos, thread-safety | Todos los hilos finalizan exitosamente sin excepciones |
| `TEST-SRV-006` | Concurrencia | `ClangFormatServiceTests.cpp` | Cancelación cooperativa multihilo en escaneo masivo | Retorno inmediato con `Cancelled` | Cero procesos zombis, sin descriptores colgados |
| `TEST-SRV-007` | Resiliencia | `ClangFormatServiceTests.cpp` | Inyección de fallo de permisos de archivo (read-only) | Retorno `PermissionDenied` o `WriteError` | Archivo original preservado, rollback exitoso |
| `TEST-SRV-008` | Resiliencia | `ClangFormatServiceTests.cpp` | Reintentos con retroceso exponencial (`formatWithRetry`) | Éxito tras liberación de contención | Retorno `Success` tras reintentos configurados |
| `TEST-SRV-009` | Seguridad | `ClangFormatServiceTests.cpp` | Archivo que excede cuota de memoria (`> maxFileSizeBytes`) | Rechazo inmediato con `FileTooLarge` | Omisión segura del archivo sin saturación de RAM |
| `TEST-SRV-010` | Regresión | `ClangFormatServiceTests.cpp` | Auditoría exhaustiva de UX Copy en español | Ausencia de términos técnicos prohibidos | Cero ocurrencias de «Senior Mode», «Modo Senior», etc. |

---

## Pasos APPLY

1. **Paso 1**: Refinar `apps/vigilio-browser/tests/unit/ClangFormatConfigurationTests.cpp` incorporando las nuevas funciones de prueba de frontera numérica (`test_clang_format_validator_boundary_values`), sintaxis YAML malformada/truncada (`test_clang_format_malformed_yaml_inputs`) y Directory Traversal complejo (`test_clang_format_complex_path_traversal`).
2. **Paso 2**: Refinar `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp` incorporando las pruebas de concurrencia masiva multihilo con 16 hilos (`test_format_service_massive_multithreading_concurrency`) e interrupción cooperativa con token asíncrono (`test_format_service_cooperative_cancellation_stress`).
3. **Paso 3**: Incorporar en `apps/vigilio-browser/tests/unit/ClangFormatServiceTests.cpp` las pruebas de inyección de fallos de permisos de archivo (`test_format_service_permission_failure_injection`) y formateo recursivo multinivel con exclusiones (`test_format_service_recursive_directory_tree`).
4. **Paso 4**: Ejecutar la suite completa mediante `apps/vigilio-browser/scripts/check.sh`, validando compilación limpia bajo C++20 con `-Werror`, cero advertencias y 100% de pruebas aprobadas en CTest.
5. **Paso 5**: Validar referencias canónicas y lockfile de ejecución mediante Prompt Builder CLI.
6. **Paso 6**: Sincronizar el documento canónico de microtarea `docs/rules/01-modules/foundations/micro/0044-clang-format-configuration/task.md`.

---

## Definición de terminado

- [ ] Todas las pruebas de `ClangFormatConfigurationTests.cpp` y `ClangFormatServiceTests.cpp` compilan limpiamente bajo C++20 con `-Werror` y cero advertencias.
- [ ] La totalidad de las suites de prueba en CTest pasan al 100% (17/17 pruebas existentes más las nuevas aserciones ampliadas).
- [ ] La ejecución completa en CTest se realiza en < 2 segundos de tiempo real, sin requerir entorno gráfico (`DISPLAY`).
- [ ] Cero fugas de descriptores de archivo, cero procesos zombis y saneamiento garantizado de copias temporales.
- [ ] El script de verificación continua `apps/vigilio-browser/scripts/check.sh` finaliza con código de salida 0.
- [ ] Prompt Builder CLI valida al 100% las referencias canónicas (`valid: true`) y el archivo de bloqueo de ejecución.
- [ ] La propuesta respeta estrictamente los principios de Local-First, protección de datos y UX Copy en español neutro.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0043`: Implementación de interacción visual y auditoría en arnés de UI completada y certificada.
  - LLVM Clang-Format 18+ instalado en el entorno de desarrollo y CI.
  - CMake 3.28+ y compilador C++20 (GCC 13+ o Clang 17+).
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Contención de hilos o carreras de datos en pruebas de concurrencia masiva.
    *Mitigación*: Uso estricto de `std::lock_guard` con `std::mutex` interno en `ClangFormatService` y diseño inmutable de `ClangFormatOptions`.
  - *Riesgo 2*: Residuos de archivos temporales en disco tras fallos inyectados de permisos o escrituras abortadas.
    *Mitigación*: Uso del guardián RAII `TempDirGuard` que asegura la purga completa del directorio temporal en su destructor, restaurando permisos de escritura antes de invocar `std::filesystem::remove_all`.
  - *Riesgo 3*: Falsos fallos en entornos de CI headless.
    *Mitigación*: Pruebas del núcleo completamente desacopladas de Qt GUI y WebEngine, ejecutándose de forma pura y determinista sin requerir servidor gráfico.

---

## Commit propuesto

`test(foundations): tests clang-format-configuration`
