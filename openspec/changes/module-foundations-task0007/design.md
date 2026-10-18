# Design — module-foundations-task0007

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- En `apps/vigilio-browser/CMakeLists.txt`, la infraestructura centralizada de advertencias del compilador `vigilio_compiler_warnings` quedó implementada en la etapa previa (`module.foundations.task0006`):
  - Declaración del target de interfaz `add_library(vigilio_compiler_warnings INTERFACE)`.
  - Definición de los catálogos canónicos completos para GNU GCC (`GCC_WARNINGS`), LLVM Clang (`CLANG_WARNINGS`) y MSVC (`MSVC_WARNINGS`).
  - Configuración de la opción `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)` que inyecta `-Werror` o `/WX`.
  - Vinculación `PRIVATE` a `vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke`.
- En la capa de interfaz de usuario de Qt Widgets (`apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`):
  - La ventana principal `BrowserWindow` hereda de `QMainWindow` y gestiona pestañas (`QTabWidget`), navegación (`QLineEdit`, `QToolBar`), favoritos locales en SQLite (`LocalRepository`), conmutación de temas (claro/oscuro) y el modo de accesibilidad («Interfaz más fácil de usar»).
  - El código de UI compila condicionalmente cuando `Qt6_FOUND` es verdadero bajo `VIGILIO_BUILD_GUI=ON`.
  - Cuando Qt6 no está disponible en el entorno (como en el contenedor actual), CMake emite una advertencia informativa con `message(WARNING ...)` y el proyecto compila limpiamente en modo headless (`VIGILIO_BUILD_GUI=OFF`).
- En el arnés de pruebas de UI (`apps/vigilio-browser/tests/ui/WindowTests.cpp`):
  - Implementa verificaciones básicas de instanciación de ventana, presencia de barra de direcciones, barra de herramientas y conteo de pestañas.
  - Se vincula condicionalmente a `Qt6::Test` y `vigilio_compiler_warnings`.

### Estado objetivo (TARGET)
- Una especificación formal y gobernanza rigurosa de la interacción entre el sistema estricto de advertencias del compilador C++20 (`vigilio_compiler_warnings`) y la capa de presentación Qt Widgets que garantice:
  1. **Inmunidad y robustez estática en la capa de presentación**: el código fuente de UI (`BrowserWindow.h`, `BrowserWindow.cpp`, `app.cpp`, `WindowTests.cpp`) cumple íntegramente con los diagnósticos estrictos de C++20 sin generar advertencias latentes ni requerir excepciones ad-hoc.
  2. **Higiene estricta de conversiones y polimorfismo en widgets**:
     - Cero conversiones implícitas estrechantes o de pérdida de signo (`-Wconversion`, `-Wsign-conversion`) al operar con geometrías de widgets (`int` frente a `qsizetype` o `size_t`), conteo de pestañas (`tabs_->count()`), escalado de fuentes y tap targets mínimos de 48px para la «Interfaz más fácil de usar».
     - Destructores virtuales correctos en jerarquías polimórficas (`-Wnon-virtual-dtor`, `-Woverloaded-virtual`).
     - Ausencia de sombreado de variables (`-Wshadow`) en callbacks de señales y slots de Qt.
     - Prevención de desreferenciación nula (`-Wnull-dereference`) en la inspección de punteros devueltos por la API de Qt (`currentWidget()`, `findChildren()`, `qobject_cast`).
  3. **Aislamiento transparente de herramientas de generación de código Qt**: `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` operan sin interferencias con las flags estrictas, aislando las cabeceras del framework Qt6 como `SYSTEM`.
  4. **Declaración formal de ausencia de cambio visual directo**: especificación contractual de que la capacidad `compiler warnings` no añade controles visibles rotulados con terminología de ingeniería en la interfaz pública del navegador, en estricto cumplimiento de `docs/UX-COPY-PUBLICO.md`.
  5. **Preservación incondicional de la compilación headless**: el modo headless (`VIGILIO_BUILD_GUI=OFF`) permanece como baseline inmutable para CI y entornos de desarrollo sin dependencias de pantalla.
  6. **Registro honesto de dependencias de prueba**: documentación transparente de `NOT_EXECUTED` para pruebas gráficas cuando falten bibliotecas de desarrollo de Qt6.

