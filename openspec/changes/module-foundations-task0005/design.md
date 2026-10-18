# Design — module-foundations-task0005

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- En `apps/vigilio-browser/CMakeLists.txt`, la configuración de advertencias del compilador se gestiona de forma ad-hoc y duplicada:
  - En la biblioteca `vigilio_core`:
    ```cmake
    if(MSVC)
      target_compile_options(vigilio_core PRIVATE /W4 /permissive-)
    else()
      target_compile_options(vigilio_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
    ```
  - En la biblioteca `vigilio_storage`, se duplica exactamente el mismo bloque de opciones.
  - En el ejecutable de escritorio `vigilio-browser`, no se especifican opciones de compilación adicionales ni advertencias explícitas.
  - En los ejecutables de pruebas (`vigilio_core_tests`, `vigilio_storage_tests`, `vigilio_ui_tests`, `vigilio_e2e_smoke`), no se aplican advertencias del compilador explícitas.
  - No existe un target de interfaz centralizado (`INTERFACE library`) para gobernar las advertencias del proyecto.
  - No existe una opción configurable para tratar advertencias como errores (`VIGILIO_WARNINGS_AS_ERRORS` o `CMAKE_COMPILE_WARNINGS_AS_ERRORS`), dejando abierta la posibilidad de que se introduzcan advertencias en entornos de CI sin provocar fallos tempranos de compilación.
  - El conjunto de flags actual para GCC/Clang (`-Wall -Wextra -Wpedantic -Wconversion`) es insuficiente para la seguridad exigida en C++20: no incluye `-Wshadow` (variables que sombrean declaraciones externas), `-Wnon-virtual-dtor` (destructores no virtuales en clases polimórficas), `-Wold-style-cast` (casts de estilo C en lugar de casts C++ explícitos), `-Wcast-align` (riesgos de alineación en memoria), `-Wunused` (parámetros y variables no utilizadas), `-Woverloaded-virtual` (funciones virtuales sobrecargadas no intencionadamente ocultas), ni `-Wnull-dereference` (desreferenciación nula comprobable estáticamente).
  - En MSVC, se utiliza `/W4` y `/permissive-`, pero no se activan advertencias complementarias de seguridad de tipos y conformidad estricta (por ejemplo `/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/w14311`, `/w14545`).

