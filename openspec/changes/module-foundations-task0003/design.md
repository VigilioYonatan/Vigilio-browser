# Design — module-foundations-task0003

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- `apps/vigilio-browser/CMakeLists.txt` contiene la modularización básica alcanzada en `module.foundations.task0002`:
  - `vigilio_core`: biblioteca estática en C++20 desacoplada de SQLite y de bibliotecas gráficas.
  - `vigilio_storage`: biblioteca estática en C++20 que enlaza `SQLite::SQLite3` y depende de `vigilio_core`.
  - Guarda condicional `if(VIGILIO_BUILD_GUI)` que ejecuta `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)`.
  - Si Qt6 está presente, define el ejecutable `vigilio-browser` compilando `src/app.cpp`, `src/ui/BrowserWindow.cpp` y `resources.qrc`, activando `CMAKE_AUTOMOC ON` y `CMAKE_AUTORCC ON`.
  - Guarda condicional adicional `if(BUILD_TESTING AND VIGILIO_ENABLE_E2E)` que busca `Qt6::Test` y compila `vigilio_e2e_smoke` con propiedades `e2e;heavy`, timeout de 60s, `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE`.
  - Si Qt6 no está presente, emite una advertencia informativa con `message(WARNING ...)` y omite la construcción de la interfaz gráfica sin interrumpir los targets de core y almacenamiento.
- `apps/vigilio-browser/src/ui/BrowserWindow.cpp` implementa la ventana gráfica del navegador:
  - Fila de pestañas con `QTabWidget` document-mode.
  - Barra de herramientas con acciones de navegación (Atrás, Adelante, Recargar, Inicio) y barra de direcciones (`QLineEdit`).
  - Soporte integrado de temas claro y oscuro mediante hojas de estilo Qt (`qApp->setStyleSheet`).
  - Modo de accesibilidad («Interfaz más fácil de usar») con escalado de fuentes a 14pt y controles con altura mínima de 48px.
  - Manejo de sesión diferida (lazy loading de pestañas restauradas para ahorro de RAM).
- `apps/vigilio-browser/resources.qrc` compila 36 recursos estáticos (iconos SVG Lucide, tokens de diseño JSON y paletas de color).

### Estado objetivo (TARGET)
- Coherencia estructural completa y verificable para todos los targets relacionados con la interfaz gráfica en CMake:
  1. **Aislamiento formal de compilación**: los flags y propiedades de compilación de UI (`CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, flags de bundle de macOS y ejecutable WIN32) deben aplicarse con precisión de target o estar rigurosamente acotados a la sección gráfica, impidiendo la contaminación de herramientas de generación de código sobre bibliotecas headless.
  2. **Arnés de pruebas de UI desacoplado de procesos pesados de navegador**: establecimiento de una suite de pruebas Qt Test enfocada exclusivamente en los contratos de widgets, foco, atajos y estados de accesibilidad de la ventana (`BrowserWindow`), separable del smoke test pesado con motor Chromium (`vigilio_e2e_smoke`).
  3. **Conformidad absoluta con el lenguaje público de producto**: garantía formal de que ningún target, diálogo, tooltip o mensaje visible al usuario contenga nombres de ingeniería prohibidos por `docs/UX-COPY-PUBLICO.md` (`cmake`, `targets`, `Senior Mode`, `DAG`, `Prompt Builder`, `OpenSpec`).
  4. **Documentación explícita de ausencia de cambio visual directo**: especificación clara en la taxonomía de producto de que la capacidad `cmake targets` es una capacidad de infraestructura de construcción y, como tal, no posee un control visual interactivo rotulado con su nombre.
  5. **Mantenimiento estricto del modo headless**: compilación e integración continua completamente funcionales en modo headless (`VIGILIO_BUILD_GUI=OFF`) sin fallos ni dependencias de servidor gráfico.

### Brechas identificadas (GAPS)
- **Falta de target de prueba unitaria de UI en CMake**: actualmente solo existe `vigilio_e2e_smoke` (pesado, con carga real de WebEngine y dependiente de `VIGILIO_ENABLE_E2E=ON`); no existe un target intermedio para probar interacciones de widgets Qt puros sin levantar el motor web completo.
- **Configuración global de MOC/RCC**: `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` se declaran a nivel de directorio en el bloque condicional en lugar de estar acotados explícitamente a las propiedades del target ejecutable o biblioteca de UI.
- **Auditoría documental de términos en UI**: las etiquetas y títulos de ventana deben cotejarse formalmente contra las directrices de `docs/UX-COPY-PUBLICO.md` para certificar que el término «Modo Senior» no aparece en las cadenas visibles de `BrowserWindow`.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar C++20 (`set(CMAKE_CXX_STANDARD 20)`).
  - Framework de interfaz: Qt 6.10+ (módulos `Core`, `Widgets`, `WebEngineWidgets`, `Test`).
  - Generador: Ninja / CMake 3.24+.
  - Sistemas operativos: Linux (X11/Wayland), Windows 10/11 x64, macOS 12+ (x64/arm64).
  - Modo headless: soporte garantizado con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`), sin requerir librerías gráficas ni paquetes de desarrollo de Qt6.
