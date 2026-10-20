# Change Proposal — module-ux-shell-task0069

Pack: `module.ux-shell.task0069`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **tab strip density** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0069 — Implementar la operación en una capa propietaria: tab strip density`).

Habiéndose completado y certificado formalmente en las microtareas precedentes del módulo `foundations` (`task0001`-`task0059`) la base de compilación estricta C++20, modularización CMake, integración de recursos vectoriales QRC, ciclo de vida, presets, test fixtures y CI multiplataforma; en las microtareas previas de `ux-shell` (`task0060`-`task0067`) el diseño estructural de la ventana principal (**main window layout**) y la barra de título integrada (**title bar integration**); y en la microtarea `task0068` el modelo de dominio analítico, taxonomía formal, configuraciones canónicas y validador de **tab strip density** (`TabStripDensityTypes.h`, `TabStripDensityConfiguration`, `TabStripDensityValidator`), este pack aborda la implementación de la capa operativa propietaria: el servicio `TabStripDensityService`.

El servicio `TabStripDensityService` es el componente responsable del ciclo de vida operativo, mutaciones deterministas de la colección de pestañas, ajuste dinámico por densidad visual («Interfaz más fácil de usar» / estándar / compacta), reconfiguración por anchura de ventana, gestión de pestañas fijadas (*pinned tabs*) con invariante de precedencia espacial, conmutación y silenciamiento de audio, transiciones de ahorro de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo»), detección y desplazamiento en desbordamiento horizontal (*overflow*), soporte de cancelación cooperativa bajo la política Zero-Zombie (`CancellationToken`), persistencia atómica local y recuperación resiliente ante corrupción en disco, preservando el aislamiento absoluto de sesiones privadas o en modo incógnito de conformidad estricta con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Capa operativa propietaria desacoplada (`TabStripDensityService.h` y `TabStripDensityService.cpp`)**:
  - Gestión atómica y multihilo del estado de la tira de pestañas con sincronización mediante `std::mutex`.
  - Inicialización flexible: construcción con configuración canónica por defecto (`canonicalDefaultDensity()`), accesible («Interfaz más fácil de usar», altura $\ge 48$ px, tap targets $\ge 48\times 48$ px), compacta (altura 28 px) o inyección declarativa de un `TabStripDensityConfig` y colección inicial de pestañas.
  - Conmutación de densidades visuales (`setDensity(LayoutDensity)`): transición determinista recalculando dimensiones de pestañas regulares ($200$ px preferido / $120$ px mín en estándar vs. $220$ px preferido / $140$ px mín en «Interfaz más fácil de usar» vs. $160$ px preferido / $80$ px mín en compacta), pestañas fijadas ($36$ px estándar vs. $\ge 48$ px Senior vs. $28$ px compacta), alturas de tira ($36$ px estándar vs. $48$ px Senior vs. $28$ px compacta) y áreas de captura de botones ($\ge 48\times 48$ px en Modo Senior).
  - Gestión integral de la colección de pestañas:
    - Adición e inserción de pestañas (`addTab`, `insertTab`), preservando invariante de precedencia de pestañas fijadas ($x_{\text{pinned}} < x_{\text{regular}}$).
    - Cierre y remoción (`removeTab`), con reasignación inteligente de selección a la pestaña adyacente más próxima (derecha preferente, o izquierda si se cerró la última).
    - Reordenamiento dentro de particiones (`moveTab`), impidiendo mezclas arbitrarias sin cambio formal de estado de fijación.
    - Activación y selección (`selectTab`), garantizando visibilidad incondicional del botón de cierre en la pestaña activa.
    - Alternancia de fijación (`setTabPinned`), reubicando la pestaña a la sección de anclaje con ancho canónico invariante.
    - Control de estados multimedia (`setTabMediaState`, `toggleTabAudioMute`), actualizando la visibilidad y coordenadas del botón de control de sonido.
    - Control de suspensión de memoria (`setTabDiscardState`), actualizando estados para «Liberar memoria de pestañas inactivas» / «Pestaña en reposo» sin perder metadatos.
    - Saneamiento y actualización de títulos (`updateTabTitle`), eliminando credenciales embebidas, tokens y query strings privadas en URLs.
  - Gestión de desbordamiento horizontal y desplazamiento (`setOverflowMode`, `setScrollOffset`):
    - Activación de `isOverflowing` cuando el ancho requerido supera el ancho disponible de la tira.
    - Soporte para modo de desplazamiento continuo (`Scrollable`) con cálculo de offset acotado y controles de scroll.
    - Soporte para modo de menú desplegable (`DropdownMenu`).
  - Redimensionamiento y recálculo analítico (`setAvailableWidth(int)`, `computeLayout(CancellationToken* cancel = nullptr)`):
    - Ajuste dinámico frente al ancho de la ventana anfitriona, comprobación de umbrales mínimos ($W \ge 400$ px estándar, $W \ge 500$ px Senior, $W \ge 360$ px Compact).
    - Aborto cooperativo si el token de cancelación es señalado (`cancel->isCancelled()`).
  - Clasificación de coordenadas cartesianas relativas (`hitTest(int x, int y)`, `tabAtPoint(int x, int y)`):
    - Delegación thread-safe al validador cartesiano para clasificar clics y áreas de interacción (`TabStripHitTestZone`) en tiempo constante $\mathcal{O}(1)$.
    - Localización rápida de pestaña impactada por identificador único.
  - Persistencia local y resiliencia de estado en disco (Local-First):
    - `saveStateToFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: serialización determinista en formato estructurado JSON con escritura atómica mediante archivo temporal (`.tmp`) y reemplazo seguro (`std::filesystem::rename`).
    - `loadStateFromFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: deserialización con validación exhaustiva de parámetros y límites dimensionales.
    - Resiliencia y recuperación automática ante corrupción: si el archivo de estado en disco está truncado, malformado o contiene valores fuera de rango, el servicio restaura automáticamente la configuración canónica por defecto sin interrumpir la aplicación ni arrojar excepciones.
    - Aislamiento absoluto de navegación en modo incógnito (`setIncognitoMode(bool)` / `isIncognitoMode()`): el estado de la tira de pestañas en modo privado permanece estrictamente en memoria volátil; cualquier invocación a guardar en disco es rechazada de inmediato retornando `false` sin escribir ningún byte.
  - Observabilidad y notificaciones desacopladas:
    - Mecanismo de suscripción (`setTabStripChangedCallback(OnTabStripChangedCallback)`) que permite a la interfaz Qt en la futura etapa UI (`module.ux-shell.task0070`) reaccionar a cambios de geometría y estado sin acoplamiento circular.
    - Formateo de resúmenes operativos de tira de pestañas y diagnósticos en español neutro cumpliendo estrictamente `docs/UX-COPY-PUBLICO.md` («Interfaz más fácil de usar», «Liberar memoria de pestañas inactivas», «Pestaña en reposo», «Ahorro de memoria»).
- **Integración en el sistema de construcción CMake (`CMakeLists.txt`)**:
  - Incorporación de `src/core/TabStripDensityService.cpp` a la biblioteca central `vigilio_core`.
  - Declaración del ejecutable de pruebas unitarias `vigilio_tab_strip_density_service_tests` vinculado contra `vigilio_core` y `vigilio_compiler_warnings`.
  - Registro de la prueba `tab_strip_density_service_tests` en CTest bajo las etiquetas `unit;fast` con un timeout estricto de 15 segundos.
- **Suite de pruebas unitarias de servicio (`tests/unit/TabStripDensityServiceTests.cpp`)**:
  - 17 pruebas unitarias exhaustivas en C++20 puro, evaluando inicialización, mutaciones de colección de pestañas, selección y adyacencia, transiciones de densidad estándar / Senior / compacta, fijación y precedencia geométrica, silenciamiento de audio, ahorro de memoria («Pestaña en reposo»), sanitización de privacidad, redimensionamiento elástico, desbordamiento horizontal, hit-testing espacial $\mathcal{O}(1)$, cancelación cooperativa Zero-Zombie, serialización atómica en disco, tolerancia a archivos corruptos, aislamiento de incógnito y observabilidad en español neutro.

---

## Out of scope

- Integración física con widgets de Qt (`QTabBar`, `QTabWidget`, `TabStripWidget.cpp`, `BrowserWindow.cpp`, estilos QSS, animaciones visuales de desplazamiento de pestañas, delegados de pintura `paintEvent`, o manejo de eventos de arrastre entre pestañas `QDrag`/`QDropEvent`) (reservado estrictamente para la etapa `ui` en `module.ux-shell.task0070`).
- Pruebas E2E completas con gestor de ventanas del sistema operativo, carga física de contenidos en WebEngine, renderizado de páginas reales y pruebas de estrés multiventana (reservado para la etapa `tests` en `module.ux-shell.task0071`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` o creación de archivos en `apps/**` durante esta fase PROPOSE (cualquier cambio en `apps/**` está estrictamente reservado para la fase APPLY tras aprobación humana).
- Conexión con servicios en la nube, sincronización de pestañas remota o telemetría de navegación (estrictamente prohibido por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Integración de procesos de renderizado Chromium WebEngine en el servicio de tira de pestañas (la tira opera en el shell exterior de la ventana).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completaron formalmente las 59 microtareas del módulo `foundations` (`task0001` a `task0059`), las 4 microtareas de `main window layout` (`task0060` a `task0063`) y las 4 microtareas de `title bar integration` (`task0064` a `task0067`).
   - Se completó formalmente la etapa de dominio de `tab strip density` en `module.ux-shell.task0068`, estableciendo los tipos de dominio (`TabStripDensityTypes.h`), la configuración canónica (`TabStripDensityConfiguration.h/.cpp`), el validador espacial analítico (`TabStripDensityValidator.h/.cpp`) y la suite de pruebas unitarias de especificación (`TabStripDensityTests.cpp`).
   - La suite de CTest ejecuta actualmente 30 de 30 pruebas pasando al 100% en ~3.68 segundos sin advertencias con `-Werror` activado en GCC 15.2.0 bajo el arnés `apps/vigilio-browser/scripts/check.sh`.
2. **Inspección de servicios de core existentes (`apps/vigilio-browser/src/core/`)**:
   - Se analizó el servicio análogo `TitleBarService` implementado en `task0065`:
     - Utiliza un mutex interno `mutable std::mutex mutex_` para sincronización multihilo de todas las lecturas y mutaciones.
     - Aplica un parser JSON embebido ligero y robusto para serialización y deserialización sin dependencias externas pesadas.
     - Implementa escritura atómica con archivo temporal (`.tmp`) y renombramiento seguro para proteger la integridad de los datos en disco ante cortes de energía o caídas de proceso.
     - Protege el aislamiento de incógnito denegando escrituras en disco cuando `isIncognitoSession` está activado.
     - Provee callbacks desacoplados `std::function` y formateadores de diagnósticos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
   - Se analizó el servicio base `MainWindowLayoutService` implementado en `task0061`:
     - Establece el patrón de desacoplamiento entre modelo, opciones de servicio y vistas consumidoras.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Refleja la tira de pestañas integrada del navegador con fondo azul oscuro sobrio, pestañas inactivas discretas, pestaña activa destacada con contraste equilibrado, botón de nueva pestaña ($+$) y área despejada.
   - `/resultado/02-vigilio-tools-productivity.png`: Refleja la disposición de pestañas junto a herramientas de productividad.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Demuestra las proporciones y requerimientos de la «Interfaz más fácil de usar» (Modo Senior), con botones de acción grandes ($\ge 48\times 48$ px), altura de tira ampliada ($\ge 48$ px), separación generosa (4 px) y legibilidad superior sin compresión extrema.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia de identidad oficial de Vigilio Browser.
4. **Verificación de calidad y pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con CMake 3.24+ y Ninja bajo C++20 con `-Werror`.
   - 30 de 30 pruebas unitarias e integración pasando en CTest (exit code 0).

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `implementation` comprende exclusivamente:
1. **Creación de `apps/vigilio-browser/src/core/TabStripDensityService.h`**:
   - Declaración de la clase `TabStripDensityService` con métodos thread-safe de ciclo de vida, mutación de colección de pestañas, cálculo de layout, hit-testing, persistencia local atómica y suscripción de eventos.
   - Definición de la estructura de opciones operativas `TabStripDensityServiceOptions` y el tipo de callback `OnTabStripChangedCallback`.
2. **Creación de `apps/vigilio-browser/src/core/TabStripDensityService.cpp`**:
   - Implementación C++20 robusta, thread-safe y desacoplada de GUI.
   - Gestión de transiciones entre configuraciones canónicas (`canonicalDefaultDensity`, `canonicalSeniorDensity`, `canonicalCompactDensity`).
   - Gestión de colección de pestañas (adición, inserción, eliminación con selección adyacente, reordenamiento, fijación, conmutación de audio, ahorro de memoria «Pestaña en reposo»).
   - Saneamiento riguroso de títulos y URLs para protección de privacidad.
   - Mecanismo de persistencia local atómica (archivo temporal `.tmp` y reemplazo atómico seguro).
   - Parser JSON embebido y deserialización con tolerancia a fallos, validación exhaustiva y fallback seguro a valores por defecto ante corrupción.
   - Aislamiento incondicional de modo incógnito / navegación privada.
   - Integración de tokens de cancelación cooperativa `CancellationToken` bajo la política Zero-Zombie.
   - Conversión de resúmenes operativos a español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Adición de `src/core/TabStripDensityService.cpp` a la biblioteca `vigilio_core`.
   - Declaración del ejecutable de pruebas unitarias `vigilio_tab_strip_density_service_tests` vinculado a `vigilio_core` y `vigilio_compiler_warnings`.
   - Registro en CTest de `tab_strip_density_service_tests` con propiedades `LABELS "unit;fast"` y `TIMEOUT 15`.
4. **Creación de `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`**:
   - 17 pruebas unitarias independientes evaluando inicialización, mutaciones, transiciones de densidad, selección adyacente, fijación, audio, descarte de memoria, privacidad, redimensionamiento, desbordamiento, hit-testing, persistencia atómica, tolerancia a corrupción, modo incógnito, cancelación Zero-Zombie y copy público.

---

## Diseño completo de la capacidad

### Entidades y responsabilidades
- `TabStripDensityService`: Autoridad operativa central en memoria para la tira de pestañas. Gestiona la configuración activa (`TabStripDensityConfig`), el ancho disponible actual (`availableWidth`), la colección de pestañas (`std::vector<TabSpec>`) y el plan calculado inmutable (`TabStripDensityPlan`).
- `TabStripDensityServiceOptions`: Parámetros operativos inyectables: validación automática ante mutaciones (`autoValidateOnMutation`), habilitación de persistencia (`enablePersistence`), sesión en modo incógnito (`isIncognitoSession`) y ruta del archivo de estado (`stateFilePath`).
- `OnTabStripChangedCallback`: Función receptora `std::function<void(const TabStripDensityPlan&, const TabStripDensityReport&)>` que notifica a observadores externos ante cualquier mutación validada del layout o estado de la tira.

### Métodos principales
- `setDensity(LayoutDensity density)`: Aplica las dimensiones canónicas de la densidad objetivo. Al conmutar a `LayoutDensity::Senior`, amplía la altura a 48 px, los anchos de pestañas fijadas a $\ge 48$ px, botones de acción a $\ge 48\times 48$ px, separación a 4 px y ancho mínimo regular a 140 px.
- `setOverflowMode(TabStripOverflowMode mode)`: Configura el comportamiento ante desbordamiento (`Scrollable` o `DropdownMenu`).
- `setScrollOffset(int offset)`: Ajusta el desplazamiento horizontal de visualización cuando hay desbordamiento.
- `setShowNewTabButton(bool show)`: Controla la visibilidad del botón de apertura de nueva pestaña.
- `addTab(const TabSpec& tab)`: Incorpora una pestaña respetando la partición de fijadas/regulares, recalculando la geometría y notificando observadores.
- `insertTab(size_t index, const TabSpec& tab)`: Inserta una pestaña en la posición especificada, validando invariantes espaciales.
- `removeTab(uint64_t tabId)`: Cierra la pestaña indicada y selecciona deterministamente la pestaña contigua si la cerrada estaba activa.
- `moveTab(uint64_t tabId, size_t newIndex)`: Desplaza la pestaña dentro de su segmento correspondiente sin violar la invariante de precedencia fijada.
- `selectTab(uint64_t tabId)`: Establece la pestaña activa, forzando la visibilidad de su botón de cierre.
- `setTabPinned(uint64_t tabId, bool pinned)`: Alterna la condición de fijada, ajustando su ancho al canónico inmutable y reubicándola en la sección izquierda.
- `setTabMediaState(uint64_t tabId, TabMediaState state)`: Actualiza la reproducción de audio y la posición del indicador sonoro.
- `toggleTabAudioMute(uint64_t tabId)`: Conmuta entre reproducción de sonido y silencio.
- `setTabDiscardState(uint64_t tabId, TabDiscardState state)`: Activa o desactiva la suspensión de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo»).
- `updateTabTitle(uint64_t tabId, std::string_view title, std::string_view url)`: Sanitiza cadenas entrantes y actualiza metadatos y tooltips.
- `setAvailableWidth(int availableWidth)`: Recalcula analíticamente la distribución frente al nuevo ancho de la ventana, detectando desbordamiento.
- `hitTest(int x, int y) const`: Clasificación espacial en $\mathcal{O}(1)$ que asigna coordenadas a zonas de impacto (`TabStripHitTestZone`).
- `tabAtPoint(int x, int y) const`: Identifica el ID de la pestaña bajo las coordenadas dadas.
- `saveStateToFile(const std::filesystem::path& path, CancellationToken* cancel)`: Serialización atómica a disco con archivo temporal `.tmp`.
- `loadStateFromFile(const std::filesystem::path& path, CancellationToken* cancel)`: Deserialización y recuperación resiliente con fallback a valores canónicos.
- `setIncognitoMode(bool incognito)`: Aislamiento absoluto de navegación privada que bloquea toda escritura a disco.

