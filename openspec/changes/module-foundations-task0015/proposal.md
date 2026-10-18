# Change Proposal — module-foundations-task0015

Pack: `module.foundations.task0015`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **WebEngine version compatibility** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado en `module.foundations.task0013` la especificación del modelo de dominio, taxonomía de capacidades funcionales e invariantes de versión de Chromium (Chromium milestone >= 128 asociado al baseline Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), e implementado en `module.foundations.task0014` el validador desacoplado en C++20 (`vigilio::WebEngineVersionValidator`), la pre-validación de arranque en `app.cpp` y la suite de pruebas unitarias en CTest, esta microtarea proyecta la interacción, gestión de estados visuales, tratamiento accesible de degradación y contratos de presentación en Qt Widgets para el subsistema de navegación WebEngine.

El alcance concreto de este pack comprende:
- **Diseño de interacción Qt Widgets y gestión de estados de WebEngine (`BrowserWindow`)**:
  - Formalización del ciclo de vida y máquinas de estados visuales para pestañas y vistas de navegación en `BrowserWindow`:
    - `empty`: pestaña inicial local (`newHomePage()`), con buscador centrado, títulos sobrios y cero consumo de instancias de motor web ni procesos auxiliares Chromium.
    - `loading`: navegación en curso, retroalimentación accesible inmediata en la barra de pestañas («Cargando…») y barra de estado («Cargando página…»), manteniendo reactivo el hilo de interfaz.
    - `success`: navegación completada exitosamente (`loadFinished(true)`), resolución del título de página (truncado a 30 caracteres para legibilidad en pestañas), actualización de la barra de direcciones con la URL canónica y registro seguro en la base de datos SQLite local de historial y visitas.
    - `error`: tratamiento visual nativo y accesible ante fallos de carga (`loadFinished(false)`), terminación anómala del proceso de renderizado (`renderProcessTerminated`) o incompatibilidad de motor web; presentación de un contenedor de estado de error nativo en Qt Widgets con mensaje en lenguaje público («No se pudo cargar la página»), botón accesible de acción primaria («Reintentar») y botón secundario («Inicio»), evitando páginas en blanco permanentes o diálogos modales disruptivos.
    - `deferred` (lazy loading): preservación del patrón de pestañas diferidas al restaurar sesiones previas (`restoreSession`), evitando la inicialización masiva simultánea de múltiples instancias de WebEngine para mantener el consumo de memoria RAM por debajo de los límites seguros.
- **Contrato UX/UI y estricta conformidad con lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
  - Restricción absoluta de terminología interna de ingeniería en la interfaz gráfica visible: proscripción total de términos como «Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «renderProcessTerminated», «ABI mismatch», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» o «OpenSpec».
  - Adopción exhaustiva de textos aprobados en español: «Interfaz más fácil de usar», «Cargar pestañas cuando las necesites», «Ahorro de memoria», «No se pudo cargar la página», «Reintentar», «Inicio», «Página cargada».
  - Integración nativa con la «Interfaz más fácil de usar» (Senior Mode):
    - Dimensiones y áreas táctiles: altura mínima de campos de entrada (`QLineEdit`) escalada de 32px a 48px, barras de herramientas (`QToolBar`) de 40px a 55px, y botones de reintento/navegación con tap targets de al menos 48px de altura.
    - Tipografía escalable: fuente del sistema o Inter configurada en 14pt (escalable a 18pt en modo accesible) con espaciado holgado y legibilidad garantizada.
    - Foco visible y navegación por teclado: secuencia lógica de tabulación (`Tab` / `Shift+Tab`), activación por `Enter` o `Space`, y atajos canónicos (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+R` / `F5`).
    - Accesibilidad asistiva: asignación de `setAccessibleName` y `setAccessibleDescription` para lectores de pantalla (Orca en Linux, Narrator en Windows, VoiceOver en macOS).
- **Tratamiento controlado de arranque y degradación de entorno (`app.cpp` y `BrowserWindow`)**:
  - Protocolo de degradación ante entornos no compatibles (`IncompatibleChromiumVersion`, `IncompatibleQtVersion`, `ProcessHelperMissing` o `EnvironmentMisconfigured`):
    - En modo interactivo de escritorio (`FullDesktopBrowser`), emisión estructurada de diagnóstico a `stderr` con código de salida 1 sin provocar cuelgues, bloqueos de procesos ni ventanas congeladas en blanco.
    - En modo headless o restringido (`DegradedHeadless` o `MinimalCore`), habilitación de ejecución controlada sin instanciación de vistas gráficas de WebEngine.
- **Gobernanza de targets de compilación y pruebas de UI en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Mantenimiento del target condicional `vigilio_ui_tests` bajo la guarda `if(TARGET Qt6::Test)` y `if(VIGILIO_BUILD_GUI)`.
  - Reporte transparente y honesto de `NOT_EXECUTED` cuando falten bibliotecas de desarrollo o display gráfico en el entorno de pruebas, prohibiendo terminantemente la introducción de mocks artificiales que finjan integración gráfica real.
  - Preservación estricta de la compilación desacoplada headless (`VIGILIO_BUILD_GUI=OFF` / preset `core-debug`), asegurando que las suites de prueba de dominio y almacenamiento (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`, `webengine_version_validator_tests`) continúen ejecutándose al 100% con éxito.