- **Migración**:
  - No requiere migraciones de base de datos ni modificaciones de esquemas.
  - La incorporación y ajuste de targets en `CMakeLists.txt` preserva compatibilidad con todos los scripts de compilación existentes (`scripts/check.sh`, presets de CMake).
- **Rollback**:
  - En la fase APPLY, la reversión se efectúa restaurando la versión previa de `apps/vigilio-browser/CMakeLists.txt`.
  - En esta fase PROPOSE, la reversión consiste únicamente en descartar los artefactos bajo `openspec/changes/module-foundations-task0003/`.

## Diseño completo de la capacidad: cmake targets (etapa ui)

### 1. Jerarquía y topología de targets de interfaz
El grafo de targets en CMake integra los componentes de interfaz de la siguiente forma:

```
[ Qt6::WebEngineWidgets ]  [ Qt6::Widgets ]  [ Qt6::Core ]
           \                     |                 /
            \                    |                /
             v                   v               v
            +------------------------------------+
            |          vigilio-browser           | <=== (src/app.cpp, src/ui/BrowserWindow.cpp,
            +------------------------------------+      resources.qrc)
                       ^              ^
                       | (PRIVATE)    | (PRIVATE)
                       |              |
              [ vigilio_core ]   [ vigilio_storage ]
                       ^              ^
                       |              | (PUBLIC)
                       +--------------+
```

1. **Target principal de aplicación (`vigilio-browser`)**:
   - *Tipo*: `add_executable(vigilio-browser ...)`.
   - *Archivos fuente*:
     - `src/app.cpp`: punto de entrada de la aplicación, configuración de atributos de alta densidad (`Qt::AA_EnableHighDpiScaling`), inicialización de `QApplication` y arranque de la ventana principal.
     - `src/ui/BrowserWindow.cpp` y `src/ui/BrowserWindow.h`: implementación y cabecera de la ventana nativa de escritorio basada en `QMainWindow`.
     - `resources.qrc`: manifiesto compilado de recursos XML que integra iconos Lucide SVG, hojas de estilo y tokens de diseño.
   - *Dependencias de enlace*:
     - `PRIVATE vigilio_core`: modelos de pestañas, validación y normalización de URLs (`UrlInput`), deserialización de sesiones.
     - `PRIVATE vigilio_storage`: repositorio de persistencia local en SQLite (`LocalRepository`).
     - `PRIVATE Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`.
   - *Propiedades de empaquetado nativo*:
     - `WIN32_EXECUTABLE TRUE`: evita la apertura de la ventana de consola secundaria en Windows.
     - `MACOSX_BUNDLE TRUE`: genera el bundle `.app` estándar en macOS con metadata de bundle identifier.
     - `OUTPUT_NAME "vigilio-browser"`.
   - *Instalación*:
     - `install(TARGETS vigilio-browser BUNDLE DESTINATION . RUNTIME DESTINATION bin)`.

2. **Target de validación de interfaz (`vigilio_ui_tests` / Qt Test)**:
   - *Tipo*: `add_executable(vigilio_ui_tests ...)` condicionado a `BUILD_TESTING AND VIGILIO_BUILD_GUI AND Qt6_FOUND`.
   - *Archivos*: arnés de pruebas de UI para instanciación de ventana, atajos y cambio de accesibilidad sin ejecutar renderizado web.
   - *Dependencias*: `PRIVATE vigilio_core vigilio_storage Qt6::Core Qt6::Widgets Qt6::Test`.
   - *Propiedades CTest*: `LABELS "ui;fast" TIMEOUT 20`.

