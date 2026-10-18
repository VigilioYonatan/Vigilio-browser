# Design — module-foundations-task0015

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Infraestructura de validación de dominio (`src/core/WebEngineVersionValidator.h` y `.cpp`)**:
   - Implementado en `module.foundations.task0014` dentro de la biblioteca estática `vigilio_core`.
   - Provee las entidades inmutables de C++20:
     - `ChromiumVersion`: versión cuatripartita (`major.minor.build.patch`) con spaceship operator (`<=>`), parsing robusto y comprobación de hitos (`milestone()`).
     - `WebEngineFeature`: enum class con las 8 capacidades funcionales del motor (`ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`).
     - `WebEngineCompatibilityProfile`: perfiles declarativos `FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`.
     - `WebEngineCompatibilityStatus`: estados deterministas `Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `FeatureUnsupported`, `EnvironmentMisconfigured`.
     - `WebEngineCompatibilityReport`: reporte inmutable con estado, versiones detectadas, características soportadas/faltantes, ruta del asistente y acción recomendada.
   - Servicio puro `WebEngineVersionValidator`: funciones estáticas sin dependencias de cabeceras de `Qt6::WebEngineWidgets` ni Chromium, con suite de pruebas unitarias al 100% en CTest (`webengine_version_validator_tests`).
2. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Ejecuta la validación de dependencias de Qt (`QtDependencyValidator::validateRuntimeEnvironment`).
   - Ejecuta la pre-validación de WebEngine (`WebEngineVersionValidator::validateStartup`).
   - Ante incompatibilidad de motor o ausencia del binario auxiliar `QtWebEngineProcess`, emite un diagnóstico estructurado a `stderr` con la acción de remediación y finaliza limpiamente con código de salida 1, impidiendo la instanciación de ventanas en blanco o bloqueos silenciosos.
3. **Capa visual de interfaz de usuario (`src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`)**:
   - Construida sobre Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
   - La navegación abre pestañas (`openTab`): si la URL está vacía, muestra la página de inicio nativa (`newHomePage()`); si tiene URL, crea una instancia de `QWebEngineView`.
   - En `loadFinished`, actualmente solo emite un mensaje efímero en la barra de estado: `statusBar()->showMessage(ok ? "Página cargada" : "No se pudo cargar la página", 3000);`.
   - Si la carga falla (`ok == false`), la vista web permanece en blanco o con el visor nativo de Chromium sin ofrecer al usuario un mecanismo directo, accesible y estructurado de recuperación.
   - Implementa carga diferida de pestañas en `restoreSession` mediante placeholders ligeros con la propiedad `deferredUrl`, inicializando la vista web solo cuando el usuario selecciona la pestaña (`activateDeferredTab`).
   - Dispone del conmutador de «Interfaz más fácil de usar» (`setSeniorMode`), escalando fuentes a 14pt y controles a 48px/55px.
4. **Configuración de construcción (`CMakeLists.txt`) y scripts de verificación**:
   - Compilación con C++20 y `-Werror` mediante `vigilio_compiler_warnings`.
   - Suite de pruebas de UI `vigilio_ui_tests` condicionada a la disponibilidad de `Qt6::Test` y `VIGILIO_BUILD_GUI`.
   - `scripts/check.sh` valida la compilación Ninja y 5/5 pruebas en CTest en 0.14s con exit code 0.

### Estado objetivo (TARGET)

1. **Especificación formal y gobernanza de la interacción Qt Widgets con WebEngine**:
   - Definición exhaustiva del ciclo de vida de pestañas y estados visuales:
     - `empty`: pestaña local inicial sin instancias WebEngine.
     - `loading`: retroalimentación inmediata en pestaña y barra de estado («Cargando…», «Cargando página…»).
     - `success`: actualización de título de pestaña (hasta 30 caracteres), URL canónica en la barra de direcciones, aviso en barra de estado («Página cargada») y persistencia local en SQLite.
     - `error`: sustitución visual por un contenedor nativo de error accesible en Qt Widgets con mensaje en español («No se pudo cargar la página»), botón primario («Reintentar») y secundario («Ir al inicio»), con foco de teclado visible y soporte para tecnologías asistivas.
     - `deferred`: retención de pestañas inactivas en marcadores ligeros hasta su activación explícita por el usuario.
2. **Conformidad estricta con el lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`)**:
   - Ningún término interno («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «renderProcessTerminated», «Modo Senior», «Senior Mode», «Prompt Builder», «OpenSpec») se muestra en la interfaz gráfica.
   - Uso riguroso de etiquetas comprensibles: «Interfaz más fácil de usar», «No se pudo cargar la página», «Reintentar», «Ir al inicio», «Cargar pestañas cuando las necesites».
