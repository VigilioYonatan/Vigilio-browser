# Change Proposal — module-foundations-task0031

Pack: `module.foundations.task0031`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0031 — Implementar la interacción y estados visuales en Qt: crash recovery policy`).

Tras haber formalizado el modelo de dominio en `module.foundations.task0029` y la capa de servicio operativo en C++20 (`CrashRecoveryService` / `CrashRecoveryCoordinator`) en `module.foundations.task0030`, esta etapa proyecta la **construcción de la interacción nativa Qt Widgets, presentación visual, banner accesible de recuperación, estados de interfaz, navegación por teclado, soporte de tecnologías asistivas y escalado en la «Interfaz más fácil de usar» (Modo Senior)**.

El objetivo central de este pack es proporcionar una experiencia de usuario clara, empática y no intrusiva ante cierres anómalos previos de la aplicación, permitiendo al usuario decidir de forma consciente si desea restaurar sus pestañas anteriores o iniciar una sesión en blanco, **sin bloquear el hilo de la interfaz gráfica, sin invocar instancias innecesarias de QWebEngineView y respetando rigurosamente el aislamiento local-first y de navegación privada (incógnito)**.

El alcance concreto de este pack comprende:
- **Componente visual de banner de recuperación (`vigilio-crash-recovery-banner`)**:
  - Implementación en Qt Widgets (`QFrame`) insertado en la parte superior del contenedor central de `BrowserWindow` (entre la barra de navegación y el contenedor de pestañas `tabs_`, coexistiendo armónicamente con `safeModeBanner_`).
  - Jerarquía visual sobria y profesional acorde a la dirección artística (`/resultado/01-vigilio-home.png`): fondo azul pizarra (`#1e293b` oscuro, `#f1f5f9` claro), borde suave (`#334155` / `#cbd5e1`), icono vectorial Lucide (`icons/tools/shield.svg` o `icons/navigation/reload.svg`), tipografía Inter/sistema y espaciado proporcional.
- **Acciones y botones interactivos**:
  - Botón primario de restauración (`vigilio-crash-recovery-restore`): texto «Restaurar pestañas», acciona `CrashRecoveryService::applyRecovery()` y abre las URLs en pestañas con carga diferida (lazy loading).
  - Botón secundario de descarte (`vigilio-crash-recovery-discard`): texto «Empezar de cero», acciona `CrashRecoveryService::discardRecovery()`, archiva la sesión previa y enfoca la barra de direcciones.
  - Botón o atajo de cierre/descarte del aviso (`vigilio-crash-recovery-dismiss`): texto «Cerrar aviso» y atajo de teclado `Escape`, acciona `CrashRecoveryService::cancelRecovery()`, oculta el banner y preserva el estado en memoria para decisiones posteriores.
- **Diferenciación contextual por causa y bucle de caídas (Crash Loop)**:
  - Cierre inesperado estándar (`AbnormalTermination` / `PromptRestore`): mensaje natural «El navegador no se cerró correctamente. ¿Quieres restaurar tus pestañas anteriores?».
  - Bucle repetido de caídas (`RepeatedCrashes` / $\ge 2$ caídas consecutivas sin sesión estable): advertencia clara «El navegador se cerró varias veces de forma inesperada. Se recomienda abrir una sesión limpia para mayor estabilidad», priorizando visualmente el botón «Empezar de cero».
- **Integración con la «Interfaz más fácil de usar» (Modo Senior)**:
  - Adaptación dinámica de tamaños y espacios según `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`:
    - Altura mínima de botones $\ge 48$ px (frente a 32 px estándar).
    - Altura mínima del contenedor del banner $\ge 56$ px (frente a 42 px estándar).
    - Área de clic expandida y tipografía aumentada (14-16 pt).
    - Iconos escalados a 28x28 px con alto contraste.
- **Accesibilidad y navegación por teclado**:
  - Atributos completos `accessibleName` y `accessibleDescription` en todos los controles interactivos.
  - Notificación de alerta a lectores de pantalla (`QAccessible::Alert` o descripción inicial accesible).
  - Navegación lógica con `Tab` / `Shift+Tab`, activación con `Enter` / `Espacio`, y descarte rápido con tecla `Escape`.