### Estado objetivo (TARGET)
- Una arquitectura de gobierno de advertencias de compilación para C++20 robusta, centralizada, desacoplada y portable que satisfaga:
  1. **Centralización arquitectónica mediante target de interfaz**:
     - Definición de un target `vigilio_compiler_warnings` de tipo `INTERFACE` en CMake.
     - Vinculación `PRIVATE` desde todos los targets de biblioteca, ejecutables y pruebas del proyecto (`vigilio_core`, `vigilio_storage`, `vigilio-browser`, suites de tests).
     - Garantía de no-fuga transitiva: consumidores externos o downstream que enlacen targets de Vigilio no heredan obligatoriamente los flags de advertencia internos del proyecto.
  2. **Catálogo canónico de advertencias por familia de compilador**:
     - **GNU GCC**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wmisleading-indentation`, `-Wduplicated-cond`, `-Wduplicated-branches`, `-Wlogical-op`, `-Wuseless-cast`.
     - **LLVM Clang / AppleClang**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wextra-semi`.
     - **Microsoft Visual C++ (MSVC)**: `/W4`, `/permissive-`, `/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/we4289`, `/w14296`, `/w14311`, `/w14545`, `/w14546`, `/w14547`, `/w14549`, `/w14555`, `/w14619`, `/w14640`, `/w14826`, `/w14905`, `/w14906`, `/w14928`, `/EHsc`.
  3. **Política de Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS`)**:
     - Configuración controlada vía CMake mediante la opción `VIGILIO_WARNINGS_AS_ERRORS` (por defecto activada en perfiles de CI y disponible en presets de depuración estricta).
     - En GCC/Clang: adición de `-Werror`.
     - En MSVC: adición de `/WX`.
     - Capacidad de desactivación local por parte del desarrollador para tareas de prototipado o diagnóstico sin bloquear el ciclo de trabajo.
  4. **Aislamiento de dependencias de terceros y código generado**:
     - Inclusión de cabeceras de terceros (`SQLite3`, bibliotecas Qt) marcada con `SYSTEM` para que los compiladores ignoren cualquier advertencia interna de librerías externas.
     - Inmunización del código generado por Qt (`moc`, `rcc`, `uic`) frente a advertencias que puedan surgir por expansiones internas de macros Qt (`Q_OBJECT`, `signals:`, `slots:`).
  5. **Invariantes del modelo de dominio**:
     - Cero advertencias como baseline continuo sobre toda la base de código propia.
     - Cero impacto en el binario en tiempo de ejecución (no altera el rendimiento ni el consumo de memoria RAM de la aplicación final).

### Brechas identificadas (GAPS)
- **Falta de especificación formal del catálogo de advertencias**: las advertencias estaban limitadas a cuatro flags genéricos (`-Wall -Wextra -Wpedantic -Wconversion`) sin formalizar las reglas para constructos complejos de C++20.
- **Duplicación de código en CMake**: configuración repetida en `vigilio_core` y `vigilio_storage` en lugar de una abstracción de interfaz reutilizable.
- **Ausencia de opción controlable de warnings-as-errors**: no existía una bandera determinista para asegurar que el pipeline de CI aborte ante cualquier advertencia.
- **Falta de cobertura en ejecutables y suites de prueba**: los ejecutables de pruebas y la aplicación GUI se compilaban con los flags por defecto del compilador, pudiendo acumular advertencias invisibles para el equipo.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar del lenguaje: C++20 puro (`set(CMAKE_CXX_STANDARD 20)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`).
  - Compiladores soportados:
    - GCC >= 11 (soporte completo de conceptos C++20 y flags de diagnóstico modernos).
    - Clang >= 14 (soporte completo de C++20 y diagnósticos LLVM).
    - AppleClang >= 14 (Xcode 14+).
    - MSVC >= 19.30 (Visual Studio 2022 con `/permissive-`).
  - Sistemas operativos: Linux (x86_64, aarch64), Windows 10/11 (x64), macOS (Intel y Apple Silicon).
- **Migración**:
  - Al introducirse la etapa `domain`, no se alteran archivos fuente existentes; la migración en la posterior etapa `implementation` consistirá en refactorizar `CMakeLists.txt` para sustituir los bloques locales por la vinculación a `vigilio_compiler_warnings`.
- **Rollback**:
  - En la fase PROPOSE, la reversión se efectúa simplemente descartando los artefactos bajo `openspec/changes/module-foundations-task0005/`.

## Diseño completo de la capacidad: compiler warnings (etapa domain)

### 1. Taxonomía del modelo de dominio de advertencias del compilador

El modelo de dominio de `compiler warnings` estructura las advertencias según su propósito de ingeniería de software y la categoría de defecto que mitigan:

```
+-----------------------------------------------------------------------------------+
|                        Modelo de Dominio: Compiler Warnings                       |
+-----------------------------------------------------------------------------------+
| 1. Seguridad de Tipos y Conversiones  | -Wconversion, -Wsign-conversion,          |
|                                       | /w14242, /w14254                          |
|---------------------------------------+-------------------------------------------|
| 2. Corrección Polimórfica y Clases   | -Wnon-virtual-dtor, -Woverloaded-virtual, |
|                                       | /w14263, /w14265                          |
|---------------------------------------+-------------------------------------------|
| 3. Higiene de Nombres y Ámbito        | -Wshadow, -Wunused, /w14545               |
|---------------------------------------+-------------------------------------------|
| 4. Prevención de Comportamiento UB    | -Wnull-dereference, -Wcast-align,         |
|                                       | -Wold-style-cast, /w14311                 |
|---------------------------------------+-------------------------------------------|
| 5. Lógica, Control de Flujo y Formato | -Wimplicit-fallthrough, -Wformat=2,       |
|                                       | -Wdouble-promotion, /w14296               |
|---------------------------------------+-------------------------------------------|
| 6. Modos de Rigor (Perfiles)          | Standard (Dev) vs Strict (Warnings-as-Err)|
+-----------------------------------------------------------------------------------+
```

### 2. Catálogo canónico y justificación técnica de flags por compilador

#### A. GNU GCC (`CMAKE_CXX_COMPILER_ID STREQUAL "GNU"`)
1. `-Wall` y `-Wextra`: activa el conjunto base y extendido de diagnósticos de uso cuestionable de constructos C++.
2. `-Wpedantic`: exige el cumplimiento estricto del estándar C++20 ISO, rechazando extensiones de compilador no portables.
3. `-Wconversion` y `-Wsign-conversion`: detecta conversiones implícitas de tipos con pérdida de precisión o cambio de signo (ejemplo: asignar `uint64_t` a `int32_t`), fuente común de desbordamientos enteros en cálculo de tamaños de pestañas o buffers.
4. `-Wshadow`: advierte si una variable local oculta a otra del ámbito exterior, a un parámetro de función o a un miembro de clase, previniendo errores de estado no intencionados.
5. `-Wnon-virtual-dtor`: exige que toda clase con funciones miembro virtuales declare un destructor virtual, evitando fugas de memoria o comportamiento indefinido al destruir instancias derivadas a través de un puntero base.
6. `-Wold-style-cast`: rechaza el uso de casts de estilo C (`(int)x`) en código C++, exigiendo `static_cast`, `reinterpret_cast` o `const_cast` para visibilizar la intención semántica de la conversión.
7. `-Wcast-align`: advierte ante conversiones de punteros que incrementen los requisitos de alineación del tipo destino, previniendo fallos de bus o degradación de rendimiento en arquitecturas ARM.
8. `-Wunused`: detecta variables, funciones, parámetros o expresiones evaluadas pero nunca utilizadas, previniendo código muerto o lógica incompleta.
9. `-Woverloaded-virtual`: advierte si una función miembro en una clase derivada tiene el mismo nombre que una función virtual en la clase base pero con distinta signatura, evitando la ocultación accidental de métodos.
10. `-Wformat=2`: activa la comprobación rigurosa de cadenas de formato en funciones tipo `printf` o `snprintf` contra los tipos de sus argumentos.
11. `-Wnull-dereference`: analiza estáticamente rutas de ejecución para detectar desreferenciaciones explícitas de punteros con valor nulo.
12. `-Wdouble-promotion`: alerta sobre conversiones implícitas de `float` a `double` en expresiones aritméticas, protegiendo contra pérdida innecesaria de rendimiento en cálculos de rendering.
13. `-Wimplicit-fallthrough`: exige anotaciones explícitas (`[[fallthrough]];`) en sentencias `switch`, impidiendo caídas accidentales de un caso a otro.
14. `-Wmisleading-indentation`: advierte si la indentación del código sugiere una estructura de control distinta a la semántica real del programa (ejemplo: un bloque `if` sin llaves con líneas indentadas subsiguientes).
15. `-Wduplicated-cond` y `-Wduplicated-branches`: detecta condiciones duplicadas en cadenas `if / else if` o bloques de código idénticos en distintas ramas.
16. `-Wlogical-op`: advierte sobre patrones sospechosos en operaciones lógicas (ejemplo: `x > 0 && x > 5`).
17. `-Wuseless-cast`: advierte si se realiza un cast a un tipo idéntico al de la propia expresión evaluada.

#### B. LLVM Clang y AppleClang (`CMAKE_CXX_COMPILER_ID MATCHES "Clang"`)
1. Comparte los mismos flags que GCC: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`.
2. `-Wextra-semi`: advierte sobre puntos y comas redundantes al final de declaraciones o fuera de ámbitos válidos, manteniendo la sintaxis pulcra.