3. **Target de prueba de humo extremo a extremo (`vigilio_e2e_smoke`)**:
   - *Tipo*: `add_executable(vigilio_e2e_smoke tests/e2e/BrowserSmoke.cpp ...)` condicionado a `BUILD_TESTING AND VIGILIO_BUILD_GUI AND VIGILIO_ENABLE_E2E`.
   - *Dependencias*: `PRIVATE vigilio_core vigilio_storage Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test`.
   - *Propiedades CTest*: `LABELS "e2e;heavy" TIMEOUT 60 RESOURCE_LOCK browser_e2e RUN_SERIAL TRUE`.

### 2. Manejo de recursos compilados (`resources.qrc`)
- Inclusión directa en la lista de fuentes de `vigilio-browser` para asegurar que el compilador de recursos de Qt (`rcc`) genere código estático embebido en el binario final.
- Prefijo virtual `/vigilio` con alias estandarizados:
  - Iconos de navegación: `/vigilio/icons/navigation/{back,forward,reload,home,search,bookmarks,history,settings,tabs,download}.svg`.
  - Iconos de herramientas: `/vigilio/icons/tools/{shield,senior,memory,reader,mcp,tools,note,camera,split,qr,extensions}.svg`.
  - Marca: `/vigilio/branding/vigilio-logo.svg`.
  - Tokens de diseño: `/vigilio/design-tokens/tokens.json`.
  - Temas: `/vigilio/themes/{light,dark,midnight,forest,sunset,aurora}.json`.
- Invariante de seguridad: los recursos embebidos son de solo lectura y no permiten la ejecución de scripts no verificados.

### 3. Aislamiento headless y guardas condicionales
- La guarda condicional en `apps/vigilio-browser/CMakeLists.txt` asegura que:
  ```cmake
  option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
  if(VIGILIO_BUILD_GUI)
    find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)
    if(Qt6_FOUND)
      # Definición de targets gráficos vigilio-browser y vigilio_e2e_smoke
    else()
      message(WARNING "Qt6 Core/Widgets/WebEngineWidgets unavailable: core and tests only; browser GUI NOT built")
    endif()
  endif()
  ```
- Si `Qt6_FOUND` es falso o `VIGILIO_BUILD_GUI=OFF`, los targets gráficos no se registran en el grafo de compilación y las suites de pruebas unitarias/integración de core (`vigilio_core_tests` y `vigilio_storage_tests`) compilan y pasan exitosamente con salida 0.

## Archivos afectados

| Archivo | Rol | Estado en PROPOSE | Modificación en APPLY |
| --- | --- | --- | --- |
| `apps/vigilio-browser/CMakeLists.txt` | Configuración de construcción de CMake | Inspeccionado / Existente | Refinamiento de propiedades de targets de UI, recursos y arnés de pruebas de UI |
| `apps/vigilio-browser/resources.qrc` | Manifiesto de recursos Qt | Inspeccionado / Existente | Verificación de integridad de alias y referencias SVG |
| `apps/vigilio-browser/src/ui/BrowserWindow.h` | Cabecera de la ventana principal | Inspeccionado / Existente | Verificación de contratos y métodos de accesibilidad |
| `apps/vigilio-browser/src/ui/BrowserWindow.cpp` | Implementación de la ventana principal | Inspeccionado / Existente | Auditoría de etiquetas de texto público según `docs/UX-COPY-PUBLICO.md` |
| `docs/rules/01-modules/foundations/micro/0003-cmake-targets/task.md` | Especificación canónica de la microtarea | Inspeccionado / Existente | Sincronización documental de estado a `CURRENT; PASS` en REVIEW |
| `openspec/changes/module-foundations-task0003/**` | Artefactos OpenSpec del cambio | Autorizado / Creado en este paso | Documentación exhaustiva de propuesta, diseño, tareas y memoria |

## Contrato UX/UI

### Naturaleza de infraestructura: sin cambio visual directo
- La capacidad **cmake targets** es un componente de bajo nivel que estructura la compilación, enlace y empaquetado del software en CMake.
- Conforme a las reglas explícitas de `docs/rules/01-modules/foundations/micro/0003-cmake-targets/task.md` («Para tareas sin interfaz, documentar por qué no hay cambio visual directo» y «Si no toca UI, declarar sin cambio visual y verificar contratos existentes»):
  - **No existe ningún botón, diálogo, menú o elemento visual rotulado con el nombre «cmake targets» en la interfaz gráfica del usuario**.
  - Los usuarios del navegador no interactúan con el sistema de construcción de CMake.
  - La relación de esta microtarea con la UI reside en la **gobernanza de compilación del target ejecutable `vigilio-browser`**, que empaqueta y entrega la interfaz gráfica nativa en Qt Widgets.

