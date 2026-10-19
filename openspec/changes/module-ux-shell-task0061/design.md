# Design — module-ux-shell-task0061

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del árbol de código y finalización de la etapa de dominio (`task0060`)**:
   - La microtarea `module.ux-shell.task0060` (commit `4cb5d8f`) formalizó la etapa `domain` de la capacidad `main window layout`, aportando:
     - `MainWindowLayoutTypes.h`: Enumeraciones fuertemente tipadas (`LayoutZone`, `LayoutDensity`, `SidebarState`, `SidebarDockPosition`, `WindowChromeMode`, `MainWindowLayoutError`) y estructuras inmutables (`WindowDimensions`, `LayoutMargins`, `LayoutRect`, `ZoneBudget`, `SidebarConfig`, `MainWindowLayoutPlan`, `ZoneGeometry`, `MainWindowLayoutReport`).
     - `MainWindowLayoutConfiguration.h/.cpp`: Constantes canónicas (dimensiones por defecto $1200\times 780$, mínimos estándar $640\times 480$, mínimos Senior $720\times 520$, mínimos Compact $500\times 380$), factorías canónicas y verificación de invariantes (`verifyInvariants`).
     - `MainWindowLayoutValidator.h/.cpp`: Métodos analíticos puros `validateDimensions`, `validateSidebar`, `validatePlan`, cálculo analítico de distribución espacial `calculateLayoutAreas` y formateo público en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
     - `tests/unit/MainWindowLayoutTests.cpp`: 12 pruebas unitarias en CTest ejecutándose limpiamente al 100%.
2. **Limitaciones de la capa de aplicación y widgets actual (`BrowserWindow`)**:
   - `apps/vigilio-browser/src/ui/BrowserWindow.cpp` continúa gestionando el layout de la ventana de forma directa y monolítica:
     - Crea y posiciona widgets (`QTabWidget`, `QToolBar`, banners condicionales) mediante layouts estáticos de Qt (`QVBoxLayout`).
     - Modifica manualmente tamaños de iconos y estilos al activar la «Interfaz más fácil de usar» (`setSeniorMode`).
     - No dispone de una capa de servicio intermedia que mantenga el plan de layout en memoria, gestione las transiciones de estado, controle la barra lateral de manera desacoplada ni persista las preferencias de ventana en disco de manera segura y atómica.
3. **Ausencia de persistencia y soporte de cancelación en la capa de layout**:
   - No existe un mecanismo en `vigilio_core` que guarde o recupere el estado del layout (dimensiones, posición de la barra lateral, densidad activa) en un archivo local sin comprometer la política Local-First o los datos privados en modo incógnito.
   - No existe integración con tokens de cancelación cooperativa (`CancellationToken`) bajo la política Zero-Zombie para abortar operaciones de layout si la ventana o la aplicación se cierran mientras se computa una reconfiguración pesada.

---

### Estado objetivo (TARGET)

1. **Servicio operativo desacoplado `MainWindowLayoutService` en `vigilio_core`**:
   - Una clase propietaria C++20 thread-safe (`std::mutex`), completamente aislada de dependencias de interfaz gráfica (`QWidget`, `QMainWindow`), que actúa como la autoridad operativa sobre el estado del layout de la ventana principal.
   - Encapsula un `MainWindowLayoutPlan` inmutable que evoluciona mediante transiciones validadas.
2. **Orquestación dinámica de estados y densidades**:
   - `setDensity(LayoutDensity)`: Transición limpia entre densidad estándar, «Interfaz más fácil de usar» (Modo Senior) y compacta, recalculando y aplicando restricciones dimensionales y presupuestos de tap targets.
   - `resizeWindow(int width, int height)`: Saneamiento y validación estricta de dimensiones antes de aceptar el cambio de tamaño.
   - `toggleSidebar()` / `setSidebarWidth(int)` / `setSidebarDockPosition(SidebarDockPosition)`: Control determinista de la barra lateral respetando los límites de diseño de `/resultado/01-vigilio-home.png` (48 px colapsada, $200..360$ px expandida).
   - Control de visibilidad de avisos (`showNotificationBanner`) y barra de estado (`showStatusBar`).
