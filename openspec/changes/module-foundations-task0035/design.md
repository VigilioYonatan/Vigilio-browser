# Design — module-foundations-task0035

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Catálogo de perfiles y validación de CMake Presets (`CMakePresets.json`, `CMakePresetsValidator`)**:
   - `apps/vigilio-browser/CMakePresets.json` formaliza 9 perfiles canónicos en versión de esquema 6:
     - Perfiles de interfaz gráfica: `desktop-debug`, `desktop-release`, `desktop-e2e`.
     - Perfiles headless: `core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`.
     - Perfiles de diagnóstico y sanitizers: `sanitizer-asan`, `sanitizer-ubsan`.
   - `apps/vigilio-browser/src/core/CMakePresetsValidator.h` y `.cpp` implementan la validación estática de esquemas v6, verificación de campos obligatorios, resolución de referencias huérfanas y reglas de higiene de rutas (`isSafeBinaryDir`).
2. **Capa operativa de servicio en C++20 (`CMakePresetsService`)**:
   - `apps/vigilio-browser/src/core/CMakePresetsService.h` y `.cpp` proporcionan una capa operativa en `vigilio_core` desacoplada de `QApplication` y `QWidget`:
     - Descubrimiento e indexación de presets de configuración, construcción y pruebas.
     - Síntesis determinista de líneas de comando CLI (`cmake --preset`, `cmake --build --preset`, `ctest --preset`).
     - Ejecución acotada con soporte para modo simulación (dry-run), timeouts finitos (60s), búferes acotados a 64 KB (política Zero-Zombie) y cancelación cooperativa multihilo vía `CancellationToken`.
     - Operaciones de recuperación y saneamiento del árbol de compilación (`cleanBinaryDir`).
3. **Capa de interfaz de usuario (`BrowserWindow`, `resources.qrc`)**:
   - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp` implementan la ventana gráfica principal basada en Qt6 Widgets con:
     - Pestañas con carga diferida (`activateDeferredTab`) para ahorro de RAM.
     - Barra de navegación con iconos Lucide SVG compilados en `resources.qrc`.
     - Soporte para «Interfaz más fácil de usar» (Modo Senior con fuentes de 14pt/18pt y tap targets mínimos $\ge 48$ px).
     - Banners nativos accesibles de Modo Seguro (`safeModeBanner_`) y Recuperación ante caídas (`crashRecoveryBanner_`).
     - Insignia y aislamiento estricto de sesiones de incógnito (`incognitoBadge_`).
     - Apagado ordenado mediante `ApplicationLifetimeCoordinator` (`quitAction_` / `closeEvent`).
4. **Sistema de construcción y pruebas de UI (`CMakeLists.txt`, `WindowTests.cpp`)**:
   - `apps/vigilio-browser/CMakeLists.txt` define la separación limpia entre `vigilio_core`, `vigilio_storage` y la sección condicional `if(VIGILIO_BUILD_GUI)`.
   - Si Qt6 está presente, define `vigilio-browser` y la suite de pruebas Qt Test `vigilio_ui_tests` (`tests/ui/WindowTests.cpp`).
   - `WindowTests.cpp` incluye detección de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`), reportando `NOT_EXECUTED` si falta display.

### Estado objetivo (TARGET)

1. **Gobernanza completa de perfiles de construcción de UI en CMake Presets**:
   - Vinculación explícita y coherente entre los presets de escritorio (`desktop-debug`, `desktop-release`, `desktop-e2e`) y la compilación de los targets visuales (`vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`).
   - Preservación estricta de la compatibilidad headless en los presets de núcleo (`core-debug`, `core-release`, etc.) con `VIGILIO_BUILD_GUI=OFF`, permitiendo compilar y probar todo el dominio sin paquetes de desarrollo gráfico de Qt6.
2. **Declaración contractual de UX/UI y «sin cambio visual directo»**:
   - Declaración formal vinculante de que CMake Presets es una capacidad de infraestructura técnica de construcción y desarrollo; por lo tanto, no introduce widgets, menús, barras de herramientas ni diálogos en la interfaz pública del navegador.
   - Cumplimiento incondicional de `docs/UX-COPY-PUBLICO.md`: proscripción total de terminología de ingeniería interna («CMake», «presets», «toolchain», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec») en todas las superficies visibles para el usuario común.
