# Change Proposal — module-foundations-task0036

Pack: `module.foundations.task0036`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0036 — Probar casos positivos, negativos y recuperación: CMake presets`).

Habiéndose completado exitosamente la especificación y contratos canónicos (`task0033`), el diseño e implementación desacoplada del validador y servicio operativo (`task0034`), y la alineación de targets de compilación y contratos de interfaz gráfica (`task0035`), el objetivo estricto de este pack es **diseñar y planificar una batería exhaustiva de pruebas automatizadas, deterministas y de alta resiliencia para CMake presets**, asegurando la estabilidad del flujo de construcción y verificación del navegador sin depender de procesos pesados ni entornos gráficos.

El alcance comprende:
- **Casos positivos y ciclo de vida integral de presets**:
  - Validación exhaustiva de los 9 perfiles canónicos definidos en `apps/vigilio-browser/CMakePresets.json`: `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan` y `sanitizer-ubsan`.
  - Verificación de consistencia cruzada entre `configurePresets`, `buildPresets` y `testPresets`, comprobando que cada perfil hijo resuelva unívocamente a su configuración padre.
  - Comprobación de variables de compilación críticas por perfil (`CMAKE_BUILD_TYPE`, `VIGILIO_BUILD_GUI`, `BUILD_TESTING`, `VIGILIO_ENABLE_E2E`, `VIGILIO_WARNINGS_AS_ERRORS`, `CMAKE_CXX_FLAGS`).
  - Síntesis determinista de líneas de comando CLI para `cmake --preset`, `cmake --build --preset` y `ctest --preset`, incluyendo inyección de argumentos adicionales (`additionalArgs`) y variables de entorno (`environmentOverrides`).
- **Casos negativos y entradas malformadas**:
  - Evaluación ante documentos JSON con sintaxis rota, claves sin cerrar, caracteres nulos embebidos (`\0`), secuencias de escape corruptas y estructuras de datos con tipos invertidos (v. gr. arrays en lugar de objetos, enteros en lugar de cadenas, booleanos numéricos).
  - Detección rigurosa de inyección de comandos shell o caracteres maliciosos en nombres de presets y variables (`&`, `;`, `|`, `` ` ``, `$()`, saltos de línea).
  - Rechazo estricto de rutas de compilación inseguras (`isSafeBinaryDir`): directory traversal (`../`), prefijos engañosos, rutas absolutas fijadas al host POSIX (`/etc`, `/var`, `/tmp`) o Windows (`C:\Windows`, rutas UNC `\\server\share`).
  - Detección de variables contradictorias (`VIGILIO_BUILD_GUI=OFF` junto con `VIGILIO_ENABLE_E2E=ON`) y macros no reconocidas (`${unknownMacro}`).
- **Concurrencia, reentrancia y cancelación cooperativa**:
  - Pruebas multihilo de lectura y resolución sobre `CMakePresetsService` (`findPreset`, `summary`, `listPresetsByKind`, `buildConfigureCommandLine`) ejecutadas simultáneamente desde múltiples hilos concurrentes sin condiciones de carrera.
  - Cancelación cooperativa multihilo mediante `CancellationToken`: interrupción atómica en transiciones de estado, idempotencia ante múltiples llamadas consecutivas a `requestCancellation()`, y código de retorno `Cancelled` (-1) con mensaje explicativo.
- **Recuperación, saneamiento y resiliencia del sistema de archivos**:
  - Operación `cleanBinaryDir`: saneamiento seguro e idempotente de carpetas temporales de compilación (`build-*`, `_build*`).
  - Simulación de fallos de E/S en disco: directorios de solo lectura (`read-only`), rutas inaccesibles o fallos de permisos durante el saneamiento, garantizando que el servicio reporte `IOError` de forma ordenada, con mensaje explicativo y sin arrojar excepciones no controladas.
  - Preservación incondicional de rutas que no pertenezcan al árbol de build autorizado.
