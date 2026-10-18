# Design — module-foundations-task0004

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- En `apps/vigilio-browser/CMakeLists.txt` la capacidad `cmake targets` cuenta con la siguiente estructura:
  - Biblioteca `vigilio_core`: estática, C++20 (`UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`), sin dependencias de GUI ni de SQLite.
  - Biblioteca `vigilio_storage`: estática, C++20 (`LocalRepository.cpp`), enlaza `SQLite::SQLite3` y depende de `vigilio_core`.
  - Target de pruebas unitarias `vigilio_core_tests` compilando `tests/unit/CoreTests.cpp`, registrado en CTest como `core_units` con etiquetas `unit;fast` y timeout de 15s.
  - Target de pruebas de integración `vigilio_storage_tests` compilando `tests/integration/StorageTests.cpp`, registrado en CTest como `storage_integration` con etiquetas `integration;fast` y timeout de 15s.
  - Bloque condicional `VIGILIO_BUILD_GUI=ON` con `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)` que genera `vigilio-browser`, incorpora `resources.qrc`, y bajo `BUILD_TESTING` define `vigilio_ui_tests` (`window_ui_tests`, etiquetas `ui;fast`, timeout 20s) y `vigilio_e2e_smoke` bajo `VIGILIO_ENABLE_E2E=ON` (`browser_e2e_smoke`, etiquetas `e2e;heavy`, timeout 60s, `RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`).
  - En ausencia de Qt6, CMake degrada elegantemente con mensaje informativo de advertencia (`message(WARNING ...)`), permitiendo la compilación y ejecución de pruebas del núcleo en entornos sin servidor gráfico.
- Las pruebas existentes en `tests/unit/CoreTests.cpp` y `tests/integration/StorageTests.cpp` cubren casos felices elementales, algunas entradas no válidas y guardado de sesión, pero carecen de pruebas profundas de recuperación ante corrupción, concurrencia en transacciones o errores de E/S simulados.

### Estado objetivo (TARGET)
- Una arquitectura de pruebas exhaustiva, determinista y trazable para **cmake targets** que verifique formalmente:
  1. **Integridad del DAG de CMake**: las dependencias entre targets respetan estrictamente la jerarquía arquitectónica; `vigilio_core` no puede depender directa ni transitivamente de `Qt6::Widgets` o `Qt6::WebEngineWidgets`.
  2. **Casos positivos y negativos en suites de prueba**:
     - Casos positivos: normalización de URLs, búsquedas por defecto, apertura y duplicación de pestañas, transacciones SQLite válidas, serialización de sesiones y carga de recursos embebidos.
     - Casos negativos: rechazo de esquemas prohibidos (`javascript:`, `file:///`, URLs con contraseñas o tokens), caracteres nulos/no imprimibles, URLs con más de 2048 caracteres, inyecciones SQL simuladas en títulos de marcadores, y apertura de bases de datos o sesiones en directorios de solo lectura o inaccesibles.
  3. **Concurrencia, límites de RAM y gobernanza de procesos**:
     - Contención de memoria: los tests unitarios y de integración se ejecutan de manera instantánea (sub-segundo) y consumen menos de 50MB de RAM.
     - Las pruebas pesadas de navegador (`vigilio_e2e_smoke`) están estrictamente serializadas en CTest mediante `RUN_SERIAL TRUE` y `RESOURCE_LOCK browser_e2e`, evitando la saturación de CPU/RAM o conflictos de puertos entre instancias concurrentes.
     - Políticas ZERO-ZOMBIE y timeout estricto en todas las suites para prevenir procesos bloqueados en espera infinita.
  4. **Recuperación y resiliencia**:
     - El parser de sesiones se recupera ante archivos truncados o con contenido basura devolviendo una lista vacía o el subconjunto válido de URLs, sin lanzar excepciones no controladas ni provocar fallos de segmentación.
     - La base de datos SQLite se recupera ante cierres forzados o reintentos de inicialización idempotentes.
     - Todos los fixtures temporales (`.sqlite`, `.txt`, `.wal`, `.shm`) se limpian automáticamente mediante patrones RAII y bloques de limpieza de archivos al término de cada ejecución de prueba.
  5. **Compatibilidad headless y aislamiento de entorno**:
     - Soporte verificado de compilación y ejecución íntegra de pruebas en modo headless (`VIGILIO_BUILD_GUI=OFF`) en entornos de integración continua sin display X11/Wayland.

