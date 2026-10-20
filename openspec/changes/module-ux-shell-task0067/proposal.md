# Change Proposal — module-ux-shell-task0067

Pack: `module.ux-shell.task0067`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **tests** de la capacidad **title bar integration** en el módulo propietario `ux-shell` de Vigilio Browser (microtarea canónica `0067 — Probar casos positivos, negativos y recuperación: title bar integration`).

Habiéndose completado y certificado formalmente en las etapas precedentes la formalización del modelo de dominio de la barra de título, configuración canónica y validador analítico (`module.ux-shell.task0064`, commit `ac3786a`), la implementación del servicio operativo desacoplado `vigilio::TitleBarService` en C++20 con sincronización de estado, persistencia atómica, aislamiento de incógnito y cancelación cooperativa Zero-Zombie (`module.ux-shell.task0065`, commit `5358b61`), y la integración física de la interacción Qt Widgets en la ventana principal (`BrowserWindow.h/.cpp`), estructuración del frame de cabecera (`vigilio-titlebar`), zona de arrastre ergonómica, controles de ventana y adaptación reactiva a la «Interfaz más fácil de usar» (`module.ux-shell.task0066`, commit `710169b`), el objetivo fundamental de este pack es **diseñar y estructurar la suite exhaustiva, determinista y de alta resiliencia de pruebas automatizadas para title bar integration**.

La suite de pruebas proyectada cubre sistemáticamente:
- **Casos positivos y ciclo de vida integral de la barra de título**:
  - Verificación exhaustiva de planes canónicos (`canonicalDefaultTitleBar`, `canonicalSeniorTitleBar`, `canonicalCompactTitleBar`), evaluando precondiciones, postcondiciones, restricciones dimensionales mínimas de altura ($32$ px en estándar, $48$ px en accesible, $28$ px en compacto), anchura mínima de ventana ($\ge 400$ px), y la distribución espacial sin solapamientos entre el icono de marca, el título sanitizado, la zona ergonómica de arrastre y la caja de controles de ventana.
  - Transiciones fluidas entre densidades de interfaz (`LayoutDensity::Default` $\leftrightarrow$ `LayoutDensity::Senior` $\leftrightarrow$ `LayoutDensity::Compact`), certificando la correcta recalculación de geometrías de controles, notificación síncrona a suscriptores vía callback y preservación del área mínima de arrastre ($\ge 120$ px en estándar, $\ge 160$ px en accesible, $\ge 100$ px en compacto).
  - Alternancia de colocación de controles (`TitleBarControlsPlacement::Right` $\leftrightarrow$ `TitleBarControlsPlacement::Left`), validando el orden canónico de botones (Close, Minimize, Maximize cuando está a la izquierda; Toggle, Minimize, Maximize, Close cuando está a la derecha) y el cálculo analítico exacto de coordenadas rectangulares sin interferir con la zona de arrastre.
  - Redimensionamiento reactivo continuo de ventana (desde $400$ px hasta resoluciones 4K $3840$ px y ultrapanorámicas), comprobando que el área de arrastre absorbe el delta dimensional de manera proporcional y que los botones de control y el icono mantienen sus dimensiones fijadas por la densidad activa.
- **Casos negativos, validación de fronteras y entradas malformadas**:
  - Detección y rechazo de dimensiones de ventana sub-mínimas:
    - Anchura de ventana inferior a 400 px (e.g. 399 px, 350 px, 100 px), emitiendo el código de error tipado `TitleBarError::DimensionsBelowMinimum`.
    - Dimensiones no positivas ($\le 0$), cero ($0$ px) o valores negativos patológicos, emitiendo `TitleBarError::DimensionsBelowMinimum`.
  - Detección y rechazo de violaciones de altura mínima en la «Interfaz más fácil de usar» (Modo Senior):
    - Altura de barra inferior a 48 px (e.g. 32 px, 40 px, 47 px) en densidad `Senior`, emitiendo `TitleBarError::SeniorModeMinimumHeightViolation` y la correspondiente pista de remediación en español neutro.
  - Detección y rechazo de espacio de arrastre insuficiente:
    - Ventanas estrechas con controles sobredimensionados donde el ancho restante para arrastre es inferior al mínimo canónico, disparando `TitleBarError::DraggableAreaInsufficient`.
  - Detección de desbordamiento de controles (`TitleBarError::ControlsOverflowWidth`) y configuraciones de colocación incompatibles (`TitleBarError::ConflictingControlPlacement`).
  - Detección de modos de decoración de ventana no soportados (`TitleBarError::UnsupportedWindowChromeMode`).
