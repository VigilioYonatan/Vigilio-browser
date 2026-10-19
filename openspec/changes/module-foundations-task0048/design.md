# Design — module-foundations-task0048

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Modelo de dominio, validador sintáctico y servicio operativo de Clang-Tidy (`task0045`, `task0046`, `task0047`)**:
   - `vigilio_core` cuenta con `src/core/ClangTidyTypes.h`, que establece enumeraciones fuertemente tipadas (`ClangTidyValidationState`, `ClangTidyError`, `ClangTidyCheckCategory`, `TidyExecutionStatus`), estructuras inmutables de configuración e inspección (`ClangTidyOptions`, `ClangTidyIssueInspection`, `SourceFileTidyInspection`, `TidyServiceOptions`, `TidyOperationResult`) y el token cooperativo de cancelación multihilo `CancellationToken`.
   - `src/core/ClangTidyValidator.h/.cpp` implementa la validación estática de opciones, comprobación de rutas seguras (`isSafeConfigPath`), prevención de Directory Traversal (`..`), parser de sintaxis YAML (`parseConfig`, `validateConfigContent`), serializador (`serializeConfig`), extracción de tokens de checks (`parseCheckTokens`) y traducción de estados a español neutro (`tidyErrorToString`, `tidyStateToString`, `categoryToString`).
   - `src/core/ClangTidyConfiguration.h/.cpp` genera la configuración canónica inmutable del proyecto (LLVM 18+, checks `-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements`, `headerFilterRegex: src/.*`), valida invariantes (`verifyInvariants`), valida extensiones compatibles (`.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`) y exclusiones de carpetas (`_build`, `build`, `generated`, `.git`, `.prompt-runner`, `assets`, `node_modules`).
   - `src/core/ClangTidyService.h/.cpp` proporciona la capa operativa desacoplada con inspección en memoria pura (`inspectSource`), corrección automática en memoria (`applyFixesToSource`), inspección dry-run en memoria y en disco (`inspectFile`, `inspectDirectory`), corrección física con respaldo atómico (`applyFixesToFile`), corrección con reintentos exponenciales ante contención (`applyFixesWithRetry`), gestión atómica de respaldos y reversión (`createBackup`, `rollbackFile`, `removeBackup`), síntesis CLI (`buildClangTidyCommandLine`) y textos públicos de estado y remediación (`executionStatusToString`, `generateRemediationHint`).
