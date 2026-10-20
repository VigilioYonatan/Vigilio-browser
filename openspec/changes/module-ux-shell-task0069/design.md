# Design — module-ux-shell-task0069

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del subsistema de pestañas tras la etapa de dominio (`module.ux-shell.task0068`)**:
   - La microtarea precedente `module.ux-shell.task0068` formalizó la etapa `domain` de la capacidad `tab strip density`, aportando:
     - `TabStripDensityTypes.h`: Enumeraciones fuertemente tipadas (`TabPinState`, `TabMediaState`, `TabDiscardState`, `TabLoadingState`, `TabInteractiveState`, `TabStripHitTestZone`, `TabStripOverflowMode`, `TabStripDensityError`) y estructuras inmutables de semántica de valor (`TabStripDimensions`, `TabSpec`, `TabItemLayout`, `TabStripDensityConfig`, `TabStripDensityPlan`, `TabStripDensityReport`).
     - `TabStripDensityConfiguration.h/.cpp`: Constantes canónicas (altura estándar 36 px, altura Senior 48 px, altura Compact 28 px; anchos preferidos, mínimos y máximos de pestañas regulares; anchos de pestañas fijadas $36$ px estándar / $\ge 48$ px Senior / $28$ px Compact; botones de nueva pestaña y áreas táctiles $\ge 48\times 48$ px en Modo Senior), factorías canónicas (`canonicalDefaultDensity`, `canonicalSeniorDensity`, `canonicalCompactDensity`) y verificación rigurosa de invariantes geométricas (`verifyInvariants`).
     - `TabStripDensityValidator.h/.cpp`: Métodos analíticos puros `validateDimensions`, `validatePlan`, algoritmo determinista de distribución espacial con compresión elástica `calculateTabStripLayout`, clasificación de zonas de impacto espacial `hitTest(x, y)` y funciones de lenguaje público y accesibilidad en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
     - `tests/unit/TabStripDensityTests.cpp`: 16 pruebas unitarias en CTest ejecutándose limpiamente al 100%.
2. **Limitaciones de la capa de gestión operativa actual**:
   - En la base de código actual no existe todavía una capa de servicio operativa (`TabStripDensityService`) en `vigilio_core`:
     - El cálculo de geometría se ejecuta como función pura estática sin un componente que mantenga el estado de la tira de pestañas en memoria de forma reactiva, determinista y thread-safe.
     - No existe gestión del ciclo de vida operativo de la colección de pestañas: inserción ordenada respetando la partición de fijadas/regulares, reordenamiento, selección adyacente ante cierres, y alternancia de estados de audio o suspensión de memoria.
     - No existe persistencia en disco de la configuración y estado de la tira de pestañas bajo la arquitectura Local-First.
     - No existe integración con la política Zero-Zombie para abortar cálculos de distribución espacial o escrituras en disco ante una solicitud de cancelación cooperativa.
3. **Aislamiento de incógnito y resiliencia ante fallos pendientes**:
   - No existe un mecanismo que garantice que en ventanas privadas o de incógnito las pestañas y metadatos se conserven exclusivamente en memoria volátil sin escribir bytes en disco.
   - No existe una estrategia de persistencia atómica mediante archivo temporal (`.tmp`) y renombramiento seguro, ni recuperación automática ante corrupción de archivos de estado de pestañas.

---

### Estado objetivo (TARGET)

1. **Servicio operativo desacoplado `TabStripDensityService` en `vigilio_core`**:
   - Una clase propietaria C++20 thread-safe sincronizada con `mutable std::mutex`, aislada de dependencias de interfaz gráfica (`QWidget`, `QTabBar`, `QTabWidget`), que actúa como la autoridad operativa sobre el estado, colección y geometría de la tira de pestañas.
   - Encapsula un `TabStripDensityConfig`, la anchura disponible actual (`availableWidth`), la colección secuencial de pestañas (`std::vector<TabSpec>`) y el `TabStripDensityPlan` calculado, asegurando consistencia matemática en todo momento.
