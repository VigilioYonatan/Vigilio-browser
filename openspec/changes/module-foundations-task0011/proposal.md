# Change Proposal — module-foundations-task0011

Pack: `module.foundations.task0011`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **Qt dependency validation** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en `module.foundations.task0009` la especificación de dominio, taxonomía de componentes y compatibilidad semántica (Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), e implementado en `module.foundations.task0010` el validador desacoplado en C++20 (`vigilio::QtDependencyValidator`), la pre-validación de arranque en `app.cpp` y la gobernanza estricta en CMake con `VIGILIO_REQUIRE_QT`, esta microtarea proyecta la interacción, gobernanza y contratos visuales de la interfaz de usuario en Qt Widgets frente al subsistema de validación de dependencias.

El alcance concreto de este pack comprende:
- **Declaración contractual de UX/UI y ausencia formal de cambio visual directo**:
  - Declaración formal vinculante de que la capacidad **Qt dependency validation** es un componente de aseguramiento de infraestructura de arranque y verificación de dependencias de tiempo de compilación y runtime; por tanto, **no introduce widgets, botones, badges de estado ni diálogos rotulados con terminología técnica de Qt** en la interfaz pública de navegación.
  - Cumplimiento riguroso de la política de lenguaje público de producto `docs/UX-COPY-PUBLICO.md`: prohibición terminante de exponer términos de ingeniería como «Qt dependency validation», «Qt6», «QPA», «WebEngineWidgets», «ABI mismatch», «CMake», «Senior Mode», «Modo Senior», «DAG» o «OpenSpec» en cualquier superficie gráfica visible al usuario final.
  - Garantía de que los controles existentes del navegador en `apps/vigilio-browser/src/ui/BrowserWindow.cpp` (pestañas, barra de direcciones, barra de herramientas, accesibilidad, favoritos y menús) operan sobre las bibliotecas Qt validadas, respetando íntegramente las directrices de diseño de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales oficiales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Gobernanza de interacción y arranque seguro de la interfaz de usuario (`app.cpp` y `BrowserWindow`)**:
  - Protocolo de inicialización previa a la creación de `QApplication`: la verificación de entorno de ejecución (`QtDependencyValidator::validateRuntimeEnvironment`) previene de forma determinista cualquier intento de instanciar ventanas Qt en ausencia de servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` en Linux) o ante desajustes mayores de versión ABI entre bibliotecas compiladas (`QT_VERSION_STR`) y runtime (`qVersion()`).
  - Tratamiento controlado de anomalías de entorno: ante fallo de entorno de display o versión incompatible, la aplicación no realiza llamadas a la API gráfica de Qt ni arroja `SIGSEGV` o cuelgues indefinidos; emite un mensaje estructurado y diagnóstico hacia `stderr` y finaliza con código de salida 1 de forma inmediata.
  - Garantía de no bloqueo del hilo de interfaz (UI thread): las comprobaciones del validador de dependencias se completan en menos de 2 milisegundos sin llamadas bloqueantes de red ni inicializaciones costosas de procesos WebEngine.
- **Preservación y robustez de la «Interfaz más fácil de usar» (Senior Mode)**:
  - Verificación contractual de que la capa de presentación mantenga intactos todos los atributos de accesibilidad: tap targets mínimos de 48px de alto para campos de entrada y 55px para barras de herramientas en modo accesible, tipografía ampliada a 14pt (escalable a 18pt), alto contraste según WCAG, foco de teclado visible y nombres accesibles (`setAccessibleName`) para tecnologías de lectura de pantalla.
  - Confirmación de que el conmutador de accesibilidad utiliza exclusivamente el texto aprobado «Interfaz más fácil de usar» en menús, persistencia local (`QSettings`) y barra de estado (`statusBar()->showMessage(...)`).
- **Gobernanza de targets de UI y suite de pruebas Qt Test (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Vinculación limpia y privada de `vigilio_core`, `vigilio_storage` y `vigilio_compiler_warnings` a los targets de interfaz (`vigilio-browser` y `vigilio_ui_tests`).
  - Preservación del aislamiento de cabeceras de Qt tratadas como `SYSTEM`, impidiendo que advertencias de terceros interfieran con la política `-Werror`.
  - Configuración condicional de la suite de pruebas de interfaz `vigilio_ui_tests` (`tests/ui/WindowTests.cpp`) bajo la guarda `if(TARGET Qt6::Test)` y reporte honesto y transparente de `NOT_EXECUTED` cuando falten bibliotecas de desarrollo de Qt6 en el entorno de ejecución.
- **Preservación incondicional de la compatibilidad headless**:
  - El modo headless (`VIGILIO_BUILD_GUI=OFF` / preset `core-debug`) se mantiene como línea base inmutable y autosuficiente, permitiendo compilar y ejecutar el 100% de las suites de prueba de dominio y almacenamiento (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en contenedores mínimos sin dependencias gráficas.

## Out of scope

- Implementación de nuevas ventanas de diálogo para ajustes avanzados o paneles de diagnóstico de hardware/GPU (diferido a módulos propietarios correspondientes como `settings` o `tools`).
- Creación de arneses automatizados y pruebas unitarias de regresión negativa específicas para simular fallos de Qt (reservado a la etapa `tests` en `module.foundations.task0012`).
- Compatibilidad específica de versiones internas de Chromium/WebEngine (reservada para la capacidad `webengine-version-compatibility`, microtareas 0013 a 0016).
- Inclusión y compilación de recursos estáticos QRC (reservada para la capacidad `resource-qrc-inclusion`, microtareas 0017 a 0020).
- Conexión con servicios cloud, sincronización remota o cuentas de usuario Vigilio (estrictamente prohibido por la constitución local-first de Vigilio Browser).
- Modificación directa de código fuente bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (toda modificación de código se ejecutará exclusivamente en la fase APPLY tras aprobación humana).

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código existente en `apps/vigilio-browser` y las herramientas del entorno de compilación:
1. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - Integra la invocación preliminar `vigilio::QtDependencyValidator::validateRuntimeEnvironment(qVersion(), QT_VERSION_STR, display, wayland)`.
   - Si el resultado no es `Satisfied`, emite diagnóstico en `stderr` y finaliza con código 1, protegiendo la ejecución gráfica previa a `QApplication`.
2. **Capa de interfaz de usuario (`apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`)**:
   - Implementa la ventana principal basada en Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
   - Soporta navegación web, gestión de pestañas diferidas (lazy loading) sin consumo innecesario de instancias WebEngine, favoritos locales en SQLite (`LocalRepository`) y temas claro/oscuro.
   - Implementa la «Interfaz más fácil de usar» con escalado de tipografías a 14pt y controles con alturas mínimas de 48px y 55px.
   - Verifica estrictamente la política de textos de `docs/UX-COPY-PUBLICO.md`: cero menciones a «Senior Mode», «Modo Senior», «Qt» o «CMake».
3. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Define el target ejecutable `vigilio-browser` y la suite de pruebas `vigilio_ui_tests` bajo `if(VIGILIO_BUILD_GUI)` y `if(Qt6_FOUND)`.
   - Soporta `VIGILIO_REQUIRE_QT` para discriminar entre degradación elegante a headless (desarrollo local) y fallo estricto (release).
   - Declara las inclusiones de Qt como `SYSTEM`, evitando que advertencias de cabeceras de terceros rompan `-Werror`.
4. **Suite de pruebas de UI (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
   - Verifica la correcta instanciación de `BrowserWindow`, existencia de controles de navegación y conteo de pestañas sin recurrir a sleeps ni mocks artificiales.
5. **Herramientas locales y verificación de calidad**:
   - Compilador: GNU GCC 15.2.0 (C++20, `-std=c++20`), CMake 4.2.3, Ninja 1.13.2, SQLite 3.46.1.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: compilación Ninja limpia bajo `-Werror` y 3/3 pruebas aprobadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.24 segundos, exit code 0.
   - Verificación de formateo con Clang-Format: 100% de archivos en `src/` y `tests/` cumplen el estándar sin divergencias.
   - Prompt Builder CLI: `validate-references` reporta 11/11 referencias canónicas válidas (exit code 0); `verify-lock` valida el lockfile `0031-module.foundations.task0011.propose.lock.json` al 100% (13/13 comprobaciones exitosas, exit code 0).
   - Registro de decisiones humanas (`validate-decisions`): decisiones Q-001 a Q-006 y Q-008 resueltas y no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito de empaquetado final `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0011`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0011/proposal.md`
  - `openspec/changes/module-foundations-task0011/design.md`
  - `openspec/changes/module-foundations-task0011/tasks.md`
  - `openspec/changes/module-foundations-task0011/execution-memory.md`
  - `openspec/changes/module-foundations-task0011/canonical-reference.yaml`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0011/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Diseño completo de la capacidad