### Brechas identificadas (GAPS)
- **Falta de cobertura de casos negativos extremos**: `CoreTests.cpp` no valida el comportamiento ante entradas con secuencias de escape no válidas, longitudes extremas de texto o deserialización de archivos de sesión con sintaxis corrupta.
- **Falta de pruebas de fallo y recuperación de persistencia**: `StorageTests.cpp` no valida el comportamiento ante fallos de disco o intentos de inserción con identificadores de marcador inválidos o caracteres Unicode suplementarios.
- **Validación de aislamiento arquitectónico en tiempo de construcción**: se requiere verificar que el target `vigilio_core` no exponga cabeceras de Qt ni genere advertencias o enlaces cruzados en tiempo de compilación.

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar C++20 (`set(CMAKE_CXX_STANDARD 20)` y `set(CMAKE_CXX_STANDARD_REQUIRED ON)`).
  - Sistemas operativos: Linux (x86_64), Windows 10/11 (x64 con MSVC / Clang), macOS (Intel / Apple Silicon).
  - Herramientas de construcción: CMake >= 3.24, Ninja >= 1.10.
  - Framework de pruebas: CTest integrado en CMake, con suites basadas en aserciones C++ y Qt Test para interacción de interfaz.
  - Modo headless: total compatibilidad garantizada bajo `VIGILIO_BUILD_GUI=OFF`.
- **Migración**:
  - No aplica migración de esquema de base de datos ni alteración de datos de usuario; se trata de una etapa de pruebas sobre targets existentes.
  - Los scripts de verificación existentes (`apps/vigilio-browser/scripts/check.sh`) se mantienen plenamente compatibles.
- **Rollback**:
  - En la fase APPLY, la reversión se efectúa restaurando los archivos de prueba en `apps/vigilio-browser/tests/` a su versión anterior.
  - En esta fase PROPOSE, la reversión consiste únicamente en descartar los artefactos bajo `openspec/changes/module-foundations-task0004/`.

## Diseño completo de la capacidad: cmake targets (etapa tests)

### 1. Grafo de dependencias y aislamiento de targets
La estructura del grafo de targets en CMake se define formalmente para garantizar la modularidad y el aislamiento de responsabilidades:

```
      [ Qt6::Core ]    [ Qt6::Widgets ]    [ Qt6::WebEngineWidgets ]
            \                 |                   /
             \                |                  /
              v               v                 v
           +---------------------------------------+
           |           vigilio-browser             |  (Ejecutable GUI)
           +---------------------------------------+
                           |            |
                (PRIVATE)  |            |  (PRIVATE)
                           v            v
  +------------------+        +--------------------+
  |   vigilio_core   |<-------|  vigilio_storage   |
  +------------------+        +--------------------+
           ^                            ^
           | (PRIVATE)                  | (PRIVATE)
           |                            |
  +------------------+        +--------------------+
  |vigilio_core_tests|        |vigilio_storage_tests|
  +------------------+        +--------------------+
```

- **Invariante 1**: `vigilio_core` no tiene dependencias hacia arriba. No conoce la existencia de SQLite, de Qt Widgets ni de Chromium WebEngine.
- **Invariante 2**: `vigilio_storage` solo depende de `vigilio_core` y de `SQLite::SQLite3`. No enlaza ninguna biblioteca visual.
- **Invariante 3**: `vigilio_core_tests` y `vigilio_storage_tests` son ejecutables de consola autónomos que no requieren servidor gráfico, display X11/Wayland ni inicialización de `QApplication`.

### 2. Taxonomía y clasificación de pruebas en CTest
Las pruebas se organizan en CTest siguiendo una taxonomía rigurosa para optimizar la velocidad y el consumo de recursos:

| Target de prueba | Suite CTest | Etiquetas CTest | Timeout | Requisitos de entorno | Concurrencia |
| --- | --- | --- | --- | --- | --- |
| `vigilio_core_tests` | `core_units` | `unit;fast` | 15s | Consola / Headless (cero dependencias externas) | Paralelizable (`ctest -jN`) |
| `vigilio_storage_tests` | `storage_integration` | `integration;fast` | 15s | Consola / Headless (requiere filesystem temporal) | Paralelizable (rutas temporales aisladas) |
| `vigilio_ui_tests` | `window_ui_tests` | `ui;fast` | 20s | Qt6 Core/Widgets (requiere display o QPA offscreen) | Paralelizable con QPA offscreen |
| `vigilio_e2e_smoke` | `browser_e2e_smoke` | `e2e;heavy` | 60s | Qt6 WebEngine completo (requiere display/render) | Serial estricto (`RUN_SERIAL TRUE`, `RESOURCE_LOCK browser_e2e`) |

