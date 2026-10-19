# Design — module-ux-shell-task0065

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del árbol de código y finalización de la etapa de dominio (`task0064`)**:
   - La microtarea `module.ux-shell.task0064` (commit `ac3786a`) formalizó la etapa `domain` de la capacidad `title bar integration`, aportando:
     - `TitleBarTypes.h`: Enumeraciones fuertemente tipadas (`TitleBarMode`, `TitleBarControlsPlacement`, `TitleBarButtonType`, `TitleBarButtonState`, `TitleBarHitTestZone`, `TitleBarError`) y estructuras inmutables de semántica de valor (`TitleBarDimensions`, `TitleBarControlSpec`, `TitleBarState`, `TitleBarConfig`, `TitleBarLayoutPlan`, `TitleBarReport`).
     - `TitleBarConfiguration.h/.cpp`: Constantes canónicas (altura estándar 32 px, altura Senior 48 px, altura Compact 28 px, botones estándar $46\times 32$ px, botones Senior $\ge 48\times 48$ px, presupuestos mínimos de arrastre $\ge 120$ px estándar / $\ge 160$ px Senior), factorías canónicas (`canonicalDefaultTitleBar`, `canonicalSeniorTitleBar`, `canonicalCompactTitleBar`) y verificación de invariantes (`verifyInvariants`).
     - `TitleBarValidator.h/.cpp`: Métodos analíticos puros `validateDimensions`, `validatePlan`, algoritmo de distribución espacial `calculateTitleBarLayout`, función de clasificación de pruebas de impacto cartesiana `hitTest(x, y)` y funciones auxiliares de copy público en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
     - `tests/unit/TitleBarIntegrationTests.cpp`: 14 pruebas unitarias en CTest ejecutándose limpiamente al 100%.
2. **Limitaciones de la capa de gestión de barra de título actual**:
   - En la base de código actual no existe una capa de servicio operativa (`TitleBarService`) en `vigilio_core`:
     - El cálculo de geometría se ejecuta como función pura estática sin un componente que mantenga el estado de la barra en memoria de forma reactiva y thread-safe.
     - No existe gestión de ciclo de vida para transiciones entre densidades visuales, alternancia de anclaje de controles (derecha/izquierda) ni actualización atómica de estados interactivos de botones (hover, pressed).
     - No existe persistencia en disco de las preferencias de la barra de título (modo integrado/nativo, visibilidad de icono, posición de controles).
     - No existe integración con la política Zero-Zombie para abortar cálculos pesados o escrituras de disco si se recibe una solicitud de cancelación cooperativa.
3. **Aislamiento de incógnito y resiliencia ante fallos pendientes**:
   - No existe un mecanismo que garantice que en ventanas privadas o de incógnito las preferencias de la barra se conserven exclusivamente en memoria volátil sin tocar el disco.
   - No existe una estrategia de respaldo atómico y fallback seguro si el archivo de estado de la barra en disco se encuentra corrupto o truncado.

---

### Estado objetivo (TARGET)

1. **Servicio operativo desacoplado `TitleBarService` en `vigilio_core`**:
   - Una clase propietaria C++20 thread-safe (`std::mutex`), aislada de dependencias de interfaz gráfica (`QWidget`, `QMainWindow`), que actúa como la autoridad operativa sobre el estado y geometría de la barra de título integrada.
   - Encapsula un `TitleBarConfig`, el ancho de ventana actual y el `TitleBarLayoutPlan` calculado, asegurando consistencia matemática en todo momento.
