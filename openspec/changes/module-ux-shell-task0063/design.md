# Design — module-ux-shell-task0063

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del subsistema `main window layout` tras las etapas precedentes (`task0060`, `task0061`, `task0062`)**:
   - `module.ux-shell.task0060` (commit `4cb5d8f`) formalizó el modelo de dominio en `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration.h/.cpp` y `MainWindowLayoutValidator.h/.cpp`, con 12 pruebas unitarias básicas de invariantes y cálculo analítico de distribución espacial.
   - `module.ux-shell.task0061` (commit `c4d75c0`) implementó la capa operativa desacoplada en `MainWindowLayoutService.h/.cpp`, proveyendo gestión multihilo del plan (`std::mutex`), conmutación de densidades (`setDensity`), control de estados de barra lateral (`toggleSidebar`, `setSidebarWidth`), persistencia atómica con tolerancia a fallos (`saveLayoutStateToFile`, `loadLayoutStateFromFile`), aislamiento de incógnito y cancelación cooperativa Zero-Zombie (`CancellationToken`), respaldado por 14 pruebas unitarias de servicio.
   - `module.ux-shell.task0062` (commit `d0e3fec`) integró el servicio de layout con la ventana principal de Qt Widgets (`BrowserWindow.h/.cpp`), estructurando el panel lateral (`vigilio-sidebar`), la captura reactiva de eventos de redimensionamiento (`resizeEvent`), la conmutación a la «Interfaz más fácil de usar» con controles ampliados $\ge 48$ px y un arnés inicial de verificación en `WindowTests.cpp`.
2. **Limitaciones de cobertura en el arnés de pruebas actual**:
   - *Fronteras y casos negativos incompletos en el validador*:
     - No existen pruebas unitarias sistemáticas que exploren las fronteras exactas de dimensiones de ventana sub-mínimas (e.g. $639\times 480$, $719\times 520$, $499\times 380$), dimensiones no positivas ($\le 0$) o negativas.
     - No se validan exhaustivamente todos los rangos prohibidos de anchura de barra lateral (rango inferior $0..47$ px, zona prohibida intermedia $49..199$ px, o exceso $> 360$ px).
     - No se prueban sistemáticamente presupuestos corruptos con alturas o márgenes negativos (`NegativeMarginOrPadding`) ni sobreasignaciones de altura que provoquen `ZeroContentAreaBudget`.
   - *Ausencia de pruebas de estrés bajo concurrencia masiva (16 hilos)*:
     - Las pruebas de servicio existentes evalúan concurrencia básica con 8 hilos ejecutando operaciones homogéneas, pero no someten la instancia compartida a estrés mixto masivo (16 hilos ejecutando simultáneamente redimensionamiento, conmutación de densidad, mutación de sidebar y consultas de plan).
   - *Pruebas de fuzzing y recuperación de persistencia limitadas*:
     - Solo se evalúa un caso básico de archivo JSON corrupto; falta evaluar inyecciones sistemáticas de JSON truncado, JSON con tipos dispares (strings en dimensiones), JSON con violaciones geométricas deliberadas y simulación de fallos de disco por permisos de solo lectura (`perms::read_only`).
   - *Aislamiento estricto de navegación privada*:
     - Se requiere certificar formalmente que en modo incógnito ni la lectura ni la escritura tocan el sistema de archivos local bajo ninguna circunstancia.

---

### Estado objetivo (TARGET)

1. **Suite integral de pruebas de dominio y fronteras en `MainWindowLayoutTests.cpp`**:
   - Cobertura exhaustiva de todos los códigos de error definidos en `MainWindowLayoutError`:
     - `DimensionsBelowMinimum` verificado en los límites exactos para `Default`, `Senior` y `Compact`.
     - `InvalidSidebarWidth` verificado en todas las regiones dimensionales prohibidas.
     - `NegativeMarginOrPadding` verificado ante márgenes negativos en `LayoutMargins`.
     - `ZeroContentAreaBudget` verificado ante sobreasignaciones fijas de zonas.
     - `InvalidDensityConfiguration` verificado ante identificadores de densidad no canónicos.
   - Cálculo analítico de áreas de layout (`calculateLayoutAreas`) comprobado bajo geometrías de pantalla extremas (ultrawide $3840\times 1080$, vertical $1080\times 1920$, alta resolución $2560\times 1440$ y $3840\times 2160$), certificando la invariante fundamental de no solapamiento espacial ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y garantía de área de contenido positiva.
   - Auditoría programática de todas las cadenas de texto y pistas de remediación (`layoutZoneToString`, `layoutDensityToString`, `sidebarStateToString`, `sidebarDockPositionToString`, `layoutErrorToString`, `generateRemediationHint`), garantizando el cumplimiento estricto de `docs/UX-COPY-PUBLICO.md` en español neutro sin términos internos proscritos.