#### C. Microsoft Visual C++ (`CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"`)
1. `/W4`: nivel más alto de advertencias de propósito general en MSVC (evitando `/Wall` que emite diagnósticos excesivos e inapropiados dentro de cabeceras estándar de Windows).
2. `/permissive-`: impone la conformidad estricta con el estándar C++ ISO, deshabilitando extensiones no estándar de Microsoft.
3. `/w14242`: conversión de tipos con posible pérdida de datos.
4. `/w14254`: conversión desde un tipo más grande a uno más pequeño.
5. `/w14263`: la función miembro no sobreescribe ninguna función virtual de la clase base.
6. `/w14265`: la clase tiene funciones virtuales pero su destructor no es virtual.
7. `/w14287`: comparación de constante sin signo con número negativo.
8. `/we4289`: uso no estándar de la variable de control de bucle fuera del ámbito del bucle for.
9. `/w14296`: la expresión es siempre verdadera o siempre falsa.
10. `/w14311`: truncamiento de puntero a tipo entero de menor tamaño.
11. `/w14545`: la expresión antes de la coma se evalúa a una función sin lista de argumentos.
12. `/w14546`: la llamada a función antes de la coma carece de lista de argumentos.
13. `/w14547`: el operador antes de la coma no tiene ningún efecto observable.
14. `/w14549`: el operador antes de la coma no tiene efecto (¿se intentó usar un AND lógico?).
15. `/w14555`: la expresión no tiene efecto.
16. `/w14619`: advertencia sobre pragma que referencia un número de warning inexistente.
17. `/w14640`: advertencia sobre inicialización estática de miembro no segura para hilos.
18. `/w14826`: la conversión con extensión de signo produce resultados inesperados.
19. `/w14905`: literal de cadena ancha convertido a LPSTR.
20. `/w14906`: literal de cadena de caracteres convertido a LPWSTR.
21. `/w14928`: inicialización por copia ilegal con múltiples conversiones de usuario implícitas.
22. `/EHsc`: manejo de excepciones síncronas estándar C++ (requerido para garantizar que el desenrollado de pila destruya objetos locales de forma determinista mediante RAII).