2. **Orquestación dinámica de estados y densidades**:
   - `setDensity(LayoutDensity)`: Transición limpia entre densidad estándar, «Interfaz más fácil de usar» (Modo Senior) y compacta, recalculando automáticamente dimensiones de botones, icono y área de arrastre.
   - `setControlsPlacement(TitleBarControlsPlacement)`: Conmuta entre controles a la derecha o a la izquierda sin generar colisiones ni solapamientos.
   - `setTitle(std::string)`: Saneamiento y recorte preventivo del título de ventana para salvaguardar la privacidad y evitar inyecciones.
   - `setWindowState(const TitleBarState&)`: Actualización del estado de ventana (activo, maximizado, fullscreen) sincronizando etiquetas accesibles.
   - `setWindowWidth(int)`: Ajuste dinámico de geometría ante cambios de tamaño de ventana con validación de umbrales mínimos.
   - `updateButtonState(TitleBarButtonType, TitleBarButtonState)`: Mutación ligera de estados interactivos de botones sin relayout completo.
3. **Persistencia local atómica y resiliencia ante corrupción**:
   - `saveStateToFile` / `loadStateFromFile`: Serialización y deserialización local determinista en formato estructurado.
   - Escritura atómica basada en archivo temporal `.tmp` y renombramiento seguro (`std::filesystem::rename`) para evitar archivos truncados.
   - Aislamiento absoluto de sesiones de incógnito (`setIncognitoMode(true)`): no se realiza ninguna escritura a disco cuando la ventana opera en modo privado.
   - Fallback de recuperación ante archivos corruptos: si el archivo de estado está dañado, el servicio restaura automáticamente `canonicalDefaultTitleBar()` sin arrojar excepciones ni colgar la aplicación.
4. **Soporte de cancelación cooperativa Zero-Zombie**:
   - Aceptación opcional de `CancellationToken*` en métodos de recálculo y persistencia, garantizando aborto inmediato si la tarea es cancelada.
5. **Observabilidad y eventos desacoplados**:
   - Callback de notificación `setTitleBarChangedCallback` que permite a la interfaz Qt en la futura etapa UI (`module.ux-shell.task0066`) reaccionar a cambios sin acoplamiento circular.
   - Formateo de diagnósticos y resúmenes de estado legibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
6. **Batería de pruebas unitarias en CTest**:
   - Target `vigilio_title_bar_service_tests` ejecutando 14 pruebas unitarias rápidas e independientes (< 100 ms).

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Capa de servicio de barra de título** | Inexistente; solo validador estático de dominio en `task0064` | `TitleBarService` en `vigilio_core` desacoplado de GUI | Implementar la clase de servicio con control de estado, mutex y métodos públicos de mutación. |
| **Transición de densidades** | Funciones canónicas aisladas en `TitleBarConfiguration` | Transición tipada reactiva en `TitleBarService::setDensity` | Implementar recálculo sistemático de restricciones dimensionales y botones según densidad. |
| **Conmutación de controles Left/Right** | Soportado analíticamente en `TitleBarValidator` | Método operativo `setControlsPlacement` con recálculo atómico | Exponer método thread-safe en el servicio y verificar persistencia del layout resultante. |
| **Persistencia de estado de barra** | Inexistente; no se guardan configuraciones ni modos | Persistencia atómica local en disco con protección de incógnito | Implementar serialización, guardado seguro `.tmp` y carga con saneamiento. |
| **Resiliencia ante archivos corruptos** | Inexistente | Detección de corrupción sintáctica y fallback a valores canónicos | Implementar captura de error de deserialización y restauración segura sin crashear. |
| **Aislamiento de navegación privada** | Inexistente en barra | Supresión estricta de persistencia cuando `isIncognitoMode() == true` | Implementar guardas en `saveStateToFile` y certificar con pruebas unitarias. |
| **Soporte de cancelación Zero-Zombie** | Inexistente en barra | Parámetro `CancellationToken*` en cálculos y operaciones de disco | Comprobar `isCancelled()` en puntos de control para abortar operaciones de forma segura. |
| **Pruebas unitarias de servicio** | 0 pruebas de servicio de barra de título | 14 pruebas unitarias exhaustivas en `TitleBarServiceTests.cpp` | Crear suite de pruebas unitarias registrada en CMake y CTest. |

---

## Compatibility, migration and rollback

### Compatibilidad y estándares C++20 / Qt6

