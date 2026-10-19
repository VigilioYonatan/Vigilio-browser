# Change Proposal — module-ux-shell-task0065

Pack: `module.ux-shell.task0065`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **title bar integration** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0065 — Implementar la operación en una capa propietaria: title bar integration`).

Habiéndose completado y formalizado rigurosamente en la etapa previa `domain` (`module.ux-shell.task0064`, commit `ac3786a`) la taxonomía formal y tipos de dominio (`TitleBarTypes.h`), las configuraciones canónicas de densidad geométrica (`TitleBarConfiguration`) y el validador estático desacoplado con cálculo analítico de distribución espacial y pruebas de impacto (*non-client hit testing*) (`TitleBarValidator`), este pack formaliza la **capa de servicio operativo propietaria (`TitleBarService`) en C++20 puro**, completamente desacoplada de la jerarquía de widgets Qt (`BrowserWindow` / `TitleBarWidget`) y de llamadas nativas directas al gestor de ventanas del sistema operativo (Win32/X11/Wayland/Cocoa).

El servicio `TitleBarService` es el componente responsable del ciclo de vida operativo, mutaciones deterministas de estado, ajuste dinámico por densidad visual («Interfaz más fácil de usar» / estándar / compacta), reconfiguración por anchura de ventana, conmutación de anclaje de controles (derecha estándar o izquierda macOS), sanitización de títulos de ventana para proteger la privacidad del usuario, soporte de cancelación cooperativa bajo la política Zero-Zombie (`CancellationToken`), persistencia atómica local y recuperación resiliente ante corrupción en disco, preservando el aislamiento absoluto de sesiones privadas o en modo incógnito de conformidad estricta con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices de `docs/UX-COPY-PUBLICO.md`.

El alcance concreto comprende:
- **Capa operativa propietaria desacoplada (`TitleBarService.h` y `TitleBarService.cpp`)**:
  - Gestión atómica y multihilo del estado de la barra de título con sincronización mediante `std::mutex`.
  - Inicialización flexible: construcción con configuración canónica por defecto (`canonicalDefaultTitleBar()`), accesible («Interfaz más fácil de usar»), compacta o inyección declarativa de un `TitleBarConfig` validado.
  - Conmutación de densidades visuales (`setDensity(LayoutDensity)`): transición determinista recalculando dimensiones de botones ($46\times 32$ estándar vs. $\ge 48\times 48$ px en «Interfaz más fácil de usar»), alturas mínimas ($32$ px vs. $48$ px) y presupuestos de área de arrastre ($\ge 120$ px vs. $\ge 160$ px).
  - Mutación declarativa de modo (`setMode(TitleBarMode)`): conmutación entre `CustomIntegrated`, `NativeSystem` y `Suppressed`.
  - Conmutación de colocación de controles (`setControlsPlacement(TitleBarControlsPlacement)`): soporte para caja de controles anclada a la derecha (`Right`) o a la izquierda (`Left`), recalculando las posiciones cartesianas sin provocar colisiones ni solapamientos.
  - Saneamiento y actualización de título (`setTitle(std::string)`): sanitización para prevenir inyección de caracteres de control o desbordamiento, y eliminación de datos sensibles (tokens, URLs completas con query strings privadas) para proteger la privacidad local.
  - Control de visibilidad de componentes auxiliares: alternancia del icono de ventana (`setShowIcon(bool)`) y del botón de acceso directo a la «Interfaz más fácil de usar» (`setShowSeniorToggle(bool)`).
  - Máquina de estados de ventana (`setWindowState(const TitleBarState&)`): reflejo de estados de ventana (activa/inactiva, maximizada/restaurada, pantalla completa) y recálculo de etiquetas accesibles e iconos de control correspondientes.
  - Redimensionamiento y recálculo analítico (`setWindowWidth(int)`, `computeLayout(CancellationToken* cancel = nullptr)`): ajuste dinámico frente al ancho de la ventana anfitriona, comprobación de umbrales mínimos ($W \ge 400$ px estándar, $W \ge 500$ px Senior) y aborto cooperativo si el token de cancelación es señalado.
  - Clasificación de coordenadas relativas (`hitTest(int x, int y)`): delegación thread-safe al validador cartesiano para clasificar clics y áreas de arrastre (`TitleBarHitTestZone`) en tiempo constante $\mathcal{O}(1)$.
  - Persistencia local y resiliencia de estado en disco (Local-First):
    - `saveStateToFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: serialización determinista en formato clave-valor/JSON estructurado con escritura atómica mediante archivo temporal (`.tmp`) y reemplazo seguro (`std::filesystem::rename`).
    - `loadStateFromFile(const std::filesystem::path& path, CancellationToken* cancel = nullptr)`: deserialización con validación exhaustiva de parámetros y límites dimensionales.
    - Resiliencia y recuperación automática ante corrupción: si el archivo de estado en disco está truncado, malformado o contiene valores fuera de rango, el servicio restaura automáticamente la configuración canónica por defecto sin interrumpir la aplicación ni arrojar excepciones.
    - Aislamiento absoluto de navegación en modo incógnito (`setIncognitoMode(bool)` / `isIncognitoMode()`): el estado de la barra en modo privado permanece estrictamente en memoria volátil; cualquier invocación a guardar en disco es rechazada de inmediato sin escribir ningún byte.
  - Observabilidad y notificaciones desacopladas:
    - Mecanismo de suscripción (`setTitleBarChangedCallback(OnTitleBarChangedCallback)`) que permite a la interfaz Qt en la futura etapa UI (`module.ux-shell.task0066`) reaccionar a cambios de geometría y estado sin acoplamiento circular.
    - Formateo de resúmenes de barra de título y diagnósticos en español neutro cumpliendo estrictamente `docs/UX-COPY-PUBLICO.md`.
