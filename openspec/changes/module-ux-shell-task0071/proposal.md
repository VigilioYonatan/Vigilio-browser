# Change Proposal — module-ux-shell-task0071

Pack: `module.ux-shell.task0071`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **tests** de la capacidad **tab strip density** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0071 — Probar casos positivos, negativos y recuperación: tab strip density`).

Habiéndose completado y certificado formalmente en las etapas precedentes:
1. La formalización del modelo de dominio de densidad de la tira de pestañas, especificaciones analíticas de pestañas (`TabSpec`), parámetros dimensionales canónicos (`TabStripDensityConfiguration`), cálculo espacial elástico sin solapamientos y validador analítico (`TabStripDensityValidator`) en `module.ux-shell.task0068` (commit `e05f659`).
2. La implementación operativa desacoplada en C++20 puro en `TabStripDensityService` con gestión multihilo del estado (`std::mutex`), compresión elástica y desbordamiento horizontal, persistencia atómica local con tolerancia a fallos, aislamiento estricto de navegación privada/incógnito y cancelación cooperativa Zero-Zombie (`CancellationToken`) en `module.ux-shell.task0069` (commit `ec06f94`).
3. La integración de la interacción física Qt Widgets de la tira de pestañas (`vigilio-tabs`), botón de nueva pestaña (`vigilio-tab-new-button`), conmutación fluida entre densidades (`Default` a 36 px, `Senior` / «Interfaz más fácil de usar» a 48 px y `Compact` a 28 px), estados interactivos (normal, hover, selected, pressed, loading, error, reposo/discarded, fijada/pinned, audio/silenciado), navegación por teclado y tap targets accesibles $\ge 48\times 48$ px en `module.ux-shell.task0070`.

El objetivo primordial de este pack es **diseñar y estructurar la suite exhaustiva, determinista y de alta resiliencia de pruebas automatizadas para tab strip density**.

La suite de pruebas proyectada cubre sistemáticamente:
- **Casos positivos y ciclo de vida integral de la tira de pestañas**:
  - Verificación exhaustiva de planes canónicos (`canonicalDefaultDensity`, `canonicalSeniorDensity`, `canonicalCompactDensity`), evaluando precondiciones, postcondiciones, restricciones dimensionales mínimas de altura ($36$ px en estándar, $48$ px en accesible, $28$ px en compacto), anchura mínima de tira ($\ge 400$ px en estándar, $\ge 500$ px en accesible, $\ge 360$ px en compacto), y la distribución espacial sin solapamientos entre pestañas fijadas, pestañas regulares y el botón de nueva pestaña.
  - Transiciones fluidas entre densidades de interfaz (`LayoutDensity::Default` $\leftrightarrow$ `LayoutDensity::Senior` $\leftrightarrow$ `LayoutDensity::Compact`), certificando la correcta recalculación de geometrías de pestañas, notificación síncrona a suscriptores vía callback y preservación de tap targets accesibles ($\ge 48\times 48$ px en modo Senior para botón de nueva pestaña y botones de cierre interactivos).
  - Algoritmo de compresión elástica y redistribución continua de anchos de pestaña ante redimensionamiento de ventana (desde $400$ px hasta resoluciones ultrapanorámicas y 4K $3840$ px), comprobando que el ancho efectivo de pestaña se contrae suavemente entre el ancho preferido ($200$ px estándar / $220$ px accesible) y el ancho mínimo ($120$ px estándar / $140$ px accesible) antes de activar el desbordamiento.
  - Activación determinista de modalidades de desbordamiento horizontal (`TabStripOverflowMode::Scrollable` y `TabStripOverflowMode::DropdownMenu`) cuando el número de pestañas excede la capacidad elástica en anchos confinados.
  - Estados avanzados de pestañas: agrupación y precedencia de pestañas fijadas (`TabPinState::Pinned`) al inicio de la tira, indicadores multimedia de reproducción y silenciamiento sonoro (`TabMediaState::AudioPlaying` y `TabMediaState::AudioMuted`), estados de ahorro de memoria («Pestaña en reposo» / `TabDiscardState::Discarded`), y transiciones de carga y conectividad (`TabLoadingState::Loading` y `TabLoadingState::Failed`).
- **Casos negativos, validación de fronteras y entradas malformadas**:
  - Detección y rechazo de dimensiones de tira sub-mínimas:
    - Anchura disponible inferior al mínimo canónico (e.g. $399$ px, $200$ px, $0$ px y valores negativos patológicos), emitiendo el código de error tipado `TabStripDensityError::WidthBelowMinimum`.
    - Altura de tira inferior al mínimo canónico ($\le 0$ px), emitiendo `TabStripDensityError::HeightBelowMinimum`.
  - Detección y rechazo de violaciones de altura mínima en la «Interfaz más fácil de usar» (Modo Senior):
    - Altura de tira inferior a 48 px (e.g. $36$ px, $40$ px, $47$ px) en densidad `Senior`, emitiendo `TabStripDensityError::SeniorModeMinimumHeightViolation` y la correspondiente pista de remediación en español neutro.
  - Detección y rechazo de violaciones de tap target accesible en Modo Senior:
    - Botón de nueva pestaña o botones de cierre con dimensiones inferiores a $48\times 48$ px o $24\times 24$ px respectivamente, emitiendo `TabStripDensityError::SeniorModeTapTargetViolation`.
  - Detección de agotamiento de ancho disponible (`TabStripDensityError::AvailableWidthExhausted`) ante cantidades excesivas de pestañas fijadas que saturan el contenedor antes de ubicar pestañas regulares.
  - Validaciones de colecciones vacías, índices fuera de rango en reordenación e inserción, e identificadores de pestaña inexistentes.
- **Hit-testing espacial cartesiano exhaustivo y disyunción geométrica**:
  - Evaluación analítica en `TabStripDensityValidator::hitTest` de coordenadas espaciales:
    - Zonas internas de pestaña: cuerpo (`TabBody`), favicon (`TabFavicon`), título (`TabTitle`), botón de cierre (`TabCloseButton`) e indicador multimedia/altavoz (`TabAudioIndicator`).
    - Controles de la tira: botón de nueva pestaña (`NewTabButton`), botones de desplazamiento horizontal (`ScrollLeftButton`, `ScrollRightButton`) y menú desplegable de desbordamiento (`OverflowMenuButton`).
    - Espacio vacío de la tira (`EmptySpace`).
    - Coordenadas exteriores a la tira de pestañas (coordenadas negativas, más allá del ancho o alto de la tira), retornando `TabStripHitTestZone::Nowhere`.
  - Verificación matemática estricta de no solapamiento geométrico:
    - $\forall i \ne j, \text{tab}_i.\text{rect} \cap \text{tab}_j.\text{rect} = \emptyset$.
    - $\forall i, \text{tab}_i.\text{rect} \cap \text{newTabButtonRect} = \emptyset$.
    - $\text{faviconRect} \cap \text{closeButtonRect} = \emptyset$ dentro de cada pestaña.
- **Concurrencia multihilo de alto estrés (16 hilos) y sincronización thread-safe**:
  - Ejecución paralela sobre `TabStripDensityService` desde 16 hilos (`std::thread`) concurrentes sin condiciones de carrera (ThreadSanitizer-clean):
    - 4 hilos mutando pestañas continuamente (`addTab`, `removeTab`, `selectTab`, `moveTab`).
    - 4 hilos redimensionando continuamente la anchura disponible (`setAvailableWidth` oscilando entre $400$ px y $3840$ px).
    - 4 hilos alternando densidades de layout (`setDensity` entre Default, Senior y Compact).
    - 4 hilos ejecutando consultas concurrentes de plan, hit-testing espacial (`hitTest`, `tabAtPoint`), alternancia de audio (`toggleTabAudioMute`) y ahorro de memoria (`setTabDiscardState`).
  - Certificación de ausencia absoluta de carreras de datos, integridad del estado protegido por `std::mutex`, y terminación limpia de todos los hilos sin interbloqueos (deadlocks).
- **Cancelación cooperativa asíncrona multihilo y política Zero-Zombie**:
  - Pruebas de estrés de interrupción cooperativa mediante `CancellationToken` activado desde hilos secundarios en distintos puntos del cálculo analítico de layout y persistencia.
  - Verificación de respuesta inmediata (< 10 ms), aborto seguro de operaciones no comprometidas, preservación de la consistencia del plan activo y ausencia total de procesos zombis o recursos colgados.
- **Recuperación ante fallos de disco, corrupción de persistencia y aislamiento Local-First**:
  - Inyección de archivos JSON corruptos, truncados a la mitad, con tipos incongruentes o campos numéricos negativos en `loadStateFromFile`.
  - Verificación del restablecimiento transparente del plan canónico por defecto (`canonicalDefaultDensity()`) y retorno limpio de `false` sin terminación abrupta ni fallos de segmentación.
  - Simulación de fallos de E/S en disco marcando el destino como de solo lectura (`perms::read_only`), certificando que `saveStateToFile` gestiona el error de forma controlada retornando `false`.
  - Certificación estricta de aislamiento de sesión privada (modo incógnito): cuando `isIncognitoMode() == true`, `saveStateToFile` y `loadStateFromFile` abortan de inmediato con 0 operaciones de E/S sobre el almacenamiento secundario, en cumplimiento de ADR-001 / Decisión Q-001.
- **Auditoría de textos públicos y accesibilidad frente a `docs/UX-COPY-PUBLICO.md`**:
  - Verificación programática de todas las etiquetas accesibles, tooltips, estados y pistas de remediación generadas por `TabStripDensityValidator`.
  - Proscripción estricta de términos internos de ingeniería («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «Linter», «toolchain»).
  - Cumplimiento de términos aprobados: «Interfaz más fácil de usar», «Nueva pestaña», «Cerrar pestaña», «Silenciar pestaña», «Activar sonido de pestaña», «Pestaña en reposo», «Cargando…», «No se pudo cargar».

---

## Out of scope

1. **Implementación de código de producción o refactorización de arquitectura**:
   - Este pack no implementa nuevos componentes de interfaz ni altera la arquitectura interna de `TabStripDensityService` o `BrowserWindow`. Su alcance se circunscribe a la suite de pruebas automatizadas y contratos de verificación.
2. **Dependencia de procesos WebEngine o red externa**:
   - Las pruebas no inician procesos Chromium/WebEngine ni realizan llamadas a servidores de red o nube. Toda prueba se ejecuta sobre fixtures locales y estructuras analíticas desacopladas.
3. **Persistencia en servicios cloud o telemetría**:
   - Prohibición total de sincronización remota o cuentas de usuario externas. Toda persistencia se valida sobre archivos JSON temporales locales con borrado determinista.
4. **Modificación de archivos fuera de `allowed_paths`**:
   - No se modifican módulos ajenos a `ux-shell` ni rutas restringidas (`apps/**`, `generated/**` durante PROPOSE).

---

## Contexto y evidencia

### Exploración comprobable del código existente

1. **Archivos de dominio y validación existentes en `apps/vigilio-browser/src/core/`**:
   - `TabStripDensityTypes.h`: Define `LayoutDensity`, `TabPinState`, `TabMediaState`, `TabDiscardState`, `TabLoadingState`, `TabInteractiveState`, `TabStripHitTestZone`, `TabStripOverflowMode`, `TabStripDensityError`, `TabStripDimensions`, `TabSpec`, `TabItemLayout`, `TabStripDensityConfig`, `TabStripDensityPlan` y `TabStripDensityReport`.
   - `TabStripDensityConfiguration.h/.cpp`: Declara y define alturas canónicas ($36$ px Default, $48$ px Senior, $28$ px Compact), anchos mínimos ($400$ px Default, $500$ px Senior, $360$ px Compact), anchos de pestaña regular ($200$ px preferido / $120$ px mín / $240$ px máx en Default; $220$ px preferido / $140$ px mín / $280$ px máx en Senior), anchos de pestañas fijadas ($36$ px Default, $48$ px Senior, $28$ px Compact), dimensiones de botón de nueva pestaña ($28\times 28$ Default, $48\times 48$ Senior, $24\times 24$ Compact), umbrales de compresión de botón de cierre ($90$ px Default, $140$ px Senior, $90$ px Compact), y la función de verificación analítica `verifyInvariants(const TabStripDensityPlan&)`.
   - `TabStripDensityValidator.h/.cpp`: Implementa `validateDimensions`, `validatePlan`, `calculateTabStripLayout`, `hitTest`, conversión de errores y denominaciones a español neutro (`tabStripErrorToString`, `densityToString`, `hitTestZoneToString`), generación de etiquetas accesibles (`tabAccessibleLabel`, `tabTooltip`, `closeButtonAccessibleLabel`, `audioButtonAccessibleLabel`, `newTabButtonAccessibleLabel`), pistas de remediación (`generateRemediationHint`) y sanitización estricta de títulos (`sanitizeTitle`).
2. **Servicio operativo desacoplado en `apps/vigilio-browser/src/core/`**:
   - `TabStripDensityService.h/.cpp`: Implementa la orquestación multihilo de la tira de pestañas con sincronización `std::mutex`, mutaciones de pestañas (`addTab`, `insertTab`, `removeTab`, `moveTab`, `selectTab`, `setTabPinned`, `setTabMediaState`, `toggleTabAudioMute`, `setTabDiscardState`, `updateTabTitle`), redimensionamiento reactivo (`setAvailableWidth`), conmutación de densidades (`setDensity`), hit-testing (`hitTest`, `tabAtPoint`), recálculo analítico con cancelación cooperativa (`computeLayout`), persistencia atómica local (`saveStateToFile`, `loadStateFromFile`), aislamiento de sesión privada (`setIncognitoMode`) y observabilidad síncrona vía callback (`setTabStripChangedCallback`).
3. **Integración UI en `apps/vigilio-browser/src/ui/`**:
   - `BrowserWindow.h/.cpp`: Integra el contenedor `QTabWidget#vigilio-tabs`, botón de nueva pestaña `QPushButton#vigilio-tab-new-button`, inicialización de `TabStripDensityService`, sincronización de eventos de redimensionamiento nativo en `resizeEvent`, conmutación bidireccional con la «Interfaz más fácil de usar» en `setSeniorMode(bool)` y estilos visuales QSS sobrios sin brillo artificial.
4. **Arnés de pruebas actual y brechas identificadas**:
   - `TabStripDensityTests.cpp` cuenta con 16 pruebas unitarias preliminares de invariantes y hit-testing básico. Requiere ampliación sistemática para explorar límites exactos de error dimensional ($399$ px, $0$ px, negativos), violaciones de altura en modo Senior ($47$ px, $36$ px), casos extremos de solapamiento y overflow con alta densidad de pestañas (e.g. 100 pestañas en $400$ px), saturación por pestañas fijadas, hit-testing exhaustivo de los 4 vértices y adyacencias $\pm 1$ px, sanitización de entradas hostiles complejas y auditoría exhaustiva frente a `docs/UX-COPY-PUBLICO.md`.
   - `TabStripDensityServiceTests.cpp` cuenta con 18 pruebas unitarias de ciclo de vida básico. Requiere ampliación con prueba de estrés de concurrencia masiva (16 hilos compitiendo con $\ge 200$ iteraciones por hilo), cancelación cooperativa en instantes asíncronos arbitrarios, fuzzing exhaustivo de archivos de persistencia corruptos, simulación de fallos de E/S por permisos de solo lectura, certificación estricta de cero escrituras en incógnito y mutaciones masivas de pestañas en ráfaga.
   - `WindowTests.cpp` cuenta con validaciones de ventana principal y barra de título. Requiere consolidar aserciones para `tabStripDensityService()`, tap targets $\ge 48\times 48$ px del botón de nueva pestaña y botones de cierre, sincronización con `resizeEvent`, foco por teclado y auditoría de textos accesibles en widgets de pestañas.

---

## Alcance exacto

El alcance exacto de esta propuesta en la etapa **tests** comprende:

1. **Ampliación de `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`**:
   - `testSubMinimumDimensionsExhaustive`: Fronteras exactas de fallo dimensional ($399$ px, $200$ px, $0$ px, valores negativos patológicos).
   - `testSeniorModeHeightViolations`: Violaciones de altura $< 48$ px en densidad Senior ($47$ px, $40$ px, $36$ px) y verificación de `TabStripDensityError::SeniorModeMinimumHeightViolation`.
   - `testExtremeOverflowAndElasticCompression`: Comportamiento elástico bajo estrés con 100 pestañas en contenedor estrecho ($400$ px), certificando la activación de `TabStripOverflowMode::Scrollable` y la invariante de no solapamiento sin colapso de geometrías.
   - `testPinnedTabsSaturationAndSpaceExhaustion`: Escenarios donde múltiples pestañas fijadas saturan el ancho disponible, evaluando la detección analítica y la preservación del botón de nueva pestaña.
   - `testExhaustiveHitTestingBoundaries`: Evaluación cartesiana de las 4 esquinas de cada pestaña, icono, título, botón de cierre e indicador de audio, así como puntos a $\pm 1$ px fuera de los límites y retorno de `TabStripHitTestZone::Nowhere`.
   - `testHostileTitleSanitizationAndPrivacy`: Inyección de títulos con URLs completas con credenciales, parámetros sensibles de autenticación, caracteres de control (`\r`, `\n`, `\t`, `\0`), y cadenas extremadamente largas (10,000 caracteres) con truncado seguro.
   - `testPublicCopyExhaustiveAudit`: Auditoría programática exhaustiva de todas las cadenas de texto, mensajes de error, pistas de remediación, tooltips y etiquetas accesibles generadas por `TabStripDensityValidator` frente a `docs/UX-COPY-PUBLICO.md`.
2. **Ampliación de `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`**:
   - `testMultiThreadedHighStressConcurrency16Threads`: Estrés multihilo con 16 hilos concurrentes compitiendo sobre `TabStripDensityService` (mutación de pestañas, redimensionamiento continuo, conmutación de densidad y consultas simultáneas de plan y hit-test) con $\ge 200$ iteraciones por hilo, validando ausencia total de carreras de datos e invariantes válidas.
   - `testCooperativeCancellationStress`: Cancelación asíncrona concurrente mediante `CancellationToken` en múltiples hilos durante el recálculo analítico, verificando respuesta < 10 ms, preservación del plan previo y política Zero-Zombie.
   - `testCorruptedPersistenceFuzzingAndRecovery`: Fuzzing de archivos JSON truncados, campos ausentes, tipos dispares (strings en identificadores o dimensiones) y valores patológicos en `loadStateFromFile`, certificando restablecimiento automático a `canonicalDefaultDensity()`.
   - `testDiskIoFailureHandling`: Simulación de fallos de disco configurando permisos de solo lectura (`perms::read_only`), comprobando que `saveStateToFile` retorna `false` limpiamente sin abortos ni caídas.
   - `testIncognitoStrictPrivacyIsolation`: Certificación formal de que cuando `isIncognitoMode() == true`, no se realiza ninguna operación de lectura ni escritura en disco bajo ninguna circunstancia.
   - `testContinuousTabMutationsBurst`: Adición y eliminación masiva de pestañas en ráfaga (50 operaciones sucesivas), validando la estabilidad de la selección adyacente y la coherencia del estado.
3. **Refinamiento de `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Integración de aserciones de presencia e inicialización de `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()`.
   - Verificación de tap targets accesibles $\ge 48\times 48$ px en el botón de nueva pestaña y botones de cierre bajo `setSeniorMode(true)`.
   - Sincronización de eventos de redimensionamiento nativo (`QResizeEvent`) con `tabStripDensityService_->setAvailableWidth()`.
   - Navegación por teclado (`focusPolicy != Qt::NoFocus`) y auditoría de textos accesibles sin términos proscritos.
   - Tolerancia determinista en entornos headless (`NOT_EXECUTED` si falta display gráfico sin fallo falso).
4. **Verificación de calidad, formato y sincronización documental**:
   - Verificación de formato canónico con Clang-Format.
   - Verificación de análisis estático con Clang-Tidy.
   - Ejecución y paso del 100% de la suite de pruebas en CTest.
   - Sincronización del documento de microtarea `docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md`.

---

## Diseño completo de la capacidad

### Arquitectura de pruebas y capas de verificación

La suite de pruebas automatizadas para **tab strip density** se estructura en tres capas ortogonales, alineadas con la pirámide de testing senior de `docs/skills/02-testing-senior.md`:

```
+-------------------------------------------------------------------------+
|                        Capa 3: Integración UI                           |
|       (apps/vigilio-browser/tests/ui/WindowTests.cpp)                   |
|  - BrowserWindow + TabStripDensityService                               |
|  - Sincronización QResizeEvent, setSeniorMode, tap targets >= 48 px     |
|  - Foco por teclado, tolerancia headless (NOT_EXECUTED)                 |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                  Capa 2: Pruebas Operativas de Servicio                 |
|       (apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp) |
|  - Estrés multihilo con 16 hilos concurrentes compitiendo               |
|  - Cancelación cooperativa asíncrona (CancellationToken) Zero-Zombie    |
|  - Fuzzing de persistencia, fallos de E/S de disco, recuperación        |
|  - Aislamiento estricto de sesión privada (incógnito)                   |
|  - Ciclo de vida y mutaciones de pestañas en ráfaga                     |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                Capa 1: Pruebas de Dominio y Validación                  |
|       (apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp)        |
|  - Fronteras dimensionales exactas (399 px, 0 px, valores negativos)   |
|  - Violaciones de altura en Modo Senior (< 48 px) y errores tipados     |
|  - Compresión elástica extrema (100 pestañas en 400 px)                 |
|  - Hit-testing espacial cartesiano exhaustivo (4 vértices y límites)    |
|  - Sanitización hostil de títulos y protección de privacidad            |
|  - Auditoría exhaustiva de textos públicos (docs/UX-COPY-PUBLICO.md)    |
+-------------------------------------------------------------------------+
```

### Invariantes geométricas y operativas auditadas

1. **Invariante de no solapamiento espacial**:
   - Para cualquier par de pestañas en el plan:
     $$\forall i \ne j \implies \text{tabs}[i].\text{rect} \cap \text{tabs}[j].\text{rect} = \emptyset$$
   - Para cualquier pestaña y el botón de nueva pestaña:
     $$\forall i \implies \text{tabs}[i].\text{rect} \cap \text{newTabButtonRect} = \emptyset$$
   - Dentro de cada pestaña:
     $$\text{faviconRect} \cap \text{closeButtonRect} = \emptyset$$
2. **Invariante de contención dimensional**:
   - Todas las pestañas y controles deben estar contenidos estrictamente dentro de los límites del contenedor:
     $$\text{tabsBoundsRect}.x \ge \text{paddingLeft}, \quad \text{tabsBoundsRect}.x + \text{tabsBoundsRect}.width \le \text{availableWidth} - \text{paddingRight}$$
   - La altura de cada pestaña debe ser idéntica a la altura configurada para la densidad activa:
     $$\forall i \implies \text{tabs}[i].\text{rect}.height = \text{height}$$
3. **Invariante de accesibilidad en la «Interfaz más fácil de usar» (Modo Senior)**:
   - La altura de la tira debe ser $\ge 48$ px:
     $$\text{density} = \text{Senior} \implies \text{height} \ge 48$$
   - El botón de nueva pestaña debe garantizar un tap target $\ge 48\times 48$ px:
     $$\text{newTabButtonRect}.width \ge 48 \land \text{newTabButtonRect}.height \ge 48$$
   - Los botones de cierre deben garantizar un área táctil expandida $\ge 24\times 24$ px.
4. **Invariante de precedencia de pestañas fijadas**:
   - Todas las pestañas con `pinState == TabPinState::Pinned` deben preceder estrictamente a cualquier pestaña con `pinState == TabPinState::Unpinned`:
     $$\forall i < j \implies \neg(\text{tabs}[i].\text{isPinned} == \text{false} \land \text{tabs}[j].\text{isPinned} == \text{true})$$
5. **Invariante de compresión elástica con umbral de botón de cierre**:
   - Cuando el ancho efectivo de pestaña se contrae por debajo del umbral crítico ($90$ px estándar / $140$ px accesible):
     $$\text{tabWidth} < \text{closeButtonThreshold} \implies (\text{isSelected} \lor \neg\text{showCloseButton})$$
     (El botón de cierre se oculta en pestañas inactivas para priorizar la legibilidad del título, pero se mantiene visible en la pestaña seleccionada).
6. **Invariante de aislamiento en navegación privada (Incógnito)**:
   - Cuando `isIncognitoMode() == true`, cualquier intento de persistencia a disco debe ser abortado inmediatamente:
     $$\text{isIncognitoMode}() \implies \text{I/O operations on disk} = 0$$

---

## Archivos afectados

1. `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`:
   - Incorporación de pruebas de fronteras exactas, violaciones de altura en modo accesible, compresión elástica extrema con 100 pestañas, saturación de pestañas fijadas, hit-testing espacial cartesiano exhaustivo (4 esquinas y adyacencias $\pm 1$ px), sanitización hostil de títulos y auditoría programática de textos públicos frente a `docs/UX-COPY-PUBLICO.md`.
2. `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`:
   - Incorporación de pruebas de estrés de concurrencia masiva con 16 hilos concurrentes compitiendo sobre una instancia compartida, cancelación cooperativa asíncrona mediante `CancellationToken`, fuzzing de archivos JSON corruptos y recuperación transparente, simulación de fallos de E/S de disco por permisos de solo lectura, certificación estricta de aislamiento en incógnito y mutaciones de pestañas en ráfaga.
3. `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
   - Refinamiento de aserciones de integración de la tira de pestañas con `BrowserWindow`, verificación de tap targets $\ge 48\times 48$ px en modo Senior, reacción ante `resizeEvent`, navegación por teclado y auditoría de textos accesibles en widgets de pestañas.
4. `docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md`:
   - Sincronización del estado de la microtarea y documentación de la certificación formal de la etapa `tests`.

---

## Contrato UX/UI

### Naturaleza del cambio en la etapa `tests`

Esta etapa es fundamentalmente de **pruebas automatizadas y certificación analítica**, por lo que no introduce componentes visuales nuevos en tiempo de ejecución. Sin embargo, su objetivo explícito es **verificar y certificar contractualmente que la interfaz visual de la tira de pestañas cumple con los estándares más estrictos de accesibilidad, estabilidad y diseño sobrio**:

1. **Consistencia visual con `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`**:
   - Certificación de alturas canónicas de tira: $36$ px en modo estándar y $48$ px en la «Interfaz más fácil de usar».
   - Fondos sobrios oscuros `#141D2B` y `#182334`, superficies `#1C2839` y pestaña activa `#33435A`, sin brillos artificiales ni estética de neón.
   - Pestañas fijadas de anchura compacta ($36$ px estándar / $48$ px accesible) que exhiben únicamente el favicon centrado.
2. **Modo Senior («Interfaz más fácil de usar»)**:
   - Verificación de tap targets amplios: botón de nueva pestaña $\ge 48\times 48$ px, botones de cierre $\ge 24\times 24$ px con padding interactivo extendido.
   - Altura de tira expandida a 48 px.
   - Separación física entre pestañas incrementada a 4 px (`kSeniorTabSpacing`).
   - Botón de cierre visible y accesible permanentemente en la pestaña seleccionada.
3. **Navegación accesible y soporte de teclado**:
   - Verificación de que las pestañas y botones aceptan foco de teclado (`Qt::TabFocus` / `focusPolicy != Qt::NoFocus`).
   - Contorno de foco visible de alto contraste (borde azul nítido de 2 px `#3B82F6`).
4. **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
   - Proscripción absoluta de jerga de ingeniería en todas las cadenas visibles, tooltips, etiquetas accesibles y descripciones para lectores de pantalla.
   - Términos aprobados y certificados por pruebas:
     - «Interfaz más fácil de usar» (en sustitución de «Senior Mode» / «Modo Senior»).
     - «Nueva pestaña» (en sustitución de etiquetas técnicas).
     - «Cerrar pestaña».
     - «Silenciar pestaña» / «Activar sonido de pestaña».
     - «Pestaña en reposo» / «Liberar memoria de pestañas inactivas» (en sustitución de «Tab discard»).
     - «Cargando…» / «No se pudo cargar».

---

## Reglas y fallos

### Matriz de fallos, códigos de error y remediación

| Condición de error | Código de error tipado | Comportamiento del sistema | Mensaje / Pista de remediación |
| :--- | :--- | :--- | :--- |
| Ancho disponible sub-mínimo ($< 400$ px en Default, $< 500$ px en Senior) | `TabStripDensityError::WidthBelowMinimum` | Se rechaza el plan y se emite reporte inválido; el servicio mantiene el último plan válido o recurre a la factoría canónica. | «El ancho de la tira de pestañas es inferior al mínimo requerido.» Sugerencia: ampliar el ancho de la ventana. |
| Altura de tira sub-mínima ($\le 0$ px) | `TabStripDensityError::HeightBelowMinimum` | Se rechaza el cálculo y se emite reporte inválido. | «La altura de la tira de pestañas es inferior al mínimo requerido.» |
| Altura $< 48$ px en modo Senior («Interfaz más fácil de usar») | `TabStripDensityError::SeniorModeMinimumHeightViolation` | Se rechaza la configuración y se emite error tipado; la pista de remediación indica elevar la altura a al menos 48 px sin usar términos proscritos. | «La altura de la tira de pestañas en la interfaz más fácil de usar debe ser de al menos 48 px.» |
| Dimensiones de tap target $< 48\times 48$ px en botón de nueva pestaña en modo Senior | `TabStripDensityError::SeniorModeTapTargetViolation` | Validación estructural detecta dimensiones insuficientes para interacción accesible. | «El tamaño del botón en la interfaz más fácil de usar debe ser de al menos 48x48 px.» |
| Saturación extrema de pestañas fijadas | `TabStripDensityError::AvailableWidthExhausted` | El ancho consumido por pestañas fijadas excede el espacio disponible; se preserva espacio mínimo para nueva pestaña. | «No hay espacio suficiente para ubicar las pestañas.» |
| Archivo de persistencia corrupto, truncado o con sintaxis JSON inválida | N/A (E/S) | `loadStateFromFile` detecta el fallo de deserialización, aborta la carga, restablece la configuración canónica por defecto (`canonicalDefaultDensity()`) y retorna `false` limpiamente. | Recuperación transparente sin caídas de proceso ni excepciones no controladas. |
| Permisos denegados en guardado de persistencia (`perms::read_only`) | N/A (E/S) | `saveStateToFile` captura el error de E/S del sistema de archivos, preserva el estado en memoria y retorna `false` sin corromper datos. | Fallo silencioso y seguro sin interrupción de la experiencia del usuario. |
| Modo incógnito activo durante intento de persistencia | N/A (Privacidad) | `saveStateToFile` y `loadStateFromFile` verifican `isIncognitoMode()`, abortan inmediatamente y retornan `false` con 0 operaciones de disco. | Aislamiento absoluto de navegación privada garantizado. |
| Cancelación cooperativa asíncrona disparada | N/A (Concurrencia) | `computeLayout` y operaciones de persistencia verifican `cancel->is_requested()`, detienen la operación antes de comprometer estado y retornan reporte de cancelación. | Respuesta < 10 ms, preservación del estado previo, cero hilos/procesos colgados (Zero-Zombie). |

---

## Seguridad y privacidad

1. **Aislamiento absoluto Local-First y navegación privada (Incógnito)**:
   - Conforme a ADR-001 y Decisión Q-001, en sesiones privadas de incógnito (`isIncognitoMode() == true`):
     - No se escriben historiales, títulos, URLs ni estados de pestañas en disco.
     - `saveStateToFile` retorna de inmediato sin abrir ningún archivo ni llamar a `std::filesystem`.
     - `loadStateFromFile` retorna de inmediato sin leer archivos del sistema.
     - El destructor de `TabStripDensityService` no deja artefactos temporales en disco.
2. **Sanitización estricta de títulos y URLs para evitar fugas de privacidad**:
   - `TabStripDensityValidator::sanitizeTitle` elimina sistemáticamente credenciales embebidas (`user:password@`), parámetros de autenticación (`token=`, `key=`, `secret=`), fragmentos (`#hash`) y secuencias de control (`\r`, `\n`, `\t`, bytes nulos `\0`).
   - Cadenas vacías o malformadas recurren de forma determinista al nombre seguro «Vigilio Browser».
   - Truncado defensivo con elipsis («…») para títulos extremadamente extensos, previniendo ataques de denegación de servicio por agotamiento de memoria o renderizado excesivo.
3. **Seguridad de hilos y prevención de vulnerabilidades de memoria**:
   - Acceso concurrente protegido exhaustivamente por `std::mutex`.
   - Cero carreras de datos (ThreadSanitizer-clean) bajo pruebas de estrés con 16 hilos concurrentes compitiendo en ráfaga.
   - Ausencia total de fugas de memoria o punteros colgantes (AddressSanitizer-clean).
   - Ausencia de procesos zombis mediante cancelación cooperativa determinista (`CancellationToken`).
4. **Prohibición de telemetría remota y servicios cloud**:
   - No se establecen conexiones de red, telemetría o peticiones HTTP/HTTPS externas.
   - No se integran cuentas Vigilio ni sincronización en la nube.

---

## Matriz de pruebas

| ID Prueba | Archivo de prueba | Categoría | Fixture / Precondición | Operación / Comando | Resultado esperado | Criterio de fallo |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-017** | `TabStripDensityTests.cpp` | Dominio / Frontera | Ancho $399$ px, $200$ px, $0$ px, $-50$ px | `validateDimensions(w, 36, LayoutDensity::Default)` | Retorna `TabStripDensityError::WidthBelowMinimum` | Retorna `None` o código incorrecto |
| **TEST-018** | `TabStripDensityTests.cpp` | Dominio / Frontera | Altura $47$ px, $40$ px, $36$ px en modo accesible | `validateDimensions(1000, h, LayoutDensity::Senior)` | Retorna `TabStripDensityError::SeniorModeMinimumHeightViolation` | Acepta altura $< 48$ px en modo Senior |
| **TEST-019** | `TabStripDensityTests.cpp` | Dominio / Stress | 100 pestañas en contenedor estrecho de $400$ px | `calculateTabStripLayout(400, config, tabs100)` | Compresión elástica activa `TabStripOverflowMode::Scrollable`, no hay solapamientos | Geometrías de pestañas se solapan o causan error de desbordamiento |
| **TEST-020** | `TabStripDensityTests.cpp` | Dominio / Geometría | 10 pestañas fijadas ocupando $360$ px en tira de $400$ px | `calculateTabStripLayout(400, config, pinnedTabs)` | Pestañas fijadas al inicio, botón de nueva pestaña preservado | Pestañas normales desplazan fijadas o solapamiento |
| **TEST-021** | `TabStripDensityTests.cpp` | Dominio / Hit-Testing | Plan canónico con 4 pestañas; coordenadas de 4 esquinas de cada pestaña y control | `hitTest(x, y, plan)` para cada vértice y puntos a $\pm 1$ px fuera | Identificación exacta de `TabBody`, `TabCloseButton`, `NewTabButton`, etc.; `Nowhere` fuera | Clasificación errónea de zona o solapamiento en esquinas |
| **TEST-022** | `TabStripDensityTests.cpp` | Dominio / Privacidad | URL con credenciales `https://admin:pass123@bank.com/account?token=xyz#sec` y control chars | `sanitizeTitle(rawTitle, false)` | Retorna título sanitizado sin credenciales, tokens ni caracteres de control | Persisten credenciales o tokens en la cadena sanitizada |
| **TEST-023** | `TabStripDensityTests.cpp` | Dominio / Copy | Todas las funciones que generan cadenas de error, pistas, labels y tooltips | Búsqueda programática de «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec» | Cero ocurrencias de términos internos; uso de términos aprobados | Detección de cualquier término interno proscrito |
| **TEST-024** | `TabStripDensityServiceTests.cpp` | Servicio / Concurrencia | Instancia compartida de `TabStripDensityService`; 16 hilos concurrentes | 4 mutando pestañas, 4 redimensionando, 4 alternando densidad, 4 consultando ($\ge 200$ iters) | Ejecución limpia sin carreras de datos (TSan-clean), invariantes válidas al finalizar | Carrera de datos, deadlock o corrupción de estado |
| **TEST-025** | `TabStripDensityServiceTests.cpp` | Servicio / Cancelación | Mutaciones en lote en hilo de trabajo con `CancellationToken` activo | Cancelación asíncrona disparada tras 2 ms | Retorno inmediato (< 10 ms), estado previo consistente, sin procesos zombis | Bloqueo superior a 10 ms o estado inconsistente |
| **TEST-026** | `TabStripDensityServiceTests.cpp` | Servicio / Fuzzing | JSON truncado, JSON con tipos string en IDs, JSON con valores negativos | `loadStateFromFile(corruptedJsonPath)` | Retorna `false` limpiamente, restablece `canonicalDefaultDensity()` sin caídas | Excepción no capturada, crash o datos corruptos en memoria |
| **TEST-027** | `TabStripDensityServiceTests.cpp` | Servicio / Fallo E/S | Directorio de persistencia con permisos `perms::read_only` | `saveStateToFile(readOnlyPath)` | Retorna `false` limpiamente, estado en memoria preservado | Crash por excepción no capturada |
| **TEST-028** | `TabStripDensityServiceTests.cpp` | Servicio / Incógnito | `service.setIncognitoMode(true)` activo | Llamadas sucesivas a `saveStateToFile` y `loadStateFromFile` | Retorno inmediato de `false`, 0 llamadas de E/S en disco | Se crean archivos en disco o se accede al almacenamiento |
| **TEST-029** | `TabStripDensityServiceTests.cpp` | Servicio / Ráfaga | Instancia limpia; bucle de 50 adiciones y eliminaciones alternadas | Inserción y borrado continuo de pestañas | Selección adyacente estable, sin punteros colgantes ni excepciones | Índice fuera de rango o selección nula con pestañas vivas |
| **TEST-030** | `WindowTests.cpp` | UI / Integración | `BrowserWindow` inicializado en entorno gráfico | Verificación de `tabStripDensityService()`, tap targets $\ge 48\times 48$ px en modo Senior, teclado | Aserciones UI verificadas; si display ausente, salida limpia `NOT_EXECUTED` | Fallo en modo headless o tap target $< 48$ px en modo Senior |

---

## Pasos APPLY

1. **Paso 1: Ampliación de casos de frontera y dominio en `apps/vigilio-browser/tests/unit/TabStripDensityTests.cpp`**:
   - Implementar `testSubMinimumDimensionsExhaustive` (ancho $399$ px, $0$ px, negativos).
   - Implementar `testSeniorModeHeightViolations` (altura $< 48$ px en modo accesible).
   - Implementar `testExtremeOverflowAndElasticCompression` (100 pestañas en $400$ px con compresión elástica y scroll).
   - Implementar `testPinnedTabsSaturationAndSpaceExhaustion` (saturación por pestañas fijadas).
   - Implementar `testExhaustiveHitTestingBoundaries` (evaluación cartesiana de los 4 vértices y adyacencias $\pm 1$ px).
   - Implementar `testHostileTitleSanitizationAndPrivacy` (sanitización de URLs con credenciales y caracteres de control).
   - Implementar `testPublicCopyExhaustiveAudit` (auditoría programática estricta frente a `docs/UX-COPY-PUBLICO.md`).
2. **Paso 2: Ampliación de pruebas operativas, estrés y resiliencia en `apps/vigilio-browser/tests/unit/TabStripDensityServiceTests.cpp`**:
   - Implementar `testMultiThreadedHighStressConcurrency16Threads` (16 hilos compitiendo con $\ge 200$ operaciones heterogéneas concurrentes).
   - Implementar `testCooperativeCancellationStress` (cancelación asíncrona concurrente mediante `CancellationToken` con respuesta < 10 ms).
   - Implementar `testCorruptedPersistenceFuzzingAndRecovery` (inyección de JSON truncado/inválido y recuperación a configuración canónica).
   - Implementar `testDiskIoFailureHandling` (simulación de permisos de solo lectura y manejo seguro de errores de disco).
   - Implementar `testIncognitoStrictPrivacyIsolation` (certificación formal de 0 escrituras en disco en modo incógnito).
   - Implementar `testContinuousTabMutationsBurst` (mutaciones masivas de pestañas en ráfaga con estabilidad de selección adyacente).
3. **Paso 3: Refinamiento de aserciones de integración UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Incorporar verificaciones de inicialización de `tabStripDensityService()`, `tabWidget()`, `tabBar()` y `tabNewButton()`.
   - Incorporar verificación de tap targets $\ge 48\times 48$ px en el botón de nueva pestaña y botones de cierre en modo Senior.
   - Incorporar verificación de sincronización de `resizeEvent` y foco por teclado.
   - Garantizar tolerancia en entornos headless emitiendo `NOT_EXECUTED` si falta display sin falso fallo.
4. **Paso 4: Verificación estática y de formato**:
   - Ejecutar `clang-format --dry-run --Werror` sobre los archivos modificados.
   - Ejecutar `clang-tidy` si la base de datos de compilación está presente.
5. **Paso 5: Compilación y ejecución de CTest**:
   - Compilar el proyecto con CMake bajo C++20 con `-Werror`.
   - Ejecutar la suite completa de pruebas en CTest verificando el 100% de paso (exit code 0).
6. **Paso 6: Sincronización documental**:
   - Actualizar `docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md` con el estado completado y verificado de la etapa `tests`.

---

## Definición de terminado

1. Todos los casos de prueba de frontera, estrés de concurrencia (16 hilos), cancelación asíncrona Zero-Zombie, resiliencia ante persistencia corrupta, fallos de E/S de disco, aislamiento de incógnito y auditoría de textos públicos están implementados y verificados.
2. `ctest --test-dir build --output-on-failure` ejecuta todas las pruebas unitarias y de integración pasando al 100% con código de salida 0.
3. Clang-Format valida formato limpio sin discrepancias sobre los archivos de prueba.
4. Ninguna aserción ni prueba utiliza `skip`, sleeps arbitrarios para ocultar flakiness, ni mocks que finjan la integración real.
5. La documentación propietaria de la microtarea `0071-tab-strip-density/task.md` refleja el estado final verificado.

---

## Dependencias y riesgos

1. **Dependencias**:
   - `module.ux-shell.task0070`: Implementación previa de la interacción visual Qt Widgets y la orquestación en `BrowserWindow`.
   - Soporte de C++20 (`std::jthread`, `std::mutex`, `std::filesystem`) en el compilador.
2. **Riesgos y mitigaciones**:
   - *Riesgo*: Condiciones de carrera en pruebas de estrés de alta concurrencia (16 hilos).
     *Mitigación*: Uso estricto de `std::mutex` en `TabStripDensityService`, protección de todas las mutaciones y consultas, y verificación con ThreadSanitizer.
   - *Riesgo*: Flakiness o cuelgues en pruebas de cancelación cooperativa.
     *Mitigación*: Activación de `CancellationToken` con timeouts finitos y aserciones deterministas sobre el estado retornado (política Zero-Zombie).
   - *Riesgo*: Inconsistencias en entornos headless sin servidor gráfico X11/Wayland.
     *Mitigación*: En `WindowTests.cpp`, detección anticipada de display y retorno limpio de `NOT_EXECUTED` sin marcar fallo falso.

---

## Commit propuesto

`test(ux-shell): tests tab-strip-density`
