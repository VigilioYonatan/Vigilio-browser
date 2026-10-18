# Change Proposal — module-foundations-task0003

Pack: `module.foundations.task0003`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **ui** de la capacidad **cmake targets** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado la especificación de dominio e invariantes arquitectónicas en `module.foundations.task0001` y haber estructurado la modularización física de bibliotecas de dominio y almacenamiento en `module.foundations.task0002`, este cambio proyecta la gobernanza, integración y validación en CMake del target de interfaz de usuario (`vigilio-browser`), su suite de pruebas visuales (`vigilio_ui_tests`) y la compilación de recursos (`resources.qrc`).

El alcance concreto de este pack comprende:
- **Gobernanza de targets de UI en CMake**:
  - Delimitación rigurosa del target ejecutable de escritorio `vigilio-browser` en `apps/vigilio-browser/CMakeLists.txt`, asegurando que el enlace a bibliotecas visuales (`Qt6::Core`, `Qt6::Widgets`, `Qt6::WebEngineWidgets`) esté estrictamente encapsulado bajo la opción `VIGILIO_BUILD_GUI=ON` y la disponibilidad real de Qt6 (`if(Qt6_FOUND)`).
  - Inclusión determinista del sistema de recursos de Qt (`resources.qrc`) con `CMAKE_AUTORCC ON` y generación de código de metaobjetos con `CMAKE_AUTOMOC ON` exclusivamente para los targets que consumen componentes de interfaz.
  - Definición del target de pruebas de interacción de interfaz (`vigilio_ui_tests` o suite de pruebas Qt Test bajo guarda condicional) para validar la instanciación de ventana, foco de teclado, accesibilidad y estados visuales sin requerir ejecución manual ni depender de procesos externos.
- **Declaración contractual de UX/UI y «sin cambio visual directo»**:
  - Declaración formal de que la capacidad **cmake targets** es un componente de infraestructura de construcción y sistema de empaquetado; por tanto, **no introduce widgets, botones ni diálogos de usuario con la etiqueta de la capacidad** en la interfaz pública del navegador.
  - Cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md`: prohibición terminante de exponer términos de ingeniería como «cmake», «targets», «DAG», «Prompt Builder», «PROPOSE», «APPLY» o «OpenSpec» en menús, tooltips o barras de estado.
  - Validación de que los controles del ejecutable `vigilio-browser` (barra de direcciones, pestañas, barra de herramientas, accesibilidad) respeten las directrices de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y la referencia visual de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Preservación estricta de la compatibilidad headless**:
  - La opción `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) debe continuar compilando de manera autónoma y limpia sin requerir bibliotecas gráficas en el entorno de desarrollo o integración continua.
  - En entornos donde Qt6 o sus componentes de WebEngine no estén disponibles en tiempo de configuración, CMake debe emitir una advertencia informativa (`message(WARNING ...)`) sin abortar la compilación del núcleo de dominio y persistencia.

## Out of scope

- Implementación de nuevas pantallas funcionales, paneles de utilidades o visores especializados (responsabilidad de sus respectivos módulos propietarios como `tools`, `settings`, `shields`, `downloads`).
- Suite exhaustiva de pruebas negativas de fallo de disco, corrupción de datos o inyección de fallos de proceso (reservado a la etapa `tests` en `module.foundations.task0004`).
- Configuración avanzada de diagnósticos adicionales del compilador y linters automatizados (cubierto por las capacidades `compiler-warnings` en microtareas 0005-0008 y `clang-tidy` en microtareas 0045-0048).
- Conexión a servicios en la nube, servidores de telemetría remota o sistemas de autenticación/login (estrictamente prohibido por la constitución de Vigilio Browser).
- Modificación directa de archivos bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (toda alteración de código fuente y documentación se ejecutará en la fase APPLY tras aprobación).

## Contexto y evidencia