- **Hit-testing espacial analítico exhaustivo y disyunción geométrica**:
  - Evaluación analítica en `TitleBarValidator::hitTest` de coordenadas espaciales:
    - Centro, esquinas y bordes interiores de cada botón de control (`CloseButton`, `MinimizeButton`, `MaximizeButton`, `SeniorModeToggle`).
    - Zona central de arrastre (`DraggableSpace`).
    - Texto del título (`CaptionText`).
    - Icono de ventana (`WindowIcon`).
    - Coordenadas externas a la barra de título (coordenadas negativas, más allá del ancho o alto de la barra), retornando `TitleBarHitTestZone::Nowhere`.
  - Verificación matemática estricta de no solapamiento geométrico:
    - $\forall i \ne j, \text{control}_i \cap \text{control}_j = \emptyset$.
    - $\text{controlsBoxRect} \cap \text{draggableRect} = \emptyset$.
    - $\text{iconRect} \cap \text{draggableRect} = \emptyset$.
- **Concurrencia multihilo de alto estrés (16 hilos) y sincronización thread-safe**:
  - Ejecución paralela sobre `TitleBarService` desde 16 hilos (`std::thread`) concurrentes sin condiciones de carrera (ThreadSanitizer-clean):
    - 4 hilos redimensionando continuamente la ventana (`setWindowWidth`).
    - 4 hilos alternando densidades (`setDensity` entre Default, Senior y Compact).
    - 4 hilos mutando colocación de controles y estados de ventana (`setControlsPlacement`, `setWindowState`, `updateButtonState`).
    - 4 hilos ejecutando hit-testing y consultas simultáneas del plan activo y reporte (`hitTest`, `plan`, `config`).
  - Certificación de ausencia absoluta de carreras de datos, integridad del estado protegido por `std::mutex`, y terminación limpia de todos los hilos sin interbloqueos (deadlocks).
- **Cancelación cooperativa asíncrona multihilo y política Zero-Zombie**:
  - Pruebas de estrés de interrupción cooperativa mediante `CancellationToken` activado desde hilos secundarios en distintos puntos del cálculo de layout y persistencia.
  - Verificación de respuesta inmediata (< 10 ms), aborto seguro de operaciones no comprometidas, preservación de la consistencia del plan activo y ausencia total de procesos zombis o recursos colgados.
- **Recuperación ante fallos de disco, corrupción de persistencia y aislamiento Local-First**:
  - Simulación de fallos de persistencia en disco: rutas no escribibles, permisos denegados (`perms::read_only`) y fallos durante la escritura atómica.
  - Fuzzing e inyección de archivos de estado de barra de título gravemente corruptos en `loadStateFromFile`:
    - Archivos JSON truncados o malformados sintácticamente.
    - Archivos JSON con tipos discordantes (strings en lugar de enteros en dimensiones).
    - Archivos JSON con valores geométricos ilegales (dimensiones sub-mínimas deliberadamente inyectadas).
    - Archivos con bytes binarios aleatorios o contenido vacío.
  - Certificación de que ante cualquier anomalía, `TitleBarService` activa su mecanismo de recuperación resiliente: retorna `false`, restablece de manera transparente el plan canónico seguro (`canonicalDefaultTitleBar()`), y no genera excepciones no capturadas ni estados inconsistentes.
  - Verificación estricta de aislamiento de sesión privada: en modo incógnito (`isIncognitoMode() == true`), `saveStateToFile` y `loadStateFromFile` abortan inmediatamente sin interactuar con el sistema de archivos local, garantizando la privacidad absoluta (ADR-001 / Decisión Q-001).
- **Sanitización de títulos y protección de privacidad**:
  - Verificación analítica de que cadenas de entrada con URLs completas, credenciales embebidas (`user:pass`), tokens de sesión (`token=...`), query strings y fragmentos (`#hash`) son purgados rigurosamente por `TitleBarValidator::sanitizeTitle`.
  - Verificación de que en navegación de incógnito el título refleja explícitamente «Vigilio Browser (Navegación privada)».
  - Manejo robusto de cadenas vacías con fallback seguro a la marca limpia «Vigilio Browser».
- **Integración UI, accesibilidad y auditoría de textos públicos**:
  - Verificación en el arnés de pruebas de interfaz de usuario (`tests/ui/WindowTests.cpp`) de que la jerarquía Qt (`BrowserWindow`) responde fielmente a las mutaciones del servicio de barra de título, valida la presencia de `vigilio-titlebar`, el cambio dinámico de icono y tooltip de maximizar/restaurar ante `WindowStateChange`, las dimensiones de tap target $\ge 48\times 48$ px en la «Interfaz más fácil de usar», y garantiza tolerancia headless reportando `NOT_EXECUTED` de manera limpia si `DISPLAY`/`WAYLAND_DISPLAY` está ausente.
  - Auditoría exhaustiva de lenguaje público (`docs/UX-COPY-PUBLICO.md`): todas las cadenas generadas por el validador, pistas de remediación, tooltips y etiquetas accesibles emplean español neutro claro, sin términos internos proscritos («Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «toolchain»).

