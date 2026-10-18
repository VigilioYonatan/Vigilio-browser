# Design — module-foundations-task0011

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)
- En `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `QtDependencyValidator.cpp`:
  - Se encuentra implementado el validador desacoplado en C++20 perteneciente a la biblioteca estática `vigilio_core`.
  - Provee las entidades tipadas `SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome` y `QtValidationReport`.
  - Ofrece los métodos puros `parseVersion()`, `requiredModules()`, `validateProfile()`, `validateRuntimeEnvironment()`, `formatOutcome()` y `formatReport()`.
  - Opera con total independencia de las bibliotecas de Qt y de los componentes de widgets, permitiendo pruebas unitarias deterministas en entornos headless.
- En `apps/vigilio-browser/src/app.cpp`:
  - El punto de entrada principal invoca `vigilio::QtDependencyValidator::validateRuntimeEnvironment(qVersion(), QT_VERSION_STR, display, wayland)`.
  - Si la validación no resulta en `Satisfied`, el proceso aborta con un mensaje estructurado en `stderr` y código de salida 1 antes de instanciar `QApplication` o `BrowserWindow`.
- En `apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`:
  - La ventana principal está construida sobre Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
  - Implementa la navegación básica, pestañas diferidas (lazy loading de páginas para optimizar RAM), favoritos locales en SQLite y soporte para «Interfaz más fácil de usar» (conmutación de fuentes a 14pt y controles con alturas mínimas de 48px y 55px).
  - Cumple estrictamente con la política de textos de `docs/UX-COPY-PUBLICO.md`: no expone términos técnicos internos como «Senior Mode» o «Modo Senior».
- En `apps/vigilio-browser/CMakeLists.txt`:
  - Incorpora la opción `VIGILIO_REQUIRE_QT` para gobernar la obligatoriedad de dependencias de Qt6.
  - Detecta de forma granular los módulos `Qt6::Core`, `Qt6::Widgets` y `Qt6::WebEngineWidgets` exigiendo versión >= 6.10.0.
  - Ofrece degradación elegante a modo headless (`VIGILIO_BUILD_GUI=OFF`) cuando `VIGILIO_REQUIRE_QT=OFF`, o fallo fatal con diagnóstico explicativo cuando `VIGILIO_REQUIRE_QT=ON`.
  - Inmuniza los directorios de inclusión de Qt marcándolos como `SYSTEM` y habilita `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` para targets de interfaz.
- En la suite de pruebas de UI (`apps/vigilio-browser/tests/ui/WindowTests.cpp`):
  - Verifica la inicialización de la ventana, controles de navegación y conteo de pestañas cuando `Qt6::Test` está presente.

### Estado objetivo (TARGET)
- Una especificación formal y gobernanza rigurosa de la etapa **ui** para **Qt dependency validation** que certifique:
  1. **Declaración contractual de UX/UI y «sin cambio visual directo»**:
     - La validación de dependencias Qt es infraestructura interna de arranque y construcción; por ende, no introduce componentes gráficos, toolbars, badges de versión ni diálogos innecesarios en la interfaz del usuario común.
     - Cumplimiento incondicional de `docs/UX-COPY-PUBLICO.md`: cero exposición de conceptos como «Qt», «Qt6», «QPA», «WebEngine», «CMake», «Senior Mode», «Modo Senior», «DAG» o «OpenSpec».
  2. **Interacción y robustez en la fase de inicialización gráfica**:
     - Si el entorno gráfico o las dependencias fallan en runtime (ausencia de servidor X11/Wayland o incompatibilidad ABI), la aplicación nunca invoca llamadas a widgets ni provoca caídas de segmentación (`SIGSEGV`); en su lugar, termina de forma determinista emitiendo un diagnóstico claro en `stderr` con código 1.
     - Si la validación es satisfactoria, la ventana `BrowserWindow` se despliega en menos de 100 ms tras el pre-flight check, garantizando que el validador no agregue latencia perceptible al arranque.
  3. **Preservación incondicional de la «Interfaz más fácil de usar» (Senior Mode)**:
     - Las garantías de accesibilidad definidas en `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md` se mantienen íntegras: tap targets mínimos de 48px/55px, tipografía legible de 14pt (escalable a 18pt), foco de teclado visible con bordes de alto contraste y soporte de lectores de pantalla.
  4. **Gobernanza de pruebas de UI y registro honesto de dependencias**:
     - Las pruebas de UI se compilan y ejecutan condicionalmente; en entornos sin servidor de pantalla o sin paquetes de desarrollo Qt, se registra transparentemente el estado `NOT_EXECUTED` sin simular falsos positivos con mocks artificiales.
  5. **Compatibilidad headless estricta**:
     - El proyecto continúa compilando limpiamente con `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`), aprobando el 100% de las pruebas unitarias y de integración de persistencia sin requerir paquetes gráficos.

### Brechas identificadas (GAPS)
- **Formalización documental de contratos visuales**: faltaba documentar formalmente en OpenSpec que la validación de dependencias Qt no debe agregar controles de usuario en la UI visible y definir el comportamiento observable ante fallos de runtime.
- **Auditoría de cadenas públicas en UI**: certificar que `BrowserWindow.cpp` no contenga residuos de terminología interna ni menciones a versiones de Qt.
- **Estandarización de reporte de ejecución**: establecer la clasificación trazable de pruebas gráficas (`NOT_EXECUTED` cuando falten bibliotecas de desarrollo de Qt6 en el entorno actual).

## Compatibility, migration and rollback

- **Compatibilidad**:
  - Estándar del lenguaje: C++20 estándar (`-std=c++20`).
  - Framework de UI: Qt Widgets 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
  - Compiladores soportados: GNU GCC >= 11, LLVM Clang >= 14, AppleClang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Sistemas operativos: Linux (x86_64, aarch64), Windows 10/11 x64, macOS 12+ (x64 y Apple Silicon).
  - Modo headless: soporte garantizado mediante `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) en entornos sin servidor gráfico.
