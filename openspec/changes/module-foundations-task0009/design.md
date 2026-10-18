# Design — module-foundations-task0009

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Configuración de descubrimiento de Qt en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Actualmente, las líneas 142 a 177 gobiernan de forma monolítica la búsqueda de Qt6:
     ```cmake
     option(VIGILIO_ENABLE_E2E "Enable resource-intensive native browser E2E smoke" OFF)
     option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
     if(VIGILIO_BUILD_GUI)
       find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)
       if(Qt6_FOUND)
         set(CMAKE_AUTOMOC ON)
         set(CMAKE_AUTORCC ON)
         add_executable(vigilio-browser src/app.cpp src/ui/BrowserWindow.cpp resources.qrc)
         target_include_directories(vigilio-browser PRIVATE src)
         target_link_libraries(vigilio-browser PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets)
         ...
       else()
         message(WARNING "Qt6 Core/Widgets/WebEngineWidgets unavailable: core and tests only; browser GUI NOT built")
       endif()
     endif()
     ```
   - La directiva `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)` solicita una versión mínima de 6.10, pero utiliza `QUIET`, lo que oculta los detalles de diagnóstico cuando un componente específico falla.
   - Si falta cualquiera de los componentes (por ejemplo, `Qt6WebEngineWidgets`), `Qt6_FOUND` se evalúa como falso en bloque, emitiendo un mensaje genérico `message(WARNING ...)` que no especifica cuál de las tres dependencias falló ni su causa.
   - No existe un conmutador de gobernanza para entornos de empaquetado o release oficial (`VIGILIO_REQUIRE_QT`), donde la ausencia de Qt debería abortar la configuración de CMake con error fatal (`FATAL_ERROR`) en lugar de degradar silenciosamente a un build headless.
2. **Presets de compilación (`apps/vigilio-browser/CMakePresets.json`)**:
   - Existen dos presets de configuración: `core-debug` (`VIGILIO_BUILD_GUI=OFF`) y `desktop-debug` (`VIGILIO_BUILD_GUI=ON`).
   - Sin embargo, los presets no definen políticas explícitas sobre qué hacer si las dependencias de Qt no se satisfacen, dependiendo de la degradación implícita de CMake.
3. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - El archivo `app.cpp` asume incondicionalmente que el entorno de ejecución cuenta con las bibliotecas compartidas de Qt y un servidor gráfico accesible:
     ```cpp
     int main(int argc, char** argv) {
         QApplication app(argc, argv);
         ...
         BrowserWindow window;
         window.show();
         return app.exec();
     }
     ```
   - No existe ninguna comprobación previa al instanciar `QApplication`:
     - No se valida si la versión de bibliotecas en runtime (`qVersion()`) coincide con las cabeceras de compilación (`QT_VERSION_STR`).
     - No se verifica la disponibilidad del plugin de plataforma (QPA, e.g. `xcb`, `wayland`, `windows`). Si el binario se ejecuta en un servidor sin display (e.g. sesión SSH sin X11 forwarding), `QApplication` aborta de forma abrupta con un error de Qt en lugar de emitir un diagnóstico estructurado.
     - No se comprueba la existencia ni permisos del ejecutable asistente multiproceso `QtWebEngineProcess`, crítico para el renderizado web.
4. **Ausencia de modelo de dominio en C++**:
   - El código en `src/core/` (`TabModel`, `UrlInput`, `SessionFile`) y `src/storage/` (`LocalRepository`) carece de cualquier representación conceptual o modelo tipado de validación de dependencias del sistema, lo que impide consultar el estado de dependencias desde herramientas de diagnóstico locales o pruebas automatizadas.

### Estado objetivo (TARGET)

