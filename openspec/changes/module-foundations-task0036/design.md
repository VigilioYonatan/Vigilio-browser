# Design — module-foundations-task0036

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Capa de Dominio y Servicio Operativo C++20 (`task0033` y `task0034`)**:
   - `CMakePresetsTypes.h`: Define formalmente los tipos del dominio (`CMakePresetKind`, `CMakePresetValidationOutcome`, `CMakePresetExecutionState`, `CMakePresetExecutionStatus`), la estructura de opciones (`CMakePresetCommandOptions`), el token de cancelación cooperativa (`CancellationToken`), las definiciones de preset en memoria (`CMakePresetDefinition`) y las estructuras de reporte (`CMakePresetsValidationReport`, `CMakePresetsSummary`).
   - `CMakePresetsValidator.h` y `.cpp`: Implementa validación estática de esquemas JSON v6 (y retrocompatibilidad con v3+), verificación de presets obligatorios (`core-debug`, `desktop-debug`), detección de referencias huérfanas en build/test presets, comprobación de nombres duplicados, verificación de higiene de rutas (`isSafeBinaryDir`), detección de variables de caché contradictorias y evaluación del archivo real del proyecto.
   - `CMakePresetsService.h` y `.cpp`: Proporciona la capa operativa desacoplada para carga de especificaciones, indexación y búsqueda de presets, resolución de jerarquías asociadas, síntesis determinista de líneas de comando CLI (`cmake --preset`, `cmake --build --preset`, `ctest --preset`), ejecución acotada con soporte para dry-run y cancelación cooperativa, y saneamiento seguro de directorios de build (`cleanBinaryDir`).
2. **Capa de Interfaz de Usuario y Presets de Compilación (`task0035`)**:
   - `CMakePresets.json` define 9 perfiles canónicos claramente categorizados entre entornos con interfaz gráfica (`VIGILIO_BUILD_GUI=ON`: `desktop-debug`, `desktop-release`, `desktop-e2e`) y entornos headless (`VIGILIO_BUILD_GUI=OFF`: `core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan`, `sanitizer-ubsan`).
   - `CMakeLists.txt` orquesta la compilación modular de `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y, condicionalmente, la interfaz Qt Widgets (`vigilio-browser`, `vigilio_ui_tests`).
   - `WindowTests.cpp` implementa detección determinista de display (`DISPLAY` / `WAYLAND_DISPLAY`) con reporte honesto de `NOT_EXECUTED` si falta entorno gráfico.
3. **Suites de Pruebas Existentes**:
   - `CMakePresetsTests.cpp` (555 líneas): Valida 11 escenarios centrados en la lógica estática del validador y benchmark de rendimiento básico.
   - `CMakePresetsServiceTests.cpp` (406 líneas): Valida 7 funciones operativas primarias (carga, búsqueda, síntesis CLI, dry-run, cancelación elemental, saneamiento seguro y formateo de diagnósticos).
   - `scripts/check.sh` ejecuta 13 suites de prueba en CTest de forma limpia y determinista.

---

### Estado objetivo (TARGET)

1. **Batería exhaustiva de pruebas de resiliencia y casos extremos (`vigilio_presets_tests`)**:
   - Cobertura completa de casos límite de entrada en `CMakePresetsValidator`:
     - Documentos JSON con sintaxis severamente truncada, caracteres nulos embebidos (`\0`), comillas desbalanceadas y arrays en el nivel raíz.
     - Inversión de tipos en estructuras JSON: claves numéricas en lugar de cadenas, booleanos no estándar, strings en campos donde se esperan arrays de objetos.
     - Sanitización rigurosa contra inyección de comandos shell en nombres de presets y variables (`&`, `;`, `|`, `` ` ``, `$()`).
     - Path traversal avanzado en `binaryDir`: secuencias complejas `${sourceDir}/build/../../secret`, rutas UNC de red (`\\server\share`) y rutas con caracteres de control.
     - Validación de macros: verificación de sustitución de `${sourceDir}`, `${presetName}`, `${generator}` y rechazo seguro de macros desconocidas o potencialmente recursivas.
2. **Pruebas de concurrencia multihilo y cancelación bajo estrés (`vigilio_presets_service_tests`)**:
   - Acceso concurrente multihilo sobre instancias compartidas de `CMakePresetsService`: 8 hilos ejecutando simultáneamente consultas (`findPreset`, `listPresetsByKind`, `buildConfigureCommandLine`, `summary`) sin carreras de datos ni corrupciones de memoria.
   - Cancelación cooperativa atómica bajo estrés: verificación de que múltiples llamadas concurrentes a `requestCancellation()` en `CancellationToken` son atómicas, idempotentes y detienen inmediatamente la ejecución operativa, retornando estado `Cancelled` y exitCode -1 con mensaje descriptivo.
