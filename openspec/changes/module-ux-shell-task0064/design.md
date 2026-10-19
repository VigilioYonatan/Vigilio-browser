# Design — module-ux-shell-task0064

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado de la arquitectura y shell visual de escritorio**:
   - Se completó formalmente el módulo `foundations` (`task0001` a `task0059`) y el primer ciclo de desarrollo del módulo `ux-shell` enfocado en el diseño estructural de la ventana principal (**main window layout**, `task0060` a `task0063`), alcanzando el commit `b7892dc (HEAD -> main) test(ux-shell): tests main-window-layout`.
   - El sistema cuenta con 27 pruebas unitarias y de integración pasando al 100% en CTest bajo `apps/vigilio-browser/_build`, un esquema de advertencias estrictas con `-Werror`, integración de recursos vectoriales Lucide SVG en QRC, y un validador geométrico para la ventana principal (`MainWindowLayoutValidator`).
   - En `MainWindowLayoutTypes.h`, se definieron los enums `LayoutZone::TitleBar` y `WindowChromeMode` (`Standard`, `Integrated`, `Fullscreen`), y en `MainWindowLayoutConfiguration` la constante `kCanonicalTitleBarHeight = 32`.
2. **Limitaciones estructurales y de dominio en la barra de título**:
   - Actualmente, `apps/vigilio-browser/src/ui/BrowserWindow.cpp` opera con la barra de título nativa proporcionada por el gestor de ventanas del sistema operativo a través de `QMainWindow` (`WindowChromeMode::Standard`).
   - No existe un modelo de dominio desacoplado que represente:
     - La configuración de barra de título personalizada e integrada (`WindowChromeMode::Integrated` o *client-side decorations*).
     - La colocación de la caja de controles de ventana (`TitleBarControlsPlacement`), tanto en el extremo derecho (predeterminado para Windows y Linux) como en el izquierdo (estilo macOS).
     - La geometría cartesiana, estado y comportamiento de los botones de ventana: Minimizar, Maximizar/Restaurar, Cerrar, y el botón de acceso rápido para la «Interfaz más fácil de usar».
     - El cálculo analítico del espacio de arrastre no-cliente (*non-client draggable area*), crítico para permitir mover la ventana física sin interferir con elementos clicables ni pestañas.
     - La clasificación analítica de pruebas de impacto (*non-client hit testing*), necesaria para que las plataformas subyacentes distingan entre zonas de arrastre, botones de control, icono de marca y área cliente sin acoplarse a APIs dependientes de Win32 (`WM_NCHITTEST`), X11 o Wayland.
     - La adaptación de accesibilidad para personas mayores (Modo Senior), garantizando que los botones de control alcancen un tamaño mínimo de $48\times 48$ px y la barra una altura mínima de 48 px.
     - Un catálogo fuertemente tipado de errores de barra de título (`TitleBarError`) y sugerencias de remediación en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.

---

### Estado objetivo (TARGET)

1. **Modelo de dominio de barra de título fuertemente tipado en `vigilio_core` (`TitleBarTypes.h`)**:
   - Enumeraciones tipadas `enum class : uint8_t`:
     - `TitleBarMode`: `NativeSystem`, `CustomIntegrated`, `Suppressed`.
     - `TitleBarControlsPlacement`: `Right`, `Left`.
     - `TitleBarButtonType`: `Minimize`, `MaximizeRestore`, `Close`, `SeniorModeToggle`.
     - `TitleBarButtonState`: `Normal`, `Hovered`, `Pressed`, `Disabled`.
     - `TitleBarHitTestZone`: `Nowhere`, `ClientArea`, `DraggableSpace`, `WindowIcon`, `CaptionText`, `MinimizeButton`, `MaximizeButton`, `CloseButton`, `SeniorModeToggle`.
     - `TitleBarError`: `None`, `DimensionsBelowMinimum`, `ControlsOverflowWidth`, `DraggableAreaInsufficient`, `ConflictingControlPlacement`, `SeniorModeMinimumHeightViolation`, `UnsupportedWindowChromeMode`.
   - Estructuras inmutables con semántica de valor:
     - `TitleBarDimensions`: Altura, anchos mínimos, tamaño de icono y dimensiones de botón.
     - `TitleBarControlSpec`: Botón individual con rectángulo cartesiano `LayoutRect`, estado, visibilidad, etiqueta accesible y tooltip.
     - `TitleBarState`: Estado de ventana (`isMaximized`, `isActive`, `isFullscreen`, `seniorModeActive`).
     - `TitleBarConfig`: Configuración declarativa de entrada para el cálculo de layout.
     - `TitleBarLayoutPlan`: Plan estructural calculado con rectángulos asignados para icono, título, zona de arrastre y caja de controles.
     - `TitleBarReport`: Reporte inmutable de evaluación con estado booleano, error tipado, rectángulos y textos descriptivos en español neutro.