2. **Suites de pruebas básicas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/ClangTidyConfigurationTests.cpp`: 11 pruebas unitarias enfocadas en opciones canónicas, validador de opciones, checks vacíos, regex inválido de cabeceras, parser YAML canónico, roundtrip, claves obligatorias, rutas inseguras, extensiones, extracción de tokens y UX copy.
   - `tests/unit/ClangTidyServiceTests.cpp`: 12 pruebas unitarias enfocadas en inicialización, inspección en memoria, autofix en memoria, dry-run conforme/discrepante, fix físico, backup/rollback, reintento básico, cancelación preliminar, escaneo de directorio con filtros, cuotas de archivo, rutas inseguras, síntesis CLI y textos públicos.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario con verificación programática dry-run de conformidad de análisis estático sobre los archivos de UI y auditoría de textos públicos de remediación.
3. **Estado de compilación y pruebas continuas**:
   - Compilación limpia bajo C++20 con `-Werror` y cero advertencias.
   - 19 de 19 pruebas en CTest pasando con éxito en ~2.59 segundos.
4. **Brecha técnica identificada para la etapa `tests` (GAP)**:
   - Las pruebas existentes cubren los flujos nominales y casos negativos estándar de forma secuencial y en escenarios controlados, pero carecen de pruebas sistemáticas de estrés bajo concurrencia masiva multihilo (16 workers simultáneos), inyección profunda de fallos de permisos de archivo en disco, cancelación asíncrona multihilo en tiempo real, verificación exhaustiva de fronteras en expresiones regulares y robustez ante documentos YAML profundamente degradados.

---

### Estado objetivo (TARGET)

1. **Suite exhaustiva de pruebas automatizadas y deterministas para Clang-Tidy**:
   - **Pruebas de frontera de expresiones regulares y robustez de configuración (`ClangTidyConfigurationTests.cpp`)**:
     - Validación en límites de expresiones regulares (`headerFilterRegex` vacío, expresiones multiancla complejas, y patrones con sintaxis inválida desbalanceada).
     - Validación de resistencia ante documentos YAML malformados, truncados, vacíos o con secuencias de escape corruptas.
     - Detección y rechazo riguroso de ataques complejos de Directory Traversal (`./../..`, `foo/../../bar`, `..\\windows\\path`, `/etc/shadow`, `/usr/bin/clang-tidy`, inyección de caracteres nulos).
     - Verificación de serialización roundtrip con variantes avanzadas de configuración (`WarningsAsErrors: '*'`, `FormatStyle: 'file'`, checks adicionales `cert-*`, `cppcoreguidelines-*`).
   - **Pruebas de concurrencia masiva y estrés multihilo (`ClangTidyServiceTests.cpp`)**:
     - 16 hilos (`std::thread`) concurrentes ejecutando simultáneamente sobre una única instancia compartida de `ClangTidyService`, combinando inspección en memoria pura, autofix en memoria e inspección dry-run en disco, certificando ausencia total de data races, contención segura mediante `std::mutex` interno y convergencia determinista.
   - **Pruebas de cancelación cooperativa multihilo y política Zero-Zombie (`ClangTidyServiceTests.cpp`)**:
     - Disparo asíncrono de `CancellationToken` desde un hilo secundario durante la inspección y corrección de árboles de directorios con múltiples archivos, comprobando interrupción inmediata con estado `TidyExecutionStatus::Cancelled`, sin procesos zombis ni fugas de recursos.
   - **Pruebas de inyección de fallos de permisos de archivo y recuperación (`ClangTidyServiceTests.cpp`)**:
     - Simulación de archivos con permisos restringidos de sólo lectura (`std::filesystem::perms::owner_read`), certificando la transición limpia a `PermissionDenied` o `WriteError`, preservación íntegra del archivo original y rollback determinista desde `.bak`.
   - **Pruebas de recorrido recursivo en árboles multinivel con exclusiones (`ClangTidyServiceTests.cpp`)**:
     - Creación sintética de jerarquías multinivel con archivos conformes, archivos no conformes, archivos no C++ (`.txt`, `.json`, `.png`) y carpetas excluidas (`_build`, `.git`, `generated`, `assets`), comprobando el recuento exacto de archivos examinados, corregidos e incidencias detectadas.
   - **Idempotencia de rollback y saneamiento de copias de seguridad (`ClangTidyServiceTests.cpp`)**:
     - Múltiples rollbacks sucesivos, rollback sin copia de seguridad previa y confirmación de eliminación física mediante `removeBackup`.
2. **Rendimiento, portabilidad y desacoplamiento**:
   - Tiempo de ejecución total de la suite < 500 ms en CTest.
   - Cero requerimientos de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) o inicialización de `QApplication` para las pruebas del núcleo.
   - Cumplimiento riguroso de la política Local-First (ADR-001 / Decisión Q-001).
3. **Auditoría de UX Copy**:
   - 100% de los mensajes de diagnóstico y remediación auditados contra `docs/UX-COPY-PUBLICO.md`, confirmando ausencia absoluta de términos de ingeniería interna.

---

### Brechas identificadas a resolver en APPLY (GAPS)

| Componente bajo prueba | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| Expresiones regulares de filtrado | Validación de regex sintácticamente inválidos básicos | Cobertura sistemática de fronteras exactas (regex vacío, multianclas, grupos) y fallos | Implementar `test_clang_tidy_validator_regex_boundaries` en `ClangTidyConfigurationTests.cpp`. |
| Robustez ante YAML corrupto | Solo se prueba omisión de claves obligatorias y colon faltante | Evaluación de documentos YAML truncados, vacíos o con caracteres nulos/binarios | Implementar `test_clang_tidy_malformed_yaml_inputs` en `ClangTidyConfigurationTests.cpp`. |
| Directory Traversal complejo | Prueba básica de `../../etc/.clang-tidy` y `/etc/shadow` | Prueba de ataques avanzados (`./../..`, `foo/../../bar`, `..\\`, rutas con nulos) | Implementar `test_clang_tidy_complex_path_traversal` en `ClangTidyConfigurationTests.cpp`. |
| Concurrencia masiva multihilo | Pruebas secuenciales monohilo en `ClangTidyServiceTests.cpp` | Prueba con 16 hilos simultáneos operando sobre la misma instancia | Implementar `test_tidy_service_massive_multithreading_concurrency` en `ClangTidyServiceTests.cpp`. |
| Estrés de cancelación cooperativa | Cancelación pre-activada en un solo hilo | Cancelación disparada concurrentemente desde hilo separado durante escaneo | Implementar `test_tidy_service_cooperative_cancellation_stress` en `ClangTidyServiceTests.cpp`. |
| Inyección de fallos de permisos | Solo se prueban fallos de cuota (`FileTooLarge`) | Fallo real de I/O forzando permisos de solo lectura (`perms::owner_read`) | Implementar `test_tidy_service_permission_failure_injection` en `ClangTidyServiceTests.cpp`. |
| Árbol recursivo multinivel | Solo se prueban subdirectorios planos inmediatos (`src/`, `include/`) | Jerarquía anidada profunda (`a/b/c/d/`) con mezcla de archivos C++, no C++ y carpetas excluidas | Implementar `test_tidy_service_recursive_directory_tree` en `ClangTidyServiceTests.cpp`. |
| Idempotencia de rollback | Prueba básica de creación y reversión de backup | Múltiples reversiones sucesivas, rollback sin backup y confirmación de borrado | Incorporar `test_tidy_service_backup_atomic_idempotence` en `ClangTidyServiceTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad C++20 y Qt6
- **Estándar C++20**: Uso estricto de `std::filesystem`, `std::string_view`, `std::thread`, `std::atomic`, `enum class` con subyacente fuertemente tipado (`uint8_t`), const-correctness y conceptos modernos.
- **Independencia del núcleo**: `vigilio_core` permanece 100% independiente de Qt GUI, Widgets y WebEngine. Las pruebas unitarias se ejecutan directamente sobre la biblioteca del núcleo sin requerir enlaces a Qt Widgets o Chromium.
- **Portabilidad de rutas**: Manipulación exclusiva mediante `std::filesystem::path` con operadores de concatenación portables (`/`), garantizando soporte indistinto en plataformas Linux, macOS y Windows.