---

## Archivos afectados

| Archivo | Tipo | Acción propuesta en APPLY | Justificación técnica |
| --- | --- | --- | --- |
| `apps/vigilio-browser/src/core/TabStripDensityService.h` | C++ Header | Crear | Declaración pública del servicio desacoplado, opciones y callbacks. |
| `apps/vigilio-browser/src/core/TabStripDensityService.cpp` | C++ Source | Crear | Implementación thread-safe del servicio, mutaciones, persistencia atómica y tolerancia a fallos. |
| `apps/vigilio-browser/CMakeLists.txt` | Build script | Modificar | Incorporar `TabStripDensityService.cpp` a `vigilio_core` y registrar target de prueba en CTest. |
| `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp` | C++ Test | Crear | Batería de 17 pruebas unitarias independientes verificando servicio, invariantes y copy público. |
| `docs/rules/01-modules/ux-shell/micro/0069-tab-strip-density/task.md` | Documentación | Actualizar | Sincronización de estado de la microtarea al completar la etapa implementation. |

---

## Contrato UX/UI

- **Declaración visual en esta etapa**: La etapa `implementation` opera estrictamente en la capa de servicio C++20 (`vigilio_core`) sin renderizado gráfico directo de Qt Widgets; la integración física de widgets (`QTabBar`/`BrowserWindow`) se realizará en la etapa subsiguiente `ui` (`module.ux-shell.task0070`).
- **Adaptación matemática a la «Interfaz más fácil de usar» (Modo Senior)**:
  - Altura mínima de tira garantizada: $\ge 48$ px.
  - Ancho de pestaña fijada: $\ge 48$ px (tap target $\ge 48\times 48$ px).
  - Ancho de pestaña regular: preferido 220 px, mínimo 140 px (sin compresión asfixiante que degrade la legibilidad).
  - Botón de nueva pestaña: $\ge 48\times 48$ px.
  - Botón de cierre: área de interacción $\ge 48\times 48$ px, siempre visible en Modo Senior (umbral 140 px).
  - Separación entre pestañas: 4 px de margen protector para prevenir pulsaciones erróneas.
