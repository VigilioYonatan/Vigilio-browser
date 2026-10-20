# Design — module-ux-shell-task0068

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del subsistema de pestañas y shell tras las etapas precedentes (`task0001`-`task0067`)**:
   - `module.ux-shell.task0060`-`task0063` formalizó, implementó e integró el diseño estructural de la ventana principal (**main window layout**), introduciendo la zona `LayoutZone::TabStrip` y la enumeración `LayoutDensity` (`Compact`, `Default`, `Senior`).
   - `module.ux-shell.task0064`-`task0067` formalizó, implementó e integró la barra de título personalizada (**title bar integration**), certificando 29 pruebas unitarias y de integración que validan distribución espacial analítica, concurrencia multihilo, hit-testing de precisión, adaptación a la «Interfaz más fácil de usar» y directrices públicas de `docs/UX-COPY-PUBLICO.md`.
   - En `apps/vigilio-browser/src/core/TabModel.h` y `TabModel.cpp` existe un modelo de datos elemental (`Tab`) que mantiene una colección secuencial de pestañas con atributos básicos (`id`, `url`, `pinned`, `muted`) y operaciones elementales (`open`, `close`, `setPinned`, `setMuted`, `duplicate`, `closeOthers`).
   - En `apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`, la interfaz gráfica contiene un puntero `QTabWidget* tabs_` que delega el renderizado y dimensionamiento de pestañas al estilo predeterminado del motor de estilos de Qt, sin soporte paramétrico de densidades de escritorio ni adaptación a Modo Senior.
2. **Deficiencias y limitaciones del estado actual**:
   - *Ausencia de modelo de dominio geométrico y espacial*:
     - No existe una separación entre el modelo de pestañas y su representación espacial analítica.
     - No existen tipos de datos inmutables que expresen anchos de pestañas fijadas, anchos de pestañas regulares, alturas por densidad, márgenes de contención ni rectángulos de componentes internos (favicon, título, botón de cierre, indicador de audio).
   - *Inexistencia de algoritmo determinista de compresión elástica y desbordamiento*:
     - Cuando el número de pestañas crece o el ancho de la ventana disminuye, no se calcula de forma analítica el ancho de compresión elástica entre `preferredTabWidth` y `minTabWidth`.
     - No se controla el estado de desbordamiento horizontal (*overflow*) ni se definen modos de navegación por desplazamiento (*scrollable*) o menú de lista (*dropdown*).
     - No se implementa la regla de protección que oculta el botón de cierre en pestañas inactivas angostas para prevenir cierres involuntarios.
   - *Falta de adaptación a la «Interfaz más fácil de usar» (Modo Senior)*:
     - El componente actual no garantiza una altura mínima de 48 px ni tap targets $\ge 48\times 48$ px para el botón de nueva pestaña, botones de cierre y pestañas fijadas, incumpliendo el estándar WCAG 2.1 AAA y la directriz de `docs/skills/05-accesibilidad-y-senior-mode.md`.
   - *Inexistencia de clasificación analítica de impacto (Hit-Testing desacoplado)*:
     - No se pueden probar de forma unitaria las coordenadas de clic en pestañas, favicons, títulos, botones de cierre o controles de audio sin instanciar un servidor gráfico o simular eventos `QMouseEvent` de Qt.
   - *Carencia de textos y errores accesibles en español neutro*:
     - No existe un vocabulario tipado de anomalías (`TabStripDensityError`) ni formateadores de accesibilidad conformes con `docs/UX-COPY-PUBLICO.md`.

---

### Estado objetivo (TARGET)

1. **Modelo de dominio matemático y fuertemente tipado en C++20 puro (`TabStripDensityTypes.h`)**:
   - Enumeraciones completas con semántica explícita: `TabPinState`, `TabMediaState`, `TabDiscardState`, `TabLoadingState`, `TabInteractiveState`, `TabStripHitTestZone`, `TabStripOverflowMode`, `TabStripDensityError`.
   - Estructuras inmutables con semántica de valor:
     - `TabStripDimensions`: Constantes y parámetros dimensionales.
     - `TabSpec`: Entrada declarativa por pestaña.
     - `TabItemLayout`: Salida calculada con rectángulos geométricos `LayoutRect` para la pestaña, favicon, título, botón de cierre e indicador de audio.
     - `TabStripDensityConfig`: Parámetros declarativos del cálculo.
     - `TabStripDensityPlan`: Plan integral inmutable consolidando rectángulos de contenedor, pestañas, botón nueva pestaña y controles de scroll/desbordamiento.
     - `TabStripDensityReport`: Reporte de diagnóstico con error tipado, sugerencia de remediación y plan resultante.