2. **Suite integral de pruebas operativas, resiliencia y concurrencia en `MainWindowLayoutServiceTests.cpp`**:
   - Prueba de estrés multihilo de alta concurrencia con 16 hilos (`std::thread`) concurrentes ejecutando operaciones heterogéneas continuas (redimensionamiento, cambio de densidad, alternancia de sidebar y consultas de reporte), validando la ausencia absoluta de carreras de datos, interbloqueos y corrupción de memoria.
   - Prueba de cancelación cooperativa multihilo mediante `CancellationToken` disparado en instantes asíncronos arbitrarios, verificando respuesta inmediata (< 10 ms), preservación del estado previo y ausencia de recursos huérfanos.
   - Batería de pruebas de recuperación y resiliencia ante corrupción de archivos de layout:
     - Inyección de JSON truncado.
     - Inyección de JSON con campos de tipos dispares.
     - Inyección de JSON con valores geométricos inválidos deliberados.
     - Fallos simulados de escritura en disco por directorios de solo lectura (`std::filesystem::perms::read_only`).
     - Verificación del restablecimiento transparente del plan canónico por defecto (`canonicalDefaultLayout()`) y reporte de `LayoutPersistenceCorrupted` sin caídas de proceso.
   - Verificación de aislamiento absoluto en modo incógnito: llamadas a persistencia abortadas inmediatamente con cero operaciones de I/O en disco.
   - Validación de soporte para anclaje lateral derecho (`SidebarDockPosition::Right`) y combinaciones arbitrarias de visibilidad de zonas opcionales (`NotificationBanner`, `StatusBar`).
3. **Arnés de integración de interfaz gráfica en `WindowTests.cpp`**:
   - Validación de la integración reactiva entre `BrowserWindow` y `MainWindowLayoutService`:
     - Verificación de que `resizeEvent` sincroniza dimensiones con el servicio.
     - Verificación de que la activación de la «Interfaz más fácil de usar» impone dimensiones mínimas $\ge 720\times 520$ px y tap targets $\ge 48$ px.
     - Verificación de tolerancia headless determinista (reportando `NOT_EXECUTED` limpiamente si `DISPLAY`/`WAYLAND_DISPLAY` está ausente).
     - Auditoría estricta de textos accesibles y cadenas visibles frente a `docs/UX-COPY-PUBLICO.md`.

---

### Brechas identificadas (GAPS)

| Componente de prueba | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Fronteras dimensionales** | Pruebas básicas de mínimos generales | Pruebas de límites exactos ($N-1$ y $N$) para Default, Senior y Compact | Añadir casos de frontera exacta en `MainWindowLayoutTests.cpp`. |
| **Anchuras de barra lateral** | Verificación de 48 px y 240 px | Cobertura de todas las regiones inválidas ($0..47$, $49..199$, $>360$) | Añadir casos negativos exhaustivos de `validateSidebar`. |
| **Sobreasignación presupuestaria** | No se evalúa el agotamiento de presupuesto | Verificación de `ZeroContentAreaBudget` cuando zonas fijas $\ge$ altura | Añadir caso de prueba de sobreasignación en `MainWindowLayoutTests.cpp`. |
| **Geometrías extremas** | Solo se prueba resolución $1200\times 780$ | Pruebas en ultrawide, formato vertical y pantallas de alta resolución | Añadir cálculos espaciales en resoluciones atípicas. |
| **Concurrencia multihilo** | Concurrencia básica con 8 hilos | Concurrencia de alto estrés con 16 hilos ejecutando operaciones mixtas | Añadir prueba masiva de 16 hilos en `MainWindowLayoutServiceTests.cpp`. |
| **Fuzzing de persistencia** | Caso simple de JSON corrupto | Inyección de JSON truncado, tipos dispares y violaciones geométricas | Añadir batería de fuzzing y recuperación de JSON. |
| **Simulación de fallos de disco** | No se simulan permisos denegados | Simulación de directorios de solo lectura y fallos en escritura atómica | Añadir pruebas de manejo de errores de I/O en persistencia. |
| **Aislamiento en incógnito** | Prueba funcional básica | Certificación estricta de cero operaciones de I/O en disco durante incógnito | Añadir aserciones de no interacción con archivos en disco. |
| **Anclaje lateral derecho** | Solo se evalúa anclaje izquierdo canónico | Validación de cálculo geométrico con `SidebarDockPosition::Right` | Añadir pruebas de distribución espacial con anclaje a la derecha. |
| **Integración UI y eventos** | Pruebas estructurales básicas de widgets | Verificación de redimensionamiento reactivo y tap targets $\ge 48$ px | Refinar aserciones en `tests/ui/WindowTests.cpp`. |

---

## Compatibility, migration and rollback

### Compatibilidad arquitectónica y estándares C++20