---

## Out of scope

- Modificación de los contratos y tipos de dominio consolidados en `TitleBarTypes.h`, `TitleBarConfiguration.h` o `TitleBarValidator.h` durante PROPOSE.
- Modificación directa del código de producción en `apps/vigilio-browser/src/**`, `apps/vigilio-browser/tests/**` o `CMakeLists.txt` durante esta fase PROPOSE (se ejecutarán estrictamente durante APPLY tras aprobación humana).
- Inicialización innecesaria del motor de renderizado de Chromium WebEngine en las pruebas unitarias y de servicio (se ejecutan en C++20 puro de forma rápida y determinista).
- Persistencia remota en la nube o telemetría (vetado taxativamente por la política Local-First constitucional, ADR-001 / Decisión Q-001).
- Generación de mocks de integración que simulen falsamente el éxito de componentes ausentes.
- Pruebas de interacción física directa con gestores de ventanas externos (e.g. X11 window managers o compositores Wayland propietarios) que escapen al arnés determinista de Qt Widgets.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica y el sistema de pruebas:

1. **Dominio e implementación de servicio de barra de título consolidados (`apps/vigilio-browser/src/core/`)**:
   - `TitleBarTypes.h`: Define las enumeraciones canónicas `TitleBarButtonType`, `TitleBarButtonState`, `TitleBarControlsPlacement`, `WindowChromeMode`, `TitleBarHitTestZone`, `TitleBarError`, así como las estructuras inmutables `TitleBarState`, `TitleBarConfig`, `TitleBarButtonLayout`, `TitleBarLayoutPlan` y `TitleBarReport`.
   - `TitleBarConfiguration.h` y `.cpp`: Implementa las configuraciones canónicas oficiales (`canonicalDefaultTitleBar`, `canonicalSeniorTitleBar`, `canonicalCompactTitleBar`), la verificación analítica de invariantes (`verifyInvariants`), y las constantes dimensionales de altura y botones para cada densidad.
   - `TitleBarValidator.h` y `.cpp`: Implementa las reglas de validación estática de dimensiones (`validateDimensions`), configuración de plan (`validatePlan`), el cálculo analítico de distribución espacial sin solapamientos (`calculateTitleBarLayout`), hit-testing geométrico analítico (`hitTest`), sanitización de títulos (`sanitizeTitle`) y las funciones de conversión a texto y pistas de remediación en español neutro (`titleBarErrorToString`, `buttonTypeToString`, `buttonAccessibleLabel`, `buttonTooltip`, `generateRemediationHint`).
   - `TitleBarService.h` y `.cpp`: Capa operativa desacoplada que administra el plan activo, orquesta conmutaciones de densidad (`setDensity`), control de colocación de controles (`setControlsPlacement`), mutaciones de estado de ventana (`setWindowState`, `updateButtonState`), redimensionamiento (`setWindowWidth`), persistencia atómica con tolerancia a archivos corruptos (`saveStateToFile`, `loadStateFromFile`), aislamiento de incógnito (`setIncognitoMode`), sincronización mediante callbacks (`setTitleBarChangedCallback`) y cancelación cooperativa Zero-Zombie (`CancellationToken`).
2. **Integración en la interfaz gráfica (`apps/vigilio-browser/src/ui/BrowserWindow.h/.cpp`)**:
   - `BrowserWindow` incorpora `titleBarService_` (`vigilio::TitleBarService`), el contenedor visual `vigilio-titlebar` (`titleBarWidget_`), icono oficial escalado (`titleBarIcon_`), título sanitizado (`titleBarTitle_`), zona interactiva de arrastre (`titleBarDragArea_`), caja de controles (`titleBarControlsBox_`), botón de accesibilidad (`titleBarSeniorToggleBtn_`), botón de minimizar (`titleBarMinimizeBtn_`), botón de maximizar/restaurar (`titleBarMaximizeBtn_`) y botón de cerrar (`titleBarCloseBtn_`).
   - Sincronización reactiva con `resizeEvent` y `changeEvent` (`WindowStateChange`, `ActivationChange`).
   - Soporte dinámico para la «Interfaz más fácil de usar» con altura de barra $\ge 48$ px y tap targets $\ge 48\times 48$ px.
