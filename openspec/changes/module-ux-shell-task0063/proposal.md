# Change Proposal — module-ux-shell-task0063

Pack: `module.ux-shell.task0063`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **tests** de la capacidad **main window layout** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0063 — Probar casos positivos, negativos y recuperación: main window layout`).

Habiéndose completado y certificado formalmente en las etapas precedentes la formalización del modelo de dominio de layout (`module.ux-shell.task0060`, commit `4cb5d8f`), la implementación del servicio operativo desacoplado `vigilio::MainWindowLayoutService` en C++20 con persistencia atómica, tolerancia a fallos, aislamiento de incógnito y cancelación cooperativa Zero-Zombie (`module.ux-shell.task0061`, commit `c4d75c0`), y la integración visual de Qt Widgets en la ventana principal (`BrowserWindow`), estructuración del panel lateral (`vigilio-sidebar`), sincronización reactiva y adaptación dinámica a la «Interfaz más fácil de usar» (`module.ux-shell.task0062`, commit `d0e3fec`), el objetivo fundamental de este pack es **diseñar y estructurar la suite exhaustiva, determinista y de alta resiliencia de pruebas automatizadas para main window layout**.

La suite de pruebas proyectada cubre sistemáticamente:
- **Casos positivos y ciclo de vida integral del layout de ventana**:
  - Verificación exhaustiva de planes canónicos (`canonicalDefaultLayout`, `canonicalSeniorLayout`, `canonicalCompactLayout`), evaluando precondiciones, postcondiciones, restricciones dimensionales mínimas de ventana ($640\times 480$ px en estándar, $720\times 520$ px en accesible, $500\times 380$ px en compacto), y la distribución espacial sin solapamientos entre las zonas estructurales (`TitleBar`, `TabStrip`, `ToolBar`, `NotificationBanner`, `Sidebar`, `Content`, `StatusBar`).
  - Transiciones fluidas entre densidades de interfaz (`LayoutDensity::Default` $\leftrightarrow$ `LayoutDensity::Senior` $\leftrightarrow$ `LayoutDensity::Compact`), certificando la correcta recalculación de presupuestos geométricos (`ZoneBudget`), notificación síncrona a suscriptores vía callback y preservación del área útil de contenido ($\ge 400\times 250$ px).
  - Variaciones y mutaciones complejas de la barra lateral:
    - Alternancia de estados (`SidebarState::Collapsed` a 48 px $\leftrightarrow$ `SidebarState::Expanded` a 240 px $\leftrightarrow$ `SidebarState::Hidden`).
    - Configuración de anchuras personalizadas dentro del rango canónico admitido ($200..360$ px).
    - Anclaje bidireccional (`SidebarDockPosition::Left` y `SidebarDockPosition::Right`), validando el cálculo exacto de coordenadas rectangulares (`LayoutRect`) para el área de contenido y paneles contiguos.
  - Redimensionamiento reactivo continuo de ventana (desde resoluciones 4K $3840\times 2160$ px hasta límites mínimos), comprobando que el área central (`contentArea`) absorbe el delta dimensional de manera proporcional y que las barras funcionales mantienen sus alturas fijadas por la densidad activa.
- **Casos negativos, validación de fronteras y entradas malformadas**:
  - Detección y rechazo de dimensiones de ventana sub-mínimas:
    - $639\times 480$ o $640\times 479$ en densidad estándar.
    - $719\times 520$ o $720\times 519$ en «Interfaz más fácil de usar».
    - $499\times 380$ o $500\times 379$ en densidad compacta.
    - Dimensiones no positivas ($\le 0$), cero ($0\times 0$) o valores negativos patológicos, emitiendo el código de error tipado `MainWindowLayoutError::DimensionsBelowMinimum`.
  - Detección y rechazo de anchuras de barra lateral no válidas:
    - Anchuras inferiores a 48 px (e.g. 0 px, 10 px, 47 px).
    - Anchuras en la zona prohibida entre colapsada y mínima expandida (rango $49..199$ px, como 100 px o 150 px).
    - Anchuras superiores al umbral máximo permitido ($> 360$ px, e.g. 500 px).
    - Verificación del código de error `MainWindowLayoutError::InvalidSidebarWidth` y de la pista de remediación explicativa.
  - Detección de configuración de zonas inválidas:
    - Zonas con presupuestos de altura negativos o cero.
    - Márgenes o paddings negativos en `LayoutMargins`.
    - Asignaciones incompatibles de anclaje (`ConflictingZoneAnchors`).
    - Suma de alturas de zonas fijas que iguale o supere la altura total de la ventana, agotando el espacio de contenido y disparando `MainWindowLayoutError::ZeroContentAreaBudget`.
  - Detección de densidades no reconocidas (`InvalidDensityConfiguration`).
- **Concurrencia multihilo de alto estrés (16 hilos) y sincronización thread-safe**:
  - Ejecución paralela sobre `MainWindowLayoutService` desde 16 hilos (`std::thread`) concurrentes sin condiciones de carrera:
    - 4 hilos redimensionando continuamente la ventana (`resizeWindow`).
    - 4 hilos alternando densidades (`setDensity` entre Default, Senior y Compact).
    - 4 hilos mutando la barra lateral (`toggleSidebar`, `setSidebarWidth`, `setSidebarPosition`).
    - 4 hilos consultando continuamente el plan activo y reportes geométricos (`plan()`, `currentReport()`).
  - Certificación de ausencia absoluta de carreras de datos (data races), integridad del estado protegido por `std::mutex`, y terminación limpia de todos los hilos sin interbloqueos (deadlocks).
- **Cancelación cooperativa asíncrona multihilo y política Zero-Zombie**:
  - Pruebas de estrés de interrupción cooperativa mediante `CancellationToken` activado desde hilos secundarios en distintos puntos del ciclo de vida del servicio de layout.
  - Verificación de respuesta inmediata (< 10 ms), aborto seguro de operaciones no comprometidas, preservación de la consistencia del plan activo y ausencia total de procesos zombis o recursos colgados.
- **Recuperación ante fallos de disco, corrupción de datos y aislamiento Local-First**:
  - Simulación de fallos de persistencia en disco: rutas no escribibles, permisos denegados (`perms::read_only`) y fallos durante la escritura atómica.
  - Fuzzing e inyección de archivos de estado de layout gravemente corruptos en `loadLayoutStateFromFile`:
    - Archivos JSON truncados o malformados sintácticamente.
    - Archivos JSON con tipos discordantes (strings en lugar de enteros en dimensiones).
    - Archivos JSON con valores geométricos ilegales (dimensiones sub-mínimas o anchuras de sidebar corruptas deliberadamente inyectadas).
    - Archivos con bytes binarios aleatorios o contenido vacío.
  - Certificación de que ante cualquier anomalía, `MainWindowLayoutService` activa su mecanismo de recuperación resiliente: retorna `MainWindowLayoutError::LayoutPersistenceCorrupted`, restablece de manera transparente el plan canónico seguro (`canonicalDefaultLayout()`), y no genera excepciones no capturadas ni estados inconsistentes.
  - Verificación estricta de aislamiento de sesión privada: en modo incógnito (`isIncognitoMode() == true`), `saveLayoutStateToFile` y `loadLayoutStateFromFile` abortan inmediatamente sin interactuar con el sistema de archivos local, garantizando la privacidad absoluta (ADR-001 / Decisión Q-001).
- **Integración UI, accesibilidad y no regresión**:
  - Verificación en el arnés de pruebas de interfaz de usuario (`tests/ui/WindowTests.cpp`) de que la jerarquía Qt (`BrowserWindow`) responde fielmente a las mutaciones del servicio de layout, valida las dimensiones de tap target $\ge 48$ px en la «Interfaz más fácil de usar», y garantiza tolerancia headless reportando `NOT_EXECUTED` de manera limpia si `DISPLAY`/`WAYLAND_DISPLAY` está ausente.
  - Auditoría exhaustiva de lenguaje público (`docs/UX-COPY-PUBLICO.md`): todas las cadenas generadas por el validador, pistas de remediación y widgets emplean español neutro claro, sin términos internos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «toolchain»).

---

## Out of scope

- Modificación de la arquitectura y contratos de dominio consolidados en `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration.h` o `MainWindowLayoutValidator.h` durante PROPOSE.
- Modificación directa del código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante APPLY tras aprobación humana).
- Inicialización innecesaria del motor de renderizado de Chromium WebEngine en las pruebas unitarias y de servicio (se ejecutan en C++20 puro de forma rápida y determinista).
- Persistencia remota en la nube o telemetría (vetado taxativamente por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Generación de mocks de integración que simulen falsamente el éxito de componentes ausentes.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Dominio e implementación de servicio de layout consolidados (`apps/vigilio-browser/src/core/`)**:
   - `MainWindowLayoutTypes.h`: Define las enumeraciones canónicas `LayoutZone`, `LayoutDensity`, `SidebarState`, `SidebarDockPosition`, `WindowChromeMode`, `MainWindowLayoutError`, así como las estructuras inmutables `WindowDimensions`, `LayoutMargins`, `LayoutRect`, `ZoneBudget`, `SidebarConfig`, `MainWindowLayoutPlan`, `ZoneGeometry` y `MainWindowLayoutReport`.
   - `MainWindowLayoutConfiguration.h` y `.cpp`: Implementa las configuraciones canónicas oficiales (`canonicalDefaultLayout`, `canonicalSeniorLayout`, `canonicalCompactLayout`), la verificación de invariantes (`verifyInvariants`), y las constantes de dimensiones y alturas para cada zona y densidad.
   - `MainWindowLayoutValidator.h` y `.cpp`: Implementa las reglas de validación estática de dimensiones (`validateDimensions`), configuración de barra lateral (`validateSidebar`), plan completo (`validatePlan`), el cálculo analítico de distribución espacial sin solapamientos (`calculateLayoutAreas`), y las funciones de conversión a texto y pistas de remediación en español neutro (`layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `sidebarDockPositionToString`, `windowChromeModeToString`, `layoutErrorToString`, `generateRemediationHint`).
   - `MainWindowLayoutService.h` y `.cpp`: Capa operativa desacoplada que administra el plan activo, orquesta conmutaciones de densidad (`setDensity`), control de barra lateral (`toggleSidebar`, `setSidebarWidth`, `setSidebarPosition`, `setSidebarState`), visibilidad de zonas (`setZoneVisible`), redimensionamiento (`resizeWindow`), persistencia atómica con tolerancia a archivos corruptos (`saveLayoutStateToFile`, `loadLayoutStateFromFile`), aislamiento de incógnito (`setIncognitoMode`), sincronización mediante callbacks (`setLayoutChangedCallback`) y cancelación cooperativa Zero-Zombie (`CancellationToken`).
2. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/MainWindowLayoutTests.cpp`: 12 pruebas unitarias de dominio aprobadas (commit `4cb5d8f`).
   - `tests/unit/MainWindowLayoutServiceTests.cpp`: 14 pruebas unitarias de servicio aprobadas (commit `c4d75c0`).
   - `tests/ui/WindowTests.cpp`: Suite de pruebas Qt Test que verifica la inicialización de `BrowserWindow`, jerarquía de widgets de layout (`vigilio-sidebar`, `vigilio-main-container`, `vigilio-tabs`, `vigilio-navigation`), toggle de sidebar, tap targets $\ge 48$ px en modo accesible y auditoría de lenguaje público.
3. **Integración en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Los ejecutables `vigilio_main_window_layout_tests` y `vigilio_main_window_layout_service_tests` están plenamente integrados en CTest con etiquetas `LABELS "unit;fast"` y timeout de 15 segundos.
   - `vigilio_ui_tests` está integrado condicionalmente con Qt6 Test con etiquetas `LABELS "ui;fast"` y timeout de 20 segundos.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GCC bajo C++20 con `-Werror` y cero advertencias.
   - 27 de 27 pruebas pasando al 100% en CTest (~5.07 s en total, exit code 0).
5. **Gobernanza con Prompt Builder CLI**:
   - `vigilio-prompt verify-lock` sobre `0187-module.ux-shell.task0063.propose.lock.json`: `valid: true` (13 verificaciones aprobadas).
   - `vigilio-prompt validate-references`: `valid: true` (63 referencias canónicas certificadas).

---

## Alcance exacto

El incremento técnico para la etapa **tests** de `module.ux-shell.task0063` abarca:

1. **Ampliación de `tests/unit/MainWindowLayoutTests.cpp` (pruebas de dominio y validación de frontera)**:
   - Incorporación de casos de prueba negativos para todas las condiciones de fallo de `MainWindowLayoutValidator`:
     - Dimensiones sub-mínimas en los bordes exactos ($639\times 480$, $640\times 479$, $719\times 520$, $720\times 519$, $499\times 380$, $500\times 379$).
     - Dimensiones cero y negativas ($-1\times 500$, $800\times -20$, $0\times 0$).
     - Anchuras de barra lateral no válidas en fronteras críticas ($0$, $1$, $47$, $49$, $100$, $199$, $361$, $1000$ px).
     - Zonas con alturas mínimas o preferidas negativas o cero.
     - Márgenes y paddings negativos en `LayoutMargins`.
     - Sobreasignación presupuestaria que cause `ZeroContentAreaBudget`.
     - Presupuestos con banderas incoherentes.
   - Evaluación exhaustiva de distribución espacial analítica (`calculateLayoutAreas`) bajo geometrías extremas (pantallas ultrapanorámicas $3840\times 1080$, cuadradas $1024\times 1024$ y formatos verticales $1080\times 1920$).
   - Cobertura completa de serialización textual y pistas de remediación sin términos proscritos de ingeniería.
2. **Ampliación de `tests/unit/MainWindowLayoutServiceTests.cpp` (pruebas de servicio, estrés, resiliencia y recuperación)**:
   - Prueba de concurrencia masiva multihilo con 16 hilos compitiendo sobre `MainWindowLayoutService` (mutaciones concurrentes de tamaño, densidad, sidebar y consultas).
   - Pruebas de estrés de cancelación cooperativa multihilo mediante `CancellationToken`.
   - Pruebas de inyección de archivos corruptos y fuzzing en `loadLayoutStateFromFile`:
     - JSON truncado, JSON con sintaxis rota, JSON con tipos incorrectos y JSON con violaciones geométricas intencionales.
     - Verificación del restablecimiento transparente de `canonicalDefaultLayout()` y emisión de `MainWindowLayoutError::LayoutPersistenceCorrupted`.
   - Pruebas de simulación de fallos de disco en `saveLayoutStateToFile` (rutas no escribibles, directorios sin permisos) verificando el manejo ordenado sin excepciones no capturadas.
   - Pruebas de conmutación de anclaje de barra lateral (`SidebarDockPosition::Right` vs `Left`) y combinaciones de visibilidad de zonas.
   - Pruebas de aislamiento estricto de incógnito (`setIncognitoMode(true)`): verificación de que las operaciones de persistencia no tocan el sistema de archivos.
3. **Ampliación de `tests/ui/WindowTests.cpp` (pruebas de integración de UI y accesibilidad)**:
   - Validación de la integración reactiva entre `BrowserWindow` y `MainWindowLayoutService`:
     - Verificación de que `resizeEvent` actualiza el reporte en el servicio.
     - Verificación de que `setSeniorMode(true)` aplica dimensiones de control $\ge 48$ px y actualiza las dimensiones mínimas de ventana.
     - Verificación de la alternancia de barra lateral en UI (48 px vs 240 px).
     - Auditoría estricta de ausencia de términos técnicos prohibidos por `docs/UX-COPY-PUBLICO.md`.
4. **Verificación y calidad estricta**:
   - Formato conforme a Clang-Format sin discrepancias.
   - Análisis estático limpio con Clang-Tidy.
   - Compilación limpia con `-Werror` y ejecución 100% exitosa en CTest.

---

## Diseño completo de la capacidad

### 1. Arquitectura de pruebas y separación de responsabilidades

```
┌────────────────────────────────────────────────────────────────────────┐
│                   Arquitectura de Pruebas Unitarias                    │
├──────────────────────────────────┬─────────────────────────────────────┤
│   tests/unit/                    │   tests/unit/                       │
│   MainWindowLayoutTests.cpp      │   MainWindowLayoutServiceTests.cpp  │
│   (Dominio, Geometría y Límites) │   (Servicio, Resiliencia y Concur.) │
├──────────────────────────────────┼─────────────────────────────────────┤
│ • Invariantes canónicas          │ • Concurrencia masiva (16 hilos)   │
│ • Validación dimensional exacta  │ • Cancelación cooperativa multihilo │
│ • Fronteras de barra lateral     │ • Fuzzing y recuperación de JSON   │
│ • Presupuestos de zonas          │ • Fallos de disco y permisos        │
│ • No solapamiento de rectángulos │ • Aislamiento de modo incógnito     │
│ • Pistas de remediación en texto │ • Sincronización de callbacks       │
└──────────────────────────────────┴─────────────────────────────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│   tests/ui/WindowTests.cpp (Integración UI, Accesibilidad y Copy)      │
├────────────────────────────────────────────────────────────────────────┤
│ • Integración de BrowserWindow con MainWindowLayoutService             │
│ • Reacción a eventos nativos de redimensionamiento (resizeEvent)       │
│ • Controles táctiles >= 48 px en «Interfaz más fácil de usar»           │
│ • Tolerancia a entornos headless (DISPLAY ausente -> NOT_EXECUTED)      │
│ • Auditoría de lenguaje público según docs/UX-COPY-PUBLICO.md         │
└────────────────────────────────────────────────────────────────────────┘
```

### 2. Entidades, métodos y aserciones de prueba

1. **`MainWindowLayoutTests.cpp`**:
   - `testSubMinimumDimensionsExhaustive()`: Verifica fronteras exactas de fallo en dimensiones para densidades Default ($639\times 480$, $640\times 479$), Senior ($719\times 520$, $720\times 519$) y Compact ($499\times 380$, $500\times 379$).
   - `testZeroAndNegativeDimensions()`: Comprueba el rechazo sistemático de dimensiones $\le 0$, negativas o patológicas con `DimensionsBelowMinimum`.
   - `testInvalidSidebarWidthBoundaries()`: Evalúa todas las regiones prohibidas de anchura de barra lateral ($0..47$ px, $49..199$ px, $> 360$ px) asegurando el retorno de `InvalidSidebarWidth`.
   - `testZoneBudgetValidationFailures()`: Inyecta presupuestos de zona inválidos (alturas negativas, márgenes negativos, suma de alturas fijas $\ge$ altura de ventana) certificando `NegativeMarginOrPadding`, `ConflictingZoneAnchors` y `ZeroContentAreaBudget`.
   - `testExtremeWindowGeometries()`: Calcula distribuciones en resoluciones atípicas ($3840\times 1080$, $1080\times 1920$, $2560\times 1440$), comprobando que no existen intersecciones entre zonas ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y que el área de contenido es positiva.
   - `testPublicCopyAndRemediationTextAudit()`: Audita que `layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `sidebarDockPositionToString`, `layoutErrorToString` y `generateRemediationHint` no contienen términos de ingeniería interna.
