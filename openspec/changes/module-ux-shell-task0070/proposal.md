# Change Proposal — module-ux-shell-task0070

Pack: `module.ux-shell.task0070`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **ui** de la capacidad **tab strip density** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0070 — Implementar la interacción y estados visuales en Qt: tab strip density`).

Habiéndose completado y certificado formalmente en la etapa precedente (`module.ux-shell.task0069`, commit `e05f659`) la formalización del modelo de dominio de densidad de tira de pestañas, especificaciones de pestaña (`TabSpec`), cálculo elástico sin solapamientos, constantes canónicas (`TabStripDensityConfiguration`), validador espacial analítico (`TabStripDensityValidator`) y el servicio desacoplado `vigilio::TabStripDensityService` en C++20 puro con sincronización multihilo, persistencia local atómica, aislamiento estricto de sesiones de incógnito y cancelación cooperativa Zero-Zombie, el objetivo primordial de esta propuesta es **diseñar e implementar la interacción física Qt Widgets de la tira de pestañas (`vigilio-tabs`), la integración visual con `TabStripDensityService` en `BrowserWindow`, el soporte elástico y reactivo de densidades visuales (`Default` a 36 px, `Senior` / «Interfaz más fácil de usar» a 48 px, y `Compact` a 28 px), la gestión visual de estados (normal, hover, selected, pressed, loading, error, discarded/reposo, fijada/pinned, reproducción/silenciamiento de audio), botón de nueva pestaña integrado con tap targets accesibles ($\ge 48\times 48$ px en modo accesible), navegación exhaustiva por teclado, soporte de tecnologías asistivas y la suite de pruebas automatizadas en `tests/ui/WindowTests.cpp`**, en estricta conformidad con las referencias visuales de `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png`, el isotipo de `/resultado/06-logo-instalador-oficial.png` y la política de lenguaje público de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
1. **Integración de `TabStripDensityService` en `BrowserWindow`**:
   - Incorporación de `std::unique_ptr<vigilio::TabStripDensityService> tabStripDensityService_` en `BrowserWindow`.
   - Inicialización coordinada del servicio en `BrowserWindow::setupTabStrip` con la configuración canónica según la preferencia de densidad persistida en `QSettings` (`canonicalDefaultDensity()` o `canonicalSeniorDensity()`).
   - Sincronización estricta del aislamiento de incógnito (`tabStripDensityService_->setIncognitoMode(isIncognito_)`), garantizando cero escrituras a disco en sesiones privadas (ADR-001 / Decisión Q-001).
   - Suscripción reactiva mediante el callback `setTabStripChangedCallback` para actualizar estilos, geometrías de pestañas y botones de control ante cualquier mutación de densidad o redimensionamiento.
2. **Estructura jerárquica de widgets de la tira de pestañas (`vigilio-tabs`)**:
   - Contenedor de pestañas `QTabWidget#vigilio-tabs` y su barra inferior `QTabBar` configurados con soporte para cierre interactivo, arrastre y ordenación de pestañas.
   - Botón de nueva pestaña (`QPushButton#vigilio-tab-new-button`): ubicado como widget de esquina (`setCornerWidget(..., Qt::TopRightCorner)`) o contiguo a las pestañas activas, con icono vectorial `icons/navigation/new-tab.svg` o `icons/navigation/add.svg`, tooltip accesible «Nueva pestaña» y dimensiones proporcionales a la densidad ($28\times 28$ px en estándar, $48\times 48$ px en accesible, $24\times 24$ px en compacto).