### Brechas identificadas (GAPS)
- **Verificación documental de higiene estática en UI**: no existía hasta esta propuesta una auditoría explícita de cómo los flags estrictos de `vigilio_compiler_warnings` interactúan con las clases de widgets y eventos de Qt.
- **Auditoría de cadenas públicas de UI**: es necesario certificar que en `BrowserWindow.cpp` no exista ninguna exposición accidental de términos técnicos prohibidos («compiler warnings», «warnings», «-Werror», «Senior Mode», «DAG», «CMake»).
- **Registro de ejecución en entornos sin Qt**: documentar formalmente la degradación limpia en ausencia de paquetes gráficos y clasificar honestamente el estado del arnés de pruebas de UI como `NOT_EXECUTED` en entornos headless.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar: C++20 (`set(CMAKE_CXX_STANDARD 20)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`, `set(CMAKE_CXX_EXTENSIONS OFF)`).
  - Framework de interfaz: Qt 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
  - Compiladores soportados: GNU GCC >= 11, LLVM Clang >= 14, AppleClang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Sistemas operativos: Linux (x86_64, aarch64), Windows 10/11 x64, macOS 12+ (x64 y Apple Silicon).
  - Modo headless: soporte garantizado mediante `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`), sin requerir librerías gráficas ni paquetes de desarrollo de Qt6.
- **Migración**:
  - No requiere migraciones de esquemas SQLite ni alteración de datos de usuario.
  - La formalización de la gobernanza de UI para `compiler warnings` no altera la API pública ni las firmas de métodos de `vigilio_core` o `vigilio_storage`.
- **Rollback**:
  - En esta fase PROPOSE, la reversión consiste únicamente en descartar los artefactos bajo `openspec/changes/module-foundations-task0007/`.
  - En la fase APPLY, la reversión se efectúa restaurando la versión previa de los archivos de UI y CMake mediante control de versiones.

## Diseño completo de la capacidad: compiler warnings (etapa ui)

### 1. Integración de advertencias estrictas en la capa de interfaz gráfica

La capa de presentación de Vigilio Browser está compuesta por el ejecutable de usuario `vigilio-browser` y el ejecutable de pruebas de interfaz `vigilio_ui_tests`. Ambos targets se configuran bajo la guarda `VIGILIO_BUILD_GUI=ON` y enlazan de forma privada `vigilio_compiler_warnings`:

```
+-----------------------------------------------------------------------------------+
|                        Target INTERFACE: vigilio_compiler_warnings                |
|  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow                 |
|  -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual   |
|  -Wformat=2 -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough          |
|  -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op   |
|  -Werror / /WX (VIGILIO_WARNINGS_AS_ERRORS=ON)                                    |
+-----------------------------------------------------------------------------------+
                                       |
                  +--------------------+--------------------+
                  | (PRIVATE)                               | (PRIVATE)
                  v                                         v
     [ vigilio-browser ]                           [ vigilio_ui_tests ]
  (src/app.cpp, src/ui/BrowserWindow.cpp,       (tests/ui/WindowTests.cpp,
   resources.qrc)                                src/ui/BrowserWindow.cpp,
                                                 resources.qrc)
```

### 2. Higiene de código C++20 en componentes Qt Widgets

Para garantizar que el código de interfaz compile limpiamente bajo `-Werror` sin excepciones ni debilitamiento de flags, se establecen las siguientes reglas de implementación en `BrowserWindow`:
1. **Prevención de conversiones estrechantes y de signo (`-Wconversion`, `-Wsign-conversion`)**:
   - En Qt, métodos como `QTabWidget::count()` devuelven `int`, mientras que `std::vector::size()` y algoritmos estándar operan con `size_t`. Toda comparación o asignación entre tamaños debe utilizar conversiones explícitas (`static_cast<int>`) o tipos consistentes.
   - En el escalado de tipografías y geometrías para la «Interfaz más fácil de usar» (Modo Senior), las asignaciones de puntos de fuente (10pt a 14pt) y alturas mínimas (32px a 48px, 40px a 55px) se definen con tipos integrales enteros bien definidos, evitando truncamientos implícitos.
