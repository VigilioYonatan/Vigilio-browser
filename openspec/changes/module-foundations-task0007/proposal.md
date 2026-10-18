# Change Proposal — module-foundations-task0007

Pack: `module.foundations.task0007`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica completa para la etapa **ui** de la capacidad **compiler warnings** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado la especificación de dominio y catálogo canónico en `module.foundations.task0005` e implementado el target de interfaz centralizado `vigilio_compiler_warnings` en `module.foundations.task0006`, este pack aborda la interacción, gobernanza y contratos visuales de la interfaz de usuario en Qt Widgets frente al régimen de advertencias estrictas del compilador C++20.

El alcance concreto de este pack comprende:
- **Declaración contractual de UX/UI y ausencia formal de cambio visual directo**:
  - Declaración formal vinculante de que la capacidad **compiler warnings** es un componente de aseguramiento de calidad estática e infraestructura del sistema de compilación; por ende, **no introduce widgets, botones, ventanas de diálogo ni menús rotulados con términos técnicos de compilación** en la interfaz pública del navegador.
  - Cumplimiento riguroso de la política de lenguaje público de producto `docs/UX-COPY-PUBLICO.md`: prohibición terminante de exponer términos de ingeniería como «compiler warnings», «warnings», «-Werror», «flags», «CMake», «Senior Mode», «DAG» o «OpenSpec» en cualquier superficie gráfica visible al usuario final.
  - Verificación de que los controles del ejecutable `vigilio-browser` (barra de direcciones, pestañas, barra de navegación, accesibilidad y menús) respeten las directrices de diseño de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales oficiales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Gobernanza de compilación limpia de la capa de interfaz gráfica en Qt con advertencias estrictas**:
  - Garantía de que todos los componentes de interfaz de usuario de Vigilio (`apps/vigilio-browser/src/app.cpp`, `apps/vigilio-browser/src/ui/BrowserWindow.h`, `apps/vigilio-browser/src/ui/BrowserWindow.cpp`, y las pruebas de UI `apps/vigilio-browser/tests/ui/WindowTests.cpp`) compilen con cero advertencias bajo el catálogo canónico completo de diagnósticos C++20 y Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS=ON`).
  - Higiene de tipos y polimorfismo en código de UI:
    - Eliminación o prevención de conversiones implícitas estrechantes o de signo (`-Wconversion`, `-Wsign-conversion`) en el cálculo de geometrías de widgets, conteo de pestañas (`tabs_->count()`), tamaños tipográficos y tap targets mínimos de 48px para la «Interfaz más fácil de usar».
    - Higiene de polimorfismo (`-Wnon-virtual-dtor`, `-Woverloaded-virtual`) en clases derivadas de `QMainWindow` y objetos Qt.
    - Prevención de sombreado de variables (`-Wshadow`) en lambdas y callbacks de eventos de señales y slots de Qt.
    - Prevención de desreferenciaciones nulas (`-Wnull-dereference`) en llamadas de inspección de widgets (`currentWebView()`, `tabs_->currentWidget()`, `findChildren()`).
- **Aislamiento de código generado por herramientas de Qt (`moc`, `rcc`, `uic`) y cabeceras de terceros**:
  - Preservación del aislamiento de cabeceras de Qt6 (`Core`, `Widgets`, `WebEngineWidgets`, `Test`) tratadas como `SYSTEM`, impidiendo que advertencias o macros internas de Qt interfieran con la política estricta de compilación del código fuente propio.
  - Inmunización del código generado por `CMAKE_AUTOMOC` y `CMAKE_AUTORCC` frente a diagnósticos hiperestrictos del compilador.
- **Validación del arnés de pruebas de UI (`vigilio_ui_tests`) y registro honesto de dependencias**:
  - Configuración condicional del arnés de pruebas de interfaz de usuario en CMake para validar la instanciación de ventanas, controles de búsqueda, atajos y estados de accesibilidad.
  - Registro transparente y honesto de `NOT_EXECUTED` para pruebas gráficas cuando falten dependencias de desarrollo de Qt6 en el entorno de ejecución, conforme a las reglas del contrato del prompt.
- **Preservación estricta de la compatibilidad headless**:
  - La opción `VIGILIO_BUILD_GUI=OFF` (preset `core-debug`) debe continuar compilando de manera autónoma, limpia y sin advertencias, superando el 100% de las pruebas de dominio y persistencia (`core_units`, `storage_integration`) en entornos CI sin servidor gráfico.

## Out of scope

- Introducción de nuevas pantallas, diálogos de ajustes avanzados o herramientas de productividad en la UI (responsabilidad de sus respectivos módulos propietarios como `tools`, `settings`, `shields`, `downloads`).
- Implementación de pruebas de regresión negativas automatizadas que fuercen diagnósticos de compilador en pipelines de prueba (diferido a la etapa `tests` en `module.foundations.task0008`).
- Modificación de herramientas de formateo de código (`clang-format`, tareas 0041-0044) o linters semánticos (`clang-tidy`, tareas 0045-0048).
- Conexión con servicios en la nube, servidores de telemetría remota o autenticación Vigilio (estrictamente prohibido por la constitución de Vigilio Browser).
- Modificación directa de archivos de código bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (toda modificación de código fuente se ejecutará exclusivamente en la fase APPLY tras aprobación).

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código existente en `apps/vigilio-browser` y la configuración de construcción:
1. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_compiler_warnings` se encuentra implementado como biblioteca `INTERFACE`, centralizando las listas completas de advertencias para GNU GCC (`GCC_WARNINGS`), LLVM Clang (`CLANG_WARNINGS`) y MSVC (`MSVC_WARNINGS`).
   - `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)` inyecta `-Werror` (GCC/Clang) o `/WX` (MSVC) de forma predeterminada.
   - Los targets de UI (`vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`) enlazan `PRIVATE vigilio_compiler_warnings`, encontrándose bajo la guarda condicional `if(VIGILIO_BUILD_GUI)` y `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)`.