- **Límites de memoria y política Zero-Zombie**:
  - Truncado determinista de búferes de salida a 64 KB (`maxOutputBytes = 65536`) para evitar agotamiento de memoria ante procesos con salida masiva.
  - Expiración determinista por timeout (`timeoutMs`) con liberación garantizada de recursos y estado `Timeout`.
- **Desacoplamiento arquitectónico y ejecución sin Chromium**:
  - Certificación de ejecución 100% en CTest headless (< 150 ms) sin requerir `DISPLAY`, `WAYLAND_DISPLAY`, `QApplication` ni procesos WebEngine/GPU.
  - Uso de fixtures locales RAII aislados en `/tmp` con limpieza garantizada al término de cada prueba.
  - Aislamiento absoluto y política local-first (Decisión Q-001): cero sockets de red externa, cero dependencias cloud y cero telemetría.

---

## Out of scope

- Modificación de contratos de negocio existentes o tipos de datos en `CMakePresetsTypes.h`, `CMakePresetsValidator.h` o `CMakePresetsService.h` (salvo que una prueba identifique una discrepancia formal con la especificación).
- Implementación de controles o menús de CMake en la interfaz gráfica pública del navegador (prohibido taxativamente por `docs/UX-COPY-PUBLICO.md`).
- Ejecución de compilaciones o suites de prueba de larga duración reales dentro de las pruebas unitarias (los tests operan con fixtures sintéticos y modo `dryRun` o comandos livianos).
- Integración con servicios de nube, autenticación remota o telemetría (estrictamente denegado por ADR-001 / Decisión Q-001).
- Modificación de código fuente fuera de las rutas autorizadas para esta etapa.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Especificación canónica v6 (`apps/vigilio-browser/CMakePresets.json`)**:
   - Define formalmente 9 perfiles de configuración: `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan` y `sanitizer-ubsan`.
   - 9 presets de compilación correspondientes y 7 presets de prueba estandarizados.
2. **Dominio desacoplado (`apps/vigilio-browser/src/core/`)**:
   - `CMakePresetsTypes.h`: Contiene enumeraciones fuertemente tipadas (`CMakePresetKind`, `CMakePresetValidationOutcome`, `CMakePresetExecutionState`, `CMakePresetExecutionStatus`), token ligero de cancelación (`CancellationToken`), opciones configurables (`CMakePresetCommandOptions`) y estructuras inmutables de resultados (`CMakePresetExecutionResult`, `CMakePresetsValidationReport`, `CMakePresetsSummary`).
   - `CMakePresetsValidator.h` / `.cpp`: Validador estático libre de dependencias de GUI o Chromium, encargado de verificar versión de esquema ($\ge 3$), presencia de presets obligatorios (`core-debug`, `desktop-debug`), resolución de referencias huérfanas, unicidad de nombres, higiene de `binaryDir` y coherencia de variables de caché.
   - `CMakePresetsService.h` / `.cpp`: Servicio operativo con estado para carga de archivos/JSON, búsqueda indexada, síntesis CLI determinista, ejecución acotada con dry-run, cancelación multihilo y saneamiento seguro de directorios de build (`cleanBinaryDir`).
3. **Suites de prueba unitarias existentes (`apps/vigilio-browser/tests/unit/`)**:
   - `CMakePresetsTests.cpp` (555 líneas): Cubre 11 casos de prueba centrados en el validador (`test_presets_canonical_valid`, `test_presets_invalid_schema_version`, `test_presets_missing_required_preset`, `test_presets_orphan_configure_reference`, `test_presets_duplicate_name`, `test_presets_unsafe_binary_dir`, `test_presets_conflicting_variables`, `test_presets_malformed_json`, `test_presets_project_file_validation`, `test_presets_performance_benchmark`, `test_presets_helper_methods_and_types`).
   - `CMakePresetsServiceTests.cpp` (406 líneas): Cubre 7 funciones de prueba para el servicio (`test_service_load_canonical`, `test_service_find_and_resolve`, `test_service_command_line_synthesis`, `test_service_dry_run`, `test_service_cancellation`, `test_service_clean_binary_dir`, `test_service_diagnostics_formatting`).