2. **Jerarquía polimórfica y destructores virtuales (`-Wnon-virtual-dtor`, `-Woverloaded-virtual`)**:
   - `BrowserWindow` hereda de `QMainWindow` (que a su vez hereda de `QWidget` y `QObject`). Su destructor está declarado como `~BrowserWindow() override;` y su definición en `.cpp` utiliza `= default;`, asegurando la destrucción ordenada de recursos.
   - Todo método virtual reimplementado (`closeEvent(QCloseEvent* event) override`) respeta exactamente la firma de la clase base.
3. **Ausencia de sombreado de identificadores (`-Wshadow`) en conexiones de señales y slots**:
   - En las expresiones lambda conectadas a señales (`connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) { ... })`), los parámetros formales (`index`) y las variables locales no colisionan con miembros de la clase `BrowserWindow` ni con variables de ámbitos envolventes.
4. **Inspección segura de punteros (`-Wnull-dereference`)**:
   - `BrowserWindow::currentWebView()` utiliza `qobject_cast<QWebEngineView*>(tabs_->currentWidget())`. Todo método consumidor verifica explícitamente `if (auto* v = currentWebView())` antes de invocar métodos de navegación (`back()`, `forward()`, `reload()`).
   - El puntero de base de datos `db_` se verifica mediante `if (db_ && db_->valid())` antes de acceder a favoritos o historial.

### 3. Aislamiento de herramientas Qt (`moc`, `rcc`) y cabeceras de terceros

1. **Aislamiento `SYSTEM` de dependencias de terceros**:
   - Los targets importados de Qt6 (`Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`, `Qt6::Test`) y SQLite3 (`SQLite::SQLite3`) tienen sus directorios de inclusión catalogados automáticamente como `SYSTEM` por CMake.
   - Esto instruye al compilador (GCC, Clang) a ignorar cualquier diagnóstico o advertencia que pudiera originarse dentro del código de las bibliotecas de Qt o SQLite.
2. **Inmunización de código generado automáticamente**:
   - `CMAKE_AUTOMOC ON`: el Meta-Object Compiler genera archivos `moc_*.cpp` necesarios para el sistema de señales y slots. Al estar acotadas las advertencias estrictas a nivel de target privado, el código generado no sufre advertencias por expansiones internas de macros Qt como `Q_OBJECT`.
   - `CMAKE_AUTORCC ON`: el compilador de recursos compila `resources.qrc` embebiendo los 36 iconos Lucide SVG y archivos de temas en el ejecutable de forma transparente.

### 4. Preservación del modo headless y gobernanza de pruebas

- Si `VIGILIO_BUILD_GUI=OFF` (o si Qt6 no está presente en el sistema), los targets `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke` no se compilan.
- El sistema ejecuta la suite rápida headless (`core_units`, `storage_integration`) con `vigilio_compiler_warnings` aplicado de forma privada, garantizando cero advertencias bajo `-Werror`.
- Para las pruebas de interfaz que no puedan ejecutarse por ausencia de paquetes gráficos de Qt en el entorno de desarrollo o CI, se registra honestamente `NOT_EXECUTED` de acuerdo con el protocolo.

## Archivos afectados

| Archivo | Rol | Estado en PROPOSE | Modificación en APPLY |
| --- | --- | --- | --- |
| `apps/vigilio-browser/CMakeLists.txt` | Sistema de construcción CMake | Inspeccionado / Existente | Verificación de vinculación privada de `vigilio_compiler_warnings` en targets de UI y propiedades de aislamiento |
| `apps/vigilio-browser/src/app.cpp` | Punto de entrada del ejecutable GUI | Inspeccionado / Existente | Auditoría de cero advertencias con flags estrictos |
| `apps/vigilio-browser/src/ui/BrowserWindow.h` | Cabecera de la ventana principal | Inspeccionado / Existente | Auditoría de destructores virtuales y const-correctness |
| `apps/vigilio-browser/src/ui/BrowserWindow.cpp` | Implementación de la ventana principal | Inspeccionado / Existente | Auditoría de higiene de tipos C++20, ausencia de sombreado y conformidad con `docs/UX-COPY-PUBLICO.md` |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Arnés de pruebas de UI | Inspeccionado / Existente | Verificación de compatibilidad con `vigilio_compiler_warnings` |
| `docs/rules/01-modules/foundations/micro/0007-compiler-warnings/task.md` | Especificación canónica de la microtarea | Inspeccionado / Existente | Sincronización documental de estado a `CURRENT; PASS` en REVIEW |
| `openspec/changes/module-foundations-task0007/**` | Artefactos OpenSpec del cambio | Autorizado / Creado en este paso | Documentación completa de propuesta, diseño, tareas y memoria de ejecución |

