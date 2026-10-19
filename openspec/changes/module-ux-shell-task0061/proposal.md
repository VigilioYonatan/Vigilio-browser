# Change Proposal — module-ux-shell-task0061

Pack: `module.ux-shell.task0061`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **main window layout** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0061 — Implementar la operación en una capa propietaria: main window layout`).

Habiéndose completado y certificado en la etapa previa `domain` (`module.ux-shell.task0060`, commit `4cb5d8f`) la taxonomía formal de zonas de ventana (`LayoutZone`), las configuraciones canónicas de densidad (`MainWindowLayoutConfiguration`), las invariantes geométricas y el validador estático desacoplado con cálculo analítico de distribución espacial (`MainWindowLayoutValidator`), este pack formaliza la **capa de servicio operativo propietaria (`MainWindowLayoutService`) en C++20 puro**, desacoplada de la jerarquía de widgets Qt (`BrowserWindow`), responsable de la gestión de ciclo de vida del layout, transiciones de estado y densidad visual, ajuste y anclaje de la barra lateral, gestión dinámica de avisos y barras de herramientas, persistencia local resiliente ante caídas y soporte de cancelación cooperativa bajo la política Zero-Zombie.

El objetivo fundamental de esta propuesta es proveer una capa de orquestación transaccional e inmutable en memoria para el layout de la ventana principal, garantizando que cualquier redimensionamiento, conmutación entre densidad estándar y «Interfaz más fácil de usar» (Modo Senior) o colapso/expansión de la barra lateral se ejecute bajo validación estricta de invariantes, sin bloquear el hilo de interfaz, con respaldo y recuperación ante datos corruptos en disco y preservando el aislamiento absoluto de datos privados/incógnito de acuerdo con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices públicas de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Capa operativa propietaria desacoplada (`MainWindowLayoutService.h` y `MainWindowLayoutService.cpp`)**:
  - Gestión atómica y multihilo del estado de layout con protección de concurrencia mediante `std::mutex`.
  - Inicialización flexible: construcción con plan canónico estándar (`canonicalDefaultLayout()`), accesible (`canonicalSeniorLayout()`), compacto (`canonicalCompactLayout()`) o inyección de un plan arbitrario verificado.
  - Conmutación de densidades (`setDensity(LayoutDensity)`): transición determinista recalculando dimensiones mínimas, alturas de barras de herramientas y presupuestos de tap targets ($\ge 48$ px en «Interfaz más fácil de usar»).
  - Control de geometría y redimensionamiento (`resizeWindow(int width, int height)`): validación preventiva de límites dimensionales contra la densidad activa mediante `MainWindowLayoutValidator::validateDimensions`, impidiendo estados inconsistentes antes de la asignación.
  - Máquina de estados de la barra lateral (`Sidebar`):
    - `toggleSidebar()`: alternancia fluida entre estado colapsado (48 px) y expandido (canónico 240 px).
    - `setSidebarState(SidebarState)`: asignación explícita (`Hidden`, `Collapsed`, `Expanded`).
    - `setSidebarWidth(int width)`: ajuste dentro del rango seguro $[200, 360]$ px o encaje automático al valor colapsado de 48 px.
    - `setSidebarDockPosition(SidebarDockPosition)`: conmutación de anclaje lateral (`Left`, `Right`).
  - Control dinámico de zonas de interfaz:
    - Visibilidad de avisos (`showNotificationBanner(bool)`) y barra de estado (`showStatusBar(bool)`).
    - Asignación genérica de visibilidad por zona (`setZoneVisible(LayoutZone, bool)`).
  - Cálculo analítico de geometría con cancelación cooperativa (`computeLayout(CancellationToken* cancel = nullptr)`):
    - Invocación segura de `MainWindowLayoutValidator::calculateLayoutAreas` verificando la ausencia matemática de solapamiento ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y suficiencia de área de contenido central ($W \ge 400$, $H \ge 250$).
    - Soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie (`CancellationToken`), abortando cálculos pesados si el token es señalado.
  - Persistencia local y resiliencia de estado en disco (Local-First):
    - `saveLayoutStateToFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: serialización determinista en formato clave-valor/JSON estructurado con escritura atómica mediante archivo temporal (`.tmp`) y reemplazo seguro.
    - `loadLayoutStateFromFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: deserialización con validación exhaustiva de campos, dimensiones y estados.
    - Recuperación automática ante corrupción: si el archivo de estado en disco está truncado, malformado o contiene valores fuera de rango, el servicio restaura automáticamente el plan canónico por defecto sin interrumpir la aplicación, emitiendo el código recuperable `MainWindowLayoutError::LayoutPersistenceCorrupted`.
    - Aislamiento de sesiones privadas/incógnito (`setIncognitoMode(bool)` / `isIncognitoMode()`): el estado de ventanas en modo incógnito se mantiene exclusivamente en memoria volátil y nunca se persiste en disco, garantizando privacidad absoluta.
  - Observabilidad y notificaciones desacopladas:
    - Mecanismo de suscripción de eventos (`setLayoutChangedCallback(OnLayoutChangedCallback)`) que permite a observadores externos (como la ventana Qt en la futura etapa UI) reaccionar a cambios sin acoplamiento bidireccional.
    - Formateo de resúmenes de layout y diagnósticos en español neutro cumpliendo estrictamente `docs/UX-COPY-PUBLICO.md`.
- **Integración en el sistema de construcción CMake (`CMakeLists.txt`)**:
  - Incorporación de `src/core/MainWindowLayoutService.cpp` a la biblioteca central `vigilio_core`.
  - Declaración del ejecutable de pruebas unitarias `vigilio_main_window_layout_service_tests` vinculado contra `vigilio_core` y `vigilio_compiler_warnings`.
  - Registro de la prueba `main_window_layout_service_tests` en CTest bajo las etiquetas `unit;fast` con un timeout estricto de 15 segundos.
- **Suite de pruebas unitarias de servicio (`tests/unit/MainWindowLayoutServiceTests.cpp`)**:
  - 14 pruebas unitarias exhaustivas en C++20 puro, evaluando inicialización, mutaciones de estado, transiciones de densidad, conmutación de barra lateral, redimensionamiento, cancelación cooperativa, serialización atómica en disco, tolerancia a archivos corruptos, aislamiento de incógnito y observabilidad en español neutro.

---

## Out of scope

- Integración física con widgets de Qt (`BrowserWindow.cpp`, `QSplitter`, `QToolBar`, `QTabWidget`, `QStatusBar`, hojas de estilo QSS, animaciones Qt) y renderizado gráfico (reservado estrictamente para la etapa `ui` en `module.ux-shell.task0062`).
- Pruebas E2E completas con servidor gráfico, redimensionamiento interactivo con ratón y pruebas de estrés de múltiples monitores (reservado para la etapa `tests` en `module.ux-shell.task0063`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` durante esta fase PROPOSE (cualquier cambio a `apps/**` está prohibido en PROPOSE y se ejecutará en APPLY tras aprobación humana).
- Conexión con servicios remotos en la nube, autenticación en servidores Vigilio o telemetría externa (estrictamente prohibido por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Integración de vistas web o procesos de renderizado Chromium WebEngine (el servicio opera en el núcleo de layout desacoplado).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completó formalmente la microtarea `0060` (`module.ux-shell.task0060`) en el commit `4cb5d8f`, estableciendo los tipos de dominio (`MainWindowLayoutTypes.h`), la configuración canónica (`MainWindowLayoutConfiguration.h/.cpp`), el validador espacial (`MainWindowLayoutValidator.h/.cpp`) y la suite de pruebas unitarias (`MainWindowLayoutTests.cpp`).
   - La suite de CTest ejecuta 26 de 26 pruebas pasando al 100% en ~5.39 segundos sin advertencias con `-Werror` activado en GCC 15.2.0.
2. **Inspección de servicios de core existentes (`apps/vigilio-browser/src/core/`)**:
   - Se analizaron los servicios consolidados del sistema: `CiLinuxJobService`, `CiWindowsJobService`, `CiMacOsJobService`, `ClangTidyService` y `TestFixtureService`.
   - Se observaron patrones homogéneos y robustos:
     - Mutex interno `mutable std::mutex mutex_` para protección multihilo.
     - Token de cancelación cooperativa `CancellationToken` bajo la guarda `#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED`.
     - Serialización determinista y manejo seguro de rutas con `std::filesystem::path`.
     - Políticas de fallo suave con retorno de estructuras inmutables de reporte/error tipado.
     - Respaldo o manejo atómico en operaciones de disco para prevenir corrupción ante interrupciones.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Refleja el layout estructural de escritorio objetivo, con sidebar colapsada a 48 px a la izquierda, barra superior de navegación y pestañas, área central con tarjetas y buscador, y barra de estado inferior.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Demuestra las proporciones y requerimientos de la «Interfaz más fácil de usar» (Modo Senior), con botones de acción grandes ($\ge 48$ px), barra de herramientas ampliada ($\ge 56$ px) y márgenes generosos sin solapamiento.
   - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.
4. **Verificación de calidad y pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con CMake 3.24+ y Ninja bajo C++20 con `-Werror`.
   - 26 de 26 pruebas unitarias e integración pasando en CTest (exit code 0).
5. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `vigilio-prompt validate-references --project-root .`: 61 referencias canónicas validadas (`valid: true`, exit code 0).
   - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0181-module.ux-shell.task0061.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `vigilio-prompt validate --pack module.ux-shell.task0061 --phase propose --project-root .`: Validación de pack OK (exit code 0).
   - `vigilio-prompt validate-decisions --project-root .`: Única decisión abierta `Q-007` (licencia jurídica), restringida a `release.public-release`, sin bloqueo sobre `module.ux-shell`.

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `implementation` comprende exclusivamente:
1. **Creación de `apps/vigilio-browser/src/core/MainWindowLayoutService.h`**:
   - Declaración de la clase `MainWindowLayoutService` con métodos de ciclo de vida, mutación, cálculo, persistencia y suscripción de eventos.
   - Definición de tipos auxiliares para el servicio: `MainWindowLayoutServiceOptions`, `OnLayoutChangedCallback` y estructuras de persistencia de estado si aplican.
2. **Creación de `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`**:
   - Implementación C++20 robusta, thread-safe y desacoplada de GUI.
   - Gestión de transiciones entre planes canónicos (`canonicalDefaultLayout`, `canonicalSeniorLayout`, `canonicalCompactLayout`).
   - Algoritmo de persistencia atómica local (con archivo temporal y reemplazo seguro).
   - Deserialización con tolerancia a fallos, validación exhaustiva y fallback seguro a valores por defecto.
   - Conversión de resúmenes operativos a español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Refinamiento de `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`**:
   - Inclusión de la definición protegida de `CancellationToken` bajo `#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED`.
   - Adición de posibles banderas operativas inmutables si son requeridas para la persistencia.
4. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Incorporación de `src/core/MainWindowLayoutService.cpp` en el target de biblioteca `vigilio_core`.
   - Declaración del ejecutable `vigilio_main_window_layout_service_tests` con `tests/unit/MainWindowLayoutServiceTests.cpp`.
   - Registro de la prueba `main_window_layout_service_tests` en CTest con etiquetas `unit;fast`.
5. **Creación de `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`**:
   - Suite completa de 14 pruebas unitarias cubriendo todos los requerimientos funcionales, límites de error, concurrencia, cancelación y persistencia.

---

## Diseño completo de la capacidad

### 1. Entidad central `MainWindowLayoutService`

```cpp
namespace vigilio {

/// @brief Opciones de configuración operativa para el servicio de layout.
struct MainWindowLayoutServiceOptions {
    bool autoValidateOnMutation{true};
    bool allowNegativeContentArea{false}; // Siempre false por invariante
    bool enablePersistence{true};
    bool isIncognitoSession{false};       // Si es true, prohíbe persistencia en disco
    std::filesystem::path stateFilePath{};
};

using OnLayoutChangedCallback = std::function<void(const MainWindowLayoutPlan&, const MainWindowLayoutReport&)>;

class MainWindowLayoutService {
  public:
    MainWindowLayoutService();
    explicit MainWindowLayoutService(MainWindowLayoutPlan initialPlan,
                                     MainWindowLayoutServiceOptions options = {});
    ~MainWindowLayoutService() = default;

    MainWindowLayoutService(const MainWindowLayoutService&) = delete;
    MainWindowLayoutService& operator=(const MainWindowLayoutService&) = delete;
    MainWindowLayoutService(MainWindowLayoutService&&) noexcept;
    MainWindowLayoutService& operator=(MainWindowLayoutService&&) noexcept;

    // Acceso thread-safe al plan y opciones
    [[nodiscard]] MainWindowLayoutPlan plan() const;
    [[nodiscard]] MainWindowLayoutServiceOptions options() const;
    void setOptions(const MainWindowLayoutServiceOptions& options);

    // Conmutación de densidades visuales
    [[nodiscard]] MainWindowLayoutReport setDensity(LayoutDensity density);

    // Dimensionamiento de ventana
    [[nodiscard]] MainWindowLayoutReport resizeWindow(int width, int height);

    // Control de barra lateral (Sidebar)
    [[nodiscard]] MainWindowLayoutReport setSidebarState(SidebarState state);
    [[nodiscard]] MainWindowLayoutReport toggleSidebar();
    [[nodiscard]] MainWindowLayoutReport setSidebarWidth(int width);
    [[nodiscard]] MainWindowLayoutReport setSidebarDockPosition(SidebarDockPosition position);

    // Control dinámico de zonas
    [[nodiscard]] MainWindowLayoutReport setZoneVisible(LayoutZone zone, bool visible);
    [[nodiscard]] MainWindowLayoutReport showNotificationBanner(bool show);
    [[nodiscard]] MainWindowLayoutReport showStatusBar(bool show);

    // Cálculo y evaluación geométrica
    [[nodiscard]] MainWindowLayoutReport computeLayout(CancellationToken* cancel = nullptr) const;

    // Persistencia y resiliencia local (Local-First)
    [[nodiscard]] bool saveLayoutStateToFile(const std::filesystem::path& path,
                                             CancellationToken* cancel = nullptr);
    [[nodiscard]] bool loadLayoutStateFromFile(const std::filesystem::path& path,
                                             CancellationToken* cancel = nullptr);
    [[nodiscard]] bool saveCurrentState(CancellationToken* cancel = nullptr);
    [[nodiscard]] bool restoreDefaultLayout();

    // Privacidad y modo incógnito
    void setIncognitoMode(bool incognito) noexcept;
    [[nodiscard]] bool isIncognitoMode() const noexcept;

    // Observabilidad y notificaciones
    void setLayoutChangedCallback(OnLayoutChangedCallback callback);
    void clearLayoutChangedCallback() noexcept;
    [[nodiscard]] static std::string formatLayoutSummary(const MainWindowLayoutPlan& plan,
                                                         const MainWindowLayoutReport& report);

  private:
    MainWindowLayoutPlan plan_;
    MainWindowLayoutServiceOptions options_;
    OnLayoutChangedCallback callback_{nullptr};
    mutable std::mutex mutex_;

    void notifyChangedLocked(const MainWindowLayoutReport& report);
};

} // namespace vigilio
```

### 2. Responsabilidades y precondiciones
- **Precondiciones**:
  - Toda mutación de dimensiones debe someterse a `MainWindowLayoutValidator::validateDimensions`.
  - Toda mutación de ancho de barra lateral debe someterse a `MainWindowLayoutValidator::validateSidebar`.
  - Toda persistencia en disco debe verificar que `isIncognitoSession == false`.
- **Postcondiciones**:
  - El plan activo cumple siempre las invariantes de `MainWindowLayoutConfiguration::verifyInvariants`.
  - Ninguna zona calculada solapa a otra en las áreas emitidas por `computeLayout`.
  - El área central de contenido nunca es menor que $400\times 250$ px en una configuración válida.
- **Invariantes**:
  - Cero dependencias GUI en `vigilio_core`.
  - Concurrencia protegida: lecturas y escrituras atómicas mediante exclusión mutua.
  - Cero filtración de datos de navegación en archivos de layout.

---

## Archivos afectados

| Archivo | Acción | Descripción del cambio |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/MainWindowLayoutService.h` | **Crear** | Declaración de la clase de servicio `MainWindowLayoutService`, opciones y callbacks. |
| `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp` | **Crear** | Implementación del servicio, transiciones, persistencia atómica, recuperación y UX copy. |
| `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h` | **Modificar** | Incorporación de la guarda de `CancellationToken` si no estuviera ya disponible. |
| `apps/vigilio-browser/CMakeLists.txt` | **Modificar** | Inclusión de `MainWindowLayoutService.cpp` en `vigilio_core` y registro del ejecutable de pruebas en CTest. |
| `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp` | **Crear** | Batería de 14 pruebas unitarias de servicio evaluando toda la superficie de API. |

---

## Contrato UX/UI

- **Sin cambio visual directo en esta etapa**:
  - `MainWindowLayoutService` es un componente de núcleo desacoplado en C++20 que opera en memoria y sobre archivos locales; no manipula directamente instancias de `QWidget` ni realiza llamadas de dibujado de interfaz.
  - La vinculación con widgets Qt (`BrowserWindow`, `QSplitter`, `QToolBar`, `QTabWidget`) se ejecutará en la etapa subsiguiente `ui` (`module.ux-shell.task0062`).
- **Coherencia con las referencias visuales (`/resultado/`)**:
  - El servicio mantiene los parámetros geométricos exigidos por `/resultado/01-vigilio-home.png` (sidebar colapsada a 48 px, expandida a 240 px, toolbar a 42 px) y `/resultado/03-vigilio-settings-senior-mode.png` (toolbar a 56 px, mínimos de ventana $720\times 520$ px).
- **Cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`**:
  - Las funciones de diagnóstico del servicio (`formatLayoutSummary`) usan exclusivamente las denominaciones públicas aprobadas:
    - «Interfaz más fácil de usar» para `LayoutDensity::Senior`.
    - «Estándar» para `LayoutDensity::Default`.
    - «Compacta» para `LayoutDensity::Compact`.
    - Prohibido terminantemente emitir tecnicismos internos («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec»).

---

## Reglas y fallos

1. **Dimensiones inferiores al mínimo (`DimensionsBelowMinimum`)**:
   - Si una petición de `resizeWindow` solicita dimensiones inferiores a los mínimos de la densidad activa ($640\times 480$ estándar, $720\times 520$ Senior, $500\times 380$ Compact), la petición es rechazada de forma no destructiva: el plan conserva sus dimensiones previas y el reporte devuelto incluye `valid = false`, error `DimensionsBelowMinimum` y pista de remediación explicativa.
2. **Ancho de barra lateral inválido (`InvalidSidebarWidth`)**:
   - Peticiones de `setSidebarWidth` fuera del intervalo $[200, 360]$ px cuando la barra está expandida son saneadas o rechazadas sin provocar estados corruptos.
3. **Persistencia corrupta o ilegible (`LayoutPersistenceCorrupted`)**:
   - Si `loadLayoutStateFromFile` encuentra un archivo inexistente, con formato inválido o valores truncados, no se genera una excepción no capturada ni caída de aplicación. El servicio restaura automáticamente el plan canónico por defecto y reporta el fallo de manera recuperable.
4. **Aislamiento de sesiones privadas**:
   - Si `isIncognitoMode() == true`, cualquier invocación a `saveCurrentState` o `saveLayoutStateToFile` aborta inmediatamente retornando `false` sin escribir en disco, evitando que configuraciones o huellas de navegación privada queden registradas.
5. **Cancelación cooperativa (Zero-Zombie)**:
   - Toda operación de cálculo analítico o persistencia que reciba un `CancellationToken` consulta periódicamente `isCancelled()`. Si la cancelación es solicitada, aborta limpiamente sin fugas de recursos ni hilos huérfanos.

---

## Seguridad y privacidad

- **Arquitectura Local-First constitucional (ADR-001 / Decisión Q-001)**:
  - Todas las operaciones de cálculo, serialización y validación residen 100% en el equipo local.
  - Cero llamadas a servicios de red, endpoints en la nube o telemetría de interfaz.
- **Aislamiento de datos y modo privado**:
  - El estado de la ventana en navegación incógnito está completamente aislado de la persistencia física en disco.
- **Escritura segura y atómica en disco**:
  - Para evitar la corrupción de datos ante cortes imprevistos de energía o caídas del sistema operativo, el servicio escribe primero en un archivo temporal con sufijo `.tmp` y posteriormente realiza un renombramiento atómico (`std::filesystem::rename`).
  - Validación previa de rutas de archivo con `std::filesystem::equivalent` para impedir escaladas de directorios o sobrescritura de archivos del sistema.
- **Presupuestos estrictos de recursos**:
  - El consumo en memoria del servicio es despreciable (< 100 KB para plan, opciones y mutex).
  - El cálculo analítico de distribución espacial se ejecuta en < 1 milisegundo en la CPU.

---

## Matriz de pruebas

La suite `vigilio_main_window_layout_service_tests` implementará 14 pruebas unitarias independientes en C++20 puro bajo `tests/unit/MainWindowLayoutServiceTests.cpp`:

| Test ID | Caso de prueba | Entrada / Escenario | Resultado esperado | Criterio de fallo negativo |
| --- | --- | --- | --- | --- |
| `TEST-SRV-001` | Inicialización por defecto | `MainWindowLayoutService()` | Plan inicial en densidad Default, ventana $1200\times 780$, sidebar colapsada a 48 px | Valores no coinciden con `canonicalDefaultLayout()` |
| `TEST-SRV-002` | Inicialización con plan Senior | Inyección de `canonicalSeniorLayout()` | Plan en densidad Senior, mínimos $\ge 720\times 520$, toolbar $\ge 56$ px | No respeta tap targets o alturas de Senior |
| `TEST-SRV-003` | Conmutación a densidad Senior | `setDensity(LayoutDensity::Senior)` | Plan muta a Senior, recalcula mínimos, emite reporte válido | Plan mantiene valores de Default o rechaza la mutación |
| `TEST-SRV-004` | Conmutación a densidad Compact | `setDensity(LayoutDensity::Compact)` | Plan muta a Compact, toolbar en 36 px, tabstrip en 32 px | Dimensiones no adaptadas |
| `TEST-SRV-005` | Redimensionamiento válido | `resizeWindow(1400, 900)` | Ventana actualizada a $1400\times 900$, reporte `valid == true` | Falla redimensionamiento válido |
| `TEST-SRV-006` | Rechazo de dimensiones sub-mínimas | `resizeWindow(600, 400)` en Default | Rechazado con `DimensionsBelowMinimum`, ventana mantiene tamaño previo | Acepta tamaño inferior a $640\times 480$ |
| `TEST-SRV-007` | Alternancia de barra lateral (toggle) | `toggleSidebar()` | Pasa de Collapsed (48 px) a Expanded (240 px) y viceversa | Estado no alterna correctamente |
| `TEST-SRV-008` | Ajuste de ancho de barra lateral | `setSidebarWidth(280)` | Ancho actualizado a 280 px dentro del rango $[200, 360]$ | Acepta valores fuera de rango ($< 200$ o $> 360$) |
| `TEST-SRV-009` | Conmutación de anclaje de sidebar | `setSidebarDockPosition(Right)` | Anclaje cambia a Right, áreas espaciales recalculadas a la derecha | Posición no se refleja en geometría |
| `TEST-SRV-010` | Visibilidad de aviso y barra de estado | `showNotificationBanner(true)` | Banner visible, área neta de contenido reducida exactamente en 44 px | Solapamiento con barra lateral o toolbar |
| `TEST-SRV-011` | Serialización y deserialización local | Guardar y cargar archivo JSON de layout | El plan recuperado coincide exactamente campo por campo con el original | Pérdida de campos o desajuste de dimensiones |
| `TEST-SRV-012` | Resiliencia ante archivo corrupto | Carga de archivo truncado / JSON malformado | Fallback seguro a `canonicalDefaultLayout()`, reporte de error recuperable | Excepción no capturada o cuelgue del proceso |
| `TEST-SRV-013` | Aislamiento de modo incógnito | Guardar con `isIncognitoMode == true` | Operación cancelada/rechazada, no se genera archivo en disco | Escritura física de estado privado |
| `TEST-SRV-014` | Cancelación cooperativa Zero-Zombie | Cancelación señalada durante cálculo | Operación aborta inmediatamente sin continuar ejecución | Ignora el token de cancelación |

---

## Pasos APPLY

1. **Paso 1: Refinar `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`**:
   - Asegurar la definición homogénea de `CancellationToken` bajo la guarda `#ifndef VIGILIO_CANCELLATION_TOKEN_DEFINED`.
2. **Paso 2: Crear `apps/vigilio-browser/src/core/MainWindowLayoutService.h`**:
   - Declarar la clase `MainWindowLayoutService`, opciones de servicio, callbacks y métodos públicos de mutación, cálculo y persistencia.
3. **Paso 3: Crear `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`**:
   - Implementar los métodos de la clase, serialización/deserialización atómica, validaciones con `MainWindowLayoutValidator`, soporte de cancelación cooperativa y formateo en español neutro.
4. **Paso 4: Actualizar `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/MainWindowLayoutService.cpp` a las fuentes de `vigilio_core`.
   - Declarar el target `vigilio_main_window_layout_service_tests` con `tests/unit/MainWindowLayoutServiceTests.cpp`.
   - Enlazar contra `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar la prueba en CTest con `add_test(NAME main_window_layout_service_tests COMMAND vigilio_main_window_layout_service_tests)` y etiquetas `unit;fast`.
5. **Paso 5: Crear `apps/vigilio-browser/tests/unit/MainWindowLayoutServiceTests.cpp`**:
   - Implementar las 14 pruebas unitarias de servicio utilizando la infraestructura de aserciones del proyecto.
6. **Paso 6: Verificar compilación y pruebas con `apps/vigilio-browser/scripts/check.sh`**:
   - Ejecutar la compilación con Ninja, Clang-Format y la suite completa de CTest (esperando 27 de 27 pruebas pasando al 100%).
7. **Paso 7: Ejecutar herramientas de gobernanza Prompt Builder CLI**:
   - Validar referencias canónicas y lockfile de ejecución.

---

## Definición de terminado

- [ ] `MainWindowLayoutService.h` y `MainWindowLayoutService.cpp` creados en `apps/vigilio-browser/src/core/` con código C++20 fuertemente tipado, const-correct, libre de dependencias GUI y protegido por `std::mutex`.
- [ ] Implementada la máquina de estados de layout: conmutación entre densidades Default, Senior y Compact, redimensionamiento validado, gestión de sidebar (toggle, ancho, dock) y visibilidad de zonas.
- [ ] Implementada la persistencia atómica en disco con archivo temporal y resiliencia ante corrupción con fallback automático al plan canónico.
- [ ] Implementado el aislamiento estricto de modo incógnito impidiendo la persistencia en disco de sesiones privadas.
- [ ] Implementado el soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie.
- [ ] Textos de diagnóstico y resúmenes formulados en español neutro cumpliendo estrictamente `docs/UX-COPY-PUBLICO.md`.
- [ ] `CMakeLists.txt` actualizado registrando la implementación en `vigilio_core` y la nueva prueba en CTest.
- [ ] 14 pruebas unitarias en `MainWindowLayoutServiceTests.cpp` pasando al 100% en CTest.
- [ ] Pipeline `apps/vigilio-browser/scripts/check.sh` pasando con 0 errores, `-Werror` limpio y formato verificado.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.ux-shell.task0060`: Completada satisfactoriamente (commit `4cb5d8f`), proveyendo los tipos, configuraciones y validador de dominio.
- **Riesgos y mitigaciones**:
  - *Riesgo 1: Corrupción de archivo de estado de layout ante cortes abruptos de energía*.
    - *Mitigación*: Escritura atómica a archivo `.tmp` seguida de `std::filesystem::rename`, más detección de JSON/datos truncados con fallback automático a `canonicalDefaultLayout()`.
  - *Riesgo 2: Fugas de información de navegación en modo incógnito*.
    - *Mitigación*: Bloqueo incondicional de persistencia en disco cuando `isIncognitoSession == true`.
  - *Riesgo 3: Inconsistencias geométricas al conmutar a «Interfaz más fácil de usar»*.
    - *Mitigación*: El servicio ajusta preventivamente las dimensiones mínimas a $\ge 720\times 520$ px y recalcula presupuestos antes de aceptar la mutación.
  - *Riesgo 4: Concurrencia entre hilo de interfaz y eventos asíncronos*.
    - *Mitigación*: Exclusión mutua exhaustiva con `std::mutex` en todos los métodos de lectura y escritura del servicio.

---

## Commit propuesto

```
feat(ux-shell): implementation main-window-layout
```