### Contrato de la ventana principal (`vigilio-browser`)
El target ejecutable `vigilio-browser` implementa la ventana de escritorio cumpliendo estrictamente con `docs/UX-UI-SENIOR-CONTRATOS.md` y las referencias visuales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`:
- **Barra de navegación**:
  - Botones con iconos vectoriales Lucide SVG (Atrás, Adelante, Recargar, Inicio).
  - Barra de direcciones (`QLineEdit`) centrada, con foco visible mediante borde resaltado de 2px, atajo `Ctrl+L` para enfoque directo y soporte completo para lectores de pantalla (`setAccessibleName("Barra de direcciones y búsqueda")`).
- **Sistema de pestañas (`QTabWidget`)**:
  - Pestañas movibles y cerrables con botón de cierre accesible (`Ctrl+W` para cerrar, `Ctrl+T` para nueva pestaña).
  - Menú contextual de pestañas (Nueva pestaña a la derecha, Duplicar, Fijar, Silenciar, Cerrar).
- **Estados visuales de controles**:
  - `idle`: fondo neutral (`#FFFFFF` en modo claro, `#222D3E` en modo oscuro) con borde sutil.
  - `hover`: oscurecimiento o aclarado del fondo con transición limpia.
  - `focus`: contorno de contraste elevado según directrices WCAG 2.2 AA.
  - `disabled`: opacidad reducida al 50%, manteniendo legibilidad y anunciando estado a tecnologías asistivas.
  - `loading`: texto temporal «Cargando…» en la pestaña e indicador de estado no invasivo en la barra de estado.
  - `error`: diálogo claro con explicación comprensible y opción de reintento («Dirección no permitida», «No se pudo cargar la página»).
- **Accesibilidad y «Interfaz más fácil de usar» (Modo Senior)**:
  - Activación mediante toggle en el menú y atajo accesible.
  - Conmutación dinámica de tamaño de tipografía a 14pt (frente a 10pt base).
  - Elevación de tap targets y campos de texto a una altura mínima de 48px (barra de herramientas a 55px) para facilitar la interacción de personas con dificultades motrices.
  - Etiqueta pública obligatoria: **«Interfaz más fácil de usar»**, conforme a `docs/UX-COPY-PUBLICO.md`. Queda terminantemente prohibido mostrar «Senior Mode» o «Modo Senior» en cualquier elemento visible.
  - Mensaje en barra de estado: «Interfaz fácil activada» o «Interfaz fácil desactivada».

## Reglas y fallos

- **Regla 1 (Aislamiento de dependencias)**: El target de biblioteca `vigilio_core` no debe enlazar ni incluir cabeceras de Qt Widgets o WebEngine. Si se detecta un intento de inclusión cruzada, la compilación debe fallar inmediatamente.
- **Regla 2 (Degradación elegante ante ausencia de Qt)**: Si en la configuración de CMake `Qt6_FOUND` resulta falso, CMake no debe abortar la ejecución, sino emitir una advertencia con `message(WARNING ...)` y configurar exitosamente los targets de core y almacenamiento.
- **Regla 3 (Integridad de recursos)**: Si falta un recurso referenciado en `resources.qrc`, la herramienta `rcc` de Qt debe fallar la compilación durante la fase de build, evitando binarios con iconos o temas faltantes en tiempo de ejecución.
- **Regla 4 (Resiliencia en persistencia y perfiles)**: Si el archivo SQLite de perfil local no puede abrirse (por ejemplo, por permisos denegados en disco), la ventana `BrowserWindow` debe iniciar en modo degradado temporal sin bloquear el hilo principal de la UI (`QApplication::exec()`), permitiendo la navegación web ordinaria y notificando en la barra de estado.
- **Regla 5 (Cancelación y escape)**: Toda acción modal o menú contextual desplegado debe cerrarse de forma segura al presionar la tecla `Escape`, restaurando el foco al control que originó la interacción.

## Seguridad y privacidad

- **Límites de confianza y modelo local-first**:
  - La aplicación de escritorio construida por el target `vigilio-browser` opera exclusivamente con almacenamiento local en el equipo del usuario (directorio estándar `QStandardPaths::AppDataLocation`).
  - Cero conexiones a servidores de autenticación, nube de Vigilio o servicios de telemetría remota.
  - Prohibición de telemetría implícita o analítica de uso embebida en la UI.
