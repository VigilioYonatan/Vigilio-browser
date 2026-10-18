# Change Proposal — module-foundations-task0019

Pack: `module.foundations.task0019`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **resource qrc inclusion** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en las microtareas precedentes la especificación del modelo de dominio (`module.foundations.task0017`) y la implementación de la capa de servicio C++20 desacoplada con validación en arranque y fallbacks deterministas (`module.foundations.task0018`), esta microtarea proyecta la **construcción de la interacción Qt Widgets, estados visuales, iconografía vectorial SVG, atajos de teclado, tecnología asistiva y adaptación para «Interfaz más fácil de usar» (Modo Senior)**, integrando los 35 recursos compilados de forma segura y accesible en la interfaz gráfica del navegador.

El alcance concreto de este pack comprende:
- **Integración de iconografía vectorial QRC en la ventana principal (`BrowserWindow`)**:
  - Asignación del icono corporativo de la aplicación (`branding/vigilio-logo.svg`) a la ventana mediante `setWindowIcon()`, resolviendo la ruta virtual canónica a través de `vigilio::ResourceInclusionService::resolveIconWithFallback()`.
  - Equipamiento de todas las acciones de navegación de la barra de herramientas (`QToolBar` `"vigilio-navigation"`) con sus correspondientes iconos vectoriales Lucide SVG (viewBox `0 0 24 24`, trazo 1.8 px):
    - Acción Atrás: `icons/navigation/back.svg`
    - Acción Adelante: `icons/navigation/forward.svg`
    - Acción Recargar: `icons/navigation/reload.svg`
    - Acción Inicio: `icons/navigation/home.svg`
    - Acción Guardar en Favoritos: `icons/navigation/bookmarks.svg`
    - Acción Lista de Favoritos: `icons/navigation/bookmarks.svg`
    - Acción Nueva Pestaña: `icons/navigation/tabs.svg`
    - Acción Menú de Opciones: `icons/navigation/settings.svg`
  - Incorporación de iconos contextuales en el menú principal (`mainMenu`):
    - Opción «Interfaz más fácil de usar»: `icons/tools/senior.svg`
    - Opción «Tema oscuro»: `icons/navigation/settings.svg`
- **Estados visuales e interacción completa en widgets Qt**:
  - *Estado vacío (`newHomePage`)*: Presencia del branding visual `branding/vigilio-logo.svg` junto al título «Vigilio Browser» y el buscador centralizado, creando una jerarquía limpia y sobria alineada con `/resultado/01-vigilio-home.png`.
  - *Estado de carga (`loading`)*: Indicador en la pestaña («Cargando…») y estado en la barra de estado («Cargando…»), con retroalimentación visual no bloqueante en el hilo de interfaz.
  - *Estado de error (`createErrorPageWidget`)*: Enriquecimiento de la página de error con el icono vectorial de advertencia/escudo (`icons/tools/shield.svg`), título «No se pudo cargar la página», mensaje claro y botones de acción primaria («Reintentar») y secundaria («Ir al inicio»).
  - *Estados interactivos de botones y acciones*: Definición y soporte para estados `idle`, `hover`, `focus` (con anillo o contorno visible de 2 px), `disabled` (para acciones de historial cuando no hay páginas previas o siguientes) y `active`.
- **Adaptabilidad dinámica para «Interfaz más fácil de usar» (Senior Mode)**:
  - Escalado de los iconos de la barra de herramientas: transición automática de tamaño estándar (22×22 px) a tamaño accesible ampliado (32×32 px) mediante `toolbar->setIconSize()`.
  - Dimensionamiento de áreas táctiles y de clic: preservación de alturas mínimas >= 48 px en botones (`QPushButton`) y campos de texto (`QLineEdit`), y >= 55 px en barras de herramientas (`QToolBar`).
  - Escalado tipográfico (+4 pt) manteniendo la legibilidad sin truncar etiquetas ni desbordar layouts.