### 3. Especificación de casos de prueba

#### A. Casos positivos / normales
1. **Resolución de URLs y consultas**:
   - URL canónica directa (`https://example.com`) clasificada como `NavigationTarget::Kind::Url`.
   - Dominio simple sin esquema (`example.com`) normalizado automáticamente a `https://example.com`.
   - Host local con puerto (`localhost:8080`) resuelto a `http://localhost:8080`.
   - Consulta de texto ordinaria (`buenas tardes`) clasificada como `NavigationTarget::Kind::Search` y codificada a consulta URL estándar (`https://www.google.com/search?q=buenas+tardes`).
   - Caracteres con diacríticos o espacios (`ñandú`, `two + two`) codificados en porcentaje correctamente.
2. **Modelo de pestañas (`TabModel`)**:
   - Apertura secuencial de pestañas con asignación determinista de identificadores únicos incrementales.
   - Marcado de pestañas fijadas (`setPinned`) y silenciadas (`setMuted`).
   - Duplicación de pestaña preservando el estado de la URL.
   - Operaciones de cierre selectivo (`closeOthers`), garantizando que las pestañas fijadas sobrevivan al cierre en masa.
3. **Persistencia de sesión (`SessionFile`)**:
   - Guardado atómico de listas de URLs en formato delimitado por líneas.
   - Carga y reconstrucción determinista del vector de URLs.
4. **Almacenamiento SQLite (`LocalRepository`)**:
   - Creación de esquema relacional (`bookmarks`, `history`) en base de datos nueva.
   - Inserción y actualización idempotente de marcadores por URL.
   - Registro de visitas de historial con marca temporal.
   - Verificación de persistencia tras cierre y reapertura de la base de datos con una nueva instancia de `LocalRepository`.

#### B. Casos negativos y entradas inválidas
1. **Filtro de seguridad de esquemas de navegación**:
   - Rechazo estricto de esquema `javascript:` (`javascript:alert(1)`) devolviendo `NavigationTarget::Kind::Rejected`.
   - Rechazo de acceso a archivos del sistema de archivos local (`file:///etc/passwd`).
   - Rechazo de esquemas malformados o incompletos (`https://`, `ftp://`).
   - Rechazo de URLs con credenciales embebidas (`https://user:secret@example.com`) para prevenir ataques de suplantación y fuga de credenciales.
   - Rechazo de cadenas compuestas exclusivamente por espacios en blanco o cadenas vacías.
2. **Robustez ante entradas extremas o maliciosas**:
   - Intentos de inyección SQL en títulos de marcadores (ej. `O'Reilly; DROP TABLE bookmarks; --` o comillas simples desbalanceadas): las consultas preparadas y parametrizadas de SQLite deben almacenar el texto literal sin alterar la estructura de la base de datos.
   - Inserción de URLs rechazadas en el repositorio: `LocalRepository::addBookmark` debe devolver `false` y no modificar el estado interno.
   - Solicitudes de cierre o duplicación sobre IDs de pestaña inexistentes (`tabs.close(9999)`): el modelo debe devolver `false` o `std::nullopt` sin lanzar excepciones no controladas.
3. **Manejo de fallos de E/S y corrupción**:
   - Archivo de sesión inexistente o inaccesible: `loadSession` debe devolver un vector vacío sin fallar de manera catastrófica.
   - Archivo de sesión con líneas vacías intermedias o caracteres nulos: el cargador debe omitir las entradas corruptas e importar únicamente las URLs válidas.
   - Base de datos en ruta de solo lectura o sin permisos de escritura: `LocalRepository` debe reportar `valid() == false` y fallar limpiamente las operaciones de escritura sin generar un bloqueo de aplicación.

#### C. Concurrencia, límites de RAM y gobernanza de procesos
1. **Límites estrictos de memoria**:
   - Las pruebas unitarias de `vigilio_core` y de integración de `vigilio_storage` operan sin instanciar componentes WebEngine ni subprocesos de renderizado de Chromium, manteniendo un consumo de memoria inferior a 50MB.
   - En pruebas de UI de ventana (`vigilio_ui_tests`), solo se instancia la estructura de widgets y hojas de estilo; no se inician procesos de renderizado web.