3. **Suites de pruebas existentes en `apps/vigilio-browser/tests/`**:
   - `tests/unit/TitleBarIntegrationTests.cpp`: 14 pruebas unitarias de dominio aprobadas (commit `ac3786a`).
   - `tests/unit/TitleBarServiceTests.cpp`: 14 pruebas unitarias de servicio aprobadas (commit `5358b61`).
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas Qt Test que verifica la inicialización de `BrowserWindow`, widgets de barra de título, alternancia de estados maximizado y auditoría de textos públicos (commit `710169b`).
4. **Integración en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Los ejecutables `vigilio_title_bar_tests` y `vigilio_title_bar_service_tests` están plenamente integrados en CTest con etiquetas `LABELS "unit;fast"` y timeout de 15 segundos.
   - `vigilio_ui_tests` está integrado condicionalmente con Qt6 Test con etiquetas `LABELS "ui;fast"` y timeout de 20 segundos.
5. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GCC bajo C++20 con `-Werror` y cero advertencias.
   - 29 de 29 pruebas pasando al 100% en CTest (~3.72 s en total, exit code 0).
6. **Referencias visuales y directrices vinculantes**:
   - `/resultado/01-vigilio-home.png`: Layout estructural de la barra de título sobria e integrada.
   - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles ampliados $\ge 48$ px.
   - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial para el isotipo y logotipo de Vigilio Browser.
   - `docs/UX-COPY-PUBLICO.md`: Proscripción de terminología interna de ingeniería.

---

## Alcance exacto

El alcance de este pack se circunscribe estrictamente a la consolidación y ampliación exhaustiva de la suite de pruebas para la capacidad **title bar integration** en la etapa **tests**:

1. **En `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`**:
   - Ampliar pruebas de fronteras dimensionales exactas y entradas negativas (`testSubMinimumDimensionsExhaustive`):
     - Anchuras $399$ px, $0$ px y valores negativos verificando `TitleBarError::DimensionsBelowMinimum`.
     - Alturas inferiores a 48 px en densidad `Senior` ($47$ px, $32$ px) verificando `TitleBarError::SeniorModeMinimumHeightViolation`.
     - Anchuras donde el espacio restante de arrastre sea inferior a 120 px (Default), 160 px (Senior) o 100 px (Compact), verificando `TitleBarError::DraggableAreaInsufficient`.
   - Ampliar pruebas de disyunción espacial estricta y geometrías extremas (`testSpatialDisjointnessAndExtremeResolutions`):
     - Verificación matemática de no solapamiento en resoluciones extremas ($3840\times 2160$, $3840\times 1080$, $1080\times 1920$).
     - Validación de que $\text{controlsBoxRect} \cap \text{draggableRect} = \emptyset$ y $\text{controls}[i] \cap \text{controls}[j] = \emptyset$.
   - Ampliar hit-testing exhaustivo de fronteras y zonas (`testExhaustiveHitTestingBoundaries`):
     - Puntos en las esquinas exactas de cada botón de control.
     - Puntos inmediatamente exteriores a cada control (distancia 1 px).
     - Puntos en coordenadas fuera de la ventana (valores negativos y superiores al ancho/alto).
   - Ampliar casos de sanitización y privacidad de títulos (`testTitleSanitizationEdgeCases`):
     - Cadenas con credenciales embebidas (`https://user:password@banco.com/`).
     - Cadenas con secuencias de escape, saltos de línea y caracteres de control.
     - Cadenas gigantes (> 4096 caracteres) y cadenas vacías con fallback seguro.
   - Auditoría programática exhaustiva de UX Copy (`testPublicCopyExhaustiveAudit`):
     - Evaluación de todos los códigos de error de `TitleBarError`, etiquetas y tooltips frente a la lista completa de términos proscritos de `docs/UX-COPY-PUBLICO.md`.