### 3. Política de Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS`)

Para evitar que las advertencias se acumulen como deuda técnica en el repositorio:
- Se define la opción de CMake:
  ```cmake
  option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
  ```
- Comportamiento condicionado:
  - En GCC / Clang: cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`, se añade `-Werror`.
  - En MSVC: cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`, se añade `/WX`.
- Entornos de aplicación:
  - **Pipelines de Integración Continua (CI)**: forzado obligatorio a `ON` para rechazar cualquier pull request que introduzca código descuidado.
  - **Entorno de desarrollo local**: `ON` por defecto en presets estrictos, pero configurable a `OFF` si un desarrollador necesita depurar cambios en progreso con variables temporales no utilizadas.

### 4. Arquitectura de encapsulación en CMake (`vigilio_compiler_warnings`)

Para adherirse a los principios SOLID y DRY de ingeniería (`docs/skills/01-principios-de-ingenieria.md`):
- Se encapsula la política completa en un target de CMake de tipo `INTERFACE`:
  ```cmake
  add_library(vigilio_compiler_warnings INTERFACE)
  ```
- Cada target del proyecto (`vigilio_core`, `vigilio_storage`, `vigilio-browser`, `vigilio_core_tests`, `vigilio_storage_tests`, etc.) vincula este target mediante:
  ```cmake
  target_link_libraries(<target_name> PRIVATE vigilio_compiler_warnings)
  ```
- El uso de `PRIVATE` garantiza que los flags no se propaguen hacia dependientes de tercer orden.
- Se evita la proliferación de sentencias `if(MSVC)` repetidas a lo largo de los archivos de compilación.

### 5. Aislamiento de dependencias de terceros y código generado por Qt

Uno de los principales riesgos al activar advertencias estrictas (`-Wconversion`, `-Wold-style-cast`, `-Werror`) es la ruptura de compilación provocada por código externo fuera de nuestro control:
1. **Bibliotecas externas (`SQLite3`, cabeceras de sistema)**:
   - Toda inclusión de bibliotecas de terceros debe marcarse con la directiva `SYSTEM`:
     ```cmake
     target_include_directories(vigilio_storage SYSTEM PRIVATE ${SQLite3_INCLUDE_DIRS})
     ```
   - El compilador (GCC, Clang y MSVC moderno con `/external:W0`) suprime automáticamente todas las advertencias emitidas por archivos de cabecera ubicados en directorios marcados como de sistema.
2. **Código generado por Qt (`moc`, `rcc`, `uic`)**:
   - Las herramientas de Qt generan archivos auxiliares (`moc_*.cpp`, `qrc_*.cpp`).
   - Las macros internas de Qt pueden utilizar conversiones que desencadenen advertencias en modos hiperestrictos (por ejemplo, `-Wuseless-cast` o `-Wold-style-cast`).
   - El diseño del target `vigilio_compiler_warnings` filtra o ajusta los flags en los targets que consumen `Qt6` para asegurar que las expansiones de macros de Qt no causen fallos de `-Werror`, mientras que el núcleo de dominio `vigilio_core` permanece bajo la suite de advertencias más estricta posible.

### 6. Invariantes del modelo de dominio