3. **Máquina de estados visuales e interactivos de pestañas**:
   - **Estado Normal / Inactivo**: Fondo sobrio `#1C2839`, texto legible `#E8ECF3`.
   - **Estado Seleccionado / Activo**: Superficie resaltada `#33435A`, texto destacado `#FFFFFF`, indicador de pestaña activa y botón de cierre visible.
   - **Estado Hover**: Realce visual sutil `#25344C` con contraste WCAG 2.1 AA.
   - **Estado Foco accesible**: Borde de 2 px de alto contraste `#3B82F6` / `#2563EB` con contorno nítido.
   - **Estado Fijada (Pinned)**: Anchura compacta ($36$ px estándar / $48$ px accesible / $28$ px compacto) mostrando únicamente el favicon o icono de sitio sin título ni botón de cierre.
   - **Estado Multimedia / Audio (`TabMediaState`)**: Indicador interactivo de altavoz en pestañas con reproducción sonora; soporte visual de audio sonando vs silenciado («Silenciar pestaña» / «Activar sonido de pestaña»).
   - **Estado en Reposo / Ahorro de memoria (`TabDiscardState`)**: Pestañas descargadas de RAM para optimización («Liberar memoria de pestañas inactivas» / «Pestaña en reposo») con título atenuado `#94A3B8` y tooltip explicativo.
   - **Estados de Carga y Error (`TabLoadingState`)**: Título provisional «Cargando…» durante navegación activa y «No se pudo cargar» ante fallos de red o recursos inexistentes.
   - **Botón de cierre dinámico**: Dimensiones canónicas ($16\times 16$ px estándar / $24\times 24$ px accesible / $14\times 14$ px compacto); compresión elástica que oculta automáticamente el botón de cierre en pestañas inactivas cuando el ancho disponible cae bajo el umbral crítico ($90$ px estándar / $140$ px accesible), manteniéndolo siempre accesible en la pestaña actualmente seleccionada.
4. **Adaptación reactiva a la «Interfaz más fácil de usar» (Modo Senior)**:
   - Sincronización bidireccional entre `setSeniorMode(bool)` y `tabStripDensityService_->setDensity()`:
     - Altura de la tira de pestañas ampliada de 36 px a 48 px.
     - Anchura mínima de pestaña incrementada de 120 px a 140 px (ancho preferido de 200 px a 220 px, máximo a 280 px).
     - Botón de nueva pestaña con tap target mínimo $\ge 48\times 48$ px.
     - Botón de cierre en pestañas con tap target accesible $\ge 24\times 24$ px (con padding táctil expandido).
     - Tipografía de pestañas escalada a 12pt/13pt con espaciado entre pestañas incrementado a 4 px.
5. **Estilos visuales QSS y diseño sobrio**:
   - Integración con el sistema de temas (`setDarkTheme(bool)`), proveyendo selectores especializados para `QTabWidget#vigilio-tabs`, `QTabBar::tab`, botones de cierre, botón de nueva pestaña e indicadores de estado, respetando los tonos oscuros `#141D2B` y `#182334` de `/resultado/01-vigilio-home.png`.
6. **Accesibilidad y cumplimiento estricto de lenguaje público (`docs/UX-COPY-PUBLICO.md`)**:
   - Proscripción absoluta de jerga de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter») en textos visibles, tooltips, etiquetas accesibles y descripciones para lectores de pantalla.
   - Uso obligatorio de términos aprobados: **«Interfaz más fácil de usar»**, **«Nueva pestaña»**, **«Cerrar pestaña»**, **«Silenciar pestaña»**, **«Pestaña en reposo»**, **«Cargando…»** y **«No se pudo cargar»**.
7. **Batería de pruebas en Qt Test (`tests/ui/WindowTests.cpp`)**:
   - Aserciones automatizadas verificando inicialización de `tabStripDensityService()`, geometrías de pestaña en estándar vs accesible, dimensiones mínimas de tap targets $\ge 48$ px en modo Senior, sincronización de redimensionamiento de ventana y auditoría de textos accesibles.

---

## Out of scope

