# Change Proposal — module-foundations-task0023

Pack: `module.foundations.task0023`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **startup bootstrap** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0023 — Implementar la interacción y estados visuales en Qt: startup bootstrap`).

Tras haber formalizado en `module.foundations.task0021` la especificación del modelo de dominio, las 11 fases discretas del ciclo de vida y los invariantes de seguridad, e implementado en `module.foundations.task0022` el coordinador desacoplado en C++20 (`vigilio::StartupBootstrapCoordinator`), la pre-validación de arranque y el soporte para entornos headless, esta microtarea proyecta la **construcción de la interacción Qt Widgets, estados visuales (vacío, cargando, error, modo seguro, incógnito), jerarquía visual, navegación por teclado, tecnología asistiva y adaptación para «Interfaz más fácil de usar» (Modo Senior)**, conectando de forma limpia, reactiva y accesible el resultado del bootstrap con la interfaz gráfica de usuario.

El alcance concreto de este pack comprende:
- **Integración del contexto de arranque en la ventana principal (`BrowserWindow`)**:
  - Extensión del constructor de `BrowserWindow` para aceptar opcionalmente las estructuras inmutables `StartupCommandLine` y `StartupDiagnosticReport`, preservando un constructor por defecto para retrocompatibilidad en tests unitarios.
  - Apertura automática de URLs especificadas en la línea de comandos (`cmdLine.urls`): si se proporcionan una o más URLs válidas en el arranque CLI, se instancian en pestañas independientes en lugar de forzar una página de inicio vacía.
  - Tratamiento visual de sesión privada de incógnito (`StartupMode::Incognito`):
    - Configuración del repositorio local (`LocalRepository`) en memoria (`":memory:"`) o aislamiento completo de persistencia en disco, garantizando que no se escriba historial, cookies ni datos de navegación en `browser.sqlite`.
    - Despliegue de un indicador visual discreto y accesible de «Navegación privada» (`incognitoBadge_`) en la barra de herramientas o barra de estado, con el icono vectorial `icons/tools/shield.svg`.
    - Asignación de título y descripción accesible indicando claramente al usuario y al lector de pantalla que la sesión es privada y no almacena datos locales.
  - Despliegue de la barra de aviso de modo seguro a prueba de fallos (`StartupMode::SafeMode`):
    - Inserción de un banner informativo nativo (`safeModeBanner_`) en la parte superior de la ventana principal con icono representativo, texto conforme a `docs/UX-COPY-PUBLICO.md` («Modo seguro activado: extensiones y configuraciones experimentales desactivadas para proteger la estabilidad») y botón accesible de descarte («Entendido»).
  - Resolución del directorio de perfil personalizado: utilización de la ruta validada y saneada `report.storagePath` para ubicar la base de datos `browser.sqlite`, respetando la defensa activa contra Directory Traversal.
- **Gestión integral de estados visuales de arranque (vacío, cargando, error)**:
  - *Estado vacío (`empty`)*: Preservación de la página de inicio nativa (`newHomePage()`) cuando no se especifican URLs ni se restaura sesión, desplegando el logotipo oficial `branding/vigilio-logo.svg`, barra de búsqueda centrada con foco inmediato (`address_->setFocus()`) y tarjetas de privacidad/rendimiento alineadas con `/resultado/01-vigilio-home.png`.
  - *Estado de carga (`loading`)*: Retroalimentación no bloqueante en el hilo de interfaz al resolver URLs iniciales o restaurar pestañas, indicando «Cargando…» en la pestaña y «Iniciando navegación…» en la barra de estado.
  - *Estado de error (`error`)*: Diálogo nativo accesible de error de arranque (`StartupErrorDialog` / `showStartupErrorDialog`):
    - Si el bootstrap experimenta un fallo irrecuperable en un entorno con servidor gráfico disponible (`DISPLAY` o `WAYLAND_DISPLAY`), la aplicación inicializa `QApplication` para presentar una ventana modal accesible con icono de alerta, título en lenguaje público («No se pudo iniciar Vigilio Browser»), explicación clara sin códigos técnicos internos y botones de acción («Reintentar», «Iniciar en modo seguro», «Cerrar»).
    - Si una URL inicial falla al cargar en la ventana principal, se activa el contenedor nativo de error (`createErrorPageWidget`) con opciones de reintento («Reintentar») y regreso («Ir al inicio»).
- **Accesibilidad integral y atajos universales de teclado**:
  - Asignación sistemática de `setAccessibleName` y `setAccessibleDescription` en español natural para todos los elementos interactivos del arranque (banner de modo seguro, indicador de incógnito, botones de error y barra de navegación).
  - Secuencia de tabulación lógica (`Tab` / `Shift+Tab`) asegurando que el foco visual de 2px sea inmediatamente perceptible.
  - Atajos de teclado canónicos de arranque: `Ctrl+L` (foco a barra de direcciones), `Ctrl+T` (nueva pestaña), `Ctrl+W` (cerrar pestaña), `Ctrl+R` / `F5` (recargar), `Ctrl+D` (marcador).
- **Adaptación dinámica para «Interfaz más fácil de usar» (Modo Senior)**:
  - Cumplimiento riguroso de `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`:
    - Áreas de interacción y clic ampliadas: altura mínima >= 48 px en botones de descarte, reintento y campos de texto; >= 55 px en barras de herramientas.
    - Escalado de tipografía a 14pt (escalable a 18pt) con espaciado generoso para evitar fatiga visual o toques accidentales.
    - Escalado de iconos vectoriales a 32×32 px en la barra de herramientas.
    - Prohibición absoluta de mostrar términos como «Senior Mode», «Modo Senior», «Prompt Builder», «bootstrap», «CLI», «DAG» u «OpenSpec» en la UI visible.
- **Suite de pruebas de interacción Qt UI (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
  - Ampliación de la suite de pruebas en CTest (`window_ui_tests`) con casos de verificación para:
    - Inicialización de `BrowserWindow` con reporte estándar y apertura de URLs pasadas por línea de comandos.
    - Inicialización en modo incógnito con aislamiento de almacenamiento y presencia del indicador privado.
    - Inicialización en modo seguro con presencia del banner informativo y botón de descarte.
    - Inicialización con directorio de perfil personalizado.
    - Escalado y contraste en «Interfaz más fácil de usar».
    - Tolerancia en entornos headless de CI: reporte limpio `NOT_EXECUTED` (exit code 0) en ausencia de servidor gráfico.

## Out of scope

- Pruebas E2E automatizadas multiproceso de Chromium o sesiones WebEngine pesadas de larga duración (diferidas a la etapa `tests` en `module.foundations.task0024`).
- Modificaciones al coordinador de dominio o servicio de bajo nivel (`StartupBootstrapCoordinator`), completadas y congeladas en `task0021` y `task0022`.
- Políticas de recuperación post-crash, journaling o restauración de pestañas cerradas abruptamente (pertenecientes a la capacidad `crash recovery policy`, microtareas `task0029`-`task0032`).
- Soporte para extensiones Manifest V3 o emulación de APIs de extensiones (reservado para el módulo `extensions`).
- Cualquier comunicación por red, comprobación de actualizaciones en la nube, autenticación o telemetría remota (terminantemente prohibido por la política local-first, decisión Q-001).
- Modificación directa de archivos de código fuente de producción bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida rigurosamente a `openspec/changes/module-foundations-task0023/**`).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código existente en `apps/vigilio-browser`, los módulos de dominio previos y las herramientas locales:
1. **Infraestructura de coordinación previa (`src/core/StartupBootstrapCoordinator.h`, `StartupBootstrapCoordinator.cpp`, `StartupTypes.h`)**:
   - Formalizada en `task0021` e implementada en `task0022` dentro de `vigilio_core`.
   - Provee las estructuras inmutables `StartupCommandLine` y `StartupDiagnosticReport`, y los métodos estáticos puros `parseCommandLine`, `executeBootstrap`, `isArgumentForbidden` e `isValidProfilePath`.
   - Soporta modos `StandardGui`, `Headless`, `SafeMode` e `Incognito`, reportando de forma determinista códigos de salida y mensajes estructurados.
2. **Punto de entrada de la aplicación (`src/app.cpp`)**:
   - Actualmente procesa los argumentos con `StartupBootstrapCoordinator::parseCommandLine` y ejecuta `executeBootstrap`.
   - Sin embargo, tras validar el reporte exitoso, instancia `BrowserWindow window;` sin transmitirle ni el reporte (`StartupDiagnosticReport`) ni los argumentos parseados (`StartupCommandLine`), ignorando las URLs iniciales, el modo incógnito, el modo seguro y la ruta de almacenamiento personalizada.
3. **Capa visual existente (`src/ui/BrowserWindow.h`, `BrowserWindow.cpp`)**:
   - Construida con Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
   - Implementa apertura de pestañas (`openTab`), página de inicio nativa (`newHomePage`), carga diferida de pestañas en restauración de sesión (`restoreSession`, `activateDeferredTab`), página nativa de error (`createErrorPageWidget`), iconografía vectorial QRC (`makeResourceIcon`) y conmutador de «Interfaz más fácil de usar» (`setSeniorMode`).
   - Requiere extender su inicialización para integrar el contexto de arranque, el banner de modo seguro, el badge de incógnito y el tratamiento de URLs iniciales.
4. **Suite de pruebas de UI (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
   - Valida la creación básica de pestañas, icono de ventana, estados visuales (`vigilio-home`, `vigilio-page-error`), accesibilidad asistiva, atajos de teclado y escalado en Senior Mode.
   - Requiere incorporar aserciones específicas para verificar los estados visuales y adaptaciones de arranque.
5. **Herramientas locales y verificación de calidad**:
   - Compilador: GNU GCC 15.2.0 (C++20, `-std=c++20`), CMake 4.2.3, Ninja 1.13.2, SQLite 3.46.1.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: compilación Ninja limpia bajo `-Werror` y 9/9 pruebas aprobadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`, `webengine_version_validator_tests`, `resource_qrc_catalog_tests`, `resource_inclusion_service_tests`, `resource_qrc_inclusion_tests`, `startup_bootstrap_tests`) en 0.19 segundos, exit code 0.
   - Prompt Builder CLI: `validate-references --project-root .` validó 23 referencias canónicas con estado `valid: true` (exit code 0); `verify-lock --lock-file generated/prompts/full-project/0067-module.foundations.task0023.propose.lock.json --project-root .` validó el 100% de las 13 comprobaciones (exit code 0); `validate-decisions` confirmó que las decisiones de foundations están resueltas.
6. **Referencias visuales y directrices de diseño**:
   - `/resultado/01-vigilio-home.png`: Ventana principal del navegador con sidebar, buscador central, pestañas sobrias y tarjetas de privacidad.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Panel de accesibilidad con «Interfaz más fácil de usar», controles amplios y alto contraste.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial del isotipo corporativo de Vigilio.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición total de tecnicismos internos; uso estricto de lenguaje comprensible para usuarios generales.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0023`.
- **Capacidad**: `startup bootstrap`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0023/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0023/proposal.md`
  - `openspec/changes/module-foundations-task0023/design.md`
  - `openspec/changes/module-foundations-task0023/tasks.md`
  - `openspec/changes/module-foundations-task0023/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0023/**`. No se autoriza modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la interacción Qt Widgets para **startup bootstrap** se organiza asegurando desacoplamiento y reactividad:

```mermaid
flowchart TD
    subgraph CLI ["Arranque y Coordinación (app.cpp)"]
        Arg[argc, argv] --> Parse["StartupBootstrapCoordinator::parseCommandLine"]
        Parse --> Exec["StartupBootstrapCoordinator::executeBootstrap"]
        Exec --> Report["StartupDiagnosticReport + StartupCommandLine"]
    end

    subgraph ErrorHandling ["Tratamiento de Error Previo a Ventana"]
        Report -->|!report.isSuccess() && Display Present| ErrorDialog["StartupErrorDialog (Modal Accesible)"]
        ErrorDialog -->|Reintentar| Parse
        ErrorDialog -->|Iniciar en Modo Seguro| SafeRestart["Forzar --safe-mode"]
        ErrorDialog -->|Cerrar| ExitProcess["Salir con report.exitCode"]
    end

    subgraph WindowInit ["Inicialización de BrowserWindow"]
        Report -->|report.isSuccess()| Ctor["BrowserWindow(cmdLine, report)"]
        Ctor --> StorageResolve{"¿Modo Incógnito o Perfil?"}
        StorageResolve -->|Incógnito| MemStorage["LocalRepository(':memory:')"]
        StorageResolve -->|Perfil Personalizado| CustomStorage["LocalRepository(report.storagePath)"]
        StorageResolve -->|Estándar| DefaultStorage["LocalRepository(profileLocation())"]
    end

    subgraph VisualStates ["Componentes y Estados Visuales"]
        Ctor --> CheckSafe{"¿Modo Seguro?"}
        CheckSafe -->|Sí| SafeBanner["safeModeBanner_ (Banner Superior)"]
        CheckSafe -->|No| NoSafeBanner["Sin banner"]
        
        Ctor --> CheckIncog{"¿Incógnito?"}
        CheckIncog -->|Sí| IncogBadge["incognitoBadge_ ('Navegación privada')"]
        CheckIncog -->|No| NormalTitle["Título Estándar"]

        Ctor --> CheckUrls{"¿URLs en CLI?"}
        CheckUrls -->|Sí| OpenUrls["openTab(url) para cada URL"]
        CheckUrls -->|No| CheckRestore{"¿Restaurar Sesión?"}
        CheckRestore -->|Pestañas > 0| RestoreTabs["restoreSession()"]
        CheckRestore -->|Pestañas == 0| NewHome["newHomePage() (Estado Vacío)"]
    end

    subgraph AccessibilitySenior ["Accesibilidad y Modo Senior"]
        SafeBanner --> SeniorAdapt["setSeniorMode: Tap targets >= 48px, texto 14pt, iconos 32px"]
        IncogBadge --> SeniorAdapt
        NewHome --> SeniorAdapt
    end
```

### Especificación de interfaces y responsabilidades

1. **Constructor parametrizado de `BrowserWindow`**:
   ```cpp
   // apps/vigilio-browser/src/ui/BrowserWindow.h
   explicit BrowserWindow(
       const vigilio::StartupCommandLine& cmdLine = {},
       const vigilio::StartupDiagnosticReport& report = {});
   ```
   - Precondiciones: Si `cmdLine.isValid` es falso, asume configuración por defecto.
   - Postcondiciones: La ventana queda instanciada en el modo operativo correspondiente (`StandardGui`, `SafeMode`, `Incognito`), con la persistencia aislada si corresponde, el banner desplegado si está en modo seguro, y las URLs iniciales cargadas.
   - Invariantes: La base de datos local nunca persiste en disco cuando `cmdLine.isIncognito` es verdadero.

2. **Banner visual de Modo Seguro (`safeModeBanner_`)**:
   - Widget: `QFrame` o `QWidget` horizontal con `objectName = "vigilio-safe-mode-banner"`.
   - Ubicación: Parte superior del layout central, sobre el contenedor de pestañas (`tabs_`).
   - Estilo: Fondo sutil azul oscuro atenuado con borde de contraste (1px solido `#2b3d5b`), sin brillos artificiales.
   - Icono: `icons/tools/shield.svg` resuelto vía `makeResourceIcon()`.
   - Etiqueta: `QLabel` con texto «Modo seguro activado: extensiones y personalizaciones no esenciales están temporalmente desactivadas para proteger la estabilidad».
   - Acción de descarte: `QPushButton` con texto «Entendido», atajo `Esc`, altura >= 48 px en modo accesible, emitiendo ocultación fluida del banner.
   - Accesibilidad: `setAccessibleName("Aviso de modo seguro")`, `setAccessibleDescription("Informa que el navegador se inició en modo seguro a prueba de fallos")`.

3. **Indicador de Navegación Privada (`incognitoBadge_`)**:
   - Widget: `QWidget` integrado en la barra de herramientas o barra de estado (`objectName = "vigilio-incognito-badge"`).
   - Icono: `icons/tools/shield.svg` (tamaño 18×18 px estándar, 24×24 px en Senior Mode).
   - Etiqueta: «Navegación privada».
   - Título de ventana: `setWindowTitle("Vigilio Browser (Navegación privada)")`.
   - Accesibilidad: Anuncio accesible «Sesión de navegación privada activa. No se guardará el historial de navegación ni cookies».

4. **Tratamiento de URLs iniciales y estados de carga**:
   - Si `!cmdLine.urls.empty()`:
     - Itera sobre cada URL válida procesada y llama a `openTab(QUrl(QString::fromStdString(url)))`.
     - La primera pestaña recibe el foco.
     - Se establece el estado visual de carga («Cargando…») hasta la finalización de la navegación.
   - Si `cmdLine.urls.empty()`:
     - Procede a la restauración habitual de sesión (`restoreSession()`); si no hay pestañas persistidas, abre `newHomePage()`.

5. **Diálogo modal de error de arranque (`StartupErrorDialog`)**:
   - Función utilitaria estática o clase auxiliar:
     ```cpp
     int showStartupErrorDialog(
         const vigilio::StartupDiagnosticReport& report,
         QWidget* parent = nullptr);
     ```
   - Utiliza `QMessageBox` nativo o diálogo modal estilizado con tokens del sistema.
   - Título de ventana: «Vigilio Browser».
   - Título principal: «No se pudo iniciar Vigilio Browser».
   - Texto informativo: Mapeo de `report.diagnosticMessage` y `report.remediationAction` a lenguaje comprensible para el usuario, omitiendo rutas del compilador o tecnicismos de Chromium.
   - Botones: «Reintentar» (`QMessageBox::Retry`), «Iniciar en modo seguro» (si el fallo no fue en safe mode), «Cerrar» (`QMessageBox::Close`).

## Archivos afectados

Durante la siguiente fase (APPLY), las modificaciones se limitarán estrictamente a los siguientes archivos autorizados:
1. `apps/vigilio-browser/src/ui/BrowserWindow.h`:
   - Declaración de inclusión de `core/StartupTypes.h`.
   - Sobrecarga del constructor para aceptar `StartupCommandLine` y `StartupDiagnosticReport`.
   - Declaración de punteros a widgets de estado de arranque: `QWidget* safeModeBanner_{nullptr};`, `QLabel* incognitoBadge_{nullptr};`.
   - Métodos auxiliares privados: `void setupStartupVisuals(const vigilio::StartupCommandLine& cmdLine, const vigilio::StartupDiagnosticReport& report);`, `void setupSafeModeBanner();`.
2. `apps/vigilio-browser/src/ui/BrowserWindow.cpp`:
   - Implementación del constructor parametrizado y resolución dinámica de la ruta del repositorio SQLite según `report.storagePath` e `isStoragePersistent`.
   - Construcción del banner de modo seguro y su botón accesible de descarte.
   - Construcción del indicador visual de navegación privada.
   - Apertura secuencial de las URLs pasadas en la línea de comandos.
   - Adaptación de alturas mínimas, tipografía e iconografía en `setSeniorMode()` para los elementos de arranque.
3. `apps/vigilio-browser/src/app.cpp`:
   - Paso de `cmdLine` y `report` al constructor de `BrowserWindow window(cmdLine, report);`.
   - Presentación de diálogo accesible de error en caso de fallo de arranque cuando existe un entorno gráfico activo (`display || wayland`).
4. `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
   - Casos de prueba automatizados en CTest (`window_ui_tests`):
     - `testStartupWithInitialUrls`: Verifica que abrir con URLs CLI crea pestañas correspondientes.
     - `testStartupIncognitoMode`: Verifica que el modo incógnito no crea base de datos en disco y despliega el indicador privado.
     - `testStartupSafeModeBanner`: Verifica que el modo seguro despliega `vigilio-safe-mode-banner` con su botón de descarte.
     - `testStartupSeniorModeScaling`: Verifica el dimensionamiento accesible (>= 48 px) de los botones de arranque.
     - `testStartupProfilePath`: Verifica la inicialización del repositorio en la ruta personalizada.

## Contrato UX/UI

### Elementos visuales, ubicación y tokens de diseño

| Elemento | Ubicación | Label en español | Icono Lucide SVG | Dimensiones estándar | Dimensiones Senior Mode | Token de color / Fondo |
|---|---|---|---|---|---|---|
| Banner Modo Seguro | Superior (sobre pestañas) | «Modo seguro activado: extensiones y personalizaciones desactivadas» | `icons/tools/shield.svg` | Alto 42px, padding 8px | Alto >= 55px, padding 12px | Superficie `#1e293b`, borde `#3b82f6` |
| Botón Descarte Banner | Extremo derecho de banner | «Entendido» | N/A (texto claro) | Alto 32px, min-width 80px | Alto >= 48px, min-width 100px | Botón secundario sobrio `#334155` |
| Badge Incógnito | Barra de estado / herramientas | «Navegación privada» | `icons/tools/shield.svg` | Icono 18×18px, texto 12pt | Icono 24×24px, texto 14pt | Superficie `#0f172a`, texto `#94a3b8` |
| Estado Vacío | Pestaña principal (`vigilio-home`) | «Vigilio Browser», buscador | `branding/vigilio-logo.svg` | Logo 64×64px, entrada 36px | Logo 80×80px, entrada >= 48px | Fondo principal `#0b0f19` |
| Diálogo Error Arranque | Ventana modal centrada | «No se pudo iniciar Vigilio Browser» | `icons/tools/shield.svg` | Ancho 480px, botones 36px | Ancho 560px, botones >= 48px | Paleta del sistema / Dark `#0f172a` |

### Estados interactivos

- `idle`: Aspecto normal integrado en el tema activo (claro u oscuro).
- `focus`: Anillo de foco visible de 2px de ancho con contraste accesible (`#60a5fa` en modo oscuro, `#2563eb` en modo claro).
- `hover`: Iluminación sutil de fondo (aumento de 5% de luminosidad) sin alteraciones de layout.
- `disabled`: Atenuación de opacidad al 40%, cursor no interactivo (`Qt::ForbiddenCursor`).
- `loading`: Pestaña muestra «Cargando…», barra de estado muestra «Iniciando navegación…».
- `empty`: Pestaña despliega `newHomePage()` con buscador centrado y accesos rápidos.
- `success`: Carga completada, título de página visible, barra de estado actualizada.
- `error`: Despliegue de `createErrorPageWidget` en la pestaña o diálogo modal en caso de fallo de arranque irrecuperable.

### Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)
- Proscripción terminante de: «Senior Mode», «Modo Senior», «Prompt Builder», «APPLY», «DAG», «OpenSpec», «Chromium», «WebEngineWidgets», «bootstrap», «CLI».
- Textos aprobados: «Interfaz más fácil de usar», «Modo seguro activado», «Navegación privada», «No se guardará el historial», «Entendido», «Reintentar», «Cerrar», «No se pudo iniciar Vigilio Browser».

## Reglas y fallos

1. **Entrada malformada o inválida en CLI**:
   - Si se pasan argumentos desconocidos o banderas inseguras (`--no-sandbox`, `--disable-web-security`), el coordinador los rechaza en `parseCommandLine`.
   - `app.cpp` emite diagnóstico a `stderr` con código 10 y no instanciará la interfaz gráfica.
2. **Ausencia de servidor gráfico**:
   - Si no existen `DISPLAY` ni `WAYLAND_DISPLAY`, el coordinador degrada a `DegradedHeadless`.
   - `app.cpp` finaliza de forma controlada con código 1 sin intentar crear widgets Qt ni arrojar `SIGSEGV`.
3. **Fallo de acceso a almacenamiento de perfil (`StorageAccessDenied`)**:
   - Si la ruta de perfil especificada carece de permisos de escritura, el bootstrap falla con `StorageAccessDenied`.
   - En entorno gráfico, despliega el diálogo accesible de error de arranque recomendando verificar permisos o iniciar con un perfil alternativo.
4. **Recursos críticos corruptos o ausentes (`CorruptCriticalResources`)**:
   - Si la verificación de integridad de recursos falla en arranque, despliega el diálogo informativo ofreciendo reintentar o cerrar.
5. **URL inicial inválida o no accesible**:
   - Si una URL pasada en CLI falla al navegar, la pestaña correspondiente transiciona al estado de error (`createErrorPageWidget`), ofreciendo «Reintentar» o «Ir al inicio» sin congelar el resto del navegador.

## Seguridad y privacidad

- **Constitución Local-First (Decisión Q-001)**:
  - Cero llamadas a servidores remotos, telemetría o comprobaciones cloud en el arranque.
  - Toda la configuración visual y de estado se inicializa exclusivamente a partir de recursos locales (`resources.qrc`), bases de datos SQLite locales y argumentos de proceso.
- **Aislamiento de Sesión Privada (Modo Incógnito)**:
  - Cuando se activa el modo incógnito (`--incognito`), la base de datos de historial y marcadores se asigna a `:memory:`.
  - Cero escrituras en el disco local para datos de navegación, historial de visitas o cookies de sesión.
  - El cierre de la ventana libera de inmediato la memoria sin dejar rastros en el sistema de archivos.
- **Defensa contra Directory Traversal**:
  - Toda ruta provista mediante `--profile-dir` es saneada y validada estrictamente por `isValidProfilePath()`, impidiendo secuencias `..` o accesos a directorios críticos del sistema (`/etc`, `/boot`, `C:\Windows`).
- **Defensa activa del Sandboxing**:
  - Se garantiza que el navegador nunca acepte banderas que deshabiliten el aislamiento de procesos de Chromium (`--no-sandbox`).
- **Presupuestos de memoria y rendimiento**:
  - Inicialización visual rápida: creación de la ventana principal y banner en menos de 50 ms.
  - Carga bajo demanda (lazy loading): si se abren múltiples URLs por CLI, las vistas de renderizado web se cargan secuencialmente sin saturar la memoria RAM.

## Matriz de pruebas

| Identificador | Nivel | Caso de prueba / Criterio | Fixture / Condiciones | Comando de ejecución | Resultado esperado | Fallo negativo verificado |
|---|---|---|---|---|---|---|
| `TEST-UI-001` | Qt UI | Inicialización GUI estándar con pestaña de inicio | Entorno con display, sin argumentos CLI | `ctest -R window_ui_tests -V` | `tabCount() == 1`, `vigilio-home` desplegado, `safeModeBanner_ == nullptr` | Pestaña vacía sin widget de inicio |
| `TEST-UI-002` | Qt UI | Arranque con URLs iniciales vía CLI | `StartupCommandLine` con 2 URLs válidas | `ctest -R window_ui_tests -V` | `tabCount() == 2`, ambas pestañas creadas con URLs respectivas | Abre solo 1 pestaña o ignora argumentos |
| `TEST-UI-003` | Qt UI | Arranque en Modo Seguro (`--safe-mode`) | `StartupCommandLine.isSafeMode = true` | `ctest -R window_ui_tests -V` | `safeModeBanner_ != nullptr`, visible, texto conforme a UX copy | Banner no visible o texto incorrecto |
| `TEST-UI-004` | Qt UI | Descarte accesible del Banner de Modo Seguro | Banner visible, clic en botón «Entendido» | `ctest -R window_ui_tests -V` | `safeModeBanner_->isVisible() == false` tras el clic | Banner permanece visible tras descartar |
| `TEST-UI-005` | Qt UI | Arranque en Modo Incógnito (`--incognito`) | `StartupCommandLine.isIncognito = true` | `ctest -R window_ui_tests -V` | `incognitoBadge_` visible, `db_` en memoria, título contiene privado | Persiste en archivo físico en incógnito |
| `TEST-UI-006` | Qt UI | Escalado adaptativo en «Interfaz más fácil de usar» | `setSeniorMode(true)` en ventana con banner | `ctest -R window_ui_tests -V` | Alturas mínimas de botones >= 48px, barra >= 55px, iconos 32×32px | Controles con altura < 48px en modo accesible |
| `TEST-UI-007` | Qt UI | Atributos de tecnología asistiva (lectores de pantalla) | Inspección de metadatos de accesibilidad | `ctest -R window_ui_tests -V` | `accessibleName()` y `accessibleDescription()` no vacíos en español | Cadenas vacías o con términos técnicos en inglés |
| `TEST-UI-008` | Qt UI | Tolerancia en ejecución CI sin servidor gráfico | `DISPLAY` y `WAYLAND_DISPLAY` vacíos | `ctest -R window_ui_tests -V` | Salida limpia `NOT_EXECUTED`, exit code 0, sin `SIGSEGV` | Crash o abort abrupto por falta de display |
| `TEST-REG-009` | Integración | Pipeline completo de pruebas unitarias y de warnings | Proyecto completo compilado en CMake | `bash apps/vigilio-browser/scripts/check.sh` | 9/9 pruebas CTest pasando al 100% en < 1 segundo | Pruebas fallidas o regresiones en `core_units` |

## Pasos APPLY

1. **Paso 1**: Extender `apps/vigilio-browser/src/ui/BrowserWindow.h` agregando la inclusión de `core/StartupTypes.h`, la sobrecarga del constructor con parámetros por defecto `explicit BrowserWindow(const vigilio::StartupCommandLine& cmdLine = {}, const vigilio::StartupDiagnosticReport& report = {});` y los miembros privados `safeModeBanner_`, `incognitoBadge_`.
2. **Paso 2**: Modificar `apps/vigilio-browser/src/ui/BrowserWindow.cpp` para inicializar el repositorio SQLite en `:memory:` si `cmdLine.isIncognito` es verdadero, o en `report.storagePath` si es personalizado; construir el banner de modo seguro y el indicador de incógnito.
3. **Paso 3**: Implementar en `BrowserWindow.cpp` el procesamiento de `cmdLine.urls`, abriendo pestañas para cada URL inicial provista en la línea de comandos.
4. **Paso 4**: Adaptar las funciones de dimensionamiento y escalado en `BrowserWindow::setSeniorMode(bool enabled)` para actualizar las áreas de interacción del banner de modo seguro y los componentes de arranque.
5. **Paso 5**: Actualizar `apps/vigilio-browser/src/app.cpp` para pasar `cmdLine` y `report` al instanciar `BrowserWindow window(cmdLine, report);`.
6. **Paso 6**: Ampliar `apps/vigilio-browser/tests/ui/WindowTests.cpp` agregando aserciones de prueba para modo seguro, modo incógnito, URLs iniciales y accesibilidad.
7. **Paso 7**: Ejecutar `bash apps/vigilio-browser/scripts/check.sh` para certificar la compilación limpia bajo C++20 con `-Werror` y la aprobación de la suite completa de pruebas en CTest.

## Definición de terminado

- [x] Propuesta técnica integral redactada en `openspec/changes/module-foundations-task0023/proposal.md` cubriendo la etapa `ui` de `startup bootstrap`.
- [x] Documento de diseño arquitectónico `design.md` con secuencias, diagramas de estado y contratos de componentes.
- [x] Plan de tareas atómicas y verificables formalizado en `tasks.md`.
- [x] Registro de trazabilidad y memoria de ejecución consolidado en `execution-memory.md`.
- [x] Cero modificaciones a archivos fuera de `openspec/changes/module-foundations-task0023/**` durante esta fase PROPOSE.
- [x] Validación al 100% de referencias canónicas mediante Prompt Builder CLI (`validate-references`).
- [x] Validación al 100% del lockfile mediante Prompt Builder CLI (`verify-lock`).
- [x] Verificación de decisiones humanas resueltas mediante Prompt Builder CLI (`validate-decisions`).
- [x] Compromiso explícito de entrega incremental y ausencia de dependencias bloqueantes para revisión humana.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0022`: Implementación del coordinador `StartupBootstrapCoordinator` y tipos inmutables en `vigilio_core` (completada y certificada).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Servidores de CI headless sin servidor gráfico provocando fallos en pruebas de UI.
    - *Mitigación*: La suite `WindowTests.cpp` detecta la ausencia de `DISPLAY`/`WAYLAND_DISPLAY` y emite `NOT_EXECUTED` retornando exit code 0 de forma limpia.
  - *Riesgo*: Filtración accidental de datos en disco durante sesiones privadas de incógnito.
    - *Mitigación*: Se condiciona contractualmente la inicialización de `LocalRepository` a `:memory:` cuando `cmdLine.isIncognito` es verdadero.
  - *Riesgo*: Infracción de las reglas de lenguaje público de producto.
    - *Mitigación*: Todos los textos de UI han sido contrastados contra `docs/UX-COPY-PUBLICO.md`, prohibiendo vocabulario de ingeniería interno.

## Commit propuesto

`feat(foundations): ui startup-bootstrap`
