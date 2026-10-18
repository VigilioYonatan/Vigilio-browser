# Design — module-foundations-task0008

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Definición de advertencias de compilador (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_compiler_warnings` está implementado como una biblioteca de interfaz (`INTERFACE`) en CMake.
   - Centraliza los conjuntos de advertencias estrictas para GNU GCC (`GCC_WARNINGS`), LLVM Clang (`CLANG_WARNINGS`) y MSVC (`MSVC_WARNINGS`), habilitando `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, entre otras.
   - Opción `VIGILIO_WARNINGS_AS_ERRORS` activada por defecto (`ON`), inyectando `-Werror` (GCC/Clang) o `/WX` (MSVC).
   - Enlace privado (`PRIVATE vigilio_compiler_warnings`) implementado en `vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, y condicionalmente en `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke`.
2. **Suites de prueba en CTest**:
   - Existen dos ejecutables de prueba activos en modo headless: `vigilio_core_tests` (etiqueta `unit;fast`) y `vigilio_storage_tests` (etiqueta `integration;fast`), ambos compilando limpiamente bajo `-Werror`.
   - Pruebas condicionales de UI (`vigilio_ui_tests`) y prueba de humo E2E (`vigilio_e2e_smoke`), desacopladas y dependientes de paquetes de desarrollo de Qt6.
3. **Validación de entorno**:
   - GCC 15.2.0, C++20 (`-std=c++20`), CMake 4.2.3 y Ninja 1.13.2 operativos.
   - El script `apps/vigilio-browser/scripts/check.sh` compila y ejecuta satisfactoriamente las pruebas headless.

### Estado objetivo (TARGET)

1. **Arnés de pruebas automatizado de `compiler warnings`**:
   - Una suite de pruebas dedicada en CTest (`vigilio_warnings_tests`) que valide de forma determinista la efectividad y el comportamiento observable de la infraestructura de advertencias.
2. **Verificación de casos positivos (compilación limpia)**:
   - Confirmación automática de que todos los módulos de código del navegador (`src/core`, `src/storage`, `src/ui`) y todas las suites de prueba compilan sin emitir ningún diagnóstico de advertencia bajo `-Werror` / `/WX`.
3. **Verificación de casos negativos (inducción y rechazo de advertencias)**:
   - Demostración reproducible de que el target `vigilio_compiler_warnings` con `VIGILIO_WARNINGS_AS_ERRORS=ON` rechaza efectivamente código fuente defectuoso que viole las reglas de higiene C++20:
     - Conversión implícita estrechante (`double` a `int`) o de signo (`int` a `unsigned int`).
     - Sombreado de variables locales sobre parámetros o miembros (`shadowing`).
     - Clases con métodos virtuales pero sin destructor virtual.
     - Conversiones de estilo C (`(int)x`) frente a conversiones C++ explícitas (`static_cast<int>(x)`).
     - Parámetros o variables locales declaradas y no utilizadas.
4. **Verificación de recuperación y modo permisivo**:
   - Validación de que la opción `VIGILIO_WARNINGS_AS_ERRORS=OFF` permite que el compilador emita advertencias diagnósticas a la salida estándar de errores (`stderr`) sin interrumpir la compilación con código de error, útil en fases tempranas de depuración o refactorización.
5. **Aislamiento headless y no regresión**:
   - Ejecución rápida (< 0.5s), determinista y 100% headless sin requerir servidor gráfico ni procesos de renderizado Chromium.

### Brechas identificadas (GAPS)

1. **Ausencia de suite de prueba específica para la capacidad**: Aunque los módulos existentes compilan bajo `vigilio_compiler_warnings`, no existe una prueba explícita en CTest que ejerza y verifique el comportamiento de `compiler warnings` ante fallos inducidos y recuperación.
2. **Falta de validación automatizada de rechazo negativo**: No existe un mecanismo formal en CTest que pruebe que el compilador aborta (exit code != 0) al encontrar violaciones de advertencias específicas bajo `-Werror`.
3. **Falta de verificación del conmutador de recuperación**: No se comprueba automáticamente el comportamiento cuando `VIGILIO_WARNINGS_AS_ERRORS` se conmuta a `OFF`.

## Compatibility, migration and rollback

- **Compatibilidad con compiladores y estándares**:
  - Compatible con GNU GCC 11+, LLVM Clang 14+ y MSVC 2019+ con soporte completo para C++20 (`cxx_std_20`).
  - Las advertencias están separadas por compilador (`GCC_WARNINGS`, `CLANG_WARNINGS`, `MSVC_WARNINGS`) para evitar flags desconocidos que causen errores sintácticos en compiladores alternativos.
  - Las cabeceras de terceros (`SQLite3`, `Qt6`) se incluyen como `SYSTEM`, impidiendo que advertencias externas rompan la compilación del proyecto.
- **Estrategia de migración**:
  - No requiere migraciones de datos ni cambios en esquemas de almacenamiento (SQLite permanece intacto).
  - La suite de pruebas de advertencias se agrega de manera modular a `CMakeLists.txt` bajo la opción estándar `BUILD_TESTING`.
- **Estrategia de reversión (rollback)**:
  - En caso de regresión o fallo en una plataforma específica, la suite de pruebas se puede aislar o ajustar mediante conmutadores de CMake (`-DVIGILIO_WARNINGS_AS_ERRORS=OFF`).
  - La eliminación del nuevo ejecutable de prueba de `CMakeLists.txt` restaura inmediatamente el comportamiento anterior sin afectar el código de producción.

## Diseño completo de la capacidad: compiler warnings (etapa tests)

### 1. Estrategia de pruebas para advertencias de compilador

La capacidad `compiler warnings` gobierna la higiene del código C++20 en todo el proyecto. Su verificación en la etapa `tests` debe abarcar el cuadrante completo de calidad:

```
               +---------------------------+---------------------------+
               |         POSITIVO          |         NEGATIVO          |
+--------------+---------------------------+---------------------------+
| COMPILACIÓN  | Compilación limpia 100%   | Rechazo con error (!= 0)  |
| ESTÁTICA     | bajo -Werror / /WX        | ante violaciones inducidas|
|              | (cero advertencias)       | (-Wconversion, -Wshadow)  |
+--------------+---------------------------+---------------------------+
| RECUPERACIÓN | Conmutación permisiva     | Aislamiento de terceros:  |
| Y GOBERNANZA | VIGILIO_WARNINGS_AS_ERRORS| Qt6/SQLite en SYSTEM      |
|              | = OFF no rompe el build   | no filtran advertencias   |
+--------------+---------------------------+---------------------------+
```

### 2. Diseño del arnés de pruebas automatizadas (`vigilio_warnings_tests`)

Para probar la capacidad sin romper el proceso de compilación principal, se implementará una arquitectura de prueba de doble nivel:

#### Nivel A: Prueba unitaria en tiempo de compilación y ejecución (`tests/unit/CompilerWarningsTests.cpp`)
- **Objetivo**: Verificar que las definiciones de compilación y la semántica estricta de C++20 están activas en el binario de prueba.
- **Aserciones en tiempo de compilación (`static_assert`)**:
  - Conformidad estricta con el estándar C++20: `static_assert(__cplusplus >= 202002L, "C++20 standard required");`
  - Comprobación de tipos fundamentales y alineación requerida por `-Wcast-align`.
  - Ausencia de conversiones implícitas estrechantes en inicializaciones directas con llaves (`int x{1};`).
- **Aserciones en tiempo de ejecución**:
  - Verificación de polimorfismo seguro con destructores virtuales bien formados.
  - Validación de coincidencia de tipos en conversiones explícitas (`static_cast`).
  - Emisión de reporte estructurado: `Compiler warnings validation: PASS`.

#### Nivel B: Prueba de rechazo negativo e inducción de advertencias
- **Objetivo**: Demostrar que el compilador rechaza efectivamente código no conforme cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`.
- **Mecanismo de verificación**:
  - Un arnés de prueba automatizado (script o target de prueba CTest `warnings_rejection_test`) que compila fragmentos de código sintéticos con infracciones controladas contra `vigilio_compiler_warnings`.
  - Se definen fragmentos sintéticos mínimos:
    1. `fixture_narrowing.cpp`: asignación implícita de `double` a `int` (viola `-Wconversion`).
    2. `fixture_sign.cpp`: comparación o asignación entre `int` y `unsigned int` (viola `-Wsign-conversion`).
    3. `fixture_shadow.cpp`: declaración de variable local que sombrea un parámetro de función (viola `-Wshadow`).
    4. `fixture_non_virtual_dtor.cpp`: clase base con método virtual pero destructor público no virtual (viola `-Wnon-virtual-dtor`).
    5. `fixture_c_cast.cpp`: conversión de puntero o valor usando sintaxis C antigua `(int)v` (viola `-Wold-style-cast`).
  - **Criterio de éxito del caso negativo**: La invocación de compilación debe fallar (exit code != 0) y el mensaje de error del compilador debe contener el identificador de la advertencia infringida.

#### Nivel C: Prueba de recuperación y conmutación
- **Objetivo**: Validar que con `VIGILIO_WARNINGS_AS_ERRORS=OFF`:
  - Los mismos fragmentos sintéticos con advertencias compilan con éxito (exit code == 0), emitiendo avisos a `stderr`.
  - El sistema no se encuentra bloqueado indebidamente ante la necesidad de diagnóstico en ramas de desarrollo.

### 3. Concurrencia, límites de recursos y gobernanza de procesos

- **Límites de RAM y CPU**: Las pruebas de advertencias no instancian WebEngine, no abren sockets de red ni crean bases de datos en disco. El consumo de memoria es mínimo (< 50 MB) y el tiempo de ejecución en CTest es inferior a 0.1 segundos.
- **Ejecución en paralelo**: Compatible con ejecución concurrente (`ctest -jN`); las pruebas no comparten estado mutable ni archivos temporales fijos.
- **Prevención de procesos colgados (Zero-Zombie)**: El target de prueba en CMake tiene asignado un timeout estricto de 15 segundos (`TIMEOUT 15`). Si cualquier proceso de prueba se bloqueara, CTest lo finaliza de forma inmediata y reporta el fallo.

### 4. Preservación del modo headless y compatibilidad multiplataforma

- La suite se compila y ejecuta al 100% en modo headless (`VIGILIO_BUILD_GUI=OFF`), siendo ideal para pipelines de CI ligeros en Linux, Windows y macOS.
- No requiere dependencias de entorno gráfico (sin X11, Wayland, Cocoa ni GDI).

## Archivos afectados

| Archivo | Tipo de cambio | Justificación técnica |
| --- | --- | --- |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación (APPLY) | Registro del target de prueba `vigilio_warnings_tests` bajo `if(BUILD_TESTING)`, vinculando `vigilio_compiler_warnings` con etiquetas CTest `unit;fast` y timeout 15s. |
| `apps/vigilio-browser/tests/unit/CompilerWarningsTests.cpp` | Archivo nuevo (APPLY) | Implementación del ejecutable de prueba para validación de banderas estrictas, higiene C++20 y aserciones estáticas y dinámicas. |
| `docs/rules/01-modules/foundations/micro/0008-compiler-warnings/task.md` | Modificación (APPLY) | Sincronización del estado de ejecución tras verificación completa de la capacidad en la etapa `tests`. |
| `openspec/changes/module-foundations-task0008/**` | Archivos nuevos/modificados (PROPOSE) | Documentos de la propuesta OpenSpec: `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`, `canonical-reference.yaml`. |

## Contrato funcional

- **Actor/Invocación**: Desarrollador o sistema de integración continua (CI) mediante `cmake --build` y `ctest --test-dir _build -R warnings`.
- **Entrada**: Código fuente C++20 compilado con las banderas del target `vigilio_compiler_warnings`.
- **Salida esperada**:
  - En caso normal (código limpio): Compilación exitosa (código 0), ejecución de pruebas en CTest con salida `PASS`.
  - En caso de infracción con `VIGILIO_WARNINGS_AS_ERRORS=ON`: Compilación abortada inmediatamente por el compilador (código != 0) indicando la advertencia convertida en error.
  - En caso de infracción con `VIGILIO_WARNINGS_AS_ERRORS=OFF`: Compilación exitosa (código 0) con advertencias emitidas en `stderr`.
- **Permisos requeridos**: Ninguno. Operación puramente local sin privilegios elevados ni acceso a red.
- **Persistencia**: N/A. No genera datos persistentes en disco ni modifica la base de datos de usuario.
- **Deshacer / Cancelar**: N/A. Operación de compilación y prueba idempotente; cancelación segura con `Ctrl+C` sin corrupción de artefactos.
- **Errores claros y recuperación**: Errores del compilador con ruta de archivo, número de línea y nombre de bandera diagnóstica (ej. `[-Werror=conversion]`).

## Contrato UX/UI

### Naturaleza de infraestructura: ausencia formal de cambio visual directo

- La capacidad `compiler warnings` es puramente infraestructura técnica de construcción y calidad del compilador.
- **Declaración explícita**: Este cambio **no introduce pantallas, controles, estilos ni elementos visuales en la interfaz gráfica** del navegador.
- Se verificó la coherencia con los contratos de UI de `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: ningún componente de compilación altera la interfaz existente de `BrowserWindow`.

### Cumplimiento estricto del lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`)