### Estrategia de migración
- No existen cambios destructivos en las APIs existentes ni modificaciones en las firmas de `ClangTidyService`, `ClangTidyValidator` o `ClangTidyConfiguration`.
- Las nuevas pruebas amplían la cobertura de regresión y resiliencia en los archivos de prueba existentes (`ClangTidyConfigurationTests.cpp` y `ClangTidyServiceTests.cpp`), manteniéndose plenamente compatibles con la infraestructura de CMake y CTest.

### Plan de rollback determinista
- Si alguna prueba introduce inestabilidad, fallo de compilación o flakiness en entornos con limitaciones de recursos:
  1. Identificar la función de prueba afectada mediante el reporte detallado de CTest (`ctest --output-on-failure`).
  2. Revertir los cambios en los archivos de prueba afectados a su estado consolidado anterior mediante `git checkout HEAD -- apps/vigilio-browser/tests/unit/`.
  3. Revalidar compilación y suites completas con `apps/vigilio-browser/scripts/check.sh`.

---

## Arquitectura técnica de pruebas y matriz de resiliencia

### 1. Desacoplamiento de componentes bajo prueba

La arquitectura de pruebas aísla con precisión las responsabilidades:
- **`ClangTidyValidator`**: Componente funcional puro y estático. No realiza operaciones de E/S en disco salvo lectura inmutable de archivos de configuración autorizados. No posee estado mutable ni acoplamientos externos.
- **`ClangTidyConfiguration`**: Fábrica estática inmutable de configuración y filtros de rutas. No realiza E/S ni depende de hilos.
- **`ClangTidyService`**: Componente operativo con estado interno protegido por `std::mutex`. Coordina operaciones en memoria pura (sin efectos colaterales), inspección en modo dry-run (lectura inmutable) y corrección física transaccional (con copias de seguridad `.bak` y rollback).