- **Migración**:
  - No requiere migraciones de base de datos ni modificaciones de esquema SQLite (`browser.sqlite`).
  - No altera firmas de métodos ni contratos de `vigilio_core` o `vigilio_storage`.
- **Rollback**:
  - En esta fase PROPOSE, la reversión se limita a descartar los archivos bajo `openspec/changes/module-foundations-task0011/`.
  - En la fase APPLY, la reversión se efectúa restaurando la versión previa de los archivos mediante control de versiones.

## Arquitectura y flujo de validación en UI

El flujo de ejecución desde el punto de entrada de la aplicación hasta la visualización de la ventana principal se rige por la siguiente secuencia determinista:

```
+-------------------------------------------------------------------------------+
|                            Punto de Entrada (main en app.cpp)                 |
|  1. Inspecciona variables de entorno: DISPLAY, WAYLAND_DISPLAY                |
|  2. Obtiene versión compilada (QT_VERSION_STR) y runtime (qVersion())         |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
|             vigilio::QtDependencyValidator::validateRuntimeEnvironment         |
|  - Verifica consistencia de versiones mayores (prevención ABI mismatch)       |
|  - Verifica presencia de display gráfico en plataformas Linux                  |
+-------------------------------------------------------------------------------+
                                        |
                 +----------------------+----------------------+
                 |                                             |
   [Outcome != Satisfied]                        [Outcome == Satisfied]
                 v                                             v
+----------------------------------+          +----------------------------------+
|  Diagnóstico en std::cerr        |          |  Inicializa QApplication(argc,v) |
|  Salida limpia: return 1         |          |  Configura metadatos y patrones  |
|  (CERO crashes, CERO widgets)    |          |  de log estructurados            |
+----------------------------------+          +----------------------------------+
                                                               |
                                                               v
                                              +----------------------------------+
                                              |  Instancia BrowserWindow window  |
                                              |  - setupUi() (Qt Widgets)        |
                                              |  - Carga ajustes locales         |
                                              |  - «Interfaz más fácil de usar»  |
                                              |  - Sin widgets técnicos de Qt    |
                                              +----------------------------------+
                                                               |
                                                               v
                                              +----------------------------------+
                                              |  window.show()                   |
                                              |  return app.exec()               |
                                              +----------------------------------+
```

### Invariantes del flujo
1. **Separación de responsabilidades**: `QtDependencyValidator` pertenece al dominio de `vigilio_core` y no efectúa llamadas a la interfaz gráfica ni conoce las clases de widgets.
2. **Pre-flight seguro**: Ningún objeto `QApplication`, `QWidget` o `BrowserWindow` es creado si la validación previa de entorno falla.
3. **Presupuesto temporal**: La validación previa toma menos de 2 milisegundos, operando exclusivamente sobre inspecciones en memoria de cadenas y variables de entorno locales.

## Contrato de interacción de interfaz y estados visuales