2. **Configuración canónica e invariantes geométricas (`TitleBarConfiguration`)**:
   - Constantes canónicas que codifican los estándares de Vigilio Browser:
     - Altura estándar: 32 px; botones estándar: $46\times 32$ px; icono: $16\times 16$ px.
     - Altura Senior («Interfaz más fácil de usar»): 48 px; botones Senior: $\ge 48\times 48$ px; icono: $24\times 24$ px.
     - Altura compacta: 28 px; botones compactos: $36\times 28$ px.
     - Área mínima de arrastre: 120 px estándar / 160 px Senior.
     - Anchura mínima de ventana para barra integrada: 400 px estándar / 500 px Senior.
   - Factorías canónicas: `canonicalDefaultTitleBar()`, `canonicalSeniorTitleBar()`, `canonicalCompactTitleBar()`.
   - Método `verifyInvariants(const TitleBarLayoutPlan&)` que certifica no solapamiento espacial ($\forall i \ne j, C_i \cap C_j = \emptyset$), suficiencia de arrastre y cumplimiento de límites de altura.
3. **Validador estático desacoplado, algoritmo de distribución espacial y hit-testing analítico (`TitleBarValidator`)**:
   - `validateDimensions()`: Comprobación estricta de umbrales dimensionales.
   - `validatePlan()`: Auditoría integral de congruencia del plan.
   - `calculateTitleBarLayout()`: Algoritmo analítico determinista que distribuye el espacio disponible entre icono, título, arrastre y controles.
   - `hitTest(int x, int y, const TitleBarLayoutPlan&)`: Clasificación espacial pura de coordenadas para desacoplar el manejo de puntero.
   - Funciones puras de UX copy público en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
4. **Batería de pruebas unitarias trazables en CTest (`TitleBarIntegrationTests.cpp`)**:
   - 14 pruebas unitarias de especificación ejecutables en < 50 ms en modo headless, sin dependencias gráficas.

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Modelo de datos de barra de título** | Inexistente; solo un valor enum `LayoutZone::TitleBar` en `MainWindowLayoutTypes.h` | `TitleBarTypes.h` en `vigilio_core` con tipos inmutables y enums fuertemente tipados | Crear cabecera formal con tipos, enums y estructuras de valor inmutables. |
| **Configuración canónica** | Altura fija aislada (32 px) sin definición de botones ni áreas de arrastre | `TitleBarConfiguration` con constantes, factorías y verificación de invariantes | Centralizar constantes dimensionales y proveer factorías para Standard, Senior y Compact. |
| **Validador y distribución espacial** | Inexistente; ninguna función calcula áreas o botones de barra | `TitleBarValidator::calculateTitleBarLayout` determinista | Implementar cálculo analítico que garantice disyunción de controles y área de arrastre suficiente ($\ge 120$ px). |
| **Clasificación de impacto (Hit-Testing)** | Inexistente; la ventana depende enteramente del marco nativo del SO | `TitleBarValidator::hitTest` analítico desacoplado de APIs nativas | Implementar función matemática pura que mapee coordenadas relativas a zonas (`DraggableSpace`, `CloseButton`, etc.). |
| **Reglas de Modo Senior** | Ninguna en la barra superior | Invariantes formales en el modelo de dominio ($\ge 48$ px altura, botones $\ge 48\times 48$ px, tap targets amplios) | Codificar y validar programáticamente las reglas de «Interfaz más fácil de usar». |
| **Soporte de colocación de controles** | Asunción implícita de controles a la derecha de Windows | Parámetro formal `TitleBarControlsPlacement` (`Right` y `Left`) | Soportar inversión de layout para entornos macOS o zurdos sin romper invariantes. |
| **Copy público y textos** | Inexistente en barra de título | Funciones puras con copy en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md` | Implementar `titleBarErrorToString`, `buttonAccessibleLabel`, `buttonTooltip`, etc. |
| **Pruebas de especificación** | 0 pruebas unitarias para barra de título en CTest | 14 pruebas unitarias en `TitleBarIntegrationTests.cpp` | Crear suite de pruebas exhaustiva para verificar invariantes, hit-testing y alternancia de estados. |

---

## Arquitectura técnica y diseño detallado

### 1. Diagrama de flujo de cálculo y evaluación de layout de barra de título

```
[ TitleBarConfig ]
       │
       ▼