3. **Persistencia local atómica y resiliencia ante corrupción**:
   - `saveLayoutStateToFile` / `loadLayoutStateFromFile`: Serialización y deserialización local determinista.
   - Escritura atómica basada en archivo temporal `.tmp` y renombramiento seguro para prevenir corrupción ante interrupciones.
   - Aislamiento absoluto de sesiones de incógnito (`setIncognitoMode(true)`): no se realiza ninguna escritura a disco cuando la ventana opera en modo privado.
   - Fallback de recuperación ante archivos corruptos o ilegibles: si el archivo de estado está dañado, el servicio restaura automáticamente `canonicalDefaultLayout()` emitiendo un código recuperable (`LayoutPersistenceCorrupted`) sin arrojar excepciones ni colgar la aplicación.
4. **Soporte de cancelación cooperativa Zero-Zombie**:
   - Parámetro opcional `CancellationToken*` en métodos de cálculo analítico y persistencia, garantizando detención inmediata si la tarea es cancelada.
5. **Observabilidad y eventos desacoplados**:
   - Callback de notificación `setLayoutChangedCallback` que permite a la capa UI (futura `task0062`) reaccionar a los cambios de layout sin acoplamiento circular.
   - Generación de resúmenes de estado legibles en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
6. **Batería de pruebas unitarias en CTest**:
   - Target `vigilio_main_window_layout_service_tests` ejecutando 14 pruebas unitarias rápidas e independientes (< 100 ms).

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Capa de orquestación de layout** | Inexistente; `BrowserWindow.cpp` maneja widgets directamente | `MainWindowLayoutService` en `vigilio_core` desacoplado de GUI | Implementar la clase de servicio con control de estado, mutex y métodos públicos de mutación. |
| **Transición de densidades** | Modificación dispersa de estilos en `BrowserWindow::setSeniorMode` | Transición de densidad tipada en `MainWindowLayoutService::setDensity` | Implementar recálculo sistemático de límites dimensionales y presupuestos por densidad. |
| **Gestión de barra lateral** | Inexistente; sin soporte en la ventana actual | Máquina de estados completa para Sidebar (toggle, ancho $200..360$, dock Left/Right) | Implementar control de estados de la barra lateral en el servicio. |
| **Persistencia de estado de ventana** | Inexistente; no se guardan dimensiones ni preferencias | Persistencia atómica local en archivo con protección de incógnito | Implementar serialización, guardado temporal/atómico y carga con saneamiento. |
| **Recuperación ante fallos de disco** | N/A | Detección de corrupción y fallback automático al plan canónico | Implementar captura de error de parseo y restauración resiliente a `canonicalDefaultLayout()`. |
| **Soporte de cancelación Zero-Zombie** | Inexistente en layout | Aceptación de `CancellationToken*` en cálculos y E/S de disco | Integrar consultas a `isCancelled()` en los métodos del servicio. |
| **Pruebas unitarias de servicio** | 0 pruebas de servicio de layout | 14 pruebas unitarias exhaustivas en `MainWindowLayoutServiceTests.cpp` | Crear suite de pruebas unitarias registrada en CMake y CTest. |

---

## Compatibility, migration and rollback

### Compatibilidad y estándares C++20 / Qt6

- **Estándar C++20**:
  - Utilización rigurosa de características estándar de C++20: `<filesystem>`, `<mutex>`, `<functional>`, `<string>`, `<string_view>`, `<vector>`, `<memory>`, semántica de movimiento (`noexcept`) y `[[nodiscard]]`.
  - Cero dependencias de bibliotecas gráficas de Qt (`QWidget`, `QApplication`, `QMainWindow`) en `MainWindowLayoutService.h/.cpp`, asegurando compilación y ejecución headless en cualquier entorno CLI o CI.
- **Banderas de compilación estrictas**:
  - Compilación limpia bajo GCC 15+, Clang 18+ y Apple Clang con `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror`.
- **Compatibilidad con la arquitectura modular de CMake**:
  - El servicio reside dentro de `vigilio_core`, disponible para ser consumido tanto por ejecutables de prueba como por el ejecutable principal `vigilio-browser`.

### Estrategia de migración

- La introducción de `MainWindowLayoutService` es puramente aditiva e incremental:
  - No altera el comportamiento observable de la aplicación en ejecución.
  - Prepara la base arquitectónica limpia para que la siguiente microtarea (`module.ux-shell.task0062`) pueda enlazar `BrowserWindow` con `MainWindowLayoutService` sin necesidad de refactorizar la lógica de negocio.