3. **Inyección de fallos en sistema de archivos y recuperación operativa**:
   - Verificación de resiliencia en `cleanBinaryDir`: simulación de carpetas de compilación de solo lectura (`perms::owner_read`), rutas no existentes y archivos bloqueados.
   - Certificación de que el servicio no lanza excepciones no controladas (`std::runtime_error`, `std::filesystem::filesystem_error`), captura el fallo limpiamente y asigna la descripción técnica en `errorMessage`.
4. **Límites de búfer y política Zero-Zombie**:
   - Verificación de que el búfer de salida estándar y error respeta taxativamente el límite de 64 KB (`maxOutputBytes = 65536`), truncando ordenadamente la captura sin agotar la memoria RAM del sistema.
   - Verificación de expiración determinista por timeout (`timeoutMs`) con liberación garantizada de recursos del proceso.
5. **Auditoría de copy público y textos (`docs/UX-COPY-PUBLICO.md`)**:
   - Certificación automatizada de que todos los mensajes observables devueltos por `outcomeToString`, `executionStatusToString`, `executionStateToString` y `remediationHint` están redactados en español neutro comprensible para el usuario, sin fugas de términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).
6. **Aislamiento y rendimiento**:
   - Ejecución 100% headless sin requerir servidor X11/Wayland ni procesos Chromium/GPU.
   - Tiempo de ejecución de toda la suite de CMake Presets inferior a 250 ms en CTest.

---

### Gaps identificados (GAPS)

1. **Ausencia de pruebas de JSON con caracteres nulos o inversión de tipos en `CMakePresetsTests.cpp`**: La suite actual solo valida cadenas JSON vacías o llaves truncadas simples, sin probar caracteres binarios embebidos, estructuras con arrays en lugar de objetos, ni claves numéricas.
2. **Falta de verificación sistemática de inyección de comandos shell en nombres de presets**: No se comprueba actualmente el comportamiento de síntesis CLI ante presets maliciosos que contengan caracteres como `;`, `&`, `|` o `$()`.
3. **Ausencia de pruebas de concurrencia multihilo en `CMakePresetsServiceTests.cpp`**: No existe verificación automatizada que certifique la seguridad de lectura multihilo simultánea sobre una misma instancia de `CMakePresetsService`.
4. **Falta de pruebas de estrés en cancelación cooperativa**: La prueba actual de cancelación es secuencial y simple; no evalúa el comportamiento ante múltiples hilos cancelando o solicitando cancelación en ventanas de carrera atómica.
5. **Falta de pruebas de inyección de fallos de permisos en `cleanBinaryDir`**: No se valida la respuesta de `cleanBinaryDir` cuando el sistema de archivos deniega permisos de borrado (`read-only`), comprobando que se reporte `false` y un diagnóstico claro sin propagar excepciones.
6. **Ausencia de prueba automatizada de cumplimiento de `docs/UX-COPY-PUBLICO.md`**: No existe una prueba en la suite que verifique la ausencia de términos prohibidos de ingeniería en todos los mensajes observables de la capacidad.

---

## Compatibility, migration and rollback

### Compatibilidad y estándares C++20
- Implementación conforme a la norma ISO C++20 utilizando constructos portables: `<filesystem>`, `<chrono>`, `<thread>`, `<atomic>`, `<string>`, `<vector>`, `<map>`.
- Compatibilidad multiplataforma garantizada en Linux (GCC 12+, Clang 15+), Windows (MSVC 2022) y macOS (Apple Clang 14+).
- Cumplimiento de los flags estrictos de compilación del proyecto: `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror`.
- Cero macros complejas o frameworks pesados de pruebas; utilización del macro estándar `CHECK(condition)` con reporte exacto de línea y archivo en caso de discrepancia.

### Estrategia de migración
- Las nuevas pruebas amplían directamente los archivos de prueba existentes `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp` y `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`.
- No se introducen nuevos ejecutables ni targets en `CMakeLists.txt`, preservando la estabilidad del grafo de dependencias y de CTest.
- No se altera la firma de ninguna API pública en `CMakePresetsTypes.h`, `CMakePresetsValidator.h` ni `CMakePresetsService.h`.