## Contrato funcional

- **Actor/Invocación**: El desarrollador o el sistema de integración continua al compilar los targets de interfaz gráfica (`cmake --build apps/vigilio-browser/_build --target vigilio-browser` o arnés de pruebas de UI).
- **Entrada**: Archivos fuente C++20 de UI (`app.cpp`, `BrowserWindow.h`, `BrowserWindow.cpp`, `WindowTests.cpp`), cabeceras de Qt6, recursos SVG en `resources.qrc`, opciones de CMake (`VIGILIO_WARNINGS_AS_ERRORS=ON`).
- **Salida**: Ejecutable `vigilio-browser` y binario de pruebas `vigilio_ui_tests` compilados limpiamente con cero advertencias y exit code 0. Si se introduce código de UI con conversiones inseguras o variables sombreadas, el compilador aborta con código de error indicando archivo y línea.
- **Validación**: Comprobación de que la compilación con `VIGILIO_WARNINGS_AS_ERRORS=ON` no emite ningún diagnóstico de advertencia.
- **Permisos y roles**: No aplica (N/A); proceso local de compilación y pruebas de software.
- **Persistencia**: No aplica (N/A); la capacidad de advertencias no persiste datos.
- **Deshacer / Cancelar**: Cancelación inmediata mediante interrupción de proceso (`Ctrl+C`).
- **Concurrencia**: Plena compatibilidad con compilación paralela (`ninja -j$(nproc)`).
- **Recuperación ante errores**: Diagnósticos legibles emitidos por el compilador con sugerencias de tipado explícito o corrección de nombres de variables.

## Contrato UX/UI

### Naturaleza de infraestructura: ausencia formal de cambio visual directo
- La capacidad **compiler warnings** es un mecanismo de aseguramiento de calidad de código estático y robustez en tiempo de compilación.
- Conforme a las reglas explícitas de `docs/rules/01-modules/foundations/micro/0007-compiler-warnings/task.md` («Para tareas sin interfaz, documentar por qué no hay cambio visual directo» y «Si no toca UI, declarar sin cambio visual y verificar contratos existentes»):
  - **No existe ningún botón, diálogo, menú, campo o control rotulado con «compiler warnings» o «warnings» en la interfaz gráfica del navegador**.
  - Los usuarios del navegador no interactúan con el sistema de advertencias ni con las directivas de compilación.
  - La relación de esta microtarea con la UI reside en asegurar que la implementación de los widgets de `vigilio-browser` compila con cero advertencias y máxima higiene de código C++20.

### Cumplimiento del lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`)
- Queda terminantemente prohibido exponer en las cadenas de texto visibles del navegador términos técnicos de ingeniería:
  - Prohibido: `Senior Mode`, `Modo Senior`, `compiler warnings`, `warnings`, `-Werror`, `/WX`, `CMake`, `DAG`, `OpenSpec`, `Prompt Builder`.
  - Aprobado: **«Interfaz más fácil de usar»** para el modo accesible; **«Ahorro de memoria»** para la gestión de recursos; **«Protección contra anuncios y rastreadores»** para el escudo de privacidad.
- En `BrowserWindow.cpp`, la acción de accesibilidad utiliza textualmente:
  ```cpp
  seniorAction_ = mainMenu->addAction("Interfaz más fácil de usar");
  ```
  y los mensajes de la barra de estado anuncian: «Interfaz fácil activada» o «Interfaz fácil desactivada».