- **Accesibilidad integral y lectores de pantalla (tecnología asistiva)**:
  - Asignación sistemática de propiedades `setAccessibleName` y `setAccessibleDescription` a todas las acciones y botones que utilicen iconos, garantizando que lectores de pantalla como ORCA o NVDA anuncien descripciones en español natural conforme a `docs/UX-COPY-PUBLICO.md`.
  - Mapeo completo de atajos de teclado accesibles: `Alt+Left` / `Ctrl+[` (Atrás), `Alt+Right` / `Ctrl+]` (Adelante), `Ctrl+R` / `F5` (Recargar), `Alt+Home` (Inicio), `Ctrl+T` (Nueva pestaña), `Ctrl+D` (Favorito), `Ctrl+B` (Favoritos) y `Ctrl+L` (Barra de direcciones).
- **Resiliencia visual y fallbacks deterministas**:
  - Todas las resoluciones de iconos se efectúan mediante el servicio `vigilio::ResourceInclusionService::resolveIconWithFallback()`, asegurando que ante cualquier alias anómalo o recurso faltante se utilice el icono genérico seguro `":/vigilio/icons/tools/tools.svg"`, evitando botones en blanco, punteros nulos o fallos en cascada.
- **Suite de pruebas de interacción Qt (`WindowTests.cpp`)**:
  - Incorporación de aserciones en CTest (`window_ui_tests`) para verificar:
    - Asignación efectiva de iconos a las acciones de la barra de herramientas.
    - Escalado de iconos y controles al conmutar «Interfaz más fácil de usar».
    - Coherencia de nombres y descripciones de accesibilidad.
    - Recuperación mediante fallback ante iconos inexistentes.
    - Tolerancia headless en CI (ejecución limpia con reporte `NOT_EXECUTED` o mock en ausencia de display).

## Out of scope

- Modificación de la capa de dominio (`ResourceQrcCatalog`) o de los métodos de servicio de `ResourceInclusionService`, los cuales quedaron consolidados en las tareas 0017 y 0018.
- Pruebas E2E pesadas con procesos multiproceso de Chromium o WebEngine interactuando contra la red exterior, diferidas a `module.foundations.task0020`.
- Construcción de pantallas secundarias complejas (panel lateral completo de productividad, visor Markdown avanzado o gestor MCP de extensiones), que corresponden a sus respectivos módulos dedicados.
- Diálogos del asistente de instalación (`04-vigilio-installer.png`) o desinstalador (`05-vigilio-uninstaller.png`), reservados para los paquetes de release (`module.foundations.task0053` a `task0056`).
- Edición de archivos de producción bajo `apps/**` o reglas en `docs/rules/**` durante esta fase PROPOSE (restringida a `openspec/changes/module-foundations-task0019/**`).
- Cualquier llamada de red, descarga remota de fuentes o telemetría en la nube (prohibido por arquitectura).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código existente en `apps/vigilio-browser`, los módulos de dominio previos y las herramientas locales:
1. **Servicio y catálogo de recursos QRC existente (`src/core/ResourceInclusionService.h`, `ResourceQrcCatalog.h`)**:
   - Consolidado en `module.foundations.task0018` e integrado en `vigilio_core`.
   - Expone `resolveIconWithFallback(alias)` retornando rutas virtuales canónicas con prefijo `:/vigilio/`, `resolveThemeWithFallback(alias)`, `loadTextResource(alias)` y `validateStartupResources()`.
   - Se validaron los 35 recursos compilados en `resources.qrc` y `assets/**`, incluyendo iconos de navegación (`back.svg`, `forward.svg`, `reload.svg`, `home.svg`, `search.svg`, `bookmarks.svg`, `tabs.svg`, `settings.svg`, `download.svg`, `history.svg`), herramientas (`senior.svg`, `tools.svg`, `shield.svg`, `mcp.svg`, etc.) y branding (`branding/vigilio-logo.svg`).