- **Desacoplamiento limpio entre capas**:
  - `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration` y `MainWindowLayoutValidator` (`src/core/`) no dependen de bibliotecas gráficas ni de Qt.
  - `MainWindowLayoutService` (`src/core/`) opera exclusivamente con tipos estándar de C++20 (`std::mutex`, `std::unique_ptr`, `std::vector`, `std::filesystem`), permitiendo su ejecución ultrarrápida y determinista en cualquier plataforma compatible.
  - Las suites de pruebas unitarias (`MainWindowLayoutTests.cpp` y `MainWindowLayoutServiceTests.cpp`) se ejecutan sin dependencias de servidor gráfico y completan su verificación en < 1 segundo en CTest.
- **Conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - Todas las operaciones de prueba se confinan a memoria o a directorios temporales locales gestionados mediante RAII, garantizando limpieza automática y cero dependencias de conectividad de red o servicios remotos.

### Modelo de pruebas y concurrencia multihilo

```
                               ┌────────────────────────────────┐
                               │ MainWindowLayoutService (inst) │
                               │  Protegido por std::mutex      │
                               └────────────────┬───────────────┘
                                                │
         ┌───────────────────┬──────────────────┴────────────────┬───────────────────┐
         ▼                   ▼                                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐                 ┌─────────────────┐ ┌─────────────────┐
│ Hilos 1-4:      │ │ Hilos 5-8:      │                 │ Hilos 9-12:     │ │ Hilos 13-16:    │
│ resizeWindow()  │ │ setDensity()    │                 │ toggleSidebar() │ │ Consultas:      │
│ (mutación dim.) │ │ (conmutación)   │                 │ (mutación bar)  │ │ plan(), rep()   │
└─────────────────┘ └─────────────────┘                 └─────────────────┘ └─────────────────┘
         │                   │                                   │                   │
         └───────────────────┴──────────────────┬────────────────┴───────────────────┘
                                                │
                                                ▼
                               ┌────────────────────────────────┐
                               │ Verificación: Cero data races, │
                               │ estado válido y terminación    │
                               └────────────────────────────────┘
```

- Las pruebas multihilo lanzan 16 hilos que ejecutan concurrentemente operaciones dispares contra la misma instancia del servicio. Cada hilo ejecuta un bucle de N iteraciones con variaciones deterministas, y al finalizar se verifica la consistencia global del estado resultante y la validez del reporte del layout.

### Mecanismos de recuperación y resiliencia ante fallos

1. **Corrupción de archivos de persistencia**:
   - `MainWindowLayoutService::loadLayoutStateFromFile` analiza el contenido del archivo con validación defensiva:
     - Si el archivo no existe, está vacío o contiene JSON sintácticamente roto, se captura la condición, se asigna `MainWindowLayoutError::LayoutPersistenceCorrupted`, y se restaura inmediatamente el plan canónico por defecto (`canonicalDefaultLayout()`).
     - Si el JSON es sintácticamente válido pero contiene valores geométricos que violan los invariantes (e.g. dimensiones sub-mínimas o anchuras de sidebar inválidas), el validador estático rechaza el plan y el servicio restablece el estado seguro.
   - En ningún caso una corrupción en disco produce una caída de la aplicación, una excepción no controlada ni un estado de layout incoherente.
2. **Fallos de escritura en disco**:
   - `saveLayoutStateToFile` utiliza un archivo temporal intermedio (`.tmp`) en el mismo directorio y realiza un reemplazo atómico (`std::filesystem::rename`). Si la escritura del archivo temporal falla (e.g. por permisos de solo lectura o espacio agotado), el archivo de destino original permanece intacto y la función retorna el código de error correspondiente sin corromper la configuración previa.

### Política Zero-Zombie y límites de ejecución

- Todas las pruebas de CTest implementan límites de tiempo acotados (`TIMEOUT 15` para pruebas unitarias, `TIMEOUT 20` para pruebas de UI).
- El uso de `CancellationToken` garantiza que las operaciones que admitan interrupción cooperativa finalicen de inmediato (< 10 ms) tras la activación de la señal, sin dejar subprocesos huérfanos ni bucles de espera indefinidos.

### Estrategia de migración y reversión (Rollback)

- Dado que este pack corresponde a la etapa **tests**, los cambios se limitan estrictamente a los archivos de prueba en `apps/vigilio-browser/tests/**` y a la sincronización documental en `docs/rules/**`.
- La base de código de producción (`apps/vigilio-browser/src/**`) no sufre alteraciones que comprometan la compatibilidad hacia atrás ni las interfaces públicas ya establecidas.
- Si cualquier prueba revelara una discrepancia o fallo inesperado durante la ejecución de APPLY, el cambio puede revertirse de forma atómica y segura mediante `git checkout` sobre las rutas de prueba modificadas, sin riesgo alguno de degradación funcional o pérdida de datos.