2. **En `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`**:
   - Implementar prueba de estrés de concurrencia masiva con 16 hilos (`testMultiThreadedHighStressConcurrency16Threads`):
     - 16 hilos compitiendo sobre una instancia compartida de `TitleBarService` durante al menos 200 iteraciones por hilo.
     - Operaciones heterogéneas: `setWindowWidth`, `setDensity`, `setControlsPlacement`, `setWindowState`, `updateButtonState`, `hitTest`, `plan`, `config`.
     - Verificación de ausencia de carreras de datos, interbloqueos y consistencia de invariantes.
   - Implementar prueba de estrés de cancelación cooperativa asíncrona (`testCooperativeCancellationStress`):
     - Activación asíncrona de `CancellationToken` en hilos secundarios durante operaciones de cálculo y guardado.
     - Respuesta en < 10 ms y preservación de la estabilidad del servicio.
   - Implementar pruebas de fuzzing y resiliencia ante persistencia corrupta (`testCorruptedPersistenceFuzzingAndRecovery`):
     - Inyección de JSON truncado, JSON con sintaxis rota, JSON con tipos dispares y JSON con violaciones geométricas deliberadas.
     - Certificación del restablecimiento automático de `canonicalDefaultTitleBar()`.
   - Implementar pruebas de manejo de fallos de disco y permisos denegados (`testDiskIoFailureHandling`):
     - Simulación de guardado en rutas con permisos de solo lectura (`perms::read_only`).
     - Verificación de retorno limpio `false` sin abortos ni excepciones no capturadas.
   - Implementar pruebas de aislamiento estricto de incógnito (`testIncognitoStrictPrivacyIsolation`):
     - Verificación de cero operaciones de I/O en disco durante sesiones privadas.
   - Implementar prueba de alternancia continua de colocación de controles y transiciones de estado de ventana (`testControlsPlacementAndStateTransitionsExhaustive`):
     - Conmutación repetida `Left` $\leftrightarrow$ `Right`, `Maximize` $\leftrightarrow$ `Restore`, `Fullscreen` $\leftrightarrow$ `Normal`.

3. **En `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Refinar aserciones de integración de barra de título, interacción por teclado y accesibilidad:
     - Comprobación de existencia y visibilidad de `titleBarWidget_` y todos sus controles.
     - Verificación de respuesta ante eventos nativos de redimensionamiento (`resizeEvent`) y cambio de estado (`WindowStateChange`).
     - Verificación de dimensiones de botones $\ge 48\times 48$ px y altura $\ge 48$ px al activar `setSeniorMode(true)`.
     - Verificación de foco por teclado (`focusPolicy`) y atributos de accesibilidad (`accessibleName`, `accessibleDescription`).
     - Auditoría de textos públicos en runtime de UI frente a `docs/UX-COPY-PUBLICO.md`.
     - Garantía de tolerancia headless con salida limpia `NOT_EXECUTED` si falta `DISPLAY`/`WAYLAND_DISPLAY`.

---

## Diseño completo de la capacidad

### 1. Arquitectura de tres capas desacoplada y trazable

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Capa de UI (Qt Widgets)                         │
│  BrowserWindow (apps/vigilio-browser/src/ui/)                          │
│  - vigilio-titlebar (QFrame)                                           │
│  - Icono, Título, Área de arrastre, Caja de controles                  │
│  - Captura resizeEvent, changeEvent y mouseEvents                      │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Sincronización bidireccional
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                    Capa Operativa y de Servicio                        │
│  TitleBarService (apps/vigilio-browser/src/core/)                      │
│  - Estado mutable protegido por std::mutex                             │
│  - Concurrencia segura (16 hilos), Cancelación Zero-Zombie             │
│  - Persistencia atómica local con recuperación ante corrupción         │
│  - Aislamiento estricto de incógnito (ADR-001 / Q-001)                 │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Validación y cálculo analítico
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                  Capa de Dominio y Validación Pura                     │
│  TitleBarValidator & TitleBarConfiguration (src/core/)                 │
│  - Cálculo analítico de rectángulos y distribución espacial            │
│  - Invariantes geométricas y detección de fronteras ilegales           │
│  - Hit-testing analítico exhaustivo y sanitización de títulos          │
│  - Auditoría de UX Copy en español neutro (docs/UX-COPY-PUBLICO.md)   │
└────────────────────────────────────────────────────────────────────────┘
```

### 2. Modelo de pruebas determinista y de alta resiliencia

- **Aislamiento absoluto**: Las pruebas unitarias y de servicio se ejecutan en C++20 puro en el espacio de usuario, sin necesidad de servidor X11, Wayland ni inicialización del motor de Chromium WebEngine.
- **Tolerancia a fallos inyectados**: La suite evalúa activamente entradas hostiles (JSON truncado, archivos de solo lectura, strings con tokens sensibles, dimensiones negativas), garantizando que el sistema responde de forma segura, determinista y recuperable.
- **Concurrencia multihilo de alto estrés**: Se somete el servicio a 16 hilos compitiendo simultáneamente, validando que la protección mediante `std::mutex` previene cualquier condición de carrera y que no existen interbloqueos.

---

## Archivos afectados