## Out of scope

- Implementación de extensiones web bajo la API Manifest V3 o emulación de APIs de extensiones (reservado para el módulo `extensions`, microtareas 0705 a 0730).
- Pruebas sintéticas automatizadas de inyección de fallos pesados o simulación de procesos Chromium caídos en CTest (reservado a la etapa `tests` en `module.foundations.task0016`).
- Inclusión, configuración y compilación de recursos binarios vectoriales QRC (reservado a la capacidad `resource-qrc-inclusion`, microtareas 0017 a 0020).
- Conexión a servidores remotos, sincronización de marcadores en la nube o autenticación de cuentas Vigilio (prohibido por la constitución local-first y la decisión Q-001).
- Modificación directa de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida exclusivamente a `openspec/changes/module-foundations-task0015/**`).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser` y las herramientas del entorno local:
1. **Infraestructura de validación existente (`src/core/WebEngineVersionValidator.h` y `.cpp`)**:
   - Implementada en `task0014` dentro de la biblioteca estática `vigilio_core`.
   - Provee las entidades inmutables `ChromiumVersion` (con spaceship operator `<=>`, parsing cuatripartito y comparación de hitos), `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus` y `WebEngineCompatibilityReport`.
   - Ofrece métodos puros desacoplados de WebEngine: `baselineChromiumVersion()`, `minimumChromiumVersionForQt()`, `supportedFeaturesForMilestone()`, `requiredFeaturesForProfile()`, `isHelperExecutable()`, `validateCompatibility()`, `validateStartup()` y formateadores de diagnósticos.
   - Pasa al 100% en CTest mediante la suite unitaria `webengine_version_validator_tests` (exit code 0).
2. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Integra la verificación de Qt en runtime (`QtDependencyValidator::validateRuntimeEnvironment`).
   - Integra la pre-validación de WebEngine (`WebEngineVersionValidator::validateStartup(report.foundVersion, "128.0.6613.119", "", false)`).
   - Maneja fallos de compatibilidad emitiendo diagnósticos claros y terminando con código 1 antes de instanciar `QApplication`.
3. **Capa visual existente (`src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`)**:
   - Construida con Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
   - Implementa apertura de pestañas (`openTab`), página de inicio nativa (`newHomePage`), carga diferida de pestañas en restauración de sesión (`restoreSession`, `activateDeferredTab`) y favoritos locales en SQLite (`db_`).
   - Implementa conmutador de «Interfaz más fácil de usar» con escalado de tipografías a 14pt y controles con alturas mínimas de 48px y 55px.
   - Estado actual de manejo de errores de WebEngine: actualmente `loadFinished` se limita a emitir un mensaje efímero en la barra de estado («No se pudo cargar la página»), careciendo de un contenedor nativo de reintento/recuperación cuando una página web no puede representarse o el proceso falla.
4. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Compila bajo C++20 con `-Werror` mediante `vigilio_compiler_warnings`.
   - Target estático `vigilio_core` desacoplado de dependencias gráficas de WebEngine.
   - Target ejecutable `vigilio-browser` y suite de pruebas `vigilio_ui_tests` bajo condiciones de GUI y Qt disponibles.
5. **Verificación de herramientas locales de desarrollo**:
   - Compilador: GNU GCC 15.2.0 (`-std=c++20`).
   - CMake: versión 4.2.3.
   - Ninja: versión 1.13.2.
   - SQLite3: versión 3.46.1.
   - Script de comprobación local `apps/vigilio-browser/scripts/check.sh`: compilación limpia y 5/5 pruebas unitarias y de integración pasando al 100% en 0.14 segundos, exit code 0.
   - Verificación de formateo con Clang-Format: 100% de archivos en `src/` y `tests/` cumplen el estándar sin divergencias, exit code 0.
   - Prompt Builder CLI: `validate-references` superó 15/15 referencias canónicas con estado `valid: true`, exit code 0.
   - Verificación de lockfile: `verify-lock` confirmó 13/13 comprobaciones exitosas para el lockfile `0043-module.foundations.task0015.propose.lock.json`.
   - Registro de decisiones humanas (`validate-decisions`): decisiones Q-001 a Q-006 y Q-008 resueltas; Q-007 permanece OPEN afectando exclusivamente el hito de empaquetado final `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0015`.
- **Capacidad**: `WebEngine version compatibility`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0015/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0015/proposal.md`
  - `openspec/changes/module-foundations-task0015/design.md`
  - `openspec/changes/module-foundations-task0015/tasks.md`
  - `openspec/changes/module-foundations-task0015/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0015/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la interacción y estados visuales en Qt para `WebEngine version compatibility` se desglosa en:
1. **Contratos de estados visuales de navegación en `BrowserWindow`**:
   - **Estado Vacío (`empty`)**: La pestaña nueva despliega `newHomePage()` con título "Vigilio Browser", subtítulo "Navega. Organiza. Protege." y barra de búsqueda accesible (`QLineEdit`), sin crear ningún objeto `QWebEngineView` en memoria hasta que el usuario introduzca una dirección válida.
   - **Estado Cargando (`loading`)**: Al emitir `view->load(url)`, la pestaña asociada actualiza su texto a `"Cargando…"`, y la barra de estado refleja `"Cargando página…"`. El foco del usuario no se interrumpe y la interfaz responde a cancelaciones (`Escape` o cierre de pestaña).
   - **Estado Éxito (`success`)**: Al recibirse la señal `loadFinished(true)`, se actualiza el título de la pestaña con `view->title().left(30)`, la barra de direcciones con `view->url().toString()`, la barra de estado con `"Página cargada"` durante 3000 ms, y se registra la visita en `db_->recordVisit` si el esquema es HTTP/HTTPS.
   - **Estado Error / Incompatibilidad (`error`)**: Al dispararse `loadFinished(false)` o ante desconexión del motor web, se despliega en el área de visualización un contenedor nativo `QWidget` estructurado y accesible:
     - Icono o emblema sobrio SVG.
     - Título legible: `"No se pudo cargar la página"`.
     - Descripción orientativa sin tecnicismos: `"Comprueba la dirección web o la conexión del equipo."`.
     - Botón de acción primaria: `"Reintentar"` (invoca `view->reload()`).
     - Botón de acción secundaria: `"Ir al inicio"` (abre `newHomePage()`).
     - Propiedades de accesibilidad completas: foco en el botón de reintento, soporte de navegación con teclas de flecha y `Tab`, y descripción semántica para lectores de pantalla.
   - **Estado Diferido (`deferred`)**: En restauración de sesiones previas, las pestañas inactivas se representan mediante marcadores ligeros (`QWidget` con etiqueta `"Pestaña en espera. Se cargará al abrirla."` y propiedad `deferredUrl`), instanciando `QWebEngineView` exclusivamente cuando el usuario conmuta hacia esa pestaña mediante `activateDeferredTab(index)`.
2. **Integración con la «Interfaz más fácil de usar» (Senior Mode)**:
   - Al activarse `setSeniorMode(true)`:
     - Los campos de entrada (`QLineEdit`), incluyendo la barra de direcciones y la búsqueda en nueva pestaña, incrementan su altura mínima de 32px a 48px.
     - Las barras de herramientas (`QToolBar`) amplían su altura mínima de 40px a 55px con espaciado amplio entre acciones (`spacing: 12px`).
     - Los botones del contenedor de error («Reintentar», «Ir al inicio») escalan a una altura mínima de 48px con padding generoso (12px vertical, 24px horizontal).
     - La tipografía global de la aplicación (`qApp->font()`) se incrementa a 14pt (escalable a 18pt), manteniendo jerarquías visuales limpias.
     - Los bordes y anillos de foco de teclado adoptan un contraste reforzado (`#2563EB` en modo claro, `#60A5FA` en modo oscuro) con grosor de 2px, cumpliendo WCAG 2.2 nivel AA.