- **Integración en el sistema de construcción CMake (`CMakeLists.txt`)**:
  - Incorporación de `src/core/TitleBarService.cpp` a la biblioteca central `vigilio_core`.
  - Declaración del ejecutable de pruebas unitarias `vigilio_title_bar_service_tests` vinculado contra `vigilio_core` y `vigilio_compiler_warnings`.
  - Registro de la prueba `title_bar_service_tests` en CTest bajo las etiquetas `unit;fast` con un timeout estricto de 15 segundos.
- **Suite de pruebas unitarias de servicio (`tests/unit/TitleBarServiceTests.cpp`)**:
  - 14 pruebas unitarias exhaustivas en C++20 puro, evaluando inicialización, mutaciones de estado, transiciones de densidad, conmutación de controles izquierda/derecha, redimensionamiento, cancelación cooperativa Zero-Zombie, serialización atómica en disco, tolerancia a archivos corruptos, aislamiento de incógnito y observabilidad en español neutro.

---

## Out of scope

- Integración física con widgets de Qt (`TitleBarWidget.cpp`, `BrowserWindow.cpp`, `QToolButton`, `QIcon`, hojas de estilo QSS, instalación de filtros de eventos de ratón `mousePressEvent`/`mouseMoveEvent` o intercepción de mensajes de ventana nativos como `WM_NCHITTEST` en Windows o `xdg-shell` en Wayland/Linux) (reservado estrictamente para la etapa `ui` en `module.ux-shell.task0066`).
- Pruebas E2E completas con gestor de ventanas del sistema operativo, arrastre físico de ventana en servidor X11/Wayland/Win32, docking interactivo y prueba de estrés multimonitor (reservado para la etapa `tests` en `module.ux-shell.task0067`).
- Modificación directa del código de producción existente en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` o creación de archivos en `apps/**` durante esta fase PROPOSE (cualquier cambio en `apps/**` está estrictamente reservado para la fase APPLY tras aprobación humana).
- Conexión con servicios en la nube, autenticación externa o telemetría remota de uso de botones (estrictamente prohibido por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Integración de vistas web o procesos de renderizado Chromium WebEngine (la barra de título opera en el shell exterior de la ventana).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Estado del árbol de código y dependencias previas**:
   - Se completó formalmente la microtarea `0064` (`module.ux-shell.task0064`) en el commit `ac3786a`, estableciendo los tipos de dominio (`TitleBarTypes.h`), la configuración canónica (`TitleBarConfiguration.h/.cpp`), el validador espacial analítico (`TitleBarValidator.h/.cpp`) y la suite de pruebas unitarias de especificación (`TitleBarIntegrationTests.cpp`).
   - La suite de CTest ejecuta actualmente 28 de 28 pruebas pasando al 100% en ~6.28 segundos sin advertencias con `-Werror` activado en GCC 15.2.0.
2. **Inspección de servicios de core existentes (`apps/vigilio-browser/src/core/`)**:
   - Se analizó el servicio análogo `MainWindowLayoutService` implementado en `task0061`:
     - Utiliza un mutex interno `mutable std::mutex mutex_` para sincronización multihilo de todas las lecturas y mutaciones.
     - Aplica un parser JSON embebido ligero y robusto (`SimpleJsonParser`) para serialización y deserialización sin dependencias externas pesadas.
     - Implementa escritura atómica con archivo temporal (`.tmp`) y renombramiento seguro para proteger la integridad de los datos en disco ante cortes de energía o caídas de proceso.
     - Protege el aislamiento de incógnito denegando escrituras en disco cuando `isIncognitoSession` está activado.
     - Provee callbacks desacoplados `std::function` y formateadores de diagnósticos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Referencias visuales canónicas (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Refleja la barra superior integrada del navegador con fondo azul oscuro sobrio, controles discretos a la derecha y área de arrastre despejada.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Demuestra las proporciones y requerimientos de la «Interfaz más fácil de usar» (Modo Senior), con botones de acción grandes ($\ge 48\times 48$ px), altura de barra ampliada ($\ge 48$ px) y márgenes generosos sin solapamiento.
   - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser para la esquina superior de la barra de título integrada.
4. **Verificación de calidad y pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con CMake 3.24+ y Ninja bajo C++20 con `-Werror`.
   - 28 de 28 pruebas unitarias e integración pasando en CTest (exit code 0).
5. **Gobernanza con Prompt Builder CLI 0.6.3**:
   - `vigilio-prompt validate-references --project-root .`: 65 referencias canónicas validadas (`valid: true`, exit code 0).
   - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0193-module.ux-shell.task0065.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `vigilio-prompt validate --pack module.ux-shell.task0065 --phase propose --project-root .`: Validación de pack OK (exit code 0).
   - `vigilio-prompt validate-decisions --project-root .`: Única decisión abierta `Q-007` (licencia jurídica), restringida a `release.public-release`, sin bloqueo sobre `module.ux-shell`.

---

## Alcance exacto

El alcance exacto de este cambio en la etapa `implementation` comprende exclusivamente:
1. **Creación de `apps/vigilio-browser/src/core/TitleBarService.h`**:
   - Declaración de la clase `TitleBarService` con métodos thread-safe de ciclo de vida, mutación, cálculo de layout, hit-testing, persistencia y suscripción de eventos.
   - Definición de la estructura de opciones operativas `TitleBarServiceOptions` y el tipo de callback `OnTitleBarChangedCallback`.
2. **Creación de `apps/vigilio-browser/src/core/TitleBarService.cpp`**:
   - Implementación C++20 robusta, thread-safe y desacoplada de GUI.
   - Gestión de transiciones entre configuraciones canónicas (`canonicalDefaultTitleBar`, `canonicalSeniorTitleBar`, `canonicalCompactTitleBar`).
   - Mecanismo de persistencia local atómica (archivo temporal `.tmp` y reemplazo atómico seguro).
   - Parser JSON embebido y deserialización con tolerancia a fallos, validación exhaustiva y fallback seguro a valores por defecto ante corrupción.
   - Saneamiento riguroso de títulos para evitar fugas de privacidad o desbordamiento.
   - Integración de tokens de cancelación cooperativa `CancellationToken` bajo la política Zero-Zombie.
   - Conversión de resúmenes operativos a español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.
3. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Adición de `src/core/TitleBarService.cpp` a la biblioteca `vigilio_core`.
   - Declaración del ejecutable de pruebas unitarias `vigilio_title_bar_service_tests` vinculado a `vigilio_core` y `vigilio_compiler_warnings`.
   - Registro en CTest de `title_bar_service_tests` con propiedades `LABELS "unit;fast"` y `TIMEOUT 15`.
4. **Creación de `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`**:
   - 14 pruebas unitarias independientes evaluando inicialización, mutaciones, transiciones de densidad, colocación de controles, redimensionamiento, persistencia atómica, tolerancia a corrupción, modo incógnito y cancelación Zero-Zombie.

---

## Diseño completo de la capacidad

### Entidades y responsabilidades
- `TitleBarService`: Autoridad operativa central en memoria para la barra de título integrada. Gestiona la configuración activa (`TitleBarConfig`), el ancho de ventana actual y el plan calculado (`TitleBarLayoutPlan`).
- `TitleBarServiceOptions`: Parámetros operativos inyectables: validación automática ante mutaciones (`autoValidateOnMutation`), habilitación de persistencia (`enablePersistence`), sesión en modo incógnito (`isIncognitoSession`) y ruta del archivo de estado (`stateFilePath`).
- `OnTitleBarChangedCallback`: Función receptora que notifica a observadores externos ante cualquier mutación validada del layout o estado de la barra.

### Métodos principales
- `setDensity(LayoutDensity density)`: Aplica las dimensiones canónicas de la densidad objetivo. Al cambiar a `LayoutDensity::Senior`, amplía la altura a 48 px, los botones a $48\times 48$ px y activa el botón de acceso rápido accesible.
- `setControlsPlacement(TitleBarControlsPlacement placement)`: Conmuta entre controles a la derecha (`Right`) o a la izquierda (`Left`), recalculando inmediatamente los rectángulos cartesianos de botones, icono y título.
- `setTitle(std::string title)`: Sanitiza la cadena entrante: recorta espacios redundantes, trunca longitud excesiva ($> 120$ caracteres) y sanitiza posibles cadenas que contengan patrones de URLs privadas con tokens de autenticación.
- `setWindowState(const TitleBarState& state)`: Actualiza las banderas de ventana (`isMaximized`, `isActive`, `isFullscreen`, `seniorModeActive`), sincronizando las etiquetas accesibles y descripciones emergentes de los botones de control (p. ej., alternando entre «Maximizar» y «Restaurar»).
- `setWindowWidth(int windowWidth)`: Recalcula analíticamente el plan de barra de título para la nueva anchura, validando umbrales mínimos y rechazando anchos sub-mínimos.
- `updateButtonState(TitleBarButtonType buttonType, TitleBarButtonState state)`: Actualiza de forma ligera el estado interactivo (normal, hover, presionado, deshabilitado) del botón indicado sin necesidad de relayout geométrico completo.
- `hitTest(int x, int y) const`: Consulta de clasificación espacial en $\mathcal{O}(1)$ que traduce coordenadas cartesianas relativas a la zona de impacto (`TitleBarHitTestZone`).
- `saveStateToFile(...)` y `loadStateFromFile(...)`: Serialización y deserialización JSON atómica en disco con recuperación ante archivos corruptos y bloqueo incondicional en modo incógnito.
- `formatTitleBarSummary(...)`: Generador de resúmenes de estado legibles para humanos en español neutro cumpliendo `docs/UX-COPY-PUBLICO.md`.

---

## Archivos afectados

| Archivo | Rol | Estado |
| --- | --- | --- |
| `apps/vigilio-browser/src/core/TitleBarService.h` | Declaración de la clase de servicio propietario, opciones y callbacks | Nuevo |
| `apps/vigilio-browser/src/core/TitleBarService.cpp` | Implementación C++20 thread-safe del servicio, persistencia atómica y copy público | Nuevo |
| `apps/vigilio-browser/CMakeLists.txt` | Registro de `TitleBarService.cpp` en `vigilio_core` y target `vigilio_title_bar_service_tests` | Modificado |
| `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp` | Suite de 14 pruebas unitarias de servicio en CTest | Nuevo |
| `docs/rules/01-modules/ux-shell/micro/0065-title-bar-integration/task.md` | Sincronización del estado de microtarea canónica | Modificado |

---

## Contrato UX/UI

### Integración visual y densidad de escritorio
- **Sin cambio visual directo en esta etapa**: La etapa `implementation` opera exclusivamente en la capa de núcleo C++20 (`vigilio_core`), preparando la lógica de servicio para que la etapa siguiente (`ui`, microtarea `0066`) ensamble físicamente el widget de barra de título en `BrowserWindow`.
- **Adaptación a «Interfaz más fácil de usar» (Modo Senior)**:
  - El servicio recalcula automáticamente la geometría para cumplir con WCAG 2.1 AAA: altura de barra $\ge 48$ px, botones de control $\ge 48\times 48$ px con separación de al menos 4 px y área de arrastre mínima $\ge 160$ px.
  - Provisión de etiquetas accesibles («Minimizar ventana», «Maximizar ventana», «Restaurar ventana», «Cerrar ventana», «Acceso a interfaz fácil») y descripciones emergentes en español neutro.
- **Copy público estricto (`docs/UX-COPY-PUBLICO.md`)**:
  - Queda prohibido emitir en resúmenes o diagnósticos términos como «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec». Se utiliza sistemáticamente la denominación **«Interfaz más fácil de usar»**.

---

## Reglas y fallos

### Condiciones de error tipadas
1. **Dimensiones insuficientes (`TitleBarError::DimensionsBelowMinimum`)**: Si la anchura de ventana solicitada es inferior a 400 px (estándar) o 500 px (Senior), el servicio rechaza la mutación y mantiene la geometría segura previa.
2. **Área de arrastre insuficiente (`TitleBarError::DraggableAreaInsufficient`)**: Si el espacio horizontal disponible entre título y controles es menor a 120 px (estándar) o 160 px (Senior), el cálculo analítico reporta anomalía.
3. **Persistencia corrupta o truncada (`TitleBarError::None` con fallback seguro)**: Si el archivo de estado en disco contiene sintaxis inválida, claves faltantes o valores numéricos corruptos, el servicio restaura automáticamente `canonicalDefaultTitleBar()` sin arrojar excepciones ni abortar la aplicación.
4. **Infracción de privacidad en modo incógnito**: Si se invoca `saveStateToFile` con `isIncognitoMode() == true`, la operación retorna `false` inmediatamente y no interactúa con el sistema de archivos.

---

## Seguridad y privacidad

1. **Aislamiento Local-First (ADR-001 / Q-001)**:
   - Todo dato de configuración se procesa y persiste 100% de manera local. Cero conexiones a servidores externos, cero telemetría remota y cero dependencia de credenciales en la nube.
2. **Aislamiento de sesiones de incógnito**:
   - En ventanas privadas o sesiones de incógnito, la persistencia en disco queda totalmente desactivada. Las mutaciones residen exclusivamente en memoria volátil y se destruyen al cerrar la instancia.
3. **Sanitización de títulos de ventana**:
   - El método `setTitle` sanitiza cadenas para evitar que URLs completas, parámetros sensibles o tokens de sesión queden expuestos en logs locales o archivos de persistencia.

---

## Matriz de pruebas

| Identificador | Descripción | Tipo | Comando / Fixture | Resultado esperado |
| --- | --- | --- | --- | --- |
| `TEST-TBS-001` | Inicialización por defecto y plan canónico estándar | Unitario | `vigilio_title_bar_service_tests` | Altura 32 px, botones $46\times 32$, controles a la derecha, válido |
| `TEST-TBS-002` | Inicialización personalizada con controles a la izquierda | Unitario | `vigilio_title_bar_service_tests` | Controles en $x=0$, área de arrastre desplazada, plan disjunto |
| `TEST-TBS-003` | Transición a densidad «Interfaz más fácil de usar» | Unitario | `vigilio_title_bar_service_tests` | Altura 48 px, botones $48\times 48$, toggle accesible activo |
| `TEST-TBS-004` | Transición a densidad compacta | Unitario | `vigilio_title_bar_service_tests` | Altura 28 px, botones $36\times 28$, área de arrastre $\ge 100$ px |
| `TEST-TBS-005` | Conmutación dinámica de placement (Right -> Left -> Right) | Unitario | `vigilio_title_bar_service_tests` | Rectángulos de controles e icono recalculados sin colisión |
| `TEST-TBS-006` | Saneamiento y recorte de título de ventana | Unitario | `vigilio_title_bar_service_tests` | Espacios superfluos recortados, longitud acotada, válido |
| `TEST-TBS-007` | Conmutación de visibilidad de icono y toggle Senior | Unitario | `vigilio_title_bar_service_tests` | Icono oculto libera espacio para arrastre, toggle añade botón disjunto |
| `TEST-TBS-008` | Mutación de estado de ventana (maximizado, activo, fullscreen) | Unitario | `vigilio_title_bar_service_tests` | Etiquetas alternan entre Maximizar y Restaurar; fullscreen suprime controles |
| `TEST-TBS-009` | Redimensionamiento de ventana y rechazo de anchos submínimos | Unitario | `vigilio_title_bar_service_tests` | Rechaza $W < 400$ px con error; acepta $W \ge 400$ px actualizando ancho |
| `TEST-TBS-010` | Hit-testing cartesiano a través del servicio | Unitario | `vigilio_title_bar_service_tests` | Coordenadas sobre botones retornan botón respectivo; espacio libre retorna `DraggableSpace` |
| `TEST-TBS-011` | Persistencia atómica local (guardado y carga roundtrip) | Unitario | `vigilio_title_bar_service_tests` (archivo temp) | Configuración serializada se recarga exactamente idéntica |
| `TEST-TBS-012` | Resiliencia ante archivo de estado corrupto o truncado | Unitario | `vigilio_title_bar_service_tests` (JSON corrupto) | Retorna false, restaura plan canónico por defecto sin crashear |
| `TEST-TBS-013` | Aislamiento incondicional de modo incógnito | Unitario | `vigilio_title_bar_service_tests` | Con `incognito=true`, `saveStateToFile` retorna false y no crea archivo |
| `TEST-TBS-014` | Cancelación cooperativa Zero-Zombie y observabilidad con copy público | Unitario | `vigilio_title_bar_service_tests` | Aborta con token cancelado; copy público no contiene términos proscritos |

---

## Pasos APPLY

1. **Paso 1**: Crear `apps/vigilio-browser/src/core/TitleBarService.h` definiendo la interfaz C++20 thread-safe, `TitleBarServiceOptions`, callbacks y métodos públicos de mutación, cálculo, hit-testing y persistencia.
2. **Paso 2**: Crear `apps/vigilio-browser/src/core/TitleBarService.cpp` implementando el servicio, la persistencia atómica con archivo temporal `.tmp`, el parser JSON tolerante a fallos, el aislamiento de modo incógnito, la cancelación cooperativa y los formateadores en español neutro.
3. **Paso 3**: Modificar `apps/vigilio-browser/CMakeLists.txt` agregando `src/core/TitleBarService.cpp` a `vigilio_core`, creando el ejecutable de prueba `vigilio_title_bar_service_tests` y registrándolo en CTest.
4. **Paso 4**: Crear `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp` con las 14 pruebas unitarias exhaustivas.
5. **Paso 5**: Ejecutar compilación y verificación con `apps/vigilio-browser/scripts/check.sh`, certificando 29 de 29 pruebas pasando al 100% en CTest sin advertencias ni regresiones.
6. **Paso 6**: Sincronizar `docs/rules/01-modules/ux-shell/micro/0065-title-bar-integration/task.md` y formalizar la entrega en git.

---

## Definición de terminado

- `TitleBarService.h` y `TitleBarService.cpp` implementados en C++20 puro en `vigilio_core` sin dependencias de GUI.
- Todas las mutaciones protegidas con `std::mutex` y soportando cancelación cooperativa Zero-Zombie (`CancellationToken`).
- Persistencia local atómica implementada con archivo temporal `.tmp` y fallback resiliente ante archivos dañados.
- Aislamiento absoluto de navegación privada/incógnito certificado en pruebas unitarias.
- 14 pruebas unitarias en `TitleBarServiceTests.cpp` pasando al 100% en CTest.
- Sincronización documental completa y sin advertencias en Clang-Format, compilador o linters.

---

## Dependencias y riesgos

- **Dependencia previa satisfecha**: `module.ux-shell.task0064` completada y verificada en el commit `ac3786a`.
- **Riesgo 1**: Inconsistencia de concurrencia si múltiples hilos solicitan mutaciones de layout simultáneamente.
  - *Mitigación*: Uso estricto de `std::lock_guard<std::mutex>` en todos los accesos de lectura y mutación del servicio.
- **Riesgo 2**: Corrupción del archivo de preferencias de la barra de título ante cortes de energía o cierres inesperados.
  - *Mitigación*: Técnica de escritura atómica en archivo temporal `.tmp` y reemplazo seguro (`std::filesystem::rename`), combinada con restauración automática al plan canónico ante errores de parseo.
- **Riesgo 3**: Violación de privacidad en sesiones de incógnito.
  - *Mitigación*: Verificación estricta de la bandera `isIncognitoSession` que anula incondicionalmente cualquier escritura a disco.

---

## Commit propuesto

`feat(ux-shell): implementation title-bar-integration`
