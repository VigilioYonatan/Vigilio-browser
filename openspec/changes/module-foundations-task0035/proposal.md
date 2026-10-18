# Change Proposal — module-foundations-task0035

Pack: `module.foundations.task0035`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0035 — Implementar la interacción y estados visuales en Qt: CMake presets`).

Habiendo formalizado en las microtareas precedentes la especificación del modelo de dominio, taxonomías, validación estática y catálogo canónico de 9 perfiles en `apps/vigilio-browser/CMakePresets.json` (`module.foundations.task0033`), y la implementación física de la capa operativa desacoplada en C++20 `vigilio::CMakePresetsService` con síntesis de líneas de comandos, ejecutor en modo simulación y ejecución acotada (política Zero-Zombie), cancelación cooperativa y saneamiento de directorios binarios (`module.foundations.task0034`), esta propuesta proyecta la **gobernanza de compilación, interacción, contratos visuales, tolerancia headless y verificación de interfaz de usuario en Qt Widgets para los perfiles de compilación gobernados por CMake Presets**.

El objetivo central de este pack es garantizar que la orquestación de compilación basada en presets gobierne de manera determinista, portable y reproducible la construcción y validación de los artefactos visuales de escritorio (`vigilio-browser` y `vigilio_ui_tests`), manteniendo la estricta política de lenguaje público de producto `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de construcción o gestión de procesos a los widgets de la interfaz gráfica y sin exponer terminología técnica interna al usuario final**.

El alcance concreto de este pack comprende:
- **Declaración contractual de UX/UI y «sin cambio visual directo en la interfaz pública»**:
  - Declaración formal vinculante de que la capacidad **CMake presets** es un componente de infraestructura de compilación, gestión de dependencias y orquestación de perfiles para desarrollo, integración continua y empaquetado; por consiguiente, **no introduce widgets, botones, barras de herramientas ni diálogos rotulados con términos técnicos de presets en la interfaz pública del navegador**.
  - Cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`: proscripción absoluta de exponer términos de ingeniería interna como «CMake», «presets», «toolchain», «ninja», «configurePresets», «buildPresets», «testPresets», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» en menús principales, barras de navegación o ventanas visibles al usuario común.
  - Validación de que los controles del ejecutable `vigilio-browser` (barra de direcciones, pestañas, barra de navegación, accesibilidad y menús) respeten rigurosamente las directrices de diseño de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales oficiales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Gobernanza de perfiles de compilación de interfaz de usuario en `CMakePresets.json` y `CMakeLists.txt`**:
  - Delimitación y validación de los presets específicos que configuran y construyen los artefactos visuales:
    - `desktop-debug`: perfil principal de desarrollo de escritorio con Qt6 Widgets y WebEngine (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=ON`, `CMAKE_BUILD_TYPE=Debug`), orquestando la compilación del ejecutable `vigilio-browser`, los recursos vectoriales compilados (`resources.qrc`), metaobjetos (`CMAKE_AUTOMOC`) y la suite de pruebas de interacción gráfica `vigilio_ui_tests` (`WindowTests.cpp`).
    - `desktop-release`: perfil optimizado para empaquetado y distribución final (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=OFF`, `CMAKE_BUILD_TYPE=Release`), configurando propiedades de bundle de escritorio nativo (`WIN32_EXECUTABLE TRUE` para evitar ventanas de consola en Windows, `MACOSX_BUNDLE TRUE` para macOS).
    - `desktop-e2e`: perfil para validación de humo end-to-end (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=ON`, `VIGILIO_ENABLE_E2E=ON`), orquestando la suite `vigilio_e2e_smoke` con motor WebEngine completo y ejecución serial con bloqueo de recursos.
  - Preservación incondicional de los perfiles headless (`core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`):
    - Configuración con `VIGILIO_BUILD_GUI=OFF`, garantizando que el núcleo de dominio (`vigilio_core`) y persistencia (`vigilio_storage`) compilen y ejecuten el 100% de las suites de prueba unitarias e integración en contenedores mínimos de integración continua sin requerir servidor gráfico (X11/Wayland) ni bibliotecas de desarrollo de Qt6 Widgets/WebEngine.