2. **Estado actual de la interfaz de usuario (`src/ui/BrowserWindow.h`, `BrowserWindow.cpp`)**:
   - Actualmente, las acciones en `BrowserWindow::setupUi()` se crean exclusivamente con texto plano (`toolbar->addAction("Atrás")`, etc.), sin iconos asociados.
   - La ventana principal no invoca `setWindowIcon()`, mostrando el icono genérico del gestor de ventanas del sistema.
   - El estado vacío (`newHomePage()`) utiliza un texto básico sin incorporar el logotipo vectorial de marca.
   - La página de error (`createErrorPageWidget()`) contiene texto y botones, pero carece de un icono vectorial que apoye visualmente la jerarquía informativa.
   - Las acciones de la barra carecen de propiedades de accesibilidad asistiva (`setAccessibleName`, `setAccessibleDescription`), dependiendo únicamente del texto del botón.
   - Al conmutar «Interfaz más fácil de usar» (`setSeniorMode()`), se ajustan alturas de botones y tipografía, pero los iconos de la barra no existen ni se escalan.
3. **Pipeline de integración y comprobaciones automáticas**:
   - Pipeline de integración `bash apps/vigilio-browser/scripts/check.sh`: 7/7 pruebas unitarias e integración pasando al 100% en 0.11 segundos (exit code 0):
     - `core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`, `webengine_version_validator_tests`, `resource_qrc_catalog_tests`, `resource_inclusion_service_tests`.
   - Formateo de código: Clang-Format verificado sin discrepancias.
   - Herramienta Prompt Builder CLI (`/config/Desktop/auto/prompt-builder/dist/cli.js`):
     - `validate-references --project-root .`: 19/19 referencias canónicas validadas con estado `valid: true` (exit code 0).
     - `verify-lock --lock-file generated/prompts/full-project/0055-module.foundations.task0019.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
     - `validate-decisions`: Confirma que Q-001 a Q-006 y Q-008 están resueltas (Q-007 solo bloquea el release público final).

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0019`.
- **Capacidad**: `resource qrc inclusion`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0019/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0019/proposal.md`
  - `openspec/changes/module-foundations-task0019/design.md`
  - `openspec/changes/module-foundations-task0019/tasks.md`
  - `openspec/changes/module-foundations-task0019/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0019/**`. No se autoriza modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la interacción visual para **resource qrc inclusion** se estructura en torno a `BrowserWindow` y los widgets Qt:
1. **Carga y resolución de iconos vectoriales vía `ResourceInclusionService`**:
   - En lugar de instanciar rutas literales frágiles (`":/vigilio/..."`), la UI consume `vigilio::ResourceInclusionService::resolveIconWithFallback(alias)` para obtener la ruta QRC garantizada.
   - Creación de un método utilitario privado en `BrowserWindow`:
     - `QIcon makeResourceIcon(std::string_view alias) const;` que encapsula la llamada al servicio, instanciando un `QIcon` respaldado por el motor SVG de Qt (`QtSvg`/QIcon).
2. **Iconografía de acciones de navegación en `BrowserWindow::setupUi()`**:
   - Barra de herramientas `"vigilio-navigation"`:
     - `backAction`: `makeResourceIcon("icons/navigation/back.svg")`, texto `"Atrás"`, atajo `QKeySequence::Back` / `Alt+Left`.
     - `forwardAction`: `makeResourceIcon("icons/navigation/forward.svg")`, texto `"Adelante"`, atajo `QKeySequence::Forward` / `Alt+Right`.
     - `reloadAction`: `makeResourceIcon("icons/navigation/reload.svg")`, texto `"Recargar"`, atajo `QKeySequence::Refresh` / `Ctrl+R` / `F5`.
     - `homeAction`: `makeResourceIcon("icons/navigation/home.svg")`, texto `"Inicio"`, atajo `Alt+Home`.
     - `bookmarkAction`: `makeResourceIcon("icons/navigation/bookmarks.svg")`, texto `"Favorito"`, atajo `Ctrl+D`.
     - `favoritesAction`: `makeResourceIcon("icons/navigation/bookmarks.svg")`, texto `"Favoritos"`, atajo `Ctrl+B`.
     - `newTabAction`: `makeResourceIcon("icons/navigation/tabs.svg")`, texto `"Nueva pestaña"`, atajo `Ctrl+T`.
     - `menuAction`: `makeResourceIcon("icons/navigation/settings.svg")`, texto `"Menú"`, atajo `Alt+F` o `F10`.
   - Icono institucional de ventana:
     - `setWindowIcon(makeResourceIcon("branding/vigilio-logo.svg"));`
   - Opciones del menú principal:
     - `seniorAction_`: `makeResourceIcon("icons/tools/senior.svg")`, texto `"Interfaz más fácil de usar"`.
     - `themeAction_`: `makeResourceIcon("icons/navigation/settings.svg")`, texto `"Tema oscuro"`.
3. **Adaptabilidad a «Interfaz más fácil de usar» (Modo Senior)**:
   - En `BrowserWindow::setSeniorMode(bool enabled)`:
     - Ajuste del tamaño de iconos en la barra de herramientas:
       - `toolbar->setIconSize(enabled ? QSize(32, 32) : QSize(22, 22));`
     - Ajuste de dimensiones de controles interactivos:
       - Campos de texto (`QLineEdit`): `minimumHeight = enabled ? 48 : 32`
       - Botones (`QPushButton`): `minimumHeight = enabled ? 48 : 32`
       - Barra de herramientas (`QToolBar`): `minimumHeight = enabled ? 55 : 40`
     - Escalado tipográfico general: 14 pt en modo accesible frente a 10 pt estándar.
     - Notificación accesible en barra de estado: `"Interfaz fácil activada"` / `"Interfaz fácil desactivada"`.
4. **Estados visuales enriquecidos con activos QRC**:
   - *Página de inicio vacía (`newHomePage()`)*: Incorporación de un `QLabel` con el logotipo vectorial de Vigilio escalado a 64×64 px sobre el título, reforzando la identidad de marca según `/resultado/01-vigilio-home.png`.
   - *Página de error (`createErrorPageWidget()`)*: Incorporación de un icono visual representativo (`icons/tools/shield.svg` o advertencia) centrado a 48×48 px antes del título «No se pudo cargar la página», mejorando la comprensión visual del problema.
5. **Accesibilidad para tecnologías asistivas**:
   - Cada acción con icono recibe `setAccessibleName` con el texto de la acción en lenguaje natural y `setAccessibleDescription` detallando la función para lectores de pantalla.
   - Navegación por foco secuencial clara entre la barra de direcciones, botones de navegación y contenido de pestañas.

## Archivos afectados

Durante esta fase PROPOSE, la escritura está rigurosamente limitada a la raíz OpenSpec del cambio:
- `openspec/changes/module-foundations-task0019/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0019/proposal.md`
- `openspec/changes/module-foundations-task0019/design.md`
- `openspec/changes/module-foundations-task0019/tasks.md`
- `openspec/changes/module-foundations-task0019/execution-memory.md`

En la fase futura APPLY (tras aprobación humana en `module.foundations.task0019`):
- `apps/vigilio-browser/src/ui/BrowserWindow.h`: Declaración de punteros a acciones de navegación, método utilitario `makeResourceIcon`, métodos de actualización de tamaño de iconos y accesibilidad.
- `apps/vigilio-browser/src/ui/BrowserWindow.cpp`: Implementación de la asignación de iconos vectoriales QRC a la ventana y acciones, escalado dinámico en `setSeniorMode`, integración de branding en `newHomePage`, icono en `createErrorPageWidget` y nombres accesibles.
- `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Nuevos casos de prueba en la suite Qt UI para validar asignación de iconos, cambio de tamaño en Senior Mode, nombres accesibles y resolución segura con fallback.
- `apps/vigilio-browser/CMakeLists.txt`: Asegurar enlace adecuado con componentes Qt y ejecución de la suite UI.

## Contrato UX/UI

- **Declaración formal**: **Interacción y contratos visuales Qt Widgets**.
- **Justificación**: Esta microtarea pertenece a la etapa `ui` y construye directamente la presentación visual, iconografía SVG, estados y accesibilidad de los recursos QRC en la interfaz gráfica de `BrowserWindow`.
- **Alineación con el diseño objetivo (`/resultado/`)**:
  - `/resultado/01-vigilio-home.png`: Logotipo de Vigilio centrado en la nueva pestaña, barra superior sobria con iconos vectoriales limpios y contrastados.
  - `/resultado/02-vigilio-tools-productivity.png`: Iconografía vectorial Lucide coherente, sin emojis ni efectos brillantes exagerados.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Icono específico `icons/tools/senior.svg` para «Interfaz más fácil de usar», con controles táctiles de gran tamaño (>= 48 px) e iconos escalados (32×32 px).
  - `/resultado/06-logo-instalador-oficial.png`: Icono oficial de marca aplicado a la ventana principal.
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Las acciones utilizan las etiquetas aprobadas: «Atrás», «Adelante», «Recargar», «Inicio», «Favorito», «Favoritos», «Nueva pestaña», «Menú», «Interfaz más fácil de usar», «Tema oscuro».
  - No se expone ningún término interno de ingeniería («QRC», «RCC», «SVG», «Senior Mode», «Prompt Builder», «OpenSpec») en la interfaz visible.
- **Especificación de estados visuales**:
  - `idle`: Icono SVG renderizado nítidamente con color acorde al tema activo (oscuro o claro).
  - `hover`: Realce de fondo sutil en los botones de la barra de herramientas sin bordes agresivos.
  - `focus`: Contorno visible de 2 px con contraste accesible para navegación por teclado.
  - `disabled`: Atenuación óptica (opacidad reducida al 40%) para acciones inactivas (ej. atrás/adelante sin historial).
  - `loading`: Título de pestaña temporal «Cargando…» y mensaje de estado informativo.
  - `empty`: Página de nueva pestaña con logotipo, título y campo de búsqueda enfocado.
  - `error`: Página de error con icono protector, título descriptivo y botones «Reintentar» / «Ir al inicio».

## Reglas y fallos

- **R-001 (Resolución de iconos segura y determinista)**: Todas las asignaciones de iconos deben resolverse a través de `ResourceInclusionService::resolveIconWithFallback(alias)`. Si un recurso no se encuentra en el bundle, debe emplearse el icono de fallback `icons/tools/tools.svg`, garantizando que ninguna acción quede vacía o sin icono.
- **R-002 (Preservación estricta de textos públicos - `docs/UX-COPY-PUBLICO.md`)**: Queda prohibido el uso de términos técnicos como «Modo Senior», «Senior Mode» o «QRC» en la interfaz visible. Debe emplearse «Interfaz más fácil de usar».
- **R-003 (Áreas de clic mínimas en modo accesible)**: En «Interfaz más fácil de usar», todos los controles interactivos deben tener una altura mínima de 48 px y la barra de herramientas una altura mínima de 55 px, con iconos de 32×32 px.
- **R-004 (Accesibilidad asistiva completa)**: Todo control visual con icono debe poseer una propiedad `accessibleName` no vacía y un atajo de teclado claramente definido donde aplique.
- **R-005 (Filosofía Local-First - Q-001)**: Todos los activos gráficos provienen exclusivamente del bundle local compilado en el ejecutable; queda estrictamente prohibida la descarga de imágenes o fuentes desde la red.
- **R-006 (Resiliencia en ejecución Headless)**: Los componentes de interfaz y pruebas automatizadas deben verificar la disponibilidad de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY`) y reportar `NOT_EXECUTED` de manera limpia en entornos CI headless, sin provocar crashes o abortos inesperados.

## Seguridad y privacidad

- **Inmutabilidad y seguridad de activos**: Los iconos SVG y marcas gráficas están embebidos directamente en la sección `.rodata` del binario a través del sistema de recursos de Qt, impidiendo su manipulación o sustitución maliciosa en disco.
- **Aislamiento de navegación privada**: La carga y renderizado de iconos no depende del estado de navegación ni interactúa con cookies, contraseñas o sesiones de usuario.
- **Prevención de XSS o inyección en SVG**: Los SVG estáticos no contienen scripts ejecutables (`<script>`) y son interpretados de forma segura por el motor nativo de renderizado vectorial de Qt.
- **Presupuesto de memoria**: La huella en RAM de los iconos de la barra de herramientas es inferior a 150 KB, aprovechando el cache compartido de `QIcon` y `QPixmapCache`.

## Matriz de pruebas

| Identificador | Criterio de validación | Fixture / Entrada | Comando / Ejecutable | Resultado esperado | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Asignación de iconos en barra de herramientas | Instanciación de `BrowserWindow` con display disponible | `ctest -R window_ui_tests` | Todas las acciones de navegación poseen `!icon().isNull()` | APPLY |
| **TEST-002** | Asignación de icono institucional a la ventana | Inspección de `window.windowIcon()` | `ctest -R window_ui_tests` | `!window.windowIcon().isNull()` | APPLY |
| **TEST-003** | Escalado de iconos en «Interfaz más fácil de usar» | Conmutación de `setSeniorMode(true)` y `(false)` | `ctest -R window_ui_tests` | IconSize pasa de (22, 22) a (32, 32); toolbar height >= 55 | APPLY |
| **TEST-004** | Resolución de fallback ante alias inexistente | Invocación de `makeResourceIcon("icons/invalid.svg")` | `ctest -R window_ui_tests` | Retorna icono válido correspondiente al fallback seguro | APPLY |
| **TEST-005** | Presencia de icono en página de error | Invocación a `createErrorPageWidget()` | `ctest -R window_ui_tests` | Contiene widget con icono visual de error/escudo | APPLY |
| **TEST-006** | Metadatos de accesibilidad asistiva en acciones | Inspección de `accessibleName()` en acciones y controles | `ctest -R window_ui_tests` | Cero acciones con nombres de accesibilidad vacíos | APPLY |
| **TEST-007** | Atajos de teclado en acciones con iconos | Inspección de `shortcut()` en acciones de navegación | `ctest -R window_ui_tests` | Atajos `Ctrl+T`, `Ctrl+D`, `Ctrl+R`, etc. configurados | APPLY |
| **TEST-008** | Tolerancia en entornos headless sin display | Ejecución con `DISPLAY` y `WAYLAND_DISPLAY` deshabilitados | `ctest -R window_ui_tests` | Finaliza limpiamente con exit code 0 y mensaje `NOT_EXECUTED` | APPLY |
| **TEST-009** | Renderizado E2E completo en sesión de navegación | Flujo de navegación interactivo con WebEngine | `ctest -R browser_e2e_smoke` | Renderizado visual correcto en todas las pantallas | LATER |
| **TEST-010** | Integración de iconos en instalador de sistema | Verificación de iconos en instalador binario | Script de empaquetado | Iconos integrados en instalador final | NOT_APPLICABLE |

## Pasos APPLY

1. **Declaración de interfaz de iconos y accesibilidad en `apps/vigilio-browser/src/ui/BrowserWindow.h`**:
   - Declarar el método utilitario privado `QIcon makeResourceIcon(std::string_view alias) const;`.
   - Declarar punteros a las acciones clave (`backAction_`, `forwardAction_`, `reloadAction_`, `homeAction_`, `bookmarkAction_`, `favoritesAction_`, `newTabAction_`, `menuAction_`, `toolbar_`).
2. **Implementación de la asignación de iconos y contratos visuales en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`**:
   - Asignar el icono de ventana mediante `setWindowIcon(makeResourceIcon("branding/vigilio-logo.svg"));`.
   - Equipar todas las acciones de navegación con sus iconos vectoriales de `icons/navigation/*.svg`.
   - Implementar el escalado de `toolbar_->setIconSize()` en `setSeniorMode(bool enabled)` (32×32 en modo accesible, 22×22 en modo estándar).
   - Enriquecer `newHomePage()` con el logotipo de Vigilio (`branding/vigilio-logo.svg`).
   - Enriquecer `createErrorPageWidget()` con el icono representativo de advertencia (`icons/tools/shield.svg`).
   - Asignar `setAccessibleName` y `setAccessibleDescription` a todas las acciones y botones.
3. **Ampliación de la suite de pruebas Qt UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp`**:
   - Añadir aserciones que verifiquen que ninguna acción de la barra de navegación tiene un icono nulo (`!action->icon().isNull()`).
   - Añadir aserciones que comprueben la variación del tamaño de icono (`iconSize()`) en la barra al activar «Interfaz más fácil de usar».
   - Verificar la presencia de nombres accesibles y atajos de teclado asociados a las acciones.
4. **Verificación de pipeline de integración y calidad**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` y certificar compilación limpia bajo C++20 con `-Werror`.
   - Validar conformidad con Clang-Format.
5. **Certificación documental y de referencias OpenSpec**:
   - Validar con la CLI de Prompt Builder (`validate-references` y `verify-lock`).

## Definición de terminado

- [ ] Propuesta OpenSpec formalizada en `openspec/changes/module-foundations-task0019/{proposal.md,design.md,tasks.md,execution-memory.md}` con estado `PROPOSED` y referencias canónicas validadas.
- [ ] Especificación completa de los contratos de interacción y estados visuales Qt Widgets para la capacidad `resource qrc inclusion`.
- [ ] Mapeo exhaustivo de iconos vectoriales SVG para ventana, barra de navegación, menús, estados vacíos y de error.
- [ ] Definición del escalado visual dinámico y áreas táctiles (>= 48 px, iconos 32×32 px) para «Interfaz más fácil de usar» (Modo Senior).
- [ ] Contrato de accesibilidad y lectores de pantalla (nombres accesibles y atajos de teclado) formalizado en cumplimiento con `docs/UX-COPY-PUBLICO.md`.
- [ ] Matriz de pruebas Qt UI trazable con 8 criterios específicos para la fase APPLY.
- [ ] Pipeline local y herramientas de Prompt Builder (`validate-references`, `verify-lock`) ejecutados con resultado limpio (exit code 0).

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0018`: Proveyó la capa de servicio C++20 `ResourceInclusionService` y la validación en arranque.
  - Framework Qt 6 (módulos `Core`, `Widgets`, `Gui`, `Svg`, `Test`).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`) al ejecutar pruebas de interfaz.
    *Mitigación*: La suite `WindowTests.cpp` incluye detección automática de variables de entorno de visualización y retorna `NOT_EXECUTED` de forma limpia con exit code 0, evitando falsos positivos de fallo en servidores de integración continua sin X11.
  - *Riesgo*: Degeneración del rendimiento o retardo en el hilo de UI al renderizar múltiples archivos SVG.
    *Mitigación*: Los archivos SVG provienen de la iconografía optimizada Lucide (viewBox compacto 24×24, trazos simples) y se benefician del cache nativo de pixmaps de `QIcon` en Qt.
  - *Riesgo*: Discrepancia de contraste o legibilidad de iconos oscuros sobre fondos oscuros en el tema nocturno.
    *Mitigación*: Los tokens de diseño (`design-tokens/tokens.json`) y las reglas de hojas de estilo QSS adaptan dinámicamente los fondos de los botones de la barra de herramientas para garantizar ratios de contraste WCAG AA/AAA.

## Commit propuesto

`feat(foundations): ui resource-qrc-inclusion`
