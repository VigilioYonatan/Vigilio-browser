# Change Proposal — module-ux-shell-task0068

Pack: `module.ux-shell.task0068`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **tab strip density** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0068 — Definir el comportamiento y las invariantes: tab strip density`).

Habiéndose completado y certificado formalmente en las microtareas previas del módulo `foundations` (`task0001`-`task0059`) la base de compilación estricta C++20, modularización CMake, integración de recursos vectoriales QRC, ciclo de vida, presets, test fixtures y CI multiplataforma, y en las 8 microtareas previas de `ux-shell` (`task0060`-`task0067`) el diseño estructural de la ventana principal (**main window layout**) y la barra de título integrada (**title bar integration**), este pack aborda la tercera capacidad visual y ergonómica crítica del shell de escritorio: **tab strip density** (densidad y dimensionamiento espacial de la tira de pestañas).

El objetivo central de esta propuesta es proporcionar un modelo de dominio matemático, determinista, declarativo y fuertemente tipado en C++20 puro para la tira de pestañas (`TabStripDensityTypes.h`, `TabStripDensityConfiguration`, `TabStripDensityValidator`), de manera que el cálculo de anchos de pestañas fijadas (*pinned tabs*) y regulares, alturas canónicas por densidad, márgenes de contención, zonificación de pruebas de impacto (*hit-testing*), visibilidad condicional de botones de cierre, detección de desbordamiento (*overflow*), estados de audio/multimedia y estados de ahorro de memoria (*tab discard / pestaña en reposo*) operen con total independencia del framework de widgets de Qt y de subprocesos de renderizado WebEngine, garantizando ergonomía de escritorio, adaptación rigurosa a la «Interfaz más fácil de usar» (Modo Senior, tap targets y alturas $\ge 48$ px según WCAG 2.1 AAA), seguridad Local-First (ADR-001 / Decisión Q-001) y estricto cumplimiento de las directrices públicas de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de densidad de tira de pestañas (`TabStripDensityTypes.h`)**:
  - `TabPinState`: Estado de fijación (`Unpinned`, `Pinned`).
  - `TabMediaState`: Estado de reproducción sonora (`None`, `AudioPlaying`, `AudioMuted`).
  - `TabDiscardState`: Estado de suspensión de memoria (`Active`, `Discarded` — «Liberar memoria de pestañas inactivas» / «Pestaña en reposo»).
  - `TabLoadingState`: Estado de carga de página (`Idle`, `Loading`, `Failed`).
  - `TabInteractiveState`: Estados interactivos de elemento (`Normal`, `Hovered`, `Selected`, `Pressed`, `Dragging`).
  - `TabStripHitTestZone`: Clasificación exhaustiva de zonas de impacto para desacoplar eventos de ratón y puntero táctil (`Nowhere`, `EmptySpace`, `TabBody`, `TabFavicon`, `TabTitle`, `TabCloseButton`, `TabAudioIndicator`, `NewTabButton`, `ScrollLeftButton`, `ScrollRightButton`, `OverflowMenuButton`).
  - `TabStripOverflowMode`: Modalidades de manejo ante exceso de pestañas (`None`, `Scrollable`, `DropdownMenu`).
  - `TabStripDensityError`: Clasificación fuertemente tipada de anomalías de layout (`None`, `WidthBelowMinimum`, `HeightBelowMinimum`, `SeniorModeMinimumHeightViolation`, `SeniorModeTapTargetViolation`, `InvalidTabDimensions`, `InvalidTabCount`, `NegativeMarginOrSpacing`, `AvailableWidthExhausted`).
  - Estructuras inmutables con semántica de valor:
    - `TabStripDimensions`: Altura de tira, ancho mínimo de tira, anchos preferidos, mínimos y máximos de pestaña regular, ancho de pestaña fijada, dimensiones de botón de nueva pestaña, tamaños de favicon, botón de cierre e indicador de audio, espaciados y umbral de compresión para ocultar botón de cierre.
    - `TabSpec`: Especificación individual de entrada de pestaña (identificador, título, URL sanitizada, fijación, selección, audio, descarte y carga).
    - `TabItemLayout`: Distribución calculada inmutable para una pestaña (rectángulo delimitador `LayoutRect`, rectángulos específicos para favicon, título, botón de cierre e indicador de audio, banderas de visibilidad, etiqueta accesible y descripción flotante).
    - `TabStripDensityConfig`: Parámetros declarativos de configuración (densidad activa, botón de nueva pestaña visible, política de desbordamiento y desplazamiento horizontal `scrollOffset`).
    - `TabStripDensityPlan`: Plan estructural inmutable calculado consolidando dimensiones totales, rectángulo contenedor, rectángulos para controles auxiliares (nueva pestaña, navegación horizontal, menú de desbordamiento), vector de pestañas calculadas (`std::vector<TabItemLayout>`), anchos efectivos asignados, estado de desbordamiento y consumo horizontal total.
    - `TabStripDensityReport`: Reporte inmutable de diagnóstico con indicador booleano de validez, error tipado, plan resultante, sugerencia de remediación y mensaje comprensible en español neutro.
- **Configuración canónica e invariantes geométricas (`TabStripDensityConfiguration`)**:
  - `canonicalDefaultDensity(int availableWidth = 1000)`: Configuración estándar de escritorio (altura 36 px, pestaña regular preferida 200 px [mín 120 px, máx 240 px], pestaña fijada 36 px, botón nueva pestaña $28\times 28$ px, favicon $16\times 16$ px, botón de cierre $16\times 16$ px, separación entre pestañas 1 px, umbral de ocultación de cierre 90 px, ancho mínimo de tira 400 px).
  - `canonicalSeniorDensity(int availableWidth = 1000)`: Configuración para la «Interfaz más fácil de usar» (altura canónica 48 px, pestaña regular preferida 220 px [mínimo estricto 140 px para legibilidad sin compresión extrema, máx 280 px], pestaña fijada $\ge 48$ px [tap target $\ge 48\times 48$ px], botón nueva pestaña $\ge 48\times 48$ px, favicon $24\times 24$ px, botón de cierre $24\times 24$ px con hit target $\ge 48\times 48$ px, separación ampliada 4 px para evitar pulsaciones accidentales, umbral de cierre 140 px para mantenerlo siempre visible, ancho mínimo de tira 500 px).
  - `canonicalCompactDensity(int availableWidth = 800)`: Configuración para pantallas de alta densidad (altura 28 px, pestaña regular preferida 160 px [mín 80 px, máx 200 px], pestaña fijada 28 px, botón nueva pestaña $24\times 24$ px, favicon $16\times 16$ px, botón de cierre $14\times 14$ px, separación 1 px, umbral de cierre 75 px, ancho mínimo de tira 360 px).
  - `verifyInvariants(const TabStripDensityPlan&)`: Verificación booleana estricta de que el plan cumple con no solapamiento entre pestañas ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$), no solapamiento interno entre elementos de cada pestaña (icono $\cap$ título = $\emptyset$, título $\cap$ cierre = $\emptyset$), contención espacial dentro de la tira, alturas mínimas por densidad y tap targets $\ge 48$ px en la «Interfaz más fácil de usar».
- **Validador estático desacoplado, algoritmo de distribución espacial y hit-testing puro (`TabStripDensityValidator`)**:
  - `validateDimensions(int availableWidth, int height, LayoutDensity density)`: Validación de límites de anchura y altura según la densidad.
  - `validatePlan(const TabStripDensityPlan&)`: Auditoría integral de congruencia del plan de la tira de pestañas.
  - `calculateTabStripLayout(int availableWidth, const TabStripDensityConfig&, const std::vector<TabSpec>&)`: Algoritmo analítico determinista que distribuye el ancho disponible entre pestañas fijadas, pestañas regulares (con compresión elástica acotada), botón de nueva pestaña y controles de desbordamiento, garantizando ausencia de solapamientos y ergonomía táctil y visual.
  - `hitTest(int x, int y, const TabStripDensityPlan&)`: Función pura que asigna cualquier par de coordenadas relativas $(x, y)$ a su correspondiente `TabStripHitTestZone`.
  - Funciones de lenguaje público y textos accesibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`:
    - `tabStripErrorToString(TabStripDensityError)`
    - `densityToString(LayoutDensity)`
    - `hitTestZoneToString(TabStripHitTestZone)`
    - `tabAccessibleLabel(const TabSpec&, bool isSelected, LayoutDensity)`
    - `tabTooltip(const TabSpec&, bool isSelected)`
    - `closeButtonAccessibleLabel(const TabSpec&)`
    - `audioButtonAccessibleLabel(bool isMuted)`
    - `newTabButtonAccessibleLabel()`
    - `generateRemediationHint(TabStripDensityError, std::string_view)`
    - `sanitizeTitle(std::string_view, bool isIncognito)`