3. **Integración profunda con la «Interfaz más fácil de usar» (Senior Mode)**:
   - Escalado táctil y visual: campos de entrada y botones con altura mínima de 48px, barras de herramientas de 55px, tipografía en 14pt (escalable a 18pt) y contraste WCAG 2.2 nivel AA.
   - Navegación por teclado unificada: atajos canónicos (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+R`, `F5`) y anillos de foco visibles de alto contraste.
4. **Preservación incondicional de la compilación y pruebas headless**:
   - La opción `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) se mantiene 100% funcional y autosuficiente para CI y contenedores mínimos.
   - En entornos gráficos sin display disponible, las pruebas de UI se reportan honestamente como `NOT_EXECUTED` sin mocks falsos.

### Brechas identificadas (GAPS)

1. **Ausencia de contenedor nativo de recuperación ante errores de carga**: actualmente `BrowserWindow.cpp` ante `loadFinished(false)` solo escribe en la barra de estado, dejando la vista web en un estado indeterminado o en blanco sin botones de reintento.
2. **Falta de formalización de la máquina de estados de pestañas en OpenSpec**: no existía la documentación canónica que vincule la compatibilidad de WebEngine con los estados visuales en Qt Widgets.
3. **Auditoría y formalización de accesibilidad en estados de error**: asegurar que el contenedor de recuperación cumpla las directrices de `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.

---

## Compatibility, migration and rollback

### Compatibilidad

- **Estándar del lenguaje**: C++20 estándar (`-std=c++20`, uso de `<compare>`, `<string_view>`, `<optional>`, RAII).
- **Framework de interfaz**: Qt Widgets 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
- **Compiladores certificados**: GNU GCC >= 11, LLVM Clang >= 14, AppleClang >= 14, MSVC >= 19.30 (Visual Studio 2022).
- **Sistemas operativos soportados**:
  - Linux: x86_64, aarch64 (glibc >= 2.31, display X11 / Wayland).
  - Windows: Windows 10/11 x64.
  - macOS: macOS 12+ (x64 y Apple Silicon).
- **Modo headless y CI**: soporte garantizado mediante `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) sin dependencias de servidor gráfico ni bibliotecas WebEngine.

### Migración

- **Persistencia local**: no requiere modificaciones en el esquema SQLite (`browser.sqlite`); las páginas con error de carga no se escriben en la base de datos local, evitando contaminación de historial.
- **Configuración de usuario (`QSettings`)**: retrocompatible con las claves existentes `accessibility/senior`, `appearance/dark` y `session/urls`.
- **APIs de dominio**: no se alteran firmas de métodos ni estructuras de `vigilio_core` o `vigilio_storage`.

### Rollback

- **Fase PROPOSE**: la reversión se limita a descartar los archivos bajo `openspec/changes/module-foundations-task0015/`.
- **Fase APPLY**: la reversión se efectúa restaurando la versión previa de los archivos en `apps/vigilio-browser/src/ui/` mediante el control de versiones.

---

## Arquitectura técnica de interacción UI y WebEngine

### Ciclo de vida y máquina de estados visuales de pestañas

```
                     +----------------------------------+
                     |         Pestaña Creada           |
                     +----------------------------------+
                                      |
                 +--------------------+--------------------+
                 |                                         |
         [URL vacía]                               [URL presente]
                 v                                         v
   +---------------------------+             +---------------------------+
   |   Estado EMPTY            |             |   Estado LOADING          |
   |   newHomePage()           |             |   view->load(url)         |
   |   - Buscador local        |             |   - Pestaña: "Cargando…"  |
   |   - Cero instancias web   |             |   - Barra: "Cargando…"    |
   +---------------------------+             +---------------------------+
                 |                                         |
         [Usuario introduce URL]                           |
                 +--------------------+--------------------+
                                      |
                       +--------------+--------------+
                       |                             |
               [loadFinished(true)]          [loadFinished(false)]
                       v                             v
         +---------------------------+ +---------------------------+
         |   Estado SUCCESS          | |   Estado ERROR            |
         |   - Título resuelto       | |   - Contenedor nativo Qt  |
         |   - URL en barra dir      | |   - "No se pudo cargar"   |
         |   - Registro en SQLite    | |   - Botón "Reintentar"    |
         |   - Barra: "Página cargada" | - Botón "Ir al inicio"   |
         +---------------------------+ +---------------------------+
                       |                             |
                       |                     +-------+-------+
                       |                     |               |
                       |              ["Reintentar"]  ["Ir al inicio"]
                       |                     |               |
                       +---------------------+               v
                                 |             +---------------------------+
                          [Nueva URL]          | Regresa a Estado EMPTY    |
                                 v             +---------------------------+
                     +-----------------------+
                     | Pasa a Estado LOADING |
                     +-----------------------+
```