- **Integración de ciclo de vida y latido periódico en UI**:
  - Enlace en `BrowserWindow` con `CrashRecoveryService`: inspección en arranque, conexión de `QTimer` periódico (30 s) para `recordHeartbeat`, y finalización ordenada en `closeEvent`.
- **Aislamiento constitucional de Incógnito (Decisión Q-001)**:
  - En sesiones privadas (`isIncognito_ == true`), el banner de recuperación **nunca** se construye ni se muestra, garantizando privacidad total y cero persistencia de datos.
- **Eficiencia de memoria y arranque (Carga diferida / Lazy Loading)**:
  - Al restaurar pestañas, solo la primera pestaña asigna un proceso `QWebEngineView`; las pestañas restantes se crean como marcadores diferidos (`deferredUrl`), evitando saturación de RAM (per `docs/skills/06-rendimiento-y-ram.md`).
- **Suite de pruebas de interacción Qt (`WindowTests.cpp`)**:
  - Casos verificables para presencia/ausencia del banner, textos de crash loop, acciones de restaurar/descartar/cerrar, escalado en Modo Senior y aislamiento en incógnito.

---

## Out of scope

- Modificación de la lógica interna de persistencia del centinela en `src/core/CrashRecoveryCoordinator.cpp` (formalizada y completada en las microtareas `0029` y `0030`).
- Implementación de pruebas E2E con inyección de señales del sistema operativo (`SIGSEGV`, `SIGKILL`, crash helpers del proceso Chromium WebEngine), reservado para la etapa `tests` en `module.foundations.task0032`.
- Diálogos modales bloqueantes que impidan la navegación mientras el usuario decide: el diseño utiliza un banner no modal integrado en el layout para no interferir en el uso del navegador.
- Envío de telemetría, volcados de memoria (minidumps/coredumps) o reportes a servidores externos (prohibido por la política local-first, decisión Q-001).
- Modificación directa de archivos de código fuente de producción (`apps/**`) durante esta fase PROPOSE (restringida exclusivamente a `openspec/changes/module-foundations-task0031/**`).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la interfaz gráfica existente y las pruebas automatizadas:
1. **Especificación de dominio y capa de servicio previa (`task0029` y `task0030`)**:
   - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h` define `CrashDetectionState`, `RecoveryAction`, `CrashReason`, `RecoveryServiceState`, `RecoveryDecision`, `CrashSentinel`, `CrashRecoveryConfig` y `CrashRecoveryReport`.
   - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` y `.cpp` implementan el servicio `CrashRecoveryService` con operaciones `initialize()`, `applyRecovery()`, `discardRecovery()`, `cancelRecovery()`, `recordHeartbeat()` y `finalizeShutdown()`.
2. **Estructura y diseño de UI existente (`apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`)**:
   - `BrowserWindow` contiene `tabs_` (`QTabWidget`), `toolbar_` (`QToolBar`), `address_` (`QLineEdit`), `safeModeBanner_` (`QWidget*`), e `incognitoBadge_` (`QLabel*`).
   - El método `setupSafeModeBanner()` provee un patrón probado para banners accesibles no modales en el layout principal (`centralLayout->insertWidget(0, banner)`).
   - Actualmente, `BrowserWindow::restoreSession()` lee directamente de `QSettings` sin consultar `CrashRecoveryService`, y no existe ningún widget ni banner para interactuar con cierres anómalos previos.
   - En `BrowserWindow::setSeniorMode(bool)`, los controles escalan dinámicamente sus tamaños mínimos (`QLineEdit` $\ge 48$ px, `QPushButton` $\ge 48$ px, `QToolBar` $\ge 55$ px, iconos $32\times 32$ px).
3. **Referencias visuales y UX (`/resultado/`)**:
   - `/resultado/01-vigilio-home.png`: Layout general con pestañas, barra de navegación y área de contenido.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar, tipografía clara, espaciado generoso y alto contraste.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».