El diseño integral de la etapa `ui` para `Qt dependency validation` se detalla exhaustivamente en `design.md`, cubriendo:
1. Arquitectura de interacción entre el validador desacoplado `vigilio_core` y el ciclo de vida gráfico de Qt en `app.cpp` y `BrowserWindow`.
2. Contrato formal de UI y declaración de «sin cambio visual directo» en la ventana principal del navegador.
3. Tratamiento seguro de fallos de inicialización gráfica sin caídas imprevistas (`SIGSEGV`).
4. Contrato de accesibilidad y preservación de la «Interfaz más fácil de usar» (Senior Mode).
5. Aislamiento de capas, presupuestos de memoria (< 100 KB RAM) y tiempo de validación (< 2ms).
6. Preservación incondicional de la compilación headless (`VIGILIO_BUILD_GUI=OFF`).

## Archivos afectados

En la fase posterior APPLY (tras aprobación de esta propuesta):
- `apps/vigilio-browser/src/app.cpp`: confirmación de la rutina de pre-validación de arranque y reporte a `stderr`.
- `apps/vigilio-browser/src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`: auditoría de conformidad con `docs/UX-COPY-PUBLICO.md`, estabilidad de widgets y robustez ante fallos de recursos.
- `apps/vigilio-browser/CMakeLists.txt`: verificación del target de interfaz `vigilio-browser` y condicionalidad de `vigilio_ui_tests`.
- `apps/vigilio-browser/tests/ui/WindowTests.cpp`: validación del arnés de pruebas de interfaz gráfica.
- `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md`: sincronización de estado de ejecución canónico.