### 1. Ausencia formal de cambio visual directo
En estricto cumplimiento de `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md` y `docs/UX-COPY-PUBLICO.md`:
- La capacidad **Qt dependency validation** no introduce botones, barras, badges de estado ni menús de depuración en la ventana principal del navegador.
- Los usuarios finales navegan por la web sin distracciones técnicas; el navegador no les expone versiones de bibliotecas, compatibilidad de módulos ni arquitecturas de compilación.
- La barra de estado de la ventana principal muestra exclusivamente mensajes públicos orientados al usuario:
  `"Vigilio Browser · Datos locales · Sin cuenta"`
  `"Interfaz fácil activada"` / `"Interfaz fácil desactivada"`
  `"Página cargada"` / `"No se pudo cargar la página"`

### 2. Estados visuales observables del navegador

| Estado | Comportamiento en `BrowserWindow` | Retroalimentación al usuario | Accesibilidad / Lector de pantalla |
| --- | --- | --- | --- |
| **Idle / Normal** | Ventana visible con pestañas, barra de navegación y página de inicio local (`newHomePage`). | Aspecto sobrio con tokens de diseño claro u oscuro; iconos SVG Lucide de 24x24 px. | Controles navegables con tabulador; foco resaltado en azul neutro. |
| **Loading** | Pestaña en proceso de carga web o restauración diferida. | Título de pestaña muestra «Cargando…» o «En espera»; indicador en barra de estado. | Anunciado como elemento en carga; no bloquea interacción con otras pestañas. |
| **Empty** | Nueva pestaña sin dirección introducida. | Muestra `newHomePage` con título «Vigilio Browser», lema «Navega. Organiza. Protege.» y campo de búsqueda centrado. | Campo de texto con `accessibleName = "Buscar desde la nueva pestaña"`. |
| **Error de navegación** | Dirección web inválida o esquema rechazado por `UrlInput`. | Diálogo estándar `QMessageBox::warning` con título «Dirección no permitida» y mensaje en lenguaje natural. | Diálogo accesible modal que devuelve el foco a la barra de direcciones al cerrarse. |
| **Fallo de runtime Qt** | Ausencia de display gráfico o incompatibilidad de bibliotecas en arranque. | Cero interfaz gráfica (prevención de `SIGSEGV`); mensaje explicativo estructurado en `std::cerr`. | Salida accesible en consola de terminal estándar para administradores o scripts. |

## Contrato de accesibilidad y «Interfaz más fácil de usar»

Conforme a las directrices de `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`, y referenciando `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`:
1. **Terminología pública obligatoria**:
   - En el menú principal, la opción se rotula con la etiqueta aprobada: **«Interfaz más fácil de usar»**.
   - Queda terminantemente prohibido mostrar «Senior Mode», «Modo Senior», «prompt», «skill» o «micro-prompt» en la interfaz visible.
2. **Dimensiones y ergonomía de controles**:
   - Campos de entrada de texto (`QLineEdit`): altura mínima de 48 px en modo fácil (frente a 32 px en modo estándar).
   - Barras de herramientas (`QToolBar`): altura mínima de 55 px en modo fácil (frente a 40 px en modo estándar).
   - Tipografía global de la aplicación: escala de 10 pt a 14 pt en modo fácil.
3. **Navegación por teclado y foco visible**:
   - Atajos estándar integrados: `Ctrl+T` (nueva pestaña), `Ctrl+W` (cerrar pestaña), `Ctrl+L` (enfocar barra de direcciones), `Ctrl+D` (guardar favorito).
   - El foco del teclado se indica claramente mediante contornos de alto contraste en campos de texto y elementos de lista.
4. **Tecnologías asistivas**:
   - Todos los controles clave cuentan con `setAccessibleName`: «Barra de direcciones y búsqueda», «Buscar desde la nueva pestaña», «Navegación».

## Aislamiento de capas y gobernanza de dependencias

La gobernanza de dependencias garantiza el desacoplamiento estricto entre el motor de validación, la persistencia y la capa de presentación:

```
+-------------------------------------------------------------------------------+
|                                apps/vigilio-browser                           |
|                                                                               |
|   +------------------------+      +--------------------+      +-----------+   |
|   |   src/ui/BrowserWindow | ---> | src/storage/       | ---> | SQLite3   |   |
|   |   (Qt Widgets / UI)    |      | LocalRepository    |      | (Local)   |   |
|   +------------------------+      +--------------------+      +-----------+   |
|               |                             |                                 |
|               v                             v                                 |
|   +------------------------+      +--------------------+                      |
|   | src/core/              |      | src/core/          |                      |
|   | QtDependencyValidator  |      | UrlInput, TabModel |                      |
|   | (C++20 PUID - SIN Qt)  |      | (C++20 Dominio)    |                      |
|   +------------------------+      +--------------------+                      |
+-------------------------------------------------------------------------------+
```