3. **Desacoplamiento arquitectónico bidireccional**:
   - `CMakePresetsService` permanece en `src/core`, sin incluir cabeceras de `QWidget` ni añadir peso en memoria a la interfaz de usuario.
   - `BrowserWindow` permanece en `src/ui`, enfocado exclusivamente en la experiencia pública del usuario y sin asumir tareas de compilación o ejecución de procesos externos.
4. **Verificación de contratos de presentación y accesibilidad en ejecutables gobernados por presets**:
   - Garantía de que los binarios generados bajo los presets de interfaz satisfagan plenamente los contratos visuales:
     - Coherencia con `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
     - Ciclo de vida y estados completos de widgets (`empty`, `loading`, `error`, `idle`, `hover`, `focus`, `disabled`).
     - Escalado dinámico en «Interfaz más fácil de usar» (Modo Senior): iconos 22x22 a 32x32 px, botones y campos $\ge 48$ px, barra de herramientas $\ge 55$ px.
     - Metadatos accesibles completos (`accessibleName`, `accessibleDescription`) para tecnologías asistivas.
     - Navegación por teclado y atajos canónicos (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+B`, `Ctrl+Q`, `Alt+Home`, `Alt+F`).

### Brechas identificadas (GAPS)

1. **Formalización de la etapa UI para CMake Presets**:
   - Es necesario formalizar en OpenSpec la relación entre la infraestructura de presets y los artefactos de interfaz gráfica, documentando taxativamente por qué no hay cambio visual directo en la UI pública conforme a `docs/UX-COPY-PUBLICO.md`.
2. **Consolidación de la matriz de pruebas de interfaz frente a perfiles de build**:
   - Documentar la trazabilidad entre los presets de CMake y la ejecución de la suite de pruebas de interacción de UI (`WindowTests.cpp`), asegurando la tolerancia a entornos headless de integración continua.

---

## Compatibility, migration and rollback

### Compatibilidad
- **Lenguaje**: C++20 estándar (`set(CMAKE_CXX_STANDARD 20)`).
- **Framework de interfaz**: Qt 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
- **Sistema de construcción**: CMake 3.24+ y generador Ninja.
- **Sistemas operativos**: Linux x64 (X11 / Wayland), Windows 10/11 x64, macOS 12+ (x64 / arm64).
- **Modo headless**: Compatibilidad 100% garantizada con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`), sin requerir librerías de interfaz gráfica.

### Migración
- No requiere migraciones de datos ni modificaciones de esquemas en SQLite.
- La gobernanza de presets preserva total compatibilidad retroactiva con los comandos y scripts existentes (`apps/vigilio-browser/scripts/check.sh`).

### Rollback
- En la fase APPLY, la reversión se efectuaría restaurando la versión previa de los archivos bajo `apps/vigilio-browser/` autorizados.
- En esta fase PROPOSE, la reversión consiste únicamente en descartar los artefactos bajo `openspec/changes/module-foundations-task0035/`.

---

## Arquitectura y topología de CMake Presets y la Interfaz Gráfica

```
+-------------------------------------------------------------------------------+
|                             apps/vigilio-browser                              |
+-------------------------------------------------------------------------------+
                                        |
                   +--------------------+--------------------+
                   |                                         |
     [ apps/vigilio-browser/src/core ]         [ apps/vigilio-browser/src/ui ]
     - CMakePresetsTypes.h                     - BrowserWindow.h
     - CMakePresetsValidator.h / .cpp          - BrowserWindow.cpp
     - CMakePresetsService.h / .cpp            - resources.qrc
     (Núcleo desacoplado de QWidget)           (Ventana pública de navegación)
                   |                                         |
                   | (LINK PRIVATE)                          | (Compila en vigilio-browser
                   v                                         |  y vigilio_ui_tests)
     +-------------------------------+                       v
     |         vigilio_core          |<----------------------+
     +-------------------------------+
                   ^
                   | (Gobernado por)
     +-------------------------------+
     |       CMakePresets.json       |
     | - desktop-debug (GUI=ON)      |
     | - desktop-release (GUI=ON)    |
     | - desktop-e2e (GUI=ON,E2E=ON) |
     | - core-debug (GUI=OFF)        |
     | - core-release (GUI=OFF)      |
     | - ci-linux-* (GUI=OFF)        |
     +-------------------------------+
