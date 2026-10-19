# Design — module-ux-shell-task0062

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del árbol de código tras las etapas de dominio e implementación (`task0060` y `task0061`)**:
   - `module.ux-shell.task0060` (commit `4cb5d8f`) formalizó el modelo de dominio en `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration.h/.cpp` y `MainWindowLayoutValidator.h/.cpp`, con 12 pruebas unitarias de cálculo analítico de distribución espacial en C++20 puro.
   - `module.ux-shell.task0061` (commit `c4d75c0`) implementó la capa de servicio operativo desacoplada en `MainWindowLayoutService.h/.cpp`, con gestión multihilo del plan (`std::mutex`), conmutación de densidades (`setDensity`), control de estados de barra lateral (`toggleSidebar`, `setSidebarWidth`), persistencia local atómica (`saveLayoutStateToFile`, `loadLayoutStateFromFile`), tolerancia a archivos corruptos, aislamiento estricto de modo incógnito, cancelación cooperativa Zero-Zombie (`CancellationToken`) y 14 pruebas unitarias en CTest.
2. **Limitaciones de la interfaz Qt actual (`BrowserWindow`)**:
   - La ventana principal (`apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`) fue diseñada inicialmente como un prototipo monolítico con widgets colocados de forma estática:
     - No integra el servicio operativo `MainWindowLayoutService`, por lo que el estado de layout no está unificado ni sincronizado.
     - No implementa la barra lateral (`vigilio-sidebar`) visible en el diseño objetivo `/resultado/01-vigilio-home.png`, impidiendo el acceso ergonómico a espacios de trabajo, herramientas y descargas.
     - La alternancia de la «Interfaz más fácil de usar» (`setSeniorMode`) aplica reglas puntuales de estilo mediante cadenas QSS directas, sin consultar los presupuestos dimensionales ni ajustar las restricciones mínimas de ventana (`setMinimumSize`).
     - No existe gestión del evento nativo de redimensionamiento (`resizeEvent`), perdiendo la capacidad de informar a la capa de servicio de los cambios de resolución o tamaño de pantalla.
3. **Brechas de accesibilidad y observabilidad en UI**:
   - Aunque existen etiquetas accesibles básicas en la barra de herramientas, la jerarquía visual carece de una secuencia de foco por teclado exhaustiva entre la barra lateral, la barra de herramientas y la vista de contenido.
   - No se emiten notificaciones de estado accesibles en la barra de estado cuando la barra lateral se pliega o despliega.

---

### Estado objetivo (TARGET)

1. **Integración nativa de `MainWindowLayoutService` en `BrowserWindow`**:
   - `BrowserWindow` aloja una instancia única `std::unique_ptr<vigilio::MainWindowLayoutService> layoutService_`, inicializada de acuerdo con las preferencias del usuario (estándar o accesible) y con el modo de sesión (aislamiento en incógnito).
   - El callback de observabilidad `setLayoutChangedCallback` conecta el servicio desacoplado con la jerarquía de widgets de Qt, permitiendo que cualquier mutación del plan se refleje de inmediato en la interfaz.
2. **Jerarquía visual estructurada y panel lateral (`vigilio-sidebar`)**:
   - El widget central (`centralWidget`) aloja un `QHBoxLayout` sin márgenes que divide la ventana horizontalmente:
     - A la izquierda: `QFrame#vigilio-sidebar` con anchura canónica de 48 px (colapsada) o 240 px (expandida).
     - A la derecha: `QWidget#vigilio-main-container` que agrupa verticalmente la tira de pestañas (`vigilio-tabs`), la barra de navegación (`vigilio-navigation`), el área de avisos (`vigilio-banner-area`), el viewport central y la barra de estado (`vigilio-statusbar`).
   - La barra lateral cuenta con un botón de alternancia superior (`vigilio-sidebar-toggle`), botones de acceso rápido con iconografía Lucide SVG (espacios, herramientas, descargas, historial, marcadores) y botón de ajustes inferior.
   - Atajo de teclado global `Ctrl+Alt+S` (y `Alt+S`) para plegar o desplegar la barra lateral de forma inmediata.