2. **Orquestación dinámica de estados, densidades y colección**:
   - `setDensity(LayoutDensity)`: Transición limpia entre densidad estándar, «Interfaz más fácil de usar» (Modo Senior) y compacta, recalculando automáticamente dimensiones de pestañas regulares, fijadas, botón de nueva pestaña, espaciado y botones de cierre.
   - Operaciones de colección de pestañas:
     - `addTab` / `insertTab`: Inserción con preservación obligatoria de la invariante de precedencia de pestañas fijadas ($x_{\text{pinned}} < x_{\text{regular}}$).
     - `removeTab`: Cierre con selección determinista de la pestaña adyacente más próxima (derecha preferente, o izquierda).
     - `moveTab`: Desplazamiento validado dentro de la partición correspondiente.
     - `selectTab`: Activación de pestaña con garantía de botón de cierre visible.
     - `setTabPinned`: Alternancia de anclaje con ajuste a ancho canónico invariante.
     - `setTabMediaState` / `toggleTabAudioMute`: Control de indicadores de sonido y silenciamiento.
     - `setTabDiscardState`: Suspensión de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo») con actualización de etiquetas y tooltips.
     - `updateTabTitle`: Sanitización de privacidad y actualización de títulos y URLs.
3. **Manejo de desbordamiento horizontal y redimensionamiento**:
   - `setAvailableWidth(int)`: Ajuste dinámico ante redimensionamiento de ventana con validación de límites mínimos ($W \ge 400$ px estándar, $500$ px Senior, $360$ px Compact).
   - Detección analítica de desbordamiento (`isOverflowing`), activando controles de desplazamiento y modo `Scrollable`.
4. **Persistencia local atómica y resiliencia ante corrupción (Local-First)**:
   - `saveStateToFile` / `loadStateFromFile`: Serialización y deserialización local estructurada.
   - Escritura atómica basada en archivo temporal `.tmp` y renombramiento seguro (`std::filesystem::rename`) para evitar archivos truncados ante caídas o cortes de energía.
   - Aislamiento absoluto de sesiones de incógnito (`setIncognitoMode(true)`): no se realiza ninguna escritura a disco en modo privado; toda persistencia es rechazada de inmediato.
   - Fallback de recuperación ante archivos corruptos: si el archivo de estado en disco está dañado, el servicio restaura automáticamente `canonicalDefaultDensity()` sin arrojar excepciones ni colgar el navegador.
5. **Soporte de cancelación cooperativa Zero-Zombie**:
   - Aceptación opcional de `CancellationToken*` en métodos de recálculo y persistencia, garantizando aborto inmediato si la tarea es cancelada.
6. **Observabilidad y eventos desacoplados**:
   - Callback de notificación `setTabStripChangedCallback` que permite a la interfaz Qt en la futura etapa UI (`module.ux-shell.task0070`) reaccionar a cambios sin acoplamiento circular.
   - Formateo de diagnósticos y resúmenes de estado legibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
7. **Batería de pruebas unitarias en CTest**:
   - Target `vigilio_tab_strip_density_service_tests` ejecutando 17 pruebas unitarias rápidas e independientes (< 100 ms).

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Capa de servicio de tira de pestañas** | Inexistente; solo validador estático y tipos de dominio en `task0068` | `TabStripDensityService` en `vigilio_core` desacoplado de GUI | Implementar la clase de servicio con control de estado, mutex y métodos públicos de mutación. |
| **Gestión de colección de pestañas** | `TabModel` elemental sin geometría ni densidades | Servicio que orquesta `std::vector<TabSpec>` con recálculo espacial atómico | Implementar métodos `addTab`, `insertTab`, `removeTab`, `selectTab`, `moveTab` en el servicio. |
| **Precedencia de pestañas fijadas** | Validada solo analíticamente en pruebas aisladas | Invariante aplicada y preservada en todas las mutaciones operativas | Garantizar que las pestañas fijadas se inserten y ordenen a la izquierda de las regulares. |
| **Selección adyacente ante cierre** | No gestionada en la capa de geometría | Selección automática inteligente de la pestaña vecina más próxima | Lógica de selección adyacente determinista en `removeTab`. |
| **Transición reactiva de densidades** | Funciones canónicas aisladas en configuración | Transición tipada reactiva en `TabStripDensityService::setDensity` | Recálculo sistemático de dimensiones de pestañas regulares, fijadas y botones según densidad. |
| **Persistencia atómica y tolerancia a fallos** | Inexistente para la tira de pestañas | Guardado atómico (`.tmp` + rename) y recuperación ante archivos corruptos | Implementar serialización JSON, deserialización tolerante y fallback canónico. |
| **Aislamiento de modo incógnito** | No enforced en la capa operativa de pestañas | Bloqueo incondicional de persistencia a disco cuando `isIncognitoSession == true` | Validación de bandera de incógnito en métodos de persistencia. |
| **Cancelación cooperativa Zero-Zombie** | No implementada en el subsistema de pestañas | Soporte de `CancellationToken` en cálculo y persistencia | Integración de comprobación `isCancelled()` en operaciones potencialmente intensivas. |
| **Observabilidad desacoplada** | Sin mecanismo de notificación de cambios de tira | Callback `OnTabStripChangedCallback` y formateador de resúmenes en español neutro | Exponer suscripción desacoplada y método `formatTabStripSummary`. |
| **Suite de pruebas de servicio** | Cero pruebas de servicio de tira de pestañas en CTest | 17 pruebas unitarias bajo `vigilio_tab_strip_density_service_tests` | Crear `tests/unit/TabStripDensityServiceTests.cpp` y registrarlo en `CMakeLists.txt`. |