- Se audita que ninguna cadena, log visible al usuario o etiqueta pública exponga nombres técnicos de compilación:
  - Términos prohibidos en la UI pública: «compiler warnings», «warnings», «-Werror», «CMake», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec».
  - En la interfaz del navegador se utiliza exclusivamente la denominación aprobada **«Interfaz más fácil de usar»**.

### Verificación de referencias visuales (`resultado/`)

- Se cotejaron las referencias visuales de la aplicación:
  - `resultado/01-vigilio-home.png`: La pantalla de inicio permanece limpia y sin mensajes de depuración o advertencias técnicas.
  - `resultado/03-vigilio-settings-senior-mode.png`: El panel de accesibilidad mantiene su estructura sobria, con opciones comprensibles para usuarios generales.
  - `resultado/06-logo-instalador-oficial.png`: La identidad gráfica oficial se preserva intacta.

## Reglas y fallos

- **Regla 1 (Tolerancia cero a advertencias en producción)**: Con `VIGILIO_WARNINGS_AS_ERRORS=ON`, cualquier advertencia generada por el código de `apps/vigilio-browser` es tratada como error fatal y detiene la compilación de forma determinista.
- **Regla 2 (Aislamiento de cabeceras de terceros)**: Las inclusiones de `Qt6` y `SQLite3` deben marcarse como `SYSTEM` en CMake, impidiendo que diagnósticos en bibliotecas upstream rompan la construcción de Vigilio Browser.
- **Regla 3 (Determinismo en CTest)**: Todas las pruebas de advertencias deben ser deterministas, reproducibles e independientes del orden de ejecución. Queda prohibido el uso de retardos arbitrarios (`sleep`).
- **Regla 4 (Gobernanza de recursos y Zero-Zombie)**: Ninguna prueba puede dejar procesos huérfanos o en ejecución infinita. Se aplican timeouts estrictos de 15 segundos en CTest.
- **Regla 5 (Honestidad en el reporte de pruebas)**: Toda prueba debe reportar honestamente su resultado (`PASS`, `FAIL`, `NOT_EXECUTED`). No se admiten mocks ficticios ni capturas vacías de error para conseguir un estado verde artificial.

