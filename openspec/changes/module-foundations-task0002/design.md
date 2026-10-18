# Design — module-foundations-task0002

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- `apps/vigilio-browser/CMakeLists.txt` define actualmente una biblioteca estática `vigilio_core` que agrupa indistintamente código de negocio (`UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`) y persistencia en base de datos (`LocalRepository.cpp`).
- `vigilio_core` enlaza directamente con `SQLite::SQLite3` mediante `find_package(SQLite3 REQUIRED)`.
- El archivo define opciones de compilación privadas (`-Wall -Wextra -Wpedantic -Wconversion` en GCC/Clang y `/W4 /permissive-` en MSVC) únicamente sobre `vigilio_core`, sin aplicarlas sistemáticamente a todos los componentes del proyecto.
- Los ejecutables de pruebas unitarias (`vigilio_core_tests`) e integración (`vigilio_storage_tests`) se definen bajo la guarda `BUILD_TESTING`, enlazando ambos a `vigilio_core`, lo que provoca que las pruebas puramente de dominio carguen innecesariamente la biblioteca de SQLite.
- El ejecutable principal `vigilio-browser` y la prueba de humo `vigilio_e2e_smoke` se encuentran bajo la guarda condicional `VIGILIO_BUILD_GUI`, buscando `Qt6` de forma silenciosa (`QUIET`).
- Existen presets en `CMakePresets.json` (`core-debug` y `desktop-debug`) y un script de validación rápida `apps/vigilio-browser/scripts/check.sh`.

### Estado objetivo (TARGET)
- Arquitectura modular limpia en la capa de targets de CMake donde cada componente C++20 refleja fielmente los principios SOLID (Single Responsibility Principle):
  1. **Core Domain Target (`vigilio_core`)**: biblioteca estática pura que contiene exclusivamente la lógica de negocio y dominio (`UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`), sin enlaces ni dependencias de SQLite ni de bibliotecas gráficas.
  2. **Storage Target (`vigilio_storage`)**: biblioteca estática de persistencia que encapsula el acceso a base de datos (`LocalRepository.cpp`), enlazando formalmente `SQLite::SQLite3` y dependiendo de `vigilio_core`.
  3. **Application Target (`vigilio-browser`)**: ejecutable de interfaz gráfica que compone la aplicación vinculando `vigilio_core`, `vigilio_storage` y los componentes de Qt6 (`Core`, `Widgets`, `WebEngineWidgets`).
  4. **Test Targets Especializados**:
     - `vigilio_core_tests`: enfocado en pruebas unitarias de dominio, enlazado únicamente a `vigilio_core`.
     - `vigilio_storage_tests`: enfocado en pruebas de integración con SQLite, enlazado a `vigilio_storage`.
     - `vigilio_e2e_smoke`: prueba de recorrido gráfico básico condicionada a `VIGILIO_ENABLE_E2E=ON`.
- Configuración estricta y transitiva de características del estándar mediante `target_compile_features(vigilio_core PUBLIC cxx_std_20)`.
- Opciones de compilación rigurosas aplicadas a los targets para prevenir advertencias y conversiones de tipo inseguras.
- Mantenimiento estricto del soporte headless (`VIGILIO_BUILD_GUI=OFF`) para permitir compilar y ejecutar pruebas de lógica y almacenamiento en servidores CI sin servidor X11/Wayland.