### Protocolo de pestañas diferidas (`deferred`) en restauración de sesión

```
+---------------------------------------------------------------------------------+
|                    restoreSession() en BrowserWindow                           |
|  - Carga lista de URLs persistidas en QSettings ("session/urls")                |
|  - Primera URL válida: se abre con openTab(url) -> instanciación activa         |
|  - URLs restantes (hasta 100): se crean como QWidget placeholder ligero         |
|    * Propiedad "deferredUrl" asignada con la URL original                       |
|    * Etiqueta descriptiva: "Pestaña en espera. Se cargará al abrirla."          |
|    * Cero instancias de QWebEngineView creadas en segundo plano                 |
+---------------------------------------------------------------------------------+
                                         |
                                         | [Usuario selecciona pestaña diferida]
                                         v
+---------------------------------------------------------------------------------+
|                 activateDeferredTab(index) en BrowserWindow                     |
|  1. Bloquea señales temporalmente (QSignalBlocker)                              |
|  2. Extrae target = oldWidget->property("deferredUrl")                          |
|  3. Elimina placeholder con deleteLater()                                       |
|  4. Invoca openTab(target) creando el QWebEngineView correspondiente           |
|  5. Reubica la pestaña a su índice original en QTabBar                          |
+---------------------------------------------------------------------------------+
```

---

## Contratos específicos de presentación y widgets

### 1. Contenedor nativo de recuperación ante error (`createErrorPageWidget`)

Cuando una vista web no logra cargar (`loadFinished(false)`) o el motor de renderizado se desconecta, se construye dinámicamente un widget nativo `QWidget` con la siguiente estructura:

- **Identificador de objeto**: `setObjectName("vigilio-page-error")`.
- **Disposición (`QVBoxLayout`)**: centrado vertical y horizontalmente con espaciado de 16px (24px en modo accesible).
- **Título (`QLabel`)**:
  - Texto: `"No se pudo cargar la página"`.
  - Identificador: `setObjectName("vigilio-error-title")`.
  - Tipografía: tamaño 20pt (24pt en modo accesible), peso `QFont::DemiBold`.
  - Accesibilidad: etiqueta anunciada como encabezado principal.
- **Mensaje de orientación (`QLabel`)**:
  - Texto: `"Comprueba la dirección web o la conexión del equipo."`.
  - Identificador: `setObjectName("vigilio-error-message")`.
  - Tipografía: tamaño 11pt (14pt en modo accesible), color secundario amortiguado.
- **Fila de acciones interactivas (`QHBoxLayout`)**:
  - **Botón Primario — Reintentar (`QPushButton`)**:
    - Texto: `"Reintentar"`.
    - Identificador: `setObjectName("vigilio-error-retry")`.
    - Altura mínima: 36px (48px en modo accesible).
    - Ancho mínimo: 120px (160px en modo accesible).
    - Atajo asociado: tecla `Enter` o `Ctrl+R`.
    - Conexión: invoca `view->reload()` y devuelve la vista web al contenedor de la pestaña.
    - Accesibilidad: `setAccessibleName("Reintentar la carga de la página")`, foco inicial predeterminado.
  - **Botón Secundario — Ir al inicio (`QPushButton`)**:
    - Texto: `"Ir al inicio"`.
    - Identificador: `setObjectName("vigilio-error-home")`.
    - Altura mínima: 36px (48px en modo accesible).
    - Ancho mínimo: 120px (160px en modo accesible).
    - Conexión: sustituye la pestaña actual por `newHomePage()`.
    - Accesibilidad: `setAccessibleName("Volver a la página de inicio")`.

### 2. Contrato de estilos CSS/QSS y tokens visuales