- **Contratos de presentación visual y accesibilidad en ejecutables generados por presets**:
  - Aseguramiento de que todo binario de escritorio generado mediante los presets de UI satisfaga íntegramente los estándares de accesibilidad y experiencia de usuario:
    - *Estados de interfaz*: soporte de estados interactivos completos (`idle`, `hover`, `focus` con anillo visible, `disabled`, `loading`, `empty`, `error`).
    - *«Interfaz más fácil de usar» (Modo Senior)*: escalado dinámico de iconos de barra de herramientas (22×22 a 32×32 px), tap targets mínimos $\ge 48$ px de altura en botones y campos de texto (`QLineEdit`), barra de navegación $\ge 55$ px, tipografía aumentada (+4 pt) y espaciado holgado sin desbordamientos de layout.
    - *Navegación universal por teclado*: atajos canónicos estandarizados (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+B`, `Ctrl+Q`, `Alt+Home`, `Alt+F`) y tabulación lógica accesible.
    - *Tecnologías asistivas*: atributos descriptivos completos `accessibleName` y `accessibleDescription` en todos los controles y acciones interactivas para compatibilidad con lectores de pantalla (Orca, NVDA, Narrator, VoiceOver).
- **Arnés de pruebas de interacción de UI (`WindowTests.cpp`) y reporte honesto de dependencias**:
  - Mantenimiento y verificación de la suite de pruebas Qt Test (`vigilio_ui_tests` / `window_ui_tests`) integrada en CTest.
  - Verificación de tolerancia a entornos sin display (`DISPLAY` / `WAYLAND_DISPLAY` ausentes), reportando honestamente `NOT_EXECUTED` en lugar de fallar o crear mocks ficticios que simulen la interfaz de usuario.
  - Cobertura de inicialización de ventana, iconografía vectorial QRC, estados visuales de home y error, conmutación accesible de interfaz fácil de usar, banners de modo seguro y recuperación ante caídas, y aislamiento de sesiones de incógnito (Q-001).
- **Desacoplamiento estricto y límites de módulo**:
  - La capa operativa `vigilio::CMakePresetsService` y el validador `vigilio::CMakePresetsValidator` residen íntegramente en `src/core`, sin dependencias ni includes de Qt Widgets (`QWidget`, `QMainWindow`), preservando la arquitectura modular en capas.
  - `BrowserWindow` y los widgets residen exclusivamente en `src/ui`, interactuando con las capacidades de dominio a través de coordinadores dedicados sin asumir responsabilidades de build o ejecución de procesos externos.

---

## Out of scope

- Implementación de paneles de depuración técnica o selectores de compiladores dentro de la interfaz pública del navegador (incompatible con la política de audiencia general de `docs/UX-COPY-PUBLICO.md`).
- Baterías de pruebas de regresión negativas de CMake Presets con simulación de corrupción de archivos de caché en disco o inyección de fallos de procesos externos (reservado para la etapa `tests` en `module.foundations.task0036`).
- Tareas de integración continua específicas para sistemas operativos individuales (cubiertas por `ci-linux-job`, `ci-windows-job`, `ci-macos-job` en microtareas 0049 a 0059).
- Modificación directa de archivos de código fuente bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida taxativamente a `openspec/changes/module-foundations-task0035/**`).
- Cualquier llamada a servicios remotos, servidores cloud o telemetría (estrictamente denegado por la política Local-First, Decisión Q-001).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia, la interfaz gráfica y el sistema de construcción:

1. **Catálogo y especificación canónica de CMake Presets (`apps/vigilio-browser/CMakePresets.json`)**:
   - Catálogo estandarizado v6 con 9 perfiles canónicos:
     - Configure presets: `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan`, `sanitizer-ubsan`.
     - Build presets: `build-core-debug`, `build-core-release`, `build-desktop-debug`, `build-desktop-release`, `build-desktop-e2e`, `build-ci-linux-debug`, `build-ci-linux-release`.
     - Test presets: `test-core-fast`, `test-desktop-full`, `test-ci-linux`, `test-desktop-e2e`.
   - Clara diferenciación entre presets que activan la interfaz gráfica (`VIGILIO_BUILD_GUI=ON`) y presets headless (`VIGILIO_BUILD_GUI=OFF`).
2. **Capa operativa de servicio implementada (`apps/vigilio-browser/src/core/`)**:
   - `CMakePresetsTypes.h`: define los tipos de dominio y operativos (`CMakePresetKind`, `CMakePresetExecutionState`, `CMakePresetExecutionStatus`, `CancellationToken`, `CMakePresetCommandOptions`, `CMakePresetExecutionResult`, `CMakePresetsSummary`).
   - `CMakePresetsValidator.h` y `.cpp`: parser JSON liviano, validación de reglas de higiene de rutas (`isSafeBinaryDir`), detección de referencias huérfanas y esquemas v6.
   - `CMakePresetsService.h` y `.cpp`: servicio operativo para carga, indexación, síntesis determinista de comandos CLI (`cmake --preset`, `cmake --build --preset`, `ctest --preset`), ejecución acotada con dry-run y límites Zero-Zombie (64 KB de búfer, 60s timeout), cancelación cooperativa y saneamiento de directorios de build.
3. **Capa de interfaz gráfica existente (`apps/vigilio-browser/src/ui/`)**:
   - `BrowserWindow.h` y `BrowserWindow.cpp`: ventana principal Qt6 Widgets con `QTabWidget`, barra de navegación `QToolBar` con iconos Lucide SVG compilados en `resources.qrc`, barra de direcciones `QLineEdit`, menús accesibles, «Interfaz más fácil de usar» con escalado dinámico (fuente 14pt/18pt, controles $\ge 48$ px, iconos 22x22 -> 32x32 px), banner de modo seguro (`safeModeBanner_`), banner de recuperación ante caídas (`crashRecoveryBanner_`), insignia de incógnito (`incognitoBadge_`), y apagado ordenado coordinado (`quitAction_` / `closeEvent`).
4. **Target de pruebas de UI y arnés de validación (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
   - Arnés de pruebas Qt Test que verifica 499 líneas de aserciones de interacción visual: inicialización de pestañas, presencia y pixmap no nulo de logo corporativo, asignación de iconos vectoriales a acciones, atajos de teclado configurados, textos y atributos de accesibilidad, escalado de Modo Senior, contenedor nativo de error, recuperación ante caídas, y detección determinista de display (`DISPLAY` / `WAYLAND_DISPLAY`) con reporte honesto de `NOT_EXECUTED` si falta display.
5. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GNU GCC 15.2.0 bajo C++20 con `-Werror` y cero advertencias.
   - 13 de 13 pruebas pasando exitosamente en CTest con 100% de éxito en 0.45s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.06s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.05s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.03s)
     - `resource_qrc_inclusion_tests`: Passed (0.05s)
     - `startup_bootstrap_tests`: Passed (0.03s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.02s)
     - `cmake_presets_tests`: Passed (0.03s)
     - `cmake_presets_service_tests`: Passed (0.01s)
6. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 35 referencias canónicas validadas con estado `valid: true` (exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0103-module.foundations.task0035.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 y Q-008 resueltas (Q-007 solo bloquea `release.public-release`).
7. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio sin saturación.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar, botones amplios y tipografía legible.
   - `docs/UX-COPY-PUBLICO.md`: Proscripción de jerga de ingeniería («CMake», «Presets», «Toolchain», «Prompt Builder», «OpenSpec», «Senior Mode») en cualquier interfaz pública.

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0035`.
- **Capacidad**: `CMake presets`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0035/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0035/proposal.md`
  - `openspec/changes/module-foundations-task0035/design.md`
  - `openspec/changes/module-foundations-task0035/tasks.md`
  - `openspec/changes/module-foundations-task0035/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0035/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase PROPOSE.

---

## Diseño completo de la capacidad

### 1. Topología de Perfiles de Construcción y su Relación con la Interfaz Gráfica

La matriz de presets en `CMakePresets.json` se articula en dos categorías arquitectónicas nítidas:

```
                                  [ CMakePresets.json ]
                                            |
                   +------------------------+------------------------+
                   |                                                 |
         [ Presets de Interfaz ]                           [ Presets Headless ]
         (VIGILIO_BUILD_GUI=ON)                            (VIGILIO_BUILD_GUI=OFF)
                   |                                                 |
     +-------------+-------------+                     +-------------+-------------+
     |             |             |                     |             |             |
[desktop-debug] [desktop-release] [desktop-e2e]   [core-debug]   [core-release] [ci-linux-*]
     |             |             |                     |             |             |
     v             v             v                     v             v             v
  vigilio-      vigilio-      vigilio_              vigilio_      vigilio_      vigilio_
  browser       browser       e2e_smoke             core_tests    storage_tests presets_*
  (Debug)       (Release)     (WebEngine)           (Fast)        (Fast)        (Fast)
     +
  vigilio_
  ui_tests
  (WindowTests)
```

1. **Perfiles de Interfaz Gráfica (`VIGILIO_BUILD_GUI=ON`)**:
   - `desktop-debug`: Perfil estándar de desarrollo interactivo. Activa `find_package(Qt6 COMPONENTS Core Widgets WebEngineWidgets)`, compila `src/app.cpp`, `src/ui/BrowserWindow.cpp` y `resources.qrc` con `CMAKE_AUTOMOC ON` y `CMAKE_AUTORCC ON`. Genera tanto el ejecutable de aplicación `vigilio-browser` como el target de pruebas Qt Test `vigilio_ui_tests` (`WindowTests.cpp`).
   - `desktop-release`: Perfil de producción y empaquetado. Optimizado con `-O3` / `/O2`, desactiva `BUILD_TESTING` para reducir tiempos de build y tamaño binario, y configura flags de bundle nativo (`WIN32_EXECUTABLE`, `MACOSX_BUNDLE`).
   - `desktop-e2e`: Perfil especializado de validación de humo end-to-end con interfaz y motor web real Chromium (`VIGILIO_ENABLE_E2E=ON`), ejecutando `vigilio_e2e_smoke` con bloqueo de recursos serial para no saturar memoria RAM.

2. **Perfiles Headless del Núcleo (`VIGILIO_BUILD_GUI=OFF`)**:
   - `core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`: Compilan exclusivamente `vigilio_core` y `vigilio_storage`, permitiendo ejecutar el pipeline completo de pruebas de lógica de negocio, bases de datos SQLite y validación de presets en servidores o contenedores sin interfaz gráfica ni paquetes de Qt Widgets.

### 2. Contrato de Aislamiento y Desacoplamiento Arquitectónico

- **Independencia del Servicio (`vigilio::CMakePresetsService`)**:
  - `CMakePresetsService` permanece en `apps/vigilio-browser/src/core/CMakePresetsService.h` y `.cpp`.
  - No incluye ninguna cabecera de Qt Widgets (`QWidget`, `QMainWindow`, `QAction`, `QMenu`).
  - Utiliza exclusivamente tipos estándar de C++20 (`std::string`, `std::vector`, `std::filesystem::path`, `std::optional`, `std::atomic<bool>`).
  - Esto garantiza que el núcleo de compilación y orquestación de presets sea portable, reutilizable en herramientas CLI headless o scripts de mantenimiento, y no introduzca sobrecarga de memoria en la interfaz gráfica del usuario.
- **Independencia de la Ventana Principal (`BrowserWindow`)**:
  - `BrowserWindow` permanece en `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`.
  - No contiene lógica de ejecución de procesos de compilación, invocación de CMake ni parsing de presets.
  - La interfaz gráfica está dedicada al 100% a la navegación web para el usuario general, preservando su estabilidad, reactividad y simplicidad.

### 3. Síntesis y Validación Operativa de Perfiles de UI en `CMakePresetsService`

El servicio operativo implementado en la etapa previa es capaz de sintetizar y validar deterministamente los comandos asociados a los presets de interfaz:

```cpp
// Consulta de perfiles de interfaz
auto desktopPreset = service.findPreset("desktop-debug", CMakePresetKind::Configure);
assert(desktopPreset.has_value());
assert(desktopPreset->cacheVariables.at("VIGILIO_BUILD_GUI") == "ON");

// Síntesis de comando de compilación para UI
std::string buildCmd = service.buildBuildCommandLine("build-desktop-debug");
// Produce: "cmake --build --preset build-desktop-debug"

// Síntesis de comando de pruebas de interfaz
std::string testCmd = service.buildTestCommandLine("test-desktop-full");
// Produce: "ctest --preset test-desktop-full"
```

El servicio verifica que los perfiles de UI tengan definidas sus variables de caché canónicas (`VIGILIO_BUILD_GUI: "ON"`, `BUILD_TESTING: "ON"`), y que su directorio binario asociado (`${sourceDir}/build-desktop-debug`) cumpla las directrices de seguridad de `CMakePresetsValidator::isSafeBinaryDir`.

---

## Archivos afectados

Durante la fase APPLY de este pack, los cambios se concentrarán en los siguientes archivos autorizados:

1. `apps/vigilio-browser/CMakeLists.txt`:
   - Auditoría y verificación de las propiedades de compilación de UI gobernadas por presets:
     - Comprobación de que `vigilio-browser` y `vigilio_ui_tests` enlazan correctamente `vigilio_core` (que contiene `CMakePresetsService`), `vigilio_storage`, `vigilio_compiler_warnings` y las bibliotecas Qt6 correspondientes.
     - Garantía de que `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` están activos para los targets de interfaz.
     - Preservación de la guarda condicional `if(VIGILIO_BUILD_GUI)` y `if(TARGET Qt6::Test)` para compilación limpia en modo headless.
2. `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
   - Incorporación de aserciones de gobernanza de perfiles UI y validación de contratos visuales:
     - Verificación de que la ventana y los controles gráficos compilan y ejecutan limpiamente bajo la configuración generada por los presets de escritorio.
     - Preservación del mecanismo de detección de entorno gráfico (`std::getenv("DISPLAY")`, `std::getenv("WAYLAND_DISPLAY")`) emitiendo `NOT_EXECUTED` de forma honesta si no hay display disponible.
3. `docs/rules/01-modules/foundations/micro/0035-cmake-presets/task.md`:
   - Actualización del estado de la microtarea de `NOT_EXECUTED` a `EXECUTED` tras la aprobación y ejecución de APPLY.
4. `openspec/changes/module-foundations-task0035/`:
   - Artefactos OpenSpec de propuesta, diseño, tareas y memoria de ejecución.

---

## Contrato UX/UI

### 1. Ausencia Formal de Cambio Visual Directo
- En estricto cumplimiento de `docs/UX-COPY-PUBLICO.md`:
  - **No se agregan menús, opciones, botones, ventanas de diálogo ni tooltips de «CMake Presets» en la ventana principal `BrowserWindow`**.
  - Los términos «CMake», «presets», «toolchain», «ninja», «configurePresets», «buildPresets», «testPresets», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» están terminantemente prohibidos en cualquier superficie gráfica visible al usuario final.
  - La razón técnica es evidente: Vigilio Browser es un navegador para el público general, no un entorno de desarrollo integrado (IDE). Las capacidades de construcción y empaquetado son exclusivas para desarrolladores, pipelines de CI y herramientas de automatización.

### 2. Preservación de Contratos de Presentación en Ejecutables Generados por Presets
Los binarios de escritorio producidos mediante los presets `desktop-debug` y `desktop-release` respetan rigurosamente:
- **Dirección artística y referencias visuales**:
  - Coherencia con `/resultado/01-vigilio-home.png` (página de inicio con logo corporativo `branding/vigilio-logo.svg`, barra de direcciones sobria, sin brillos excesivos).
  - Coherencia con `/resultado/03-vigilio-settings-senior-mode.png` (interfaz accesible, controles amplios).
- **Iconografía vectorial SVG compilada en QRC**:
  - Barra de herramientas con iconos Lucide SVG (viewBox `0 0 24 24`, trazo 1.8px): Atrás, Adelante, Recargar, Inicio, Favorito, Favoritos, Nueva pestaña, Menú.
  - Menú principal con opciones «Interfaz más fácil de usar», «Tema oscuro», «Exportar configuración local…» y «Cerrar navegador».
- **Estados interactivos de widgets**:
  - `idle`: presentación base sobria.
  - `hover`: realce discreto sin animaciones agresivas.
  - `focus`: contorno visible accesible de 2px para navegación por teclado.
  - `disabled`: atenuación visual accesible en acciones sin historial previo/siguiente.
  - `loading`: indicador accesible en pestaña («Cargando…») y barra de estado («Cargando…»).
  - `empty`: página de inicio nativa (`newHomePage`) con cero instancias activas de WebEngine.
  - `error`: contenedor nativo accesible (`vigilio-page-error`) con icono de escudo, título «No se pudo cargar la página», botón primario «Reintentar» y secundario «Ir al inicio».
- **Adaptabilidad dinámica para «Interfaz más fácil de usar» (Senior Mode)**:
  - Activación mediante el menú principal (`seniorAction_`) o conmutador de accesibilidad.
  - Escalado automático de iconos de barra de herramientas: 22×22 px en modo estándar $\to$ 32×32 px en modo accesible.
  - Tap targets mínimos $\ge 48$ px de altura en botones (`QPushButton`) y campos de entrada (`QLineEdit`).
  - Altura mínima de barra de herramientas $\ge 55$ px.
  - Tipografía aumentada (+4 pt) manteniendo legibilidad y evitando truncados.
- **Accesibilidad asistiva y atajos de teclado**:
  - Atributos completos `accessibleName` y `accessibleDescription` en todos los controles interactivos.
  - Atajos canónicos: `Ctrl+T` (Nueva pestaña), `Ctrl+W` (Cerrar pestaña), `Ctrl+L` (Barra de direcciones), `Ctrl+D` (Guardar favorito), `Ctrl+B` (Ver favoritos), `Ctrl+Q` (Cerrar navegador), `Alt+Home` (Inicio), `Alt+F` (Menú).

---

## Reglas y fallos

1. **Gobernanza de configuración inválida o faltante**:
   - Si `CMakePresets.json` se encuentra ausente o malformado, `CMakePresetsValidator` y `CMakePresetsService` emiten diagnósticos de error claros y estructurados sin provocar segmentation faults ni excepciones no controladas.
   - Si un preset de interfaz intenta definir un directorio binario no seguro (ej. `/`, `C:\Windows`, o conteniendo `..`), la validación estática y operativa rechaza la ejecución con `CMakePresetExecutionStatus::BinaryDirUnsafe`.
2. **Degradación controlada ante ausencia de dependencias de Qt6**:
   - Si Qt6 o sus componentes de WebEngine Widgets no están instalados en el host, la configuración de CMake emite un mensaje de degradación (`message(WARNING "Qt dependency validation DEGRADED: ...")`) y omite la construcción de los targets de interfaz gráfica (`vigilio-browser`, `vigilio_ui_tests`), permitiendo compilar y probar limpiamente el núcleo headless.
3. **Tolerancia a entornos sin display en pruebas gráficas**:
   - Si se ejecutan pruebas de UI en un entorno sin servidor X11 o Wayland (`DISPLAY` ni `WAYLAND_DISPLAY`), `WindowTests.cpp` detecta la condición en tiempo de ejecución y retorna exitosamente con mensaje descriptivo `NOT_EXECUTED`, evitando fallos en cascada en entornos de integración continua headless.
4. **Resguardo de memoria y recursos (Política Zero-Zombie)**:
   - Los ejecutables producidos por presets no crean procesos auxiliares ni hilos de fondo descontrolados.
   - Toda operación de captura de salida en `CMakePresetsService` está acotada estrictamente a 64 KB (`maxOutputBytes = 65536`) y un tiempo de espera finito (`timeoutMs = 60000`).
5. **Aislamiento de sesiones de incógnito (Decisión Q-001)**:
   - En ventanas con `isIncognitoSession() == true`, no se construyen banners de recuperación, no se consulta ni escribe historial en `browser.sqlite` y se destruye el contexto de sesión al cerrar la ventana.

---

## Seguridad y privacidad

- **Local-First Constitucional (Decisión Q-001)**:
  - Cero dependencias de servicios externos, servidores en la nube o telemetría en los binarios compilados mediante cualquier preset.
  - Todas las herramientas invocadas por los presets de CMake residen localmente en el host del desarrollador o del pipeline de CI.
- **Higiene de Rutas y Prevención de Directory Traversal**:
  - Todo directorio binario de presets debe residir obligatoriamente dentro de `${sourceDir}/build-*` o `_build-*`.
  - Prohibición estricta de rutas absolutas no autorizadas o secuencias de escape de directorio (`..`).
- **Aislamiento de Procesos WebEngine y Permisos Locales**:
  - Los ejecutables de escritorio compilan con los flags estándar de sandboxing y aislamiento multiproceso provistos por Chromium WebEngine.
  - La base de datos SQLite local (`browser.sqlite`) se ubica en el directorio de perfil protegido del usuario con permisos de lectura/escritura restringidos.

---

## Matriz de pruebas

| ID Prueba | Tipo | Fixture / Comando | Resultado Esperado | Manejo de Fallo Negativo |
| --- | --- | --- | --- | --- |
| `TEST-PRESET-UI-001` | Integración CMake | `cmake --preset desktop-debug` | Configuración exitosa con Qt6 encontrado, definiendo `vigilio-browser` y `vigilio_ui_tests`. | Fallo si faltan variables de caché o targets obligatorios. |
| `TEST-PRESET-UI-002` | Compilación UI | `cmake --build --preset build-desktop-debug` | Compilación limpia bajo C++20 con `-Werror`, cero advertencias y enlace de `vigilio_core`. | Fallo si ocurren advertencias del compilador o errores de enlazado. |
| `TEST-PRESET-UI-003` | Suite UI CTest | `ctest --preset test-desktop-full` | Ejecución de `window_ui_tests` y suites unitarias; 100% PASS en entorno gráfico. | Reporta `NOT_EXECUTED` honestamente si no hay display. |
| `TEST-PRESET-UI-004` | Tolerancia Headless | `cmake --preset core-debug && ctest --preset test-core-fast` | Compilación y ejecución limpia de 100% de tests unitarios sin dependencias de GUI. | Falla si `core-debug` intenta buscar o enlazar Qt6 Widgets. |
| `TEST-PRESET-UI-005` | Interacción Ventana | `vigilio_ui_tests` (WindowTests) | Pestañas, buscador, branding corporate, iconografía QRC y atajos validados. | Falla si algún control carece de icono o shortcut. |
| `TEST-PRESET-UI-006` | Accesibilidad y Modo Senior | `vigilio_ui_tests` (WindowTests) | Iconos escalan 22$\to$32px, botones $\ge 48$px, toolbar $\ge 55$px, nombres accesibles completos. | Falla si no se respetan las dimensiones mínimas de tap target. |
| `TEST-PRESET-UI-007` | Página de Error Nativa | `vigilio_ui_tests` (WindowTests) | Contenedor `vigilio-page-error` con icono Lucide, textos públicos y botones de acción. | Falla si muestra terminología técnica o falta botón Reintentar. |
| `TEST-PRESET-UI-008` | Aislamiento Incógnito | `vigilio_ui_tests` (WindowTests) | Sesión privada con insignia visual, sin banner de recuperación y sin persistencia en disco. | Falla si se crea banner o se escribe en SQLite (Q-001). |
| `TEST-PRESET-UI-009` | Auditoría de Lenguaje | Inspección grep sobre UI | Cero ocurrencias de «CMake», «presets», «Senior Mode», «Prompt Builder», «DAG». | Falla si existe terminología interna en cadenas de UI visible. |
| `TEST-PRESET-UI-010` | Pipeline Completo | `bash apps/vigilio-browser/scripts/check.sh` | 13/13 tests pasando en CTest con salida limpia en < 1s. | Falla si algún test unitario o de integración no pasa. |

---

## Pasos APPLY

Cuando esta propuesta sea aprobada para la fase APPLY, se ejecutarán deterministamente los siguientes pasos:

1. **Auditar y consolidar las propiedades de los targets de UI en `apps/vigilio-browser/CMakeLists.txt`**:
   - Verificar que `vigilio-browser` y `vigilio_ui_tests` estén formalmente vinculados a `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y las bibliotecas Qt6 correspondientes bajo las guardas de presets `VIGILIO_BUILD_GUI` y `Qt6_FOUND`.
   - Confirmar la correcta activación de `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` para targets de interfaz.
2. **Revalidar la suite de pruebas de interfaz de usuario en `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Verificar la consistencia de las pruebas de UI frente a los contratos de compilación de presets.
   - Confirmar la detección y reporte transparente de `NOT_EXECUTED` cuando no haya display gráfico en el entorno de ejecución.
3. **Auditar el cumplimiento de lenguaje público (`docs/UX-COPY-PUBLICO.md`) en todas las superficies visuales**:
   - Confirmar mediante inspección estática que ningún archivo bajo `src/ui/` exponga terminología técnica («CMake», «presets», «Senior Mode», etc.).
4. **Ejecutar el pipeline de compilación y pruebas continuas**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh`, validando compilación limpia bajo C++20 con `-Werror` y 100% de tests en CTest.
5. **Verificar gobernanza y sincronización de documentación**:
   - Ejecutar `validate-references` y `verify-lock` con Prompt Builder CLI.
   - Actualizar el estado de la microtarea `0035` en `docs/rules/01-modules/foundations/micro/0035-cmake-presets/task.md`.

---

## Definición de terminado

La capacidad se considerará completa en la fase APPLY cuando:
1. Los perfiles canónicos de escritorio en `CMakePresets.json` (`desktop-debug`, `desktop-release`, `desktop-e2e`) compilen y ejecuten limpiamente los targets visuales de escritorio.
2. Los perfiles headless (`core-debug`, `core-release`, `ci-linux-debug`, `ci-linux-release`) continúen compilando y pasando el 100% de pruebas de lógica y almacenamiento sin requerir librerías gráficas.
3. Se declare formalmente el contrato de «sin cambio visual directo» para CMake Presets en la UI pública conforme a `docs/UX-COPY-PUBLICO.md`.
4. Todos los controles y estados de interfaz de usuario de `BrowserWindow` mantengan su coherencia con `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y las directrices de accesibilidad.
5. `apps/vigilio-browser/scripts/check.sh` complete con exit code 0 y 100% de pruebas aprobadas en CTest.
6. Se verifique la gobernanza documental mediante Prompt Builder CLI.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0034`: Implementación previa de la capa de servicio `vigilio::CMakePresetsService`.
  - Qt 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`): Dependencia condicional para perfiles de interfaz.
  - CMake 3.24+ y Ninja: Entorno de generación y construcción de perfiles.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Contaminación de código de UI con lógica de build de CMake.
    - *Mitigación*: Desacoplamiento estricto; `CMakePresetsService` reside exclusivamente en `vigilio_core` sin incluir cabeceras de Qt Widgets.
  - *Riesgo*: Exposición accidental de términos técnicos en la interfaz pública.
    - *Mitigación*: Cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md`; CMake Presets es una capacidad interna de compilación sin cambio visual en la UI pública.
  - *Riesgo*: Fallo de pruebas de UI en entornos CI sin servidor gráfico.
    - *Mitigación*: Detección en tiempo de ejecución de `DISPLAY` / `WAYLAND_DISPLAY` en `WindowTests.cpp`, reportando honestamente `NOT_EXECUTED`.

---

## Commit propuesto

- **Mensaje sugerido**: `feat(foundations): ui cmake-presets`
- **Alcance**: Contrato de UI, gobernanza de presets de interfaz gráfica, tolerancia headless y verificación de accesibilidad para CMake Presets.