### Brechas identificadas (GAPS)
- **Acoplamiento innecesario**: `vigilio_core` compila conjuntamente código de dominio y de almacenamiento SQLite, diluyendo los límites de responsabilidad del módulo.
- **Sobredependencia en pruebas unitarias**: `tests/unit/CoreTests.cpp` no utiliza SQLite pero enlaza a una librería que lo incluye, ocultando potenciales fugas de abstracción.
- **Falta de visibilidad explícita**: no se utiliza `target_compile_features` para propagar el estándar C++20 de forma idiomática a través del grafo de targets de CMake.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar: C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`).
  - Compiladores soportados: GCC 12+, Clang 15+, MSVC 2022 (v143+).
  - CMake: versión mínima 3.24.
  - Generador recomendado: Ninja para compilaciones reproducibles y concurrentes.
  - Plataformas objetivo: Linux x64, Windows 10/11 x64, macOS x64/arm64.
- **Migración**:
  - La introducción del target `vigilio_storage` se realiza de forma totalmente retrocompatible: `vigilio_storage` depende de `vigilio_core`, y cualquier consumidor de almacenamiento simplemente declara la dependencia del target correspondiente.
  - Los nombres de los ejecutables de pruebas y presets en `CMakePresets.json` se preservan intactos, asegurando compatibilidad con scripts existentes como `apps/vigilio-browser/scripts/check.sh`.
- **Rollback**:
  - La reversión en la fase APPLY se realiza mediante restauración directa del archivo `apps/vigilio-browser/CMakeLists.txt` a su versión previa al cambio.
  - En esta fase PROPOSE, la reversión consiste únicamente en descartar los archivos en `openspec/changes/module-foundations-task0002/`.

## Diseño completo de la capacidad: cmake targets (etapa implementation)

### 1. Jerarquía y topología modular de targets
La estructura de construcción implementada en CMake define un grafo acíclico dirigido (DAG) limpio y predecible:

```
[ SQLite::SQLite3 ]
       ^
       | (PRIVATE/PUBLIC)
[ vigilio_storage ] ---- (PUBLIC) ----> [ vigilio_core ] <==== (PUBLIC cxx_std_20)
       ^                                       ^
       |                                       |
       +---------------+-----------------------+
                       |
        +--------------+---------------+
        |                              |