2. **`MainWindowLayoutServiceTests.cpp`**:
   - `testMultiThreadedHighStressConcurrency16Threads()`: Lanza 16 hilos ejecutando operaciones concurrentes sobre una misma instancia de `MainWindowLayoutService` durante múltiples iteraciones, verificando la ausencia de data races y deadlocks.
   - `testCooperativeCancellationStress()`: Activa `CancellationToken` en momentos asíncronos y certifica la interrupción limpia y el estado inmutable del plan.
   - `testCorruptedLayoutPersistenceRecovery()`: Inyecta archivos JSON corruptos (truncados, tipos dispares, valores geométricos sub-mínimos) y verifica el retorno de `LayoutPersistenceCorrupted` y la restauración de `canonicalDefaultLayout()`.
   - `testDiskFailureHandling()`: Simula rutas en directorios sin permisos de escritura y comprueba que `saveLayoutStateToFile` maneja el error de forma segura sin abortar.
   - `testIncognitoModeStrictIsolation()`: Verifica que cuando el servicio está en modo incógnito, ni `saveLayoutStateToFile` ni `loadLayoutStateFromFile` acceden al sistema de archivos local.
   - `testSidebarDockPositionAndVisibilityTransitions()`: Evalúa la conmutación entre anclaje izquierdo y derecho con zonas opcionales activadas/desactivadas.