4. **Skills y directrices de ingeniería**:
   - `docs/skills/03-codigo-cpp-qt.md`: Desacoplamiento de lógica en `core`, RAII, propiedad clara de widgets Qt, no bloqueo del hilo de UI.
   - `docs/skills/04-ux-ui-senior.md` y `05-accesibilidad-y-senior-mode.md`: Estilo moderno, sobrio, azul oscuro base, iconos SVG consistentes, controles accesibles, navegación por teclado completa y foco visible.
   - `docs/skills/06-rendimiento-y-ram.md`: Carga diferida de pestañas restauradas; evitar instanciar múltiples procesos WebEngine simultáneos.
   - `docs/skills/07-seguridad-y-privacidad.md`: Aislamiento estricto de modo incógnito; cero telemetría remota.
   - `docs/skills/08-observabilidad-calidad-linters.md`: Verificación continua con CTest, ClangFormat y Clang-Tidy.
5. **Verificación de pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
   - 11/11 tests pasando exitosamente en 0.50s (exit code 0).
6. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `validate-references`: 31 referencias canónicas validadas (`valid: true`, exit code 0).
   - `verify-lock`: 13/13 comprobaciones aprobadas sobre el lockfile de 0091 (`valid: true`, exit code 0).
   - `validate-decisions`: Confirma que las decisiones Q-001 a Q-006 están resueltas (Q-007 solo bloquea `release.public-release`).

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0031`.
- **Capacidad**: `crash recovery policy`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0031/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0031/proposal.md`
  - `openspec/changes/module-foundations-task0031/design.md`
  - `openspec/changes/module-foundations-task0031/tasks.md`
  - `openspec/changes/module-foundations-task0031/execution-memory.md`

---

## Diseño completo de la capacidad

La solución integral de **crash recovery policy** en su etapa **ui** articula la interacción entre el núcleo operativo y la presentación gráfica a través de los siguientes componentes:

### 1. Banner de recuperación de sesión (`vigilio-crash-recovery-banner`)
Un widget nativo de Qt (`QFrame`) insertado en el layout vertical de `BrowserWindow`, situado directamente encima de las pestañas (`tabs_`) y debajo de cualquier aviso prioritario de seguridad (como `safeModeBanner_`):
- **Estructura del widget**:
  - `QFrame* crashRecoveryBanner_`
  - Layout horizontal `QHBoxLayout` con márgenes `(12, 8, 12, 8)` y espaciado de 10 px.
  - Icono indicador: `QLabel` (`vigilio-crash-recovery-icon`) mostrando SVG de `icons/tools/shield.svg` o `icons/navigation/reload.svg`.
  - Etiqueta explicativa: `QLabel` (`vigilio-crash-recovery-label`) con texto multilínea descriptivo en lenguaje natural.
  - Botón de restauración: `QPushButton` (`vigilio-crash-recovery-restore`), estilo primario destacado.
  - Botón de descarte: `QPushButton` (`vigilio-crash-recovery-discard`), estilo secundario neutro.
  - Botón de cierre: `QPushButton` (`vigilio-crash-recovery-dismiss`), botón de descarte rápido con atajo `Escape`.
- **Jerarquía y coexistencia**:
  - Si coincide con modo seguro (`safeModeBanner_`), ambos banners se apilan ordenadamente en la parte superior sin solaparse ni desplazar controles de navegación.