## Contrato UX/UI

- **Ubicación y pantallas**: Ventana principal (`BrowserWindow`), barra de navegación y menú principal.
- **Declaración visual**: «Sin cambio visual directo». La validación de dependencias Qt es infraestructura de ejecución; no añade badges, iconos de depuración ni menús técnicos a la superficie de usuario.
- **Textos públicos aprobados (`docs/UX-COPY-PUBLICO.md`)**:
  - «Interfaz más fácil de usar» (conmutador en menú principal y ajustes).
  - «Interfaz fácil activada» / «Interfaz fácil desactivada» (mensajes temporales en barra de estado).
  - Prohibición estricta de términos de ingeniería: cero menciones a «Qt», «Qt6», «QPA», «WebEngine», «CMake», «Senior Mode», «Modo Senior».
- **Interacción y accesibilidad**:
  - Navegación por teclado completa (atajos `Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`).
  - Foco visible y etiquetas legibles para lectores de pantalla en campos de entrada y botones.
  - Dimensiones táctiles mínimas: 48px de altura para campos de texto y 55px para barras de herramientas en modo fácil.
  - Tipografía escalable de 10pt a 14pt (modo fácil).
  - Iconos vectoriales Lucide SVG coherentes desde `resources.qrc` conforme a `docs/skills/11-assets-svg-quality.md`.

## Reglas y fallos

- **Precondiciones**:
  - Versión mínima de Qt: >= 6.10.0 para todos los componentes gráficos (`Core`, `Widgets`, `WebEngineWidgets`).
  - Disponibilidad de servidor de pantalla en Linux (`DISPLAY` o `WAYLAND_DISPLAY`) para inicialización GUI.
- **Tratamiento de fallos**:
  - Servidor gráfico ausente: salida limpia por `stderr` con código 1 sin intentar invocar APIs gráficas.
  - Versión de Qt incompatible o ABI divergente: reporte de incompatibilidad por `stderr` y terminación inmediata con código 1.
  - Entorno de desarrollo headless (`VIGILIO_REQUIRE_QT=OFF`): CMake omite la construcción de la GUI con un aviso informativo y compila limpiamente el núcleo y sus pruebas.
  - Entorno de release (`VIGILIO_REQUIRE_QT=ON`): CMake aborta la configuración con error fatal y lista detallada de componentes ausentes.
- **Postcondiciones e invariantes**:
  - Cero corrupción de memoria o crashes por desajustes de bibliotecas dinámicas.
  - Cero retención de procesos zombie de WebEngine.
  - Datos de navegación e historial estrictamente confinados a SQLite local (`browser.sqlite`).