3. **`WindowTests.cpp`**:
   - `testBrowserWindowLayoutIntegration()`: Verifica la presencia de `layoutService()`, la reacción de `BrowserWindow` a `resizeEvent` y la adaptación visual de controles a $\ge 48$ px bajo la «Interfaz más fácil de usar».

---

## Archivos afectados

1. `apps/vigilio-browser/tests/unit/MainWindowLayoutTests.cpp`:
   - Incorporación de pruebas de frontera dimensional, anchuras inválidas de sidebar, presupuestos corruptos, geometrías extremas y auditoría de textos.
2. `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`:
   - Incorporación de pruebas de concurrencia a 16 hilos, cancelación cooperativa, fuzzing de persistencia, fallos de disco simulados y aislamiento de incógnito.
3. `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
   - Refinamiento de aserciones de integración de layout, eventos de tamaño y auditoría de textos públicos.
4. `apps/vigilio-browser/CMakeLists.txt`:
   - Mantenimiento y verificación de las propiedades de pruebas de CTest (etiquetas, timeouts).
5. `docs/rules/01-modules/ux-shell/micro/0063-main-window-layout/task.md`:
   - Actualización del estado de ejecución de la microtarea canónica 0063.

---

## Contrato UX/UI

- **Declaración sobre interfaz gráfica en etapa tests**:
  - La etapa `tests` formaliza la verificación automatizada y determinista de los contratos de `main window layout`. No introduce nuevos widgets visuales ni altera los controles existentes en `BrowserWindow`.
- **Preservación y verificación de contratos visuales**:
  - Se verifica que la ventana principal cumple rigurosamente con `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
  - Se comprueba que en la «Interfaz más fácil de usar» todos los controles interactivos tienen altura mínima $\ge 48$ px y que la ventana respeta un mínimo de $720\times 520$ px.