2. **Serialización y bloqueo de recursos en pruebas pesadas**:
   - La prueba `vigilio_e2e_smoke` utiliza `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE` en CMake para impedir que dos instancias compitan por recursos gráficos o puertos de depuración en la misma máquina.
3. **Limpieza determinista de fixtures (RAII)**:
   - Todo archivo temporal (`.sqlite`, `-wal`, `-shm`, `.txt`) creado por los tests debe ubicarse en el directorio temporal del sistema (`std::filesystem::temp_directory_path()`) y eliminarse obligatoriamente al finalizar el bloque de prueba, incluso en caso de excepción o fallo de aserción.

## Archivos afectados

| Archivo | Rol | Estado en PROPOSE | Modificación en APPLY |
| --- | --- | --- | --- |
| `apps/vigilio-browser/CMakeLists.txt` | Definición de targets y suites CTest | Inspeccionado / Existente | Verificación y formalización de propiedades de CTest (`TIMEOUT`, `LABELS`, `RESOURCE_LOCK`) |
| `apps/vigilio-browser/tests/unit/CoreTests.cpp` | Suite de pruebas unitarias de dominio | Inspeccionado / Existente | Incorporación de casos negativos de URL, límites de longitud y robustez de sesión |
| `apps/vigilio-browser/tests/integration/StorageTests.cpp` | Suite de pruebas de persistencia SQLite | Inspeccionado / Existente | Incorporación de casos de inyección SQL, manejo de errores de base de datos y verificación de rollback |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Suite de pruebas de interacción de ventana | Inspeccionado / Existente | Verificación de contratos de widgets y accesibilidad sin procesos pesados de WebEngine |
| `apps/vigilio-browser/tests/e2e/BrowserSmoke.cpp` | Suite de humo E2E con motor Chromium | Inspeccionado / Existente | Aislamiento bajo guarda `VIGILIO_ENABLE_E2E=ON` y bloqueo de recursos `browser_e2e` |
| `docs/rules/01-modules/foundations/micro/0004-cmake-targets/task.md` | Especificación canónica de la microtarea | Inspeccionado / Existente | Sincronización documental de estado a `CURRENT; PASS` en fase REVIEW |
| `openspec/changes/module-foundations-task0004/**` | Artefactos OpenSpec del cambio | Autorizado / Creado en este paso | Documentación completa de propuesta, diseño, tareas y memoria de ejecución |

## Contrato UX/UI

### Naturaleza de infraestructura: sin cambio visual directo
- La capacidad **cmake targets** en su etapa **tests** es una función puramente de infraestructura de software, validación y aseguramiento de la calidad de compilación.
- De conformidad con las directrices de `docs/rules/01-modules/foundations/micro/0004-cmake-targets/task.md` («Para tareas sin interfaz, documentar por qué no hay cambio visual directo» y «Si no toca UI, declarar sin cambio visual y verificar contratos existentes»):
  - **No se añade ningún botón, menú, diálogo ni indicador visual con la etiqueta «cmake targets» en la interfaz pública del navegador**.
  - Los usuarios del navegador no tienen contacto directo con el sistema de pruebas ni con los comandos de CTest.
  - El impacto de esta etapa en la UI consiste en **garantizar que el target ejecutable `vigilio-browser` pueda ser compilado y probado de forma automatizada y sin regresiones**.

### Cumplimiento estricto de lenguaje de producto público
- Conforme a `docs/UX-COPY-PUBLICO.md`:
  - Queda estrictamente prohibido que cualquier mensaje de diálogo, barra de estado o texto de interfaz visible contenga jerga técnica de ingeniería como `cmake`, `targets`, `DAG`, `Prompt Builder`, `PROPOSE`, `APPLY`, `OpenSpec` o `Senior Mode`.
  - La funcionalidad de accesibilidad se rotula de forma obligatoria y exclusiva como **«Interfaz más fácil de usar»**.
  - Los mensajes de error observables en la interfaz ante fallos de navegación deben utilizar lenguaje natural en español: «Dirección no permitida», «No se pudo cargar la página», «Verifica la conexión o la dirección escrita».

### Verificación de contratos existentes de UI y accesibilidad
- Las pruebas de UI (`vigilio_ui_tests`) verifican:
  - Inicialización de la ventana con al menos una pestaña activa.
  - Presencia y foco accesible de la barra de direcciones (`QLineEdit`) con nombre accesible asignado (`setAccessibleName("Barra de direcciones y búsqueda")`).
  - Navegación por teclado completa (atajos `Ctrl+T`, `Ctrl+W`, `Ctrl+L`).
  - Modos de accesibilidad: soporte de escalado tipográfico a 14pt y controles con dimensiones mínimas de 48px de altura según los contratos de `docs/UX-UI-SENIOR-CONTRATOS.md` y las referencias visuales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.