### Contrato de interacción de la ventana principal (`vigilio-browser`)
La ventana principal implementa los contratos visuales y de accesibilidad descritos en `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`:
- **Barra de herramientas y navegación**:
  - Botones de acción: Atrás, Adelante, Recargar, Inicio, Favorito, Favoritos, Nueva pestaña, Menú.
  - Barra de direcciones (`QLineEdit`): centrada, con placeholder explicativo («Buscar o escribir una dirección web»), borde redondeado de 7px y atajo `Ctrl+L`.
- **Sistema de pestañas (`QTabWidget`)**:
  - Pestañas con modo documento (`setDocumentMode(true)`), cerrables (`setTabsClosable(true)`), movibles (`setMovable(true)`).
  - Atajos de teclado: `Ctrl+T` para nueva pestaña, `Ctrl+W` para cerrar pestaña activa.
  - Menú contextual de pestañas: Nueva pestaña a la derecha, Duplicar pestaña, Fijar/Desfijar pestaña, Activar/Silenciar sonido, Cerrar pestaña, Cerrar las demás pestañas.
- **Estados visuales de los controles**:
  - `idle`: superficie neutral (`#141D2B` oscuro / `#F7F9FC` claro), texto legible con alto contraste.
  - `hover`: retroalimentación sutil con cambio de tonalidad (`#294F89` en menú seleccionado / `#DCE9FA` en modo claro).
  - `focus`: contorno visible de enfoque para navegación accesible.
  - `disabled`: opacidad reducida manteniendo contraste legible.
  - `loading`: texto provisional «Cargando…» en la pestaña e indicador informativo en la barra de estado.
  - `empty`: página de inicio limpia con título «Vigilio Browser», lema «Navega. Organiza. Protege.» y campo de búsqueda centrado con accesibilidad.
  - `success`: mensaje informativo en la barra de estado («Página cargada», «Favorito guardado localmente»).
  - `error`: advertencia no bloqueante («Dirección no permitida», «No se pudo cargar la página»).
- **Accesibilidad y «Interfaz más fácil de usar» (Modo Senior)**:
  - Activación accesible con atajo y opción visible en el menú principal.
  - Escalado tipográfico general a 14pt (frente a 10pt base).
  - Altura mínima de campos de texto (`QLineEdit`) escalada a 48px (frente a 32px base).
  - Altura mínima de barra de herramientas (`QToolBar`) escalada a 55px (frente a 40px base).
  - Soporte completo para lectores de pantalla con nombres accesibles explícitos (`setAccessibleName`).

## Reglas y fallos

