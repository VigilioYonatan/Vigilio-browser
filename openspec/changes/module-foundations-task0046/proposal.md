# Change Proposal — module-foundations-task0046

Pack: `module.foundations.task0046`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **clang-tidy configuration** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0046 — Implementar la operación en una capa propietaria: clang-tidy configuration`).

Habiéndose completado y certificado en las capacidades precedentes la modularización de targets CMake (`task0001`-`task0004`), la política de diagnósticos y advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política integral de recuperación tras caídas (`task0029`-`task0032`), la orquestación de CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures desacoplada (`task0037`-`task0040`), la capacidad completa de Clang-Format (`task0041`-`task0044`), y la formalización del modelo de dominio, tipos inmutables, validador estático y parser YAML para Clang-Tidy (`task0045`), esta propuesta formaliza la **implementación de la capa operativa propietaria de servicio (`vigilio::ClangTidyService`), extensiones operativas de inspección estática en memoria y en disco (dry-run), corrección determinista (autofix), cancelación cooperativa multihilo (Zero-Zombie), resiliencia con reintentos exponenciales ante contención de bloqueos de archivo, preservación atómica de copias de seguridad (`.bak`), rollback ante fallos y síntesis determinista de argumentos de línea de comandos en C++20**.

El objetivo central de esta etapa es dotar al navegador de un servicio desacoplado, seguro y robusto en `src/core/` que orqueste el análisis estático de código C++20 conforme a las reglas oficiales del proyecto, permita la inspección segura en memoria y en disco sin mutar archivos (dry-run), ofrezca corrección automática determinista para directivas soportadas (`modernize-use-nullptr`, `readability-braces-around-statements`), soporte cancelación cooperativa para prevenir procesos zombis colgados indefinidamente, gestione copias de seguridad locales y rollback atómico en caso de anomalías de E/S, aplique reintentos con retroceso exponencial ante bloqueos transitorios de archivo, y limite estrictamente el consumo de recursos mediante cuotas por archivo (máximo 10 MB) y profundidad de escaneo acotada, **sin acoplar lógica de negocio a los widgets de interfaz gráfica ni añadir dependencias hacia WebEngine o `BrowserWindow`**.

El alcance concreto comprende:
- **Capa operativa de servicio desacoplada (`vigilio::ClangTidyService`)**:
  - Inicialización flexible y configuración: carga desde archivo físico canónico (`loadConfiguration`) o inyección directa de opciones inmutables (`setOptions`, `options`).
  - Inspección y corrección en memoria pura:
    - `inspectSource`: Análisis en memoria sin acceso a disco, evaluando código C++ frente a las reglas configuradas (`modernize-use-nullptr`, `readability-braces-around-statements`, `bugprone-*`, `performance-*`) y reportando hallazgos fuertemente tipados.
    - `applyFixesToSource`: Transformación determinista en memoria que aplica correcciones automáticas sobre el código fuente (reemplazo de `NULL`/`0` por `nullptr`, adición de llaves a estructuras de control).
  - Inspección en disco en modo seguro (dry-run):
    - `inspectFile`: Evaluación individual de archivos fuente C++ (`.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`), verificando conformidad estricta sin alterar el sistema de archivos.
    - `inspectDirectory`: Análisis por lotes de un directorio, aplicando filtros canónicos de extensión (`isSupportedSourceExtension`) y exclusión (`isExcludedDirectory`: `_build`, `build`, `generated`, `.git`, `.prompt-runner`, `assets`), con límite de profundidad de 32 niveles.
  - Corrección física en disco y resiliencia con respaldo atómico:
    - `applyFixesToFile`: Aplicación directa de correcciones sobre disco con generación previa de copia de seguridad (`.bak`) atómica si `options.createBackup` está habilitado.
    - `applyFixesWithRetry`: Resiliencia ante bloqueos transitorios de archivos (*file locking*), aplicando reintentos con retroceso exponencial (*exponential backoff*: $50\text{ ms} \times 2^i$, hasta 3 reintentos) antes de reportar error.
    - `applyFixesToDirectory`: Corrección por lotes a nivel de directorio con soporte de cancelación cooperativa en cada archivo procesado.
    - `rollbackFile` y `removeBackup`: Restauración atómica del archivo prístino desde su copia de seguridad y purga ordenada tras éxito comprobado.
  - Soporte de **cancelación cooperativa multihilo** mediante `CancellationToken*`:
    - Permite interrumpir de manera inmediata y segura operaciones intensivas de escaneo o análisis por lotes.
    - Garantiza taxativamente la política Zero-Zombie: ningún hilo o subproceso queda colgado indefinidamente.
  - Síntesis determinista de línea de comandos:
    - `buildClangTidyCommandLine`: Generación reproducible de argumentos CLI para invocar la herramienta externa `clang-tidy` (`-p <build-path>`, `--checks=...`, `-fix`, `-warnings-as-errors=...`).
- **Ampliación de tipos y contratos del dominio operativo (`ClangTidyTypes.h`)**:
  - `TidyExecutionStatus`: Taxonomía fuertemente tipada de resultados operativos (`Success`, `IssuesFound`, `Cancelled`, `FileLocked`, `PermissionDenied`, `ReadError`, `WriteError`, `FileTooLarge`, `UnsafePath`, `FileNotFound`, `ConfigurationError`, `ToolUnavailable`).
  - `SourceFileTidyInspection`: Estructura inmutable que encapsula la inspección individual de un archivo (`relativePath`, `isClean`, `issuesCount`, lista de `ClangTidyIssueInspection`, `summary`).
  - `TidyServiceOptions`: Opciones inmutables de configuración operativa (`dryRun`, `createBackup`, `maxFileSizeBytes = 10 MB`, `timeoutMs = 15000`, `maxRetries = 3`, `retryBackoffMs = 50`, `recursive = true`, `maxScanDepth = 32`, `buildDirectory = "_build"`, `exportFixes = false`).
  - `TidyOperationResult`: Estructura inmutable que encapsula el resultado consolidado de la operación (`status`, `filesExamined`, `filesFixed`, `issuesCount`, `durationMs`, `remediationHint`, `details`).
  - Token de cancelación cooperativa `CancellationToken` bajo protección de macro `VIGILIO_CANCELLATION_TOKEN_DEFINED`.
- **Invariantes constitucionales de calidad, portabilidad y privacidad**:
  - *Local-First Constitucional (ADR-001 / Decisión Q-001)*: Cero conexiones de red, cero servicios cloud y cero telemetría remota.
  - *Seguridad de rutas y contención*: Rechazo categórico de directory traversal (`..`) mediante `ClangTidyValidator::isSafeConfigPath`.
  - *Presupuestos estrictos de recursos y Zero-Zombie*: Rechazo inmediato de archivos que superen 10 MB (`FileTooLarge`), tiempos de ejecución en CTest < 150 ms para la suite completa y comprobación periódica de `CancellationToken`.
- **Suite completa de pruebas unitarias del servicio (`tests/unit/ClangTidyServiceTests.cpp`)**:
  - 12 pruebas unitarias deterministas en CTest (`TEST-TIDY-SRV-001` a `TEST-TIDY-SRV-012`) cubriendo inicialización, inspección en memoria, autofix en memoria, dry-run en disco, autofix con backups, rollback, reintentos con backoff, cancelación cooperativa con token, escaneo de directorio con filtros, cuotas de archivo y rutas inseguras, síntesis CLI y cumplimiento del copy público en español neutro sin jerga técnica.

---

## Out of scope

- Implementación física de componentes de interfaz gráfica (widgets Qt, ventanas de configuración, paneles de linteo en la UI o diálogos de visualización de advertencias); estos corresponden a la etapa `screens` en `module.foundations.task0047`.
- Pruebas pesadas de estrés de linteo multihilo masivo y pruebas de integración E2E de navegador completo (reservadas para la etapa `tests` en `module.foundations.task0048`).
- Modificación directa del código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante la fase APPLY tras aprobación humana).
- Descarga o instalación automatizada de binarios del sistema operativo o invocación no controlada de herramientas externas sin timeout.

---

## Contexto y evidencia

1. **Estado del módulo foundations y capacidad de linteo**:
   - En la etapa anterior (`task0045`, commit `aec7f7e`), se formalizó el modelo de dominio en `ClangTidyTypes.h`, `ClangTidyValidator.h/.cpp`, `ClangTidyConfiguration.h/.cpp` y la suite `ClangTidyConfigurationTests.cpp` (11 pruebas pasando exitosamente).
   - El pipeline actual de verificación continua (`apps/vigilio-browser/scripts/check.sh`) compila limpiamente con C++20 y `-Werror`, ejecutando 18/18 pruebas con 100% de éxito en 2.86 segundos.
2. **Configuración física canónica en el repositorio**:
   - `apps/vigilio-browser/.clang-tidy`:
     ```yaml
     Checks: '-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements'
     WarningsAsErrors: ''
     HeaderFilterRegex: 'src/.*'
     ```
   - Este conjunto de reglas canónicas está alineado con los chequeos implementados en `ClangTidyService`.
3. **Patrón de servicio probado y existente en el proyecto**:
   - La arquitectura de `ClangFormatService` (`task0042`) demostró ser robusta, desacoplada y libre de fallos en CTest, sirviendo como arquetipo directo para la implementación de `ClangTidyService`.
   - Se mantiene la compatibilidad de firmas, contratos de cancelación y convención de backups atómicos `.bak`.

---

## Alcance exacto

El incremento técnico para la etapa `implementation` de `module.foundations.task0046` abarca:
1. **Modelo de tipos ampliado (`apps/vigilio-browser/src/core/ClangTidyTypes.h`)**:
   - Incorporar `CancellationToken` (compartido/protegido por macro `VIGILIO_CANCELLATION_TOKEN_DEFINED`).
   - Declarar `enum class TidyExecutionStatus : uint8_t` con los 12 estados operativos.
   - Declarar `struct SourceFileTidyInspection` con detalle de problemas detectados por archivo.
   - Declarar `struct TidyServiceOptions` con parámetros de ejecución, cuota de 10 MB, timeout de 15s y profundidad de 32 niveles.
   - Declarar `struct TidyOperationResult` con métricas consolidadas y métodos `isSuccess()` y `hasIssues()`.
2. **Declaración del servicio (`apps/vigilio-browser/src/core/ClangTidyService.h`)**:
   - Declarar la clase `ClangTidyService` en el namespace `vigilio`.
   - Métodos públicos de ciclo de vida: constructores, `loadConfiguration`, `setOptions`, `options`, `isConfigured`.
   - Métodos de análisis y corrección en memoria: `inspectSource`, `applyFixesToSource`.
   - Métodos de disco: `inspectFile`, `inspectDirectory`, `applyFixesToFile`, `applyFixesWithRetry`, `applyFixesToDirectory`.
   - Métodos de respaldo: `createBackup`, `rollbackFile`, `removeBackup`.
   - Síntesis CLI: `buildClangTidyCommandLine`.
   - Funciones estáticas de UX Copy: `executionStatusToString`, `generateRemediationHint`.
3. **Implementación del servicio (`apps/vigilio-browser/src/core/ClangTidyService.cpp`)**:
   - Implementación C++20 portable empleando `<filesystem>`, `<fstream>`, `<regex>`, `<chrono>`, `<thread>`, `<atomic>`, `<mutex>`.
   - Análisis estático determinista para las reglas canónicas del proyecto.
   - Lógica de reintento exponencial con retroceso ($50\text{ ms} \times 2^i$) y detección de cancelación en cada iteración.
   - Recorrido de directorios respetando exclusiones y extensiones permitidas.
   - Manejo de respaldos atómicos y restauración segura.
4. **Batería de pruebas unitarias (`apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp`)**:
   - 12 pruebas unitarias deterministas (`TEST-TIDY-SRV-001` a `TEST-TIDY-SRV-012`).
5. **Configuración de CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Añadir `src/core/ClangTidyService.cpp` a la biblioteca `vigilio_core`.
   - Crear ejecutable de pruebas `vigilio_clang_tidy_service_tests`.
   - Registrar test `clang_tidy_service_tests` con labels `unit;fast` y timeout de 15 segundos.
6. **Sincronización documental**:
   - Actualizar `docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md` para reflejar el estado completado de la etapa `implementation`.

---

## Diseño completo de la capacidad

### 1. Entidades de datos y responsabilidades

- **`ClangTidyService`**: Entidad operativa propietaria responsable de coordinar el análisis estático, la aplicación de correcciones, la gestión de respaldos y la síntesis CLI. Encapsula un mutex interno para proteger el estado de configuración en entornos multihilo.
- **`TidyExecutionStatus`**: Enumeración fuertemente tipada que describe con precisión matemática el resultado de cualquier invocación del servicio.
- **`TidyServiceOptions`**: Parámetros inmutables de ejecución que gobiernan límites de memoria, tiempo, reintentos y recursión.
- **`TidyOperationResult`**: Estructura inmutable que consolida las métricas de la operación, lista de inspecciones y recomendación accesible en español neutro.
- **`CancellationToken`**: Token cooperativo atómico que permite interrumpir de inmediato cualquier escaneo o corrección por lotes.

### 2. Contrato funcional

- **Actor / Invocación**:
  - Hilo principal o hilos de trabajo en segundo plano (workers) del navegador o del arnés de pruebas de CTest.
- **Entradas**:
  - Código fuente en memoria (`std::string_view`) o rutas de archivos/directorios en disco (`std::filesystem::path`).
  - Opciones operativas (`TidyServiceOptions`) y token de cancelación opcional (`CancellationToken*`).
- **Salidas**:
  - `SourceFileTidyInspection` para operaciones individuales.
  - `TidyOperationResult` para operaciones consolidada o por lotes.
  - Archivos corregidos en disco con copias `.bak` intermedias según la política configurada.
- **Transiciones de estado**:
  - `Unconfigured` $\rightarrow$ `Configured` (mediante `loadConfiguration` o `setOptions`).
  - `Configured` $\rightarrow$ `Inspecting` $\rightarrow$ `Success` / `IssuesFound` / `Cancelled` / `Error`.
- **Deshacer y recuperación**:
  - Si una operación de corrección falla a mitad de escritura, `rollbackFile` restituye el contenido prístino desde el archivo `.bak`.
  - Si la operación concluye con éxito, `removeBackup` elimina el archivo de respaldo para no contaminar el espacio de trabajo.
- **Concurrencia**:
  - Métodos reentrantes y protegidos mediante `std::mutex` interno sobre las opciones del servicio.
  - Cancelación cooperativa periódica (`isCancelled()`) garantizando cumplimiento estricto de la política Zero-Zombie.

---

## Archivos afectados

| Archivo | Acción | Propósito |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/ClangTidyTypes.h` | Modificación | Incorporar tipos operativos: `TidyExecutionStatus`, `SourceFileTidyInspection`, `TidyServiceOptions`, `TidyOperationResult` y guarda `VIGILIO_CANCELLATION_TOKEN_DEFINED`. |
| `apps/vigilio-browser/src/core/ClangTidyService.h` | Creación | Declaración de la clase desacoplada `ClangTidyService` y sus métodos de inspección, corrección, respaldo y síntesis CLI. |
| `apps/vigilio-browser/src/core/ClangTidyService.cpp` | Creación | Implementación en C++20 de las capacidades operativas del servicio, reintentos exponenciales, backups atómicos y copy público. |
| `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp` | Creación | Batería exhaustiva de 12 pruebas unitarias deterministas en CTest (`TEST-TIDY-SRV-001` a `TEST-TIDY-SRV-012`). |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Registrar `ClangTidyService.cpp` en `vigilio_core`, definir ejecutable `vigilio_clang_tidy_service_tests` y registrar test `clang_tidy_service_tests`. |
| `docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md` | Modificación | Actualizar el estado y evidencia de la microtarea a etapa `implementation` completada. |
| `openspec/changes/module-foundations-task0046/**` | Creación | Artefactos OpenSpec de la propuesta (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`). |

---

## Contrato UX/UI

- **Impacto visual directo**: **Sin cambio visual directo**. Esta etapa corresponde exclusivamente a la infraestructura operativa en `src/core/` (biblioteca headless `vigilio_core`).
- **Preparación para Senior Mode y accesibilidad**:
  - Los textos de diagnóstico y recomendaciones emitidos por `executionStatusToString` y `generateRemediationHint` cumplen estrictamente con `docs/UX-COPY-PUBLICO.md`.
  - Prohibición taxativa de jerga interna: no se incluyen términos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain» ni cadenas de error crípticas.
  - Todo mensaje es claro, amigable y redactado en español neutro comprensible por adultos mayores o usuarios sin conocimientos técnicos.
- **Preparación para la etapa UI (`task0047`)**:
  - La interfaz gráfica de la ventana principal podrá consultar `ClangTidyService` en un hilo secundario sin congelar el hilo de UI, mostrando estados accesibles con iconos SVG Lucide de contraste WCAG AAA.

---

## Reglas y fallos

- **Matriz de fallos y recuperación**:
  1. *Ruta insegura (`UnsafePath`)*: Si la ruta contiene `..` o caracteres de escape hacia directorios superiores, se rechaza inmediatamente sin acceder al disco.
  2. *Archivo no encontrado (`FileNotFound`)*: Se retorna estado tipado y sugerencia clara en español; no se lanza excepción no capturada.
  3. *Archivo bloqueado (`FileLocked`)*: Se aplican hasta `maxRetries` (3) con retroceso exponencial ($50\text{ ms} \times 2^i$); si persiste el bloqueo, se retorna `FileLocked`.
  4. *Cuota excedida (`FileTooLarge`)*: Archivos mayores a 10 MB son rechazados inmediatamente para preservar presupuestos de memoria RAM.
  5. *Cancelación por el usuario (`Cancelled`)*: Al activarse `CancellationToken`, la operación se detiene en el archivo actual, no se inician nuevas tareas y se limpian respaldos temporales huérfanos.
  6. *Fallo durante la escritura (`WriteError`)*: Se activa `rollbackFile` inmediatamente, restituyendo el contenido prístino y evitando corrupción de datos.

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Todo el análisis estático y linteo se realiza estrictamente en el entorno local del usuario.
  - Cero dependencias hacia servidores remotos, cero servicios cloud y cero envío de telemetría de código.
- **Aislamiento de procesos y memoria**:
  - No se instancian procesos de Chromium ni WebEngine para el análisis de linteo.
  - Operación confinada a la memoria de la aplicación con límite estricto de 10 MB por archivo.
- **Seguridad en el sistema de archivos**:
  - Validación de rutas mediante `ClangTidyValidator::isSafeConfigPath`.
  - Creación atómica de copias de seguridad `.bak` con permisos restringidos.

---

## Matriz de pruebas

| ID Prueba | Caso evaluado | Fixture / Entrada | Comportamiento esperado | Fase |
| --- | --- | --- | --- | --- |
| `TEST-TIDY-SRV-001` | Inicialización y configuración | Sin config, con canonical, carga desde `.clang-tidy` | `isConfigured()` refleja estado; opciones coinciden con canónicas | APPLY |
| `TEST-TIDY-SRV-002` | Inspección en memoria pura | Código con `NULL`, sin llaves en `if`, código limpio | Detecta violaciones en código sucio; `isClean == true` en código limpio | APPLY |
| `TEST-TIDY-SRV-003` | Corrección automática en memoria | Código con `NULL` y `if` sin llaves | Transforma a `nullptr` y añade llaves deterministamente | APPLY |
| `TEST-TIDY-SRV-004` | Inspección de archivo en disco (dry-run) | Archivo físico C++ en carpeta temporal | Reporta hallazgos sin mutar archivo ni modificar timestamp | APPLY |
| `TEST-TIDY-SRV-005` | Corrección física con backup | Archivo físico con violaciones de estilo | Aplica corrección en disco, crea respaldo `.bak` y limpia tras éxito | APPLY |
| `TEST-TIDY-SRV-006` | Respaldo atómico y rollback | Archivo físico con fallo simulado de escritura | Restaura fielmente el archivo original desde la copia `.bak` | APPLY |
| `TEST-TIDY-SRV-007` | Reintentos con retroceso exponencial | Archivo con bloqueo transitorio simulado | Reintenta con backoff exponencial ($50\text{ ms} \times 2^i$) y recupera | APPLY |
| `TEST-TIDY-SRV-008` | Cancelación cooperativa multihilo | Hilo de trabajo con `CancellationToken` activado | Detiene escaneo inmediatamente y retorna `Cancelled` (Zero-Zombie) | APPLY |
| `TEST-TIDY-SRV-009` | Escaneo de directorios y filtros | Árbol con `.cpp`, `.h`, `_build/`, `.git/` | Procesa solo fuentes admitidos; ignora carpetas excluidas | APPLY |
| `TEST-TIDY-SRV-010` | Cuota de tamaño y rutas inseguras | Archivo > 10 MB y ruta con `../../etc/passwd` | Retorna `FileTooLarge` y `UnsafePath` respectivamente | APPLY |
| `TEST-TIDY-SRV-011` | Síntesis de línea de comandos CLI | Opciones con checks, build path y flags | Genera comando `clang-tidy` canónico y reproducible | APPLY |
| `TEST-TIDY-SRV-012` | Auditoría de copy público en español | Todos los estados y hints del servicio | Textos en español sin jerga interna ni términos prohibidos | APPLY |

---

## Pasos APPLY

1. **Paso 1**: Extender `apps/vigilio-browser/src/core/ClangTidyTypes.h` con enumeraciones y estructuras del servicio operativo.
2. **Paso 2**: Crear `apps/vigilio-browser/src/core/ClangTidyService.h` con la interfaz desacoplada de la clase `ClangTidyService`.
3. **Paso 3**: Crear `apps/vigilio-browser/src/core/ClangTidyService.cpp` implementando inicialización, análisis en memoria, corrección, backups, reintentos y copy público.
4. **Paso 4**: Crear `apps/vigilio-browser/tests/unit/ClangTidyServiceTests.cpp` con las 12 pruebas unitarias operativas.
5. **Paso 5**: Actualizar `apps/vigilio-browser/CMakeLists.txt` incorporando `ClangTidyService.cpp` en `vigilio_core` y registrando el target y test de CTest `clang_tidy_service_tests`.
6. **Paso 6**: Ejecutar el pipeline de compilación y pruebas con `apps/vigilio-browser/scripts/check.sh`, verificando 19/19 pruebas en verde.
7. **Paso 7**: Validar referencias canónicas y lockfile mediante Prompt Builder CLI.
8. **Paso 8**: Sincronizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0046-clang-tidy-configuration/task.md`.

---

## Definición de terminado

- [ ] `ClangTidyTypes.h` ampliado y compilando limpiamente sin dependencias de GUI ni WebEngine.
- [ ] `ClangTidyService.h` y `ClangTidyService.cpp` implementados con arquitectura desacoplada, resiliencia y Zero-Zombie.
- [ ] 12 pruebas unitarias operativas en `ClangTidyServiceTests.cpp` ejecutándose y pasando en CTest en menos de 2 segundos.
- [ ] Pipeline `apps/vigilio-browser/scripts/check.sh` pasando al 100% (19/19 tests) con `-Werror` y cero advertencias.
- [ ] `validate-references` y `verify-lock` aprobados con `valid: true`.
- [ ] Documento de microtarea sincronizado y formalizado.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0045` completado y certificado en `aec7f7e`.
  - Herramientas estándar del sistema: compilador C++20, CMake 3.22+, Ninja.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Modificación accidental de archivos prístinos durante la aplicación de correcciones.
    *Mitigación*: Mecanismo atómico obligatorio de respaldo `.bak` previo a cualquier escritura, con función garantizada de `rollbackFile`.
  - *Riesgo*: Bloqueo o cuelgue indefinido en directorios masivos o de gran profundidad.
    *Mitigación*: Límite estricto de profundidad a 32 niveles, comprobación obligatoria de `CancellationToken` en cada archivo procesado y timeout configurable.
  - *Riesgo*: Consumo excesivo de memoria ante archivos descomunales.
    *Mitigación*: Rechazo inmediato de archivos mayores a 10 MB mediante `maxFileSizeBytes`.

---

## Commit propuesto

```
feat(foundations): implementation clang-tidy-configuration
```