### Plan de contingencia y rollback

- Si la implementación de `MainWindowLayoutService` presentara algún defecto durante la integración o pruebas:
  - Se puede revertir limpiamente el commit correspondiente (`feat(ux-shell): implementation main-window-layout`).
  - La ventana existente `BrowserWindow` y los tipos de dominio de `task0060` permanecen intactos y funcionando de forma independiente.

---

## Arquitectura técnica detallada de la capa de servicio

### Modelo de estados y concurrencia

```
                      +---------------------------------------+
                      |       MainWindowLayoutService         |
                      |---------------------------------------|
                      | - plan_: MainWindowLayoutPlan         |
                      | - options_: Options                   |
                      | - callback_: OnLayoutChangedCallback  |
                      | - mutex_: mutable std::mutex          |
                      +---------------------------------------+
                                          |
                      +-------------------+-------------------+
                      |                   |                   |
                      v                   v                   v
              [ Mutaciones ]      [ Persistencia ]     [ Cálculos ]
              - setDensity()      - saveToFile()       - computeLayout()
              - resizeWindow()    - loadFromFile()     - calculateAreas()
              - toggleSidebar()   - restoreDefault()   - validation
              - setZoneVisible()
```

- **Thread-Safety**: Todos los métodos públicos que leen o mutan `plan_` u `options_` adquieren un `std::lock_guard<std::mutex>` o `std::unique_lock<std::mutex>` sobre `mutex_`.
- **Notificación asíncrona segura**: Las llamadas al callback registrado `callback_` se efectúan dentro o inmediatamente después de la mutación, garantizando que el observador recibe una copia consistente e inmutable del reporte y el plan.

### Mecanismo de persistencia local y resiliencia

1. **Estructura del archivo de estado local**:
   - El estado de la ventana se almacena localmente en un formato estructurado y determinista:
     ```json
     {
       "schema_version": 1,
       "density": "Default",
       "window": {
         "width": 1200,
         "height": 780
       },
       "sidebar": {
         "state": "Collapsed",
         "position": "Left",
         "width": 48
       },
       "zones": {
         "showTitleBar": false,
         "showTabStrip": true,
         "showToolBar": true,
         "showBanner": false,
         "showSidebar": true,
         "showStatusBar": true
       }
     }
     ```
2. **Escritura atómica**:
   - `saveLayoutStateToFile(targetPath)`:
     - Comprueba si `isIncognitoSession == true`. Si es así, retorna `false` inmediatamente sin tocar el disco.
     - Escribe los datos serializados en `targetPath.string() + ".tmp"`.
     - Vuelca buffers a disco (`flush`).
     - Renombra atómicamente el archivo temporal al destino definitivo mediante `std::filesystem::rename`.
3. **Carga y tolerancia a fallos**:
   - `loadLayoutStateFromFile(sourcePath)`:
     - Verifica existencia y permisos del archivo.
     - Lee el contenido y analiza los campos requeridos.
     - Si el archivo no existe, está vacío, contiene sintaxis inválida o declara dimensiones inferiores a los mínimos de la densidad indicada, el servicio no lanza excepciones: restaura el plan canónico `canonicalDefaultLayout()` y retorna `false`, notificando el error recuperable `MainWindowLayoutError::LayoutPersistenceCorrupted`.

### Integración con el validador de dominio

- Toda mutación que altera la geometría invoca a `MainWindowLayoutValidator::calculateLayoutAreas` o las funciones de validación específicas (`validateDimensions`, `validateSidebar`).
- Si una mutación viola las invariantes (por ejemplo, reducir el área neta de contenido web por debajo de $400\times 250$ px o intentar dimensionar una ventana en densidad Senior por debajo de $720\times 520$ px), la mutación es rechazada y el servicio preserva el estado válido previo.

### Observabilidad y UX Copy

- En estricto apego a `docs/UX-COPY-PUBLICO.md`, el servicio formatea diagnósticos y resúmenes utilizando los términos oficiales aprobados:
  - Densidad `Senior`: «Interfaz más fácil de usar».
  - Densidad `Default`: «Estándar».
  - Densidad `Compact`: «Compacta».
- Ningún mensaje de error o diagnóstico emitido por el servicio expondrá términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).