## Seguridad y privacidad

- **Modelo local-first y cero telemetría**:
  - Las pruebas de advertencias se ejecutan exclusivamente en la máquina local.
  - No se realiza ninguna conexión de red, consulta DNS ni transmisión de datos a servidores externos o servicios cloud de Vigilio.
- **Aislamiento de memoria y ejecución segura**:
  - Las pruebas no ejecutan código web arbitrario ni requieren permisos de red.
  - No se cargan scripts externos ni se crean perfiles de usuario en disco durante las pruebas.

## Matriz de pruebas trazables

| Criterio / Capacidad | Escenario de prueba | Categoría | Fixture / Comando | Resultado esperado | Fallo negativo esperado | Clasificación |
| --- | --- | --- | --- | --- | --- | --- |
| Compilación limpia de targets existentes | Compilación completa de `vigilio_core` y `vigilio_storage` con advertencias estrictas | Build / Positive | `cmake --build apps/vigilio-browser/_build` | 0 advertencias, 0 errores, compilación exitosa | Fallo con exit code != 0 si alguna línea emite advertencia bajo `-Werror` | APPLY |
| Aserciones estáticas de C++20 | Validación estática de estándar C++20 y tipos | Unit / Positive | `ctest --test-dir apps/vigilio-browser/_build -R vigilio_warnings_tests` | Aprobación de `static_assert` en C++20 (`PASS`) | Error de compilación si el compilador no opera en modo C++20 | APPLY |
| Detección y rechazo de conversiones implícitas | Infracción inducida de conversión estrechante (`double` a `int`) | Negative / Inyección | Fixture sintético con `double d = 3.14; int i = d;` compilado con flags de `vigilio_compiler_warnings` | El compilador rechaza el código con exit code != 0 por `-Wconversion` | Fallo de seguridad de calidad si el compilador acepta la conversión sin error | APPLY |
| Detección y rechazo de sombreado de variables | Infracción inducida de variable local sombreada | Negative / Inyección | Fixture sintético con variable local con el mismo nombre que parámetro de función | El compilador rechaza el código con exit code != 0 por `-Wshadow` | Fallo si la variable sombreada es ignorada por el compilador | APPLY |
| Detección de destructores no virtuales | Infracción inducida de clase base polimórfica sin destructor virtual | Negative / Inyección | Fixture sintético con clase polimórfica y destructor no virtual | El compilador rechaza el código con exit code != 0 por `-Wnon-virtual-dtor` | Fallo si no se detecta la omisión del destructor virtual | APPLY |
| Recuperación mediante conmutador permisivo | Compilación con `VIGILIO_WARNINGS_AS_ERRORS=OFF` | Recovery / Toggle | `cmake -S apps/vigilio-browser -B _build_warn -DVIGILIO_WARNINGS_AS_ERRORS=OFF && cmake --build _build_warn` | Compilación exitosa emitiendo avisos a stderr sin abortar el build | Fallo si la opción OFF no remueve `-Werror` / `/WX` | APPLY |
| Preservación del modo headless | Ejecución integral de suite de pruebas sin entorno gráfico | Headless / CI | `bash apps/vigilio-browser/scripts/check.sh` | Compilación Ninja limpia y 100% tests pasados con exit code 0 | Exit code != 0 si algún test falla o requiere display | APPLY |
| Ausencia de regresión en suites existentes | Verificación de `core_units` y `storage_integration` | Regresión | `ctest --test-dir apps/vigilio-browser/_build -L fast` | 100% tests aprobados sin cambios de comportamiento | Fallo de aserción si se altera el comportamiento de core o storage | APPLY |
| Pruebas de UI condicionadas a Qt6 | Verificación de `window_ui_tests` en presencia de SDK Qt6 | UI / Qt Test | `ctest -R window_ui_tests` (condicionado a Qt6) | Verificación de widgets y accesibilidad | Registrado como `NOT_EXECUTED` si falta SDK Qt6 en entorno CI | NOT_EXECUTED / CONDICIONAL |
| Prueba de humo E2E Chromium | Ejecución opcional de `vigilio_e2e_smoke` con fixture local | E2E / Heavy | `ctest -R browser_e2e_smoke` (condicionado a `VIGILIO_ENABLE_E2E=ON`) | Carga de fixture HTML local en < 20s | Salida `GUI smoke FAIL` si el motor WebEngine se bloquea | LATER / E2E HEAVY |

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0007` (etapa `ui` de compiler warnings, finalizada con targets de UI vinculados limpiamente a `vigilio_compiler_warnings`).
- **Dependencias posteriores**: `module.foundations.task0009` (etapa `contract` de `qt-dependency-validation`).
- **Riesgos técnicos y mitigaciones**:
  1. *Riesgo*: Diferencias sutiles de nombres de advertencias entre GCC y Clang (por ejemplo, `-Wduplicated-cond` no existe en Clang; `-Wextra-semi` puede comportarse diferente en GCC).
     *Mitigación*: Mantener listas explícitas y aisladas (`GCC_WARNINGS`, `CLANG_WARNINGS`, `MSVC_WARNINGS`) en `apps/vigilio-browser/CMakeLists.txt`, verificando la compatibilidad de cada bandera antes de inyectarla.
  2. *Riesgo*: Aparición de advertencias no controladas en código expandido por macros de bibliotecas externas.
     *Mitigación*: Tratamiento riguroso de dependencias de terceros como cabeceras del sistema (`SYSTEM` en CMake), impidiendo que diagnósticos externos bloqueen el build propio.
  3. *Riesgo*: Incremento en el tiempo de construcción o pruebas de CI por ejecución de compilaciones de prueba negativas.
     *Mitigación*: Diseñar los fixtures negativos como snippets ultra-compactos (< 15 líneas de código C++ puro) sin inclusiones pesadas de cabeceras de sistema.