- **Auditoría estricta de lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
  - Prohibición total de exponer términos de ingeniería interna: cero apariciones de «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain» o «runner».
  - Uso exclusivo de las denominaciones aprobadas: **«Interfaz más fácil de usar»**, **«Ahorro de memoria»**, **«Liberar memoria de pestañas inactivas»** / **«Pestaña en reposo»**, **«Cargar pestañas cuando las necesites»**.
  - Textos de diagnóstico y tooltips formateados en español neutro, claro y accesible.

---

## Reglas y fallos

1. **Invariante de precedencia de pestañas fijadas**:
   - Toda pestaña fijada precede en el orden de disposición a las pestañas regulares ($\forall i \in \text{pinned}, \forall j \in \text{regular}, x_i < x_j$).
   - Las pestañas fijadas no sufren compresión elástica; conservan estrictamente su ancho canónico ($36$ px estándar, $\ge 48$ px Senior, $28$ px compacta).
2. **Invariante de no colisión espacial**:
   - Las áreas calculadas de las pestañas son mutuamente excluyentes ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$).
   - Los elementos internos de cada pestaña no se solapan entre sí ($\text{favicon} \cap \text{title} = \emptyset$, $\text{title} \cap \text{close} = \emptyset$).
3. **Regla de compresión elástica y desbordamiento**:
   - Mientras la tira disponga de espacio, las pestañas regulares se comprimen uniformemente entre `preferredTabWidth` y `minTabWidth`.
   - Cuando la compresión alcanza `minTabWidth` y se agregan más pestañas, el servicio activa `isOverflowing = true`, asigna `overflowMode = TabStripOverflowMode::Scrollable` y expone botones de desplazamiento horizontal sin reducir las pestañas por debajo de su tamaño legible.