[ TitleBarValidator::calculateTitleBarLayout(width, config) ]
       │
       ├─► 1. Verificar dimensiones mínimas (width >= minWidth, density)
       ├─► 2. Si NativeSystem o Suppressed ──► Retornar Plan inactivo/vacío
       ├─► 3. Asignar Icono de marca (margen opuesto a controles)
       ├─► 4. Calcular ancho total de caja de controles sumando botones visibles
       ├─► 5. Asignar posición de caja de controles (Left o Right)
       ├─► 6. Distribuir botones dentro de la caja de controles (disjuntos)
       ├─► 7. Calcular área neta restante para Título y Espacio de Arrastre
       ├─► 8. Validar suficiencia de arrastre (draggableWidth >= minDraggable)
       │        └─► Si insuficiente: reportar DraggableAreaInsufficient
       └─► 9. Construir y certificar TitleBarLayoutPlan con verifyInvariants
```

### 2. Algoritmo de Hit-Testing no-cliente

Dado un punto cartesiano relativo $(x, y)$ donde $0 \le x < \text{width}$ y $0 \le y < \text{height}$:

1. Si $y < 0$ o $y \ge \text{plan.totalHeight}$ o $x < 0$ o $x \ge \text{plan.totalWidth}$, retornar `TitleBarHitTestZone::Nowhere`.
2. Para cada botón $B \in \text{plan.controls}$:
   - Si $B.\text{visible}$ y $B.\text{rect.contains}(x, y)$:
     - Mapear $B.\text{type}$ a `MinimizeButton`, `MaximizeButton`, `CloseButton` o `SeniorModeToggle`.
3. Si $\text{plan.iconRect.contains}(x, y)$, retornar `TitleBarHitTestZone::WindowIcon`.
4. Si $\text{plan.titleRect.contains}(x, y)$, retornar `TitleBarHitTestZone::CaptionText`.
5. Si $\text{plan.draggableRect.contains}(x, y)$, retornar `TitleBarHitTestZone::DraggableSpace`.
6. En cualquier otro caso dentro de la barra, retornar `TitleBarHitTestZone::DraggableSpace` (garantizando que el espacio entre controles sea arrastrable en lugar de bloquear el puntero).

### 3. Integración con la «Interfaz más fácil de usar» (Modo Senior)

En la configuración canónica Senior:
- **Altura de barra**: 48 px.
- **Botón Minimizar**: $48\times 48$ px.
- **Botón Maximizar/Restaurar**: $48\times 48$ px.
- **Botón Cerrar**: $48\times 48$ px.
- **Botón Acceso Fácil (`SeniorModeToggle`)**: $48\times 48$ px, situado contiguo a la caja de controles principales, permitiendo al usuario revertir o comprobar la modalidad visual con un solo clic.
- **Márgenes y separadores**: 4 px entre botones para evitar clics erráticos en pantallas táctiles o por temblores motores.
- **Icono oficial**: $24\times 24$ px derivado de `resultado/06-logo-instalador-oficial.png`.

---

## Compatibility, migration and rollback

### Compatibilidad y estándares C++20 / Qt6
- **Estándar C++20**:
  - Implementación en C++20 puro, haciendo uso de enumeraciones fuertemente tipadas (`enum class : uint8_t`), semántica de valor inmutable, `std::string`, `std::string_view`, `std::vector` y métodos `constexpr`/`noexcept`.
  - Cero dependencias hacia bibliotecas gráficas de Qt (`QWidget`, `QApplication`, `QMainWindow`) o APIs nativas de plataforma (`windows.h`, `X11/Xlib.h`) en los archivos de dominio (`TitleBarTypes.h`, `TitleBarConfiguration`, `TitleBarValidator`), posibilitando la compilación y ejecución de pruebas en modo headless en cualquier entorno CI.
- **Integrabilidad con `MainWindowLayoutPlan`**:
  - La estructura `TitleBarLayoutPlan` se acopla limpiamente con la zona `LayoutZone::TitleBar` calculada por `MainWindowLayoutValidator`. Cuando `MainWindowLayoutPlan::showTitleBar` es `true`, la altura de la zona `TitleBar` se sincroniza de forma determinista con `plan.totalHeight`.

### Estrategia de migración
- La adición de los tipos y validadores de barra de título a `vigilio_core` es puramente aditiva y no intrusiva.
- La ventana principal `BrowserWindow` mantendrá su modo operativo actual hasta que la etapa `ui` (`task0066`) introduzca el widget visual `TitleBarWidget`, eliminando cualquier riesgo de regresión en las fases intermedias.

### Plan de contingencia y rollback
- En caso de anomalía dimensional o configuración incompatible, `calculateTitleBarLayout` genera un `TitleBarReport` con `valid = false`, manteniendo un fallback degradado y seguro que no colapsa la aplicación.
- Si fuera necesario revertir este microcambio antes de la aprobación final, basta con descartar los 5 archivos agregados en `vigilio_core` y `tests/unit`, revirtiendo la entrada correspondiente en `CMakeLists.txt` sin afectar el funcionamiento del resto del shell de `apps/vigilio-browser`.