3. **Manejo de atajos de teclado y accesibilidad**:
   - `Ctrl+T`: Nueva pestaña (abre `newHomePage()`).
   - `Ctrl+W`: Cerrar pestaña actual (si no está fijada).
   - `Ctrl+L`: Enfocar barra de direcciones y seleccionar todo el texto.
   - `Ctrl+D`: Guardar página actual en favoritos locales.
   - `Ctrl+R` / `F5`: Recargar página o reintentar carga tras error.
   - `Escape`: Cancelar carga en curso o cerrar menús contextuales.
   - `Tab` / `Shift+Tab`: Navegación circular accesible entre barra de navegación, pestañas y controles de página.
4. **Robustez y prevención de fugas de recursos**:
   - Gestión estricta del ciclo de vida de `QWebEngineView`: destrucción limpia mediante `deleteLater()` al cerrar pestañas, evitando instancias huérfanas de `QWebEnginePage`.
   - Límite de instancias activas de renderizado: gracias al patrón diferido (`deferred`), una sesión con 50 pestañas restauradas consume inicialmente una única instancia de WebEngine, manteniendo el uso de memoria RAM del navegador por debajo de 150 MB en reposo.
   - Prevención de procesos zombies: cero procesos auxiliares Chromium lanzados en modo headless o antes de que el usuario solicite explícitamente navegación web.

