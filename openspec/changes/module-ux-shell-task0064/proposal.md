# Change Proposal — module-ux-shell-task0064

Pack: `module.ux-shell.task0064`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **title bar integration** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0064 — Definir el comportamiento y las invariantes: title bar integration`).

Habiéndose completado y certificado formalmente en las 59 microtareas previas del módulo `foundations` (`task0001`-`task0059`) la base de compilación estricta C++20, modularización CMake, integración de recursos QRC, ciclo de vida, presets, test fixtures y CI multiplataforma, y en las 4 microtareas fundacionales de `ux-shell` (`task0060`-`task0063`) el modelo de dominio, servicio de persistencia, integración en `BrowserWindow` y suite de pruebas para el diseño estructural de la ventana principal (**main window layout**), este pack aborda la segunda gran capacidad visual del shell: **title bar integration** (integración de la barra de título).

El objetivo central de esta propuesta es proporcionar un modelo de dominio matemático, declarativo y fuertemente tipado en C++20 puro para la barra de título integrada y personalizada (`TitleBarTypes.h`, `TitleBarConfiguration`, `TitleBarValidator`), de manera que el cálculo de coordenadas, dimensiones canónicas, áreas de arrastre (*non-client draggable space*), caja de controles de ventana (minimizar, maximizar/restaurar, cerrar, acceso rápido a la «Interfaz más fácil de usar»), zonificación de pruebas de impacto (*hit-testing*) y estados de activación operen con total independencia del framework de widgets de Qt y de los subsistemas de ventanas del sistema operativo (Win32/X11/Wayland/Cocoa), garantizando ergonomía de escritorio, adaptación rigurosa al Modo Senior (tap targets y botones $\ge 48$ px), seguridad local-first (ADR-001 / Decisión Q-001) y estricto cumplimiento de las directrices públicas de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Taxonomía formal y tipos de dominio de barra de título (`TitleBarTypes.h`)**:
  - `TitleBarMode`: Modalidades operativas (`NativeSystem`, `CustomIntegrated`, `Suppressed`).
  - `TitleBarControlsPlacement`: Posición de anclaje de controles de ventana (`Right` estándar para Windows/Linux, `Left` estilo macOS o setups zurdos).
  - `TitleBarButtonType`: Tipos de botones de control (`Minimize`, `MaximizeRestore`, `Close`, `SeniorModeToggle`).
  - `TitleBarButtonState`: Estados interactivos de botón (`Normal`, `Hovered`, `Pressed`, `Disabled`).
  - `TitleBarHitTestZone`: Clasificación exhaustiva de zonas de impacto (*non-client hit testing*) para desacoplar el soporte de arrastre nativo y clics (`Nowhere`, `ClientArea`, `DraggableSpace`, `WindowIcon`, `CaptionText`, `MinimizeButton`, `MaximizeButton`, `CloseButton`, `SeniorModeToggle`).
  - `TitleBarError`: Clasificación fuertemente tipada de anomalías de layout (`None`, `DimensionsBelowMinimum`, `ControlsOverflowWidth`, `DraggableAreaInsufficient`, `ConflictingControlPlacement`, `SeniorModeMinimumHeightViolation`, `UnsupportedWindowChromeMode`).
  - Estructuras inmutables con semántica de valor:
    - `TitleBarDimensions`: Altura de barra, anchura mínima, área mínima de arrastre, tamaño de icono y dimensiones de botones ($W\times H$).
    - `TitleBarControlSpec`: Especificación individual de botón de control (tipo, rectángulo cartesiano `LayoutRect`, estado, visibilidad, etiqueta accesible y descripción flotante).
    - `TitleBarState`: Estado de ventana reflejado en la barra (`isMaximized`, `isActive`, `isFullscreen`, `seniorModeActive`).
    - `TitleBarConfig`: Parámetros declarativos de entrada (modo, colocación de controles, título de ventana sanitizado, visualización de icono, activación de botón de acceso fácil, estado y densidad de layout).
    - `TitleBarLayoutPlan`: Plan estructural inmutable calculado consolidando dimensiones totales, rectángulos asignados para icono, título, zona de arrastre y caja de controles, y vector de botones calculados.
    - `TitleBarReport`: Reporte inmutable de diagnóstico con indicador booleano de validez, error tipado, plan resultante, sugerencia de remediación y mensaje comprensible en español neutro.
- **Configuración canónica e invariantes geométricas (`TitleBarConfiguration`)**:
  - `canonicalDefaultTitleBar()`: Configuración estándar de escritorio (altura 32 px, botones $46\times 32$ px, icono $16\times 16$ px, espacio mínimo de arrastre de 120 px, controles anclados a la derecha).
  - `canonicalSeniorTitleBar()`: Configuración para la «Interfaz más fácil de usar» (altura canónica ampliada a 48 px, botones $\ge 48\times 48$ px, icono $24\times 24$ px, espacio de arrastre $\ge 160$ px, botón de toggle rápido para accesibilidad, separación reforzada).
  - `canonicalCompactTitleBar()`: Configuración para pantallas de alta densidad (altura 28 px, botones $36\times 28$ px, espacio de arrastre $\ge 100$ px).
  - `verifyInvariants(const TitleBarLayoutPlan&)`: Verificación booleana estricta de que el plan cumple con no solapamiento ($\forall i \ne j, C_i \cap C_j = \emptyset$), suficiencia de área de arrastre, alturas mínimas por densidad y márgenes de contención.
- **Validador estático desacoplado, algoritmo de distribución espacial y hit-testing puro (`TitleBarValidator`)**:
  - `validateDimensions(int width, int height, LayoutDensity density)`: Validación rigurosa de límites de anchura y altura.
  - `validatePlan(const TitleBarLayoutPlan&)`: Auditoría integral de congruencia del plan de barra de título.
  - `calculateTitleBarLayout(int windowWidth, const TitleBarConfig&)`: Algoritmo analítico determinista que distribuye el espacio disponible entre icono, título, área de arrastre y controles de ventana, certificando ausencia de solapamiento y suficiencia de área de manipulación.
  - `hitTest(int x, int y, const TitleBarLayoutPlan&)`: Función pura que asigna cualquier par de coordenadas relativas $(x, y)$ a su correspondiente `TitleBarHitTestZone`.
  - Funciones de lenguaje público y textos accesibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`:
    - `titleBarErrorToString(TitleBarError)`
    - `titleBarModeToString(TitleBarMode)`
    - `hitTestZoneToString(TitleBarHitTestZone)`
    - `buttonTypeToString(TitleBarButtonType, bool isMaximized)`
    - `buttonAccessibleLabel(TitleBarButtonType, bool isMaximized)`
    - `buttonTooltip(TitleBarButtonType, bool isMaximized)`
    - `generateRemediationHint(TitleBarError, std::string_view)`