## Seguridad y privacidad

- **Filosofía Local-First estricta**: Cero conexiones de red para verificación de componentes o telemetría; no existen cuentas de usuario ni servicios en la nube de Vigilio.
- **Aislamiento de procesos**: No se inician procesos de renderizado de Chromium WebEngine durante la fase de validación de dependencias.
- **Presupuestos de recursos**: Consumo de memoria RAM del validador < 100 KB, tiempo de evaluación < 2 ms en arranque.

## Matriz de pruebas

- **Unitarias (APPLY)**: Pruebas deterministas de `SemanticVersion` y perfiles en `tests/unit/CoreTests.cpp` (ejecución headless, 100% PASS).
- **Integración de UI (APPLY / NOT_EXECUTED)**: Pruebas de instanciación de ventana y controles en `tests/ui/WindowTests.cpp` mediante `vigilio_ui_tests` (ejecutadas cuando Qt6::Test esté disponible; registradas transparentemente como `NOT_EXECUTED` en entornos headless).
- **End-to-End (LATER)**: Pruebas de recorrido completo del navegador en `tests/e2e/BrowserSmoke.cpp` bajo opción `VIGILIO_ENABLE_E2E=ON` ejecutadas de forma serial.
- **Accesibilidad y ergonomía (APPLY)**: Auditoría de tamaños mínimos de controles (>= 48px), contrastes WCAG y conformidad de textos públicos con `docs/UX-COPY-PUBLICO.md`.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. Auditar exhaustivamente la capa de interfaz de usuario (`apps/vigilio-browser/src/app.cpp`, `src/ui/BrowserWindow.h`, `src/ui/BrowserWindow.cpp`, y `tests/ui/WindowTests.cpp`) para certificar la estabilidad de la inicialización de widgets y la correcta captura de fallos de display.
2. Confirmar que en `apps/vigilio-browser/CMakeLists.txt` el target `vigilio-browser` y `vigilio_ui_tests` mantienen las inclusiones de Qt como `SYSTEM`, enlazan las bibliotecas validadas y aíslan el código generado por `moc` y `rcc`.
3. Auditar todas las cadenas públicas y etiquetas visibles en `BrowserWindow.cpp` contra `docs/UX-COPY-PUBLICO.md`, confirmando la ausencia de jerga técnica y el empleo estricto de «Interfaz más fácil de usar».
4. Ejecutar la compilación y pruebas en modo headless mediante `bash apps/vigilio-browser/scripts/check.sh`, verificando cero advertencias bajo `-Werror` y 100% de tests aprobados.
5. Sincronizar el estado de ejecución en la documentación canónica `docs/rules/01-modules/foundations/micro/0011-qt-dependency-validation/task.md`.

## Definición de terminado (DoD)

- [ ] Propuesta OpenSpec completamente articulada y aprobada por el revisor humano.
- [ ] Declaración formal de «sin cambio visual directo» y cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`.
- [ ] Contratos de interacción visual, accesibilidad y Modo Senior validados conforme a las directrices de `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.
- [ ] Compilación headless limpia con Ninja bajo `-Werror` y 100% de pruebas unitarias/integración pasadas (`check.sh`).
- [ ] Registro honesto y auditable de dependencias gráficas (`NOT_EXECUTED` documentado para pruebas de UI cuando falte Qt6).
- [ ] Validación de referencias de Prompt Builder (`validate-references`) sin errores.

## Dependencias y riesgos

- **Dependencias**: `module.foundations.task0010` (implementación de `vigilio::QtDependencyValidator` y gobernanza en CMake, completada y verificada).
- **Riesgo 1**: Indisponibilidad de Qt6 WebEngine en entornos CI mínimos.
  - *Mitigación*: Preservación estricta de la ruta headless (`VIGILIO_BUILD_GUI=OFF`), asegurando que las suites de prueba de núcleo y almacenamiento compilen y pasen al 100%.
- **Riesgo 2**: Exposición inadvertida de terminología de ingeniería en la UI visible.
  - *Mitigación*: Auditoría automatizada con scripts y cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`.
- **Riesgo 3**: Crashes al arrancar en contenedores Linux sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`).
  - *Mitigación*: Detección temprana en `app.cpp` mediante `validateRuntimeEnvironment` y salida controlada por `stderr` con código 1.

## Commit propuesto

- `feat(foundations): ui qt-dependency-validation`