- **Suite de pruebas unitarias trazables en CTest (`TabStripDensityTests.cpp`)**:
  - 16 pruebas unitarias en C++20 puro bajo el ejecutable `vigilio_tab_strip_density_tests` enlazado a `vigilio_core`, ejecutables en modo headless (< 50 ms), sin servidor gráfico ni widgets Qt, verificando planes canónicos, casos de desbordamiento, hit-testing exhaustivo de todas las zonas, compresión elástica de pestañas, pestañas fijadas, conmutación de densidad estándar / accesible / compacta, estados de audio y ahorro de memoria, y auditoría de copy público.

---

## Out of scope

- Implementación del servicio de persistencia y configuración de tira de pestañas en disco o SQLite (`TabStripDensityService`) (reservado para la etapa `implementation` en `module.ux-shell.task0069`).
- Integración física con widgets de Qt (`QTabBar`, `QTabWidget`, estilos QSS, `BrowserWindow.cpp`, manejo de eventos de arrastre entre pestañas `QDrag`/`QDropEvent` y animación de transiciones) (reservado para la etapa `ui` en `module.ux-shell.task0070`).
- Pruebas E2E de interacción directa con motor WebEngine, carga real de URLs en pestañas, reproducción de audio nativo y suspensión Chromium de pestañas en memoria (reservado para la etapa `tests` en `module.ux-shell.task0071`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` durante esta fase PROPOSE.
- Telemetría remota, envío de métricas de pestañas a servidores externos o sincronización en la nube (estrictamente proscrito por la arquitectura Local-First, ADR-001 / Decisión Q-001).
- Inclusión de subprocesos o dependencias de renderizado Chromium WebEngine en el modelo de dominio.

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completaron formalmente las 59 microtareas del módulo `foundations` (`task0001` a `task0059`), las 4 microtareas de `main window layout` (`task0060` a `task0063`) y las 4 microtareas de `title bar integration` (`task0064` a `task0067`), alcanzando el commit `710169b feat(ux-shell): ui title-bar-integration` y la ejecución exitosa de `task0067`.
   - `MainWindowLayoutTypes.h` definió la zona estructural `LayoutZone::TabStrip` y la enumeración `LayoutDensity` (`Compact`, `Default`, `Senior`).
   - `TabModel.h` y `TabModel.cpp` en `apps/vigilio-browser/src/core/` ya proveen la estructura de datos elemental para pestañas (`Tab` con `id`, `url`, `pinned`, `muted`), pero carecen de información espacial, dimensiones, densidades, hit-testing y reglas de compresión elástica.
   - `BrowserWindow.h` y `BrowserWindow.cpp` instancian actualmente un `QTabWidget* tabs_` genérico sin control paramétrico de densidades ni adaptación rigurosa de tap targets a la «Interfaz más fácil de usar».
2. **Brecha arquitectónica identificada (GAP)**:
   - No existe un modelo de datos analítico que desacople el dimensionamiento de pestañas del motor de dibujo de Qt.
   - No existe un algoritmo que calcule cómo deben redimensionarse las pestañas cuando la ventana se estrecha o cuando el usuario abre decenas de pestañas:
     - Cuándo comprimir pestañas entre el ancho máximo y el mínimo.
     - Cuándo preservar las pestañas fijadas (*pinned tabs*) con tamaño invariante para que sigan siendo reconocibles por su favicon.
     - Cuándo ocultar el botón de cierre en pestañas no seleccionadas para evitar cierres involuntarios al hacer clic en pestañas angostas.
     - Cuándo activar el modo de desbordamiento horizontal (*overflow*) y habilitar botones de desplazamiento o menú de lista.
     - Cómo adaptar la tira a la «Interfaz más fácil de usar» (Modo Senior), garantizando que las pestañas midan al menos 48 px de alto, los tap targets de cierre y nueva pestaña cumplan con el estándar de $\ge 48\times 48$ px, y la separación entre elementos impida pulsaciones erróneas.
     - Cómo asociar coordenadas $(x, y)$ a zonas de impacto analíticas (*hit-testing*) sin depender de eventos nativos del sistema operativo.
     - Tipos de error estructurados y sugerencias de remediación en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Muestra la barra superior y la tira de pestañas limpias, sobrias, con integración armónica entre la pestaña activa, pestañas inactivas discretas, favicon visible, botón de cierre y botón de nueva pestaña ($+$).
   - `/resultado/02-vigilio-tools-productivity.png`: Muestra la disposición de pestañas y herramientas de productividad con jerarquía visual evidente.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Muestra el comportamiento y proporciones de la «Interfaz más fácil de usar» (Modo Senior), con elementos ampliados, márgenes amplios y textos legibles.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia de identidad oficial de Vigilio Browser.
4. **Verificación de calidad actual**:
   - El sistema de pruebas en CTest compila y ejecuta 29 de 29 pruebas al 100% en ~3.66s (exit code 0) en `apps/vigilio-browser/_build`.

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `domain` está estrictamente delimitado a la modelización matemática, lógica y tipada de **tab strip density**:

1. **Creación de `apps/vigilio-browser/src/core/TabStripDensityTypes.h`**:
   - Enumeraciones tipadas `enum class : uint8_t`: `TabPinState`, `TabMediaState`, `TabDiscardState`, `TabLoadingState`, `TabInteractiveState`, `TabStripHitTestZone`, `TabStripOverflowMode`, `TabStripDensityError`.
   - Estructuras inmutables de semántica de valor: `TabStripDimensions`, `TabSpec`, `TabItemLayout`, `TabStripDensityConfig`, `TabStripDensityPlan`, `TabStripDensityReport`.
2. **Creación de `apps/vigilio-browser/src/core/TabStripDensityConfiguration.h` y `.cpp`**:
   - Constantes canónicas dimensionales para densidades Estándar (36 px), Senior (48 px) y Compacta (28 px).
   - Anchos preferidos, mínimos y máximos de pestañas regulares por densidad.
   - Dimensiones de pestañas fijadas ($36$ px estándar, $48$ px Senior, $28$ px Compact).
   - Botón de nueva pestaña ($28\times 28$ px estándar, $48\times 48$ px Senior, $24\times 24$ px Compact).
   - Separación entre pestañas (1 px estándar/compacto, 4 px Senior).
   - Umbrales de compresión para ocultación condicional del botón de cierre.
   - Factorías canónicas `canonicalDefaultDensity()`, `canonicalSeniorDensity()` y `canonicalCompactDensity()`.
   - Método estático `verifyInvariants(const TabStripDensityPlan&)` que certifica no solapamiento espacial, límites dimensionales, contención y accesibilidad.
3. **Creación de `apps/vigilio-browser/src/core/TabStripDensityValidator.h` y `.cpp`**:
   - Métodos puros de validación: `validateDimensions`, `validatePlan`.
   - Algoritmo espacial determinista `calculateTabStripLayout(int availableWidth, const TabStripDensityConfig&, const std::vector<TabSpec>&)`.
   - Función analítica de clasificación espacial `hitTest(int x, int y, const TabStripDensityPlan&)`.
   - Funciones de texto público y accesibilidad en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
   - Sanitización de títulos de pestañas para protección de privacidad en modo incógnito.
4. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Incorporación de `TabStripDensityConfiguration.cpp` y `TabStripDensityValidator.cpp` a la biblioteca `vigilio_core`.
   - Definición del nuevo target ejecutable de pruebas unitarias `vigilio_tab_strip_density_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`, etiquetado en CTest como `unit;fast`.
5. **Creación de la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`**:
   - 16 pruebas unitarias exhaustivas en C++20 puro validando el modelo de dominio en modo headless.

---

## Diseño completo de la capacidad

### 1. Entidades de dominio y modelo conceptual
- **`LayoutDensity`** (reutilizado desde `MainWindowLayoutTypes.h`):
  - `Default`: Densidad estándar de escritorio optimizada para puntero de ratón y resolución estándar.
  - `Senior`: Densidad accesible («Interfaz más fácil de usar») con tap targets $\ge 48$ px, textos ampliados y separación preventiva.
  - `Compact`: Densidad compacta optimizada para pantallas pequeñas o flujos intensivos con gran densidad de información.
- **`TabPinState`**:
  - `Unpinned`: Pestaña regular con título textual, icono, botón de cierre y compresión elástica.
  - `Pinned`: Pestaña fijada compacta, ubicada en el extremo izquierdo de la tira, que muestra exclusivamente su favicon y posee anchura fija.
- **`TabMediaState`**:
  - `None`: Sin reproducción sonora.
  - `AudioPlaying`: Pestaña emitiendo sonido; muestra indicador visual interactivo de altavoz.
  - `AudioMuted`: Pestaña silenciada; muestra altavoz tachado.
- **`TabDiscardState`**:
  - `Active`: Pestaña residente en memoria activa.
  - `Discarded`: Pestaña suspendida por el mecanismo de «Liberar memoria de pestañas inactivas» (Memory Saver / Ahorro de memoria); visualmente se muestra con opacidad sutil o icono de reposo sin perder su título ni favicon.
- **`TabStripHitTestZone`**:
  - Clasificación desacoplada para la capa de presentación:
    - `Nowhere`: Fuera de los límites de la tira de pestañas.
    - `EmptySpace`: Área libre no ocupada por pestañas (un doble clic aquí abre una nueva pestaña; arrastre mueve la ventana si está integrada).
    - `TabBody`: Superficie interactiva de la pestaña (clic la activa; arrastre permite reordenar).
    - `TabFavicon`: Área del icono de la pestaña.
    - `TabTitle`: Área del texto del título.
    - `TabCloseButton`: Botón de cierre de pestaña.
    - `TabAudioIndicator`: Botón/indicador de altavoz para silenciar/activar sonido.
    - `NewTabButton`: Botón para abrir una nueva pestaña ($+$).
    - `ScrollLeftButton`, `ScrollRightButton`: Controles de desplazamiento horizontal cuando hay desbordamiento.
    - `OverflowMenuButton`: Botón para desplegar la lista completa de pestañas abiertas.

### 2. Geometría analítica y cálculo determinista
El algoritmo `calculateTabStripLayout(int availableWidth, const TabStripDensityConfig& config, const std::vector<TabSpec>& tabs)` opera en pasos deterministas:

1. **Determinación de dimensiones base según `config.density`**:
   - Extrae `TabStripDimensions` canónicas para la densidad especificada (`Default`, `Senior`, `Compact`).
2. **Validación de precondiciones**:
   - Si `availableWidth < dimensions.minStripWidth`, emite reporte con `TabStripDensityError::WidthBelowMinimum`.
   - Si no hay pestañas, devuelve un plan válido con área libre y botón de nueva pestaña anclado.
3. **Distribución del espacio horizontal disponible**:
   - Reserva espacio fijo para el botón de nueva pestaña (`newTabButtonWidth + margin`).
   - Identifica pestañas fijadas ($N_{\text{pinned}}$) y pestañas regulares ($N_{\text{regular}}$).
   - Ancho consumido por pestañas fijadas: $W_{\text{pinned\_total}} = N_{\text{pinned}} \times (\text{pinnedTabWidth} + \text{spacing})$.
   - Espacio horizontal neto para pestañas regulares:
     $$W_{\text{net}} = \text{availableWidth} - W_{\text{pinned\_total}} - W_{\text{new\_tab}} - \text{margins}$$
   - Si $W_{\text{net}} \le 0$ y $N_{\text{regular}} > 0$:
     - Se agota el espacio horizontal: se activa `TabStripOverflowMode::Scrollable` o `DropdownMenu`, asignando a las pestañas regulares su `minTabWidth`.
   - Si $W_{\text{net}} > 0$:
     - Ancho ideal por pestaña regular: $W_{\text{ideal}} = \lfloor W_{\text{net}} / N_{\text{regular}} \rfloor - \text{spacing}$.
     - Clamping dentro de los límites canónicos:
       - Si $W_{\text{ideal}} \ge \text{preferredTabWidth}$, se asigna $\min(W_{\text{ideal}}, \text{maxTabWidth})$.
       - Si $W_{\text{ideal}} < \text{preferredTabWidth}$ pero $\ge \text{minTabWidth}$, las pestañas se comprimen uniformemente a $W_{\text{ideal}}$.
       - Si $W_{\text{ideal}} < \text{minTabWidth}$, se fija $W_{\text{tab}} = \text{minTabWidth}$, y se activa el estado de desbordamiento (`isOverflowing = true`, `overflowMode = TabStripOverflowMode::Scrollable`).
4. **Cálculo de rectángulos individuales (`TabItemLayout`)**:
   - Pestañas fijadas:
     - Ubicadas sucesivamente desde $x = \text{paddingLeft}$.
     - Rectángulo: ancho `pinnedTabWidth`, alto `dimensions.height`.
     - Favicon centrado ópticamente: $(\text{pinnedTabWidth} - \text{faviconSize})/2$.
     - Botón de cierre: oculto (`showCloseButton = false`).
   - Pestañas regulares:
     - Ubicadas a continuación del bloque de pestañas fijadas.
     - Rectángulo: ancho $W_{\text{tab}}$, alto `dimensions.height`.
     - Favicon: margen izquierdo de 8-12 px.
     - Indicador de audio: si `mediaState != None`, se ubica adyacente al favicon o antes del botón de cierre.
     - Botón de cierre:
       - En modo Senior: **siempre visible**, con hit target $\ge 48\times 48$ px.
       - En modo Estándar/Compacto: visible en la pestaña activa (`isSelected == true`); en pestañas inactivas se muestra solo si $W_{\text{tab}} \ge \text{closeButtonThresholdWidth}$ (90 px estándar, 75 px compacto). Si $W_{\text{tab}} < \text{threshold}$, se oculta para evitar cierres accidentales.
     - Título: ocupa el espacio residual entre el icono/audio y el botón de cierre con margen protector de 4 px.
5. **Ubicación del botón de nueva pestaña**:
   - Si no hay desbordamiento, se ubica inmediatamente a la derecha de la última pestaña ($x_{\text{last}} + W_{\text{last}} + 4$).
   - Si hay desbordamiento, se ancla al extremo derecho de la tira, precedido por los controles de scroll u overflow.

---

## Archivos afectados

| Archivo | Acción | Propósito técnico |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/TabStripDensityTypes.h` | Nuevo | Tipos de datos, enumeraciones, estructuras de semántica de valor inmutable y reporte de diagnóstico de la tira de pestañas. |
| `apps/vigilio-browser/src/core/TabStripDensityConfiguration.h` | Nuevo | Constantes canónicas dimensionales, factorías de planes por densidad y verificación de invariantes geométricas. |
| `apps/vigilio-browser/src/core/TabStripDensityConfiguration.cpp` | Nuevo | Implementación de factorías y comprobación estricta de invariantes en C++20 puro. |
| `apps/vigilio-browser/src/core/TabStripDensityValidator.h` | Nuevo | Declaración de validador estático, algoritmo de distribución analítica, hit-testing y utilidades de UX copy accesible. |
| `apps/vigilio-browser/src/core/TabStripDensityValidator.cpp` | Nuevo | Implementación del algoritmo de cálculo espacial, hit-testing y funciones públicas en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Registro de nuevos fuentes en `vigilio_core` y definición del target ejecutable de pruebas `vigilio_tab_strip_density_tests`. |
| `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp` | Nuevo | Suite de 16 pruebas unitarias de especificación en CTest. |
| `docs/rules/01-modules/ux-shell/micro/0068-tab-strip-density/task.md` | Modificación | Actualización de estado formal tras la etapa domain. |
| `openspec/changes/module-ux-shell-task0068/proposal.md` | Modificación | Propuesta técnica integral y exhaustiva (este documento). |
| `openspec/changes/module-ux-shell-task0068/design.md` | Modificación | Documento de diseño técnico, estados CURRENT/TARGET/GAP, compatibilidad y rollback. |
| `openspec/changes/module-ux-shell-task0068/tasks.md` | Modificación | Desglose ordenado de tareas verificables con comandos y criterios. |
| `openspec/changes/module-ux-shell-task0068/execution-memory.md` | Modificación | Memoria de ejecución, registro de skills, invariantes, riesgos y handoff. |

---

## Contrato UX/UI

### 1. Directrices públicas de producto y terminología aprobada
De conformidad absoluta con `docs/UX-COPY-PUBLICO.md`:
- Queda terminantemente proscrito exponer en etiquetas, tooltips, descripciones o reportes diagnósticos las expresiones técnicas: «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter», «toolchain», «runner».
- La denominación pública y obligatoria para la modalidad accesible es **«Interfaz más fácil de usar»**.
- La funcionalidad de suspensión de pestañas para ahorrar memoria se denomina **«Liberar memoria de pestañas inactivas»** (o «Pestaña en reposo»), jamás «Tab discard» ni «Memory Saver».
- Textos y etiquetas oficiales de los componentes de la tira de pestañas:
  - **Pestaña normal**:
    - Etiqueta accesible: `Pestaña: {título}` (o `Pestaña activa: {título}` si está seleccionada).
    - Pestaña fijada: `Pestaña fijada: {título}`.
    - Pestaña en reposo: `Pestaña en reposo: {título}. Haz clic para activarla.`
    - Pestaña reproduciendo sonido: `Pestaña: {título} (reproduciendo audio)`.
  - **Botón de cierre de pestaña**:
    - Etiqueta pública: `Cerrar`.
    - Etiqueta accesible: `Cerrar pestaña {título}`.
    - Tooltip: `Cerrar esta pestaña`.
  - **Botón de control de audio / silenciar**:
    - Si está reproduciendo: Etiqueta `Silenciar pestaña`, tooltip `Silenciar el audio de esta pestaña`.
    - Si está silenciada: Etiqueta `Reactivar audio`, tooltip `Reactivar el audio de esta pestaña`.
  - **Botón de nueva pestaña**:
    - Etiqueta pública: `Nueva pestaña`.
    - Etiqueta accesible: `Abrir una nueva pestaña`.
    - Tooltip: `Abrir una nueva pestaña (Ctrl+T)`.
  - **Controles de desplazamiento / desbordamiento**:
    - Desplazar a la izquierda: `Desplazar pestañas hacia la izquierda`.
    - Desplazar a la derecha: `Desplazar pestañas hacia la derecha`.
    - Lista de pestañas: `Ver todas las pestañas abiertas`.

### 2. Especificación visual y Modo Senior («Interfaz más fácil de usar»)
- **Modo Estándar (Default)**:
  - Altura de tira: 36 px.
  - Pestaña regular: preferida 200 px, mín 120 px, máx 240 px.
  - Pestaña fijada: 36 px de ancho.
  - Icono de pestaña (favicon): $16\times 16$ px.
  - Botón de cierre: $16\times 16$ px.
  - Botón de nueva pestaña: $28\times 28$ px.
  - Separación entre pestañas: 1 px.
  - Tipografía: Inter / fuente del sistema a 12 px regular, colores sobrios sobre azul oscuro de fondo.
- **Modo Senior («Interfaz más fácil de usar»)**:
  - Altura de tira: 48 px (cumplimiento estricto del criterio WCAG 2.1 AAA de tap targets $\ge 48$ px).
  - Pestaña regular: preferida 220 px, mínimo ineludible 140 px (garantiza lectura del título y previene compresión extrema), máx 280 px.
  - Pestaña fijada: ancho $\ge 48$ px (tap target $\ge 48\times 48$ px).
  - Icono de pestaña (favicon): escalado a $24\times 24$ px para máxima claridad visual.
  - Botón de cierre: $24\times 24$ px visual con área de hit-testing de $\ge 48\times 48$ px, siempre visible.
  - Botón de nueva pestaña: $\ge 48\times 48$ px.
  - Separación entre pestañas: 4 px de margen protector que evita pulsaciones accidentales con dedos o punteros imprecisos.
  - Tipografía: Inter / fuente del sistema a 14 px semi-bold con contraste de color reforzado.
  - Indicador de foco visible: Borde perimetral continuo de 2 px de alto contraste al navegar con teclado (`Tab` / flechas horizontales).
- **Modo Compacto (Compact)**:
  - Altura de tira: 28 px.
  - Pestaña regular: preferida 160 px, mín 80 px, máx 200 px.
  - Pestaña fijada: 28 px.
  - Icono de pestaña: $16\times 16$ px.
  - Botón de cierre: $14\times 14$ px.
  - Botón de nueva pestaña: $24\times 24$ px.
  - Separación entre pestañas: 1 px.

---

## Reglas y fallos

### 1. Invariantes y precondiciones
- **Invariante de no solapamiento exterior**: Ningún par de pestañas en la tira puede solaparse espacialmente ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$).
- **Invariante de no solapamiento interior**: Dentro de una pestaña, el favicon, el título y el botón de cierre ocupan regiones disjuntas ($\text{faviconRect} \cap \text{titleRect} = \emptyset$, $\text{titleRect} \cap \text{closeRect} = \emptyset$).
- **Invariante de orden de pestañas fijadas**: Todas las pestañas fijadas preceden en el eje horizontal $X$ a las pestañas regulares ($x_{\text{pinned}} < x_{\text{regular}}$).
- **Invariante de altura accesible**: En la «Interfaz más fácil de usar», la altura de la tira no puede ser inferior a 48 px bajo ninguna circunstancia.
- **Invariante de tap target accesible**: En la «Interfaz más fácil de usar», los controles interactivos individuales (pestañas fijadas, botón de nueva pestaña y botón de cierre) cumplen `width >= 48` y `height >= 48` px en su área de captura.
- **Invariante de estabilidad de pestaña fijada**: Las pestañas fijadas no sufren compresión elástica ante el aumento del número de pestañas regulares.

### 2. Taxonomía de errores tipados (`TabStripDensityError`) y recuperación
- `None`: Configuración o cálculo plenamente válido.
- `WidthBelowMinimum`: El ancho total disponible es inferior al mínimo requerido ($< 400$ px estándar / $< 500$ px Senior / $< 360$ px Compact).
- `HeightBelowMinimum`: La altura provista es inferior al mínimo requerido.
- `SeniorModeMinimumHeightViolation`: Se intenta configurar una tira en Modo Senior con altura menor a 48 px.
- `SeniorModeTapTargetViolation`: Se violan las dimensiones mínimas de tap target en modo accesible.
- `InvalidTabDimensions`: Dimensiones de configuración absurdas (`minWidth > maxWidth`, valores $\le 0$).
- `InvalidTabCount`: Cantidad de pestañas inválida o conteo incoherente.
- `NegativeMarginOrSpacing`: Márgenes o espaciados con valores negativos.
- `AvailableWidthExhausted`: El ancho disponible es inferior a la suma requerida para las pestañas fijadas y el botón de nueva pestaña.

En caso de cualquier anomalía geométrica, `calculateTabStripLayout` no lanza excepciones no capturadas ni aborta el hilo: emite un `TabStripDensityReport` con `valid = false`, el código de error tipado, una sugerencia de remediación comprensible en español neutro generada por `generateRemediationHint`, y un plan de fallback seguro con dimensiones degradadas.

---

## Seguridad y privacidad

- **Arquitectura Local-First estricta (ADR-001 / Decisión Q-001)**:
  - Todo el cómputo de la tira de pestañas opera 100% en local en memoria de proceso C++20.
  - Cero llamadas a servidores remotos, cero almacenamiento en la nube y cero dependencias de telemetría.
- **Aislamiento en navegación privada / incógnito**:
  - En modo incógnito, los títulos de las pestañas no exponen credenciales embebidas (`user:pass@host`), parámetros sensibles ni hashes de sesión.
  - La función `sanitizeTitle` filtra proactivamente cualquier dato potencialmente sensible antes de construir las etiquetas y representaciones del dominio.
  - El modelo de dominio no escribe archivos temporales ni persistencia en disco durante sesiones privadas.
- **Prevención de colapso por desbordamiento y consumo de RAM**:
  - El algoritmo de cálculo opera en $O(N)$ donde $N$ es el número de pestañas, sin asignaciones dinámicas recurrentes innecesarias.
  - Desacoplamiento total de Chromium WebEngine: la evaluación de la tira de pestañas durante las pruebas se realiza sin instanciar subprocesos de renderizado ni consumir presupuestos de memoria gráfica.

---

## Matriz de pruebas

| Identificador | Nivel | Propósito de la prueba | Fixture / Entrada | Resultado esperado |
| --- | --- | --- | --- | --- |
| `testCanonicalDefaultInvariants` | Unitario | Invariantes geométricas de la densidad estándar por defecto | Ancho $1000$ px, 4 pestañas | Altura 36 px, pestañas 200 px, no solapamiento, invariants pass |
| `testCanonicalSeniorInvariants` | Unitario | Invariantes geométricas de la «Interfaz más fácil de usar» | Ancho $1000$ px, 3 pestañas | Altura $\ge 48$ px, tap targets $\ge 48$ px, separación 4 px |
| `testCanonicalCompactInvariants` | Unitario | Invariantes geométricas de la densidad compacta | Ancho $800$ px, 5 pestañas | Altura 28 px, pestañas 160 px, espacio de cierre reducido |
| `testPinnedTabsGeometricSeparation` | Unitario | Pestañas fijadas en extremo izquierdo y tamaño invariante | 2 fijadas + 3 regulares | Fijadas a la izquierda, ancho fijo 36 px, no solapamiento |
| `testElasticShrinkingUnderHighTabCount` | Unitario | Compresión elástica progresiva de pestañas regulares | 10 pestañas regulares en 1000 px | Pestañas se comprimen uniformemente entre 120 px y 200 px |
| `testOverflowDetectionAndModeActivation` | Unitario | Detección de desbordamiento horizontal y activación de scroll | 20 pestañas regulares en 1000 px | Se activa `isOverflowing = true`, `overflowMode = Scrollable` |
| `testCloseButtonVisibilityThreshold` | Unitario | Ocultación de botón de cierre en pestañas angostas inactivas | Pestañas comprimidas a 85 px (< 90 px) | Inactivas ocultan cierre, activa lo mantiene |
| `testSeniorModeCloseButtonPermanence` | Unitario | Permanencia obligatoria del botón de cierre en Modo Senior | Pestañas en Modo Senior comprimidas al mínimo (140 px) | Botón de cierre siempre visible con hit target $\ge 48$ px |
| `testSubMinimumWidthViolation` | Unitario | Rechazo de anchuras inferiores al mínimo canónico | Ancho 350 px en modo estándar (< 400 px) | Retorna `WidthBelowMinimum`, `valid = false` |
| `testSeniorHeightViolation` | Unitario | Rechazo de altura $< 48$ px en la «Interfaz más fácil de usar» | Configuración Senior con altura 36 px | Retorna `SeniorModeMinimumHeightViolation` |
| `testExhaustiveHitTestingZones` | Unitario | Clasificación precisa de coordenadas relativas en zonas | Coordenadas en favicon, título, cierre, nueva pestaña, espacio libre | Identificación exacta de cada `TabStripHitTestZone` |
| `testHitTestingNowhereAndBoundaries` | Unitario | Hit-testing en puntos exteriores o fuera de la tira | Coordenadas $(x < 0, y < 0, y > \text{height})$ | Retorna `TabStripHitTestZone::Nowhere` |
| `testMediaIndicatorStateAndHitTest` | Unitario | Posicionamiento de icono de altavoz y hit-testing de mute | Pestaña con `TabMediaState::AudioPlaying` | `audioIndicatorRect` asignado, hit-test retorna `TabAudioIndicator` |
| `testTabDiscardStateRepresentation` | Unitario | Representación y etiquetas de pestañas en ahorro de memoria | Pestaña con `TabDiscardState::Discarded` | Etiqueta accesible incluye «Pestaña en reposo» |
| `testTitleSanitizationForPrivacy` | Unitario | Sanitización de títulos en modo privado/incógnito | Título con credenciales `user:pass@host/secret` | Retorna versión limpia sin tokens ni credenciales |
| `testUxCopySpanishCompliance` | Unitario | Auditoría exhaustiva frente a `docs/UX-COPY-PUBLICO.md` | Todas las cadenas generadas por el validador | 0 ocurrencias de términos proscritos; uso de «Interfaz más fácil de usar» |

---

## Pasos APPLY

1. **Crear `apps/vigilio-browser/src/core/TabStripDensityTypes.h`**:
   - Declarar las enumeraciones `TabPinState`, `TabMediaState`, `TabDiscardState`, `TabLoadingState`, `TabInteractiveState`, `TabStripHitTestZone`, `TabStripOverflowMode`, `TabStripDensityError`.
   - Declarar las estructuras de valor `TabStripDimensions`, `TabSpec`, `TabItemLayout`, `TabStripDensityConfig`, `TabStripDensityPlan`, `TabStripDensityReport`.
2. **Crear `apps/vigilio-browser/src/core/TabStripDensityConfiguration.h` y `.cpp`**:
   - Definir las constantes canónicas para densidades Estándar, Senior y Compacta.
   - Implementar las factorías `canonicalDefaultDensity()`, `canonicalSeniorDensity()` y `canonicalCompactDensity()`.
   - Implementar el método estático de verificación geométrica `verifyInvariants()`.
3. **Crear `apps/vigilio-browser/src/core/TabStripDensityValidator.h` y `.cpp`**:
   - Declarar e implementar los métodos de validación `validateDimensions` y `validatePlan`.
   - Implementar el algoritmo determinista de distribución analítica `calculateTabStripLayout`.
   - Implementar la función de clasificación analítica de coordenadas `hitTest`.
   - Implementar las funciones de lenguaje público y accesibilidad en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
   - Implementar la función de sanitización de títulos `sanitizeTitle`.
4. **Modificar `apps/vigilio-browser/CMakeLists.txt`**:
   - Agregar `src/core/TabStripDensityConfiguration.cpp` y `src/core/TabStripDensityValidator.cpp` a la biblioteca `vigilio_core`.
   - Declarar el target ejecutable de pruebas unitarias `vigilio_tab_strip_density_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar la prueba en CTest con las etiquetas `unit;fast` y timeout de 15 segundos.
5. **Crear `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`**:
   - Implementar la suite de 16 pruebas unitarias de especificación en C++20 puro.
6. **Ejecutar el pipeline de calidad**:
   - Formatear con `clang-format` sobre los nuevos archivos.
   - Compilar con CMake y `-Werror`.
   - Ejecutar la suite completa en CTest (esperando 30/30 pruebas pasando).
7. **Sincronizar el documento de microtarea**:
   - Actualizar `docs/rules/01-modules/ux-shell/micro/0068-tab-strip-density/task.md` documentando la finalización de la etapa `domain`.

---

## Definición de terminado

- [ ] `TabStripDensityTypes.h` define todos los tipos, enums y estructuras inmutables con semántica de valor en C++20 puro.
- [ ] `TabStripDensityConfiguration` define las constantes canónicas, factorías para las 3 densidades y verificación estricta de invariantes.
- [ ] `TabStripDensityValidator` implementa el cálculo analítico de distribución espacial, hit-testing desacoplado y textos en español neutro.
- [ ] Ninguna cadena pública contiene expresiones proscritas («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», etc.); la denominación oficial es «Interfaz más fácil de usar».
- [ ] `CMakeLists.txt` registra los nuevos fuentes en `vigilio_core` y el target ejecutable `vigilio_tab_strip_density_tests`.
- [ ] Las 16 pruebas unitarias compilan limpiamente bajo C++20 y pasan al 100% en CTest en modo headless (< 1 segundo).
- [ ] El pipeline general de CTest ejecuta 30 pruebas con 0 fallos (exit code 0).
- [ ] `task.md` queda sincronizado con el estado formal del pack.

---

## Dependencias y riesgos

- **Dependencias ejecutables previas**:
  - `module.ux-shell.task0067` (completada y certificada formalmente con 29 pruebas pasando al 100%).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Acoplamiento prematuro con clases de widgets de Qt (`QTabBar`, `QTabWidget`) en la etapa de dominio.
    *Mitigación*: Mantener `TabStripDensityTypes`, `TabStripDensityConfiguration` y `TabStripDensityValidator` en C++20 puro dentro de `vigilio_core`, sin incluir cabeceras de Qt GUI ni WebEngine.
  - *Riesgo*: Compresión excesiva de pestañas en resoluciones angostas provocando ilegibilidad o pulsaciones accidentales.
    *Mitigación*: Imposición de un ancho mínimo ineludible (`minTabWidth`, 120 px estándar, 140 px Senior) y activación obligatoria del estado de desbordamiento horizontal (`TabStripOverflowMode::Scrollable`).
  - *Riesgo*: Violación de directrices de accesibilidad en la «Interfaz más fácil de usar».
    *Mitigación*: Validación programática automática en `verifyInvariants` exigiendo altura $\ge 48$ px, tap targets $\ge 48\times 48$ px y separación protectora de 4 px entre pestañas.

---

## Commit propuesto

`docs(ux-shell): domain tab-strip-density`
