# Design — module-foundations-task0044

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Modelo de dominio, validador estático y servicio operativo de Clang-Format (`task0041`, `task0042`, `task0043`)**:
   - `vigilio_core` cuenta con `src/core/ClangFormatTypes.h`, que establece enumeraciones fuertemente tipadas (`ClangFormatStylePreset`, `PointerAlignmentPolicy`, `ShortFunctionPolicy`, `ClangFormatValidationState`, `ClangFormatError`, `FormatExecutionStatus`), estructuras inmutables de configuración e inspección (`ClangFormatOptions`, `ClangFormatValidationReport`, `SourceFileFormatInspection`, `FormatServiceOptions`, `FormatOperationResult`) y el token cooperativo de cancelación multihilo `CancellationToken`.
   - `src/core/ClangFormatValidator.h/.cpp` implementa la validación estática de opciones, comprobación de rutas seguras (`isSafeConfigPath`), prevención de Directory Traversal (`..`), parser básico de sintaxis YAML, serializador y formateador de diagnósticos en español neutro (`formatErrorToString`).
   - `src/core/ClangFormatConfiguration.h/.cpp` genera la configuración canónica inmutable del proyecto (LLVM, indentWidth 4, columnLimit 105, PointerAlignment Left, C++20), valida extensiones compatibles (`.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`) y exclusiones de carpetas (`_build`, `build`, `generated`, `.git`, `.prompt-runner`).
   - `src/core/ClangFormatService.h/.cpp` proporciona la capa operativa desacoplada con formateo en memoria pura (`formatSourceCode`), inspección dry-run en memoria y en disco (`inspectSource`, `inspectFile`, `inspectDirectory`), formateo físico con respaldo (`formatFile`, `formatDirectory`), formateo con reintentos exponenciales ante contención (`formatWithRetry`), gestión atómica de respaldos y reversión (`createBackup`, `rollbackFile`, `removeBackup`), síntesis CLI (`buildClangFormatCommandLine`) y textos públicos de estado (`executionStatusToString`).
