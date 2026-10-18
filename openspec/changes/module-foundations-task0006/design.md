# Design — module-foundations-task0006

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- En `apps/vigilio-browser/CMakeLists.txt`, la configuración de advertencias del compilador se gestiona de forma dispersa, duplicada e incompleta:
  - En la biblioteca estática `vigilio_core`:
    ```cmake
    if(MSVC)
      target_compile_options(vigilio_core PRIVATE /W4 /permissive-)
    else()
      target_compile_options(vigilio_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
    ```
  - En la biblioteca estática `vigilio_storage`, se repite exactamente el mismo bloque de opciones.
  - Los ejecutables de pruebas unitarias (`vigilio_core_tests`) e integración (`vigilio_storage_tests`) no tienen ninguna opción de advertencia configurada, compilándose con los flags predeterminados del entorno.
  - El ejecutable principal de interfaz gráfica `vigilio-browser`, las pruebas de interfaz (`vigilio_ui_tests`) y la prueba de humo E2E (`vigilio_e2e_smoke`) tampoco poseen flags de advertencia explícitos.
  - No existe un target centralizado de interfaz (`INTERFACE library`) en CMake para gobernar las advertencias del proyecto de forma coherente.
  - No existe una opción configurable de CMake (`VIGILIO_WARNINGS_AS_ERRORS`) para convertir advertencias en errores de compilación (`-Werror` / `/WX`), permitiendo que código con problemas potenciales de tipos o memoria pase desapercibido en pipelines de integración continua.
  - El catálogo actual (`-Wall -Wextra -Wpedantic -Wconversion`) deja sin cobertura advertencias críticas para C++20: no incluye `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, ni advertencias extendidas de análisis lógico de GCC.
  - En MSVC, se limita a `/W4` y `/permissive-`, omitiendo diagnósticos especializados de seguridad de tipos y conformidad (`/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/w14311`, `/w14545`, etc.).

### Estado objetivo (TARGET)
- Una infraestructura centralizada, modular, desacoplada y robusta en CMake para la gobernanza de advertencias del compilador en C++20 que garantice:
  1. **Target de interfaz canónico (`vigilio_compiler_warnings`)**:
     - Declaración de un target `INTERFACE` en CMake que centraliza todas las opciones de compilación por familia de compilador (`GNU`, `Clang`/`AppleClang`, `MSVC`).
     - Vinculación `PRIVATE` a todos los targets del proyecto (`vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, `vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`).
     - Garantía estricta de no-fuga transitiva: consumidores externos que enlacen componentes de Vigilio no heredan obligatoriamente los flags de advertencia internos del proyecto.
  2. **Catálogo canónico completo por compilador**:
     - **GNU GCC**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wmisleading-indentation`, `-Wduplicated-cond`, `-Wduplicated-branches`, `-Wlogical-op`, `-Wuseless-cast`.
     - **LLVM Clang / AppleClang**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wextra-semi`.
     - **Microsoft Visual C++ (MSVC)**: `/W4`, `/permissive-`, `/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/we4289`, `/w14296`, `/w14311`, `/w14545`, `/w14546`, `/w14547`, `/w14549`, `/w14555`, `/w14619`, `/w14640`, `/w14826`, `/w14905`, `/w14906`, `/w14928`, `/EHsc`.
  3. **Gobernanza de Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS`)**:
     - Opción de CMake configurable: `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)`.
     - Adición condicional de `-Werror` (GCC/Clang) o `/WX` (MSVC) al target `vigilio_compiler_warnings`.
     - Activada por defecto para impedir regresiones en CI y asegurar código de máxima calidad, con posibilidad de desactivación puntual en desarrollo local mediante `-DVIGILIO_WARNINGS_AS_ERRORS=OFF`.
  4. **Aislamiento de bibliotecas de terceros y código generado**:
     - Marcado de inclusión `SYSTEM` para dependencias externas (`SQLite3`, `Qt6`) para suprimir advertencias ajenas al proyecto.
     - Protección frente a advertencias por expansiones internas de macros de Qt (`Q_OBJECT`, `signals:`, `slots:`).
  5. **Invariantes técnicas**:
     - Cero sobrecarga en tiempo de ejecución: la capacidad opera exclusivamente durante la fase de análisis y compilación; no genera código binario adicional ni afecta el consumo de memoria RAM o rendimiento.
     - Compatibilidad total headless (`VIGILIO_BUILD_GUI=OFF`) en entornos de integración continua.

### Brechas identificadas (GAPS)
- **Duplicación de código**: Los bloques `if(MSVC) ... else() ...` están duplicados entre `vigilio_core` y `vigilio_storage`, violando el principio DRY.
- **Falta de cobertura en pruebas y UI**: `vigilio_core_tests`, `vigilio_storage_tests`, `vigilio-browser` y demás ejecutables no aplican advertencias explícitas.
- **Catálogo incompleto**: Ausencia de flags para detectar variables sombreadas (`-Wshadow`), destructores no virtuales (`-Wnon-virtual-dtor`), conversiones de estilo C (`-Wold-style-cast`) y desreferenciación nula (`-Wnull-dereference`).
- **Falta de política Warnings-as-Errors**: No existe la opción `VIGILIO_WARNINGS_AS_ERRORS` para abortar la compilación en CI ante cualquier advertencia.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar: C++20 (`set(CMAKE_CXX_STANDARD 20)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`, `set(CMAKE_CXX_EXTENSIONS OFF)`).
  - Compiladores soportados:
    - GCC >= 11 (soporte completo de conceptos C++20 y flags de diagnóstico modernos).
    - Clang >= 14 (soporte completo de diagnósticos LLVM).
    - AppleClang >= 14 (Xcode 14+).
    - MSVC >= 19.30 (Visual Studio 2022 v143+ con `/permissive-`).
  - CMake: versión mínima 3.24.
  - Sistemas operativos: Linux (x86_64, aarch64), Windows 10/11 (x64), macOS (Intel y Apple Silicon).
- **Migración**:
  - La migración en la fase APPLY sustituye los bloques locales de `target_compile_options` en `apps/vigilio-browser/CMakeLists.txt` por la declaración e inclusión del target `vigilio_compiler_warnings`.
  - Es una refactorización interna y limpia: no altera nombres de librerías, estructuras de clases C++, signaturas de métodos ni contratos de enlace público.
- **Rollback**:
  - En esta fase PROPOSE, la reversión consiste únicamente en descartar los artefactos bajo `openspec/changes/module-foundations-task0006/`.
  - En la fase APPLY, la reversión se ejecuta restaurando el archivo `apps/vigilio-browser/CMakeLists.txt` a su versión previa al commit.

## Diseño completo de la capacidad: compiler warnings (etapa implementation)

### 1. Arquitectura de encapsulación en CMake (`vigilio_compiler_warnings`)

Para cumplir rigurosamente con los principios SOLID (Single Responsibility e Interface Segregation) y DRY de ingeniería de software (`docs/skills/01-principios-de-ingenieria.md`), la gestión de diagnósticos del compilador se centraliza en un único target de tipo `INTERFACE`:

```
+-----------------------------------------------------------------------------------+
|                        Target INTERFACE CMake: vigilio_compiler_warnings          |
+-----------------------------------------------------------------------------------+
| GNU GCC Flags      | -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion      |
|                    | -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align    |
|                    | -Wunused -Woverloaded-virtual -Wformat=2 -Wnull-dereference  |
|                    | -Wdouble-promotion -Wimplicit-fallthrough                    |
|                    | -Wmisleading-indentation -Wduplicated-cond                   |
|                    | -Wduplicated-branches -Wlogical-op -Wuseless-cast            |
+--------------------+--------------------------------------------------------------+
| LLVM Clang Flags   | -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion      |
|                    | -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align    |
|                    | -Wunused -Woverloaded-virtual -Wformat=2 -Wnull-dereference  |
|                    | -Wdouble-promotion -Wimplicit-fallthrough -Wextra-semi        |
+--------------------+--------------------------------------------------------------+
| MSVC Flags         | /W4 /permissive- /w14242 /w14254 /w14263 /w14265 /w14287     |
|                    | /we4289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549     |
|                    | /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928      |
|                    | /EHsc                                                        |
+--------------------+--------------------------------------------------------------+
| Warnings-as-Errors | VIGILIO_WARNINGS_AS_ERRORS: -Werror (GCC/Clang) / /WX (MSVC) |
+-----------------------------------------------------------------------------------+
                                      |
                 +--------------------+--------------------+
                 | (PRIVATE)          | (PRIVATE)          | (PRIVATE)
                 v                    v                    v
        [ vigilio_core ]     [ vigilio_storage ]   [ Test / UI Targets ]