- **Estándar C++20**:
  - Utilización rigurosa de características estándar de C++20: `<filesystem>`, `<mutex>`, `<functional>`, `<string>`, `<string_view>`, `<vector>`, `<memory>`, semántica de movimiento (`noexcept`) y `[[nodiscard]]`.
  - Cero dependencias de bibliotecas gráficas de Qt (`QWidget`, `QApplication`, `QMainWindow`) en `TitleBarService.h/.cpp`, asegurando compilación limpia y ejecución headless en cualquier entorno CLI o CI.
- **Banderas de compilación estrictas**:
  - Compilación limpia bajo GCC 15+, Clang 18+ y Apple Clang con `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror`.
- **Compatibilidad con la arquitectura modular de CMake**:
  - El servicio reside dentro de `vigilio_core`, disponible para ser consumido tanto por ejecutables de prueba como por la futura integración en `vigilio-browser` (`task0066`).

### Estrategia de migración

- La introducción de `TitleBarService` es puramente aditiva e incremental:
  - No altera el comportamiento de `BrowserWindow` existente ni introduce regresiones sobre `MainWindowLayoutService`.
  - Establece la base operativa que consumirá el widget de barra de título en la siguiente etapa (`ui`, microtarea `0066`).

### Plan de contingencia y rollback

- Si la implementación de `TitleBarService` presentara algún defecto:
  - Se puede revertir limpiamente el commit correspondiente (`feat(ux-shell): implementation title-bar-integration`).
  - Las capas previas de dominio de `task0064` y el layout de ventana principal de `task0060`-`task0063` permanecen intactos e inalterados.

---

## Arquitectura técnica detallada de la capa de servicio

### Modelo de estados y concurrencia

```
                      +---------------------------------------+
                      |           TitleBarService             |
                      |---------------------------------------|
                      | - config_: TitleBarConfig             |
                      | - plan_: TitleBarLayoutPlan           |
                      | - windowWidth_: int                   |
                      | - options_: TitleBarServiceOptions    |
                      | - callback_: OnTitleBarChangedCallback|
                      | - mutex_: mutable std::mutex          |
                      +---------------------------------------+
                                          |
                      +-------------------+-------------------+
                      |                   |                   |
                      v                   v                   v
               [ Mutaciones ]      [ Persistencia ]     [ Consultas ]
               - setDensity()      - saveToFile()       - hitTest(x, y)
               - setPlacement()    - loadFromFile()     - plan()
               - setTitle()        - restoreDefault()   - config()
               - setWindowState()  - incognito guard    - computeLayout()
               - updateButton()
```

- **Thread-Safety**: Todos los métodos públicos que leen o mutan el estado interno adquieren un `std::lock_guard<std::mutex>` o `std::unique_lock<std::mutex>` sobre `mutex_`.
- **Notificación asíncrona segura**: Las llamadas al callback registrado `callback_` se efectúan transmitiendo copias inmutables del plan y el reporte, evitando condiciones de carrera en el receptor.

### Mecanismo de persistencia local y resiliencia

1. **Estructura del archivo de estado local**:
   - Claves normalizadas: `version` (1), `mode` (`custom_integrated` / `native_system` / `suppressed`), `placement` (`right` / `left`), `title` (cadena sanitizada), `show_icon` (booleano), `show_senior_toggle` (booleano), `density` (`default` / `senior` / `compact`).
2. **Escritura atómica**:
   - Se serializa a un archivo temporal `<ruta>.tmp`.
   - Se realiza `std::filesystem::rename` atómico sobre el archivo de destino final.
3. **Recuperación ante fallos**:
   - Si el archivo no existe, está dañado o contiene valores inválidos, se inicializa automáticamente con `TitleBarConfiguration::canonicalDefaultTitleBar()`, asegurando que el navegador siempre arranque en un estado visual coherente.
4. **Modo incógnito**:
   - `saveStateToFile` verifica `isIncognitoMode()` antes de cualquier operación de E/S; si está activo, aborta inmediatamente retornando `false`.