| Archivo | Rol | Modificación proyectada en APPLY |
| --- | --- | --- |
| `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp` | Suite de pruebas de dominio | Ampliar con pruebas de fronteras exactas, geometrías extremas, hit-testing de bordes, sanitización hostil y auditoría de UX copy. |
| `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp` | Suite de pruebas de servicio | Ampliar con pruebas de estrés de 16 hilos, cancelación cooperativa, fuzzing de persistencia, fallos de disco y aislamiento de incógnito. |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Arnés de integración UI Qt | Refinar aserciones de jerarquía de widgets de barra de título, redimensionamiento reactivo, accesibilidad Senior $\ge 48$ px y textos públicos. |
| `docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md` | Documentación canónica | Actualizar estado a completado y registrar evidencias tras la ejecución de APPLY. |
| `openspec/changes/module-ux-shell-task0067/**` | Propuesta OpenSpec | Contenedor formal de la propuesta, diseño, tareas y memoria de ejecución. |

---

## Contrato UX/UI

Dado que esta etapa corresponde a **tests**, no se introducen modificaciones directas en el aspecto visual de la aplicación en producción, sino que se formaliza la verificación automatizada de los contratos de interfaz existentes:

1. **Jerarquía visual y componentes**:
   - Contenedor principal: `QFrame#vigilio-titlebar`.
   - Icono de ventana: `QLabel#vigilio-titlebar-icon` (isotipo oficial `branding/vigilio-logo.svg`, $16\times 16$ px estándar, $24\times 24$ px accesible).
   - Título de ventana: `QLabel#vigilio-titlebar-title` (fuente del sistema sobria, alineado, contraste accesible).
   - Zona ergonómica de arrastre: `QWidget#vigilio-titlebar-drag-area` (soporte de arrastre y doble clic para maximizar).
   - Caja de controles: `QWidget#vigilio-titlebar-controls`.
   - Botón toggle accesible: `QPushButton#vigilio-titlebar-senior-toggle` (etiqueta «Interfaz más fácil de usar»).
   - Botón minimizar: `QPushButton#vigilio-titlebar-minimize` («Minimizar la ventana»).
   - Botón maximizar/restaurar: `QPushButton#vigilio-titlebar-maximize` («Maximizar la ventana» / «Restaurar la ventana»).
   - Botón cerrar: `QPushButton#vigilio-titlebar-close` («Cerrar la ventana»).
2. **Adaptación reactiva a la «Interfaz más fácil de usar» (Modo Senior)**:
   - Altura de barra de título verificada en $\ge 48$ px.
   - Dimensiones de tap target de todos los botones de control verificadas en $\ge 48\times 48$ px.
   - Área mínima de arrastre ergonómica verificada en $\ge 160$ px.
3. **Navegación por teclado y foco visible**:
   - Todos los botones interactivos poseen `Qt::TabFocus` y foco visual de 2 px de alto contraste (`#2563EB`).
   - Soporte para lectores de pantalla con `accessibleName` y `accessibleDescription` en español neutro claro.
4. **Auditoría de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
   - Ausencia total de términos internos proscritos: «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG», «Linter», «toolchain».

---

## Reglas y fallos

1. **Reglas de validación estática de dimensiones**:
   - Anchura de ventana $W < 400 \implies \text{Error: } \text{DimensionsBelowMinimum}$.
   - Densidad Senior con altura $H < 48 \implies \text{Error: } \text{SeniorModeMinimumHeightViolation}$.
   - Espacio restante de arrastre $W_{\text{drag}} < W_{\text{drag,min}} \implies \text{Error: } \text{DraggableAreaInsufficient}$.
2. **Reglas de persistencia y tolerancia a fallos**:
   - Persistencia local atómica en `$LOCALAPPDATA/Vigilio/titlebar_state.json` o ruta equivalente local.
   - Si el archivo JSON no existe, está truncado, tiene sintaxis rota o valores ilegales $\implies$ el servicio restablece de inmediato `canonicalDefaultTitleBar()`, retorna `false` y continúa operando de forma estable.
   - En caso de fallo de permisos en disco (`perms::read_only`) $\implies$ la operación de guardado retorna `false` limpiamente sin generar excepciones no capturadas.
3. **Reglas de sincronización y concurrencia**:
   - Toda mutación de estado en `TitleBarService` está protegida por `std::lock_guard<std::mutex>`.
   - Las operaciones que reciben un `CancellationToken` comprueban cooperativamente `isCancellationRequested()` antes y después de cómputos pesados o I/O, abortando de inmediato si se solicita cancelación.

---

## Seguridad y privacidad

1. **Aislamiento estricto Local-First (ADR-001 / Decisión Q-001)**:
   - La barra de título no realiza peticiones de red ni telemetría remota.
   - Todos los datos de configuración residen en el equipo local del usuario.
2. **Protección de sesiones privadas (Modo Incógnito)**:
   - Cuando `isIncognitoMode() == true`, las funciones `saveStateToFile` y `loadStateFromFile` abortan de inmediato sin interactuar con el sistema de archivos local, previniendo cualquier fuga de metadatos de navegación al disco.