4. **Protección de cierre accidental**:
   - En densidades estándar y compacta, las pestañas inactivas comprimidas por debajo de 90 px ocultan el botón de cierre para evitar clics accidentales al seleccionarlas. En Modo Senior, el botón de cierre permanece siempre visible y protegido.
5. **Manejo de fallos de persistencia y corrupción en disco**:
   - Si el archivo de estado en disco se encuentra truncado, ausente o contiene JSON malformado, `loadStateFromFile` no arroja excepciones ni interrumpe la aplicación: restaura automáticamente la configuración canónica por defecto (`canonicalDefaultDensity()`) y reporta un estado limpio.
   - La escritura en disco emplea un archivo temporal `.tmp` y renombramiento atómico (`std::filesystem::rename`), garantizando que un corte eléctrico o caída de proceso nunca deje un archivo a medio escribir en disco.
6. **Política Zero-Zombie de cancelación cooperativa**:
   - Los métodos de cálculo y de I/O aceptan un puntero opcional `CancellationToken*`. Si el token es cancelado, la operación aborta de inmediato retornando `TabStripDensityError::OperationCancelled` o `false`.

---

## Seguridad y privacidad

- **Arquitectura Local-First constitucional (ADR-001 / Decisión Q-001)**:
  - Cero conexiones de red, cero llamadas cloud y cero telemetría de uso de pestañas.
  - Todos los datos de pestañas (títulos, URLs, estados) se procesan estrictamente en la memoria local del equipo y se guardan únicamente en el almacenamiento local configurado.