### 2. Flujo de detección e inicialización en `BrowserWindow`
- Durante la construcción de `BrowserWindow`:
  1. Si `isIncognito_ == true`: la recuperación tras caídas se omite enteramente (cero UI, cero persistencia).
  2. Si es sesión ordinaria o en modo seguro:
     - Se inicializa `vigilio::CrashRecoveryService` con la configuración del perfil (`sentinel.json` y `session.json`).
     - Se invoca `crashRecoveryService_->initialize(config, pid, now)`.
     - Si el reporte indica `CrashDetectionState::AbnormalTermination` o `RepeatedCrashes` (o acción recomendada `PromptRestore` / `SafeModeRecovery`):
       - Se invoca `setupCrashRecoveryBanner(report)`.
       - Si no hay pestañas abiertas aún por línea de comandos, se abre una pestaña vacía (Home) para permitir al usuario usar el navegador de inmediato.
     - Si el reporte indica salida limpia (`CleanExit`) o sin sesión previa (`NoPreviousSession`):
       - El banner no se crea ni se muestra.
       - Si la configuración del navegador indica restaurar sesión ordinaria, se cargan las pestañas normalmente.
  3. Se inicia un `QTimer` con intervalo de 30 segundos conectado a `recordHeartbeat(now)` para mantener actualizado el centinela durante la sesión activa.

### 3. Resolución interactiva de decisiones
- **Acción Restaurar (`vigilio-crash-recovery-restore`)**:
  - Deshabilita temporalmente los botones del banner para evitar dobles pulsaciones.
  - Invoca `crashRecoveryService_->applyRecovery()`, obteniendo la lista de URLs saneadas.
  - Inserta las pestañas correspondientes en `tabs_`:
    - La primera pestaña abre o activa su contenido.
    - Las pestañas sucesivas se configuran con carga diferida (`setProperty("deferredUrl", url)`), consumiendo 0 procesos WebEngine adicionales hasta que el usuario las seleccione.
  - Oculta el banner (`crashRecoveryBanner_->hide()`).
  - Muestra un mensaje temporal en la barra de estado: «Pestañas restauradas con éxito».
  - Devuelve el foco a la primera pestaña restaurada.
- **Acción Descartar / Empezar de cero (`vigilio-crash-recovery-discard`)**:
  - Invoca `crashRecoveryService_->discardRecovery()`, archivando la sesión caída de forma segura a `.discarded.bak`.
  - Oculta el banner (`crashRecoveryBanner_->hide()`).
  - Si no hay pestañas abiertas, abre una pestaña nueva (Home).
  - Muestra un mensaje temporal en la barra de estado: «Sesión anterior descartada».
  - Devuelve el foco a la barra de direcciones (`address_->setFocus()`).
- **Acción Cerrar aviso (`vigilio-crash-recovery-dismiss` / Tecla `Escape`)**:
  - Invoca `crashRecoveryService_->cancelRecovery()`.
  - Oculta el banner sin eliminar el archivo de sesión en disco, permitiendo al usuario restaurar pestañas posteriormente desde el menú de opciones si lo desea.
  - Devuelve el foco a la barra de direcciones.

### 4. Coordinación con el ciclo de vida en el apagado
- En `BrowserWindow::closeEvent`:
  - `lifetimeCoordinator_->executeShutdown(...)` coordina el guardado de la sesión activa y el cierre de la base de datos.
  - Si el apagado concluye con éxito (`cleanExit == true`), se invoca `crashRecoveryService_->finalizeShutdown(true, CrashReason::CleanShutdown)` para asentar `clean_exit = true` en el centinela.

---

## Archivos afectados

### Durante PROPOSE (etapa actual)
- `openspec/changes/module-foundations-task0031/canonical-reference.yaml`: Archivo de referencia canónica v1.
- `openspec/changes/module-foundations-task0031/proposal.md`: Propuesta técnica integral de la etapa UI.
- `openspec/changes/module-foundations-task0031/design.md`: Diseño detallado de componentes, estados, diagramas y tokens.
- `openspec/changes/module-foundations-task0031/tasks.md`: Lista secuencial y determinista de tareas verificables con comandos y criterios.
- `openspec/changes/module-foundations-task0031/execution-memory.md`: Memoria de ejecución, autoridades, skills, invariantes y handoff.