- **Cumplimiento de `docs/UX-COPY-PUBLICO.md`**:
  - Prohibición absoluta de términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter», «toolchain») en cualquier mensaje de error o pista de remediación expuesta al usuario. Se utiliza exclusivamente el término oficial **«Interfaz más fácil de usar»**.

---

## Reglas y fallos

1. **Entradas vacías, malformadas o patológicas**:
   - Dimensiones de ventana no positivas ($\le 0$) $\to$ `MainWindowLayoutError::DimensionsBelowMinimum`.
   - Dimensiones inferiores al mínimo canónico de la densidad activa $\to$ `MainWindowLayoutError::DimensionsBelowMinimum`.
   - Anchuras de barra lateral fuera de los rangos válidos (48 px fijo colapsado, o $200..360$ px expandido) $\to$ `MainWindowLayoutError::InvalidSidebarWidth`.
   - Presupuestos de zonas con alturas negativas o márgenes negativos $\to$ `NegativeMarginOrPadding`.
   - Presupuestos de zonas fijas que consumen la totalidad de la altura de la ventana $\to$ `ZeroContentAreaBudget`.
2. **Concurrencia y multihilo**:
   - Acceso simultáneo desde múltiples hilos a `MainWindowLayoutService` sincronizado mediante `std::mutex`. Sin condiciones de carrera ni bloqueos mutuos.
