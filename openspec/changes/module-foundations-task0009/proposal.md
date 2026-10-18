# Change Proposal — module-foundations-task0009

Pack: `module.foundations.task0009`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **Qt dependency validation** en el módulo propietario `foundations` de Vigilio Browser. Tras haber completado la modularización de `cmake targets` (`module.foundations.task0001` a `task0004`) y la suite de higiene de código de `compiler warnings` (`module.foundations.task0005` a `task0008`), esta microtarea formaliza el modelo de dominio, las reglas de negocio, las invariantes de versión y el ciclo de vida de validación de dependencias Qt para garantizar una arquitectura robusta, desacoplada, headless-first y con alta observabilidad en tiempo de configuración, compilación y ejecución.

El alcance concreto de esta etapa **domain** comprende:
- **Modelo de dominio de dependencias Qt (`Qt dependency validation`)**:
  - Definición formal de las entidades y taxonomía de módulos Qt: `Core`, `Widgets`, `WebEngineWidgets` y `Test`.
  - Especificación del estándar de versión canónico: Qt >= 6.10.0 (conforme a la decisión de arquitectura Q-002 de `docs/rules/answers.md` y la directiva de configuración en `apps/vigilio-browser/CMakeLists.txt`).
  - Definición de perfiles de requerimiento de dependencias:
    - *Perfil HeadlessCore*: requiere cero dependencias de Qt; compila y ejecuta 100% en C++20 estándar con SQLite3.
    - *Perfil MinimalGui*: requiere `Qt6::Core` y `Qt6::Widgets` para renderizado de ventanas nativas básicas sin motor web.
    - *Perfil FullBrowser*: requiere `Qt6::Core`, `Qt6::Widgets` y `Qt6::WebEngineWidgets` para navegación web desktop con Chromium multiproceso.
    - *Perfil DeveloperTest*: añade `Qt6::Test` para suites de pruebas gráficas (`vigilio_ui_tests`) y pruebas de humo E2E (`vigilio_e2e_smoke`).
  - Definición formal de los estados y resultados de validación (`QtValidationOutcome`):
    - `Satisfied`: todos los módulos requeridos por el perfil están presentes con versión compatible (>= 6.10.0).
    - `DegradedHeadless`: dependencias de GUI ausentes o insuficientes, pero el núcleo de dominio y persistencia opera normalmente en modo headless.
    - `IncompatibleVersion`: Qt detectado, pero con versión menor a la mínima requerida (< 6.10.0, e.g. Qt 5 o Qt 6.5 LTS incompatible con extensiones MV3 modernas).
    - `MissingComponent`: versión base compatible, pero falta un componente crítico específico (típicamente `WebEngineWidgets`).
    - `EnvironmentError`: discrepancia de compilador/toolchain, fallo de ABI o rutas de prefijo corruptas en `CMAKE_PREFIX_PATH`.
- **Reglas e invariantes de validación en tiempo de configuración (CMake)**:
  - Gobernanza mediante conmutadores de configuración: `VIGILIO_BUILD_GUI` (activado por defecto) y `VIGILIO_REQUIRE_QT` (para control estricto en CI/empaquetado).
  - Política de degradación elegante: cuando `VIGILIO_REQUIRE_QT=OFF` y Qt6 no está disponible en el entorno, CMake emite un diagnóstico informativo y continúa compilando el núcleo headless y sus pruebas unitarias/integración, sin abortar la construcción.
  - Política de fallo estricto: cuando `VIGILIO_REQUIRE_QT=ON` (perfil de packaging/release oficial), la ausencia o incompatibilidad de Qt6.10+ aborta inmediatamente la configuración con un mensaje de diagnóstico accionable que lista los componentes faltantes y cómo resolverlos.
  - Aislamiento de cabeceras externas: las inclusiones de cabeceras Qt deben marcarse como `SYSTEM` en CMake para evitar que advertencias de terceros interfieran con la política `-Werror` de `vigilio_compiler_warnings`.