2. **Componentes de interfaz gráfica existentes**:
   - `apps/vigilio-browser/src/app.cpp`: inicialización de `QApplication` y punto de entrada de la UI nativa.
   - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `BrowserWindow.cpp`: implementación de la ventana principal con pestañas, barra de navegación, favoritos locales en SQLite, conmutación de tema claro/oscuro y «Interfaz más fácil de usar» con escalado de tipografía a 14pt y controles con altura mínima de 48px.
   - `apps/vigilio-browser/resources.qrc`: compilación de 36 recursos estáticos (iconos Lucide SVG y temas).
   - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: prueba unitaria de widgets Qt para `BrowserWindow`.
3. **Entorno de ejecución y verificación de gates**:
   - Compilador: GNU GCC 15.2.0 C++20, CMake 4.2.3, Ninja 1.13.2.
   - Entorno actual: bibliotecas runtime headless disponibles (`libsqlite3.so`), SDK de desarrollo de Qt6 no presente (emite aviso informativo y omite construcción de GUI sin interrumpir el modo headless).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 7 referencias canónicas válidas, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0019-module.foundations.task0007.propose.lock.json`: 100% de verificaciones aprobadas, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Q-001 a Q-006, Q-008 resueltas; Q-007 permanece OPEN afectando únicamente `release.public-release`.
   - `bash apps/vigilio-browser/scripts/check.sh`: compilación Ninja limpia y 100% tests pasados (`core_units`, `storage_integration`) con exit code 0.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0007/proposal.md`
  - `openspec/changes/module-foundations-task0007/design.md`
  - `openspec/changes/module-foundations-task0007/tasks.md`
  - `openspec/changes/module-foundations-task0007/execution-memory.md`
  - `openspec/changes/module-foundations-task0007/canonical-reference.yaml`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0007/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. Auditar exhaustivamente el código de la capa de interfaz de usuario (`apps/vigilio-browser/src/app.cpp`, `src/ui/BrowserWindow.h`, `src/ui/BrowserWindow.cpp`, y `tests/ui/WindowTests.cpp`) para verificar que cumple con todas las restricciones del catálogo de `vigilio_compiler_warnings` sin advertencias latentes ni conversiones estrechantes implícitas.
2. Comprobar que en `apps/vigilio-browser/CMakeLists.txt` el target `vigilio-browser` y los ejecutables de pruebas de interfaz enlacen de manera limpia y privada `vigilio_compiler_warnings` y mantengan las inclusiones de Qt como cabeceras `SYSTEM`.
3. Auditar todas las cadenas y etiquetas públicas en `BrowserWindow.cpp` contra `docs/UX-COPY-PUBLICO.md`, confirmando la ausencia de jerga de compilación («compiler warnings», «warnings», «-Werror», «CMake») y el empleo exclusivo de «Interfaz más fácil de usar».
4. Ejecutar la compilación y pruebas en modo headless mediante `bash apps/vigilio-browser/scripts/check.sh`, verificando cero advertencias bajo `-Werror` y 100% de tests aprobados.
5. Sincronizar el estado de ejecución en la documentación canónica `docs/rules/01-modules/foundations/micro/0007-compiler-warnings/task.md`.

## Definición de terminado (Definition of Done)

- [x] Especificación de interfaz y gobernanza de advertencias del compilador para la capa de UI completada para la capacidad `compiler warnings`.
- [x] Ausencia de cambio visual directo formalmente documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Contrato de interacción, higiene de C++20 en widgets, accesibilidad y Modo Senior («Interfaz más fácil de usar») especificado y cotejado con las referencias visuales de `/resultado/`.
- [x] Preservación obligatoria del modo de compilación headless (`VIGILIO_BUILD_GUI=OFF`) validada para entornos CI sin servidor gráfico.
- [x] Matriz de pruebas trazable que distingue pruebas aplicables en APPLY, pruebas diferidas a LATER (task0008) y condiciones NOT_APPLICABLE / NOT_EXECUTED.
- [x] Tareas de implementación atómicas y verificables redactadas con IDs estables en `tasks.md`.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0006` (etapa `implementation` de compiler warnings, completada exitosamente con el target `vigilio_compiler_warnings`).
- **Dependencias posteriores**: `module.foundations.task0008` (etapa `tests` de compiler warnings, con pruebas negativas de inducción de diagnósticos).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de encabezados de desarrollo de Qt6 en entornos ligeros de CI o contenedores de compilación.
    *Mitigación*: Mantener la verificación condicional `find_package(Qt6 6.10 QUIET ...)` con mensaje de advertencia y registro transparente de `NOT_EXECUTED` para pruebas de GUI en ausencia de dependencias de Qt, sin romper la compilación headless.
  - *Riesgo*: Aparición de advertencias en expansiones de macros internas de Qt (`Q_OBJECT`, `emit`, `tr`) bajo flags de diagnóstico hiperestrictos.
    *Mitigación*: Inclusión de cabeceras de Qt como `SYSTEM` a través de targets importados de CMake, suprimiendo diagnósticos ajenos al código propio de Vigilio.

## Commit propuesto

`feat(foundations): ui compiler-warnings`
