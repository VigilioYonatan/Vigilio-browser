# Change Proposal — module-ux-shell-task0060

Pack: `module.ux-shell.task0060`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **main window layout** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0060 — Definir el comportamiento y las invariantes: main window layout`).

Habiéndose completado y certificado formalmente en las 59 microtareas previas del módulo `foundations` la modularización de targets CMake (`task0001`-`task0004`), la política de advertencias estrictas (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), la política de recuperación ante caídas (`task0029`-`task0032`), los CMake Presets (`task0033`-`task0036`), la infraestructura de test fixtures (`task0037`-`task0040`), los servicios canónicos de Clang-Format (`task0041`-`task0044`) y Clang-Tidy (`task0045`-`task0048`), y los pipelines de integración continua multiplataforma para Linux (`task0049`-`task0052`), Windows (`task0053`-`task0056`) y macOS (`task0057`-`task0059`), este pack inaugura formalmente el módulo `ux-shell` formalizando el **modelo de dominio, taxonomía fuertemente tipada de zonas de ventana, configuraciones canónicas de densidad visual, invariantes geométricas, presupuestos de área central de contenido, validador estático desacoplado y batería de pruebas unitarias de especificación para el diseño estructural de la ventana principal (main window layout)**.

El objetivo central de esta propuesta es proporcionar una base matemática, declarativa y fuertemente tipada en C++20 puro para el layout de la ventana principal de Vigilio Browser (`MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration`, `MainWindowLayoutValidator`), de manera que el cálculo de coordenadas, dimensiones mínimas, visibilidad de barras y presupuestos de renderizado web operen con total independencia del framework de widgets de Qt, previniendo solapamientos visuales, desbordamientos de interfaz y estados inconsistentes al alternar entre la densidad estándar, la densidad compacta y la «Interfaz más fácil de usar» (Modo Senior), todo ello en rigurosa conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices públicas de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de layout (`MainWindowLayoutTypes.h`)**:
  - `LayoutZone`: Zonas funcionales de la ventana (`TitleBar`, `TabStrip`, `ToolBar`, `NotificationBanner`, `Sidebar`, `Content`, `StatusBar`).
  - `LayoutDensity`: Densidades visuales de escritorio (`Compact`, `Default`, `Senior`).
  - `SidebarState`: Estados operativos de la barra lateral (`Hidden`, `Collapsed`, `Expanded`).
  - `SidebarDockPosition`: Posicionamiento de acople lateral (`Left`, `Right`).
  - `WindowChromeMode`: Modalidades de decoración de ventana (`Standard`, `Integrated`, `Fullscreen`).
  - `MainWindowLayoutError`: Clasificación exhaustiva y fuertemente tipada de anomalías de layout (`None`, `DimensionsBelowMinimum`, `InvalidSidebarWidth`, `ConflictingZoneAnchors`, `ZeroContentAreaBudget`, `NegativeMarginOrPadding`, `InvalidDensityConfiguration`, `LayoutPersistenceCorrupted`).
  - Estructuras inmutables con semántica de valor:
    - `WindowDimensions`: Anchura, altura y límites mínimos de ventana (`minWidth`, `minHeight`).
    - `LayoutMargins` y `LayoutRect`: Geometría rectangular cartesiana con métodos puros para intersección, contención y cálculo de área.
    - `ZoneBudget`: Presupuestos dimensionales (preferidos, mínimos y máximos) por zona horizontal y vertical.
    - `SidebarConfig`: Parámetros de la barra lateral (estado, posición, ancho actual, colapsado canónico de 48 px y rango expandible de 200..360 px).
    - `MainWindowLayoutPlan`: Plan estructural inmutable consolidando dimensiones, densidad, modalidad de chrome, configuración de barra lateral y visibilidad de zonas.
    - `ZoneGeometry`: Asignación espacial calculada (`x`, `y`, `width`, `height`, visibilidad) por zona sin solapamiento.
    - `MainWindowLayoutReport`: Diagnóstico inmutable de evaluación de layout con reporte booleano de validez, error tipado, rectángulos calculados, área neta de contenido web y pista de remediación en español neutro.
- **Configuración canónica e invariantes geométricas (`MainWindowLayoutConfiguration`)**:
  - `canonicalDefaultLayout()`: Configuración equilibrada de escritorio moderno (ventana $1200\times 780$ px, mínimos $640\times 480$ px, barra de navegación 42 px, tira de pestañas 38 px, barra lateral colapsada en 48 px, status bar 28 px).
  - `canonicalSeniorLayout()`: Configuración de «Interfaz más fácil de usar» (mínimos ampliados $720\times 520$ px, tap targets y botones $\ge 48$ px, barra de navegación $\ge 56$ px, tira de pestañas 48 px, iconos ampliados a $32\times 32$ px, banner 56 px, status bar 34 px).
  - `canonicalCompactLayout()`: Configuración de alta densidad para pantallas limitadas (mínimos $500\times 380$ px, toolbar 36 px, tab strip 32 px, status bar 24 px).
  - `verifyInvariants(const MainWindowLayoutPlan&)`: Verificación booleana estricta de que el plan cumple con todas las restricciones de no negatividad, presupuestos mínimos de área de contenido ($400\times 250$ px), límites de barra lateral y coherencia de densidad.
- **Validador estático desacoplado y cálculo geométrico puro (`MainWindowLayoutValidator`)**:
  - `validateDimensions(const WindowDimensions&, LayoutDensity)`: Verificación rigurosa de límites dimensionales según la densidad activa.
  - `validateSidebar(const SidebarConfig&)`: Comprobación de invariantes de la barra lateral (colapsada exactamente 48 px; expandida dentro del rango $[200, 360]$ px).
  - `validatePlan(const MainWindowLayoutPlan&)`: Auditoría estructural del plan completo.
  - `calculateLayoutAreas(int windowWidth, int windowHeight, const MainWindowLayoutPlan&)`: Algoritmo determinista que distribuye el espacio disponible entre las zonas visibles y calcula el `contentArea` residual, certificando ausencia matemática de solapamientos (`intersects == false`) y área neta positiva.
  - Funciones puras de UX copy público y descripciones en español neutro conforme a `docs/UX-COPY-PUBLICO.md`:
    - `layoutErrorToString(MainWindowLayoutError)`
    - `layoutZoneToString(LayoutZone)`
    - `layoutDensityToString(LayoutDensity)`
    - `sidebarStateToString(SidebarState)`
    - `sidebarDockPositionToString(SidebarDockPosition)`
    - `chromeModeToString(WindowChromeMode)`
    - `generateRemediationHint(MainWindowLayoutError, std::string_view)`
- **Suite de pruebas unitarias trazables en CTest (`MainWindowLayoutTests.cpp`)**:
  - 12 pruebas unitarias en C++20 puro bajo `vigilio_main_window_layout_tests` ejecutables en modo headless (< 50 ms), sin dependencias gráficas, validando planes canónicos, casos de desbordamiento, cálculos geométricos, docking lateral, banners y conformidad de textos.

---

## Out of scope

- Implementación del servicio de persistencia en disco o SQLite del estado de ventana (`MainWindowLayoutService`) (reservado para la etapa `implementation` en `module.ux-shell.task0061`).
- Integración física con widgets de Qt (`QSplitter`, `QToolBar`, `QTabWidget`, `BrowserWindow.cpp`) y animaciones de expansión/colapso de barra lateral (reservado para la etapa `ui` en `module.ux-shell.task0062`).
- Pruebas E2E completas de interacción con servidor gráfico, redimensionamiento interactivo de ratón y pruebas de estrés de DPI múltiple (reservado para la etapa `tests` en `module.ux-shell.task0063`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` durante esta fase PROPOSE (la implementación física se efectuará estrictamente en APPLY tras aprobación humana).
- Conexión a servicios en la nube o telemetría de uso de interfaz (prohibido por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Integración o renderizado de subprocesos Chromium WebEngine (el modelo de dominio es 100% independiente de WebEngine).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completaron formalmente las 59 microtareas del módulo `foundations` (`task0001` a `task0059`), alcanzando el hito de validación de UI para CI macOS en el commit `cf8c1a3`.
   - El módulo `ux-shell` se inicia con esta microtarea `0060`, siendo responsable del shell visual de escritorio, layout, barra de título, densidad de pestañas, barras de herramientas y accesibilidad.
2. **Inspección de la implementación existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`**:
   - Actualmente, `BrowserWindow` inicializa sus componentes visuales de manera estática y monolítica en `setupUi()`:
     - Crea un widget central con `QVBoxLayout` y márgenes a cero (`centralLayout->setContentsMargins(0, 0, 0, 0); centralLayout->setSpacing(0)`).
     - Añade directamente el `QTabWidget` (`tabs_`) como widget central.
     - Añade una barra de herramientas de navegación superior con `addToolBar("Navegación")`.
     - Gestiona banners condicionales insertados verticalmente (`safeModeBanner_`, `crashRecoveryBanner_`).
     - Modifica manualmente el tamaño de los iconos (`toolbar_->setIconSize(QSize(22, 22))` en estándar vs `QSize(32, 32)` en modo Senior) y las alturas mínimas mediante sentencias dispersas.
   - *Brecha arquitectónica identificada*: No existe una capa de dominio que modele abstractamente las zonas de la ventana, sus relaciones topológicas, restricciones dimensionales mínimas ni el cálculo exacto del área disponible para el contenido web antes de crear o redimensionar widgets.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Muestra el layout objetivo de la ventana principal, integrando barra lateral izquierda con acceso rápido, barra superior de navegación y pestañas, área central con la página de inicio y tarjetas informativas, y barra de estado inferior.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Muestra el comportamiento y proporciones de la «Interfaz más fácil de usar» (Modo Senior), con botones de navegación ampliados, márgenes generosos y alta legibilidad.
   - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.
4. **Verificación de calidad y pipeline continuo (`scripts/check.sh`)**:
   - Compilación limpia con GNU GCC 15.2.0 bajo C++20 con `-Werror` y cero advertencias.
   - 25 de 25 pruebas pasando exitosamente en CTest al 100% en ~5.36s (exit code 0).
5. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `validate-references --project-root .`: 60 referencias canónicas validadas (`valid: true`, exit code 0).
   - `verify-lock --lock-file generated/prompts/full-project/0178-module.ux-shell.task0060.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `validate-decisions`: Única decisión abierta `Q-007` (licencia jurídica), restringida a `release.public-release`, sin bloqueo sobre `module.ux-shell`.

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `domain` está estrictamente delimitado a la modelización matemática y lógica de **main window layout**:
1. **Creación de `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`**:
   - Definición de enumeraciones C++20 fuertemente tipadas (`LayoutZone`, `LayoutDensity`, `SidebarState`, `SidebarDockPosition`, `WindowChromeMode`, `MainWindowLayoutError`).
   - Definición de estructuras de datos con semántica de valor inmutable (`WindowDimensions`, `LayoutMargins`, `LayoutRect`, `ZoneBudget`, `SidebarConfig`, `MainWindowLayoutPlan`, `ZoneGeometry`, `MainWindowLayoutReport`).
2. **Creación de `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h` y `.cpp`**:
   - Especificación de constantes canónicas dimensionales para resoluciones de escritorio estándar, compacta y accesible.
   - Funciones de factoría para los tres planes canónicos oficiales: `canonicalDefaultLayout()`, `canonicalSeniorLayout()`, `canonicalCompactLayout()`.
   - Método de validación de invariantes fundamentales: `verifyInvariants()`.
3. **Creación de `apps/vigilio-browser/src/core/MainWindowLayoutValidator.h` y `.cpp`**:
   - Métodos estáticos de validación de dimensiones, configuración de barra lateral y plan integral.
   - Algoritmo de cálculo de áreas `calculateLayoutAreas()` que descompone el rectángulo total de la ventana en zonas disjuntas y computa el área de contenido neto.
   - Funciones de copy público y textos descriptivos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
4. **Creación de la suite de pruebas unitarias `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`**:
   - 12 pruebas unitarias de especificación que validan todas las invariantes, límites, fallos negativos y textos públicos.
5. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Inclusión de los nuevos archivos de cabecera y fuente en la biblioteca `vigilio_core`.
   - Adición del target ejecutable de pruebas `vigilio_main_window_layout_tests` asociado a CTest con etiquetas `unit;fast`.
6. **Sincronización documental de `docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md`**:
   - Actualización del estado a `TARGET; VERIFIED_DOMAIN` tras certificar la etapa en los gates de calidad.

---

## Diseño completo de la capacidad

### Topología y zonificación de la ventana principal
La ventana principal de Vigilio Browser se concibe como una retícula rectangular delimitada por las dimensiones globales $[0, W) \times [0, H)$. La distribución espacial se divide en zonas horizontales y verticales gobernadas por el siguiente orden topológico:

```
┌────────────────────────────────────────────────────────────────────────┐
│ TitleBarZone (y=0, h=32px) [Opcional si ChromeMode != Integrated]      │
├────────────────────────────────────────────────────────────────────────┤
│ TabStripZone (y=y1, h=38px estándar / 48px Senior / 32px Compact)      │
├────────────────────────────────────────────────────────────────────────┤
│ ToolBarZone  (y=y2, h=42px estándar / 56px Senior / 36px Compact)      │
├────────────────────────────────────────────────────────────────────────┤
│ NotificationBannerZone (y=y3, h=44px estándar / 56px Senior) [Dinámico]│
├───────────────────┬────────────────────────────────────────────────────┤
│ SidebarZone       │ ContentZone                                        │
│ (x=0, w=48px col. │ (x=w_sidebar, y=y_top, w=W - w_sidebar,            │
│  o 200..360px exp)│  h=H - y_top - h_bottom)                           │
│ [Dock: Left/Right]│ Área neta para pestañas, web o error local         │
├───────────────────┴────────────────────────────────────────────────────┤
│ StatusBarZone (y=H - h_status, h=28px estándar / 34px Senior)          │
└────────────────────────────────────────────────────────────────────────┘
```

### Algoritmo de distribución de áreas (`calculateLayoutAreas`)
1. **Entradas**: Ancho $W$ y alto $H$ de la ventana, junto con un `MainWindowLayoutPlan` inmutable.
2. **Validación previa**: Si $W < \text{minWidth}$ o $H < \text{minHeight}$ para la densidad activa, se devuelve inmediatamente un reporte con `valid = false`, error `DimensionsBelowMinimum` y pista de remediación explicativa.
3. **Distribución vertical superior**:
   - Se acumula $y_{\text{top}} = 0$.
   - Si `showTitleBar`: se asigna `TitleBarZone` en $[0, 0, W, h_{\text{title}}]$ y $y_{\text{top}} \mathrel{+}= h_{\text{title}}$.
   - Si `showTabStrip`: se asigna `TabStripZone` en $[0, y_{\text{top}}, W, h_{\text{tab}}]$ y $y_{\text{top}} \mathrel{+}= h_{\text{tab}}$.
   - Si `showToolBar`: se asigna `ToolBarZone` en $[0, y_{\text{top}}, W, h_{\text{tool}}]$ y $y_{\text{top}} \mathrel{+}= h_{\text{tool}}$.
   - Si `showBanner`: se asigna `NotificationBannerZone` en $[0, y_{\text{top}}, W, h_{\text{banner}}]$ y $y_{\text{top}} \mathrel{+}= h_{\text{banner}}$.
4. **Distribución vertical inferior**:
   - Si `showStatusBar`: $y_{\text{bottom}} = H - h_{\text{status}}$, se asigna `StatusBarZone` en $[0, y_{\text{bottom}}, W, h_{\text{status}}]$. Altura vertical disponible para el centro: $H_{\text{center}} = y_{\text{bottom}} - y_{\text{top}}$.
   - Si no hay barra de estado: $H_{\text{center}} = H - y_{\text{top}}$.
5. **Distribución horizontal central**:
   - Ancho lateral efectivo $W_{\text{side}} = 0$.
   - Si `showSidebar` y `state != Hidden`:
     - Si `state == Collapsed`: $W_{\text{side}} = 48$ px.
     - Si `state == Expanded`: $W_{\text{side}} \in [200, 360]$ px (por defecto 240 px).
   - Si `dockPosition == Left`:
     - `SidebarZone`: $[0, y_{\text{top}}, W_{\text{side}}, H_{\text{center}}]$.
     - `ContentZone`: $[W_{\text{side}}, y_{\text{top}}, W - W_{\text{side}}, H_{\text{center}}]$.
   - Si `dockPosition == Right`:
     - `ContentZone`: $[0, y_{\text{top}}, W - W_{\text{side}}, H_{\text{center}}]$.
     - `SidebarZone`: $[W - W_{\text{side}}, y_{\text{top}}, W_{\text{side}}, H_{\text{center}}]$.
6. **Verificación de presupuesto central**:
   - Si $W - W_{\text{side}} < kMinContentWidth$ ($400$ px) o $H_{\text{center}} < kMinContentHeight$ ($250$ px), el reporte emite `valid = false`, error `ZeroContentAreaBudget` y el cálculo aborta antes de emitir geometrías no renderizables.
7. **Verificación de disyunción**: El reporte certifica formalmente que ningún par de zonas visibles presenta intersección espacial: $\forall Z_i, Z_j (i \ne j) \implies Z_i \cap Z_j = \emptyset$.

---

## Archivos afectados

| Archivo | Acción | Descripción y responsabilidad |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h` | Creación | Tipos fuertemente tipados, enumeraciones, estructuras de valor inmutables y reporte de layout. |
| `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h` | Creación | Declaración de constantes dimensionales canónicas, planes estándar, senior y compacto, y verificación de invariantes. |
| `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.cpp` | Creación | Implementación de factorías de planes canónicos y algoritmos de certificación de invariantes. |
| `apps/vigilio-browser/src/core/MainWindowLayoutValidator.h` | Creación | Declaración del validador estático, cálculo espacial y funciones de copy público en español neutro. |
| `apps/vigilio-browser/src/core/MainWindowLayoutValidator.cpp` | Creación | Implementación del algoritmo geométrico `calculateLayoutAreas` y formateo accesible de cadenas. |
| `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp` | Creación | Suite de 12 pruebas unitarias de especificación en CTest. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Registro de nuevos fuentes en `vigilio_core` y target `vigilio_main_window_layout_tests`. |
| `docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md` | Modificación | Sincronización documental de la microtarea canónica 0060. |

---

## Contrato UX/UI

- **Declaración sobre cambios visuales en esta etapa**:
  - Al tratarse de la etapa **domain**, este cambio introduce exclusivamente modelos lógicos, tipos C++20 y validadores matemáticos de diseño en `vigilio_core`. **No altera directamente la renderización visual de widgets en la aplicación en tiempo de ejecución**.
  - Sin embargo, establece el contrato formal y vinculante que gobernará las etapas subsiguientes (`implementation` en `task0061` y `ui` en `task0062`).
- **Coherencia con las referencias visuales oficiales (`/resultado/`)**:
  - `/resultado/01-vigilio-home.png`: El modelo de dominio formaliza de manera explícita la presencia de la barra lateral izquierda (`SidebarZone`, ancho colapsado 48 px / expandido 240 px), la barra de herramientas superior (`ToolBarZone`, altura 42 px) y el espacio central para la página de inicio.
  - `/resultado/03-vigilio-settings-senior-mode.png`: El plan `canonicalSeniorLayout()` formaliza las directrices de «Interfaz más fácil de usar»:
    - Altura mínima de barra de herramientas $\ge 56$ px (asegurando tap targets $\ge 48$ px para todos los controles).
    - Tira de pestañas ampliada a 48 px.
    - Iconos de navegación escalados de $22\times 22$ px a $32\times 32$ px.
    - Espaciados y márgenes ampliados previniendo agrupaciones densas.
- **Cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`**:
  - Todas las cadenas emitidas por `layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `layoutErrorToString` y `generateRemediationHint` utilizan un lenguaje natural claro en español neutro:
    - `LayoutZone::TitleBar` $\to$ «Barra de título»
    - `LayoutZone::TabStrip` $\to$ «Tira de pestañas»
    - `LayoutZone::ToolBar` $\to$ «Barra de navegación»
    - `LayoutZone::NotificationBanner` $\to$ «Aviso de notificación»
    - `LayoutZone::Sidebar` $\to$ «Barra lateral»
    - `LayoutZone::Content` $\to$ «Área de contenido»
    - `LayoutZone::StatusBar` $\to$ «Barra de estado»
    - `LayoutDensity::Default` $\to$ «Densidad estándar»
    - `LayoutDensity::Senior` $\to$ «Interfaz más fácil de usar»
    - `LayoutDensity::Compact` $\to$ «Densidad compacta»
  - Proscripción absoluta de exponer términos internos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec» o «Linter».
- **Estados interactivos modelados**:
  - `idle`: Distribución normal y equilibrada sin banners transitorios.
  - `banner`: Desplazamiento dinámico hacia abajo del área central ante la presencia de avisos de modo seguro o recuperación de pestañas.
  - `senior`: Transición instantánea a áreas ampliadas garantizando área neta de contenido $\ge 400\times 250$ px.

---

## Reglas y fallos

1. **Invariante dimensional mínima**:
   - Una ventana no puede calcular áreas si su anchura o altura es menor que el umbral estipulado para su densidad:
     - Estándar: $W \ge 640$, $H \ge 480$.
     - Senior: $W \ge 720$, $H \ge 520$.
     - Compacta: $W \ge 500$, $H \ge 380$.
   - En caso de infracción, el validador emite `MainWindowLayoutError::DimensionsBelowMinimum` y sugiere aumentar el tamaño de la ventana.
2. **Invariante de la barra lateral**:
   - Colapsada: ancho inmutable de 48 px (espacio exacto para los iconos Lucide).
   - Expandida: ancho restringido estrictamente al intervalo $[200, 360]$ px (valor típico 240 px).
   - Valores menores a 200 px o mayores a 360 px en estado expandido emiten `MainWindowLayoutError::InvalidSidebarWidth`.
3. **Invariante de suficiencia del área central (`ZeroContentAreaBudget`)**:
   - El área asignada al contenido web nunca debe colapsar a cero o valores inferiores a $400\times 250$ px. Si las barras visibles consumen la totalidad del espacio, el validador emite `ZeroContentAreaBudget` y rechaza la distribución.
4. **No negatividad**:
   - Todos los anchos, alturas, coordenadas y márgenes deben ser estrictamente no negativos ($\ge 0$). Cualquier valor negativo dispara `MainWindowLayoutError::NegativeMarginOrPadding`.
5. **Garantía Zero-Zombie**:
   - Todas las funciones de validación y cálculo geométrico son puras, en memoria, de tiempo de ejecución acotado (< 1 ms), sin bucles indefinidos, sin E/S bloqueante y sin llamadas a subprocesos.

---

## Seguridad y privacidad

- **Arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - Todo el cómputo de layout, dimensiones y preferencias opera 100% de manera local en el dispositivo del usuario.
  - Cero telemetría de red, cero llamadas a endpoints externos y cero recolección de hábitos de navegación.
- **Aislamiento de sesiones e incógnito**:
  - Los cálculos geométricos no contienen ni exponen información sensible de las URLs navegadas, títulos de páginas ni cookies.
  - El layout es agnóstico al contenido confidencial renderizado en el área central.
- **Protección de memoria y robustez en C++20**:
  - Tipos fuertemente tipados (`enum class : uint8_t`), semántica de valor inmutable, `std::string_view` para operaciones libres de asignaciones dinámicas superfluas y aserciones de invariantes.
  - Cero desbordamientos de enteros mediante el uso de tipos con signo coherentes y comprobaciones previas.

---

## Matriz de pruebas

| Identificador de prueba | Tipo | Caso / Condición | Resultado esperado | Clasificación |
| --- | --- | --- | --- | --- |
| `testCanonicalDefaultLayoutInvariants` | Unitario | Evaluación del plan canónico por defecto (`canonicalDefaultLayout`) | `verifyInvariants == true`, `validatePlan == None`, reporte `valid == true` | APPLY |
| `testCanonicalSeniorLayoutInvariants` | Unitario | Evaluación del plan accesible (`canonicalSeniorLayout`) | `verifyInvariants == true`, altura de toolbar $\ge 56$ px, tap targets $\ge 48$ px, reporte `valid == true` | APPLY |
| `testCanonicalCompactLayoutInvariants` | Unitario | Evaluación del plan compacto (`canonicalCompactLayout`) | `verifyInvariants == true`, dimensiones mínimas coherentes ($500\times 380$ px) | APPLY |
| `testDimensionsValidationStandard` | Unitario | Evaluación de límites dimensionales estándar ($640\times 480$ px vs $639\times 479$ px) | Acepta $\ge 640\times 480$; rechaza valores inferiores con `DimensionsBelowMinimum` | APPLY |
| `testDimensionsValidationSenior` | Unitario | Evaluación de límites dimensionales Senior ($720\times 520$ px vs $719\times 519$ px) | Acepta $\ge 720\times 520$; rechaza con `DimensionsBelowMinimum` | APPLY |
| `testSidebarValidationBounds` | Unitario | Barra lateral colapsada (48 px), expandida en límites (200 px, 240 px, 360 px) y fuera de rango (100 px, 400 px) | Válido en límites autorizados; emite `InvalidSidebarWidth` fuera de rango | APPLY |
| `testLayoutCalculationDisjointAreas` | Unitario | Cálculo de áreas para ventana $1200\times 780$ px y verificación de no solapamiento | Todas las zonas visibles satisfacen $\text{area} > 0$ y ninguna presenta intersección espacial | APPLY |
| `testLayoutCalculationWithBanner` | Unitario | Inclusión dinámica de `NotificationBannerZone` en el plan | El área de contenido se desplaza verticalmente en exactamente la altura del banner | APPLY |
| `testLayoutCalculationDockLeftVsRight` | Unitario | Conmutación de acople de barra lateral entre `Left` y `Right` | En `Left`, sidebar $x=0$, content $x=48$; en `Right`, content $x=0$, sidebar $x=W-48$ | APPLY |
| `testZeroContentAreaBudgetRejection` | Unitario | Ventana configurada artificialmente con barras que consumen todo el espacio | Emite `ZeroContentAreaBudget` con reporte `valid == false` y sugerencia de ajuste | APPLY |
| `testUxCopySpanishCompliance` | Unitario | Auditoría de cadenas en español neutro de errores, zonas, densidades y sugerencias | Todas las cadenas devuelven texto válido en español y no contienen términos internos proscritos | APPLY |
| `testDynamicDensityTransition` | Unitario | Recálculo de layout al alternar dinámicamente entre Default y Senior | Las áreas calculadas se redimensionan coherentemente sin generar solapamientos | APPLY |
| `testHeadlessExecutionTolerance` | Integración | Ejecución en entornos de CI sin display gráfico | Ejecutable en < 50 ms sin requerir `DISPLAY` ni servidor X11/Wayland | APPLY |
| `testLiveWidgetResize` | UI / Qt | Redimensionamiento dinámico interactivo con eventos de ratón en `QMainWindow` | Ajuste en vivo de widgets Qt | LATER (`task0062`) |
| `testE2EShellInteraction` | E2E | Recorrido completo de navegación y apertura de barra lateral en navegador real | Comportamiento end-to-end integrado | LATER (`task0063`) |

---

## Pasos APPLY

1. **Paso 1: Implementar tipos y contratos de datos (`src/core/MainWindowLayoutTypes.h`)**:
   - Definir `enum class LayoutZone : uint8_t`, `LayoutDensity : uint8_t`, `SidebarState : uint8_t`, `SidebarDockPosition : uint8_t`, `WindowChromeMode : uint8_t`, `MainWindowLayoutError : uint8_t`.
   - Definir estructuras `WindowDimensions`, `LayoutMargins`, `LayoutRect`, `ZoneBudget`, `SidebarConfig`, `MainWindowLayoutPlan`, `ZoneGeometry`, `MainWindowLayoutReport`.
2. **Paso 2: Implementar configuración canónica e invariantes (`src/core/MainWindowLayoutConfiguration.h` y `.cpp`)**:
   - Definir constantes canónicas dimensionales para las tres densidades.
   - Implementar `canonicalDefaultLayout()`, `canonicalSeniorLayout()`, `canonicalCompactLayout()`.
   - Implementar `verifyInvariants(const MainWindowLayoutPlan& plan)`.
3. **Paso 3: Implementar validador y cálculo espacial (`src/core/MainWindowLayoutValidator.h` y `.cpp`)**:
   - Implementar `validateDimensions`, `validateSidebar`, `validatePlan`.
   - Implementar `calculateLayoutAreas(int, int, const MainWindowLayoutPlan&)`.
   - Implementar funciones de UX copy público en español neutro (`layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `layoutErrorToString`, etc.).