4. **Verificación de pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GCC 15.2.0 bajo C++20 con `-Werror`, cero advertencias de compilador y Clang-Format verificado.
   - 13 de 13 pruebas unitarias e integración en CTest pasando exitosamente en 0.44s (100% pass, exit code 0).
5. **Validación de Prompt Builder CLI**:
   - `validate-references` certifica 36 referencias canónicas válidas (`valid: true`).
   - `verify-lock` certifica la integridad del archivo lock `0106-module.foundations.task0036.propose.lock.json` (`valid: true`).

---

## Alcance exacto

La propuesta formaliza el incremento de la etapa **tests** para alcanzar cobertura exhaustiva de resiliencia, robustez y recuperación de CMake presets en el módulo `foundations`:

1. **Batería de robustez en el Validador (`CMakePresetsTests.cpp`)**:
   - `test_presets_corrupted_json_syntax_variations`: evaluación de documentos JSON con caracteres binarios, terminaciones abruptas, llaves desbalanceadas, comillas faltantes, comentarios no estándar y arrays en la raíz.
   - `test_presets_type_inversion_and_malformed_structures`: validación ante `version` como cadena ("6"), `configurePresets` como objeto asociativo en lugar de array, `generator` no soportado y `cacheVariables` con tipos numéricos o nulos.
   - `test_presets_security_injection_and_path_hygiene`: verificación de rechazo de inyección de comandos shell en nombres de preset (`; rm -rf /`, `&& ls`, `| cat`, `$(whoami)`) y rutas complejas de escape (`${sourceDir}/build/../../secret`).
   - `test_presets_macro_expansion_edge_cases`: análisis de macros estándar (`${sourceDir}`, `${presetName}`, `${generator}`) y rechazo de macros desconocidas o recursivas.
2. **Batería de resiliencia y concurrencia en el Servicio (`CMakePresetsServiceTests.cpp`)**:
   - `test_service_concurrency_multi_threaded_reads`: múltiples hilos ejecutando concurrentemente consultas (`findPreset`, `listPresetsByKind`, `buildConfigureCommandLine`, `summary`) sobre una misma instancia de `CMakePresetsService` sin carreras de datos.
   - `test_service_cancellation_stress_and_idempotence`: verificación de que múltiples llamadas concurrentes a `requestCancellation()` sobre `CancellationToken` son atómicas, idempotentes y detienen inmediatamente la ejecución en modo dry-run / real, retornando estado `Cancelled` y exitCode `-1`.
   - `test_service_filesystem_io_errors_and_recovery`: simulación de directorios de solo lectura y rutas bloqueadas durante `cleanBinaryDir`, certificando que retorna `false` limpiamente, asigna mensaje explicativo en `errorMessage` y no propaga excepciones no capturadas.
   - `test_service_bounded_buffers_and_zero_zombie`: comprobación del límite estricto de captura de salida estándar a 64 KB (`maxOutputBytes`), truncado seguro sin desbordamiento de búfer y respeto del límite de tiempo `timeoutMs`.
   - `test_service_uninitialized_and_invalid_state_transitions`: verificación del comportamiento de invocaciones a métodos de ejecución (`executeConfigure`, `executeBuild`, `executeTest`) sobre un servicio no cargado o ante presets inexistentes, validando que retornan `PresetNotFound` o `ValidationError` sin violaciones de acceso a memoria.
3. **Auditoría de textos y copy público (`UX-COPY-PUBLICO.md`)**:
   - Certificación de que las descripciones textuales de estado y error (`outcomeToString`, `executionStatusToString`, `executionStateToString`, `remediationHint`) están redactadas en español neutro comprensible, sin jerga interna ni etiquetas prohibidas.

---

## Diseño completo de la capacidad

El diseño de la batería de pruebas organiza las verificaciones en suites de pruebas modulares, deterministas y de ejecución inmediata:

```
+-------------------------------------------------------------------------------+
|                       CMake Presets Testing Architecture                     |
+-------------------------------------------------------------------------------+
|                                                                               |
|  [ Unit Tests: Validator ]                     [ Unit Tests: Service ]       |
|  (CMakePresetsTests.cpp)                       (CMakePresetsServiceTests.cpp) |
|  - Valid Schema & Canonicity                   - Load Memory / Disk           |
|  - Corrupted Syntax & Inversion                - Synthesis & Macro Expansion  |
|  - Security & Injection Defense                - Multi-threaded Read Access   |
|  - Path Hygiene & Traversal                    - Cancellation Under Stress    |
|  - Conflicting Cache Variables                 - Filesystem IO Failures       |
|  - Performance Benchmark (<150ms)              - Bounded Buffers (64KB limit) |
|                                                - Recovery & cleanBinaryDir    |
|                                                                               |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                       Quality & Safety Requirements                           |
|  * 100% Headless execution in CTest without X11/Wayland display              |
|  * Execution time < 250 ms for entire test suite                              |
|  * RAII TempDirectory fixture in /tmp with automatic deterministic cleanup   |
|  * Zero external network sockets / Local-First invariant (Q-001)             |
|  * Full compliance with docs/UX-COPY-PUBLICO.md                              |
+-------------------------------------------------------------------------------+
```

### Entidades y contratos de prueba

1. **Fixture RAII `TempPresetsDirectory`**:
   - Estructura auxiliar para crear directorios temporales `/tmp/vigilio_presets_test_XXXXXX` usando `std::filesystem::temp_directory_path()`.
   - Genera archivos `CMakePresets.json`, `CMakeLists.txt`, `CMakeCache.txt` y subcarpetas `CMakeFiles/` sintéticas.
   - Su destructor invoca `std::filesystem::remove_all()` ignorando códigos de error, garantizando contención estricta y limpieza total.
2. **Pruebas de Inyección y Sanitización de Comandos**:
   - Simulación de nombres de preset hostiles:
     - `core-debug; rm -rf /`
     - `core-debug && whoami`
     - `core-debug | cat /etc/passwd`
     - `core-debug$(calc)`
   - El sintetizador `buildConfigureCommandLine` debe entrecomillar o sanear tokens, o el validador debe rechazar presets con caracteres fuera del conjunto alfanumérico seguro `[a-zA-Z0-9_\-\.]`.
3. **Pruebas de Concurrencia y Sincronización**:
   - Lanzamiento de 8 hilos ejecutando 50 ciclos de lectura simultánea (`findPreset`, `summary`, `buildConfigureCommandLine`) sobre `CMakePresetsService`.
   - Comprobación de integridad de los resultados retornados; cero segfaults, cero lecturas sucias.
4. **Pruebas de Saneamiento y Resiliencia en E/S**:
   - Creación de estructura de directorios en `/tmp/vigilio_io_test/_build-locked/CMakeFiles`.
   - Modificación de permisos de archivo con `std::filesystem::permissions` a solo lectura (`perms::owner_read`).
   - Invocación a `cleanBinaryDir`: verificación de retorno `false`, preservación del proceso sin abortar, captura del error de E/S en `errorMessage` y restauración de permisos para limpieza en el teardown.

---

## Archivos afectados

La fase APPLY de este pack se limitará estrictamente a los archivos autorizados:

1. `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`:
   - Incorporación de casos de prueba negativos para sintaxis JSON corrompida, tipos invertidos, inyección de comandos shell y macros inválidas.
2. `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`:
   - Incorporación de pruebas de concurrencia multihilo, estrés de cancelación atómica, inyección de errores de permisos en `cleanBinaryDir`, límites de búfer de 64 KB y transiciones de estado anómalas.
3. `docs/rules/01-modules/foundations/micro/0036-cmake-presets/task.md`:
   - Sincronización documental canónica del resultado de la etapa `tests`.

*Nota:* Durante esta fase PROPOSE, la escritura está restringida exclusivamente a `openspec/changes/module-foundations-task0036/**`.

---

## Contrato UX/UI