```

### 1. Desacoplamiento entre la capa de servicio y la capa visual
Para garantizar los principios de ingeniería de `docs/skills/01-principios-de-ingenieria.md` (Responsabilidad Única e Inversión de Dependencias):
- `vigilio::CMakePresetsService` no interactúa con ningún widget ni componente visual. Su salida son estructuras inmutables de datos (`CMakePresetExecutionResult`, `CMakePresetsSummary`) o cadenas de texto estándar de C++20.
- `BrowserWindow` no invoca ni almacena instancias de `CMakePresetsService`. Los usuarios del navegador navegan por la web y configuran sus preferencias de privacidad o accesibilidad sin ser expuestos a la infraestructura de compilación.

### 2. Contratos de Compilación de Presets de UI
Los presets definen de forma determinista la configuración de los targets de interfaz:

```json
{
  "name": "desktop-debug",
  "displayName": "Desktop Debug",
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/build-desktop-debug",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "VIGILIO_BUILD_GUI": "ON",
    "BUILD_TESTING": "ON",
    "VIGILIO_ENABLE_E2E": "OFF"
  }
}
```

- `VIGILIO_BUILD_GUI: "ON"`: Activa `find_package(Qt6 COMPONENTS Core Widgets WebEngineWidgets)`, incluye `src/ui/BrowserWindow.cpp` y `resources.qrc`, y configura `CMAKE_AUTOMOC` y `CMAKE_AUTORCC`.
- `BUILD_TESTING: "ON"`: Activa `find_package(Qt6 COMPONENTS Test)` y compila la suite `vigilio_ui_tests` (`tests/ui/WindowTests.cpp`).
- `VIGILIO_ENABLE_E2E: "OFF"`: Desactiva pruebas E2E pesadas con procesos Chromium en segundo plano, protegiendo el presupuesto de RAM (< 2 MB para tests unitarios).

### 3. Matriz de Estados de Interfaz en Ejecutables de Escritorio

Los ejecutables generados bajo los presets de interfaz implementan la siguiente máquina de estados visuales:

| Estado | Componente / Widget | Presentación Visual | Accesibilidad / Senior Mode |
| --- | --- | --- | --- |
| `empty` | `newHomePage()` | Logo `vigilio-logo.svg`, título sobrio, buscador centrado con foco inmediato. Cero consumo WebEngine. | Foco automático en `QLineEdit`, texto escalable a 18pt. |
| `loading` | Pestaña y `statusBar` | Texto «Cargando…» en pestaña y barra de estado. Hilo de UI 100% reactivo. | Anunciado a lectores de pantalla como progreso accesible. |
| `success` | `currentWebView()` | Renderizado web nativo, título truncado a 30 caracteres en pestaña, URL canónica en `address_`. | Navegación accesible por tabulación y atajos de teclado. |
| `error` | `createErrorPageWidget()` | Contenedor `vigilio-page-error`, icono Lucide escudo, texto «No se pudo cargar la página», botones «Reintentar» e «Ir al inicio». | Botones con tap target $\ge 48$ px, atajos de teclado y nombres accesibles. |
| `senior` | `setSeniorMode(true)` | Iconos de barra de herramientas escalan de 22×22 a 32×32 px. Altura de botones $\ge 48$ px, toolbar $\ge 55$ px, fuentes +4 pt. | Modo de visualización accesible («Interfaz más fácil de usar») conforme a `docs/UX-COPY-PUBLICO.md`. |
| `incognito` | `incognitoBadge_` | Insignia «Navegación privada» con icono de escudo. Sin banner de recuperación ni persistencia en disco (Q-001). | Metadatos accesibles descriptivos de privacidad total. |
| `safe_mode` | `safeModeBanner_` | Banner nativo superior con advertencia sobria de extensiones desactivadas y botón accesible «Entendido». | Altura $\ge 48$ px, atajo de descarte rápido con tecla `Escape`. |