2. **Suites de pruebas básicas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/ClangFormatConfigurationTests.cpp`: 10 pruebas unitarias enfocadas en opciones canónicas, límites básicos, parser YAML canónico, omisión de claves, rutas inseguras, extensiones y UX copy.
   - `tests/unit/ClangFormatServiceTests.cpp`: 12 pruebas unitarias enfocadas en inicialización, formateo en memoria, dry-run conforme/discrepante, formateo físico, backup/rollback, reintento básico, cancelación preliminar, cuotas de archivo, rutas inseguras, síntesis CLI y textos públicos.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario con verificación programática dry-run de conformidad estilística de los archivos de UI y auditoría de textos públicos de remediación.
3. **Estado de compilación y pruebas continuas**:
   - Compilación limpia bajo C++20 con `-Werror` y cero advertencias.
   - 17 de 17 pruebas en CTest pasando con éxito en ~1.06 segundos.
4. **Brecha técnica identificada para la etapa `tests` (GAP)**:
   - Las pruebas existentes cubren los flujos nominales y casos negativos estándar, pero carecen de pruebas sistemáticas de estrés, concurrencia masiva multihilo, inyección profunda de fallos de permisos y verificación rigurosa de límites de frontera numérica en la configuración.

---

### Estado objetivo (TARGET)

1. **Suite exhaustiva de pruebas automatizadas y deterministas para Clang-Format**:
   - **Pruebas de frontera numérica y robustez de configuración (`ClangFormatConfigurationTests.cpp`)**:
     - Validación en las fronteras exactas permitidas ($1, 16, 20, 500$) y rechazo estricto fuera de rango ($0, 17, 19, 501$, valores negativos).
     - Validación de resistencia ante documentos YAML malformados, truncados, vacíos o con secuencias de escape corruptas.
     - Detección y rechazo riguroso de ataques complejos de Directory Traversal (`./../..`, `foo/../../bar`, `..\\windows\\path`).
     - Verificación de serialización roundtrip con variantes no estándar (`PointerAlignmentPolicy::Right`, `ShortFunctionPolicy::Inline`).
   - **Pruebas de concurrencia masiva y estrés multihilo (`ClangFormatServiceTests.cpp`)**:
     - 16 hilos (`std::thread`) concurrentes ejecutando simultáneamente sobre una única instancia compartida de `ClangFormatService`, combinando formateo en memoria pura e inspección dry-run, certificando ausencia total de data races, contención segura mediante `std::mutex` interno y convergencia determinista.
   - **Pruebas de cancelación cooperativa multihilo y política Zero-Zombie (`ClangFormatServiceTests.cpp`)**:
     - Disparo asíncrono de `CancellationToken` desde un hilo secundario durante la inspección y formateo de árboles de directorios con múltiples archivos, comprobando interrupción inmediata con estado `FormatExecutionStatus::Cancelled`, sin procesos zombis ni fugas de recursos.
   - **Pruebas de inyección de fallos de permisos de archivo y recuperación (`ClangFormatServiceTests.cpp`)**:
     - Simulación de archivos con permisos restringidos de sólo lectura (`std::filesystem::perms::owner_read`), certificando la transición limpia a `PermissionDenied` o `WriteError`, preservación íntegra del archivo original y rollback determinista desde `.bak`.
   - **Pruebas de recorrido recursivo en árboles multinivel con exclusiones (`ClangFormatServiceTests.cpp`)**:
     - Creación sintética de jerarquías multinivel con archivos conformes, archivos no conformes, archivos no C++ (`.txt`, `.json`) y carpetas excluidas (`_build`, `.git`), comprobando el recuento exacto de archivos examinados, formateados y discrepancias detectadas.
   - **Idempotencia de rollback y saneamiento de copias de seguridad (`ClangFormatServiceTests.cpp`)**:
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
| Frontera numérica de configuración | Límites básicos ($0$ e inválidos) probados de forma aislada | Cobertura sistemática de fronteras exactas ($1, 16, 20, 500$) y fuera de rango ($0, 17, 19, 501$) | Implementar `test_clang_format_validator_boundary_values` en `ClangFormatConfigurationTests.cpp`. |
| Robustez ante YAML corrupto | Solo se prueba omisión de claves obligatorias | Evaluación de documentos YAML truncados, vacíos o con caracteres nulos | Implementar `test_clang_format_malformed_yaml_inputs` en `ClangFormatConfigurationTests.cpp`. |
| Directory Traversal complejo | Prueba básica de `../../etc/passwd` | Prueba de ataques avanzados (`./../..`, `foo/../../bar`, `..\\`) | Implementar `test_clang_format_complex_path_traversal` en `ClangFormatConfigurationTests.cpp`. |
| Concurrencia masiva multihilo | Pruebas secuenciales monohilo en `ClangFormatServiceTests.cpp` | Prueba con 16 hilos simultáneos operando sobre la misma instancia | Implementar `test_format_service_massive_multithreading_concurrency` en `ClangFormatServiceTests.cpp`. |
| Estrés de cancelación cooperativa | Cancelación pre-activada en un solo hilo | Cancelación disparada concurrentemente desde hilo separado durante escaneo | Implementar `test_format_service_cooperative_cancellation_stress` en `ClangFormatServiceTests.cpp`. |
| Inyección de fallos de permisos | Solo se prueban fallos de cuota (`FileTooLarge`) | Fallo real de I/O forzando permisos de solo lectura (`perms::owner_read`) | Implementar `test_format_service_permission_failure_injection` en `ClangFormatServiceTests.cpp`. |
| Árbol recursivo multinivel | Solo se prueban archivos planos en la raíz temporal | Jerarquía anidada (`a/b/c/`) con mezcla de archivos C++, no C++ y carpetas excluidas | Implementar `test_format_service_recursive_directory_tree` en `ClangFormatServiceTests.cpp`. |
| Idempotencia de rollback | Prueba básica de creación y reversión de backup | Múltiples reversiones sucesivas, rollback sin backup y confirmación de borrado | Incorporar aserciones de idempotencia y saneamiento en `ClangFormatServiceTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad C++20 y Qt6
- **Estándar C++20**: Uso estricto de `std::filesystem`, `std::string_view`, `std::thread`, `std::atomic`, `enum class` con subyacente fuertemente tipado (`uint8_t`), const-correctness y conceptos modernos.
- **Independencia del núcleo**: `vigilio_core` permanece 100% independiente de Qt GUI, Widgets y WebEngine. Las pruebas unitarias se ejecutan directamente sobre la biblioteca del núcleo sin requerir enlaces a Qt Widgets o Chromium.
- **Portabilidad de rutas**: Manipulación exclusiva mediante `std::filesystem::path` con operadores de concatenación portables (`/`), garantizando soporte indistinto en plataformas Linux, macOS y Windows.