- Modificación directa del código de producción en `apps/vigilio-browser/src/ui/**` o creación de archivos en `apps/**` durante esta fase PROPOSE (reservado exclusivamente para la fase APPLY tras aprobación humana).
- Implementación de extensiones de navegador Chromium propietarias para manipulación de pestañas (la gestión se realiza mediante Qt Widgets y el modelo de datos local).
- Telemetría remota, sincronización en la nube de pestañas o cuentas Vigilio (prohibido taxativamente por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Pruebas E2E de estrés multimonitor con compositores de ventana Wayland propietarios (la suite de verificación se concentra en pruebas unitarias e integración en Qt Test determinista).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completó formalmente la microtarea `0069` (`module.ux-shell.task0069`) en el commit `e05f659`, estableciendo los tipos de dominio (`TabStripDensityTypes.h`), configuración canónica (`TabStripDensityConfiguration.h/.cpp`), validador espacial analítico (`TabStripDensityValidator.h/.cpp`) y 33 pruebas unitarias de cálculo de distribución espacial sin solapamientos en `tests/unit/TabStripDensityTests.cpp`.
   - Se cuenta con la implementación del servicio desacoplado `vigilio::TabStripDensityService` (`TabStripDensityService.h/.cpp`) con soporte multihilo (`std::mutex`), persistencia local atómica (`saveStateToFile`, `loadStateFromFile`), aislamiento de incógnito, sincronización de callbacks y cancelación cooperativa Zero-Zombie (`CancellationToken`).
   - La suite de CTest ejecuta 30 de 30 pruebas pasando al 100% en ~3.8 segundos sin advertencias con `-Werror` activado en GCC 15.2.0 bajo `apps/vigilio-browser/scripts/check.sh`.
2. **Exploración comprobable de la interfaz actual (`BrowserWindow`)**:
   - `BrowserWindow.h` y `BrowserWindow.cpp` instancian actualmente un `QTabWidget* tabs_` (`setObjectName("vigilio-tabs")`) con pestañas cerrables y movibles.
   - Sin embargo, la interfaz actual presenta las siguientes carencias respecto a la capacidad canónica de densidad de tira de pestañas:
     - No integra `TabStripDensityService` como miembro ni delega en él las mutaciones de pestañas, densidades o cálculos geométricos.
     - La altura de la tira de pestañas y de las pestañas individuales es estática o fijada por defecto en estilos genéricos (`QTabBar::tab { padding: 8px 12px; }`), sin adaptarse canónicamente a 36 px (Default), 48 px (Senior) ni 28 px (Compact).
     - La activación de la «Interfaz más fácil de usar» (`setSeniorMode`) no actualiza la densidad de la tira de pestañas ni garantiza el tap target accesible $\ge 48\times 48$ px para el botón de nueva pestaña o controles de pestaña.
     - No existe un botón de nueva pestaña (`vigilio-tab-new-button`) integrado ergonómicamente en la cabecera de la tira de pestañas.
     - No se representan visualmente los estados de ahorro de memoria (`TabDiscardState::Discarded`), multimedia (`TabMediaState`), ni la compresión elástica que oculta botones de cierre en pestañas estrechas inactivas.
3. **Referencias visuales y directrices vinculantes**:
   - `/resultado/01-vigilio-home.png`: Muestra una barra de pestañas integrada sobre el contenido, fondo sobrio azul oscuro (`#141D2B`), pestaña seleccionada con superficie contrastada (`#33435A`), pestañas inactivas en `#1C2839` y botón de nueva pestaña '+' discreto.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Muestra la interfaz accesible («Interfaz más fácil de usar») con botones amplios, fuentes escaladas, alta legibilidad y separaciones cómodas.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial para el isotipo y proporciones de marca.
   - `docs/UX-COPY-PUBLICO.md`: Prohíbe estrictamente mostrar «Senior Mode» o «Modo Senior» en la interfaz visible y exige «Interfaz más fácil de usar».

---

## Alcance exacto

El alcance de este pack se circunscribe estrictamente a la integración de interfaz Qt Widgets para la capacidad **tab strip density**:

1. **En `apps/vigilio-browser/src/ui/BrowserWindow.h`**:
   - Inclusión de las cabeceras de servicio y tipos: `core/TabStripDensityService.h`, `core/TabStripDensityTypes.h` y `core/TabStripDensityConfiguration.h`.
   - Declaración de miembros de servicio y widgets de tira de pestañas:
     - `std::unique_ptr<vigilio::TabStripDensityService> tabStripDensityService_;`
     - `QPushButton* tabNewButton_{nullptr};`
   - Declaración de métodos privados de configuración y sincronización:
     - `void setupTabStrip(const vigilio::StartupDiagnosticReport& report);`
     - `void applyTabStripDensityPlan(const vigilio::TabStripDensityPlan& plan);`
     - `void syncTabsToService();`
   - Métodos públicos de consulta para pruebas e inspección:
     - `[[nodiscard]] vigilio::TabStripDensityService* tabStripDensityService() const noexcept { return tabStripDensityService_.get(); }`
     - `[[nodiscard]] QTabWidget* tabWidget() const noexcept { return tabs_; }`
     - `[[nodiscard]] QTabBar* tabBar() const noexcept { return tabs_ ? tabs_->tabBar() : nullptr; }`
     - `[[nodiscard]] QPushButton* tabNewButton() const noexcept { return tabNewButton_; }`
2. **En `apps/vigilio-browser/src/ui/BrowserWindow.cpp`**:
   - En el constructor de `BrowserWindow`:
     - Invocar `setupTabStrip(report)` de forma coordinada con `setupLayoutService(report)` y `setupTitleBar(report)`.
   - En `setupTabStrip(const vigilio::StartupDiagnosticReport& report)`:
     - Instanciar `tabStripDensityService_` con el plan canónico correspondiente (`canonicalDefaultDensity()` o `canonicalSeniorDensity()`) según la preferencia de accesibilidad persistida.
     - Configurar el modo incógnito (`tabStripDensityService_->setIncognitoMode(isIncognito_)`).
     - Configurar el botón de nueva pestaña (`tabNewButton_`) con el icono `icons/navigation/add.svg` o glifo '+', tooltip «Nueva pestaña», conectando su señal `clicked` a `openTab()`.
     - Insertar `tabNewButton_` como corner widget de `tabs_` (`tabs_->setCornerWidget(tabNewButton_, Qt::TopRightCorner)`).
     - Registrar el callback de observabilidad `setTabStripChangedCallback` para invocar `applyTabStripDensityPlan` ante cualquier recalculo.
   - En `applyTabStripDensityPlan(const vigilio::TabStripDensityPlan& plan)`:
     - Ajustar la altura visual de `tabs_->tabBar()` y estilos de `QTabBar::tab` según `plan.height` (36 px en estándar, 48 px en accesible, 28 px en compacto).
     - Ajustar dimensiones mínimas de `tabNewButton_` según densidad ($28\times 28$ px en estándar, $48\times 48$ px en accesible).
     - Actualizar tooltips y nombres accesibles sin términos proscritos.
   - En `resizeEvent(QResizeEvent* event)`:
     - Sincronizar el ancho disponible con `tabStripDensityService_->setAvailableWidth(width())`.
   - En `setSeniorMode(bool enabled)`:
     - Sincronizar la densidad del servicio de pestañas: `tabStripDensityService_->setDensity(enabled ? vigilio::LayoutDensity::Senior : vigilio::LayoutDensity::Default)`.
     - Actualizar la apariencia y dimensiones táctiles de las pestañas a $\ge 48$ px.
   - En `setDarkTheme(bool enabled)`:
     - Incorporar selectores QSS detallados para `QTabWidget#vigilio-tabs`, `QTabBar::tab`, estados `:selected`, `:hover`, `:focus`, `QPushButton#vigilio-tab-new-button` y botones de cierre.
3. **En `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Incorporar pruebas deterministas verificando:
     - Inicialización válida de `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()`.
     - Verificación de altura canónica de tira de pestañas ($36$ px estándar, $48$ px accesible).
     - Verificación de tap target mínimo $\ge 48\times 48$ px para `tabNewButton()` en la «Interfaz más fácil de usar».
     - Sincronización de ancho ante `resizeEvent`.
     - Auditoría estricta de textos públicos en etiquetas y tooltips.

---

## Diseño completo de la capacidad

### 1. Entidades, métodos y responsabilidades
- **`vigilio::TabStripDensityService` (`src/core/`)**:
  - Propietario canónico del cálculo matemático de distribución espacial, gestión de colección de pestañas (`TabSpec`), selección activa, estados fijados, multimedia y reposo, y persistencia atómica local.
- **`BrowserWindow` (`src/ui/`)**:
  - Presentador Qt Widgets que aloja la jerarquía de interfaz. No duplica lógica de cálculo geométrico; delega en `tabStripDensityService_` y materializa el plan recibido (`TabStripDensityPlan`) mediante QSS y ajustes de propiedades en `QTabWidget` y `QTabBar`.
- **`QTabWidget#vigilio-tabs` y `QTabBar`**:
  - Contenedor gráfico nativo de pestañas, con soporte de cierre, reordenación y menús contextuales.
- **`QPushButton#vigilio-tab-new-button`**:
  - Botón de acción primaria para crear pestañas con tap target adaptativo.

### 2. Precondiciones, postcondiciones e invariantes
- **Precondiciones**:
  - `BrowserWindow` debe inicializarse con un ancho $\ge 400$ px (límite mínimo del shell de escritorio).
- **Postcondiciones**:
  - Tras `setupTabStrip`, `tabStripDensityService()` no es nulo, `tabNewButton()` no es nulo y la tira de pestañas refleja la densidad activa.
  - Al cambiar a modo accesible (`setSeniorMode(true)`), la altura de la tira pasa a 48 px y todos los controles interactivos cumplen con tap targets $\ge 48$ px.
- **Invariantes**:
  - Ningún dato de pestaña en sesión de incógnito se persiste en disco local.
  - Ninguna etiqueta visible o accesible expone nombres internos proscritos.
  - Las pestañas nunca se solapan en la proyección horizontal.

---

## Archivos afectados

| Archivo | Naturaleza | Responsabilidad y cambios previstos |
| --- | --- | --- |
| `apps/vigilio-browser/src/ui/BrowserWindow.h` | Existente | Inclusión de cabeceras de `TabStripDensityService`, declaración de `tabStripDensityService_`, `tabNewButton_`, métodos `setupTabStrip`, `applyTabStripDensityPlan`, `syncTabsToService` y métodos de consulta públicos. |
| `apps/vigilio-browser/src/ui/BrowserWindow.cpp` | Existente | Implementación de `setupTabStrip`, construcción de `tabNewButton_`, suscripción al callback de densidad, aplicación de estilos QSS para pestañas en claro/oscuro, sincronización en `resizeEvent` y `setSeniorMode`. |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Existente | Batería de pruebas Qt Test verificando presencia de servicio, dimensiones canónicas, adaptación accesible y auditoría de textos públicos. |
| `docs/rules/01-modules/ux-shell/micro/0070-tab-strip-density/task.md` | Existente | Sincronización documental del estado de la microtarea canónica 0070. |

---

## Contrato UX/UI

### 1. Elementos visuales, posiciones y dimensiones
- **Ubicación**: Cúspide del contenedor principal (`mainContainer_`), inmediatamente debajo de `titleBarWidget_` y sobre `toolbar_`.
- **Altura de tira de pestañas**:
  - Estándar (`Default`): **36 px**.
  - Accesible (`Senior` / «Interfaz más fácil de usar»): **48 px**.
  - Compacto (`Compact`): **28 px**.
- **Ancho de pestañas**:
  - Estándar: preferido 200 px, mínimo 120 px, máximo 240 px, pestaña fijada 36 px.
  - Accesible: preferido 220 px, mínimo 140 px, máximo 280 px, pestaña fijada 48 px.
  - Compacto: preferido 160 px, mínimo 80 px, máximo 200 px, pestaña fijada 28 px.
- **Botón de nueva pestaña (`vigilio-tab-new-button`)**:
  - Estándar: $28\times 28$ px.
  - Accesible: **$\ge 48\times 48$ px** (tap target accesible).
  - Compacto: $24\times 24$ px.
- **Botón de cierre de pestaña**:
  - Estándar: $16\times 16$ px (oculto en inactivas si ancho $< 90$ px).
  - Accesible: $24\times 24$ px con padding ampliado (oculto en inactivas si ancho $< 140$ px).

### 2. Paleta de colores y tokens de diseño
- Fondo de barra: `#141D2B` (modo oscuro) / `#F3F6FB` (modo claro).
- Pestaña inactiva: `#1C2839` (texto `#E8ECF3`).
- Pestaña seleccionada: `#33435A` (texto `#FFFFFF`, borde inferior/acento `#3B82F6`).
- Hover en pestaña: `#25344C`.
- Foco accesible: borde de 2 px `#3B82F6` (`outline: none`).
- Botón de cierre hover: `#DC2626` (rojo de alerta sobrio con texto blanco).

### 3. Navegación por teclado y tecnologías asistivas
- Atajos: `Ctrl+T` (Nueva pestaña), `Ctrl+W` (Cerrar pestaña activa), `Ctrl+Tab` (Siguiente pestaña), `Ctrl+Shift+Tab` (Pestaña anterior), `Ctrl+1` a `Ctrl+8` (Pestaña N), `Ctrl+9` (Última pestaña).
- Foco visible en pestañas y botones mediante tabulación (`Qt::StrongFocus`).
- Nombres accesibles generados por `TabStripDensityValidator`:
  - Pestaña activa: «Pestaña 1 de 3: [Título], activa».
  - Botón nueva pestaña: «Nueva pestaña».
  - Botón de cierre: «Cerrar pestaña [Título]».
  - Botón de audio: «Silenciar pestaña con audio» / «Activar sonido de pestaña silenciada».
  - Pestaña en reposo: «Pestaña 2 de 3: [Título], en reposo para liberar memoria».

---

## Reglas y fallos

1. **Compresión elástica y desbordamiento**:
   - Cuando el número de pestañas excede el ancho disponible, las pestañas se comprimen proporcionalmente hasta su límite inferior (`minTabWidth`).
   - Al alcanzar dicho límite, los botones de cierre de las pestañas inactivas se ocultan para maximizar la legibilidad del título, permaneciendo siempre visible en la pestaña activa.
   - Ante desbordamiento extremo, la tira activa su modo de desplazamiento horizontal (`TabStripOverflowMode::Scrollable`).
2. **Entradas malformadas o títulos extensos**:
   - Los títulos se sanitizan mediante `TabStripDensityValidator::sanitizeTitle`, eliminando credenciales embebidas, parámetros sensibles de URLs y truncándose con elipsis visual sin romper la geometría.
3. **Manejo de estados de pestaña**:
   - En caso de error de carga en la página web, la pestaña muestra «No se pudo cargar» con opción de reintento desde la vista.
   - En caso de cierre de la última pestaña restante, se crea automáticamente una nueva pestaña vacía en la página de inicio, preservando la ventana abierta y operativa.

---

## Seguridad y privacidad

1. **Aislamiento Local-First (ADR-001 / Decisión Q-001)**:
   - Todo cálculo de distribución geométrica y representación gráfica se ejecuta 100% en local.
   - No se realiza ninguna llamada de red, sincronización cloud ni consulta externa para renderizar la tira de pestañas.
2. **Aislamiento de sesiones de navegación privada (Modo Incógnito)**:
   - En ventanas de incógnito (`isIncognitoSession() == true`), `tabStripDensityService_->setIncognitoMode(true)` desactiva la persistencia de estado de pestañas en disco.
   - Las pestañas privadas nunca se escriben en archivos de sesión ordinarios ni en el historial local.
3. **Sanitización de títulos**:
   - Los títulos de pestaña se limpian rigurosamente para evitar la exposición de tokens de sesión, credenciales (`user:pass@host`) o datos personales sensibles en etiquetas accesibles o tooltips.

---

## Matriz de pruebas

| Tipo de prueba | Criterio a verificar | Fixture / Arnés | Comando de verificación | Resultado esperado |
| --- | --- | --- | --- | --- |
| **Qt UI (Unit)** | Presencia de `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()` | `WindowTests.cpp` | `ctest --test-dir build -R ui_tests` | `tabStripDensityService()` y widgets no nulos. |
| **Qt UI (Geometría)** | Altura canónica de tira de pestañas en densidad estándar (36 px) | `WindowTests.cpp` | `ctest --test-dir build -R ui_tests` | Altura de pestaña / tira igual a 36 px. |
| **Qt UI (Accesibilidad)** | Altura y tap target en «Interfaz más fácil de usar» (48 px) | `WindowTests.cpp` | `ctest --test-dir build -R ui_tests` | Altura $\ge 48$ px, `tabNewButton` $\ge 48\times 48$ px. |
| **Qt UI (Redimensionamiento)** | Sincronización reactiva del ancho de tira en `resizeEvent` | `WindowTests.cpp` | `ctest --test-dir build -R ui_tests` | `tabStripDensityService()->availableWidth()` actualizado. |
| **Accesibilidad (Auditoría)** | Ausencia total de términos internos proscritos en tooltips y etiquetas | `WindowTests.cpp` | `ctest --test-dir build -R ui_tests` | 0 coincidencias de «Senior Mode», «Modo Senior», «Prompt Builder», «DAG». |
| **Integración CI** | Compilación estricta con `-Werror`, Clang-Format y ejecución CTest | `check.sh` | `bash apps/vigilio-browser/scripts/check.sh` | Exit code 0, 100% tests PASS. |

---

## Pasos APPLY

1. **APPLY-01**: Declarar en `apps/vigilio-browser/src/ui/BrowserWindow.h` los miembros `tabStripDensityService_`, `tabNewButton_`, métodos privados `setupTabStrip`, `applyTabStripDensityPlan`, `syncTabsToService` y métodos de consulta públicos.
2. **APPLY-02**: Implementar en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` el método `setupTabStrip` con inicialización del servicio, configuración de incógnito y registro del callback de observabilidad.
3. **APPLY-03**: Construir en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` el botón de nueva pestaña (`vigilio-tab-new-button`), conectando su señal a `openTab()` e integrándolo como corner widget en `tabs_`.
4. **APPLY-04**: Implementar en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` el método `applyTabStripDensityPlan` para sincronizar alturas de pestañas, padding, dimensiones de botones y textos accesibles.
5. **APPLY-05**: Sincronizar eventos de redimensionamiento en `BrowserWindow::resizeEvent` notificando a `tabStripDensityService_->setAvailableWidth(width())`.
6. **APPLY-06**: Adaptar reactivamente `BrowserWindow::setSeniorMode(bool)` para sincronizar la densidad en `tabStripDensityService_` (Senior vs Default) y redimensionar controles a $\ge 48$ px.
7. **APPLY-07**: Enriquecer los estilos QSS en `BrowserWindow::setDarkTheme(bool)` para la tira de pestañas, pestañas seleccionadas, hover, foco accesible y botón de nueva pestaña.
8. **APPLY-08**: Incorporar en `apps/vigilio-browser/tests/ui/WindowTests.cpp` las pruebas de presencia, dimensiones estándar y accesibles, tap targets y auditoría de textos públicos.
9. **APPLY-09**: Validar compilación limpia, Clang-Format y ejecución de la suite de pruebas mediante `apps/vigilio-browser/scripts/check.sh`.
10. **APPLY-10**: Sincronizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0070-tab-strip-density/task.md`.

---

## Definición de terminado

- [ ] Todos los archivos afectados en `apps/vigilio-browser/src/ui/**` y `tests/ui/**` implementados y compilados con C++20 y GCC 15.2.0 sin advertencias con `-Werror`.
- [ ] La tira de pestañas refleja la altura de 36 px en estándar y 48 px en la «Interfaz más fácil de usar».
- [ ] El botón de nueva pestaña y los controles de pestaña cumplen con el tap target mínimo $\ge 48\times 48$ px en modo accesible.
- [ ] La navegación por teclado y el foco visible de 2 px están plenamente operativos.
- [ ] Ningún texto de la interfaz pública contiene términos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter»).
- [ ] La sesión de incógnito preserva el aislamiento absoluto sin persistir datos en disco local.
- [ ] La suite de pruebas de CTest pasa al 100% con exit code 0.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.ux-shell.task0069` (completado y certificado en commit `e05f659`).
  - Qt 6 Widgets y recursos SVG en `resources.qrc`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI headless sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    - *Mitigación*: `WindowTests.cpp` detecta la ausencia de display y reporta `NOT_EXECUTED` limpiamente sin fallar el pipeline de compilación.

---

## Commit propuesto

`feat(ux-shell): ui tab-strip-density`