## Reglas y fallos

- **Regla 1 (Aislamiento absoluto de capas en CMake)**: `vigilio_core` no debe enlazar ni incluir símbolos de Qt Widgets o SQLite. Si un desarrollador intenta incluir una cabecera de UI en el código de core, la compilación de `vigilio_core` debe fallar inmediatamente.
- **Regla 2 (Determinismo y cero flakiness en pruebas)**: Todas las pruebas de CTest deben ser reproducibles e independientes del orden de ejecución. Queda prohibido el uso de esperas pasivas con retardos temporales arbitrarios (`sleep`); toda sincronización debe basarse en eventos observables o condiciones explícitas con timeout acotado.
- **Regla 3 (Aislamiento de persistencia y limpieza RAII)**: Ninguna prueba puede escribir en la base de datos de producción o en el directorio de usuario. Todas las operaciones de prueba deben utilizar rutas en directorios temporales (`/tmp` o equivalente del SO) y garantizar la eliminación completa de los archivos al concluir, evitando la contaminación del entorno.
- **Regla 4 (Gobernanza de recursos y prevención de procesos zombies)**: Las pruebas pesadas de navegador (`vigilio_e2e_smoke`) deben ejecutarse de manera estrictamente serializada mediante el bloqueo CTest `browser_e2e`. Todo proceso de prueba debe finalizar limpiamente antes del timeout establecido (60s). Si un proceso no responde dentro de su timeout, CTest debe abortarlo y reportar fallo explícito.
- **Regla 5 (Honestidad en el reporte de estado de pruebas)**: En entornos donde no se disponga de bibliotecas gráficas o servidor X11/Wayland, las pruebas que requieran interfaz deben registrarse honestamente como `NOT_EXECUTED` o no compilarse mediante guardas condicionales (`if(Qt6_FOUND)`). Queda terminantemente prohibido falsear un resultado positivo (`PASS`) mediante mocks simulados o capturas vacías de error.

## Seguridad y privacidad

- **Modelo local-first y ausencia de telemetría**:
  - Ninguna prueba realiza llamadas de red externa, consultas DNS remotas ni envía telemetría a servidores de terceros.
  - Las pruebas de navegación utilizan exclusivamente fixtures HTML locales generados en el directorio temporal (`file://` acotado al fixture efímero) o esquemas sintéticos controlados.
- **Aislamiento de sesiones e incógnito**:
  - Las sesiones y bases de datos de prueba se inicializan con identificadores efímeros (`VigilioTests`) y se descartan tras la ejecución.
  - Las URLs no permitidas (`javascript:`, `file:///etc/passwd`, URLs con credenciales embebidas) se rechazan formalmente en la frontera de validación de `UrlInput`, impidiendo ataques de Cross-Site Scripting (XSS), inclusión de archivos locales (LFI) o filtración de credenciales.
- **Prevención de inyecciones SQL**:
  - `LocalRepository` utiliza exclusivamente consultas preparadas (`sqlite3_prepare_v2`) y vinculación de parámetros tipados (`sqlite3_bind_text`). Se valida que cadenas con contenido SQL malicioso (`' OR 1=1; --`) se almacenen como texto plano sin alterar la semántica de la consulta.

## Matriz de pruebas trazables