### Estrategia de migración
- No existen cambios destructivos en las APIs existentes ni modificaciones en las firmas de `ClangFormatService`, `ClangFormatValidator` o `ClangFormatConfiguration`.
- Las nuevas pruebas amplían la cobertura de regresión y resiliencia en los archivos de prueba existentes (`ClangFormatConfigurationTests.cpp` y `ClangFormatServiceTests.cpp`), manteniéndose plenamente compatibles con la infraestructura de CMake y CTest.

### Plan de rollback determinista
- Si alguna prueba introduce inestabilidad, fallo de compilación o flakiness en entornos con limitaciones de recursos:
  1. Identificar la función de prueba afectada mediante el reporte detallado de CTest (`ctest --output-on-failure`).
  2. Revertir los cambios en los archivos de prueba afectados a su estado consolidado anterior mediante `git checkout HEAD -- apps/vigilio-browser/tests/unit/`.
  3. Revalidar compilación y suites completas con `apps/vigilio-browser/scripts/check.sh`.

---

## Arquitectura técnica de pruebas y matriz de resiliencia

### 1. Desacoplamiento de componentes bajo prueba

La arquitectura de pruebas aísla con precisión las responsabilidades:
- **`ClangFormatValidator`**: Componente funcional puro y estático. No realiza operaciones de E/S en disco salvo lectura inmutable de archivos de configuración autorizados. No posee estado mutable ni acoplamientos externos.
- **`ClangFormatConfiguration`**: Fábrica estática inmutable de configuración y filtros de rutas. No realiza E/S ni depende de hilos.
- **`ClangFormatService`**: Componente operativo con estado interno protegido por `std::mutex`. Coordina operaciones en memoria pura (sin efectos colaterales), inspección en modo dry-run (lectura inmutable) y formateo físico transaccional (con copias de seguridad `.bak` y rollback).

### 2. Jerarquía y cobertura exhaustiva de pruebas para Clang-Format

#### Nivel 1: Validación estática, límites y YAML malformado (`ClangFormatConfigurationTests.cpp`)
- Evalúa la corrección del parser YAML ante entradas sintéticamente degradadas.
- Verifica el comportamiento en las fronteras matemáticas de los parámetros de configuración.
- Certifica el rechazo sin excepciones de cualquier ruta con patrones de escape de directorio.

#### Nivel 2: Capa operativa de servicio, concurrencia masiva y cancelación (`ClangFormatServiceTests.cpp`)
- Ejecuta pruebas multihilo con 16 workers simultáneos operando sobre la misma instancia de servicio.
- Simula cancelación inmediata en operaciones de inspección y formateo de árboles de directorios, verificando la terminación limpia sin fugas de memoria ni hilos bloqueados.

#### Nivel 3: Resiliencia ante fallos de disco, permisos y reintentos (`ClangFormatServiceTests.cpp`)
- Simula situaciones adversas del sistema de archivos:
  - Archivos con permisos de sólo lectura que provocan errores de apertura en modo escritura.
  - Bloqueos transitorios simulados que requieren reintentos con retroceso exponencial (`retryBackoffMs`).
  - Verificación de la integridad del archivo original mediante la restauración atómica desde su copia de respaldo `.bak`.

#### Nivel 4: Auditoría de integración y UX copy (`WindowTests.cpp` / `ClangFormatServiceTests.cpp`)
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
  - Toda operación de escaneo o formateo respeta el token cooperativo de cancelación `CancellationToken`.
  - Cero subprocesos huérfanos o procesos esperando EOF indefinidamente.