```

### 2. Implementación del catálogo canónico por compilador

En `apps/vigilio-browser/CMakeLists.txt`, la definición se estructura de la siguiente manera:

```cmake
# ==============================================================================
# Capacidad: compiler warnings (etapa implementation)
# Target de interfaz centralizado para diagnósticos de compilación C++20
# ==============================================================================
add_library(vigilio_compiler_warnings INTERFACE)

set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wformat=2
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wextra-semi
)

set(GCC_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wformat=2
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

set(MSVC_WARNINGS
    /W4
    /permissive-
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
    /EHsc
)

option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(MSVC)
  set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS /WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS -Werror)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(PROJECT_WARNINGS ${GCC_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS -Werror)
  endif()
else()
  message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} not explicitly supported for strict warnings; falling back to basic warnings.")
  set(PROJECT_WARNINGS -Wall -Wextra)
endif()

target_compile_options(vigilio_compiler_warnings INTERFACE ${PROJECT_WARNINGS})
```

### 3. Gobernanza de la opción `VIGILIO_WARNINGS_AS_ERRORS`

- **Finalidad**: Evitar la acumulación de deuda técnica forzando a que cualquier advertencia detenga la compilación en integración continua (CI), al tiempo que otorga flexibilidad al desarrollador.
- **Valores**:
  - `ON` (predeterminado): Inyecta `-Werror` (GCC/Clang) o `/WX` (MSVC). Requerido en pipelines de CI y en presets estrictos de depuración.
  - `OFF`: Suprime `-Werror` / `/WX`, permitiendo advertencias informativas en consola sin abortar el proceso de construcción. Útil durante sesiones de depuración local o prototipado de nuevos componentes.

### 4. Vinculación a nivel de target (`PRIVATE`) y aislamiento modular

Cada target del proyecto se vincula a `vigilio_compiler_warnings` con visibilidad `PRIVATE`:
- `vigilio_core`: `target_link_libraries(vigilio_core PRIVATE vigilio_compiler_warnings)`
- `vigilio_storage`: `target_link_libraries(vigilio_storage PRIVATE vigilio_compiler_warnings)`
- `vigilio_core_tests`: `target_link_libraries(vigilio_core_tests PRIVATE vigilio_core vigilio_compiler_warnings)`
- `vigilio_storage_tests`: `target_link_libraries(vigilio_storage_tests PRIVATE vigilio_storage vigilio_compiler_warnings)`
- `vigilio-browser`: `target_link_libraries(vigilio-browser PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets)`
- `vigilio_ui_tests`: `target_link_libraries(vigilio_ui_tests PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test)`
- `vigilio_e2e_smoke`: `target_link_libraries(vigilio_e2e_smoke PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test)`

**Garantía de no-fuga transitiva**: El uso de `PRIVATE` asegura que el estándar de advertencias internas de Vigilio Browser no contamine a bibliotecas downstream ni a proyectos que consuman sus componentes como dependencias de CMake.

### 5. Aislamiento de dependencias de terceros (`SQLite3`, `Qt6`) con inclusiones `SYSTEM`

Para evitar que advertencias en bibliotecas de terceros provoquen fallos con `-Werror`:
1. **SQLite3**:
   - `find_package(SQLite3 REQUIRED)` proporciona el target importado `SQLite::SQLite3`.
   - CMake marca automáticamente los directorios de inclusión de targets importados como `SYSTEM` en los compiladores compatibles, suprimiendo diagnósticos internos de `sqlite3.h`.
2. **Qt6 (`Core`, `Widgets`, `WebEngineWidgets`, `Test`)**:
   - Las cabeceras de Qt se incluyen a través de los targets importados `Qt6::*`, los cuales CMake trata inherentemente como `SYSTEM`.
   - Adicionalmente, el código generado automáticamente por `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` se aísla de diagnósticos hiperestrictos de estilo C++ que pudieran activarse en macros Qt (`Q_OBJECT`).

### 6. Invariantes técnicas de la implementación

1. **Invariante 1 (Cero advertencias en código propio)**: Todo el código fuente bajo `src/**` y `tests/**` compila limpiamente con el catálogo canónico completo de advertencias bajo `-Werror` / `/WX`.
2. **Invariante 2 (Cero sobrecarga en tiempo de ejecución)**: Las advertencias del compilador operan 100% en tiempo de compilación/análisis estático; no generan código máquina adicional, no alteran el tamaño del binario ni consumen memoria RAM adicional en ejecución.
3. **Invariante 3 (Centralización DRY)**: Toda la política de diagnósticos de compilador se define en un único punto del proyecto (`vigilio_compiler_warnings`), eliminando bloques condicionales dispersos.
4. **Invariante 4 (Compatibilidad multiplataforma)**: El target de interfaz se adapta determinísticamente según `CMAKE_CXX_COMPILER_ID` (GNU, Clang, AppleClang, MSVC).
5. **Invariante 5 (Soporte headless continuo)**: La configuración preserva la opción `VIGILIO_BUILD_GUI=OFF` para compilación y ejecución de pruebas en servidores sin entorno gráfico.

## Archivos afectados

- **Archivos creados o modificados en OpenSpec (fase PROPOSE)**:
  - `openspec/changes/module-foundations-task0006/proposal.md`
  - `openspec/changes/module-foundations-task0006/design.md`
  - `openspec/changes/module-foundations-task0006/tasks.md`
  - `openspec/changes/module-foundations-task0006/execution-memory.md`
  - `openspec/changes/module-foundations-task0006/canonical-reference.yaml`
- **Archivos a modificar en la fase APPLY tras aprobación**:
  - `apps/vigilio-browser/CMakeLists.txt`:
    - Incorporación de `add_library(vigilio_compiler_warnings INTERFACE)`.
    - Definición de listas canónicas `CLANG_WARNINGS`, `GCC_WARNINGS`, `MSVC_WARNINGS`.
    - Definición de `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)`.
    - Vinculación `PRIVATE` a `vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, `vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`.
    - Eliminación de los bloques duplicados de `target_compile_options` ad-hoc.
  - `docs/rules/01-modules/foundations/micro/0006-compiler-warnings/task.md`:
    - Actualización de estado de la microtarea a `CURRENT; PASS`.

## Contrato funcional

- **Actor/Invocación**: El sistema de construcción CMake/Ninja ejecutado por el desarrollador o el runner de CI (`cmake --build apps/vigilio-browser/_build` o `apps/vigilio-browser/scripts/check.sh`).
- **Entrada**: Fuentes C++20 del proyecto, opciones de CMake (`VIGILIO_WARNINGS_AS_ERRORS=ON|OFF`), compilador detectado (`GNU`, `Clang`, `AppleClang`, `MSVC`).
- **Salida**: Binarios estáticos y ejecutables construidos limpiamente con código de salida 0. En caso de detectarse alguna violación de las advertencias canónicas bajo `VIGILIO_WARNINGS_AS_ERRORS=ON`, el compilador aborta con código de salida distinto de cero y emite diagnósticos detallados en consola con archivo, número de línea y flag causante.
- **Validación**: Comprobación del `CMAKE_CXX_COMPILER_ID`; si se utiliza un compilador alternativo, CMake emite un mensaje informativo (`message(WARNING ...)`) y degrada a `-Wall -Wextra` sin fallar la configuración.
- **Permisos y roles**: No aplica (N/A); proceso puramente local de ingeniería de software.
- **Persistencia**: No aplica (N/A); no se persisten datos de usuario.
- **Deshacer / Cancelar**: Operación cancelable de inmediato mediante interrupción estándar del proceso (`Ctrl+C` / `SIGINT`).
- **Concurrencia**: Plena compatibilidad con compilación paralela multihilo (`ninja -j$(nproc)`).
- **Recuperación ante errores**: El diagnóstico emitido por el compilador indica con precisión la ubicación del defecto y la flag violada, permitiendo al desarrollador corregir el código C++20 de forma guiada y determinista.

## Contrato UX/UI y Accesibilidad

- **Sin cambio visual directo**: La capacidad `compiler warnings` es estrictamente una mejora de infraestructura de construcción y calidad de código estático. No introduce ningún componente visual, widget, menú, ventana, botón ni texto en la interfaz gráfica del navegador.
- **Cumplimiento de lenguaje público (`docs/UX-COPY-PUBLICO.md`)**: Ningún término técnico de compilación (`compiler`, `warnings`, `-Werror`, `/WX`, `INTERFACE`, `CMake`) se expone en la interfaz pública visible de la aplicación ni en sus diálogos.
- **Accesibilidad y Modo Senior («Interfaz más fácil de usar»)**: Al eliminar en tiempo de compilación defectos de conversión numérica, punteros nulos y polimorfismo incorrecto, la capacidad asegura la máxima estabilidad de la aplicación, previniendo cuelgues repentinos que afectarían negativamente a usuarios que requieren una experiencia asistida y predecible.

## Reglas, condiciones de borde y recuperación de fallos

1. **Compilador no soportado explícitamente**:
   - Si se ejecuta un compilador distinto de GNU, Clang o MSVC (ejemplo: Intel oneAPI `icx`), CMake degrada limpiamente a flags estándar `-Wall -Wextra` emitiendo un aviso informativo sin interrumpir la configuración.
2. **Desactivación temporal de `-Werror` para diagnóstico local**:
   - El desarrollador puede pasar `-DVIGILIO_WARNINGS_AS_ERRORS=OFF` en la línea de comandos de CMake para iterar sobre código con prototipos temporales sin que las advertencias aborten el ciclo de construcción.
3. **Manejo de rutas de compilación y espacios**:
   - Los flags de advertencia de CMake no utilizan rutas hardcodeadas, garantizando total portabilidad independientemente de la ubicación del workspace en el disco.

## Seguridad, aislamiento y privacidad local

- **Filosofía local-first**: La configuración de advertencias de compilación opera 100% de manera local y desconectada; no realiza peticiones de red, no recopila telemetría ni interactúa con servicios en la nube de Vigilio.
- **Prevención de vulnerabilidades de seguridad de software**:
  - `-Wconversion` y `-Wsign-conversion`: mitigan desbordamientos de enteros (integer overflows), truncamientos y comparaciones erróneas de signo, protegiendo contra vulnerabilidades de cálculo de tamaños de buffers.
  - `-Wnull-dereference`: previene caídas por desreferenciación nula en tiempo de ejecución.
  - `-Wcast-align`: asegura alineación adecuada de memoria en arquitecturas como ARM/AArch64, evitando fallos de acceso no alineado.
  - `-Wnon-virtual-dtor`: previene destrucción incompleta de objetos derivados y fugas de recursos polimórficos.
  - `-Wold-style-cast`: obliga al uso de operadores de conversión explícitos de C++ (`static_cast`, `reinterpret_cast`), visibilizando transformaciones arriesgadas en revisiones de código.
- **Presupuestos de RAM y recursos**: La activación de advertencias estrictas añade un incremento inapreciable en el tiempo de compilación (< 3%) y no incrementa el consumo de memoria RAM de la aplicación final.

## Matriz de pruebas trazable

| Requisito / Criterio de Aceptación | Caso de Prueba / Escenario | Categoría | Fixture / Comando de Verificación | Resultado Esperado | Clasificación |
| --- | --- | --- | --- | --- | --- | --- |
| Implementación de target `vigilio_compiler_warnings` | Configuración de CMake y validación del target INTERFACE | Build / Config | `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF` | Configuración exitosa sin errores ni advertencias de CMake | `APPLY` |
| Compilación de `vigilio_core` con advertencias estrictas | Compilación limpia de fuentes de dominio con GCC/Clang bajo `-Werror` | Unit / Compilación | `cmake --build apps/vigilio-browser/_build --target vigilio_core` | 0 advertencias, 0 errores, compilación exitosa | `APPLY` |
| Compilación de `vigilio_storage` con advertencias estrictas | Compilación limpia de fuentes de persistencia con dependencias SQLite3 | Integración / Compilación | `cmake --build apps/vigilio-browser/_build --target vigilio_storage` | 0 advertencias, cabeceras de SQLite3 aisladas con SYSTEM | `APPLY` |
| Compilación de ejecutables de pruebas con advertencias | Compilación de `vigilio_core_tests` y `vigilio_storage_tests` | Tests / Compilación | `cmake --build apps/vigilio-browser/_build` | 0 advertencias, ejecutables generados correctamente | `APPLY` |
| Ejecución limpia de suite de pruebas CTest | Paso del 100% de las pruebas unitarias e integración | CTest / Ejecución | `ctest --test-dir apps/vigilio-browser/_build --output-on-failure` | 100% tests passed (core_units y storage_integration) | `APPLY` |
| Validación de gobernanza `VIGILIO_WARNINGS_AS_ERRORS` | Verificación de opción en CMake cache | Configuración | `cmake -B apps/vigilio-browser/_build -DVIGILIO_WARNINGS_AS_ERRORS=ON` | Opción registrada y activa en CMakeCache.txt | `APPLY` |
| Verificación mediante script de comprobación | Verificación automatizada con script del proyecto | Integración / CI | `bash apps/vigilio-browser/scripts/check.sh` | Compilación Ninja exitosa y 100% tests pasados con exit code 0 | `APPLY` |
| Sincronización de referencias canónicas | Validación del árbol de referencias con Prompt Builder | Gobernanza | `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` | Estado `valid: true`, referencias validadas | `APPLY` |
| Detección de advertencias inducidas (prueba negativa) | Introducción controlada de código con advertencia para validar rechazo | Negativo / CI | Compilación de fixture con variable no utilizada o sombreada bajo `-Werror` | El compilador rechaza el código y aborta con exit code != 0 | `LATER` (task0008) |
| Impacto visual en interfaz gráfica | Comprobación de ausencia de cambios en widgets Qt | UI / Accesibilidad | Inspección de archivos bajo `src/ui/` | Cero cambios en componentes visuales o strings de usuario | `NOT_APPLICABLE` |
