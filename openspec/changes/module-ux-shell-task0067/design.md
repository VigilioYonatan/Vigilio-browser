# Design — module-ux-shell-task0067

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del subsistema `title bar integration` tras las etapas precedentes (`task0064`, `task0065`, `task0066`)**:
   - `module.ux-shell.task0064` (commit `ac3786a`) formalizó el modelo de dominio en `TitleBarTypes.h`, `TitleBarConfiguration.h/.cpp` y `TitleBarValidator.h/.cpp`, con 14 pruebas unitarias iniciales de invariantes y cálculo analítico de distribución espacial.
   - `module.ux-shell.task0065` (commit `5358b61`) implementó la capa operativa desacoplada en `TitleBarService.h/.cpp`, proveyendo gestión multihilo del plan (`std::mutex`), conmutación de densidades (`setDensity`), control de colocación de controles (`setControlsPlacement`), mutaciones de estado de ventana (`setWindowState`, `updateButtonState`), persistencia atómica local con tolerancia a fallos (`saveStateToFile`, `loadStateFromFile`), aislamiento de incógnito y cancelación cooperativa Zero-Zombie (`CancellationToken`), respaldado por 14 pruebas unitarias de servicio.
   - `module.ux-shell.task0066` (commit `710169b`) integró físicamente la interacción Qt Widgets en la ventana principal (`BrowserWindow.h/.cpp`), estructurando el contenedor superior (`vigilio-titlebar`), la zona ergonómica de arrastre (`vigilio-titlebar-drag-area`), la caja de controles de ventana (`vigilio-titlebar-controls`), el botón de alternancia rápida a la «Interfaz más fácil de usar» (`vigilio-titlebar-senior-toggle`), la captura reactiva de eventos nativos (`resizeEvent`, `WindowStateChange`) y un arnés inicial de verificación en `WindowTests.cpp`.
2. **Limitaciones de cobertura en el arnés de pruebas actual**:
   - *Fronteras y casos negativos incompletos en el validador*:
     - No existen pruebas unitarias sistemáticas que exploren las fronteras exactas de dimensiones de ventana sub-mínimas (e.g. $399$ px, $350$ px, $100$ px), dimensiones no positivas ($\le 0$) o valores negativos patológicos.
     - No se validan exhaustivamente las violaciones de altura mínima en la densidad `Senior` ($47$ px, $32$ px) frente a `TitleBarError::SeniorModeMinimumHeightViolation`.
     - No se prueban sistemáticamente configuraciones donde el área de arrastre se agota por controles sobredimensionados en ventanas estrechas (`TitleBarError::DraggableAreaInsufficient`).
     - No se prueba la disyunción espacial estricta en resoluciones ultrapanorámicas ($3840\times 1080$), verticales ($1080\times 1920$) y 4K ($3840\times 2160$).
   - *Ausencia de pruebas de estrés bajo concurrencia masiva (16 hilos)*:
     - Las pruebas de servicio existentes evalúan concurrencia básica con 8 hilos ejecutando operaciones homogéneas, pero no someten la instancia compartida a estrés mixto masivo (16 hilos ejecutando simultáneamente redimensionamiento, conmutación de densidad, mutación de controles y consultas de plan).
   - *Pruebas de fuzzing y recuperación de persistencia limitadas*:
     - Solo se evalúa un caso básico de archivo JSON corrupto; falta evaluar inyecciones sistemáticas de JSON truncado, JSON con tipos dispares (strings en dimensiones), JSON con violaciones geométricas deliberadas y simulación de fallos de disco por permisos de solo lectura (`perms::read_only`).
   - *Aislamiento estricto de navegación privada*:
     - Se requiere certificar formalmente que en modo incógnito ni la lectura ni la escritura tocan el sistema de archivos local bajo ninguna circunstancia (0 llamadas de I/O en disco).
   - *Hit-testing de bordes y esquinas*:
     - El hit-testing actual solo evalúa los puntos centrales de los controles; falta verificar las coordenadas exactas de las 4 esquinas de cada botón y puntos a 1 px de distancia fuera de los límites.

---

### Estado objetivo (TARGET)