- **Reglas e invariantes de validación en tiempo de ejecución (Startup Bootstrap)**:
  - Protocolo de pre-validación previo a la instanciación de `BrowserWindow` y componentes gráficos:
    1. Verificación de consistencia entre la versión de cabeceras en tiempo de compilación (`QT_VERSION_STR`) y la versión de bibliotecas compartidas cargadas en tiempo de ejecución (`qVersion()`).
    2. Verificación de disponibilidad del plugin de plataforma de interfaz (QPA, e.g. `xcb`, `wayland`, `windows`, `cocoa`); detección y manejo de entornos sin servidor gráfico (display server ausente) para evitar fallos de segmentación (`SIGSEGV`) no controlados.
    3. Verificación de la ruta del ejecutable asistente multiproceso `QtWebEngineProcess`, asegurando su existencia y permisos de ejecución antes de inicializar vistas web.
- **Invariantes de calidad, rendimiento y privacidad**:
  - *Filosofía local-first*: la validación de dependencias opera estrictamente con información del sistema local y de CMake; prohibida cualquier petición de red para descarga de binarios, telemetría o conexión a servidores cloud.
  - *Presupuesto de recursos*: impacto nulo en tiempo de compilación (< 0.1s en CMake) y tiempo de arranque (< 2ms en runtime); consumo de memoria RAM inferior a 100 KB para estructuras de validación; cero procesos zombies o demonios en segundo plano.
  - *Ausencia total de cambio visual directo*: la validación de dependencias es infraestructura interna de ingeniería; no introduce ventanas, menús ni diálogos en la UI, garantizando pleno cumplimiento con `docs/UX-COPY-PUBLICO.md`, `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.

## Out of scope

- Implementación del servicio o adaptador C++ en `src/` o refactorización del script `CMakeLists.txt` (diferida a la etapa `implementation` en `module.foundations.task0010`).
- Adaptación de flujos de UI o verificación de diálogos de error de usuario (diferida a la etapa `ui` en `module.foundations.task0011`, donde se constatará la ausencia de impacto visual directo).
- Creación de ejecutables de prueba automatizados y arneses de fallo en CTest (diferida a la etapa `tests` en `module.foundations.task0012`).
- Compatibilidad específica de versiones internas de Chromium/WebEngine (reservada para la capacidad `webengine-version-compatibility`, microtareas 0013 a 0016).
- Inclusión de recursos Qt QRC y empaquetado de assets (reservada para la capacidad `resource-qrc-inclusion`, microtareas 0017 a 0020).
- Modificación directa de archivos de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Contexto y evidencia

Se realizó una inspección exhaustiva de la base de código existente en `apps/vigilio-browser` y de las herramientas del sistema:
1. **Configuración de construcción actual (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Líneas 142-177 gobiernan condicionalmente los componentes Qt:
     ```cmake
     option(VIGILIO_ENABLE_E2E "Enable resource-intensive native browser E2E smoke" OFF)
     option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
     if(VIGILIO_BUILD_GUI)
       find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)
       if(Qt6_FOUND)
         set(CMAKE_AUTOMOC ON)
         set(CMAKE_AUTORCC ON)
         add_executable(vigilio-browser src/app.cpp src/ui/BrowserWindow.cpp resources.qrc)
         ...
       else()
         message(WARNING "Qt6 Core/Widgets/WebEngineWidgets unavailable: core and tests only; browser GUI NOT built")
       endif()
     endif()
     ```
   - Diagnóstico: la comprobación actual es preliminar y monolítica: utiliza una llamada `find_package` con `QUIET` que no discrimina qué componente específico falta (¿Core? ¿Widgets? ¿WebEngineWidgets?), no ofrece una opción de fallo estricto para perfiles de release (`VIGILIO_REQUIRE_QT`), ni expone un modelo tipado de validación para el dominio del navegador.
2. **Presets de CMake (`apps/vigilio-browser/CMakePresets.json`)**:
   - `core-debug`: configura `VIGILIO_BUILD_GUI=OFF` y `BUILD_TESTING=ON`, permitiendo compilación y pruebas 100% headless sin requerir Qt.
   - `desktop-debug`: configura `VIGILIO_BUILD_GUI=ON`, requiriendo Qt6 WebEngine para la aplicación de escritorio.
3. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - Instancia directamente `QApplication app(argc, argv)` y `BrowserWindow window` sin comprobaciones previas de runtime (sin validar si `qVersion()` coincide con la compilación o si existe un display server activo).
4. **Verificación de herramientas del entorno local**:
   - Compilador: GNU GCC 15.2.0 (C++20, `-std=c++20`).
   - CMake: versión 4.2.3.
   - Ninja: versión 1.13.2.
   - SQLite3: versión 3.46.1 encontrada y funcional.
   - Ejecución del script de comprobación local `apps/vigilio-browser/scripts/check.sh`: compilación limpia bajo `-Werror` con Ninja y 100% de pruebas aprobadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.13 segundos, exit code 0.
5. **Verificación de gobernanza con Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 9 referencias canónicas validadas, estado `valid: true`, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0025-module.foundations.task0009.propose.lock.json`: 13/13 comprobaciones pasadas exitosamente, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando exclusivamente el hito `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0009`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0009/proposal.md`
  - `openspec/changes/module-foundations-task0009/design.md`
  - `openspec/changes/module-foundations-task0009/tasks.md`
  - `openspec/changes/module-foundations-task0009/execution-memory.md`
  - `openspec/changes/module-foundations-task0009/canonical-reference.yaml`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0009/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. **Formalización de especificaciones de dominio en documentación canónica**:
   - Actualizar `docs/rules/01-modules/foundations/micro/0009-qt-dependency-validation/task.md` para reflejar el modelo formal de entidades, perfiles, reglas de versión e invariantes de validación de Qt.
2. **Definición de contratos de datos e invariantes**:
   - Consolidar los contratos para `QtModuleId`, `SemanticVersion`, `QtRequirementProfile`, `QtValidationOutcome` y `QtValidationReport`.
3. **Comprobación de no-regresión y compatibilidad de arquitectura**:
   - Verificar que las bibliotecas `vigilio_core` y `vigilio_storage` conservan cero acoplamiento con encabezados o símbolos de Qt, salvaguardando la capacidad de construcción y ejecución headless.
4. **Verificación integral de gobernanza y calidad**:
   - Revalidar referencias canónicas y decisiones con Prompt Builder (`validate-references`, `validate-decisions`).
   - Ejecutar el script `apps/vigilio-browser/scripts/check.sh` garantizando que el baseline headless se mantiene 100% verde bajo `-Werror`.

## Definición de terminado (Definition of Done)

- [x] Modelo de dominio, entidades, taxonomía de módulos Qt y perfiles de requerimiento formalizados exhaustivamente en la propuesta.
- [x] Invariantes de versión mínima (Qt >= 6.10.0), C++20 y compatibilidad de plataforma documentadas rigurosamente según Q-002 y `docs/rules/answers.md`.
- [x] Reglas de validación para tiempo de configuración (CMake permissivo vs estricto) y tiempo de ejecución (startup bootstrap, QPA, WebEngineProcess) formalizadas.
- [x] Ausencia total de cambio visual directo justificada documentalmente en cumplimiento de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Principios de ingeniería (SOLID, DRY, KISS, YAGNI) aplicados al diseño desacoplado de validación de dependencias.
- [x] Requisitos de rendimiento (< 100 KB RAM, < 2ms arranque), privacidad (local-first estricto) y seguridad de procesos multiproceso verificados.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0008` (etapa `tests` de compiler warnings, completada con éxito asegurando compilación limpia con cero advertencias bajo `-Werror`).
- **Dependencias posteriores**: `module.foundations.task0010` (etapa `implementation` de Qt dependency validation en la capa C++20 y CMake).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de paquetes de desarrollo de Qt6 WebEngine en entornos de CI o contenedores Linux mínimos.
    *Mitigación*: Preservación estricta de la política headless-first: el proyecto compila y prueba 100% en modo headless (`VIGILIO_BUILD_GUI=OFF` o detección automática con degradación a `core-debug`) sin fallar la integración continua.
  - *Riesgo*: Discrepancia entre la versión de Qt utilizada al compilar y las librerías dinámicas del sistema en tiempo de ejecución.
    *Mitigación*: Validación en runtime mediante comparación semántica entre `QT_VERSION` / `QT_VERSION_STR` (compile-time) y `qVersion()` (runtime), abortando con diagnóstico claro en stderr antes de corromper memoria.
  - *Riesgo*: Ejecución accidental de la aplicación gráfica en entornos sin servidor X11/Wayland (e.g. runners headless de CI).
    *Mitigación*: Detección temprana de disponibilidad de plugin QPA antes de instanciar ventanas, evitando bloqueos o fallos no controlados.

## Commit propuesto

`docs(foundations): domain qt-dependency-validation`