3. **Cancelación cooperativa**:
   - Comprobación recurrente de `CancellationToken::isCancelled()`. Aborto inmediato sin efectos secundarios ante señal de cancelación.
4. **Resiliencia ante fallos de persistencia**:
   - Archivos JSON de layout corruptos, truncados o no válidos $\to$ captura segura, reporte de `LayoutPersistenceCorrupted`, fallback automático al plan canónico por defecto y cero caídas de proceso.
   - Permisos denegados o disco no escribible $\to$ reporte de error limpio sin terminación abrupta.
5. **Aislamiento en navegación privada**:
   - En modo incógnito, no se lee ni escribe ningún archivo en disco.

---

## Seguridad y privacidad

- **Constitución Local-First (ADR-001 / Decisión Q-001)**:
  - Todas las pruebas operan 100% de manera local y determinista en memoria o sobre rutas temporales del sistema (`std::filesystem::temp_directory_path()`).
  - Cero llamadas a servicios de red externos, sockets o nube.
- **Aislamiento de sesiones privadas**:
  - Prohibición estricta de persistir configuraciones de layout cuando `isIncognitoMode() == true`.
- **Defensas de sistema de archivos**:
  - Toda persistencia utiliza escritura atómica con reemplazo seguro de archivos temporales, evitando corrupción de datos ante cortes imprevistos.
- **Política Zero-Zombie**:
  - Ninguna prueba genera procesos huérfanos o en espera indefinida. Todas las operaciones tienen límites de tiempo estrictos en CTest ($\le 15$ s).

---

## Matriz de pruebas