1. **Invariante 1 (Cero advertencias de compilación)**: Todo el código fuente del proyecto bajo `src/**` y `tests/**` debe compilar limpiamente sin una sola advertencia en los compiladores objetivo (GCC, Clang, MSVC).
2. **Invariante 2 (Desacoplamiento de capas)**: Las opciones de advertencia de `vigilio_core` no dependen ni asumen la presencia de Qt ni de SQLite.
3. **Invariante 3 (Inmunidad frente a dependencias externas)**: Ninguna advertencia proveniente de dependencias de terceros o librerías de sistema puede abortar la construcción de Vigilio Browser.
4. **Invariante 4 (Determinismo multiplataforma)**: El catálogo de advertencias se traduce fielmente según el `CMAKE_CXX_COMPILER_ID` detectado, garantizando idéntico estándar de calidad en Linux, Windows y macOS.
5. **Invariante 5 (Cero sobrecarga en tiempo de ejecución)**: La capacidad de compiler warnings actúa exclusivamente en tiempo de traducción; no añade instrucciones adicionales, no impacta la memoria RAM ni altera los tiempos de ejecución de la aplicación.

## Archivos afectados

En esta etapa **domain**, la propuesta formaliza el alcance y los contratos:
- **Archivos creados o modificados en OpenSpec**:
  - `openspec/changes/module-foundations-task0005/proposal.md`
  - `openspec/changes/module-foundations-task0005/design.md`
  - `openspec/changes/module-foundations-task0005/tasks.md`
  - `openspec/changes/module-foundations-task0005/execution-memory.md`
  - `openspec/changes/module-foundations-task0005/canonical-reference.yaml`
- **Archivos de referencia a actualizar en la fase APPLY**:
  - `docs/rules/01-modules/foundations/micro/0005-compiler-warnings/task.md` (sincronización de estado documental de la microtarea).
- **Archivos objetivo para etapas posteriores (no modificados en este paso)**:
  - `apps/vigilio-browser/CMakeLists.txt` (implementación del target `vigilio_compiler_warnings` en `task0006`).
  - `apps/vigilio-browser/CMakePresets.json` (presets de warnings-as-errors en microtareas de presets).

## Contrato funcional

- **Actor/Invocación**: El motor de compilación de CMake invocado por el desarrollador (`cmake --build`) o el pipeline de integración continua.
- **Entrada**: Fuentes C++20 del proyecto, opciones de configuración de CMake (`VIGILIO_WARNINGS_AS_ERRORS`).
- **Salida**: Diagnósticos claros y accionables del compilador en consola; código de salida 0 en caso de compilación limpia, o código de salida distinto de cero si se detecta alguna violación bajo `-Werror` / `/WX`.
- **Validación de entradas**: Verificación de la identidad del compilador soportado (`GNU`, `Clang`, `AppleClang`, `MSVC`). En caso de compilador no reconocido, CMake degrada a advertencias estándar sin abortar.
- **Permisos y roles**: No aplica; operación exclusiva de entorno de desarrollo y compilación.
- **Persistencia y estado**: No aplica (N/A); el proceso de compilación no persiste datos de usuario.
- **Deshacer / Cancelar**: Operación cancelable de inmediato mediante interrupción estándar del proceso (`SIGINT` / `Ctrl+C`).
- **Concurrencia**: Plena compatibilidad con sistemas de compilación paralelos (`ninja -jN`, `make -jN`).
- **Errores claros y recuperación**: Mensajes de advertencia o error emitidos con ruta exacta de archivo, línea y columna, indicando el flag específico que originó el diagnóstico (ejemplo: `error: conversion to 'int' from 'size_t' may alter its value [-Werror=conversion]`).

## Contrato UX/UI y Accesibilidad

- **Sin cambio visual directo**: La capacidad `compiler warnings` es estrictamente una herramienta de calidad de código e infraestructura de compilación. No introduce ningún componente visual, ventana, diálogo, menú ni texto en la interfaz gráfica del navegador.
- **Cumplimiento de lenguaje público (`docs/UX-COPY-PUBLICO.md`)**: Ningún término técnico de compilación (`compiler`, `warnings`, `-Werror`, `/WX`, `flags`, `CMake`) se expone en la interfaz pública visible de la aplicación.
- **Accesibilidad y Modo Senior (`docs/skills/05-accesibilidad-y-senior-mode.md`)**: No aplicable directamente al no existir superficie gráfica modificada; sin embargo, al elevar la solidez del código base y prevenir caídas imprevistas por errores de memoria o tipos, la capacidad contribuye directamente a la estabilidad requerida por usuarios que dependen de la «Interfaz más fácil de usar».