- **Regla 1 (Cero advertencias bajo Warnings-as-Errors)**: Todo el código de UI bajo `src/ui/**` y `tests/ui/**` debe compilar sin una sola advertencia cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`. Cualquier advertencia aborta la compilación.
- **Regla 2 (Aislamiento de cabeceras de terceros)**: Las cabeceras de Qt6 y SQLite3 deben incluirse como `SYSTEM`, impidiendo que advertencias en bibliotecas upstream rompan la compilación del proyecto.
- **Regla 3 (Degradación ante ausencia de entorno gráfico)**: En entornos donde no estén instalados los paquetes de desarrollo de Qt6, CMake debe emitir un aviso informativo con `message(WARNING ...)` y configurar limpiamente los targets headless sin abortar la construcción del proyecto.
- **Regla 4 (Conformidad estricta de textos públicos)**: Ningún texto visible al usuario puede contener jerga de compilación ni los términos prohibidos por `docs/UX-COPY-PUBLICO.md`.
- **Regla 5 (No bloquear el hilo de interfaz)**: Toda operación de persistencia, carga de página o restauración de pestañas debe realizarse de forma asíncrona o diferida para mantener la interfaz receptiva.

## Seguridad y privacidad

- **Filosofía local-first**: Los datos del navegador (favoritos, historial de navegación, configuración visual) se almacenan exclusivamente en SQLite local en la ruta del usuario (`QStandardPaths::AppDataLocation`).
- **Cero telemetría y sin nube**: La aplicación no se conecta a servidores de autenticación, nube de Vigilio ni servicios remotos de telemetría.
- **Aislamiento de memoria y procesos WebEngine**:
  - En la restauración de sesiones (`restoreSession`), únicamente la primera pestaña inicializa un proceso de renderizado `QWebEngineView`; las pestañas secundarias se inicializan como marcadores diferidos (`deferredUrl`), impidiendo la sobrecarga masiva de memoria RAM.
- **Protección de exportación de datos**: La función de exportación de ajustes locales genera un archivo JSON que incluye únicamente preferencias de apariencia y favoritos, excluyendo explícitamente contraseñas, cookies e historial de incógnito.

## Matriz de pruebas trazables

| Requisito / Criterio de Aceptación | Caso de Prueba / Escenario | Categoría | Fixture / Comando de Verificación | Resultado Esperado | Clasificación |
| --- | --- | --- | --- | --- | --- |
| Vinculación privada de advertencias en targets de UI | Inspección de dependencias de enlace de `vigilio-browser` y `vigilio_ui_tests` | Build / CMake | `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF` | Enlace privado a `vigilio_compiler_warnings` configurado correctamente | APPLY |
| Compilación headless limpia con advertencias estrictas | Compilación de `vigilio_core` y `vigilio_storage` con flags estrictos | Compilación / Unit | `cmake --build apps/vigilio-browser/_build` | 0 advertencias, 0 errores, compilación exitosa | APPLY |
| Ejecución limpia de suite de pruebas headless | Paso del 100% de tests unitarios e integración en CTest | CTest / Ejecución | `ctest --test-dir apps/vigilio-browser/_build --output-on-failure` | 100% tests aprobados (`core_units` y `storage_integration`) en < 0.05s | APPLY |
| Verificación mediante script automatizado | Ejecución de script de comprobación local | Integración / CI | `bash apps/vigilio-browser/scripts/check.sh` | Compilación Ninja exitosa y tests pasados con exit code 0 | APPLY |
| Auditoría de lenguaje público en código de UI | Búsqueda estática de términos técnicos prohibidos en cadenas de UI | UI / Copy | Búsqueda de `Senior Mode`, `compiler warnings`, `DAG`, `OpenSpec` en `src/ui/BrowserWindow.cpp` | Cero coincidencias en cadenas visibles al usuario | APPLY |
| Ausencia de cambio visual directo formalizada | Verificación de documentación y taxonomía de producto | Documentación | `grep -q "sin cambio visual directo" openspec/changes/module-foundations-task0007/design.md` | Declaración formal de ausencia de cambio visual directo | APPLY |
| Sincronización de referencias canónicas | Validación del árbol de referencias de Prompt Builder | Gobernanza | `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` | Estado `valid: true`, 7 referencias validadas | APPLY |
| Verificación de arnés de pruebas de UI (`WindowTests`) | Ejecución de pruebas de widgets Qt cuando Qt6 esté presente | UI / Qt Test | `ctest -R window_ui_tests` (condicionado a Qt6) | Verificación de widgets, foco y accesibilidad | NOT_EXECUTED (falta SDK Qt6 en entorno) |
| Pruebas de inducción negativa de advertencias de compilador | Verificación de rechazo de advertencias en pipeline dedicado | Negativo / Tests | Inyección deliberada de advertencia de compilación bajo `-Werror` | El compilador rechaza el código y aborta con exit code != 0 | LATER (task0008) |

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0006` (etapa `implementation` de compiler warnings, completada con el target de interfaz `vigilio_compiler_warnings`).
- **Dependencias posteriores**: `module.foundations.task0008` (etapa `tests` de compiler warnings, con pruebas negativas de rechazo).
- **Riesgos y mitigaciones**:
  1. *Riesgo*: Indisponibilidad de encabezados de desarrollo de Qt6 en máquinas virtuales o contenedores de integración continua.
     *Mitigación*: Preservar la guarda condicional `find_package(Qt6 6.10 QUIET ...)` con mensaje de advertencia y registro transparente de `NOT_EXECUTED` para pruebas de GUI, garantizando que el build headless nunca se bloquee.
  2. *Riesgo*: Aparición de falsos positivos en código generado automáticamente por MOC o RCC de Qt.
     *Mitigación*: Marcado de directorios de cabeceras de Qt como `SYSTEM` a través de los targets importados de CMake, suprimiendo advertencias ajenas al código propio.