3. **Sanitización de títulos y protección de datos sensibles**:
   - Eliminación estricta de parámetros de consulta (`?token=...`, `?session=...`), fragmentos (`#...`), credenciales embebidas (`user:pass@`) y protocolos (`https://`) antes de desplegar el título en la barra.
   - Si el navegador está en incógnito, el título añade obligatoriamente la indicación «(Navegación privada)».

---

## Matriz de pruebas

| ID Prueba | Capacidad evaluada | Tipo | Fixture / Entrada | Resultado esperado | Criterio de verificación / Comando |
| --- | --- | --- | --- | --- | --- |
| **TEST-015** | Fronteras de anchura sub-mínima | Unitario | $W \in \{399, 350, 0, -10\}$ px | Retorno `DimensionsBelowMinimum` | `testSubMinimumDimensionsExhaustive` en `TitleBarIntegrationTests.cpp` |
| **TEST-016** | Violación de altura en interfaz accesible | Unitario | Densidad `Senior`, $H \in \{47, 40, 32\}$ px | Retorno `SeniorModeMinimumHeightViolation` | `testSeniorModeHeightViolations` en `TitleBarIntegrationTests.cpp` |
| **TEST-017** | Agotamiento de área de arrastre | Unitario | Ventana estrecha con controles inflados | Retorno `DraggableAreaInsufficient` | `testDraggableAreaExhaustion` en `TitleBarIntegrationTests.cpp` |
| **TEST-018** | Disyunción espacial en resoluciones extremas | Unitario | $3840\times 2160$, $3840\times 1080$, $1080\times 1920$ | $\forall i \ne j, R_i \cap R_j = \emptyset$, área de arrastre válida | `testSpatialDisjointnessAndExtremeResolutions` en `TitleBarIntegrationTests.cpp` |
| **TEST-019** | Hit-testing de bordes y fronteras exteriores | Unitario | Puntos en esquinas de botones y coordenadas exteriores | Detección precisa de zonas y `Nowhere` fuera de barra | `testExhaustiveHitTestingBoundaries` en `TitleBarIntegrationTests.cpp` |
| **TEST-020** | Sanitización hostil de títulos | Unitario | URLs con tokens, saltos de línea, credenciales | Título sanitizado limpio sin datos sensibles | `testTitleSanitizationEdgeCases` en `TitleBarIntegrationTests.cpp` |
| **TEST-021** | Auditoría exhaustiva de UX Copy | Unitario | Cadenas de error, tooltips, etiquetas accesibles | 0 ocurrencias de términos internos proscritos | `testPublicCopyExhaustiveAudit` en `TitleBarIntegrationTests.cpp` |
| **TEST-TBS-015** | Concurrencia masiva con 16 hilos | Integración / Servicio | 16 hilos ejecutando operaciones mixtas (200 iter/hilo) | 0 carreras de datos, 0 interbloqueos, estado consistente | `testMultiThreadedHighStressConcurrency16Threads` en `TitleBarServiceTests.cpp` |
| **TEST-TBS-016** | Cancelación cooperativa asíncrona | Integración / Servicio | `CancellationToken` disparado desde hilo secundario | Respuesta < 10 ms, aborto seguro, estado preservado | `testCooperativeCancellationStress` en `TitleBarServiceTests.cpp` |
| **TEST-TBS-017** | Fuzzing de persistencia corrupta | Integración / Servicio | JSON truncado, roto, tipos discordantes, valores ilegales | Retorno `false`, restablecimiento a `canonicalDefaultTitleBar()` | `testCorruptedPersistenceFuzzingAndRecovery` en `TitleBarServiceTests.cpp` |
| **TEST-TBS-018** | Fallos de disco y permisos denegados | Integración / Servicio | Directorio de persistencia con `perms::read_only` | Retorno `false` sin excepciones ni fallos de proceso | `testDiskIoFailureHandling` en `TitleBarServiceTests.cpp` |
| **TEST-TBS-019** | Aislamiento estricto de incógnito | Integración / Servicio | `isIncognitoMode() == true` con llamadas a guardado/carga | 0 llamadas de I/O a disco, archivo no creado | `testIncognitoStrictPrivacyIsolation` en `TitleBarServiceTests.cpp` |
| **TEST-TBS-020** | Transiciones continuas de colocación y estado | Integración / Servicio | Alternancia repetida Left/Right, Maximize/Restore | Geometría y estados consistentes en todo momento | `testControlsPlacementAndStateTransitionsExhaustive` en `TitleBarServiceTests.cpp` |
| **TEST-UI-005** | Integración reactiva de barra de título en UI | UI (Qt Test) | `BrowserWindow` con `resizeEvent` y `changeEvent` | Sincronización de dimensiones y estado maximizado | `WindowTests.cpp` |
| **TEST-UI-006** | Modo Senior y tap targets $\ge 48$ px en UI | UI (Qt Test) | Activación de `setSeniorMode(true)` en `BrowserWindow` | Altura $\ge 48$ px, botones $\ge 48\times 48$ px | `WindowTests.cpp` |
| **TEST-UI-007** | Foco por teclado y accesibilidad de barra | UI (Qt Test) | `focusPolicy` y propiedades accesibles de widgets | Foco visible, nombres y descripciones accesibles | `WindowTests.cpp` |
| **TEST-UI-008** | Auditoría de UX Copy en runtime de UI | UI (Qt Test) | Inspección de tooltips y accessibleName de widgets de barra | 0 términos proscritos en la interfaz activa | `WindowTests.cpp` |