1. **Suite integral de pruebas de dominio y fronteras en `TitleBarIntegrationTests.cpp`**:
   - Cobertura exhaustiva de todos los códigos de error definidos en `TitleBarError`:
     - `DimensionsBelowMinimum` verificado en los límites exactos ($399$ px, $0$ px, valores negativos).
     - `SeniorModeMinimumHeightViolation` verificado ante alturas $< 48$ px en modo accesible.
     - `DraggableAreaInsufficient` verificado cuando el espacio restante de arrastre es inferior a 120 px (Default), 160 px (Senior) o 100 px (Compact).
     - `ControlsOverflowWidth` y `ConflictingControlPlacement` verificados ante geometrías incompatibles.
   - Cálculo analítico de áreas comprobado bajo geometrías extremas (ultrawide $3840\times 1080$, vertical $1080\times 1920$ y 4K $3840\times 2160$), certificando la invariante fundamental de no solapamiento espacial ($\forall i \ne j, \text{control}_i \cap \text{control}_j = \emptyset$, $\text{controlsBoxRect} \cap \text{draggableRect} = \emptyset$).
   - Hit-testing espacial exhaustivo evaluando esquinas exactas, fronteras y puntos exteriores (`TitleBarHitTestZone::Nowhere`).
   - Sanitización robusta ante entradas hostiles (URLs con credenciales embebidas, tokens, secuencias de escape, fragmentos y strings de longitud extrema).
   - Auditoría programática de todas las cadenas de texto, pistas de remediación, tooltips y etiquetas accesibles frente a `docs/UX-COPY-PUBLICO.md`.
2. **Suite integral de pruebas operativas, resiliencia y concurrencia en `TitleBarServiceTests.cpp`**:
   - Prueba de estrés multihilo de alta concurrencia con 16 hilos (`std::thread`) concurrentes ejecutando operaciones heterogéneas continuas (redimensionamiento, cambio de densidad, alternancia de colocación y consultas de plan), validando la ausencia absoluta de carreras de datos, interbloqueos y corrupción de memoria.
   - Prueba de cancelación cooperativa multihilo mediante `CancellationToken` disparado en instantes asíncronos arbitrarios, verificando respuesta inmediata (< 10 ms), preservación del estado previo y ausencia de recursos huérfanos (política Zero-Zombie).
   - Batería de pruebas de recuperación y resiliencia ante corrupción de archivos de persistencia:
     - Inyección de JSON truncado.
     - Inyección de JSON con campos de tipos dispares.
     - Inyección de JSON con valores geométricos inválidos deliberados.
     - Fallos simulados de escritura en disco por directorios de solo lectura (`std::filesystem::perms::read_only`).
     - Verificación del restablecimiento transparente del plan canónico por defecto (`canonicalDefaultTitleBar()`) y retorno de `false` sin caídas de proceso.
   - Verificación de aislamiento absoluto en modo incógnito: llamadas a persistencia abortadas inmediatamente con cero operaciones de I/O en disco.
   - Validación de transiciones continuas de colocación de controles (`Left` $\leftrightarrow$ `Right`) y estados de ventana (`Maximize` $\leftrightarrow$ `Restore`, `Fullscreen` $\leftrightarrow$ `Normal`).
3. **Arnés de integración de interfaz gráfica en `WindowTests.cpp`**:
   - Validación de la integración reactiva entre `BrowserWindow` y `TitleBarService`:
     - Verificación de que `resizeEvent` sincroniza dimensiones con el servicio.
     - Verificación de que `changeEvent(WindowStateChange)` actualiza dinámicamente el botón de maximizar/restaurar.
     - Verificación de que la activación de la «Interfaz más fácil de usar» impone altura $\ge 48$ px y tap targets $\ge 48\times 48$ px en todos los botones de la barra.
     - Verificación de navegación por teclado y atributos de accesibilidad (`accessibleName`, `accessibleDescription`).
     - Verificación de tolerancia headless determinista (reportando `NOT_EXECUTED` limpiamente si `DISPLAY`/`WAYLAND_DISPLAY` está ausente).
     - Auditoría estricta de textos accesibles y cadenas visibles frente a `docs/UX-COPY-PUBLICO.md`.

---

### Brechas identificadas (GAPS)