| Identificador | Nivel | Componente / API | Condición / Caso de prueba | Resultado esperado | Clasificación |
| --- | --- | --- | --- | --- | --- |
| `TEST-DOM-013` | Unitario | `MainWindowLayoutValidator::validateDimensions` | Dimensiones en frontera exacta de fallo ($639\times 480$, $719\times 520$, $499\times 380$) | Retorna `DimensionsBelowMinimum` | APPLY |
| `TEST-DOM-014` | Unitario | `MainWindowLayoutValidator::validateDimensions` | Dimensiones cero o negativas ($0\times 0$, $-10\times 500$, $800\times -5$) | Retorna `DimensionsBelowMinimum` | APPLY |
| `TEST-DOM-015` | Unitario | `MainWindowLayoutValidator::validateSidebar` | Anchura en rango prohibido ($0..47$, $49..199$, $> 360$ px) | Retorna `InvalidSidebarWidth` | APPLY |
| `TEST-DOM-016` | Unitario | `MainWindowLayoutValidator::validatePlan` | Presupuesto de zonas con alturas negativas o márgenes negativos | Retorna `NegativeMarginOrPadding` | APPLY |
| `TEST-DOM-017` | Unitario | `MainWindowLayoutValidator::calculateLayoutAreas` | Alturas fijas de zonas superan la altura de ventana | Retorna `ZeroContentAreaBudget` con `valid == false` | APPLY |
| `TEST-DOM-018` | Unitario | `MainWindowLayoutValidator::calculateLayoutAreas` | Geometrías de ventana extremas (ultrawide $3840\times 1080$, vertical $1080\times 1920$) | Distribución válida, sin solapamientos, $contentArea > 0$ | APPLY |
| `TEST-DOM-019` | Unitario | `MainWindowLayoutValidator` (textos públicos) | Auditoría de cadenas y pistas de remediación frente a UX Copy | Cero términos proscritos de ingeniería en español neutro | APPLY |
| `TEST-SRV-015` | Integración | `MainWindowLayoutService` (multihilo) | Estrés de concurrencia con 16 hilos simultáneos (resize, density, sidebar, read) | Cero carreras de datos, finalización limpia | APPLY |
| `TEST-SRV-016` | Integración | `MainWindowLayoutService` (cancelación) | Cancelación cooperativa multihilo mediante `CancellationToken` | Aborto inmediato, plan consistente | APPLY |
| `TEST-SRV-017` | Integración | `MainWindowLayoutService::loadLayoutStateFromFile` | Inyección de archivos JSON truncados o sintácticamente corruptos | Retorna `LayoutPersistenceCorrupted`, fallback a default | APPLY |
| `TEST-SRV-018` | Integración | `MainWindowLayoutService::loadLayoutStateFromFile` | Inyección de JSON con violaciones geométricas ilegales | Detección de invalidez, fallback a default seguro | APPLY |
| `TEST-SRV-019` | Integración | `MainWindowLayoutService::saveLayoutStateToFile` | Simulación de ruta de disco no escribible / permisos denegados | Manejo de error seguro sin excepciones no capturadas | APPLY |
| `TEST-SRV-020` | Integración | `MainWindowLayoutService` (incógnito) | Intentos de lectura y escritura con modo incógnito activado | Operación omitida inmediatamente, cero acceso a disco | APPLY |
| `TEST-SRV-021` | Integración | `MainWindowLayoutService` (anclaje derecho) | Configuración de barra lateral en `SidebarDockPosition::Right` | Geometrías calculadas correctamente con sidebar a la derecha | APPLY |
| `TEST-UI-007` | UI Qt | `BrowserWindow` y `MainWindowLayoutService` | Integración reactiva de eventos de redimensionamiento y modo Senior | Conmutación de tap targets $\ge 48$ px, mínimos respetados | APPLY |
| `TEST-E2E-001` | E2E | Navegación nativa completa con Chromium | Recorrido end-to-end con inicialización de páginas remotas | Ejecución serial pesada bajo switch opcional | NOT_APPLICABLE |

---

## Pasos APPLY

1. **Paso 1: Extender la suite de pruebas unitarias de dominio en `tests/unit/MainWindowLayoutTests.cpp`**:
   - Añadir pruebas exhaustivas para límites dimensionales (`testSubMinimumDimensionsExhaustive`, `testZeroAndNegativeDimensions`).
   - Añadir pruebas de frontera de anchura de barra lateral (`testInvalidSidebarWidthBoundaries`).
   - Añadir pruebas de presupuestos corruptos y sobreasignación de zonas (`testZoneBudgetValidationFailures`).
   - Añadir pruebas de cálculo espacial en geometrías extremas (`testExtremeWindowGeometries`).
   - Añadir auditoría de textos públicos y pistas de remediación (`testPublicCopyAndRemediationTextAudit`).