- **Aislamiento de procesos y memoria RAM**:
  - En la restauración de sesión (`BrowserWindow::restoreSession`), únicamente la primera pestaña activa inicializa una instancia de `QWebEngineView`. Las pestañas secundarias se inicializan como marcadores diferidos (`deferredUrl`), evitando la creación masiva de procesos auxiliares de renderizado de Chromium y conteniendo el uso de memoria por debajo de los límites especificados en `docs/skills/06-rendimiento-y-ram.md`.
- **Privacidad en navegación**:
  - Los datos de historial de navegación solo se registran para URLs con esquema explícito `http` o `https`.
  - La exportación de datos locales (`BrowserWindow::exportSettings`) genera un archivo JSON que incluye únicamente ajustes visuales y marcadores, excluyendo expresamente cookies, contraseñas e historial privado.

## Matriz de pruebas trazables

| Criterio | Prueba | Fixture | Comando | Resultado esperado | Fallo negativo esperado | Clasificación |
| --- | --- | --- | --- | --- | --- | --- |
| Desacoplamiento de targets en CMake | Inspección de dependencias de enlace de `vigilio_core` y `vigilio_storage` | `apps/vigilio-browser/CMakeLists.txt` | `cmake --preset core-debug` | Configuración limpia sin mención a Qt6 en targets de core | Error de configuración si `vigilio_core` intenta enlazar `Qt6::Widgets` | APPLY |
| Compilación y paso de suite headless | Ejecución de suite CTest de core y persistencia | Tests de `CoreTests.cpp` y `StorageTests.cpp` | `ctest --preset core-debug` | 100% de tests aprobados (`core_units`, `storage_integration`) | Exit code != 0 si falla alguna aserción | APPLY |
| Degeneración limpia sin Qt6 en desktop-debug | Configuración de preset desktop en entorno sin dev headers de Qt | `CMakeLists.txt` con `VIGILIO_BUILD_GUI=ON` | `cmake --preset desktop-debug` | Advertencia informativa de CMake; configuración exitosa sin abortar | Aborto fatal con `FATAL_ERROR` si se fuerza `find_package(Qt6 REQUIRED)` | APPLY |
| Aislamiento de recursos embebidos | Validación de manifiesto `resources.qrc` | `apps/vigilio-browser/resources.qrc` | Verificación de rutas de archivos en disco | 36 recursos presentes en `assets/` | Error de compilación de `rcc` si falta algún SVG o JSON | APPLY |
| Verificación de textos públicos en UI | Auditoría estática de cadenas en código fuente de UI | `src/ui/BrowserWindow.cpp` | Búsqueda de términos prohibidos (`Senior Mode`, `cmake`, `DAG`, `OpenSpec`) | Cero coincidencias en cadenas visibles al usuario | Fallo de auditoría si se encuentra «Modo Senior» en lugar de «Interfaz más fácil de usar» | APPLY |
| Suite de pruebas de interacción de ventana Qt Test | Pruebas de instanciación, foco de teclado y toggle de accesibilidad | Arnés de pruebas Qt Test (`tests/ui/`) | `ctest -R ui_tests` (condicionado a Qt6) | Verificación de foco, escalado a 14pt y tamaño de controles a 48px | Asignado como `NOT_EXECUTED` si falta paquete de desarrollo de Qt6 | LATER / CONDICIONAL |
| Prueba de humo gráfica E2E con Chromium | `vigilio_e2e_smoke` | `tests/e2e/BrowserSmoke.cpp` con fixture HTML local | `ctest --preset desktop-debug -R browser_e2e_smoke` | Carga de fixture HTML local y título correcto en 20s | `GUI smoke FAIL` si el motor WebEngine no responde | LATER / E2E HEAVY |

## Dependencias y riesgos

- **Dependencia canónica**: `module.foundations.task0002` (etapa `implementation` de cmake targets, finalizada y validada).
- **Riesgos técnicos y mitigaciones**:
  1. *Riesgo*: Indisponibilidad de encabezados de desarrollo de Qt6 en entornos de integración continua o máquinas virtuales ligeras.
     *Mitigación*: Mantener el diseño modular con opción `VIGILIO_BUILD_GUI=OFF` y registro honesto del estado `NOT_EXECUTED` para pruebas de GUI en ausencia del SDK completo de Qt6, garantizando que el build headless nunca se bloquee.
  2. *Riesgo*: Fugas de memoria o acumulación de procesos huérfanos de WebEngine en pruebas automatizadas.
     *Mitigación*: Gobernanza estricta en CMake con `RUN_SERIAL TRUE` y `RESOURCE_LOCK browser_e2e`, limitando la ejecución concurrente y garantizando la liberación de perfiles temporales.
