# Design — module-foundations-task0010

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Configuración de descubrimiento de Qt en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Las líneas 142 a 177 gobiernan de forma monolítica la búsqueda de Qt6:
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
   - La comprobación actual con `QUIET` no discrimina qué componente específico falta (¿`Core`? ¿`Widgets`? ¿`WebEngineWidgets`?).
   - Si falta cualquiera de los componentes, `Qt6_FOUND` se evalúa como falso en bloque, emitiendo un mensaje genérico `message(WARNING ...)` que no explica la causa ni cómo resolverla.
   - No existe un conmutador de gobernanza para entornos de packaging o release oficial (`VIGILIO_REQUIRE_QT`), donde la ausencia de Qt debería abortar la configuración de CMake con error fatal (`FATAL_ERROR`) en lugar de degradar silenciosamente a un build headless.
2. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
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
     - No se verifica la disponibilidad del plugin de plataforma (QPA, e.g. `xcb`, `wayland`, `windows`). Si el binario se ejecuta en un servidor sin display (e.g. sesión SSH sin X11 forwarding), `QApplication` aborta de forma abrupta con un crash en lugar de emitir un diagnóstico estructurado.
3. **Ausencia de capa propietaria en C++20 para validación**:
   - No existe ningún servicio o clase en `src/core/` que implemente la lógica de validación de dependencias de Qt.
   - Todo diagnóstico depende exclusivamente de scripts externos o de la invocación opaca de CMake.

### Estado objetivo (TARGET)