### Durante APPLY (próxima ejecución `0092`)
- `apps/vigilio-browser/src/ui/BrowserWindow.h`: Declaración del banner `crashRecoveryBanner_`, servicio `crashRecoveryService_`, temporizador de latido `heartbeatTimer_`, y métodos de configuración y manejo de eventos.
- `apps/vigilio-browser/src/ui/BrowserWindow.cpp`: Implementación de `setupCrashRecoveryBanner()`, manejadores de botones, integración de latido periódico, escalado en `setSeniorMode()` y finalización en `closeEvent()`.
- `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Nuevos casos de prueba para verificar la creación del banner, textos, interacción de botones, Modo Senior, navegación por teclado y aislamiento de incógnito.

---

## Contrato UX/UI

### 1. Ubicación y jerarquía visual
- **Posición**: Insertado dinámicamente en `centralLayout` en el índice 0 (o índice 1 si `safeModeBanner_` está activo), situado justo debajo de la barra de navegación y encima de la barra de pestañas.
- **Dimensiones estándar**:
  - Altura mínima: 42 px.
  - Márgenes de contenido: 12 px horizontal, 8 px vertical.
  - Espaciado entre elementos: 10 px.
- **Dimensiones en «Interfaz más fácil de usar» (Modo Senior)**:
  - Altura mínima: 56 px.
  - Altura mínima de botones: $\ge 48$ px.
  - Márgenes de contenido: 16 px horizontal, 12 px vertical.
  - Espaciado entre elementos: 14 px.

### 2. Textos públicos aprobados (cumpliendo `docs/UX-COPY-PUBLICO.md`)
- **Caída anómala estándar (`AbnormalTermination` / `PromptRestore`)**:
  - Etiqueta: «El navegador no se cerró correctamente. ¿Quieres restaurar tus pestañas anteriores?»
  - Botón primario: «Restaurar pestañas»
  - Botón secundario: «Empezar de cero»
  - Botón descartar: «Cerrar aviso»
- **Bucle de caídas repetidas (`RepeatedCrashes` / `SafeModeRecovery`)**:
  - Etiqueta: «El navegador se cerró varias veces de forma inesperada. Se recomienda abrir una sesión limpia para mayor estabilidad.»
  - Botón primario: «Empezar de cero»
  - Botón secundario: «Restaurar pestañas de todos modos»
  - Botón descartar: «Cerrar aviso»
- **Aislamiento en incógnito**:
  - Banner completamente ausente. Barra de estado muestra: «Vigilio Browser · Navegación privada · No se guardan datos locales».

### 3. Paleta de colores y tokens de tema
- **Tema Oscuro / Medianoche**:
  - Fondo del banner: `#1e293b` (Slate 800).
  - Borde: 1px solid `#334155` (Slate 700).
  - Texto principal: `#f8fafc` (Slate 50).
  - Botón primario: Fondo `#2563eb` (Blue 600), texto `#ffffff`, hover `#1d4ed8`.
  - Botón secundario: Fondo `#334155` (Slate 700), texto `#e2e8f0`, hover `#475569`.
  - Borde de foco: 2px solid `#60a5fa` (Blue 400).
- **Tema Claro**:
  - Fondo del banner: `#f1f5f9` (Slate 100).
  - Borde: 1px solid `#cbd5e1` (Slate 300).
  - Texto principal: `#0f172a` (Slate 900).
  - Botón primario: Fondo `#2563eb`, texto `#ffffff`, hover `#1d4ed8`.
  - Botón secundario: Fondo `#e2e8f0`, texto `#1e293b`, hover `#cbd5e1`.
  - Borde de foco: 2px solid `#2563eb`.

### 4. Estados interactivos del componente
- `idle`: Banner visible con contraste regulado, botones listos para interacción.
- `focus`: Borde de enfoque de 2 px visible en el botón actualmente activo según navegación por teclado.
- `hover`: Cambio sutil de tono de fondo en los botones para confirmar interacción del cursor.
- `disabled`: Durante el proceso de apertura de pestañas, los botones se deshabilitan para evitar condiciones de carrera.
- `loading`: Si la carga de URLs toma varios milisegundos, el botón primario muestra estado ocupado.
- `empty`: Si la sesión no contiene URLs válidas, el banner se omite automáticamente.
- `success`: Confirmación transitoria en la barra de estado y cierre animado/inmediato del banner.
- `error`: Si el archivo de sesión estaba irremediablemente corrupto, el banner informa «No se pudieron recuperar las pestañas dañadas» con un único botón «Aceptar».