| Criterio | Prueba | Fixture | Comando | Resultado esperado | Fallo negativo esperado | Clasificación |
| --- | --- | --- | --- | --- | --- | --- |
| Desacoplamiento de targets en CMake | Inspección de dependencias de enlace de `vigilio_core` | `apps/vigilio-browser/CMakeLists.txt` | `cmake --preset core-debug` | Configuración exitosa sin enlace a Qt6 ni SQLite en `vigilio_core` | Error de configuración si `vigilio_core` intenta enlazar bibliotecas gráficas | APPLY |
| Resolución de URLs y búsquedas | Pruebas unitarias de resolución y normalización de entradas | `tests/unit/CoreTests.cpp` | `ctest --test-dir apps/vigilio-browser/_build -R core_units` | Entradas válidas clasificadas como `Url` o `Search` con codificación adecuada | Error de aserción (`CHECK`) si la URL no es normalizada o la consulta falla | APPLY |
| Filtro de seguridad de esquemas de navegación | Pruebas de rechazo de esquemas maliciosos o peligrosos | `tests/unit/CoreTests.cpp` | `ctest --test-dir apps/vigilio-browser/_build -R core_units` | `javascript:`, `file:///`, credenciales embebidas y espacios devuelven `Kind::Rejected` | Fallo de seguridad si alguna entrada maliciosa es aceptada como `Url` | APPLY |
| Gestión de pestañas y concurrencia lógica | Operaciones sobre `TabModel` (abrir, fijar, silenciar, duplicar, cerrar otros) | `tests/unit/CoreTests.cpp` | `ctest --test-dir apps/vigilio-browser/_build -R core_units` | Operaciones exitosas; pestañas fijadas sobreviven a `closeOthers` | Fallo si se cierran pestañas fijadas o se desborda el índice | APPLY |
| Persistencia y recuperación de sesión | Serialización y deserialización de archivos de sesión | `tests/unit/CoreTests.cpp` con archivo temporal | `ctest --test-dir apps/vigilio-browser/_build -R core_units` | Guardado y recuperación exacta de lista de URLs; archivo eliminado tras test | Fallo si no se recuperan todas las URLs o el archivo temporal no se elimina | APPLY |
| Persistencia e integridad en SQLite | Creación de base de datos, inserción de marcadores y visitas | `tests/integration/StorageTests.cpp` con base SQLite temporal | `ctest --test-dir apps/vigilio-browser/_build -R storage_integration` | Marcadores e historial almacenados y recuperados correctamente tras reapertura | Fallo si la base de datos se corrompe o los marcadores se pierden | APPLY |
| Resistencia a inyección SQL | Inserción de marcadores con cadenas SQL maliciosas en títulos | `tests/integration/StorageTests.cpp` con carga inyectada | `ctest --test-dir apps/vigilio-browser/_build -R storage_integration` | Texto malicioso almacenado como literal; estructura de tabla intacta | Fallo de base de datos o excepción de SQLite si la consulta no está parametrizada | APPLY |
| Aislamiento headless en script de verificación | Compilación y paso íntegro de pruebas rápidas en modo headless | `apps/vigilio-browser/scripts/check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | Compilación limpia con Ninja y 100% de tests aprobados (`fast`) con exit code 0 | Exit code != 0 si falla alguna compilación o test | APPLY |
| Suite de pruebas de interacción de ventana Qt Test | Pruebas de instanciación de ventana, pestañas y controles accesibles | `tests/ui/WindowTests.cpp` | `ctest -R window_ui_tests` (condicionado a Qt6) | Verificación de ventana, pestañas, `QLineEdit` y `QToolBar` | `NOT_EXECUTED` si falta entorno Qt6 completo | LATER / CONDICIONAL |
| Prueba de humo gráfica E2E con motor Chromium | Carga de fixture HTML local y verificación de título en `BrowserSmoke` | `tests/e2e/BrowserSmoke.cpp` con fixture local | `ctest -R browser_e2e_smoke` (condicionado a `VIGILIO_ENABLE_E2E=ON`) | Fixture local cargado y señal `loadFinished` recibida en menos de 20s | Salida `GUI smoke FAIL` si el motor WebEngine se bloquea o no carga el fixture | LATER / E2E HEAVY |

## Dependencias y riesgos

- **Dependencia canónica**: `module.foundations.task0003` (etapa `ui` de cmake targets, finalizada y validada).
- **Riesgos técnicos y alternativas evaluadas**:
  1. *Riesgo*: Fugas de procesos de renderizado de WebEngine al interrumpirse una prueba.
     *Mitigación*: Uso obligatorio de `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE` en CMake para pruebas pesadas, con timeouts estrictos para prevenir ejecuciones colgadas indefinidamente.
  2. *Riesgo*: Conflictos de concurrencia al ejecutar múltiples suites de prueba en paralelo en máquinas multinúcleo.
     *Mitigación*: Cada suite de prueba utiliza nombres de archivo temporales únicos basados en el identificador de proceso (`pid`) o directorio temporal aislado del sistema (`std::filesystem::temp_directory_path()`), garantizando independencia absoluta entre workers.
  3. *Riesgo*: Indisponibilidad de display en servidores de integración continua.
     *Mitigación*: Arquitectura dividida en suites: las pruebas esenciales de dominio y almacenamiento son 100% de consola y no requieren display; las pruebas de GUI se ejecutan bajo plataformas con soporte gráfico o se registran como `NOT_EXECUTED` sin romper el pipeline.
