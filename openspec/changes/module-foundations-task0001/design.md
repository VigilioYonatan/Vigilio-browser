# Design — module-foundations-task0001

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- `apps/vigilio-browser/CMakeLists.txt` contiene un target central `vigilio_core` como librería estática (`STATIC`), que compila conjuntamente código de dominio (`UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`) y almacenamiento (`LocalRepository.cpp`).
- Enlaza incondicionalmente a `SQLite::SQLite3` mediante `find_package(SQLite3 REQUIRED)`.
- Define opciones booleanas: `BUILD_TESTING` (vía `include(CTest)`), `VIGILIO_ENABLE_E2E` (por defecto `OFF`) y `VIGILIO_BUILD_GUI` (por defecto `ON`).
- Configura pruebas en CTest: `vigilio_core_tests` (etiquetas `unit;fast`, timeout 15s) y `vigilio_storage_tests` (etiquetas `integration;fast`, timeout 15s).
- Define condicionalmente el ejecutable de interfaz `vigilio-browser` cuando `Qt6 6.10` está presente con componentes `Core`, `Widgets` y `WebEngineWidgets`.
- Define condicionalmente `vigilio_e2e_smoke` cuando `BUILD_TESTING` y `VIGILIO_ENABLE_E2E` están activos, con `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE`.

### Estado objetivo (TARGET)
- Arquitectura modular limpia donde cada subsistema tiene responsabilidades delimitadas (SOLID, SRP):
  1. **Core Domain Target** (`vigilio_core`): Modelo de dominio puro en C++20, cálculo de estados de navegación, sanitización de URLs y manejo de sesiones. Cero dependencias de Widgets, X11 o WebEngine.
  2. **Storage Target** (`vigilio_storage` / sub-biblioteca de persistencia): Encapsulación de SQLite3 y operaciones atómicas de disco.
  3. **Application Target** (`vigilio-browser`): Punto de composición (`app.cpp`), interfaz gráfica Qt6 Widgets y orquestación de perfiles WebEngine.
  4. **Test Targets**: Jerarquía de ejecutables independientes clasificados por nivel de pirámide de pruebas (`unit`, `integration`, `e2e`) ejecutables sin interfaz gráfica obligatoria en los niveles unitario e integración.