---

## Compatibility, migration and rollback

### Compatibilidad arquitectónica y estándares C++20

- **Desacoplamiento limpio entre capas**:
  - `TabStripDensityService` reside íntegramente en `apps/vigilio-browser/src/core/` y pertenece a la biblioteca estática `vigilio_core`.
  - No incluye ninguna cabecera de Qt Widgets (`QWidget`, `QTabBar`, `QTabWidget`), Qt Core ni Chromium WebEngine.
  - Se garantiza compatibilidad total con compiladores modernos de C++20 (GCC 11+, Clang 14+, MSVC 2022) con flags estrictas `-Wall -Wextra -Wpedantic -Werror`.
- **Conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - Todo el ciclo de vida y cálculo espacial opera 100% en memoria local, sin interacción con redes externas ni sincronización remota.
- **Eficiencia de memoria y tiempo de CPU**:
  - Complejidad lineal $\mathcal{O}(N)$ respecto al número de pestañas $N$ en recálculo geométrico.
  - Sincronización multihilo eficiente mediante exclusión mutua granular (`std::mutex`) sin bloqueos de contención prolongados.

---

### Diagrama arquitectónico del subsistema de pestañas

```
┌────────────────────────────────────────────────────────────────────────┐
│                          ux-shell: Capa UI                             │
│               (TabStripWidget / BrowserWindow / QSS)                   │
│                     [Etapas futuras: 0070, 0071]                       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Consulta plan / Se suscribe
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                ux-shell: Capa Operativa Propietaria                    │
│                     TabStripDensityService                             │
│                  [Esta etapa: 0069 (implementation)]                   │
│  - Mutaciones de pestañas (add, remove, select, pin, mute, discard)   │
│  - Transición de densidades (Default, Senior, Compact)                 │
│  - Persistencia atómica local (.tmp + rename) y resiliencia            │
│  - Aislamiento estricto de incógnito y política Zero-Zombie           │
│  - Observabilidad y resúmenes públicos en español neutro               │
└───────────────────┬────────────────────────────────┬───────────────────┘
                    │                                │
                    ▼                                ▼
┌────────────────────────────────────┐ ┌─────────────────────────────────┐
│     TabStripDensityValidator       │ │  TabStripDensityConfiguration   │
│  - calculateTabStripLayout (O(N))  │ │  - Constantes dimensionales     │
│  - hitTest (O(1)) cartesiano       │ │  - Factorías canónicas          │
│  - validatePlan / validateDims     │ │  - verifyInvariants             │
│  - Textos accesibles español       │ └─────────────────────────────────┘
└────────────────────────────────────┘
                    │
                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     TabStripDensityTypes                               │
│  (TabPinState, TabMediaState, TabDiscardState, TabItemLayout, etc.)    │
│                        [Etapa 0068 (domain)]                           │
└────────────────────────────────────────────────────────────────────────┘
```

---

### Estrategia de migración y rollback

- **Migración incremental**:
  - `TabStripDensityService` opera como una capa de servicio independiente y no modifica las APIs existentes de `TabModel` ni `MainWindowLayoutService`.
  - La integración con `BrowserWindow` ocurrirá en la etapa `ui` (`module.ux-shell.task0070`), asegurando que ningún código de producción en ejecución se vea alterado prematuramente.
- **Estrategia de rollback**:
  - En caso de necesidad de revertir este cambio, basta con eliminar `TabStripDensityService.h`, `TabStripDensityService.cpp` y `TabStripDensityServiceTests.cpp`, y remover sus entradas en `CMakeLists.txt`.
  - La base de código retornará limpiamente al estado de la etapa de dominio (`module.ux-shell.task0068`) sin dejar artefactos huérfanos ni alterar la estabilidad del proyecto.