### 2. Jerarquía y cobertura exhaustiva de pruebas para Clang-Tidy

#### Nivel 1: Validación estática, límites y YAML malformado (`ClangTidyConfigurationTests.cpp`)
- Evalúa la corrección del parser YAML ante entradas sintéticamente degradadas o truncadas.
- Verifica el comportamiento en las fronteras de expresiones regulares de filtrado (`headerFilterRegex`).
- Certifica el rechazo sin excepciones de cualquier ruta con patrones de escape de directorio.

#### Nivel 2: Capa operativa de servicio, concurrencia masiva y cancelación (`ClangTidyServiceTests.cpp`)
- Ejecuta pruebas multihilo con 16 workers simultáneos operando sobre la misma instancia de servicio.
- Simula cancelación asíncrona concurrente en operaciones de inspección y corrección de árboles de directorios, verificando la terminación limpia sin fugas de memoria ni hilos bloqueados.

#### Nivel 3: Resiliencia ante fallos de disco, permisos y reintentos (`ClangTidyServiceTests.cpp`)
- Simula situaciones adversas del sistema de archivos:
  - Archivos con permisos de sólo lectura que provocan errores de apertura en modo escritura.
  - Bloqueos transitorios simulados que requieren reintentos con retroceso exponencial (`retryBackoffMs`).
  - Verificación de la integridad del archivo original mediante la restauración atómica desde su copia de respaldo `.bak`.
  - Idempotencia de reversión ante múltiples invocaciones de `rollbackFile()`.

#### Nivel 4: Auditoría de integración y UX copy (`WindowTests.cpp` / `ClangTidyServiceTests.cpp`)
- Audita que todos los textos observables al usuario respeten estrictamente la política de lenguaje de producto en español neutro según `docs/UX-COPY-PUBLICO.md`.

### 3. Matriz de aserciones y comportamiento determinista

- Se prohíbe el uso de retardos fijos arbitrarios (`sleep`) para esperar condiciones; se utilizan primitivas atómicas (`std::atomic<bool>`) y sincronización determinista (`join`).
- Cada aserción utiliza la macro `CHECK(condition)` que lanza `std::runtime_error` descriptivo con el archivo y la línea exacta ante cualquier discrepancia.
- Todas las pruebas retornan códigos de salida estándar (0 en caso de éxito, 1 en caso de error) para una integración transparente con el arnés de CTest.

### 4. Resiliencia, saneamiento de recursos y garantía Zero-Zombie

- **Guardián RAII para directorios temporales (`TempDirGuard`)**:
  - Crea un subdirectorio aislado en `std::filesystem::temp_directory_path()` con nombre único formado por prefijo, PID y marca de tiempo en nanosegundos (`steady_clock`).
  - En su destructor, restablece los permisos de escritura en todos los subdirectorios y archivos antes de invocar `std::filesystem::remove_all`, asegurando que incluso en caso de fallos inyectados de permisos o excepciones, no queden residuos en el disco del sistema.
- **Garantía Zero-Zombie**:
  - Toda operación de escaneo o corrección respeta el token cooperativo de cancelación `CancellationToken`.
  - Cero subprocesos huérfanos o procesos esperando EOF indefinidamente.