### 5. Accesibilidad y Tecnologías Asistivas
- `accessibleName` asignado en banner y cada botón.
- `accessibleDescription` con explicación detallada de la consecuencia de cada acción.
- Atajo de teclado directo `Escape` para cerrar el aviso sin realizar cambios destructivos.
- Secuencia lógica de tabulación (`Tab` / `Shift+Tab`) circulando por los controles del banner antes de pasar a la barra de direcciones.

---

## Reglas y fallos

1. **Regla de oro de Incógnito (Q-001)**:
   - Si la sesión se inició con `--incognito`, el banner de crash recovery nunca se crea ni se muestra en pantalla bajo ninguna circunstancia.
2. **Regla de prioridad de Modo Seguro**:
   - Si `isSafeMode_ == true` y coincide con una recuperación tras caídas, `safeModeBanner_` se posiciona en el tope y `crashRecoveryBanner_` inmediatamente debajo.
3. **Regla de Crash Loop**:
   - Ante $\ge 2$ caídas consecutivas, el botón prioritario sugerido cambia a «Empezar de cero» para evitar que pestañas defectuosas sigan provocando bloqueos del navegador.
4. **Manejo de entradas vacías o URLs malformadas**:
   - `CrashRecoveryCoordinator::sanitizeRecoverableUrls` descarta esquemas no autorizados (`javascript:`, `data:`, `file:`) y limita la longitud a 8192 bytes por URL y 1000 pestañas.
5. **Manejo de fallos de E/S o permisos denegados**:
   - Si `sentinel.json` o `session.json` no pueden leerse o modificarse, la UI degrada silenciosamente a inicio limpio sin lanzar excepciones no capturadas ni bloquear la interfaz.
6. **Manejo de concurrencia y latido**:
   - El temporizador de latido se ejecuta en el hilo principal mediante `QTimer`, garantizando que la actualización atómica del centinela no interfiera con eventos del usuario.

---

## Seguridad y privacidad

- **Privacidad Local-First constitucional (Q-001)**:
  - Cero conexiones de red, cero solicitudes de telemetría y cero envío de volcados de memoria a servidores externos.
  - La inspección, el centinela y la decisión de recuperación ocurren íntegramente en el equipo local del usuario.
- **Aislamiento de Incógnito**:
  - Los datos de pestañas abiertas en modo incógnito nunca tocan el archivo de sesión ni el centinela, imposibilitando la persistencia accidental de historial privado tras una caída.
- **Saneamiento estricto de esquemas**:
  - Solo se restauran URLs con esquemas `http://`, `https://` o `vigilio://`. Esquemas con capacidad de inyección de código como `javascript:` son purgados antes de llegar a la UI.
- **Presupuestos de memoria y recursos**:
  - La carga diferida de pestañas restauradas asegura que un usuario que recupere 50 pestañas no consuma gigabytes de RAM instantáneamente ni sature la GPU.

---

## Matriz de pruebas

La siguiente matriz define las pruebas verificables para esta etapa UI, categorizadas según su ámbito de ejecución:

| ID de Prueba | Ámbito | Componente / Fixture | Entrada / Condición | Resultado Esperado | Modo de Fallo Negativo |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `UI-TEST-001` | Qt UI | `BrowserWindow` con reporte `AbnormalTermination` | Inicio tras cierre abrupto simulado | Banner `vigilio-crash-recovery-banner` visible en ventana | Banner nulo o no visible |
| `UI-TEST-002` | Qt UI | `BrowserWindow` con reporte `CleanExit` | Inicio tras cierre limpio previo | Banner ausente (`crashRecoveryBanner_ == nullptr` o no visible) | Banner mostrado indebidamente |
| `UI-TEST-003` | Qt UI / Seg | `BrowserWindow` en modo `--incognito` | Inicio en incógnito con rastro previo | Banner no se crea; estricto aislamiento | Fuga de UI en incógnito |
| `UI-TEST-004` | Qt UI | Botón `vigilio-crash-recovery-restore` | Clic en «Restaurar pestañas» | Invoca `applyRecovery()`, agrega pestañas diferidas, oculta banner | Pestañas no creadas o banner visible |
| `UI-TEST-005` | Qt UI | Botón `vigilio-crash-recovery-discard` | Clic en «Empezar de cero» | Invoca `discardRecovery()`, oculta banner, enfoca barra de direcciones | Sesión no descartada |
| `UI-TEST-006` | Qt UI | Botón `vigilio-crash-recovery-dismiss` / `Escape` | Clic en cerrar aviso o tecla Escape | Oculta banner sin borrar sesión; foco a barra de direcciones | Atajo inactivo o banner fijo |
| `UI-TEST-007` | Qt UI / UX | Reporte con `consecutiveCrashes >= 2` | Inicio tras crash loop | Texto de banner enfatiza estabilidad y recomienda empezar de cero | Texto estándar sin aviso de bucle |
| `UI-TEST-008` | Qt UI / A11y | `setSeniorMode(true)` sobre banner | Activación de Modo Senior | Botones con altura $\ge 48$ px, banner $\ge 56$ px, iconos $28\times 28$ | Dimensiones menores a 48 px |
| `UI-TEST-009` | Qt UI / A11y | Inspección de metadatos accesibles | Banner y botones inicializados | `accessibleName` y `accessibleDescription` no vacíos | Cadenas accesibles vacías |
| `UI-TEST-010` | Qt UI / A11y | Simulación de eventos de teclado | `Tab`, `Shift+Tab`, `Return` en banner | Foco cíclico observable entre botones del banner | Foco atrapado o inaccesible |
| `UI-TEST-011` | Qt UI / Perf | Restauración de múltiples URLs (ej. 5) | Clic en «Restaurar pestañas» | Solo pestaña 0 activa; pestañas 1-4 diferidas (`deferredUrl`) | Asignación de 5 QWebEngineView |
| `UI-TEST-012` | Qt UI / Vida | Temporizador `heartbeatTimer_` | Sesión activa en ejecución | Tics periódicos cada 30s invocan `recordHeartbeat` | Temporizador detenido o inactivo |

---

## Pasos APPLY

Plan de ejecución secuencial y determinista para la fase APPLY (`module.foundations.task0031` en ejecución `0092`):

1. **Declaración en `BrowserWindow.h`**:
   - Incorporar `#include "core/CrashRecoveryCoordinator.h"`.
   - Declarar punteros miembro: `std::unique_ptr<vigilio::CrashRecoveryService> crashRecoveryService_`, `QWidget* crashRecoveryBanner_{nullptr}`, `QTimer* heartbeatTimer_{nullptr}`.
   - Declarar métodos privados:
     - `void setupCrashRecovery(const vigilio::StartupCommandLine& cmdLine);`
     - `void setupCrashRecoveryBanner(const vigilio::CrashRecoveryReport& report);`
     - `void handleCrashRecoveryRestore();`
     - `void handleCrashRecoveryDiscard();`
     - `void handleCrashRecoveryDismiss();`
   - Declarar getter público de prueba: `[[nodiscard]] QWidget* crashRecoveryBanner() const noexcept { return crashRecoveryBanner_; }` y `[[nodiscard]] vigilio::CrashRecoveryService* crashRecoveryService() const noexcept`.
2. **Implementación de inicialización y banner en `BrowserWindow.cpp`**:
   - En el constructor `BrowserWindow::BrowserWindow`, instanciar y configurar `CrashRecoveryService` salvo si `isIncognito_ == true`.
   - Implementar `setupCrashRecoveryBanner` creando el `QFrame` con `setObjectName("vigilio-crash-recovery-banner")`, layout horizontal, icono de advertencia, etiqueta accesible en español, botón «Restaurar pestañas», botón «Empezar de cero» y botón «Cerrar aviso».
   - Conectar atajo `Key_Escape` para descarte accesible.
   - Insertar el banner en `centralLayout` en la posición superior.