## Archivos afectados

Para la subsiguiente fase APPLY (tras revisión y aprobación humana):
1. `apps/vigilio-browser/src/ui/BrowserWindow.h` (modificado): declaración de métodos auxiliares para la creación de la vista de error nativa (`createErrorPage`), actualización de estados de navegación y gestión de atajos de recarga.
2. `apps/vigilio-browser/src/ui/BrowserWindow.cpp` (modificado): integración del contenedor nativo de error en `openTab` ante `loadFinished(false)`, cableado de acciones «Reintentar» e «Ir al inicio», adaptación a la «Interfaz más fácil de usar» y atajos `Ctrl+R` / `F5`.
3. `apps/vigilio-browser/CMakeLists.txt` (verificado): gobernanza del target de interfaz `vigilio-browser` y suite condicional `vigilio_ui_tests`.
4. `apps/vigilio-browser/tests/ui/WindowTests.cpp` (modificado/ampliado): pruebas de interfaz con `QTest` que validan la apertura de pestañas, el estado diferido, el escalado de controles en la «Interfaz más fácil de usar» y la presencia de la vista nativa de recuperación.
5. `docs/rules/01-modules/foundations/micro/0015-webengine-version-compatibility/task.md` (verificado): alineación y trazabilidad del estado de ejecución.