Se inspeccionó detalladamente la base de código existente en `apps/vigilio-browser`:
1. `apps/vigilio-browser/CMakeLists.txt`: define actualmente la separación limpia entre `vigilio_core` (dominio puro), `vigilio_storage` (persistencia SQLite) y la sección condicional `if(VIGILIO_BUILD_GUI)` con `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)`.
2. `apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`: implementan la ventana principal basada en Qt Widgets con `QTabWidget`, `QLineEdit` accesible, `QToolBar` de navegación, soporte de temas claro/oscuro y «Interfaz más fácil de usar» (Modo Senior con fuentes de 14pt/18pt y controles de 48px de altura mínima).
3. `apps/vigilio-browser/resources.qrc`: agrupa 36 archivos de recursos vectoriales Lucide SVG (navegación y herramientas), temas JSON (`light`, `dark`, `midnight`, `forest`, `sunset`, `aurora`), tokens de diseño y fondos panorámicos.
4. Entorno de ejecución: verificado CMake 4.2.3, Ninja 1.13.2, GCC 15.2.0 y paquetes runtime de Qt 6.10.2 instalados.
5. Verificación de calidad documental:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` (3 referencias canónicas válidas, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0007-module.foundations.task0003.propose.lock.json` (100% de verificaciones aprobadas, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .` (Q-001 a Q-006 y Q-008 resueltas; Q-007 permanece OPEN afectando únicamente `release.public-release`).
6. Suite de pruebas headless: `ctest --preset core-debug` reporta 100% de pruebas pasadas (2/2 pruebas en 0.01s).

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0003/proposal.md`
  - `openspec/changes/module-foundations-task0003/design.md`
  - `openspec/changes/module-foundations-task0003/tasks.md`
  - `openspec/changes/module-foundations-task0003/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0003/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. Auditar y refinar las declaraciones del target `vigilio-browser` en `apps/vigilio-browser/CMakeLists.txt` para garantizar la correcta inclusión de `resources.qrc`, propiedades de bundle/ejecutable nativo de escritorio (`WIN32_EXECUTABLE TRUE`, `MACOSX_BUNDLE TRUE`) y directorios de inclusión privados.
2. Incorporar en `CMakeLists.txt` la configuración condicional para la suite de pruebas de interacción de interfaz gráfica (`vigilio_ui_tests` o arnés de validación Qt Test) bajo las guardas `BUILD_TESTING`, `VIGILIO_BUILD_GUI` y disponibilidad de `Qt6::Test`.
3. Validar que ninguna cadena de texto en `src/ui/BrowserWindow.cpp` o metadatos de target exponga jerga técnica de ingeniería, garantizando conformidad absoluta con `docs/UX-COPY-PUBLICO.md`.
4. Ejecutar la compilación y pruebas headless mediante `apps/vigilio-browser/scripts/check.sh` asegurando que no existan regresiones en los targets preexistentes.
5. Sincronizar el estado de ejecución en la documentación canónica `docs/rules/01-modules/foundations/micro/0003-cmake-targets/task.md`.

## Definición de terminado (Definition of Done)

- [x] Especificación de interfaz y gobernanza de targets de UI completada para la capacidad `cmake targets`.
- [x] Ausencia de cambio visual directo para el usuario final formalmente documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Contrato de interacción, jerarquía de widgets, accesibilidad y Modo Senior del ejecutable `vigilio-browser` especificado y cotejado con las referencias visuales de `/resultado/`.
- [x] Preservación obligatoria del modo de compilación headless (`VIGILIO_BUILD_GUI=OFF`) validada para entornos CI sin servidor gráfico.
- [x] Matriz de pruebas trazable que distingue pruebas aplicables en APPLY, pruebas diferidas a LATER y condiciones NOT_APPLICABLE / NOT_EXECUTED.
- [x] Tareas de implementación atómicas y verificables redactadas con IDs estables en `tasks.md`.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0002` (etapa `implementation` de cmake targets, completada con éxito).
- **Dependencias posteriores**: `module.foundations.task0004` (etapa `tests` de cmake targets).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entornos de construcción de CI que carecen de paquetes de desarrollo de Qt6 (`qt6-base-dev` / `libqt6webenginewidgets6`).
    *Mitigación*: Mantener la verificación condicional `find_package(Qt6 6.10 QUIET ...)` con mensaje de advertencia y registro de estado `NOT_EXECUTED` para pruebas de GUI cuando falten dependencias Qt, sin romper la compilación headless.
  - *Riesgo*: Exposición inadvertida de terminología interna en etiquetas de interfaz de usuario.
    *Mitigación*: Auditoría documental contra la tabla de términos prohibidos de `docs/UX-COPY-PUBLICO.md` verificando que solo se emplee «Interfaz más fácil de usar» y lenguaje natural.

## Commit propuesto

`feat(foundations): ui cmake-targets`