- **Aislamiento estricto de navegación en modo incógnito**:
  - Cuando `isIncognitoSession` está activado (`setIncognitoMode(true)`), cualquier invocación a `saveStateToFile` o `saveCurrentState` es rechazada de inmediato retornando `false`.
  - Cero bytes de historial de navegación o pestañas de incógnito tocan el disco o el almacenamiento persistente.
- **Sanitización proactiva de privacidad**:
  - `updateTabTitle` y `addTab` procesan y sanitizan títulos y URLs, eliminando credenciales embebidas (`http://user:password@host/`), tokens de autenticación en parámetros de consulta (`?token=...`, `?access_token=...`) y hashes de sesión sensibles.
- **Protección de memoria y límites de recursos**:
  - Límites estrictos en recuentos de pestañas y prevención de recursión o copias profundas innecesarias; estructuras con semántica de movimiento y paso por referencia constante.

---

## Matriz de pruebas

| ID Prueba | Categoría | Archivo / Fixture | Comando de ejecución | Resultado esperado | Criterio de fallo negativo |
| --- | --- | --- | --- | --- | --- |
| `TEST-TSS-001` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Servicio se inicializa con plan canónico por defecto, ancho 1000 y 0 pestañas. | Plan nulo o dimensiones no coinciden con `canonicalDefaultDensity()`. |
| `TEST-TSS-002` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Transición a `LayoutDensity::Senior` eleva altura a 48 px, pestañas a 220 px, espaciado a 4 px y tap targets a $\ge 48$ px. | Altura $< 48$ px o tap targets inferiores a $48\times 48$ px. |
| `TEST-TSS-003` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Transición a `LayoutDensity::Compact` fija altura en 28 px y pestaña preferida en 160 px. | Altura o anchos no coinciden con constantes canónicas compactas. |
| `TEST-TSS-004` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Adición e inserción de pestañas regulares actualizan recuento y generan rectángulos disjuntos. | Recuento incorrecto o colisión espacial entre pestañas adyacentes. |
| `TEST-TSS-005` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Cierre de pestaña seleccionada transfiere automáticamente el foco a la pestaña adyacente contigua. | Foco desincronizado o pestaña eliminada continúa seleccionada. |
| `TEST-TSS-006` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Pestañas fijadas se ubican a la izquierda ($x_{\text{pinned}} < x_{\text{regular}}$) con ancho invariante sin compresión. | Pestaña fijada posicionada a la derecha de regular o con ancho comprimido. |
| `TEST-TSS-007` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Reordenamiento de pestañas (`moveTab`) valida límites de partición sin violar precedencia fijada. | Pestaña regular desplazada a la zona de fijadas sin marcaje formal. |
| `TEST-TSS-008` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Conmutación de reproducción sonora a silencio actualiza rectángulo de altavoz y etiqueta accesible. | Estado de audio no reflejado en geometría o etiqueta accesible incorrecta. |
| `TEST-TSS-009` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Transición a «Pestaña en reposo» («Liberar memoria de pestañas inactivas») genera etiquetas aprobadas. | Exposición de términos internos como «discard» o «memory saver». |
| `TEST-TSS-010` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Sanitización de títulos y URLs elimina credenciales embebidas y tokens de sesión. | Presencia de contraseñas o tokens en el título o tooltip almacenado. |
| `TEST-TSS-011` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Reducción de ancho de tira comprime pestañas elásticamente hasta el mínimo antes de desbordar. | Compresión por debajo de `minTabWidth` o salto abrupto sin compresión. |
| `TEST-TSS-012` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Desbordamiento horizontal activa `isOverflowing == true` y modo `Scrollable`. | Desbordamiento no detectado o solapamiento fuera de los límites de tira. |
| `TEST-TSS-013` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Hit-testing cartesiano $\mathcal{O}(1)$ clasifica clics en cuerpo, favicon, título, cierre y altavoz. | Coordenadas cartesianas clasificadas en zona errónea o fuera de rango. |
| `TEST-TSS-014` | Unitario | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Cancelación cooperativa Zero-Zombie mediante `CancellationToken` aborta inmediatamente cálculos pesados. | Cálculo continúa ejecutándose ignorando la solicitud de cancelación. |
| `TEST-TSS-015` | Integración | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Persistencia atómica local en disco (escritura `.tmp` + rename) guarda y recupera estado idéntico. | Archivo en disco corrupto, truncado o con discrepancia en el estado recuperado. |
| `TEST-TSS-016` | Integración | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Recuperación ante archivo de estado corrupto restaura automáticamente la configuración canónica. | Lanzamiento de excepción no controlada o caída de proceso ante archivo corrupto. |
| `TEST-TSS-017` | Seguridad | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Aislamiento incondicional de modo incógnito bloquea toda escritura a disco retornando `false`. | Cualquier byte escrito a disco cuando `isIncognitoSession == true`. |
| `TEST-TSS-018` | Accesibilidad | `TabStripDensityServiceTests.cpp` | `ctest -R tab_strip_density_service_tests` | Auditoría de cadenas emitidas certifica cero ocurrencias de términos internos proscritos. | Aparición de «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec». |