- Opciones de compilación portables estandarizadas en C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`).
- Banderas de diagnóstico exigentes para prevenir degradación de código: `-Wall -Wextra -Wpedantic -Wconversion` (GCC/Clang) y `/W4 /permissive-` (MSVC).

### Brechas identificadas (GAPS)
- Ausencia de formalización de reglas que impidan acoplamientos indeseados entre el núcleo y bibliotecas gráficas en futuros cambios.
- Necesidad de asegurar que la falta de paquetes de desarrollo (como `libsqlite3-dev` o `qt6-base-dev`) en entornos mínimos o de análisis no produzca falsos resultados o bloqueos descontrolados, documentando claramente el estado `NOT_EXECUTED`.
- Falta de especificación canónica sobre la evolución de targets a medida que se incorporen módulos de `shields`, `memory`, `mcp` y `command-palette`.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar C++20 soportado por GCC 12+, Clang 15+ y MSVC 2022 (v143+).
  - CMake 3.24+ con presets versión 6 compatibles con herramientas modernas e IDEs.
  - Generador preferido: Ninja para compilación determinista y paralela.
  - Multiplataforma: compatible con Linux x64, Windows 10/11 x64 y macOS x64/arm64.
- **Migración**:
  - Transición transparente: se preservan los nombres de los targets actuales (`vigilio_core`, `vigilio-browser`, `vigilio_core_tests`) para garantizar compatibilidad con scripts existentes (`apps/vigilio-browser/scripts/check.sh`).
- **Rollback**:
  - Como esta fase es PROPOSE, no se han alterado archivos de código fuente. Cualquier reversión consiste simplemente en descartar o modificar la propuesta documental en `openspec/changes/module-foundations-task0001`.

## Diseño completo de la capacidad: cmake targets

### 1. Entidades y topología del modelo de compilación
El dominio de `cmake targets` dentro de `foundations` modela la estructura de construcción del software mediante las siguientes entidades lógicas:
- **Core Library (`vigilio_core`)**:
  - *Responsabilidad*: Entidades de negocio, validadores de URL (`UrlInput`), modelo de estado de pestañas (`TabModel`), serialización de sesión (`SessionFile`).
  - *Invariantes*:
    - Depende únicamente de la biblioteca estándar C++20 y de tipos de almacenamiento local.
    - PROHIBIDO incluir cabeceras de `Qt6Widgets`, `Qt6WebEngineWidgets` o Chromium.
    - Se compila en modo estático para enlace directo y optimización LTO.
- **Desktop Executable (`vigilio-browser`)**:
  - *Responsabilidad*: Ventana principal (`BrowserWindow`), inicialización de la aplicación (`app.cpp`), inclusión de recursos (`resources.qrc`).
  - *Invariantes*:
    - Solo se configura cuando `VIGILIO_BUILD_GUI=ON` y se resuelven los paquetes Qt6 requeridos.
    - No asume que el proceso WebEngine comparte el mismo espacio de memoria; respeta el modelo multiproceso de Chromium.
- **Suite de Pruebas (`*_tests`, `*_smoke`)**:
  - *Responsabilidad*: Verificación reproducible y determinista del comportamiento en cada capa.
  - *Invariantes*:
    - `vigilio_core_tests` solo enlaza `vigilio_core` y no requiere display gráfico (`DISPLAY`/Wayland).
    - `vigilio_storage_tests` verifica persistencia local en SQLite sin servicios remotos.
    - `vigilio_e2e_smoke` está desactivado por defecto (`VIGILIO_ENABLE_E2E=OFF`) y cuando se activa utiliza `RESOURCE_LOCK browser_e2e` para garantizar ejecución estrictamente serial y contención de RAM.

### 2. Invariantes de compilación y límites arquitectónicos
1. **Local-First & Zero-Cloud**: Ningún target CMake descargará dependencias en tiempo de compilación mediante `FetchContent` externo no controlado, ni enlaza endpoints de red externos.
2. **Independencia Headless**: La configuración con `VIGILIO_BUILD_GUI=OFF` debe ser siempre válida y permitir compilar y ejecutar pruebas unitarias en servidores sin entorno gráfico.
3. **Manejo de Advertencias**: Las opciones privadas de compilador deben forzar limpieza estricta de tipos (`-Wconversion`) para evitar desbordamientos o conversiones implícitas inseguras.

## Archivos afectados

### Archivos existentes inspeccionados y validados
- `apps/vigilio-browser/CMakeLists.txt`: Archivo canónico de definición de targets.
- `apps/vigilio-browser/CMakePresets.json`: Configuración de presets para desarrolladores y CI.
- `apps/vigilio-browser/scripts/check.sh`: Automatización de verificación local.
- `apps/vigilio-browser/src/app.cpp`: Entrada de la aplicación gráfica.
- `apps/vigilio-browser/src/core/TabModel.h` y `TabModel.cpp`: Modelo de pestañas.
- `apps/vigilio-browser/src/core/UrlInput.h` y `UrlInput.cpp`: Lógica de análisis de URLs.
- `apps/vigilio-browser/src/core/SessionFile.h` y `SessionFile.cpp`: Manejo de archivo de sesión.
- `apps/vigilio-browser/src/storage/LocalRepository.h` y `LocalRepository.cpp`: Acceso a SQLite.
- `apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`: Ventana Qt.
- `apps/vigilio-browser/tests/unit/CoreTests.cpp`: Pruebas unitarias de dominio.
- `apps/vigilio-browser/tests/integration/StorageTests.cpp`: Pruebas de integración de base de datos.
- `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp`: Prueba E2E básica de la interfaz.

### Archivos de propuesta OpenSpec (modificados en PROPOSE)
- `openspec/changes/module-foundations-task0001/proposal.md`
- `openspec/changes/module-foundations-task0001/design.md`
- `openspec/changes/module-foundations-task0001/tasks.md`
- `openspec/changes/module-foundations-task0001/execution-memory.md`

## Contrato funcional

- **Actor**: Desarrollador / Pipeline de Integración Continua / Sistema de compilación.
- **Invocación**:
  ```bash
  cmake -S apps/vigilio-browser -B build [opciones de configuración]
  cmake --build build [--target <nombre_target>]
  ctest --test-dir build [--output-on-failure] [-L <filtro_labels>]
  ```
- **Entradas**:
  - `CMAKE_BUILD_TYPE`: `Debug`, `Release`, `RelWithDebInfo`.
  - `VIGILIO_BUILD_GUI`: Booleano (`ON`/`OFF`), controla la inclusión del ejecutable Qt6.
  - `BUILD_TESTING`: Booleano (`ON`/`OFF`), controla la generación de targets de prueba.
  - `VIGILIO_ENABLE_E2E`: Booleano (`ON`/`OFF`), controla pruebas pesadas de navegador.
- **Salidas**:
  - Target `vigilio_core`: biblioteca estática portable (`libvigilio_core.a` / `vigilio_core.lib`).
  - Target `vigilio-browser`: ejecutable nativo de escritorio (`vigilio-browser` / `vigilio-browser.exe`).
  - Targets de pruebas: binarios ejecutables con salida de CTest formateada.
- **Precondiciones**: Compilador C++20 instalado, CMake versión 3.24 o superior.
- **Postcondiciones**: Binarios generados sin mezclar dependencias no declaradas; pruebas aisladas con timeouts acotados.
- **Errores claros y recuperación**:
  - Ausencia de SQLite3: CMake detiene la configuración con mensaje de diagnóstico indicando la dependencia faltante.
  - Ausencia de Qt6 cuando `VIGILIO_BUILD_GUI=ON`: emite una advertencia informativa (`WARNING`) y omite el target gráfico sin interrumpir la compilación de `vigilio_core` ni sus pruebas unitarias.

## Contrato UX/UI

- **Declaración obligatoria**: **Sin cambio visual directo**.
- **Justificación**: La capacidad `cmake targets` pertenece íntegramente al dominio de infraestructura de construcción y modelo de targets de CMake en `foundations`. No modifica pantallas, widgets, paletas, tipografías ni menús del navegador.
- **Alineación con Senior Mode y Copia Pública**:
  - Se cumple estrictamente `docs/UX-COPY-PUBLICO.md`: ninguna etiqueta interna («cmake», «target», «prompt», «skill», «DAG») es expuesta al usuario.
  - Para la interfaz pública, cuando aplique, se mantiene el compromiso con la «Interfaz más fácil de usar» (Modo Senior), áreas de clic amplias (≥44px) y contraste WCAG 2.2 AA según lo detallado en `docs/skills/04-ux-ui-senior.md` y `05-accesibilidad-y-senior-mode.md`.

## Reglas y fallos

| Escenario de fallo | Causa | Detección | Comportamiento / Recuperación |
| --- | --- | --- | --- |
| Compilador antiguo (< C++20) | Falta de soporte para características de C++20 | CMake configure (`CMAKE_CXX_STANDARD_REQUIRED ON`) | Error fatal inmediato con mensaje explicativo; no compila con estándares obsoletos. |
| Dependencia SQLite3 no encontrada | Ausencia del paquete de desarrollo en el sistema | `find_package(SQLite3 REQUIRED)` | Detención limpia en fase de configuración; requiere instalación de SQLite3 o provisión de ruta vía `CMAKE_PREFIX_PATH`. |
| Paquetes Qt6 ausentes | Entorno headless o CI básico sin dependencias GUI | `find_package(Qt6 ... QUIET)` | Mensaje `WARNING`: se deshabilita la construcción del navegador gráfico y se permite continuar con `vigilio_core` y pruebas unitarias. |
| Consumo excesivo de RAM en pruebas E2E | Múltiples instancias de WebEngine concurrentes | Ejecución de CTest sin límites | Mitigado por diseño mediante propiedad `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE` con timeout estricto de 60 segundos. |

## Seguridad y privacidad

- **Aislamiento de procesos**: La arquitectura de targets respeta que el renderizado web corre bajo procesos secundarios de Chromium aislados (sandboxed), sin otorgar privilegios innecesarios a nivel de enlace de CMake.
- **Sin telemetría ni red en compilación**: Ningún target añade flags de enlace a servicios remotos, SDKs de analítica o servidores de Vigilio. La aplicación es estrictamente local-first (`docs/rules/00-global/security.md` y `docs/skills/07-seguridad-y-privacidad.md`).
- **Sanitización de almacenamiento**: Las pruebas de integración que ejecutan operaciones SQLite operan sobre bases de datos locales temporales deterministas, previniendo fuga de información o colisiones de sesión.

## Matriz de pruebas

| Tipo de prueba | Target / Fixture | Comando de ejecución | Resultado esperado | Manejo de fallo / Estado |
| --- | --- | --- | --- | --- |
| **Configuración Headless** | Preset `core-debug` | `cmake --preset core-debug` | Configuración exitosa sin requerir Qt6 Widgets | PASS si las dependencias mínimas están presentes; si falta SQLite3 se reporta dependencia. |
| **Compilación Core** | Target `vigilio_core` | `cmake --build --preset core-debug --target vigilio_core` | Generación de biblioteca estática sin errores ni advertencias | Compilación limpia bajo `-Wall -Wextra -Wpedantic -Wconversion`. |
| **Pruebas Unitarias** | Target `vigilio_core_tests` | `ctest --preset core-debug -L fast` | 100% de tests de `UrlInput`, `TabModel` y `SessionFile` superados (< 15s) | PASS en entorno estándar; fallo negativo si falla aserción de negocio. |
| **Pruebas de Almacenamiento** | Target `vigilio_storage_tests` | `ctest --preset core-debug -R storage_integration` | Operaciones CRUD y transacciones sobre SQLite verificadas | Integración local validada; no requiere red externa. |
| **Compilación Desktop GUI** | Target `vigilio-browser` | `cmake --build --preset desktop-debug --target vigilio-browser` | Binario ejecutable vinculado a Qt6 Core/Widgets/WebEngineWidgets | Si Qt6 no está disponible, registra `NOT_EXECUTED` (nunca PASS ficticio). |
| **Prueba E2E Smoke** | Target `vigilio_e2e_smoke` | `ctest -R browser_e2e_smoke` con `VIGILIO_ENABLE_E2E=ON` | Ventana principal abre y carga URL local en serie (< 60s) | Ejecución serial garantizada por `RESOURCE_LOCK browser_e2e`. LATER (task0004). |