---

## Pasos APPLY

1. **Paso 1**: Implementar los nuevos casos de prueba de dominio y validación analítica en `apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp` (fronteras dimensionales, resoluciones extremas, hit-testing de bordes, sanitización hostil y auditoría exhaustiva de UX copy).
2. **Paso 2**: Implementar los nuevos casos de prueba operativos, concurrencia de 16 hilos, cancelación cooperativa, fuzzing de persistencia, fallos de disco y aislamiento de incógnito en `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`.
3. **Paso 3**: Refinar las aserciones de integración de UI, eventos nativos de ventana, Modo Senior ($\ge 48$ px), navegación por teclado y auditoría de textos en `apps/vigilio-browser/tests/ui/WindowTests.cpp`.
4. **Paso 4**: Ejecutar formateo canónico con Clang-Format sobre todos los archivos de prueba modificados (`apps/vigilio-browser/tests/unit/TitleBarIntegrationTests.cpp`, `apps/vigilio-browser/tests/unit/TitleBarServiceTests.cpp`, `apps/vigilio-browser/tests/ui/WindowTests.cpp`).
5. **Paso 5**: Ejecutar análisis estático con Clang-Tidy y verificar ausencia de advertencias.
6. **Paso 6**: Compilar mediante CMake con `-Werror` bajo C++20 y ejecutar la suite completa en CTest comprobando que el 100% de las pruebas pasan limpiamente.
7. **Paso 7**: Actualizar el documento de microtarea `docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md` con las evidencias de ejecución y resultado verificado.

---

## Definición de terminado

- [ ] Todas las pruebas nuevas de dominio en `TitleBarIntegrationTests.cpp` compilan y pasan al 100%.
- [ ] Todas las pruebas nuevas de servicio y concurrencia de 16 hilos en `TitleBarServiceTests.cpp` compilan y pasan al 100% sin condiciones de carrera.
- [ ] Las pruebas de interfaz en `WindowTests.cpp` validan fielmente la jerarquía de `vigilio-titlebar`, redimensionamiento reactivo, tap targets $\ge 48$ px y tolerancia headless (`NOT_EXECUTED` si display no está disponible).
- [ ] Cero advertencias con `-Werror` activado en GCC/Clang bajo C++20.
- [ ] `clang-format --dry-run --Werror` reporta conformidad absoluta con el estilo de código del proyecto.
- [ ] CTest ejecuta el 100% de las pruebas con éxito (exit code 0) en tiempo total < 10 segundos.
- [ ] Cero términos internos proscritos en la auditoría de textos públicos frente a `docs/UX-COPY-PUBLICO.md`.
- [ ] La microtarea `docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md` queda debidamente sincronizada.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.ux-shell.task0066`: Completado y certificado en el commit `710169b`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` no disponible).
    *Mitigación*: `WindowTests.cpp` implementa detección temprana determinista y reporta `NOT_EXECUTED` de manera limpia, permitiendo que las pruebas de dominio y servicio en C++20 puro se ejecuten sin impedimentos.
  - *Riesgo*: Contención o interbloqueo en pruebas multihilo de alto estrés con 16 hilos.
    *Mitigación*: Uso riguroso de `std::lock_guard` sobre `std::mutex` en `TitleBarService`, limitando la sección crítica al mínimo indispensable y evitando llamadas a callbacks de usuario bajo el cerrojo.
  - *Riesgo*: Falsos positivos por flakiness en timeouts de cancelación asíncrona.
    *Mitigación*: La cancelación cooperativa evalúa directamente `std::atomic<bool>` en `CancellationToken` con latencia < 10 ms sin sleeps arbitrarios.

---

## Commit propuesto

`test(ux-shell): tests title-bar-integration`