## Reglas, condiciones de borde y recuperación de fallos

1. **Compilador no estándar o versión antigua**:
   - Si se detecta un compilador C++20 no contemplado en la matriz explícita (ejemplo: Intel oneAPI `IntelLLVM`), CMake aplicará flags de compatibilidad Clang genéricos o degradará limpiamente sin abortar la configuración.
2. **Advertencias introducidas por macros de pruebas**:
   - En frameworks de aserciones o pruebas unitarias, algunas macros pueden generar advertencias de expresiones con efecto nulo o conversiones redundantes. El target `vigilio_compiler_warnings` permite la exclusión granular de advertencias específicas en targets de test si fuera estrictamente indispensable, manteniendo inviolable el código de producción.
3. **Manejo de rutas con espacios o caracteres no ASCII**:
   - Los flags de advertencia de CMake no dependen de rutas de archivos, garantizando compatibilidad en entornos de compilación con nombres de directorios complejos.

## Seguridad, aislamiento y privacidad local

- **Modelo local-first y privacidad**: La configuración de advertencias del compilador opera íntegramente de forma local en la máquina de desarrollo o en el nodo de CI; no realiza conexiones de red, no recopila telemetría ni envía datos a servidores externos.
- **Seguridad del software**: Al forzar la detección temprana de conversiones con pérdida de precisión (`-Wconversion`), desreferenciaciones nulas (`-Wnull-dereference`), alineación errónea de punteros (`-Wcast-align`) y polimorfismo mal implementado (`-Wnon-virtual-dtor`), la capacidad elimina proactivamente clases enteras de vulnerabilidades de corrupción de memoria y desbordamiento de enteros antes de que el código llegue a producción.
- **Presupuestos de RAM y recursos**: La verificación estricta de advertencias durante la compilación añade una sobrecarga de tiempo de CPU insignificante (menor al 3%) y no altera en absoluto el consumo de memoria en tiempo de ejecución.

## Matriz de pruebas trazable

| Requisito / Criterio de Aceptación | Caso de Prueba / Escenario | Categoría | Fixture / Comando de Verificación | Resultado Esperado | Clasificación |
| --- | --- | --- | --- | --- | --- |
| Especificación canónica de flags | Validación de completitud de flags para GCC, Clang y MSVC | Inspección estática | Revisión documental de `design.md` vs `docs/rules/01-modules/foundations/micro/0005-compiler-warnings/task.md` | Catálogo canónico 100% cubierto sin ambigüedades | `APPLY` |
| Verificación de no-regresión en baseline | Compilación limpia de la base de código actual con flags vigentes | Integración / Build | `bash apps/vigilio-browser/scripts/check.sh` | Compilación exitosa con Ninja, 0 advertencias, tests pasan 100% | `APPLY` |
| Coherencia de referencias canónicas | Validación del árbol de referencias OpenSpec con Prompt Builder | Gobernanza | `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` | Estado `valid: true`, 5 referencias validadas | `APPLY` |
| Verificación del lock criptográfico | Comprobación de integridad del prompt lockfile | Gobernanza | `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0013-module.foundations.task0005.propose.lock.json` | Estado `valid: true`, 13 checks aprobados | `APPLY` |
| Implementación de target `vigilio_compiler_warnings` | Creación del target INTERFACE y vinculación en CMakeLists.txt | Build / Config | `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build` | Configuración limpia sin advertencias de CMake | `LATER` (task0006) |
| Validación de Warnings-as-Errors | Intento de compilación de código con advertencia inducida bajo `-Werror` | Negativo / Compilación | Creación temporal de variable sombreada bajo `VIGILIO_WARNINGS_AS_ERRORS=ON` | Compilador aborta con código de salida != 0 | `LATER` (task0008) |
| Aislamiento de cabeceras de terceros | Compilación con dependencias de SQLite3 y Qt6 bajo flags estrictos | Integración / Build | Compilación completa de `vigilio_storage` y `vigilio-browser` con `SYSTEM` includes | 0 advertencias originadas en cabeceras de terceros | `LATER` (task0006) |
| Impacto visual o controles UI | Verificación de superficie visual en widgets | UI / Accesibilidad | Inspección de `BrowserWindow.cpp` y contratos visuales | Ausencia total de cambios en widgets o strings de UI | `NOT_APPLICABLE` |