- **Suite de pruebas unitarias trazables en CTest (`TitleBarIntegrationTests.cpp`)**:
  - 14 pruebas unitarias en C++20 puro bajo el ejecutable `vigilio_title_bar_tests` enlazado a `vigilio_core`, ejecutables en modo headless (< 50 ms), sin servidor gráfico ni widgets Qt, verificando planes canónicos, casos de desbordamiento, hit-testing de todas las zonas, alternancia de maximizado, colocación de controles izquierda/derecha, Modo Senior y copy público.

---

## Out of scope

- Implementación del servicio de persistencia y configuración de barra de título en disco o SQLite (`TitleBarService`) (reservado para la etapa `implementation` en `module.ux-shell.task0065`).
- Integración física con widgets de Qt (`TitleBarWidget`, `QToolButton`, `BrowserWindow.cpp`, manejo de eventos de ratón `mousePressEvent`/`mouseMoveEvent` y filtros de eventos nativos de plataforma como `WM_NCHITTEST` en Windows o extensiones de cliente en Wayland/X11) (reservado para la etapa `ui` en `module.ux-shell.task0066`).
- Pruebas E2E de interacción directa con el gestor de ventanas del sistema operativo, arrastre de ventana con cursor físico, acople de ventanas (*window snapping*) y redimensionamiento multimonitor (reservado para la etapa `tests` en `module.ux-shell.task0067`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` durante esta fase PROPOSE.
- Telemetría remota, envío de diagnósticos de interacción a servidores externos o autenticación en la nube (estrictamente proscrito por la arquitectura Local-First, ADR-001 / Decisión Q-001).
- Inclusión de subprocesos o dependencias de renderizado Chromium WebEngine en el modelo de dominio.

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completaron formalmente las 59 microtareas del módulo `foundations` (`task0001` a `task0059`) y las 4 microtareas de `main window layout` (`task0060` a `task0063`), alcanzando el commit `b7892dc (HEAD -> main) test(ux-shell): tests main-window-layout`.
   - `MainWindowLayoutTypes.h` ya introdujo la enumeración `LayoutZone::TitleBar`, la modalidad `WindowChromeMode::Integrated` y la constante canónica de altura `MainWindowLayoutConfiguration::kCanonicalTitleBarHeight = 32`.
   - `BrowserWindow.h` y `BrowserWindow.cpp` cuentan actualmente con la integración de `MainWindowLayoutService`, gestión de barra lateral (`sidebar_`), barra de herramientas (`toolbar_`), pestañas (`tabs_`) y banners de estado (`safeModeBanner_`, `crashRecoveryBanner_`).
2. **Brecha arquitectónica identificada (GAP)**:
   - Actualmente, la barra de título de la ventana se delega pasivamente a la decoración estándar del sistema operativo provista por `QMainWindow` (`WindowChromeMode::Standard`), o se asume implícitamente sin modelar cuando se planea `WindowChromeMode::Integrated`.
   - No existe un modelo de datos formal que represente:
     - La geometría de los botones de la barra de título (minimizar, maximizar/restaurar, cerrar).
     - El cálculo analítico del área de arrastre (*draggable area*), indispensable para mover la ventana cuando el marco nativo se oculta o se integra.
     - La distribución espacial desacoplada que soporte controles a la derecha (estándar Windows/Linux) o a la izquierda (estilo macOS).
     - La adaptación obligatoria al Modo Senior («Interfaz más fácil de usar»), donde los botones deben alcanzar un tamaño mínimo de $48\times 48$ px para accesibilidad motora y visual (WCAG 2.1 AAA / Directriz de accesibilidad Senior).
     - Una función matemática de *hit-testing* en coordenadas relativas que permita a las capas superiores determinar si un evento de puntero debe arrastrar la ventana, accionar un botón de control o delegarse al contenido.
     - Tipos de error estructurados y sugerencias de remediación en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Muestra la barra superior limpia, sobria, con tonos azul oscuro de base, integración armónica con la pestaña activa y controles de ventana discretos sin brillos artificiales ni estética genérica.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Muestra el comportamiento y proporciones de la «Interfaz más fácil de usar» (Modo Senior), con botones ampliados, márgenes amplios y textos legibles.
   - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser para la esquina superior izquierda de la barra de título integrada.
4. **Verificación de calidad actual**:
   - El sistema de pruebas en CTest ejecuta 27 de 27 pruebas al 100% en ~5.55s (exit code 0) en `apps/vigilio-browser/_build`.
   - Prompt Builder CLI 0.6.3 certifica 64 referencias canónicas válidas (`validate-references`) y el lockfile de la ejecución 0190 pasa el 100% de los 13 chequeos (`verify-lock`).

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `domain` está estrictamente delimitado a la modelización matemática, lógica y tipada de **title bar integration**:
1. **Creación de `apps/vigilio-browser/src/core/TitleBarTypes.h`**:
   - Enumeraciones tipadas `enum class : uint8_t`: `TitleBarMode`, `TitleBarControlsPlacement`, `TitleBarButtonType`, `TitleBarButtonState`, `TitleBarHitTestZone`, `TitleBarError`.
   - Estructuras inmutables de semántica de valor: `TitleBarDimensions`, `TitleBarControlSpec`, `TitleBarState`, `TitleBarConfig`, `TitleBarLayoutPlan`, `TitleBarReport`.
2. **Creación de `apps/vigilio-browser/src/core/TitleBarConfiguration.h` y `.cpp`**:
   - Constantes canónicas de dimensiones para densidades Estándar (32 px), Senior (48 px) y Compacta (28 px).
   - Dimensiones mínimas de botones de control ($46\times 32$ px estándar, $48\times 48$ px Senior).
   - Presupuestos mínimos de área de arrastre (120 px estándar, 160 px Senior).
   - Factorías canónicas `canonicalDefaultTitleBar()`, `canonicalSeniorTitleBar()` y `canonicalCompactTitleBar()`.
   - Método estático `verifyInvariants(const TitleBarLayoutPlan&)` que certifica no solapamiento, límites dimensionales y suficiencia de arrastre.
3. **Creación de `apps/vigilio-browser/src/core/TitleBarValidator.h` y `.cpp`**:
   - Métodos puros de validación: `validateDimensions`, `validatePlan`.
   - Algoritmo espacial determinista `calculateTitleBarLayout(int windowWidth, const TitleBarConfig&)`.
   - Función analítica de clasificación espacial `hitTest(int x, int y, const TitleBarLayoutPlan&)`.
   - Funciones de texto público y accesibilidad en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
4. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Incorporación de `TitleBarConfiguration.cpp` y `TitleBarValidator.cpp` a la biblioteca `vigilio_core`.
   - Definición del nuevo target ejecutable de pruebas unitarias `vigilio_title_bar_tests` enlazando `vigilio_core` y `vigilio_compiler_warnings`, etiquetado en CTest como `unit;fast`.
5. **Creación de la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`**:
   - 14 pruebas unitarias exhaustivas en C++20 puro validando el modelo de dominio en modo headless.

---

## Diseño completo de la capacidad

### 1. Entidades de dominio y modelo conceptual
- **`TitleBarMode`**:
  - `NativeSystem`: La ventana delega la barra de título al gestor de ventanas del sistema operativo.
  - `CustomIntegrated`: La ventana dibuja su propia barra de título integrada, gestionando iconos, título, área de arrastre y botones de control.
  - `Suppressed`: La barra de título se oculta (modo pantalla completa / fullscreen o presentación sin bordes).
- **`TitleBarControlsPlacement`**:
  - `Right`: Los botones de minimizar, maximizar y cerrar se sitúan en el extremo superior derecho (estándar Windows y distribuciones Linux habituales).
  - `Left`: Los botones de control se ubican en el extremo superior izquierdo (estilo macOS o configuraciones de accesibilidad zurda).
- **`TitleBarButtonType`**:
  - `Minimize`: Minimiza la ventana a la barra de tareas o dock.
  - `MaximizeRestore`: Alterna entre el estado maximizado a pantalla completa y el tamaño restaurado de ventana.
  - `Close`: Cierra la ventana del navegador.
  - `SeniorModeToggle`: Botón de acceso rápido para alternar la «Interfaz más fácil de usar» directamente desde la barra de título.
- **`TitleBarHitTestZone`**:
  - Clasificación desacoplada para la capa de presentación:
    - `Nowhere`: Fuera de los límites de la barra.
    - `ClientArea`: Área asignada a controles interactivos del cliente (ej. omnibox o pestañas integradas).
    - `DraggableSpace`: Región no interactiva donde presionar y arrastrar debe mover la ventana física.
    - `WindowIcon`: Área del icono de marca (doble clic puede cerrar la ventana según convención de plataforma).
    - `CaptionText`: Área del título textual de la ventana (arrastrable salvo selección).
    - `MinimizeButton`, `MaximizeButton`, `CloseButton`, `SeniorModeToggle`: Áreas rectangulares de botones interactivos de control.

### 2. Geometría analítica y cálculo determinista
El algoritmo `calculateTitleBarLayout(int windowWidth, const TitleBarConfig& config)` descompone la anchura total de la ventana en una secuencia de rectángulos disjuntos:
1. Determina la altura canónica según `config.density`:
   - `LayoutDensity::Default`: 32 px.
   - `LayoutDensity::Senior`: 48 px.
   - `LayoutDensity::Compact`: 28 px.
2. Si `config.mode == TitleBarMode::NativeSystem` o `Suppressed`, genera un plan con altura 0 o inactiva y zonas no asignadas.
3. Si `config.mode == TitleBarMode::CustomIntegrated`:
   - Reserva el bloque de icono de marca en el margen opuesto a los controles de ventana (o margen izquierdo si controles a la derecha).
   - Calcula el ancho de la caja de controles sumando los anchos de cada botón visible (`Minimize`, `MaximizeRestore`, `Close`, y opcionalmente `SeniorModeToggle` si `config.showSeniorToggle`).
   - Ubica la caja de controles según `config.placement`:
     - `Right`: alineada al borde derecho ($x = \text{windowWidth} - \text{boxWidth}$).
     - `Left`: alineada al borde izquierdo ($x = 0$).
   - Asigna las posiciones individuales de los botones dentro de la caja de controles garantizando separación de 1 px o 4 px y ausencia total de solapamiento.
   - El espacio horizontal restante entre el icono (o margen) y la caja de controles se divide entre el título de ventana (`CaptionText`) y el área de arrastre pura (`DraggableSpace`).
   - Si el espacio de arrastre resultante es menor que `kMinDraggableAreaWidth` (120 px estándar / 160 px Senior), emite `TitleBarError::DraggableAreaInsufficient`.

---

## Archivos afectados

| Archivo | Acción | Propósito técnico |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/TitleBarTypes.h` | Nuevo | Tipos de datos, enums, estructuras de semántica de valor inmutable y reporte de barra de título. |
| `apps/vigilio-browser/src/core/TitleBarConfiguration.h` | Nuevo | Constantes canónicas dimensionales, factorías de planes canónicos y verificación de invariantes. |
| `apps/vigilio-browser/src/core/TitleBarConfiguration.cpp` | Nuevo | Implementación de factorías y comprobación de invariantes en C++20. |
| `apps/vigilio-browser/src/core/TitleBarValidator.h` | Nuevo | Declaración de validador estático, cálculo espacial, hit-testing y funciones de texto accesible. |
| `apps/vigilio-browser/src/core/TitleBarValidator.cpp` | Nuevo | Implementación del algoritmo de distribución espacial, hit-testing y UX copy en español neutro. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Registro de nuevos fuentes en `vigilio_core` y definición del target de prueba `vigilio_title_bar_tests`. |
| `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp` | Nuevo | Suite de 14 pruebas unitarias de especificación en CTest. |
| `docs/rules/01-modules/ux-shell/micro/0064-title-bar-integration/task.md` | Modificación | Actualización de estado formal tras la etapa domain. |
| `openspec/changes/module-ux-shell-task0064/proposal.md` | Modificación | Propuesta técnica integral y exhaustiva (este documento). |
| `openspec/changes/module-ux-shell-task0064/design.md` | Modificación | Documento de diseño técnico, estados CURRENT/TARGET/GAP, compatibilidad y rollback. |
| `openspec/changes/module-ux-shell-task0064/tasks.md` | Modificación | Desglose ordenado de tareas verificables con comandos y criterios. |
| `openspec/changes/module-ux-shell-task0064/execution-memory.md` | Modificación | Memoria de ejecución, registro de skills, invariantes, riesgos y handoff. |

---

## Contrato UX/UI

### 1. Directrices públicas de producto y terminología aprobada
De conformidad absoluta con `docs/UX-COPY-PUBLICO.md`:
- Queda terminantemente proscrito exponer en etiquetas, tooltips, descripciones o reportes diagnósticos las expresiones técnicas: «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter», «toolchain».
- La denominación pública y obligatoria para la modalidad accesible es **«Interfaz más fácil de usar»**.
- Textos y etiquetas oficiales de los botones de control:
  - **Minimizar**:
    - Etiqueta pública: `Minimizar`.
    - Etiqueta accesible: `Minimizar la ventana`.
    - Descripción flotante (tooltip): `Minimizar la ventana del navegador`.
  - **Maximizar / Restaurar**:
    - Cuando no está maximizada: Etiqueta `Maximizar`, accesible `Maximizar la ventana`, tooltip `Aumentar la ventana al tamaño completo de la pantalla`.
    - Cuando está maximizada: Etiqueta `Restaurar`, accesible `Restaurar la ventana`, tooltip `Restaurar la ventana al tamaño normal`.
  - **Cerrar**:
    - Etiqueta pública: `Cerrar`.
    - Etiqueta accesible: `Cerrar la ventana`.
    - Descripción flotante (tooltip): `Cerrar la ventana del navegador`.
  - **Acceso rápido a interfaz accesible**:
    - Etiqueta pública: `Interfaz más fácil de usar`.
    - Etiqueta accesible: `Activar o desactivar la interfaz más fácil de usar`.
    - Descripción flotante (tooltip): `Aumentar el tamaño de los botones y textos para una lectura más cómoda`.

### 2. Especificación visual y Modo Senior
- **Modo Estándar (Default)**:
  - Altura de barra: 32 px.
  - Dimensiones de botones de control: $46\times 32$ px (ancho generoso para ratón de escritorio).
  - Icono de marca: $16\times 16$ px con margen lateral de 10 px.
  - Tipografía: Inter / fuente del sistema a 12 px regular, color texto sobrio de alto contraste sobre fondo azul oscuro.
- **Modo Senior («Interfaz más fácil de usar»)**:
  - Altura de barra: 48 px (garantizando cumplimiento estricto del criterio WCAG 2.1 AAA de tap targets $\ge 48$ px).
  - Dimensiones de botones de control: $\ge 48\times 48$ px (área táctil y de clic amplia, evitando pulsaciones accidentales).
  - Separación entre botones: 4 px de margen protector entre el botón Cerrar y los controles adyacentes.
  - Icono de marca: $24\times 24$ px.
  - Tipografía: Inter / fuente del sistema a 14 px bold con contraste reforzado.
  - Foco visible: Borde perimetral de 2 px de alto contraste en el estado de foco por teclado.
- **Modo Compacto (Compact)**:
  - Altura de barra: 28 px.
  - Dimensiones de botones: $36\times 28$ px.
  - Preservación estricta de un área mínima de arrastre de 100 px.

---

## Reglas y fallos

### 1. Invariantes y precondiciones
- **Invariante de no solapamiento**: Ningún par de botones o elementos asignados en la barra de título puede intersecarse espacialmente ($\forall i \ne j, C_i \cap C_j = \emptyset$).
- **Invariante de suficiencia de arrastre**: En una barra integrada personalizada, la suma de anchuras de icono, título y controles no puede asfixiar el área libre para mover la ventana. Debe garantizarse un mínimo ineludible de 120 px (estándar) o 160 px (Senior).
- **Invariante de altura accesible**: En la «Interfaz más fácil de usar», la altura de la barra no puede ser inferior a 48 px bajo ninguna circunstancia.

### 2. Taxonomía de errores tipados (`TitleBarError`) y recuperación
- `None`: La configuración o cálculo es plenamente válida.
- `DimensionsBelowMinimum`: La anchura total de ventana proporcionada es inferior al mínimo requerido ($< 400$ px estándar / $< 500$ px Senior).
- `ControlsOverflowWidth`: El ancho requerido por los controles solicitados excede el ancho total disponible de la ventana.
- `DraggableAreaInsufficient`: El espacio restante para mover la ventana es inferior al umbral canónico seguro ($< 120$ px).
- `ConflictingControlPlacement`: Se especifican parámetros de posicionamiento incompatibles o solapados.
- `SeniorModeMinimumHeightViolation`: Se intenta configurar una barra en Modo Senior con altura inferior a 48 px.
- `UnsupportedWindowChromeMode`: Modalidad de ventana no soportada.

En caso de cualquier anomalía, `calculateTitleBarLayout` no lanza excepciones no controladas ni bloquea el hilo principal: retorna un `TitleBarReport` con `valid = false`, el código de error tipado, una sugerencia de remediación comprensible en español neutro generada por `generateRemediationHint`, y un plan de fallback seguro con dimensiones degradadas.

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Cero dependencias externas, cero telemetría y cero conexiones de red.
  - Todos los cálculos se realizan localmente en memoria y en tiempo lineal $\mathcal{O}(N)$ donde $N$ es el número de controles ($N \le 5$).
- **Privacidad y sanitización del título de ventana**:
  - La barra de título no debe exponer ni registrar URLs completas, parámetros sensibles de autenticación (tokens, hashes, session keys en cadenas de consulta `?auth=...`) ni contraseñas.
  - El título modelado debe representar exclusivamente el nombre del sitio web o el título limpio de la página, complementado con `"Vigilio Browser"` o la indicación `"Vigilio Browser (Navegación privada)"` en sesiones de incógnito.
- **Aislamiento de procesos y memoria**:
  - El modelo de dominio es 100% independiente de Qt WebEngine y no instancia subprocesos Chromium.
  - Memoria estática y libre de asignaciones dinámicas pesadas (consumo $< 2$ KB por cálculo de layout).

---

## Matriz de pruebas

La suite `vigilio_title_bar_tests` en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp` implementará 14 pruebas unitarias de especificación trazables:

| ID de Prueba | Caso evaluado | Fixture / Datos | Criterio de aceptación esperado |
| --- | --- | --- | --- |
| `TEST-001` | Invariantes del plan canónico Estándar | Ventana $1200\times 780$ px | Altura 32 px, botones $46\times 32$ px, controles a la derecha, arrastre $\ge 120$ px, `valid == true`. |
| `TEST-002` | Invariantes del plan canónico Senior | Ventana $1280\times 840$ px | Altura $\ge 48$ px, botones $\ge 48\times 48$ px, botón de acceso fácil presente, arrastre $\ge 160$ px. |
| `TEST-003` | Invariantes del plan canónico Compacto | Ventana $1024\times 640$ px | Altura 28 px, botones $36\times 28$ px, arrastre $\ge 100$ px, `valid == true`. |
| `TEST-004` | Hit-testing de área de arrastre | Coordenadas en zona central vacía | `hitTest(x, y)` retorna exactamente `TitleBarHitTestZone::DraggableSpace`. |
| `TEST-005` | Hit-testing de botones de control | Coordenadas sobre botón de cerrar, minimizar y maximizar | Retorna `CloseButton`, `MinimizeButton`, `MaximizeButton` con exactitud de píxel. |
| `TEST-006` | Hit-testing de icono de marca | Coordenadas sobre $x \in [10, 26]$, $y \in [8, 24]$ | Retorna `TitleBarHitTestZone::WindowIcon`. |
| `TEST-007` | Alternancia de estado maximizado | `config.state.isMaximized = true` vs `false` | Botón cambia entre `Maximize` y `Restore`, tooltip y accesibilidad sincronizados. |
| `TEST-008` | Estado de ventana inactiva | `config.state.isActive = false` | El plan refleja el estado inactivo para atenuación visual sin alterar geometría. |
| `TEST-009` | Colocación de controles a la izquierda (macOS) | `config.placement = Left` | Caja de controles situada en $x = 0$, orden `Close`, `Minimize`, `MaximizeRestore`. |
| `TEST-010` | Rechazo por anchura insuficiente | Ventana $< 400$ px o controles excesivos | Retorna `TitleBarError::DraggableAreaInsufficient` o `DimensionsBelowMinimum`. |
| `TEST-011` | Rechazo por violación de altura mínima Senior | Plan Senior con altura $< 48$ px | `verifyInvariants` retorna `false` y validador emite `SeniorModeMinimumHeightViolation`. |
| `TEST-012` | Disyunción analítica de controles | Plan calculado estándar y Senior | Ningún par de rectángulos asignados tiene intersección ($\forall i \ne j, C_i \cap C_j = \emptyset$). |
| `TEST-013` | Cumplimiento de lenguaje público y copy | Evaluación de funciones `*ToString` | Cero apariciones de «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec». |
| `TEST-014` | Sanitización de títulos y privacidad | Entrada con URL completa y tokens | Verificación de que el título devuelto no expone parámetros sensibles. |

---

## Pasos APPLY

1. **Crear `apps/vigilio-browser/src/core/TitleBarTypes.h`**:
   - Declarar enums tipados `TitleBarMode`, `TitleBarControlsPlacement`, `TitleBarButtonType`, `TitleBarButtonState`, `TitleBarHitTestZone`, `TitleBarError`.
   - Declarar estructuras inmutables `TitleBarDimensions`, `TitleBarControlSpec`, `TitleBarState`, `TitleBarConfig`, `TitleBarLayoutPlan`, `TitleBarReport`.
2. **Crear `apps/vigilio-browser/src/core/TitleBarConfiguration.h` y `.cpp`**:
   - Implementar constantes canónicas dimensionales y de accesibilidad.
   - Implementar factorías canónicas `canonicalDefaultTitleBar`, `canonicalSeniorTitleBar`, `canonicalCompactTitleBar`.
   - Implementar función matemática pura `verifyInvariants`.
3. **Crear `apps/vigilio-browser/src/core/TitleBarValidator.h` y `.cpp`**:
   - Implementar validaciones de dimensiones y congruencia de plan.
   - Implementar algoritmo analítico `calculateTitleBarLayout`.
   - Implementar función de clasificación espacial `hitTest`.
   - Implementar funciones de UX copy público en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
4. **Actualizar `apps/vigilio-browser/CMakeLists.txt`**:
   - Agregar `TitleBarConfiguration.cpp` y `TitleBarValidator.cpp` a la biblioteca `vigilio_core`.
   - Registrar el ejecutable `vigilio_title_bar_tests` enlazado a `vigilio_core` y `vigilio_compiler_warnings` en CTest con etiquetas `unit;fast`.
5. **Crear `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`**:
   - Implementar las 14 pruebas unitarias de especificación en C++20 puro.
6. **Compilar y validar en el pipeline continuo**:
   - Compilar con CMake y GCC/Clang con `-Werror`.
   - Ejecutar la suite completa de pruebas en CTest verificando que las 28 pruebas pasan al 100%.
7. **Sincronizar documentación y referencias**:
   - Actualizar el estado de `docs/rules/01-modules/ux-shell/micro/0064-title-bar-integration/task.md`.
   - Validar referencias canónicas y lockfile con Prompt Builder CLI 0.6.3.

---

## Definición de terminado (DoD)

- [ ] Todos los tipos de datos, enumeraciones y estructuras inmutables de `TitleBarTypes.h` están implementados en C++20 estándar sin dependencias de GUI.
- [ ] Las constantes canónicas, factorías y verificación de invariantes en `TitleBarConfiguration` garantizan alturas $\ge 48$ px y tap targets $\ge 48\times 48$ px para el Modo Senior.
- [ ] El validador estático, algoritmo espacial y función `hitTest` en `TitleBarValidator` operan de forma analítica, determinista y sin solapamiento espacial.
- [ ] Todas las funciones de texto cumplen estrictamente con `docs/UX-COPY-PUBLICO.md`, sin tecnicismos proscritos.
- [ ] La suite de 14 pruebas unitarias de especificación compila limpiamente bajo `-Werror` y corre en CTest con 100% de éxito en modo headless (< 50 ms).
- [ ] El script de verificación continua pasa con exit code 0.
- [ ] Las referencias canónicas y el lockfile de Prompt Builder CLI son validados exitosamente (`valid: true`).

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.ux-shell.task0063`: Concluida y verificada exitosamente en el commit `b7892dc`.
  - No existen dependencias abiertas ni bloqueantes técnicos para la etapa `domain`.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Acoplamiento accidental de dependencias de ventana nativa del sistema operativo en el núcleo de dominio.
    - *Mitigación*: El modelo opera exclusivamente con geometría cartesiana abstracta (`LayoutRect`), dejando la traducción a eventos Win32/X11 a las etapas posteriores `ui` (`task0066`).
  - *Riesgo 2*: Reducción excesiva del área de arrastre al integrar títulos largos de pestaña o controles adicionales.
    - *Mitigación*: Validación preventiva que rechaza planes con espacio de arrastre $< 120$ px (`DraggableAreaInsufficient`), asegurando siempre la maniobrabilidad de la ventana.
  - *Riesgo 3*: Incumplimiento de accesibilidad en pantallas táctiles o para usuarios mayores.
    - *Mitigación*: Aplicación incondicional de botones $\ge 48\times 48$ px y altura $\ge 48$ px en la «Interfaz más fácil de usar».

---

## Commit propuesto

```
docs(ux-shell): domain title-bar-integration
```