## Contrato UX/UI

- **Ubicación en la interfaz**: Ventana principal del navegador (`BrowserWindow`), área central de visualización de pestañas (`QTabWidget`), barra de navegación y barra de estado.
- **Labels en español aprobados (`docs/UX-COPY-PUBLICO.md`)**:
  - Pestaña vacía: `"Nueva pestaña"`.
  - Pestaña cargando: `"Cargando…"`.
  - Barra de estado cargando: `"Cargando página…"`.
  - Barra de estado éxito: `"Página cargada"`.
  - Barra de estado fallo: `"No se pudo cargar la página"`.
  - Título del contenedor de error: `"No se pudo cargar la página"`.
  - Subtítulo del contenedor de error: `"Comprueba la dirección web o la conexión del equipo."`.
  - Botón primario de reintento: `"Reintentar"`.
  - Botón secundario de navegación: `"Ir al inicio"`.
  - Conmutador de accesibilidad: `"Interfaz más fácil de usar"`.
  - Estado de accesibilidad: `"Interfaz fácil activada"` / `"Interfaz fácil desactivada"`.
- **Dimensiones y tokens visuales**:
  - Modo normal:
    - Barra de direcciones: altura 32px, radio 7px, padding horizontal 10px.
    - Barra de navegación: altura 40px, espaciado entre botones 8px.
    - Pestañas: altura 34px–40px, padding 8px 12px.
    - Tipografía: tamaño base 10pt.
  - Modo «Interfaz más fácil de usar» (Senior Mode):
    - Barra de direcciones: altura mínima 48px, radio 8px, padding horizontal 14px.
    - Barra de navegación: altura mínima 55px, espaciado entre botones 12px.
    - Botones de acción en página de error: altura mínima 48px, padding 12px 24px, radio 8px.
    - Pestañas: padding ampliado 12px 18px.
    - Tipografía: tamaño base 14pt (escalable a 18pt).
- **Esquema de color y contraste**:
  - Tema Claro: fondo principal `#F7F9FC`, superficies de widgets `#FFFFFF`, texto principal `#192538`, bordes `#D9E0EA`, acento `#2563EB`, botón de acción primaria fondo `#2563EB` con texto blanco.
  - Tema Oscuro: fondo principal `#141D2B`, superficies de widgets `#222D3E`, texto principal `#E9EDF5`, bordes `#394557`, acento `#3B82F6`, botón de acción primaria fondo `#294F89` con texto blanco.
  - Contraste superior a 4.5:1 en textos estándar y 3:1 en controles interactivos (WCAG 2.2 nivel AA).
- **Referencias visuales oficiales**:
  - Coherencia con `/resultado/01-vigilio-home.png` (diseño sobrio de la página de inicio y barra de navegación).
  - Coherencia con `/resultado/03-vigilio-settings-senior-mode.png` (apariencia de la «Interfaz más fácil de usar»).
  - Coherencia con `/resultado/06-logo-instalador-oficial.png` (identidad de marca y logotipo oficial de Vigilio Browser).

## Reglas y fallos

- **R-001 (Aislamiento de inicialización)**: Ninguna vista `QWebEngineView` se instanciará antes de que el usuario solicite explícitamente navegar a una dirección web válida o active una pestaña diferida.
- **R-002 (Cero tecnicismos en UI)**: Ante cualquier error del motor WebEngine, error de resolución DNS, certificado o proceso terminado, la interfaz presentará exclusivamente el texto público `"No se pudo cargar la página"` con la opción de `"Reintentar"`; queda prohibido mostrar códigos de error internos de Chromium o rastros de depuración en la pantalla.
- **R-003 (Recuperabilidad sin reinicio)**: Un fallo en una pestaña específica no debe cerrar la ventana principal ni afectar el funcionamiento de las demás pestañas abiertas. La pestaña fallida permite reintentar la carga con `view->reload()` o volver al inicio con `openTab()`.
- **R-004 (Persistencia no contaminada)**: Las páginas que fallen en su carga (`loadFinished(false)`) no se registrarán en la tabla de historial ni visitas de SQLite (`db_->recordVisit`).
- **R-005 (Preservación del hilo principal)**: Todas las operaciones de carga y resolución de estados del motor web son asíncronas; no se realizarán llamadas bloqueantes ni esperas activas (`sleep`) en el hilo de interfaz de usuario.
- **R-006 (Gobernanza de Senior Mode)**: El estado de «Interfaz más fácil de usar» se persiste localmente en `QSettings` y se aplica de forma reactiva e inmediata a todas las ventanas, pestañas y controles visibles sin requerir reinicio del navegador.