3. **Implementación de manejadores de acción en `BrowserWindow.cpp`**:
   - `handleCrashRecoveryRestore`: invoca `applyRecovery()`, itera sobre URLs recuperables creando la primera como pestaña activa y las demás como pestañas diferidas, oculta el banner y actualiza la barra de estado.
   - `handleCrashRecoveryDiscard`: invoca `discardRecovery()`, oculta el banner, enfoca `address_` y actualiza la barra de estado.
   - `handleCrashRecoveryDismiss`: invoca `cancelRecovery()`, oculta el banner y enfoca `address_`.
4. **Implementación del latido periódico y cierre ordenado en `BrowserWindow.cpp`**:
   - Configurar `heartbeatTimer_` con intervalo de 30.000 ms conectado a una lambda que invoca `crashRecoveryService_->recordHeartbeat(now)`.
   - En `closeEvent`, asegurar que tras `lifetimeCoordinator_->executeShutdown`, si `report.cleanExit == true`, se invoque `crashRecoveryService_->finalizeShutdown(true, CrashReason::CleanShutdown)`.
5. **Soporte de escalado dinámico en `BrowserWindow::setSeniorMode(bool)`**:
   - Ajustar altura mínima del banner ($\ge 56$ px en Senior Mode, 42 px estándar).
   - Ajustar altura mínima de los botones del banner ($\ge 48$ px en Senior Mode, 32 px estándar).
6. **Ampliación de la suite de pruebas en `WindowTests.cpp`**:
   - Añadir casos de prueba verificando la creación del banner ante reporte de caída, ausencia en salida limpia, aislamiento en incógnito, clics en restaurar y descartar, escalado en Modo Senior y navegación por teclado.
7. **Verificación de calidad y gates**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` asegurando compilación Ninja limpia en C++20, ClangFormat y 100% de tests pasando en CTest.

---

## Definición de terminado

La etapa UI de la capacidad **crash recovery policy** se considerará completa cuando:
1. El banner `vigilio-crash-recovery-banner` se presente de manera clara y no intrusiva al iniciar el navegador tras un cierre inesperado.
2. Los botones «Restaurar pestañas», «Empezar de cero» y «Cerrar aviso» ejecuten sus contratos funcionales sin errores.
3. En sesiones de navegación privada (incógnito), el banner nunca se construya ni aparezca.
4. En Modo Senior («Interfaz más fácil de usar»), el banner y sus botones escalen con áreas de clic amplias ($\ge 48$ px) y texto de alto contraste.
5. Se cumpla rigurosamente `docs/UX-COPY-PUBLICO.md` sin utilizar términos de ingeniería en la interfaz visible.
6. La suite completa de pruebas en `WindowTests.cpp` y el pipeline `check.sh` se ejecuten con código de salida 0.
7. Las validaciones de Prompt Builder (`validate-references`, `verify-lock`) aprueben todas las comprobaciones.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0030`: Completada satisfactoriamente, proveyendo la capa de servicio `CrashRecoveryService` y `CrashRecoveryCoordinator`.
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Apertura simultánea de un gran volumen de pestañas restauradas que agote la RAM o bloquee la interfaz gráfica.
    *Mitigación*: Aplicar la regla de carga diferida (lazy loading), inicializando únicamente la primera pestaña con WebEngine y dejando las demás como marcadores ligeros.
  - *Riesgo*: Ambigüedad visual cuando coexisten el banner de modo seguro y el banner de recuperación tras caídas.
    *Mitigación*: Apilamiento vertical ordenado en `centralLayout`, manteniendo jerarquía clara y atajos `Escape` independientes.
  - *Riesgo*: Violación de privacidad en navegación privada.
    *Mitigación*: Guarda de comprobación estricta al inicio de `setupCrashRecovery`: si `isIncognito_ == true`, cortocircuitar de inmediato sin instanciar widgets ni tocar disco.

---

## Commit propuesto

`feat(foundations): ui crash-recovery-policy`