4. **Paso 4: Actualizar `apps/vigilio-browser/CMakeLists.txt`**:
   - Registrar `MainWindowLayoutConfiguration.cpp` y `MainWindowLayoutValidator.cpp` en el target `vigilio_core`.
   - Declarar el target ejecutable `vigilio_main_window_layout_tests` enlazando `vigilio_core` y registrándolo en CTest con labels `unit;fast`.
5. **Paso 5: Implementar suite de pruebas unitarias (`tests/unit/MainWindowLayoutTests.cpp`)**:
   - Codificar las 12 pruebas unitarias de especificación descritas en la matriz.
6. **Paso 6: Ejecutar verificación continua (`scripts/check.sh`)**:
   - Compilar con `-Werror`, verificar Clang-Format y validar que las pruebas pasen al 100% en CTest (26/26 pruebas).
7. **Paso 7: Sincronización documental**:
   - Actualizar `docs/rules/01-modules/ux-shell/micro/0060-main-window-layout/task.md`.

---

## Definición de terminado

El incremento de este pack se considerará terminado y listo para revisión cuando:
1. Todos los tipos y estructuras declaradas en `MainWindowLayoutTypes.h` compilen sin errores bajo C++20 con `-Werror`.
2. Las configuraciones canónicas en `MainWindowLayoutConfiguration` verifiquen satisfactoriamente todas las invariantes geométricas del diseño de escritorio y de la «Interfaz más fácil de usar».
3. El algoritmo `calculateLayoutAreas` en `MainWindowLayoutValidator` demuestre matemáticamente la asignación de áreas disjuntas sin solapamientos para todos los estados de layout.
4. Las 12 pruebas unitarias en `MainWindowLayoutTests.cpp` pasen al 100% en CTest en menos de 100 ms en modo headless.
5. Todas las cadenas públicas cumplan estrictamente con `docs/UX-COPY-PUBLICO.md`, sin terminología técnica interna.
6. El pipeline local ejecutado mediante `apps/vigilio-browser/scripts/check.sh` complete con código de salida 0 y cero advertencias.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0059`: Concluida y verificada formalmente (commit `cf8c1a3`).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo 1*: Regresiones en la compilación por dependencias de Qt en el núcleo de dominio.
    - *Mitigación*: `MainWindowLayoutTypes`, `MainWindowLayoutConfiguration` y `MainWindowLayoutValidator` se diseñan con cero inclusiones de Qt (`QWidget`, `QApplication`), utilizando exclusivamente facilidades estándar de C++20.
  - *Riesgo 2*: Desbordamientos visuales o incompatibilidad de tap targets en Modo Senior al escalar ventanas pequeñas.
    - *Mitigación*: Validación preventiva mediante `validateDimensions` que exige $W \ge 720$ px y $H \ge 520$ px para la densidad Senior, rechazando configuraciones insuficientes antes de la renderización.
  - *Riesgo 3*: Ambigüedad en la terminología pública para usuarios no técnicos.
    - *Mitigación*: Auditoría automatizada en pruebas unitarias certificando la proscripción de «Senior Mode» y «Modo Senior», empleando únicamente «Interfaz más fácil de usar».

---

## Commit propuesto

`docs(ux-shell): domain main-window-layout`