## Seguridad y privacidad

- **Filosofía local-first (Decisión Q-001)**: El manejo de estados y errores de WebEngine opera de forma 100% autónoma en el equipo local; cero peticiones a servidores remotos para comprobación de conectividad o telemetría de caídas.
- **Aislamiento de perfiles**: Las vistas de navegación respetan el perfil de almacenamiento local (`QStandardPaths::AppDataLocation + "/browser.sqlite"`). No se transmiten URLs ni metadatos de navegación fuera del equipo.
- **Sanitización de mensajes**: Los diagnósticos emitidos hacia `stderr` en caso de fallos de inicialización registran exclusivamente estados funcionales de componentes sin incluir datos personales ni historial del usuario.
- **Gestión segura de memoria y procesos**: Uso exhaustivo de RAII, punteros inteligentes (`std::unique_ptr`) y eliminación segura en Qt (`deleteLater()`), impidiendo fugas de memoria y procesos zombis en el sistema operativo.

## Matriz de pruebas

| Identificador | Criterio de prueba | Tipo | Comando de ejecución | Resultado esperado |
| :--- | :--- | :--- | :--- | :--- |
| `TEST-001` | Estado vacío en nueva pestaña (`openTab()`) despliega `newHomePage()` | UI / Unitario | `ctest -R vigilio_ui_tests` / inspección | PASS: widget `vigilio-home` activo, 0 instancias WebEngine creadas |
| `TEST-002` | Estado de carga (`loading`) actualiza título a `"Cargando…"` | UI / Unitario | `ctest -R vigilio_ui_tests` | PASS: texto de pestaña `"Cargando…"`, barra de estado `"Cargando página…"` |
| `TEST-003` | Estado de éxito (`success`) actualiza título y registra historial | UI / Integración | `ctest -R vigilio_ui_tests` | PASS: título truncado a 30 caracteres, URL en barra de direcciones, visita en SQLite |
| `TEST-004` | Estado de error (`error`) despliega contenedor nativo de reintento | UI / Unitario | `ctest -R vigilio_ui_tests` | PASS: contenedor con `"No se pudo cargar la página"`, botón `"Reintentar"`, foco accesible |
| `TEST-005` | Acción de reintento en estado de error ejecuta `reload()` | UI / Unitario | `ctest -R vigilio_ui_tests` | PASS: evento de recarga emitido sin recrear ventana |
| `TEST-006` | Pestañas diferidas (`deferred`) no inicializan WebEngine hasta su activación | UI / Rendimiento | `ctest -R vigilio_ui_tests` | PASS: pestaña en espera con propiedad `deferredUrl`, inicializada solo en `activateDeferredTab` |
| `TEST-007` | Conmutación a «Interfaz más fácil de usar» redimensiona controles a >= 48px/55px | UI / Accesibilidad | `ctest -R vigilio_ui_tests` | PASS: campos >= 48px, toolbars >= 55px, fuentes escaladas a 14pt |
| `TEST-008` | Navegación por teclado completa (`Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+R`, `F5`) | UI / Accesibilidad | `ctest -R vigilio_ui_tests` | PASS: atajos responden deterministamente y foco de teclado visible |
| `TEST-009` | Auditoría de textos públicos de UI según `docs/UX-COPY-PUBLICO.md` | Documental / Lint | `grep -rn "Senior Mode" apps/` | PASS: 0 ocurrencias en interfaces visibles de usuario |
| `TEST-010` | Reporte transparente de ejecución gráfica en entornos sin display | Calidad / CI | `ctest -R vigilio_ui_tests` | PASS / NOT_EXECUTED limpio según disponibilidad de servidor gráfico |