2. **Paso 2: Extender la suite de pruebas operativas de servicio en `tests/unit/MainWindowLayoutServiceTests.cpp`**:
   - Añadir prueba de concurrencia masiva con 16 hilos (`testMultiThreadedHighStressConcurrency16Threads`).
   - Añadir prueba de estrés de cancelación cooperativa (`testCooperativeCancellationStress`).
   - Añadir pruebas de recuperación ante archivos JSON corruptos y fuzzing (`testCorruptedLayoutPersistenceRecovery`).
   - Añadir pruebas de simulación de fallos de disco y permisos (`testDiskFailureHandling`).
   - Añadir pruebas de aislamiento de modo incógnito (`testIncognitoModeStrictIsolation`).
   - Añadir pruebas de anclaje derecho y visibilidad de zonas (`testSidebarDockPositionAndVisibilityTransitions`).
3. **Paso 3: Refinar las aserciones de integración en `tests/ui/WindowTests.cpp`**:
   - Verificar la integración de `MainWindowLayoutService` en `BrowserWindow` ante eventos nativos de redimensionamiento.
   - Validar que bajo «Interfaz más fácil de usar» los tap targets cumplen $\ge 48$ px y las dimensiones mínimas de ventana son de al menos $720\times 520$ px.
   - Auditar cadenas visibles y descripciones accesibles frente a `docs/UX-COPY-PUBLICO.md`.
4. **Paso 4: Validar formato canónico con Clang-Format**:
   - Ejecutar `clang-format --dry-run --Werror` sobre todos los archivos de prueba modificados.
5. **Paso 5: Validar análisis estático con Clang-Tidy**:
   - Ejecutar `clang-tidy` sobre las unidades de traducción modificadas.
6. **Paso 6: Compilación y ejecución de la suite completa en CTest**:
   - Compilar con CMake y ejecutar CTest certificando 100% de pruebas aprobadas con exit code 0.
7. **Paso 7: Sincronizar la documentación de microtarea en `docs/rules/`**:
   - Actualizar `docs/rules/01-modules/ux-shell/micro/0063-main-window-layout/task.md` para reflejar el estado completado tras APPLY.

---

## Definición de terminado

- [ ] Todas las nuevas pruebas unitarias y de integración de `main window layout` están implementadas y pasan al 100% en CTest.
- [ ] La prueba de concurrencia con 16 hilos finaliza sin condiciones de carrera, bloqueos ni violaciones de memoria.
- [ ] Los casos de recuperación ante archivos corruptos y fallos de persistencia demuestran la restauración transparente de `canonicalDefaultLayout()`.
- [ ] El aislamiento de modo incógnito certifica cero operaciones de lectura/escritura en disco.
- [ ] El arnés de UI valida la adaptación a la «Interfaz más fácil de usar» con tap targets $\ge 48$ px y mantiene tolerancia a entornos headless.
- [ ] Auditoría de `docs/UX-COPY-PUBLICO.md` certifica la ausencia de términos de ingeniería interna proscritos.
- [ ] Cero advertencias con `-Werror`, cero discrepancias con Clang-Format y Clang-Tidy.
- [ ] Validación de referencias de OpenSpec certifica `valid: true`.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.ux-shell.task0062` (completada y certificada formalmente en commit `d0e3fec`).
  - Dependencias estándar de C++20 y bibliotecas Qt6 ya presentes en el proyecto.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    *Mitigación*: `WindowTests.cpp` detecta de forma determinista la ausencia de display y reporta `NOT_EXECUTED` de manera limpia sin fallar en cascada, mientras que las pruebas unitarias y de servicio en C++20 puro se ejecutan de forma 100% autónoma.
  - *Riesgo*: Contención o bloqueos en pruebas multihilo de alta concurrencia (16 hilos).
    *Mitigación*: Uso estricto de `std::lock_guard` con RAII sobre `std::mutex`, evitando bloqueos anidados y garantizando tiempos de ejecución acotados (< 500 ms).

---

## Commit propuesto

`test(ux-shell): tests main-window-layout`