- **Sin cambio visual directo en la aplicación**: CMake Presets es una capacidad de infraestructura de construcción y soporte operativo del núcleo. No tiene representación gráfica directa en la barra de herramientas ni menús principales de `BrowserWindow`.
- **Textos y copy público**: Todos los mensajes de estado, diagnósticos de error y pistas de recuperación expuestos a través de `outcomeToString`, `executionStatusToString`, `executionStateToString` y `remediationHint` deben cumplir rigurosamente con `docs/UX-COPY-PUBLICO.md`:
  - Uso de español claro, respetuoso y comprensible para el usuario general.
  - Cero etiquetas de ingeniería prohibidas: «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain».
  - Verificación automatizada en las pruebas unitarias de ausencia de estas cadenas prohibidas.

---

## Reglas y fallos

1. **Aislamiento de procesos y memoria**:
   - Prohibido dejar procesos zombie o esperar entrada de consola indefinidamente (Zero-Zombie).
   - Todo comando sintético o simulado debe contar con timeout explícito ($\le 60$ s) y límite estricto de captura ($\le 64$ KB).
2. **Manejo de errores del sistema de archivos**:
   - Errores de permisos o directorios bloqueados deben mapearse a `CMakePresetExecutionStatus::IOError` o retorno booleano `false` con diagnóstico en `errorMessage`.
   - Ninguna función de servicio debe lanzar excepciones sin capturar (`throw`) al invocar operaciones de disco.
3. **Determinismo y reproducibilidad**:
   - Los tests deben arrojar exactamente el mismo resultado independientemente del orden de ejecución, la presencia de servidor gráfico o la velocidad de la máquina anfitriona.
   - Todos los paths utilizados en las pruebas deben ser sintéticos o resolverse relativos al directorio del proyecto.

---

## Seguridad y privacidad

- **Política Local-First (ADR-001 / Decisión Q-001)**:
  - Las pruebas de CMake Presets no generan tráfico de red, no abren puertos de escucha y no recopilan telemetría.
- **Defensa contra Path Traversal e Inyección**:
  - Toda ruta binaria `binaryDir` debe ser contenida en subdirectorios `build-*` o `_build*`.
  - Nombres de presets y argumentos deben filtrarse contra secuencias de inyección de shell.
- **Privacidad e Incógnito**:
  - Las operaciones de build no acceden a perfiles de usuario, cookies, historial ni bases de datos de navegación.

---

## Matriz de pruebas

| Identificador | Nivel | Propósito de la prueba | Fixture / Entrada | Resultado esperado |
| :--- | :--- | :--- | :--- | :--- |
| `TEST-PRESET-013` | Unitario | Sintaxis JSON rota, binarios nulos y llaves huérfanas | JSON con `\0` y llaves sin cerrar | `MalformedJson`, `isValid() == false` |
| `TEST-PRESET-014` | Unitario | Inversión de tipos en esquema (array por objeto, string por entero) | JSON con `"version": "6"` y arrays en presets | `InvalidSchemaVersion` o `MalformedJson` |
| `TEST-PRESET-015` | Unitario | Inyección de comandos shell en nombres de preset | `core-debug; rm -rf /` | Rechazo seguro o saneamiento |
| `TEST-PRESET-016` | Unitario | Path traversal avanzado y rutas de red UNC | `${sourceDir}/build/../../secret`, `\\share\build` | `UnsafeBinaryDir`, `isValid() == false` |
| `TEST-PRESET-SVC-013` | Concurrencia | Lectura multihilo simultánea en `CMakePresetsService` | 8 hilos realizando 50 consultas concurrentes | Cero colisiones, datos íntegros |
| `TEST-PRESET-SVC-014` | Concurrencia | Cancelación atómica y cooperativa bajo estrés | Hilo de ejecución + hilo cancelador | Estado `Cancelled`, exitCode -1 |
| `TEST-PRESET-SVC-015` | Resiliencia | Fallo de permisos en `cleanBinaryDir` | Directorio con permisos solo lectura (`0444`) | Retorno `false`, `IOError` limpio sin crash |
| `TEST-PRESET-SVC-016` | Robustez | Límite estricto de captura de 64 KB y timeouts | Salida simulada masiva de 256 KB | Truncado seguro a 64 KB, sin desbordamiento |
| `TEST-PRESET-SVC-017` | Estado | Invocación de operaciones sobre servicio no cargado | `CMakePresetsService` sin cargar JSON | `PresetNotFound` o `ValidationError` |
| `TEST-PRESET-SVC-018` | Calidad | Cumplimiento estricto de `UX-COPY-PUBLICO.md` | Mensajes de diagnóstico de servicio y validador | Cero etiquetas de ingeniería prohibidas |
| `TEST-PRESET-SVC-019` | Regresión | Ejecución rápida sin display ni Chromium | CTest headless con todas las suites activas | 100% PASS en $< 250$ ms |