[ vigilio-browser ]          [ vigilio_storage_tests ]    [ vigilio_core_tests ]
(Qt6 Widgets/WebEngine)      (Integration tests)          (Unit tests)
```

1. **Target `vigilio_core` (STATIC Library)**:
   - *Fuentes*: `src/core/UrlInput.cpp`, `src/core/TabModel.cpp`, `src/core/SessionFile.cpp`.
   - *Cabeceras*: `src/core/UrlInput.h`, `src/core/TabModel.h`, `src/core/SessionFile.h`.
   - *Include directories*: `PUBLIC src`.
   - *Compile features*: `PUBLIC cxx_std_20`.
   - *Dependencias*: Ninguna biblioteca externa; exclusivamente cabeceras estándar de C++20 (`<string>`, `<vector>`, `<filesystem>`, `<optional>`, `<cstddef>`).
   - *Invariante*: Prohibido incluir SQLite, Qt Widgets o Qt WebEngine.

2. **Target `vigilio_storage` (STATIC Library)**:
   - *Fuentes*: `src/storage/LocalRepository.cpp`.
   - *Cabeceras*: `src/storage/LocalRepository.h`.
   - *Include directories*: `PUBLIC src`.
   - *Compile features*: `PUBLIC cxx_std_20`.
   - *Dependencias*: Enlace `PUBLIC SQLite::SQLite3` y `PUBLIC vigilio_core`.
   - *Invariante*: Manejo exclusivo de persistencia SQLite local; sin dependencias de UI.

3. **Target `vigilio-browser` (EXECUTABLE)**:
   - *Fuentes*: `src/app.cpp`, `src/ui/BrowserWindow.cpp`, `resources.qrc`.
   - *Cabeceras*: `src/ui/BrowserWindow.h`.
   - *Include directories*: `PRIVATE src`.
   - *Dependencias*: `PRIVATE vigilio_core vigilio_storage Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets`.
   - *Condición de activación*: `VIGILIO_BUILD_GUI=ON` y `Qt6_FOUND`.
   - *Propiedades*: `WIN32_EXECUTABLE TRUE`, `MACOSX_BUNDLE TRUE`.
   - *Instalación*: `install(TARGETS vigilio-browser BUNDLE DESTINATION . RUNTIME DESTINATION bin)`.

4. **Target `vigilio_core_tests` (EXECUTABLE)**:
   - *Fuentes*: `tests/unit/CoreTests.cpp`.
   - *Dependencias*: `PRIVATE vigilio_core`.
   - *Registro CTest*: `add_test(NAME core_units COMMAND vigilio_core_tests)`.
   - *Propiedades CTest*: `LABELS "unit;fast" TIMEOUT 15`.

5. **Target `vigilio_storage_tests` (EXECUTABLE)**:
   - *Fuentes*: `tests/integration/StorageTests.cpp`.
   - *Dependencias*: `PRIVATE vigilio_storage`.
   - *Registro CTest*: `add_test(NAME storage_integration COMMAND vigilio_storage_tests)`.
   - *Propiedades CTest*: `LABELS "integration;fast" TIMEOUT 15`.

6. **Target `vigilio_e2e_smoke` (EXECUTABLE)**:
   - *Fuentes*: `tests/e2e/BrowserSmoke.cpp`, `src/ui/BrowserWindow.cpp`, `resources.qrc`.
   - *Dependencias*: `PRIVATE vigilio_core vigilio_storage Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test`.
   - *Condición de activación*: `BUILD_TESTING AND VIGILIO_ENABLE_E2E AND Qt6_FOUND`.
   - *Registro CTest*: `add_test(NAME browser_e2e_smoke COMMAND vigilio_e2e_smoke)`.
   - *Propiedades CTest*: `LABELS "e2e;heavy" TIMEOUT 60 RESOURCE_LOCK browser_e2e RUN_SERIAL TRUE`.

### 2. Opciones de configuración y variables de compilador
- `CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`.
- Opciones de compilación aplicadas con ámbito `PRIVATE`:
  - En GCC/Clang: `-Wall -Wextra -Wpedantic -Wconversion`.
  - En MSVC: `/W4 /permissive-`.
- Opciones booleanas de control:
  - `VIGILIO_BUILD_GUI`: Permite desactivar la interfaz gráfica (`OFF` por defecto en entornos headless/CI mínimos).
  - `BUILD_TESTING`: Controla la generación de ejecutables de pruebas unitarias e integración.
  - `VIGILIO_ENABLE_E2E`: Desactivado por defecto (`OFF`) para evitar sobrecarga de memoria RAM en entornos concurrentes.

## Archivos afectados

### Archivos existentes inspeccionados y validados
- `apps/vigilio-browser/CMakeLists.txt`: Archivo canónico que contendrá la implementación de targets.
- `apps/vigilio-browser/CMakePresets.json`: Define presets de compilación y pruebas compatibles.
- `apps/vigilio-browser/scripts/check.sh`: Script de verificación rápida en entorno local.
- `apps/vigilio-browser/src/app.cpp`: Punto de entrada de la aplicación GUI.
- `apps/vigilio-browser/src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`: Ventana principal de escritorio.
- `apps/vigilio-browser/src/core/UrlInput.h` y `src/core/UrlInput.cpp`: Validador y procesador de URLs de navegación.
- `apps/vigilio-browser/src/core/TabModel.h` y `src/core/TabModel.cpp`: Modelo de datos y gestión de pestañas.
- `apps/vigilio-browser/src/core/SessionFile.h` y `src/core/SessionFile.cpp`: Serializador atómico de sesión en archivo local.
- `apps/vigilio-browser/src/storage/LocalRepository.h` y `src/storage/LocalRepository.cpp`: Adaptador de persistencia SQLite.
- `apps/vigilio-browser/tests/unit/CoreTests.cpp`: Pruebas unitarias de dominio sin dependencias gráficas.
- `apps/vigilio-browser/tests/integration/StorageTests.cpp`: Pruebas de integración de base de datos SQLite.
- `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp`: Prueba de humo de interfaz gráfica.

### Archivos a modificar en la fase APPLY
- `apps/vigilio-browser/CMakeLists.txt`: Actualización con la nueva topología de targets.
- `docs/rules/01-modules/foundations/micro/0002-cmake-targets/task.md`: Actualización del estado de ejecución.

### Archivos generados en esta fase PROPOSE
- `openspec/changes/module-foundations-task0002/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0002/proposal.md`
- `openspec/changes/module-foundations-task0002/design.md`
- `openspec/changes/module-foundations-task0002/tasks.md`
- `openspec/changes/module-foundations-task0002/execution-memory.md`

## Contrato funcional

- **Actor**: Desarrollador de software / Entorno de Integración Continua (CI).
- **Invocación**:
  ```bash
  cmake -S apps/vigilio-browser -B build -G Ninja -DVIGILIO_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
  cmake --build build
  ctest --test-dir build --output-on-failure -L fast
  ```
- **Entradas**:
  - Directorio fuente de CMake con `CMakeLists.txt` válido.
  - Parámetros de configuración (`CMAKE_BUILD_TYPE`, `VIGILIO_BUILD_GUI`, `BUILD_TESTING`, `VIGILIO_ENABLE_E2E`).
- **Salidas**:
  - `libvigilio_core.a`: Biblioteca estática de lógica de negocio en C++20.
  - `libvigilio_storage.a`: Biblioteca estática de persistencia SQLite.
  - `vigilio-browser`: Binario ejecutable de interfaz gráfica (cuando `VIGILIO_BUILD_GUI=ON` y Qt6 está presente).
  - `vigilio_core_tests` y `vigilio_storage_tests`: Binarios de prueba ejecutables por CTest.
- **Precondiciones**:
  - Compilador con soporte C++20 instalado (GCC 12+, Clang 15+, MSVC 2022).
  - CMake 3.24 o superior.
  - Biblioteca SQLite3 y sus cabeceras disponibles en el sistema.
- **Postcondiciones**:
  - Binarios generados sin mezclar dependencias no declaradas.
  - Pruebas rápidas ejecutables en menos de 15 segundos sin requerir display gráfico.
- **Manejo de errores y recuperación**:
  - Si falta `SQLite3`: CMake detiene la configuración con error informativo claro indicando que el paquete requerido no fue hallado.
  - Si falta `Qt6` y `VIGILIO_BUILD_GUI=ON`: CMake emite un mensaje de advertencia (`WARNING`) y omite limpiamente el ejecutable de escritorio, permitiendo compilar el núcleo y ejecutar pruebas unitarias.

## Contrato UX/UI

- **Declaración obligatoria**: **Sin cambio visual directo**.
- **Justificación**:
  - La capacidad `cmake targets` es una capacidad de infraestructura de compilación y modularización en CMake dentro de la capa `foundations`. No implementa ni modifica pantallas, menús, widgets, diálogos ni controles visibles por el usuario final.
- **Alineación con Senior Mode y Copia Pública**:
  - De acuerdo con `docs/UX-COPY-PUBLICO.md`: ningún término de ingeniería interna («cmake», «target», «prompt», «skill», «DAG») es visible ni expuesto al usuario.
  - Para cualquier componente visual futuro, se respetan las directrices de «Interfaz más fácil de usar» (Modo Senior), áreas de interacción amplias (≥44px), contraste WCAG 2.2 AA y soporte completo de navegación por teclado, de conformidad con `docs/skills/04-ux-ui-senior.md` y `05-accesibilidad-y-senior-mode.md`.

## Reglas y fallos

- **Reglas de negocio e invariantes**:
  1. *Aislamiento local-first*: Ningún target en CMake descarga binarios remotos no verificados ni incluye librerías de telemetría en la nube.
  2. *Independencia de UI*: El código del target `vigilio_core` no debe incluir cabeceras de `QtWidgets` ni `QtWebEngineWidgets`.
  3. *Seguridad en perfiles de prueba*: Las pruebas de integración y unitarias utilizan rutas de archivos temporales efímeras (`std::filesystem::temp_directory_path()`) con limpieza automática tras la ejecución, sin tocar perfiles de usuario reales.
- **Condiciones de fallo y tratamiento**:
  - Fallo de compilación por tipo inseguro: Forzado preventivamente por las banderas `-Wconversion -Wpedantic` para detectar errores en tiempo de compilación.
  - Ausencia de display en pruebas: Las pruebas de CTest con etiqueta `fast` no deben invocar subprocesos de Chromium ni exigir variables de entorno `DISPLAY` o `WAYLAND_DISPLAY`.

## Seguridad y privacidad

- **Límites de confianza y aislamiento**:
  - El núcleo de dominio no tiene acceso directo a interfaces del sistema operativo no controladas.
  - La persistencia se realiza exclusivamente en bases de datos SQLite locales y archivos de texto de sesión planos, sin credenciales ni tokens en texto claro.
  - Prohibido el uso de telemetría remota, analítica en línea o sincronización cloud sin intervención explícita del usuario (respetando Q-001 y las reglas de `docs/rules/00-global/security.md`).
- **Gestión de memoria y contención**:
  - En cumplimiento de `docs/skills/06-rendimiento-y-ram.md`, las pruebas pesadas E2E (`vigilio_e2e_smoke`) se aíslan con `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE` para evitar agotar la memoria RAM del sistema de construcción por ejecuciones paralelas no coordinadas.

## Matriz de pruebas trazables

| Criterio | Nivel de prueba | Target CMake | Fixture / Comando | Resultado esperado | Caso negativo / Restricción | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Modularización de targets core y storage | Build / Configuración | `vigilio_core`, `vigilio_storage` | `cmake --preset core-debug` | Configuración exitosa sin errores de dependencias cíclicas | Error si `vigilio_core` intenta enlazar Qt | APPLY |
| Compilación de biblioteca core pura | Build C++20 | `vigilio_core` | `cmake --build --preset core-debug --target vigilio_core` | Generación de archivo estático `libvigilio_core.a` | Falla si se usan características no compatibles con C++20 | APPLY |
| Compilación de biblioteca storage con SQLite | Build C++20 | `vigilio_storage` | `cmake --build --preset core-debug --target vigilio_storage` | Generación de archivo estático `libvigilio_storage.a` | Falla si falta `sqlite3.h` o libsqlite3 | APPLY |
| Pruebas unitarias de dominio sin dependencias de base de datos | Unit | `vigilio_core_tests` | `ctest --preset core-debug -R core_units` | Exit code 0, etiqueta `unit;fast`, tiempo < 1s | Falla si `resolveInput` o `TabModel` violan contratos | APPLY |
| Pruebas de integración de base de datos SQLite | Integration | `vigilio_storage_tests` | `ctest --preset core-debug -R storage_integration` | Exit code 0, etiqueta `integration;fast`, operaciones SQLite verificadas | Falla si falla apertura de DB o consultas atómicas | APPLY |
| Compilación condicional de interfaz gráfica | Build GUI | `vigilio-browser` | `cmake -S apps/vigilio-browser -B build-gui -DVIGILIO_BUILD_GUI=ON` | Generación del ejecutable `vigilio-browser` si Qt6 está presente | Si Qt6 falta, mensaje WARNING y omisión suave | LATER (task0003) |
| Ejecución serial de pruebas de humo E2E de navegador | E2E Browser | `vigilio_e2e_smoke` | `ctest -R browser_e2e_smoke` con `VIGILIO_ENABLE_E2E=ON` | Exit code 0, `RESOURCE_LOCK browser_e2e`, ejecución serial estricta | Falla o timeout de 60s si Chromium cuelga | LATER (task0004) |
| Verificación de consistencia de formato ClangFormat | Linter / Calidad | Código fuente C++ modificado | `clang-format --dry-run --Werror src/...` | Sin discrepancias de estilo detectadas | Falla si el código no sigue las reglas de formateo | APPLY |