```css
/* Estado de Error Nativo en Tema Claro */
QWidget#vigilio-page-error {
    background-color: #F7F9FC;
    color: #192538;
}
QLabel#vigilio-error-title {
    color: #192538;
    font-size: 20px;
    font-weight: 600;
}
QLabel#vigilio-error-message {
    color: #5A6A80;
    font-size: 14px;
}
QPushButton#vigilio-error-retry {
    background-color: #2563EB;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 8px 20px;
    font-weight: 500;
}
QPushButton#vigilio-error-retry:hover {
    background-color: #1D4ED8;
}
QPushButton#vigilio-error-retry:focus {
    outline: 2px solid #2563EB;
    outline-offset: 2px;
}
QPushButton#vigilio-error-home {
    background-color: #FFFFFF;
    color: #192538;
    border: 1px solid #D9E0EA;
    border-radius: 6px;
    padding: 8px 20px;
}
QPushButton#vigilio-error-home:hover {
    background-color: #F1F4F9;
}

/* Estado de Error Nativo en Tema Oscuro */
QWidget#vigilio-page-error {
    background-color: #141D2B;
    color: #E9EDF5;
}
QLabel#vigilio-error-title {
    color: #E9EDF5;
    font-size: 20px;
    font-weight: 600;
}
QLabel#vigilio-error-message {
    color: #94A3B8;
    font-size: 14px;
}
QPushButton#vigilio-error-retry {
    background-color: #294F89;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 8px 20px;
}
QPushButton#vigilio-error-retry:hover {
    background-color: #3B82F6;
}
QPushButton#vigilio-error-home {
    background-color: #222D3E;
    color: #FFFFFF;
    border: 1px solid #394557;
    border-radius: 6px;
    padding: 8px 20px;
}
QPushButton#vigilio-error-home:hover {
    background-color: #2D3B50;
}
```

### 3. Adaptación reactiva a la «Interfaz más fácil de usar» (Senior Mode)

Cuando el usuario activa la «Interfaz más fácil de usar»:
1. **Dimensionamiento de controles**:
   - Campos de texto (`QLineEdit`): `minimumHeight = 48px`.
   - Barras de herramientas (`QToolBar`): `minimumHeight = 55px`.
   - Botones interactivos (`QPushButton`): `minimumHeight = 48px`.
   - Pestañas (`QTabBar`): altura mínima de 44px con padding vertical ampliado.
2. **Escalado tipográfico**:
   - `QFont font = qApp->font(); font.setPointSize(14); qApp->setFont(font);`
   - Los textos de error (`vigilio-error-title`) escalan a 24pt y el subtítulo a 16pt.
3. **Alto contraste y foco**:
   - Los anillos de foco adquieren un contorno grueso de 2px en color de alto contraste (`#2563EB` en tema claro, `#60A5FA` en tema oscuro) para garantizar una visibilidad inequívoca durante la navegación por teclado.
4. **Mensaje de estado accesible**:
   - La barra de estado anuncia: `"Interfaz fácil activada"` durante 3000 ms.

---

## Presupuestos de rendimiento, memoria y procesos auxiliares

1. **Presupuesto de latencia en UI**:
   - Renderizado del contenedor de error o nueva pestaña: < 10 ms (sin operaciones de I/O de disco ni llamadas síncronas bloqueantes).
   - Conmutación entre pestañas activas: < 16 ms (garantizando 60 fps en la interfaz).
   - Invocación de la «Interfaz más fácil de usar»: refresco reactivo instantáneo en menos de 20 ms.
2. **Presupuesto de memoria RAM**:
   - Huella base del proceso de interfaz con 1 pestaña activa: < 120 MB RAM.
   - Pestaña diferida (`deferred`): < 15 KB de RAM por pestaña inactiva en lugar de ~30–50 MB que consumiría una instancia viva de WebEngine.
   - 100 pestañas restauradas en espera: consumo acumulado inferior a 150 MB RAM total.
3. **Prevención de procesos zombies y fugas**:
   - Cero procesos Chromium auxiliares creados mientras el usuario permanezca en la página de inicio o en pestañas diferidas.
   - Destrucción inmediata de páginas con `deleteLater()` al cerrar pestañas, liberando sockets IPC y memoria compartida.

---

## Estrategia de pruebas de UI y trazabilidad en CTest

Las pruebas de interacción de interfaz se organizan en `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
- **Casos cubiertos con `QTest`**:
  - `testEmptyHomePageCreation`: verificación de que una nueva pestaña inicia con `newHomePage` y sin vista web activa.
  - `testLoadingAndSuccessStates`: validación de la actualización de títulos y texto de pestaña al navegar.
  - `testErrorContainerCreation`: inyección o simulación de `loadFinished(false)` y comprobación de que el widget de error se construye con botones "Reintentar" e "Ir al inicio".
  - `testSeniorModeScaling`: verificación de que al invocar `setSeniorMode(true)` todos los controles hijos `QLineEdit` y `QPushButton` cumplen con `minimumHeight() >= 48`.
  - `testDeferredTabActivation`: verificación de que un widget placeholder contiene la propiedad `deferredUrl` y se reemplaza limpiamente al seleccionarse.
- **Gobernanza de entorno**:
  - Si el ejecutable de pruebas se invoca sin display gráfico ni servidor virtual (`xvfb`), se registra de forma honesta y limpia `NOT_EXECUTED` sin abortos de memoria ni aserciones artificiales.