---

## Pasos APPLY

En la etapa APPLY subsiguiente (`task0036`), se ejecutarán los siguientes pasos deterministas:

1. **Implementar pruebas de robustez en el Validador (`CMakePresetsTests.cpp`)**:
   - Añadir funciones de prueba para sintaxis corrupta, tipos invertidos, inyección de comandos shell y path traversal avanzado.
   - Registrar aserciones `CHECK` deterministas en el punto de entrada `main()`.
2. **Implementar pruebas de concurrencia y resiliencia en el Servicio (`CMakePresetsServiceTests.cpp`)**:
   - Añadir funciones de prueba multihilo para consultas concurrentes y cancelación atómica bajo estrés con `std::thread`.
   - Añadir pruebas de inyección de fallos de permisos en `cleanBinaryDir` y límites de búfer de salida (64 KB).
   - Añadir pruebas de auditoría de copy público según `docs/UX-COPY-PUBLICO.md`.
   - Registrar las nuevas pruebas en el punto de entrada `main()`.
3. **Compilación y verificación completa con el arnés local**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` asegurando compilación sin advertencias bajo C++20 con `-Werror` y aprobación del 100% de tests en CTest.
4. **Validación de referencias y lockfile**:
   - Ejecutar el CLI de Prompt Builder (`validate-references` y `verify-lock`).
5. **Sincronización documental canónica**:
   - Actualizar el documento de microtarea `docs/rules/01-modules/foundations/micro/0036-cmake-presets/task.md`.

---

## Definición de terminado

La capacidad se considerará terminada al cumplir con la totalidad de los siguientes criterios:
1. Las suites de prueba `vigilio_presets_tests` y `vigilio_presets_service_tests` incluyen las nuevas pruebas de robustez, concurrencia, fallo de permisos y copy público.
2. La totalidad de las suites de prueba unitarias e integración en CTest (13/13 o superior) se ejecutan con 100% de éxito en modo headless en menos de 1 segundo.
3. El código fuente C++ cumple con Clang-Format y compila limpiamente sin advertencias bajo `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror`.
4. El CLI de Prompt Builder valida todas las referencias canónicas (`valid: true`) y el archivo lock correspondiente.
5. Los documentos de OpenSpec y microtarea reflejan fielmente el estado implementado y verificado.

---

## Dependencias y riesgos

- **Dependencia completada**: `module.foundations.task0035` (Alineación de UI y CMake Presets).
- **Riesgo 1: Comportamiento heterogéneo de permisos de solo lectura entre sistemas de archivos**: En ciertos entornos de contenedores o montajes virtuales (como tmpfs con root), `chmod` puede comportarse de manera no estándar. *Mitigación*: Se implementa verificación previa del soporte de permisos y se utiliza un fallback determinista que valida el manejo de errores sin depender exclusivamente de atributos de sistema operativo.
- **Riesgo 2: Condiciones de carrera en pruebas multihilo de cancelación**: Si el hilo de ejecución termina antes de que el hilo cancelador invoque `requestCancellation()`. *Mitigación*: Se utiliza sincronización explícita mediante barreras / variables atómicas para garantizar que la solicitud de cancelación ocurra durante la ventana activa de ejecución.

---

## Commit propuesto

`test(foundations): tests cmake-presets`