1. **Modelo de dominio de dependencias Qt robusto y tipado**:
   - Una taxonomía clara de módulos, versiones y estados de validación en el dominio de `foundations`:
     - `QtModuleId`: enum class que identifica módulos individuales (`Core`, `Widgets`, `WebEngineWidgets`, `Test`, `Network`, `Svg`).
     - `SemanticVersion`: estructura inmutable para parsing, validación y comparación semántica de versiones (`major.minor.patch`).
     - `QtRequirementProfile`: especificación de requerimientos según el perfil de ejecución (`HeadlessCore`, `MinimalGui`, `FullBrowser`, `DeveloperTest`).
     - `QtValidationOutcome`: enum class que modela exhaustivamente los estados posibles (`Satisfied`, `DegradedHeadless`, `IncompatibleVersion`, `MissingComponent`, `EnvironmentError`).
     - `QtValidationReport`: objeto inmutable que consolida la versión detectada, lista de componentes validados, diagnóstico legible y recomendaciones de resolución.
2. **Gobernanza de configuración en CMake (Build-time Validation)**:
   - Separación de responsabilidades y diagnóstico granular en CMake:
     - Detección individualizada de cada componente requerido (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
     - Opción de gobernanza `VIGILIO_REQUIRE_QT` (OFF por defecto para desarrollo local permisivo, ON para perfiles de CI de packaging y release).
     - Diagnósticos detallados en la salida de CMake: versión encontrada, rutas de bibliotecas, componentes satisfechos y advertencias específicas sobre componentes ausentes.
     - Enlace como `SYSTEM` de los directorios de inclusión de Qt, garantizando que advertencias internas de Qt no rompan la política `-Werror` de `vigilio_compiler_warnings`.
3. **Protocolo de validación en tiempo de ejecución (Startup Bootstrap)**:
   - Verificación de precondiciones antes de invocar `QApplication` en el punto de entrada:
     - Validación de compatibilidad binaria: `qVersion()` vs `QT_VERSION_STR` (garantizando compatibilidad ABI dentro de la misma serie mayor/menor).
     - Validación de backend gráfico (QPA): comprobación de variables de entorno de display (`DISPLAY` / `WAYLAND_DISPLAY` en Linux) o detección temprana de plataforma headless para permitir fallback a `-platform offscreen` o mensaje de error claro a `stderr`.
     - Validación de ruta de `QtWebEngineProcess` y flags de aislamiento de procesos Chromium.
4. **Aislamiento headless absoluto y preservación del núcleo**:
   - Invariante arquitectónica: las bibliotecas `vigilio_core` y `vigilio_storage` continúan siendo 100% independientes de Qt. Compilan limpiamente con C++20 estándar y SQLite3, permitiendo su uso en cualquier entorno sin servidor gráfico.

### Brechas identificadas (GAPS)

1. **GAP-001 (Falta de modelo de dominio tipado)**: No existen tipos de datos en C++20 que representen módulos Qt, versiones semánticas ni resultados de validación en el módulo `foundations`.
2. **GAP-002 (Diagnóstico granular ausente en CMake)**: La llamada monolítica a `find_package(Qt6 ... QUIET)` no reporta cuál componente específico falta ni distingue entre versión incompatible o componente ausente.
3. **GAP-003 (Falta de conmutador de fallo estricto)**: No existe la opción `VIGILIO_REQUIRE_QT` para forzar fallo en pipelines de CI donde la GUI es un requisito innegociable.
4. **GAP-004 (Falta de validación de arranque en runtime)**: `app.cpp` no valida la presencia de plugins de plataforma gráfica (QPA) ni la correspondencia de versiones de librerías dinámicas antes de iniciar la interfaz de usuario.
5. **GAP-005 (Falta de pruebas de especificación de dominio)**: No existen pruebas unitarias en `vigilio_core_tests` que validen las reglas de comparación semántica de versiones ni la lógica de decisión de los perfiles de requerimiento.

## Compatibility, migration and rollback

- **Compatibilidad de lenguaje y herramientas**:
  - Estándar del lenguaje: C++20 estricto (`set(CMAKE_CXX_STANDARD 20)`).
  - Versión mínima de Qt: Qt >= 6.10.0 (conforme a Q-002).
  - Compiladores soportados: GNU GCC >= 11, LLVM Clang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Sistemas operativos: Linux (x86_64), Windows 10/11 (x64), macOS (Intel y Apple Silicon).
- **Estrategia de migración**:
  - En esta etapa **domain**, únicamente se definen las reglas, entidades e invariantes en la propuesta OpenSpec.
  - En la etapa `implementation` (`module.foundations.task0010`), se implementarán las estructuras de dominio y la refactorización de CMake de forma retrocompatible: si Qt no está presente, el comportamiento actual de compilar el núcleo headless se mantiene intacto.
- **Estrategia de reversión (rollback)**:
  - En la fase PROPOSE, la reversión se efectúa simplemente eliminando o descartando los artefactos bajo `openspec/changes/module-foundations-task0009/`.
  - La base de código y la configuración de construcción actual permanecen completamente funcionales sin alteración.

## Contexto y evidencia

Se inspeccionó la totalidad del árbol de código y documentación relevante:
1. `apps/vigilio-browser/CMakeLists.txt`: contiene la directiva preliminar de búsqueda de Qt6 (líneas 142-177).
2. `apps/vigilio-browser/CMakePresets.json`: presets `core-debug` (headless) y `desktop-debug` (GUI).
3. `apps/vigilio-browser/src/app.cpp`: inicialización directa de `QApplication` y `BrowserWindow`.
4. `apps/vigilio-browser/src/ui/BrowserWindow.h`: cabecera de ventana principal Qt con `QWebEngineView`.
5. `apps/vigilio-browser/scripts/check.sh`: ejecución exitosa de construcción headless Ninja y 3/3 tests aprobados (`core_units`, `storage_integration`, `vigilio_warnings_tests`), exit code 0.
6. `docs/rules/answers.md`: decisión Q-002 establece C++20 / Qt6.10+ / WebEngine / SQLite / CMake como base tecnológica oficial. Q-001 confirma arquitectura local-first sin nube Vigilio. Q-007 (licencia) permanece OPEN pero solo bloquea `release.public-release`.
7. `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 9 referencias canónicas validadas, exit code 0.
8. `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0025-module.foundations.task0009.propose.lock.json`: 13/13 verificaciones aprobadas al 100%, exit code 0.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0009`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0009/proposal.md`
  - `openspec/changes/module-foundations-task0009/design.md`
  - `openspec/changes/module-foundations-task0009/tasks.md`
  - `openspec/changes/module-foundations-task0009/execution-memory.md`
  - `openspec/changes/module-foundations-task0009/canonical-reference.yaml`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0009/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Diseño completo de la capacidad: Qt dependency validation (etapa domain)

### 1. Modelo de dominio y taxonomía de dependencias Qt

El modelo de dominio formaliza los componentes de software requeridos para la ejecución del navegador, categorizándolos según su rol funcional y criticidad:

```
+-----------------------------------------------------------------------------------+
|                        Modelo de Dominio: Qt Dependency Validation                |
+-----------------------------------------------------------------------------------+
| 1. Identificación de Módulos (QtModuleId)                                         |
|    - Core              : Núcleo de bucle de eventos, cadenas y abstracciones SO   |
|    - Widgets           : Sistema de ventanas y controles de interfaz de usuario   |
|    - WebEngineWidgets  : Motor web Chromium multiproceso y vistas QWebEngineView  |
|    - Test              : Arnés de pruebas unitarias y de integración de Qt        |
|    - Svg               : Renderizado vectorial de iconos Lucide y logos oficiales |
|-----------------------------------------------------------------------------------|
| 2. Versión Semántica (SemanticVersion)                                            |
|    - major : uint32_t (obligatorio == 6)                                          |
|    - minor : uint32_t (mínimo canónico >= 10)                                     |
|    - patch : uint32_t                                                             |
|    - Operaciones: parse(string), isCompatibleWith(minVersion), toString()         |
|-----------------------------------------------------------------------------------|
| 3. Perfiles de Requerimiento (QtRequirementProfile)                               |
|    - HeadlessCore  : Requiere [] (cero Qt; opera en servidores/CI headless)       |
|    - MinimalGui    : Requiere [Core, Widgets] (renderizado básico sin web)        |
|    - FullBrowser   : Requiere [Core, Widgets, WebEngineWidgets] (modo desktop)    |
|    - DeveloperTest : Requiere [Core, Widgets, WebEngineWidgets, Test] (CI GUI)    |
|-----------------------------------------------------------------------------------|
| 4. Estados de Validación (QtValidationOutcome)                                    |
|    - Satisfied                : Todos los módulos requeridos presentes >= 6.10.0  |
|    - DegradedHeadless         : Faltan módulos de GUI; núcleo headless activo     |
|    - IncompatibleVersion      : Versión detectada < 6.10.0 (rechazo estricto)     |
|    - MissingComponent         : Falta un componente crítico (e.g. WebEngine)      |
|    - EnvironmentError         : Fallo de QPA, display ausente o ABI mismatch      |
|-----------------------------------------------------------------------------------|
| 5. Reporte de Validación (QtValidationReport)                                     |
|    - outcome      : QtValidationOutcome                                           |
|    - foundVersion : optional<SemanticVersion>                                     |
|    - modulesFound : vector<QtModuleId>                                            |
|    - missingModules: vector<QtModuleId>                                           |
|    - diagnostic   : string (mensaje estructurado para logs/stderr)                |
+-----------------------------------------------------------------------------------+
```

### 2. Reglas e invariantes de validación

#### Regla R-001: Versión mínima canónica estricta
- Toda compilación que incluya la interfaz gráfica desktop (`VIGILIO_BUILD_GUI=ON`) exige una versión de Qt mayor o igual a **6.10.0**.
- **Invariante**: Se rechaza explícitamente cualquier versión de Qt 5 o versiones de Qt 6 inferiores a 6.10.0. Las versiones de Qt < 6.10 no garantizan el soporte adecuado de las APIs de extensiones Manifest V3 en WebEngine ni las características de C++20 requeridas por el proyecto.

#### Regla R-002: Desacoplamiento e independencia del núcleo (Headless-First)
- Las bibliotecas de dominio y almacenamiento (`vigilio_core` y `vigilio_storage`), así como sus suites de pruebas (`vigilio_core_tests` y `vigilio_storage_tests`), **no deben enlazar ni incluir directa o indirectamente ningún archivo de cabecera de Qt**.
- **Invariante**: El proyecto debe poder configurarse, compilarse y pasar el 100% de sus pruebas unitarias y de integración de persistencia con `VIGILIO_BUILD_GUI=OFF` en sistemas operativos donde Qt no esté instalado.

#### Regla R-003: Política de degradación elegante vs fallo estricto en CMake
- Cuando `VIGILIO_BUILD_GUI=ON`:
  - Si `VIGILIO_REQUIRE_QT=OFF` (modo desarrollo/local predeterminado): Si Qt6 no está instalado o su versión es incompatible, CMake emite una advertencia descriptiva (`message(WARNING ...)`), omite los targets `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke`, y procede exitosamente a compilar las bibliotecas y pruebas headless.
  - Si `VIGILIO_REQUIRE_QT=ON` (modo estricto para pipelines de release/packaging): Si Qt6 no está instalado o falta algún componente requerido (`Core`, `Widgets`, `WebEngineWidgets`), CMake interrumpe la configuración inmediatamente con un error fatal (`message(FATAL_ERROR ...)`), detallando la lista exacta de dependencias no satisfechas.

#### Regla R-004: Inmunización contra advertencias externas (SYSTEM Include)
- Todos los directorios de inclusión generados por los targets importados de Qt (`Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`, `Qt6::Test`) deben ser tratados como cabeceras de sistema (`SYSTEM`).
- **Invariante**: Ninguna advertencia originada dentro del código de terceros de Qt o en el código generado automáticamente por `moc` / `rcc` debe detener la compilación cuando `-Werror` / `/WX` está activado por `vigilio_compiler_warnings`.

#### Regla R-005: Verificación de compatibilidad binaria en tiempo de ejecución (Runtime ABI Check)
- En el arranque de la aplicación de escritorio (`app.cpp`), antes de invocar la construcción de la ventana gráfica:
  - Se evalúa si la versión mayor y menor de la biblioteca dinámica cargada (`qVersion()`) es idéntica a la versión de cabeceras compiladas (`QT_VERSION`).
  - Si existe discrepancia en la versión mayor (ejemplo: ejecutable compilado con Qt6 ejecutándose contra Qt5 compartido), el proceso aborta inmediatamente con mensaje en `stderr` y código de salida 1, previniendo corrupción de memoria o comportamientos indefinidos (UB).

#### Regla R-006: Verificación de entorno gráfico y QPA (Display Server Check)
- En sistemas Linux/BSD, el inicio de `QApplication` requiere un servidor de display activo (`DISPLAY` para X11 o `WAYLAND_DISPLAY` para Wayland).
- Si ninguna variable de display está definida y no se ha especificado un argumento de plataforma alternativo (e.g. `-platform offscreen`), el validador de arranque detecta la condición antes de que Qt emita un fallo no controlado, ofreciendo un mensaje claro y salida limpia con código de error.

### 3. Arquitectura y ciclo de vida de validación

El flujo de vida de la validación de dependencias de Qt abarca dos fases temporales bien diferenciadas:

```
+-----------------------------------------------------------------------------------+
|                        CICLO DE VIDA DE VALIDACIÓN DE QT                          |
+-----------------------------------------------------------------------------------+
| 1. FASE DE CONFIGURACIÓN Y COMPILACIÓN (CMake)                                    |
|                                                                                   |
|  [CMakePresets.json / CLI]                                                        |
|         |                                                                         |
|         v                                                                         |
|  +-----------------------------------------------------------------------------+  |
|  | Inspección de opciones: VIGILIO_BUILD_GUI, VIGILIO_REQUIRE_QT               |  |
|  +-----------------------------------------------------------------------------+  |
|         |                                                                         |
|         v                                                                         |
|  +-----------------------------------------------------------------------------+  |
|  | Detección granular de componentes Qt6 (>= 6.10.0):                          |  |
|  |   - Qt6::Core              -> Requerido por GUI                             |  |
|  |   - Qt6::Widgets           -> Requerido por GUI                             |  |
|  |   - Qt6::WebEngineWidgets  -> Requerido por GUI                             |  |
|  |   - Qt6::Test              -> Opcional (solo si BUILD_TESTING)              |  |
|  +-----------------------------------------------------------------------------+  |
|         |                                                                         |
|         +---> ¿Satisfecho? = SÍ ---> Genera targets GUI (vigilio-browser, etc.)   |
|         |                                                                         |
|         +---> ¿Satisfecho? = NO:                                                  |
|                   |                                                               |
|                   +---> VIGILIO_REQUIRE_QT=ON  ---> message(FATAL_ERROR)          |
|                   |                                                               |
|                   +---> VIGILIO_REQUIRE_QT=OFF ---> Degradación a Headless        |
|                                                     (core y tests headless)       |
+-----------------------------------------------------------------------------------+
| 2. FASE DE ARRANQUE EN RUNTIME (app.cpp)                                          |
|                                                                                   |
|  [main(argc, argv)]                                                               |
|         |                                                                         |
|         v                                                                         |
|  +-----------------------------------------------------------------------------+  |
|  | QtRuntimeValidator::validate()                                              |  |
|  |   1. Comprobar ABI: qVersion() vs QT_VERSION_STR                            |  |
|  |   2. Comprobar display: DISPLAY / WAYLAND_DISPLAY o -platform              |  |
|  |   3. Comprobar helper: QtWebEngineProcess accesible                         |  |
|  +-----------------------------------------------------------------------------+  |
|         |                                                                         |
|         +---> ¿Válido? = SÍ ---> QApplication app(argc, argv) -> BrowserWindow    |
|         |                                                                         |
|         +---> ¿Válido? = NO ---> std::cerr << diagnostic << '\n'; return 1;       |
+-----------------------------------------------------------------------------------+
```

## Archivos afectados

### Archivos verdaderamente existentes inspeccionados:
- `apps/vigilio-browser/CMakeLists.txt`: contiene la directiva condicional de búsqueda de Qt6 que será enriquecida en la etapa de implementación (`task0010`).
- `apps/vigilio-browser/CMakePresets.json`: presets de compilación donde se formalizarán las variables de control de dependencias.
- `apps/vigilio-browser/src/app.cpp`: punto de entrada donde se integrará la pre-validación de runtime en etapas posteriores.
- `apps/vigilio-browser/src/ui/BrowserWindow.h`: declaración de la ventana que depende de `Qt6::Widgets` y `Qt6::WebEngineWidgets`.
- `apps/vigilio-browser/src/ui/BrowserWindow.cpp`: implementación de la interfaz gráfica.
- `apps/vigilio-browser/scripts/check.sh`: arnés de verificación local de calidad.

### Archivos propuestos para etapas subsiguientes (diferidos a APPLY de packs futuros):
- `apps/vigilio-browser/src/core/QtDependencyDomain.h` (diferido a `task0010`): definición de entidades puras C++20 del dominio de validación (`SemanticVersion`, `QtModuleId`, `QtValidationOutcome`, `QtValidationReport`).
- `apps/vigilio-browser/tests/unit/QtDependencyValidationTests.cpp` (diferido a `task0012`): pruebas unitarias deterministas sobre el modelo de dominio y la comparación de versiones semánticas.

## Contrato funcional

- **Actor/Invocación**:
  - En tiempo de configuración: CMake (invocado por el desarrollador, script `check.sh` o runner de CI).
  - En tiempo de ejecución: `main(argc, argv)` en `src/app.cpp` invocado por el usuario o entorno de prueba.
- **Entradas**:
  - Parámetros de CMake: `VIGILIO_BUILD_GUI` (booleano), `VIGILIO_REQUIRE_QT` (booleano), `CMAKE_PREFIX_PATH` (ruta opcional a instalación de Qt).
  - Variables de entorno del sistema: `PATH`, `QT_DIR`, `CMAKE_PREFIX_PATH`, `DISPLAY`, `WAYLAND_DISPLAY`.
  - Argumentos de línea de comandos en runtime: `argc`, `argv`.
- **Salidas**:
  - En tiempo de configuración: Reporte de configuración estructurado en stdout/stderr de CMake con variables de exportación de targets.
  - En tiempo de ejecución: Código de salida `0` si el arranque es exitoso, o código `1` con diagnóstico detallado a `stderr` si la validación falla.
- **Transiciones observables**:
  - `Unchecked -> Validating -> Satisfied` (arranque normal).
  - `Unchecked -> Validating -> DegradedHeadless` (construcción headless exitosa).
  - `Unchecked -> Validating -> Rejected` (aborto controlado con diagnóstico claro).
- **Persistencia**:
  - N/A. La validación de dependencias de Qt no persiste datos en disco ni en SQLite.
- **Deshacer / Cancelar**:
  - N/A. Operación de solo lectura y verificación de entorno libre de efectos secundarios.
- **Concurrencia**:
  - La validación en runtime se ejecuta exclusivamente en el hilo principal de la aplicación antes de instanciar el bucle de eventos de Qt. Sin condiciones de carrera.
- **Recuperación y errores**:
  - Si falta Qt en modo permisivo (`VIGILIO_REQUIRE_QT=OFF`), el sistema degrada a `DegradedHeadless`, emitiendo una advertencia pero completando la construcción del núcleo.

## Contrato UX/UI

- **Declaración formal**: **Sin cambio visual directo**.
- **Justificación**: La capacidad `Qt dependency validation` es una función de infraestructura de compilación y validación de entorno de bajo nivel. No añade pantallas, paneles, botones ni menús a la interfaz del navegador.
- **Cumplimiento de marca y lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
  - Queda estrictamente prohibido exponer términos técnicos internos como «Qt dependency validation», «Qt6», «QPA», «WebEngineWidgets», «Prompt Builder», «OpenSpec» o «Senior Mode» en cualquier superficie visible al usuario.
  - El producto mantiene su terminología aprobada: «Interfaz más fácil de usar» para cualquier configuración de accesibilidad.
- **Interacción con Modo Senior y accesibilidad (`docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md`)**:
  - Esta capacidad preserva íntegramente los contratos existentes de UI y accesibilidad (contraste WCAG 2.2 AA, navegación por teclado, foco visible).
  - Al no modificar elementos gráficos, no existe impacto en el escalado de texto, áreas táctiles de 44-48px ni en los temas visuales (light, dark, midnight, forest, sunset).

## Reglas y fallos

- **Casos de prueba negativos y de borde contemplados en el diseño**:
  1. *Qt totalmente ausente en el sistema*:
     - Modo permisivo: emitir advertencia informativa; compilar bibliotecas headless; no fallar el build.
     - Modo estricto (`VIGILIO_REQUIRE_QT=ON`): abortar configuración con mensaje explicativo de paquetes requeridos.
  2. *Versión de Qt insuficiente (< 6.10.0)*:
     - Detectar Qt 5 o Qt 6.0 a 6.9; rechazar con diagnóstico explícito que indique la versión encontrada y la mínima requerida (>= 6.10.0).
  3. *Versión compatible pero componente WebEngineWidgets faltante*:
     - Detectar que `Core` y `Widgets` están presentes pero `WebEngineWidgets` falta (común en distribuciones Linux donde WebEngine se empaqueta por separado); reportar específicamente el paquete faltante.
  4. *Fallo de QPA en tiempo de ejecución (entorno sin servidor gráfico)*:
     - En Linux, si `DISPLAY` y `WAYLAND_DISPLAY` están vacíos y no se especificó `-platform offscreen`, emitir error estructurado a `stderr` indicando que no se pudo abrir la pantalla gráfica, saliendo con código 1 sin provocar crash de segmentación.
  5. *Discrepancia ABI de bibliotecas compartidas*:
     - Si `qVersion()` mayor no coincide con `QT_VERSION` de compilación, abortar inmediatamente con diagnóstico de incompatibilidad binaria.

## Seguridad y privacidad

- **Límites de confianza**:
  - La validación de dependencias no confía ciegamente en variables de entorno arbitrarias; valida rutas y versiones mediante mecanismos nativos de CMake y funciones del sistema operativo.
- **Modelo Local-First estricto (`docs/skills/07-seguridad-y-privacidad.md`)**:
  - Cero peticiones de red externas.
  - Ninguna descarga automática de binarios o paquetes de terceros desde internet.
  - Cero telemetría, métricas de telemetría remota ni conexión a servidores de Vigilio.
- **Aislamiento de procesos y memoria (`docs/skills/06-rendimiento-y-ram.md`)**:
  - Las estructuras de validación de dependencias tienen una huella de memoria insignificante (< 100 KB).
  - La validación de dependencias no lanza procesos en segundo plano ni deja procesos zombies.
  - No se inician procesos de renderizado Chromium (`QtWebEngineProcess`) durante la verificación de dependencias en tiempo de configuración ni en pruebas unitarias headless.

## Matriz de pruebas trazable

| Criterio de verificación | Tipo de prueba | Fixture / Arnés | Comando de verificación | Resultado esperado | Clasificación |
|---|---|---|---|---|---|
| **CRIT-001**: Validación de modelo de dominio y comparación de versiones semánticas (>= 6.10.0) | Unitaria (C++20) | `tests/unit/QtDependencyValidationTests.cpp` | `ctest -R qt_dependency_domain` | Comparación semántica determinista; versiones < 6.10 rechazadas | LATER (`task0012`) |
| **CRIT-002**: Desacoplamiento estricto del núcleo headless sin cabeceras Qt | Unitaria / Compilación | `tests/unit/CoreTests.cpp`, `vigilio_core` | `cmake --build _build --target vigilio_core_tests` | Compilación limpia con 0 dependencias Qt bajo `-Werror` | APPLY (`task0009`/actual) |
| **CRIT-003**: Detección de Qt6 >= 6.10 en CMake con componentes Core, Widgets, WebEngineWidgets | Integración (CMake) | `CMakeLists.txt` con Qt6 presente | `cmake -S apps/vigilio-browser -B _build_gui -DVIGILIO_BUILD_GUI=ON` | Configuración exitosa de targets GUI cuando Qt6.10+ existe | LATER (`task0010`) |
| **CRIT-004**: Degradación elegante a modo headless cuando Qt6 está ausente (`VIGILIO_REQUIRE_QT=OFF`) | Integración (CMake) | Entorno sin Qt6 o ruta vacía | `cmake -S apps/vigilio-browser -B _build_headless -DVIGILIO_BUILD_GUI=OFF` | CMake emite advertencia pero completa configuración con éxito | APPLY (`task0009`/actual) |
| **CRIT-005**: Rechazo estricto con error fatal en CMake cuando Qt6 falta (`VIGILIO_REQUIRE_QT=ON`) | Negativa (CMake) | Entorno sin Qt6 con opción forzada | `cmake -S apps/vigilio-browser -B _build_fail -DVIGILIO_REQUIRE_QT=ON` | CMake aborta con FATAL_ERROR detallando dependencias faltantes | LATER (`task0010`) |
| **CRIT-006**: Verificación de runtime ABI (qVersion vs QT_VERSION_STR) en startup | Unitaria / Integración | Fixture de versiones sintetizadas | `ctest -R qt_runtime_validation` | Coincidencia de versiones aprobada; discrepancia rechazada con código 1 | LATER (`task0012`) |
| **CRIT-007**: Ausencia formal de cambio visual en UI y cumplimiento de lenguaje público | Documental / Estática | Auditoría de archivos fuente y `UX-COPY-PUBLICO.md` | Inspección de diffs y grep de UI labels | Cero términos técnicos en UI; sin cambios visuales directos | APPLY (`task0009`/actual) |
| **CRIT-008**: No-regresión del baseline headless y suite rápida | Integración completa | `scripts/check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | Compilación Ninja limpia y 100% pruebas aprobadas (exit code 0) | APPLY (`task0009`/actual) |

## Pasos APPLY

Cuando este change sea aprobado por el revisor humano para proceder a la fase APPLY:
1. **Actualización de documentación canónica**:
   - Actualizar el estado de ejecución y registrar las invariantes de dominio en `docs/rules/01-modules/foundations/micro/0009-qt-dependency-validation/task.md`.
2. **Validación de la no-regresión de la arquitectura modular**:
   - Confirmar que ninguna cabecera o símbolo de Qt se filtre a `vigilio_core` o `vigilio_storage`.
3. **Verificación automatizada y revalidación de gobernanza**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` comprobando que el baseline se mantiene 100% verde bajo `-Werror`.
   - Ejecutar las validaciones de Prompt Builder (`validate-references` y `validate-decisions`).

## Definición de terminado (Definition of Done)

- [x] Especificación formal del modelo de dominio de dependencias Qt (`QtModuleId`, `SemanticVersion`, `QtRequirementProfile`, `QtValidationOutcome`, `QtValidationReport`) completada.
- [x] Invariantes de versión mínima (Qt >= 6.10.0), C++20 y compatibilidad de plataforma formalizadas según Q-002 y `docs/rules/answers.md`.
- [x] Reglas de validación para tiempo de compilación (CMake) y tiempo de arranque (runtime bootstrap) detalladas.
- [x] Desacoplamiento headless absoluto de `vigilio_core` y `vigilio_storage` garantizado y verificado.
- [x] Ausencia de impacto visual directo documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0008` (tests compiler warnings, completada con éxito).
- **Dependencias posteriores**: `module.foundations.task0010` (implementación de Qt dependency validation en C++20 y CMake).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de Qt6 en plataformas mínimas de CI.
    *Mitigación*: Arquitectura headless-first donde `vigilio_core` y pruebas unitarias/integración no dependen de Qt y compilan limpiamente en modo headless.
  - *Riesgo*: Discrepancia de comportamiento entre plataformas (Linux vs Windows vs macOS) en nombres de módulos o rutas de plugins QPA.
    *Mitigación*: Uso exclusivo de abstracciones canónicas de CMake (`find_package(Qt6 COMPONENTS ...)`) y APIs portables de Qt.

## Commit propuesto

`docs(foundations): domain qt-dependency-validation`