## Pasos APPLY

Tras la revisión y aprobación humana de esta propuesta, la fase APPLY ejecutará en orden determinista:
1. **Paso 1**: Modificar `apps/vigilio-browser/src/ui/BrowserWindow.h` agregando la definición del widget o layout nativo de recuperación ante fallos de carga (`createErrorPageWidget`), estados de navegación y atajos de recarga (`Ctrl+R` / `F5`).
2. **Paso 2**: Modificar `apps/vigilio-browser/src/ui/BrowserWindow.cpp` implementando el contenedor accesible de error con botones `"Reintentar"` e `"Ir al inicio"`, conectando la señal `loadFinished(false)` y asegurando el escalado de controles para la «Interfaz más fácil de usar».
3. **Paso 3**: Verificar y actualizar la suite de pruebas de interfaz en `apps/vigilio-browser/tests/ui/WindowTests.cpp` para validar la respuesta visual de pestañas ante cargas normales y estados diferidos.
4. **Paso 4**: Ejecutar la suite completa de calidad local: `bash apps/vigilio-browser/scripts/check.sh`, Clang-Format y CTest, comprobando compilación limpia con `-Werror` y 100% de pruebas aprobadas en el entorno local.
5. **Paso 5**: Verificar la sincronización documental y la validación de referencias canónicas con Prompt Builder (`vigilio-prompt validate-references`).

## Definición de terminado

Se considerará terminado el cambio cuando:
1. La interacción y estados visuales (`empty`, `loading`, `success`, `error`, `deferred`) para WebEngine estén formalmente implementados en `BrowserWindow`.
2. El contenedor nativo ante fallos de carga (`loadFinished(false)`) proporcione una vía accesible de reintento («Reintentar») e inicio («Ir al inicio») sin exponer tecnicismos internos de ingeniería.
3. La «Interfaz más fácil de usar» escale los controles a alturas mínimas de 48px y 55px, con tipografías ampliadas a 14pt y foco accesible WCAG 2.2 AA.
4. Se mantenga estricto cumplimiento con `docs/UX-COPY-PUBLICO.md`: cero términos de ingeniería en la interfaz gráfica.
5. El proyecto compile sin advertencias bajo `-Werror` en C++20 y apruebe el 100% de las pruebas automatizadas locales.
6. La propuesta en `openspec/changes/module-foundations-task0015/` quede completamente especificada y verificada mediante `validate-references` de Prompt Builder.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0014`: implementación de `vigilio::WebEngineVersionValidator` en `vigilio_core`.
  - Qt Widgets 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`).
  - Estándar C++20 con compilador compatible (GCC >= 11, Clang >= 14, MSVC >= 19.30).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Caída inesperada del proceso de renderizado de Chromium (`renderProcessTerminated`) dejando un viewport negro o congelado.
    *Mitigación*: Capturar la señal correspondiente en `QWebEnginePage` y sustituir la vista afectada por el contenedor nativo de error en Qt Widgets, permitiendo al usuario recargar la página limpiamente.
  - *Riesgo*: Alto consumo de memoria RAM al abrir múltiples pestañas.
    *Mitigación*: Preservación estricta del patrón de carga diferida (`deferredUrl`), asegurando que solo la pestaña activa en primer plano mantenga una instancia de WebEngine inicializada.

## Commit propuesto

- **Mensaje base**: `feat(foundations): ui webengine-version-compatibility`
- **Ámbito**: Módulo `foundations`, capacidad `WebEngine version compatibility`, etapa `ui`.