3. **Adaptación integral a la «Interfaz más fácil de usar» (Modo Senior)**:
   - Sincronización bidireccional entre `setSeniorMode(bool)` y `layoutService_->setDensity()`:
     - Dimensiones mínimas de ventana ajustadas a $720\times 520$ px (frente a $640\times 480$ px en estándar).
     - Altura de botones y controles interactivos $\ge 48$ px.
     - Altura de barra de herramientas a 56 px y tira de pestañas a 48 px.
     - Iconos vectoriales escalados a $32\times 32$ px (28 px en barra lateral).
     - Tipografía accesible a 14 pt base.
4. **Respeto estricto de la política de lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
   - Proscripción absoluta de los términos «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter» o jerga técnica interna en textos visibles, tooltips, descripciones accesibles o mensajes de estado.
   - Empleo sistemático del término oficial **«Interfaz más fácil de usar»**.
5. **Aislamiento Local-First y navegación privada**:
   - Modo incógnito notificado al servicio de layout (`setIncognitoMode(true)`), impidiendo cualquier escritura o sincronización en disco.
6. **Batería de pruebas en Qt Test (`tests/ui/WindowTests.cpp`)**:
   - Verificación automatizada de la presencia de los widgets de layout, conmutación de densidad, alternancia de barra lateral, tap targets mínimos y auditoría de textos accesibles.

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Capa de orquestación en UI** | Inexistente; widgets se configuran sin consultar servicio | `BrowserWindow` delega el cálculo a `MainWindowLayoutService` | Integrar puntero a `MainWindowLayoutService`, conectar callback y sincronizar cambios. |
| **Panel lateral (`vigilio-sidebar`)** | Ausente; la ventana solo tiene toolbar y tabs | Widget `QFrame#vigilio-sidebar` con estados colapsado (48 px) y expandido (240 px) | Construir la jerarquía Qt del sidebar, botón toggle, botones de navegación e iconos Lucide. |
| **Contenedor horizontal raíz** | Layout vertical directo en `centralWidget` | `QHBoxLayout` dividiendo sidebar y contenedor de navegación/contenido | Reestructurar `setupUi()` para alojar `sidebar_` y `mainContainer_`. |
| **Control de redimensionamiento** | Sin captura de eventos de tamaño | `resizeEvent()` notifica a `layoutService_->resizeWindow()` y mantiene mínimos | Sobrescribir `resizeEvent` y aplicar `setMinimumSize` según densidad activa. |
| **Modo accesible en UI** | Estilos aislados en `BrowserWindow::setSeniorMode` | Sincronización integral de densidad en servicio y widgets ($\ge 48$ px) | Refactorizar `setSeniorMode` para invocar `setDensity`, redimensionar controles y ajustar fuentes. |
| **Atajos de teclado de layout** | Solo existen `Ctrl+W` y `Ctrl+L` | Añadido `Ctrl+Alt+S` para plegar/desplegar sidebar | Registrar atajos de teclado para el panel lateral. |
| **Estilos QSS de layout** | Estilos genéricos sin clases para sidebar | Reglas CSS completas para `vigilio-sidebar`, estados hover/focus y modo accesible | Enriquecer `setDarkTheme()` con selectores específicos de layout y borde de foco visible. |
| **Pruebas de UI de layout** | `WindowTests.cpp` no valida sidebar ni servicio de layout | Pruebas exhaustivas de widgets, dimensiones, toggles y accesibilidad | Añadir aserciones específicas en `tests/ui/WindowTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad arquitectónica

- **Compatibilidad con C++20 y Qt 6**:
  - Toda la interacción con Qt Widgets se implementa utilizando APIs canónicas de Qt 6 (`QWidget`, `QFrame`, `QVBoxLayout`, `QHBoxLayout`, `QPushButton`, `QAction`, `QShortcut`).
  - No se introducen macros no estándar ni dependencias de bibliotecas externas adicionales; el enlace se mantiene contra `vigilio_core`, `vigilio_compiler_warnings` y las bibliotecas Qt ya presentes.
- **Conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - La ventana no realiza ninguna petición de red para renderizar su interfaz ni depende de servicios en la nube.
  - La persistencia de preferencias de ventana utiliza `QSettings` para ajustes locales de usuario y `MainWindowLayoutService` para la gestión geométrica local.
- **Sincronización desacoplada mediante Callbacks y Signals/Slots**:
  - `MainWindowLayoutService` reside en `src/core/` sin incluir ninguna cabecera de Qt.
  - La ventana `BrowserWindow` se suscribe a los cambios del servicio a través de `setLayoutChangedCallback` con un lambda C++20, evitando acoplamiento circular.

### Modelo de interacción y máquina de estados del panel lateral

```
                  ┌──────────────────────┐
                  │   Estado Oculto      │
                  │ (showSidebar = false)│
                  └──────────┬───────────┘
                             │ setZoneVisible(Sidebar, true)
                             ▼