2. **Configuración canónica e invariantes estrictas (`TabStripDensityConfiguration`)**:
   - Factorías estáticas canónicas: `canonicalDefaultDensity()`, `canonicalSeniorDensity()`, `canonicalCompactDensity()`.
   - Verificador estático de invariantes `verifyInvariants(const TabStripDensityPlan&)` comprobando:
     - No solapamiento entre pestañas ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$).
     - No solapamiento interno entre favicon, título y botón de cierre.
     - Pestañas fijadas ubicadas estrictamente a la izquierda de las regulares ($x_{\text{pinned}} < x_{\text{regular}}$).
     - Altura $\ge 48$ px y tap targets $\ge 48\times 48$ px en la «Interfaz más fácil de usar».
     - Separación protectora de 4 px entre pestañas en Modo Senior.
3. **Validador estático desacoplado, algoritmo de cálculo y hit-testing analítico (`TabStripDensityValidator`)**:
   - Cálculo determinista del ancho de pestañas: compresión elástica entre $W_{\text{min}}$ y $W_{\text{max}}$, preservación fija de pestañas ancladas, y activación de desbordamiento horizontal cuando $W_{\text{ideal}} < W_{\text{min}}$.
   - Clasificación espacial de coordenadas $(x, y)$ a zonas discretas (`TabStripHitTestZone`).
   - Utilidades de texto público y accesibilidad en español neutro cumpliendo al 100% `docs/UX-COPY-PUBLICO.md`.
   - Sanitización de títulos de pestañas para protección de privacidad en sesiones privadas / incógnito.
4. **Suite de pruebas unitarias headless en CTest (`TabStripDensityTests.cpp`)**:
   - 16 pruebas unitarias ejecutadas en < 50 ms sin dependencias de GUI ni WebEngine, garantizando compilación con `-Werror` y cobertura total de casos normales, bordes, compresión elástica, fijación, desbordamiento, estados multimedia, ahorro de memoria y copy público.

---

### Brechas identificadas (GAPS)

| Componente | Estado actual (CURRENT) | Estado objetivo (TARGET) | Brecha a subsanar (GAP) |
| --- | --- | --- | --- |
| **Tipos de dominio de tira** | Solo existe `struct Tab` elemental en `TabModel.h` | Taxonomía formal completa en `TabStripDensityTypes.h` | Crear `TabStripDensityTypes.h` con enums, estructuras de semántica de valor inmutable y reporte. |
| **Configuración canónica** | No existe configuración ni factorías para pestañas | `TabStripDensityConfiguration` con factorías para Estándar, Senior y Compacto | Implementar constantes canónicas, factorías y método de invariantes en `TabStripDensityConfiguration.h/.cpp`. |
| **Algoritmo de compresión elástica** | Delegado opacamente al widget de Qt | Algoritmo analítico determinista $O(N)$ con compresión y desbordamiento | Implementar `calculateTabStripLayout` en `TabStripDensityValidator.cpp`. |
| **Hit-testing analítico** | Delegado a eventos nativos de Qt | Función pura `hitTest(x, y, plan)` que clasifica coordenadas en zonas | Implementar `hitTest` en `TabStripDensityValidator.cpp`. |
| **«Interfaz más fácil de usar»** | Pestañas genéricas sin garantía de tamaño accesible | Altura $\ge 48$ px, tap targets $\ge 48\times 48$ px, separación 4 px | Formalizar invariantes y factoría `canonicalSeniorDensity`. |
| **Ocultación de botón de cierre** | Siempre presente o según estilo nativo de Qt | Ocultación condicional en pestañas inactivas con ancho $< 90$ px | Regla analítica en `calculateTabStripLayout` respetando permanencia en Modo Senior. |
| **Estados de audio y ahorro de memoria** | No representados espacialmente | Zonas analíticas para altavoz y etiquetas de «Pestaña en reposo» | Modelar `TabMediaState` y `TabDiscardState` en `TabItemLayout` y funciones de accesibilidad. |
| **Copy público y privacidad** | Sin validación de textos accesibles en pestañas | Conformidad estricta con `docs/UX-COPY-PUBLICO.md` y sanitización | Implementar funciones de formateo público y sanitización en `TabStripDensityValidator.cpp`. |
| **Suite de pruebas unitarias** | Cero pruebas unitarias de tira de pestañas en CTest | 16 pruebas unitarias bajo `vigilio_tab_strip_density_tests` | Crear `tests/unit/TabStripDensityTests.cpp` y registrarlo en `CMakeLists.txt`. |

---

## Compatibility, migration and rollback

### Compatibilidad arquitectónica y estándares C++20

- **Desacoplamiento limpio entre capas**:
  - `TabStripDensityTypes.h`, `TabStripDensityConfiguration` y `TabStripDensityValidator` residen íntegramente en `apps/vigilio-browser/src/core/` y pertenecen a la biblioteca estática `vigilio_core`.
  - No incluyen ninguna cabecera de Qt Widgets (`QWidget`, `QTabBar`, `QTabWidget`), Qt Core ni Chromium WebEngine.
  - Se garantiza compatibilidad total con compiladores modernos de C++20 (GCC 11+, Clang 14+, MSVC 2022) con flags estrictas `-Wall -Wextra -Wpedantic -Werror`.