---

## Pasos APPLY

1. **Crear `apps/vigilio-browser/src/core/TabStripDensityService.h`**:
   - Declarar `TabStripDensityServiceOptions`, `OnTabStripChangedCallback` y la clase `TabStripDensityService` con mutex de sincronización multihilo y métodos públicos de ciclo de vida, mutación, geometría, persistencia y privacidad.
2. **Crear `apps/vigilio-browser/src/core/TabStripDensityService.cpp`**:
   - Implementar los constructores, transiciones de densidad, mutaciones de colección de pestañas, ordenación de fijadas, selección adyacente, hit-testing, persistencia atómica con archivo temporal `.tmp`, tolerancia a archivos corruptos, bloqueo en modo incógnito, cancelación Zero-Zombie y copy en español neutro.
3. **Actualizar `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/TabStripDensityService.cpp` a la biblioteca estática `vigilio_core`.
   - Declarar el target ejecutable `vigilio_tab_strip_density_service_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar la prueba `tab_strip_density_service_tests` en CTest con `LABELS "unit;fast"` y timeout de 15 segundos.
4. **Crear `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`**:
   - Implementar las 17 pruebas unitarias independientes evaluando todo el espectro de comportamiento del servicio.
5. **Ejecutar validación de calidad continua**:
   - Compilar y ejecutar pruebas con `apps/vigilio-browser/scripts/check.sh`.
   - Verificar Clang-Format sobre archivos modificados y creados.
6. **Sincronizar documentación de microtarea**:
   - Actualizar `docs/rules/01-modules/ux-shell/micro/0069-tab-strip-density/task.md` documentando la finalización de la etapa `implementation`.

---

## Definición de terminado

- [ ] `TabStripDensityService.h` y `TabStripDensityService.cpp` implementados en C++20 puro sin dependencias de GUI ni WebEngine.
- [ ] Mutex multihilo `mutable std::mutex` protegiendo todas las operaciones de lectura y mutación del estado.
- [ ] Conmutación determinista de densidades (Estándar, «Interfaz más fácil de usar», Compacta) con recálculo analítico.
- [ ] Operaciones de colección de pestañas (adición, inserción, eliminación con selección contigua, fijación y reordenamiento).
- [ ] Precedencia estricta de pestañas fijadas ($x_{\text{pinned}} < x_{\text{regular}}$) con ancho canónico invariante.
- [ ] Soporte para estados de reproducción sonora (`AudioPlaying`, `AudioMuted`) y ahorro de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo»).
- [ ] Detección analítica de desbordamiento horizontal y soporte para modo de desplazamiento continuo (`Scrollable`).
- [ ] Hit-testing cartesiano $\mathcal{O}(1)$ desacoplado de eventos del sistema operativo.
- [ ] Persistencia atómica local con archivo temporal `.tmp` y renombramiento seguro.
- [ ] Resiliencia automática ante corrupción de archivo de estado con fallback limpio a valores canónicos.
- [ ] Aislamiento incondicional de sesiones privadas en modo incógnito (cero escrituras a disco).
- [ ] Cancelación cooperativa Zero-Zombie respetada mediante `CancellationToken`.
- [ ] Cero ocurrencias de vocabulario técnico proscrito («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec»).
- [ ] 17 pruebas unitarias pasando al 100% en CTest sin advertencias bajo `-Werror`.
- [ ] `check.sh` ejecutado con éxito total.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.ux-shell.task0068`: Modelo de dominio analítico, configuración e invariantes geométricas de tira de pestañas (precondición satisfecha).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo 1*: Corrupción del archivo de preferencias de pestañas ante cortes de energía o caídas abruptas de proceso.
    - *Mitigación*: Escritura atómica a archivo `.tmp` seguida de renombramiento atómico en el sistema de archivos, con recuperación automática a `canonicalDefaultDensity()` si el parseo falla.
  - *Riesgo 2*: Fuga de datos de historial o navegación privada a disco en sesiones de incógnito.
    - *Mitigación*: Comprobación ineludible de `isIncognitoSession` antes de cualquier operación de I/O, abortando de inmediato sin tocar el sistema de archivos.
  - *Riesgo 3*: Bloqueo del hilo de usuario ante cálculos de redimensionamiento repetitivos.
    - *Mitigación*: Complejidad lineal $\mathcal{O}(N)$ sobre estructuras ligeras en memoria, ejecución en microsegundos y cancelación cooperativa con `CancellationToken`.

---

## Commit propuesto

```
feat(ux-shell): implementation tab-strip-density
```