1. **`vigilio_core`**: Biblioteca estática de dominio C++20 puro. No incluye cabeceras de Qt ni enlaza con bibliotecas gráficas. `QtDependencyValidator` puede ser probado en cualquier entorno sin servidor gráfico.
2. **`vigilio_storage`**: Capa de persistencia aislada basada en SQLite3 local.
3. **`vigilio-browser`**: Target ejecutable que enlaza `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y las bibliotecas gráficas validadas de Qt6 (`Core`, `Widgets`, `WebEngineWidgets`).
4. **Cabeceras de terceros como `SYSTEM`**: Todas las inclusiones de Qt se declaran con `SYSTEM` en CMake, impidiendo que diagnósticos de terceros activen `-Werror`.

## Gestión de fallos de entorno gráfico y degradación

### 1. Detección temprana en `app.cpp`
```cpp
const char* display = std::getenv("DISPLAY");
const char* wayland = std::getenv("WAYLAND_DISPLAY");

const auto report = vigilio::QtDependencyValidator::validateRuntimeEnvironment(
    qVersion(), QT_VERSION_STR, display, wayland);

if (report.outcome != vigilio::QtValidationOutcome::Satisfied) {
    std::cerr << "[Vigilio Startup Error] " << report.diagnosticMessage << "\n";
    return 1;
}
```

### 2. Políticas de degradación en tiempo de compilación (`CMakeLists.txt`)
- Si `VIGILIO_REQUIRE_QT=OFF` (desarrollo local / CI headless):
  - CMake emite un aviso informativo detallando qué componentes faltan (`Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`).
  - Omite la construcción del ejecutable de escritorio y del arnés de pruebas de UI.
  - Continúa compilando limpiamente el núcleo `vigilio_core`, la persistencia `vigilio_storage` y sus pruebas unitarias e integración.
- Si `VIGILIO_REQUIRE_QT=ON` (pipelines de empaquetado y release oficial):
  - CMake aborta la configuración con `message(FATAL_ERROR ...)` indicando los componentes faltantes y la versión mínima requerida (>= 6.10.0).

## Matriz detallada de pruebas de UI y trazabilidad

| Requisito / Criterio | Tipo de prueba | Archivo / Arnés | Comando de verificación | Resultado esperado | Fallo negativo / Tratamiento |
| --- | --- | --- | --- | --- | --- |
| Lógica de validación de dependencias | Unitaria (APPLY) | `tests/unit/CoreTests.cpp` | `ctest -R core_units` | PASS (100% pruebas de versiones y perfiles) | Falla si versión < 6.10.0 es aceptada |
| Interacción y controles de `BrowserWindow` | UI Widgets (APPLY) | `tests/ui/WindowTests.cpp` (`vigilio_ui_tests`) | `ctest -R window_ui_tests` | PASS cuando Qt6 esté presente; NOT_EXECUTED si no hay Qt | Falla si controles requeridos faltan |
| Ausencia de jerga técnica en cadenas visibles | Auditoría UX (APPLY) | `src/ui/BrowserWindow.cpp` | `! grep -E -i "Senior Mode|Modo Senior|compiler warnings|Qt" src/ui/BrowserWindow.cpp` | Cero coincidencias en texto visible | Falla si se introduce terminología técnica prohibida |
| Ergonomía de «Interfaz más fácil de usar» | Inspección UI (APPLY) | `src/ui/BrowserWindow.cpp` | Inspección de `setSeniorMode()` | Altura >= 48px en QLineEdit, >= 55px en QToolBar, fuente 14pt | Falla si dimensiones son inferiores a 48px |
| Degradación limpia en modo headless | Integración build (APPLY) | `apps/vigilio-browser/scripts/check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | PASS (compilación limpia bajo `-Werror` y 100% tests headless) | Falla si el build headless depende de Qt |
| Recorrido completo E2E del navegador | E2E crítico (LATER) | `tests/e2e/BrowserSmoke.cpp` | `ctest -R browser_e2e_smoke` | PASS serial en entorno con display y Qt WebEngine | NOT_EXECUTED en entornos sin GPU/display |
| Navegación por teclado y accesibilidad | Inspección UI (APPLY) | `src/ui/BrowserWindow.cpp` | Inspección de atajos y `accessibleName` | Atajos `Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D` operativos | Falla si faltan atajos básicos |