| Componente de prueba | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Fronteras dimensionales** | Pruebas básicas de mínimos generales | Pruebas de límites exactos ($399$ px, $0$ px, negativos, altura Senior $< 48$ px) | Añadir casos de frontera exacta en `TitleBarIntegrationTests.cpp`. |
| **Agotamiento de arrastre** | Prueba simple de ventana estrecha | Validación de `DraggableAreaInsufficient` con controles expandidos | Añadir caso de prueba de arrastre insuficiente en `TitleBarIntegrationTests.cpp`. |
| **Geometrías extremas** | Solo se prueba resolución $1200$ px | Pruebas en ultrawide, formato vertical y 4K sin solapamientos | Añadir cálculos espaciales en resoluciones atípicas. |
| **Hit-testing de bordes** | Hit-testing en puntos medios | Hit-testing en esquinas de botones y coordenadas exteriores a la ventana | Añadir pruebas de límites de hit-testing en `TitleBarIntegrationTests.cpp`. |
| **Sanitización hostil** | URL simple con token | URLs con credenciales embebidas, saltos de línea y longitud extrema | Añadir casos borde de sanitización en `TitleBarIntegrationTests.cpp`. |
| **Concurrencia multihilo** | Concurrencia básica con 8 hilos | Concurrencia de alto estrés con 16 hilos ejecutando operaciones mixtas | Añadir prueba masiva de 16 hilos en `TitleBarServiceTests.cpp`. |
| **Fuzzing de persistencia** | Caso simple de JSON corrupto | Inyección de JSON truncado, tipos dispares y valores ilegales | Añadir batería de fuzzing y recuperación de JSON. |
| **Simulación de fallos de disco** | No se simulan permisos denegados | Simulación de directorios de solo lectura y fallos en escritura atómica | Añadir pruebas de manejo de errores de I/O en persistencia. |
| **Aislamiento en incógnito** | Prueba funcional básica | Certificación estricta de cero operaciones de I/O en disco durante incógnito | Añadir aserciones de no interacción con archivos en disco. |
| **Integración UI y eventos** | Pruebas estructurales básicas | Verificación de redimensionamiento reactivo, foco de teclado y tap targets $\ge 48$ px | Refinar aserciones en `tests/ui/WindowTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad arquitectónica y estándares C++20

- **Desacoplamiento limpio entre capas**:
  - `TitleBarTypes.h`, `TitleBarConfiguration` y `TitleBarValidator` (`src/core/`) no dependen de bibliotecas gráficas ni de Qt.
  - `TitleBarService` (`src/core/`) opera exclusivamente con tipos estándar de C++20 (`std::mutex`, `std::unique_ptr`, `std::vector`, `std::filesystem`), permitiendo su ejecución ultrarrápida y determinista en cualquier plataforma compatible.
  - Las suites de pruebas unitarias (`TitleBarIntegrationTests.cpp` y `TitleBarServiceTests.cpp`) se ejecutan sin dependencias de servidor gráfico y completan su verificación en < 1 segundo en CTest.
- **Conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - Todas las operaciones de prueba se confinan a memoria o a directorios temporales locales gestionados mediante RAII, garantizando limpieza automática y cero dependencias de conectividad de red o servicios remotos.

### Modelo de pruebas y concurrencia multihilo

```
                               ┌────────────────────────────────┐
                               │     TitleBarService (inst)     │
                               │    Protegido por std::mutex    │
                               └────────────────┬───────────────┘
                                                │
         ┌───────────────────┬──────────────────┴────────────────┬───────────────────┐
         ▼                   ▼                                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐                 ┌─────────────────┐ ┌─────────────────┐
│ Hilos 1-4:      │ │ Hilos 5-8:      │                 │ Hilos 9-12:     │ │ Hilos 13-16:    │
│ setWindowWidth  │ │ setDensity      │                 │ setControls     │ │ Consultas:      │
│ (redimensionar) │ │ (conmutar)      │                 │ & windowState   │ │ hitTest, plan   │
└─────────────────┘ └─────────────────┘                 └─────────────────┘ └─────────────────┘
```

### Plan de rollback y mitigación

1. **Aislamiento de cambios en pruebas**: Las modificaciones planificadas en APPLY se concentran en archivos de prueba (`tests/unit/TitleBarIntegrationTests.cpp`, `tests/unit/TitleBarServiceTests.cpp`, `tests/ui/WindowTests.cpp`), por lo que no alteran la lógica de negocio ni la interfaz de producción de la aplicación.
2. **Reversibilidad inmediata**: En caso de que alguna prueba introduzca inestabilidad o flakiness en entornos con recursos limitados, el commit puede revertirse de forma atómica mediante `git revert` sin afectar el código de producción.
3. **Preservación del pipeline**: La suite de pruebas actual cuenta con 29 pruebas aprobadas al 100%. Las nuevas pruebas son aditivas y deben mantener el 100% de éxito en CTest bajo `scripts/check.sh`.