┌──────────────────┐  toggleSidebar()   ┌──────────────────┐
│ Estado Colapsado │ ─────────────────> │ Estado Expandido │
│ (Ancho: 48 px)   │ <───────────────── │ (Ancho: 240 px)  │
│ Iconos solos     │  toggleSidebar()   │ Iconos + Texto   │
└──────────────────┘                    └──────────────────┘
```

1. **Estado Colapsado (Default)**:
   - Anchura fija de 48 px.
   - Los botones muestran exclusivamente iconos Lucide SVG centrados ópticamente ($20\times 20$ px en estándar, $28\times 28$ px en accesible).
   - Tooltips enriquecidos muestran la etiqueta de la acción y el atajo de teclado asociado.
2. **Estado Expandido**:
   - Anchura canónica de 240 px (respetando el rango permitido $[200, 360]$ px).
   - Los botones muestran icono a la izquierda y texto descriptivo a la derecha, con alineación a la izquierda y margen de 12 px.
3. **Transición**:
   - La alternancia se ejecuta invocando `layoutService_->toggleSidebar()`, aplicando inmediatamente el nuevo ancho mediante `sidebar_->setFixedWidth(newWidth)`.

### Accesibilidad, alto contraste y tecnologías asistivas

1. **Identificadores y etiquetas accesibles**:
   - Todos los nuevos widgets creados reciben `setObjectName()` semántico con prefijo `vigilio-`.
   - Todos los botones reciben `setAccessibleName()` y `setAccessibleDescription()` en español neutro, cumpliendo las directrices de `docs/UX-COPY-PUBLICO.md`.
2. **Foco visible y contraste WCAG 2.2 AA**:
   - En tema claro: fondo `#F7F9FC`, acento `#2563EB`, borde de foco `2px solid #2563EB` con desplazamiento de 2 px.
   - En tema oscuro: fondo `#141D2B`, acento `#3B82F6`, borde de foco `2px solid #3B82F6` con desplazamiento de 2 px.
   - Relación de contraste de texto $\ge 4.5:1$ y de componentes de interfaz gráfica $\ge 3:1$.
3. **Adaptación accesible («Interfaz más fácil de usar»)**:
   - Los botones de la barra lateral aumentan su altura de 36 px a 48 px.
   - El botón toggle aumenta su tamaño a $48\times 48$ px.
   - La tipografía aumenta a 14 pt con peso tipográfico legible.

### Estrategia de migración y reversibilidad (Rollback)

- **Compatibilidad hacia atrás con sesiones existentes**:
  - Si un usuario ya tiene una configuración previa guardada en `QSettings` (`appearance/dark` o `accessibility/senior`), `BrowserWindow` la lee y la inyecta automáticamente en `MainWindowLayoutService` al inicializarse.
  - Si no existe estado previo, se adopta el plan canónico estándar `canonicalDefaultLayout()` sin disrupciones.
- **Rollback seguro**:
  - En caso de regresión visual o fallo en la inicialización gráfica, `layoutService_->restoreDefaultLayout()` restablece instantáneamente el plan canónico sin perder pestañas abiertas ni datos locales de marcadores.
  - Si se requiriese revertir el commit `feat(ux-shell): ui main-window-layout`, la interfaz vuelve a la disposición previa sin alterar la base de datos SQLite ni los archivos de sesión.