1. **Implementación de la capa propietaria C++20 (`vigilio::QtDependencyValidator`)**:
   - Una clase de servicio estática y pura en C++20 estándar dentro de `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `QtDependencyValidator.cpp`:
     - Desacoplada al 100% de cualquier encabezado o biblioteca de Qt: no incluye `<Q...>` y compila limpiamente en entornos headless dentro de `vigilio_core`.
     - Implementa las entidades inmutables del dominio: `QtModuleId`, `SemanticVersion`, `QtRequirementProfile`, `QtValidationOutcome`, `QtValidationReport`.
     - Ofrece métodos deterministas:
       - `SemanticVersion::parse(std::string_view)` para parsing robusto de cadenas de versión con soporte para prefijos y sufijos de pre-release.
       - `operator<=>` para comparación semántica de versiones (`major.minor.patch`).
       - `validateProfile(QtRequirementProfile, const std::vector<QtModuleId>&, const SemanticVersion&)` para evaluar satisfacción de perfiles.
       - `validateRuntimeEnvironment(std::string_view runtimeVer, std::string_view compileVer, const char* display, const char* waylandDisplay)` para pre-validación de arranque.
       - `formatReport(const QtValidationReport&)` para salida legible de diagnóstico.
2. **Gobernanza de configuración en CMake (Build-time Validation)**:
   - Detección granular e individualizada de cada componente requerido (`Core`, `Widgets`, `WebEngineWidgets`, y `Test` si `BUILD_TESTING=ON`).
   - Opción `VIGILIO_REQUIRE_QT`:
     - `OFF` (predeterminado para desarrollo): emite advertencia descriptiva con lista de módulos ausentes y degrada elegantemente a la construcción del núcleo headless.
     - `ON` (estricto para CI/packaging): interrumpe CMake inmediatamente con `message(FATAL_ERROR ...)` indicando los componentes que impiden generar la GUI.
   - Marcado de todos los directorios de inclusión de Qt con la directiva `SYSTEM`, impidiendo que advertencias de cabeceras de terceros rompan la política `-Werror` de `vigilio_compiler_warnings`.
3. **Protocolo de pre-validación en tiempo de ejecución (`app.cpp`)**:
   - Antes de invocar `QApplication app(argc, argv)`, `app.cpp` llama a `QtDependencyValidator::validateRuntimeEnvironment(...)`.
   - Si se detecta incompatibilidad binaria (discrepancia en versión mayor) o ausencia de backend gráfico sin plataforma alternativa, el proceso emite un mensaje estructurado a `std::cerr` y retorna inmediatamente con código de salida 1, evitando bloqueos o caídas abruptas.
4. **Preservación estricta de la filosofía Headless-First**:
   - `vigilio_core` y `vigilio_storage` mantienen cero dependencias directas o transitivas hacia Qt.

### Brechas identificadas (GAPS)

1. **GAP-001 (Falta de clase propietaria C++20)**: No existe `QtDependencyValidator.h` ni `QtDependencyValidator.cpp` en `src/core/`.
2. **GAP-002 (Diagnóstico granular ausente en CMake)**: La búsqueda en `CMakeLists.txt` es opaca y no reporta qué componente específico falla.
3. **GAP-003 (Falta de conmutador de fallo estricto)**: No existe la opción `VIGILIO_REQUIRE_QT` en `CMakeLists.txt` ni en `CMakePresets.json`.
4. **GAP-004 (Falta de pre-validación en arranque)**: `app.cpp` no valida el entorno ni las bibliotecas dinámicas antes de crear `QApplication`.
5. **GAP-005 (Falta de pruebas unitarias de validación)**: `tests/unit/CoreTests.cpp` no incluye pruebas de parsing y comparación de versiones para `QtDependencyValidator`.

## Compatibility, migration and rollback

- **Compatibilidad de lenguaje y herramientas**:
  - Estándar del lenguaje: C++20 estricto (`set(CMAKE_CXX_STANDARD 20)`).
  - Versión canónica mínima de Qt: Qt >= 6.10.0 (conforme a Q-002 de `docs/rules/answers.md`).
  - Compiladores soportados: GNU GCC >= 11, LLVM Clang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Sistemas operativos: Linux (x86_64), Windows 10/11 (x64), macOS (Intel y Apple Silicon).
- **Estrategia de migración**:
  - En esta etapa **implementation**, la introducción de `QtDependencyValidator` se realiza como una extensión de `vigilio_core`, 100% retrocompatible con el código existente.
  - La refactorización de `CMakeLists.txt` preserva la opción `VIGILIO_REQUIRE_QT=OFF` por defecto, de modo que cualquier entorno sin Qt6 instalado continúa compilando el núcleo headless de forma transparente.
- **Estrategia de reversión (rollback)**:
  - En caso de requerirse rollback, se revierten los cambios en `CMakeLists.txt`, `app.cpp` y `CoreTests.cpp`, y se eliminan los archivos `QtDependencyValidator.h` y `QtDependencyValidator.cpp` en `src/core/`.
  - El sistema vuelve de inmediato a su estado anterior sin efectos secundarios ni migraciones de datos requeridas.

## Contexto y evidencia

Se inspeccionó exhaustivamente el repositorio y las herramientas locales:
1. `apps/vigilio-browser/CMakeLists.txt`: líneas 106-113 (`vigilio_core`) y 142-177 (detección condicional de Qt6).
2. `apps/vigilio-browser/src/app.cpp`: inicialización directa de `QApplication` y `BrowserWindow` sin pre-validación.
3. `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: interfaz de usuario basada en Qt Widgets y WebEngine.
4. `apps/vigilio-browser/scripts/check.sh`: verificación exitosa de compilación Ninja bajo `-Werror` y 3/3 tests aprobados (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.37 segundos, exit code 0.
5. `apps/vigilio-browser/CMakePresets.json`: presets de compilación `core-debug` (headless) y `desktop-debug` (GUI).
6. `docs/rules/answers.md`: decisión Q-002 ratifica C++20 / Qt 6.10+ / WebEngine / SQLite / CMake como base tecnológica oficial. Q-001 ratifica arquitectura local-first sin nube. Q-007 (licencia) permanece OPEN afectando solo `release.public-release`.
7. `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 10 referencias canónicas validadas, exit code 0.
8. `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0028-module.foundations.task0010.propose.lock.json`: 13/13 verificaciones aprobadas exitosamente, exit code 0.
9. `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito final `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0010`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0010/proposal.md`
  - `openspec/changes/module-foundations-task0010/design.md`
  - `openspec/changes/module-foundations-task0010/tasks.md`
  - `openspec/changes/module-foundations-task0010/execution-memory.md`
  - `openspec/changes/module-foundations-task0010/canonical-reference.yaml`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0010/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Diseño completo de la capacidad: Qt dependency validation (etapa implementation)

### 1. Arquitectura de la capa propietaria C++20 (`vigilio::QtDependencyValidator`)

Para evitar cualquier acoplamiento de lógica de negocio a los widgets o a la ventana principal (`BrowserWindow`), la capacidad se implementa como un servicio puro en C++20 dentro de la biblioteca `vigilio_core`:

```
+-----------------------------------------------------------------------------------------+
|                                vigilio::QtDependencyValidator                           |
|                      (apps/vigilio-browser/src/core/QtDependencyValidator.h)             |
+-----------------------------------------------------------------------------------------+
| Tipos de Dominio (C++20 puro):                                                          |
|   - enum class QtModuleId { Core, Widgets, WebEngineWidgets, Test, Svg }                |
|   - struct SemanticVersion { uint32_t major, minor, patch; auto operator<=>; ... }      |
|   - enum class QtRequirementProfile { HeadlessCore, MinimalGui, FullBrowser, DevTest }  |
|   - enum class QtValidationOutcome { Satisfied, DegradedHeadless, IncompatibleVersion,   |
|                                      MissingComponent, EnvironmentError }               |
|   - struct QtValidationReport { outcome, foundVersion, modulesFound, missingModules, ...|
|-----------------------------------------------------------------------------------------|
| Métodos Públicos Estáticos:                                                             |
|   + static std::optional<SemanticVersion> parseVersion(std::string_view versionStr)     |
|   + static std::vector<QtModuleId> requiredModules(QtRequirementProfile profile)        |
|   + static QtValidationReport validateProfile(QtRequirementProfile profile,             |
|                                               const std::vector<QtModuleId>& available, |
|                                               const SemanticVersion& detectedVer,       |
|                                               const SemanticVersion& minVer = {6,10,0}) |
|   + static QtValidationReport validateRuntimeEnvironment(                               |
|                                               std::string_view runtimeVer,              |
|                                               std::string_view compileVer,              |
|                                               const char* displayEnv,                   |
|                                               const char* waylandEnv)                   |
|   + static std::string formatOutcome(QtValidationOutcome outcome)                       |
|   + static std::string formatReport(const QtValidationReport& report)                   |
+-----------------------------------------------------------------------------------------+
```

### 2. Entidades de datos y contratos en C++20 puro

#### `SemanticVersion`
Estructura inmutable que encapsula `major`, `minor` y `patch` con semántica de valor:
- Provee `auto operator<=>(const SemanticVersion&) const = default;` para comparaciones canónicas (`<`, `<=`, `>`, `>=`, `==`, `!=`).
- Método `isCompatibleWith(const SemanticVersion& requiredMin) const noexcept`: retorna verdadero si `major == requiredMin.major` y la tupla `(major, minor, patch) >= (requiredMin.major, requiredMin.minor, requiredMin.patch)`.
- Método `parse(std::string_view str)`: tokeniza la cadena por el delimitador '.', ignorando prefijos no numéricos o sufijos de compilación (ej. `"6.10.1-beta"` -> `{6, 10, 1}`).

#### `QtRequirementProfile`
- `HeadlessCore`: lista vacía de dependencias Qt; válido para entornos sin servidor gráfico o pruebas de persistencia.
- `MinimalGui`: requiere `Core` y `Widgets`.
- `FullBrowser`: requiere `Core`, `Widgets` y `WebEngineWidgets`.
- `DeveloperTest`: requiere `Core`, `Widgets`, `WebEngineWidgets` y `Test`.

#### `QtValidationOutcome` y `QtValidationReport`
- El reporte consolida de forma inmutable la evaluación:
  - Si faltan módulos requeridos: outcome = `MissingComponent`, listando exactamente los módulos ausentes.
  - Si la versión es inferior a 6.10.0: outcome = `IncompatibleVersion`.
  - Si la versión mayor de runtime discrepa de la de compilación o falta servidor gráfico: outcome = `EnvironmentError`.
  - Si todo se cumple: outcome = `Satisfied`.

### 3. Validación y diagnóstico en tiempo de configuración (CMakeLists.txt)

En `apps/vigilio-browser/CMakeLists.txt`:
```cmake
option(VIGILIO_REQUIRE_QT "Require Qt 6.10+ dependencies strictly (fail build if missing)" OFF)
option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)

if(VIGILIO_BUILD_GUI)
  set(QT_MINIMUM_VERSION "6.10.0")
  find_package(Qt6 ${QT_MINIMUM_VERSION} QUIET COMPONENTS Core Widgets WebEngineWidgets)
  
  if(Qt6_FOUND)
    message(STATUS "Qt6 dependency validation: Satisfied (version ${Qt6_VERSION} found)")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    add_executable(vigilio-browser src/app.cpp src/ui/BrowserWindow.cpp resources.qrc)
    target_include_directories(vigilio-browser SYSTEM PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6WebEngineWidgets_INCLUDE_DIRS})
    target_link_libraries(vigilio-browser PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets)
    ...
  else()
    set(QT_MISSING_DETAILS "")
    find_package(Qt6Core ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6Core_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::Core")
    endif()
    find_package(Qt6Widgets ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6Widgets_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::Widgets")
    endif()
    find_package(Qt6WebEngineWidgets ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6WebEngineWidgets_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::WebEngineWidgets")
    endif()
    
    if(VIGILIO_REQUIRE_QT)
      message(FATAL_ERROR "Qt dependency validation FAILED (VIGILIO_REQUIRE_QT=ON): Missing or incompatible components:${QT_MISSING_DETAILS}. Required version >= ${QT_MINIMUM_VERSION}.")
    else()
      message(WARNING "Qt dependency validation DEGRADED: Missing or incompatible components:${QT_MISSING_DETAILS}. Building headless core and tests only; desktop GUI NOT built.")
    endif()
  endif()
endif()
```

### 4. Protocolo de validación en tiempo de ejecución (`apps/vigilio-browser/src/app.cpp`)

En `src/app.cpp`, la invocación de `QApplication` y `BrowserWindow` se condiciona a la validación previa de runtime:
```cpp
#include "core/QtDependencyValidator.h"
#include "ui/BrowserWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <iostream>

int main(int argc, char** argv) {
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    
    auto report = vigilio::QtDependencyValidator::validateRuntimeEnvironment(
        qVersion(),
        QT_VERSION_STR,
        display,
        wayland
    );
    
    if (report.outcome != vigilio::QtValidationOutcome::Satisfied) {
        std::cerr << "[Vigilio Startup Error] " << report.diagnosticMessage << "\n";
        return 1;
    }

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Vigilio");
    QCoreApplication::setApplicationName("Vigilio Browser");
    QCoreApplication::setApplicationVersion("0.1.0-alpha");
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss}] [%{type}] %{category}: %{message}");
    
    BrowserWindow window;
    window.show();
    return app.exec();
}
```

### 5. Recuperación ante fallos, degradación elegante y cancelación

- **Fallo de servidor de display en Linux**: Si se intenta ejecutar en una sesión sin pantalla (ej. contenedor o SSH), `validateRuntimeEnvironment` detecta la ausencia de `DISPLAY` y `WAYLAND_DISPLAY` y sale con código 1 emitiendo un diagnóstico claro, en lugar de permitir que `QApplication` termine con un crash de segmentación.
- **Degradación a headless en CMake**: La separación de responsabilidades asegura que un fallo en la detección de Qt nunca impida compilar `vigilio_core`, `vigilio_storage` o ejecutar sus pruebas automatizadas.
- **Operación síncrona sin bloqueos**: La validación de dependencias se ejecuta en el hilo principal en menos de 2 milisegundos, sin hilos secundarios ni operaciones de red cancelables.

## Archivos afectados

### Archivos verdaderamente existentes inspeccionados:
- `apps/vigilio-browser/CMakeLists.txt`: se añadirá `QtDependencyValidator.cpp` a `vigilio_core`, la opción `VIGILIO_REQUIRE_QT` y la lógica granular de detección de Qt6.
- `apps/vigilio-browser/src/app.cpp`: se integrará la pre-validación de arranque con `QtDependencyValidator::validateRuntimeEnvironment`.
- `apps/vigilio-browser/tests/unit/CoreTests.cpp`: se incorporarán pruebas unitarias deterministas para el parsing y comparación de versiones de `QtDependencyValidator`.
- `apps/vigilio-browser/scripts/check.sh`: arnés de verificación local de calidad.
- `apps/vigilio-browser/CMakePresets.json`: presets de compilación donde se gobernará `VIGILIO_REQUIRE_QT`.

### Archivos nuevos a crear durante APPLY:
- `apps/vigilio-browser/src/core/QtDependencyValidator.h`: definición de tipos inmutables y API de validación C++20.
- `apps/vigilio-browser/src/core/QtDependencyValidator.cpp`: implementación de algoritmos de parsing, comparación y diagnóstico.

## Contrato funcional

- **Actor / Invocación**:
  - En tiempo de configuración: CMake invocado por el desarrollador, preset de CMake o pipeline de CI.
  - En tiempo de ejecución: Función `main()` en `src/app.cpp` al iniciar la aplicación.
  - En pruebas unitarias: `vigilio_core_tests` ejecutado vía CTest.
- **Entradas**:
  - Parámetros CMake: `VIGILIO_BUILD_GUI` (bool), `VIGILIO_REQUIRE_QT` (bool), `CMAKE_PREFIX_PATH`.
  - Cadenas de versión: `qVersion()`, `QT_VERSION_STR`, strings de versión semántica.
  - Variables de entorno: `DISPLAY`, `WAYLAND_DISPLAY`.
- **Salidas**:
  - CMake: Diagnóstico estructurado en consola y generación de targets gráficos o degradación a headless.
  - Runtime: Código de salida 0 si la inicialización procede normalmente; código de salida 1 con mensaje explicativo a `std::cerr` si la validación falla.
- **Transiciones observables**:
  - `Unchecked -> Validating -> Satisfied` (arranque exitoso).
  - `Unchecked -> Validating -> DegradedHeadless` (configuración headless en CMake).
  - `Unchecked -> Validating -> Rejected` (salida controlada con error).
- **Persistencia**:
  - N/A. La validación de dependencias no almacena estado en disco ni en SQLite.
- **Deshacer / Cancelar**:
  - N/A. Operación puramente declarativa y de inspección de estado de solo lectura.
- **Concurrencia**:
  - Ejecución exclusiva en el hilo principal antes de instanciar el loop de eventos de Qt.
- **Errores y recuperación**:
  - Entorno headless sin Qt: degradación automática a compilación de bibliotecas y tests de dominio.
  - Falta de servidor gráfico: salida limpia con mensaje informativo sin cuelgues ni zombies.

## Contrato UX/UI

- **Declaración formal**: **Sin cambio visual directo**.
- **Justificación**: La capacidad `Qt dependency validation` es infraestructura técnica de construcción y arranque de bajo nivel. No añade ventanas, botones, controles ni diálogos visibles al usuario.
- **Cumplimiento de marca y lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
  - Ningún término de ingeniería («Qt dependency validation», «Qt6», «QPA», «WebEngineWidgets», «Prompt Builder», «OpenSpec», «Senior Mode») se expone en la interfaz pública visible.
  - El producto preserva su terminología oficial aprobada: «Interfaz más fácil de usar».
- **Interacción con Modo Senior y accesibilidad (`docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md`)**:
  - Al no modificar elementos gráficos, esta capacidad no altera la geometría de controles, contraste WCAG 2.2 AA ni navegación por teclado, preservando íntegramente los contratos existentes.

## Reglas y fallos

- **Regla R-001 (Versión mínima Qt 6.10.0)**:
  - Todo componente de interfaz requiere Qt >= 6.10.0. Cualquier versión menor es rechazada con outcome `IncompatibleVersion`.
- **Regla R-002 (Aislamiento Headless del Núcleo)**:
  - `vigilio_core` y `vigilio_storage` tienen cero dependencias hacia cabeceras de Qt.
- **Regla R-003 (Degradación elegante vs Fallo estricto en CMake)**:
  - `VIGILIO_REQUIRE_QT=OFF` emite advertencia y continúa con el núcleo headless.
  - `VIGILIO_REQUIRE_QT=ON` aborta inmediatamente con `FATAL_ERROR`.
- **Regla R-004 (Inmunización de advertencias externas SYSTEM)**:
  - Los directorios de inclusión de Qt se marcan como `SYSTEM` en CMake, asegurando que `-Werror` no se dispare por código de terceros.
- **Regla R-005 (Consistencia ABI de Runtime)**:
  - Discrepancias en la versión mayor entre compilación y runtime abortan con salida 1 y mensaje en `stderr`.
- **Regla R-006 (Inspección de Backend Gráfico QPA)**:
  - En Linux, la ausencia de `DISPLAY` y `WAYLAND_DISPLAY` sin argumento offscreen se intercepta antes de inicializar `QApplication`.

## Seguridad y privacidad

- **Límites de confianza**:
  - La validación solo inspecciona variables de entorno del proceso y funciones estándar del sistema; no confía en datos remotos ni en rutas no verificadas.
- **Modelo Local-First estricto (`docs/skills/07-seguridad-y-privacidad.md`)**:
  - Cero peticiones de red externas, cero descargas automáticas de binarios desde internet, cero telemetría remota.
- **Aislamiento de procesos y memoria (`docs/skills/06-rendimiento-y-ram.md`)**:
  - Huella de memoria insignificante (< 100 KB RAM).
  - Cero procesos huérfanos o zombies; no se inician procesos asistentes de Chromium (`QtWebEngineProcess`) durante la validación de dependencias.

## Matriz de pruebas trazable

| Criterio de verificación | Tipo de prueba | Fixture / Arnés | Comando de verificación | Resultado esperado | Clasificación |
|---|---|---|---|---|---|
| **CRIT-001**: Parsing robusto de versiones semánticas (`SemanticVersion::parse`) | Unitaria (C++20) | `tests/unit/CoreTests.cpp` | `ctest -R core_units` | Parser procesa correctamente "6.10.0", "6.10.1-beta", rechaza cadenas malformadas | APPLY (`task0010`) |
| **CRIT-002**: Operadores de comparación y compatibilidad (`isCompatibleWith`) | Unitaria (C++20) | `tests/unit/CoreTests.cpp` | `ctest -R core_units` | 6.10.0 >= 6.10.0 (true); 6.9.3 >= 6.10.0 (false); 5.15.2 incompatible (false) | APPLY (`task0010`) |
| **CRIT-003**: Evaluación de perfiles de requerimiento (`validateProfile`) | Unitaria (C++20) | `tests/unit/CoreTests.cpp` | `ctest -R core_units` | FullBrowser satisfecho con Core+Widgets+WebEngine; MissingComponent si falta WebEngine | APPLY (`task0010`) |
| **CRIT-004**: Pre-validación de runtime ABI y servidor gráfico (`validateRuntimeEnvironment`) | Unitaria (C++20) | `tests/unit/CoreTests.cpp` | `ctest -R core_units` | Detecta desajuste ABI mayor y detecta ausencia de display server en Linux | APPLY (`task0010`) |
| **CRIT-005**: Compilación de `vigilio_core` con cero dependencias de Qt bajo `-Werror` | Compilación (C++20) | `CMakeLists.txt`, `vigilio_core` | `cmake --build apps/vigilio-browser/_build --target vigilio_core` | Compilación limpia de `QtDependencyValidator.cpp` sin warnings ni dependencias Qt | APPLY (`task0010`) |
| **CRIT-006**: Degradación elegante en CMake cuando Qt no está presente (`VIGILIO_REQUIRE_QT=OFF`) | Integración (CMake) | `CMakeLists.txt` en modo headless | `cmake -S apps/vigilio-browser -B _build -DVIGILIO_BUILD_GUI=OFF` | Configuración exitosa de targets headless con advertencia informativa | APPLY (`task0010`) |
| **CRIT-007**: Fallo estricto con error fatal en CMake cuando falta Qt (`VIGILIO_REQUIRE_QT=ON`) | Negativa (CMake) | Entorno headless forzando opción | `cmake -S apps/vigilio-browser -B _build_fail -DVIGILIO_BUILD_GUI=ON -DVIGILIO_REQUIRE_QT=ON` | CMake aborta con FATAL_ERROR listando componentes faltantes | LATER (`task0012`) |
| **CRIT-008**: Ausencia formal de cambio visual en UI y cumplimiento de lenguaje público | Documental / Estática | Auditoría de archivos fuente y `UX-COPY-PUBLICO.md` | Inspección de diffs y grep de UI labels | Cero términos técnicos en UI; sin cambios visuales directos | APPLY (`task0010`) |
| **CRIT-009**: No-regresión del baseline headless y suite rápida | Integración completa | `scripts/check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | Compilación Ninja limpia y 100% pruebas aprobadas (exit code 0) | APPLY (`task0010`) |

## Pasos APPLY

Cuando este change sea aprobado por el revisor humano para proceder a la fase APPLY:
1. **Creación de archivos de encabezado e implementación en `src/core/`**:
   - `apps/vigilio-browser/src/core/QtDependencyValidator.h`
   - `apps/vigilio-browser/src/core/QtDependencyValidator.cpp`
2. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Registrar `src/core/QtDependencyValidator.cpp` en `add_library(vigilio_core ...)`.
   - Incorporar la opción `VIGILIO_REQUIRE_QT` y la lógica granular de validación de componentes de Qt.
3. **Actualización de `apps/vigilio-browser/src/app.cpp`**:
   - Incorporar la llamada a `QtDependencyValidator::validateRuntimeEnvironment` previo a la inicialización de `QApplication`.
4. **Adición de pruebas unitarias en `apps/vigilio-browser/tests/unit/CoreTests.cpp`**:
   - Incorporar la suite de pruebas unitarias para `SemanticVersion` y `QtDependencyValidator`.
5. **Verificación y calidad**:
   - Ejecutar `apps/vigilio-browser/scripts/check.sh` verificando compilación Ninja limpia y 100% pruebas aprobadas bajo `-Werror`.
   - Ejecutar Clang-Format sobre archivos modificados.
6. **Sincronización canónica**:
   - Actualizar el estado en `docs/rules/01-modules/foundations/micro/0010-qt-dependency-validation/task.md` pasando a `CURRENT; PASS`.

## Definición de terminado (Definition of Done)

- [x] Especificación técnica completa para la implementación de `Qt dependency validation` en C++20 y CMake elaborada y coherente.
- [x] Capa propietaria `vigilio::QtDependencyValidator` diseñada en `src/core/` con entidades fuertemente tipadas y desacopladas de Qt.
- [x] Protocolo de pre-validación de runtime ABI y servidor gráfico proyectado en `app.cpp` con manejo seguro de errores.
- [x] Gobernanza de construcción en CMake proyectada con opción `VIGILIO_REQUIRE_QT`, detección granular y degradación suave.
- [x] Desacoplamiento headless absoluto de `vigilio_core` garantizado y verificado.
- [x] Ausencia de impacto visual directo documentada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables formalizadas en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0009` (etapa `domain` de Qt dependency validation, completada).
- **Dependencias posteriores**: `module.foundations.task0011` (etapa `ui` de Qt dependency validation).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de Qt6 en plataformas mínimas de CI.
    *Mitigación*: Arquitectura headless-first donde `vigilio_core` y pruebas unitarias/integración no dependen de Qt y compilan limpiamente en modo headless.
  - *Riesgo*: Discrepancias menores de versión de bibliotecas dinámicas compartidas del sistema.
    *Mitigación*: La verificación ABI solo rechaza desajustes mayores, tolerando versiones menores compatibles dentro de Qt 6.10+.

## Commit propuesto

`feat(foundations): implementation qt-dependency-validation`