- **Conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001)**:
  - Todo el cálculo espacial se ejecuta puramente en memoria, sin interacción con red, servidores remotos o almacenamiento cloud.
- **Eficiencia de memoria y tiempo de CPU**:
  - La asignación de geometría analítica se realiza en una sola pasada $O(N)$ lineal respecto al número de pestañas $N$.
  - Las estructuras utilizan semántica de valor inmutable y tipos enteros estándar (`int`, `uint8_t`, `uint64_t`), minimizando sobrecargas de indirección y memoria dinámica.

---

### Diagrama arquitectónico del subsistema de pestañas

```
┌────────────────────────────────────────────────────────────────────────┐
│                          ux-shell: Capa UI                             │
│                  (QTabWidget / BrowserWindow / QSS)                    │
│                     [Etapas futuras: 0070, 0071]                       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Consulta / Aplica plan
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   ux-shell: Capa de Servicio / Datos                   │
│                       (TabStripDensityService)                         │
│                         [Etapa futura: 0069]                           │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Orquesta cálculos y mutaciones
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│               ux-shell: Modelo de Dominio (task0068)                   │
│                                                                        │
│   ┌───────────────────────────┐     ┌──────────────────────────────┐   │
│   │  TabStripDensityTypes.h   │     │ TabStripDensityConfiguration │   │
│   │  - Enums tipados          │     │ - Constantes canónicas       │   │
│   │  - TabStripDimensions     │◄────┤ - Factorías Default/Senior/C │   │
│   │  - TabItemLayout          │     │ - verifyInvariants()         │   │
│   │  - TabStripDensityPlan    │     └──────────────┬───────────────┘   │
│   └─────────────┬─────────────┘                    │                   │
│                 │                                  │                   │
│                 ▼                                  ▼                   │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │                   TabStripDensityValidator                     │   │
│   │  - validateDimensions()                                        │   │
│   │  - calculateTabStripLayout() [Compresión elástica $O(N)$]      │   │
│   │  - hitTest(x, y) [Zonificación analítica de impacto]           │   │
│   │  - Funciones de texto público (docs/UX-COPY-PUBLICO.md)        │   │
│   │  - sanitizeTitle() [Protección de privacidad en incógnito]     │   │
│   └────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
```

---

### Algoritmo de compresión elástica y desbordamiento horizontal

```
                          ┌────────────────────────┐
                          │   availableWidth,      │
                          │   N_pinned, N_regular  │
                          └───────────┬────────────┘
                                      │
                                      ▼
             ┌──────────────────────────────────────────────────┐
             │ Reservar: W_newtab + N_pinned * W_pinnedTab      │
             │ W_net = availableWidth - fijos - márgenes        │
             └────────────────────────┬─────────────────────────┘
                                      │
                   ┌──────────────────┴──────────────────┐
                   ▼                                     ▼
           W_net <= 0 ?                             W_net > 0 ?
                   │                                     │
         ┌─────────┴─────────┐                           ▼
         │ SÍ:               │            W_ideal = W_net / N_regular
         │ isOverflowing=true│                           │
         │ mode = Scrollable │             ┌─────────────┼─────────────┐
         │ W_tab = minTabW   │             ▼             ▼             ▼
         └───────────────────┘       W_ideal >=      minTabW <=      W_ideal <
                                     preferredTabW   W_ideal <       minTabW
                                           │         preferredTabW      │
                                           │             │              │
                                           ▼             ▼              ▼
                                     W_tab =         W_tab =        W_tab =
                                     clamp(ideal,    W_ideal        minTabW;
                                           maxTabW)  (compresión)   isOverflowing
                                                                    = true;
                                                                    Scrollable
```

---

### Estrategia de migración y rollback

- **Evolución sin rotura de código existente**:
  - Los nuevos archivos se introducen exclusivamente en `src/core/` como nuevas unidades de compilación dentro de `vigilio_core`.
  - No se modifica ni refactoriza `TabModel.h` ni `BrowserWindow.cpp` en esta etapa, previniendo regresiones en el código existente.
  - La suite de pruebas unitarias `vigilio_tab_strip_density_tests` es un target independiente que se integra de manera limpia al pipeline de CTest sin afectar los 29 tests precedentes.
- **Plan de Rollback**:
  - Si se requiriese revertir la propuesta o el incremento, basta con eliminar los nuevos archivos en `src/core/` y `tests/unit/`, y remover las líneas correspondientes de `CMakeLists.txt`, restaurando el commit base `710169b` sin efectos secundarios.