### Plan de contingencia y rollback
- Dado que este pack solo añade pruebas de verificación automatizada en `tests/unit/`, cualquier regresión en la etapa APPLY se subsana eliminando o ajustando las aserciones de prueba sin impactar el código de producción ni requerir migraciones de datos.
- Reversión determinista mediante git checkout o eliminación del changeset en caso de fallo crítico en el pipeline.

---

## Arquitectura de pruebas y matriz de resiliencia

### Estructura modular de las pruebas

```
apps/vigilio-browser/tests/unit/
├── CMakePresetsTests.cpp
│   ├── [Existente] TEST-PRESET-001 .. 012 (Esquema, obligatorios, unicidad, rutas básicas, benchmark)
│   ├── [Nuevo]     TEST-PRESET-013: Sintaxis JSON corrompida extrema y bytes nulos
│   ├── [Nuevo]     TEST-PRESET-014: Inversión de tipos estructurales en JSON
│   ├── [Nuevo]     TEST-PRESET-015: Inyección de shell en nombres de preset y variables
│   ├── [Nuevo]     TEST-PRESET-016: Path traversal avanzado y rutas de red UNC
│   └── [Nuevo]     TEST-PRESET-017: Expansión de macros válidas y rechazo de desconocidas
└── CMakePresetsServiceTests.cpp
    ├── [Existente] TEST-PRESET-SVC-001 .. 012 (Carga, síntesis, dry-run, cancelación básica, clean)
    ├── [Nuevo]     TEST-PRESET-SVC-013: Consultas de lectura concurrentes multihilo (8 hilos)
    ├── [Nuevo]     TEST-PRESET-SVC-014: Estrés de cancelación cooperativa e idempotencia atómica
    ├── [Nuevo]     TEST-PRESET-SVC-015: Inyección de fallos de permisos en cleanBinaryDir (E/S de disco)
    ├── [Nuevo]     TEST-PRESET-SVC-016: Bounded buffers (64 KB) y política Zero-Zombie
    ├── [Nuevo]     TEST-PRESET-SVC-017: Invocaciones en estado no inicializado y presets inexistentes
    └── [Nuevo]     TEST-PRESET-SVC-018: Auditoría rigurosa de copy público (UX-COPY-PUBLICO.md)
```

### Detalles de implementación de casos clave

1. **Fixture de Sistema de Archivos Temporal RAII**:
   ```cpp
   struct ScopedTestDirectory {
       std::filesystem::path path;
       ScopedTestDirectory(const std::string& prefix) {
           path = std::filesystem::temp_directory_path() /
                  (prefix + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
           std::filesystem::create_directories(path);
       }
       ~ScopedTestDirectory() {
           std::error_code ec;
           std::filesystem::remove_all(path, ec);
       }
   };
   ```
2. **Prueba de Concurrencia Multihilo de Lectura**:
   - Instancia única de `CMakePresetsService` cargada con `CANONICAL_VALID_JSON`.
   - Lanzamiento de 8 hilos (`std::thread`), cada uno iterando 50 veces buscando presets por nombre y kind, generando líneas de comando y extrayendo el resumen.
   - Sincronización mediante `join()` y verificación de consistencia en todas las ejecuciones.
3. **Prueba de Saneamiento ante Permisos Denegados**:
   - Creación de subcarpeta de build sintética: `dir / "_build-readonly" / "CMakeFiles"`.
   - Modificación de permisos a solo lectura (`std::filesystem::perms::owner_read`).
   - Invocación a `svc.cleanBinaryDir("readonly-preset", &errMsg)`.
   - Verificación de que retorna `false`, `errMsg` contiene una descripción legible del fallo y la prueba no aborta por excepciones no capturadas.
   - Restauración de permisos antes de la salida del scope para permitir la limpieza RAII.
4. **Auditoría de UX-COPY-PUBLICO**:
   - Inspección exhaustiva de todas las cadenas devueltas por:
     - `CMakePresetsValidator::outcomeToString(outcome)` para los 8 valores del enum.
     - `CMakePresetsService::executionStatusToString(status)` para los 9 valores del enum.
     - `CMakePresetsService::executionStateToString(state)` para los 8 valores del enum.
   - Aserción de que ninguna cadena contiene términos de ingeniería prohibidos (`Senior Mode`, `Modo Senior`, `Prompt Builder`, `OpenSpec`, `DAG`, `toolchain`).
