# Change Proposal — module-foundations-task0010

Pack: `module.foundations.task0010`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **Qt dependency validation** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado en `module.foundations.task0009` la especificación del modelo de dominio, la taxonomía de componentes, los perfiles de requerimiento y las reglas de compatibilidad de versiones (Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), esta microtarea proyecta la implementación física, desacoplada y tipada en C++20 de la validación de dependencias Qt, sin acoplar lógica de negocio a los widgets ni añadir responsabilidades a `BrowserWindow`.

El alcance concreto de este pack comprende:
- **Implementación de la capa propietaria C++20 (`vigilio::QtDependencyValidator`)**:
  - Creación del componente de validación desacoplado en `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `QtDependencyValidator.cpp`, perteneciente a la biblioteca `vigilio_core`.
  - Estructuración inmutable y tipada en C++20 estándar de las entidades del dominio:
    - `QtModuleId`: enum class fuertemente tipado (`Core`, `Widgets`, `WebEngineWidgets`, `Test`, `Svg`).
    - `SemanticVersion`: estructura con semántica de valor para parsing de strings de versión (`major.minor.patch`), comparación mediante spaceship operator (`operator<=>`), formateo textual y evaluación de compatibilidad (`isCompatibleWith(minVersion)`).
    - `QtRequirementProfile`: perfiles de ejecución (`HeadlessCore`, `MinimalGui`, `FullBrowser`, `DeveloperTest`).
    - `QtValidationOutcome`: estados deterministas de validación (`Satisfied`, `DegradedHeadless`, `IncompatibleVersion`, `MissingComponent`, `EnvironmentError`).
    - `QtValidationReport`: estructura inmutable que consolida el estado final, versión detectada, lista de componentes presentes/ausentes y mensajes de diagnóstico estructurados.
  - Implementación de métodos puros de evaluación:
    - `QtDependencyValidator::validateProfile(...)`: evalúa si una colección de módulos y versiones satisface los requisitos de un perfil específico.
    - `QtDependencyValidator::validateRuntimeEnvironment(...)`: evalúa las condiciones de ejecución en arranque (consistencia ABI `qVersion()` vs `QT_VERSION_STR`, existencia de variables de entorno de servidor gráfico como `DISPLAY` o `WAYLAND_DISPLAY` en Linux).
    - `QtDependencyValidator::formatReport(...)`: genera un diagnóstico claro y accionable para logging o salida en `stderr`.
- **Implementación de la gobernanza de construcción en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Incorporación de la opción de configuración `option(VIGILIO_REQUIRE_QT "Require Qt 6.10+ dependencies strictly (fail build if missing)" OFF)`.
  - Sustitución de la búsqueda monolítica y opaca de Qt por una verificación granular de componentes (`Core`, `Widgets`, `WebEngineWidgets`, y condicionalmente `Test` si `BUILD_TESTING=ON`).
  - Detección precisa de la causa de no satisfacción: discriminación entre versión incompatible (< 6.10.0) y componentes individuales faltantes.
  - Política de degradación elegante: si `VIGILIO_REQUIRE_QT=OFF` y Qt6 no está disponible o es incompatible, CMake emite advertencias informativas detallando los componentes ausentes y continúa compilando el núcleo headless y sus pruebas unitarias/integración.
  - Política de fallo estricto: si `VIGILIO_REQUIRE_QT=ON` (para packaging oficial y pipelines de release), la ausencia o incompatibilidad de Qt6.10+ interrumpe la configuración con `message(FATAL_ERROR ...)`.
  - Aislamiento de inclusiones externas: marcado de todos los directorios de inclusión de Qt como `SYSTEM` en los targets consumidores para prevenir que advertencias de cabeceras de terceros rompan la política `-Werror` de `vigilio_compiler_warnings`.
- **Protocolo de pre-validación en tiempo de ejecución (Startup Bootstrap en `apps/vigilio-browser/src/app.cpp`)**:
  - Integración de una comprobación previa a la instanciación de `QApplication` y `BrowserWindow`:
    1. Verificación de coincidencia de versión mayor de bibliotecas compartidas (`qVersion()`) frente a cabeceras compiladas (`QT_VERSION_STR`), previniendo corrupción de memoria por desajustes ABI.
    2. Verificación de backend gráfico disponible (QPA); si no se detecta `DISPLAY` o `WAYLAND_DISPLAY` en Linux y no se provee argumento de plataforma offscreen, se emite un error estructurado a `stderr` y se termina con código 1 de forma controlada sin fallos de segmentación (`SIGSEGV`).
- **Garantías de aislamiento, rendimiento y privacidad**:
  - *Desacoplamiento headless absoluto*: `vigilio_core` (incluido `QtDependencyValidator`) no enlaza ni incluye ningún archivo de cabecera de Qt; compila y ejecuta 100% con C++20 estándar.
  - *Filosofía local-first*: cero peticiones de red, cero descargas de binarios, cero telemetría remota y sin servicios cloud.
  - *Presupuesto de recursos*: tiempo de ejecución del validador < 2ms, huella de memoria < 100 KB RAM, cero procesos Chromium WebEngine iniciados durante la verificación.
  - *Ausencia total de cambio visual directo*: la validación es infraestructura técnica interna sin alteración de la interfaz gráfica del navegador, preservando estrictamente `docs/UX-COPY-PUBLICO.md`, `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.

## Out of scope

- Modificación visual de pantallas, menús, barras de herramientas o ventanas de usuario (diferido a la etapa `ui` en `module.foundations.task0011`, donde se constatará la ausencia de impacto visual directo).
- Creación de ejecutables de prueba dedicados y arneses automatizados de fallo en CTest (diferido a la etapa `tests` en `module.foundations.task0012`).
- Compatibilidad específica de versiones internas de Chromium/WebEngine (reservada para la capacidad `webengine-version-compatibility`, microtareas 0013 a 0016).
- Inclusión de recursos Qt QRC y empaquetado de assets (reservada para la capacidad `resource-qrc-inclusion`, microtareas 0017 a 0020).
- Modificación directa de archivos de código de producción bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (la implementación se realizará exclusivamente en la fase APPLY tras aprobación).

## Contexto y evidencia

Se inspeccionó minuciosamente la base de código existente en `apps/vigilio-browser` y las herramientas del sistema:
1. **Configuración de construcción actual (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Líneas 106-113 definen el target estático `vigilio_core`:
     ```cmake
     add_library(vigilio_core STATIC
       src/core/UrlInput.cpp
       src/core/TabModel.cpp
       src/core/SessionFile.cpp)
     target_include_directories(vigilio_core PUBLIC src)
     target_compile_features(vigilio_core PUBLIC cxx_std_20)
     target_link_libraries(vigilio_core PRIVATE vigilio_compiler_warnings)
     ```
   - Líneas 142-177 gobiernan de forma monolítica la búsqueda de Qt6:
     ```cmake
     option(VIGILIO_ENABLE_E2E "Enable resource-intensive native browser E2E smoke" OFF)
     option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
     if(VIGILIO_BUILD_GUI)
       find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)
       if(Qt6_FOUND)
         ...
       else()
         message(WARNING "Qt6 Core/Widgets/WebEngineWidgets unavailable: core and tests only; browser GUI NOT built")
       endif()
     endif()
     ```
   - Diagnóstico: la llamada con `QUIET` no especifica cuál componente falta (¿Core?, ¿Widgets?, ¿WebEngineWidgets?), no soporta una política estricta de release (`VIGILIO_REQUIRE_QT`) y no existe una capa tipada en C++20 que encapsule la validación.
2. **Punto de entrada de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - Actualmente instancia directamente `QApplication app(argc, argv)` y `BrowserWindow window` sin comprobaciones previas de entorno ni validación de versión ABI de runtime.
3. **Presets de CMake (`apps/vigilio-browser/CMakePresets.json`)**:
   - `core-debug` (`VIGILIO_BUILD_GUI=OFF`) y `desktop-debug` (`VIGILIO_BUILD_GUI=ON`) operan correctamente, pero carecen de la opción `VIGILIO_REQUIRE_QT`.
4. **Verificación de herramientas del entorno local**:
   - Compilador: GNU GCC 15.2.0 (C++20, `-std=c++20`).
   - CMake: versión 4.2.3.
   - Ninja: versión 1.13.2.
   - SQLite3: versión 3.46.1.
   - Ejecución del script de comprobación local `apps/vigilio-browser/scripts/check.sh`: compilación limpia bajo `-Werror` con Ninja y 100% de pruebas aprobadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.37 segundos, exit code 0.
   - Verificación de formateo con Clang-Format: 100% de archivos C++ cumplen el estándar sin diferencias, exit code 0.
5. **Verificación de gobernanza con Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 10 referencias canónicas validadas, estado `valid: true`, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0028-module.foundations.task0010.propose.lock.json`: 13/13 comprobaciones pasadas exitosamente, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando exclusivamente el hito `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0010`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0010/proposal.md`
  - `openspec/changes/module-foundations-task0010/design.md`
  - `openspec/changes/module-foundations-task0010/tasks.md`
  - `openspec/changes/module-foundations-task0010/execution-memory.md`
  - `openspec/changes/module-foundations-task0010/canonical-reference.yaml`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0010/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Pasos APPLY

Cuando este change sea aprobado por un revisor humano para proceder a la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. **Implementación de la capa C++20 de validación en `vigilio_core`**:
   - Crear `apps/vigilio-browser/src/core/QtDependencyValidator.h` definiendo los tipos inmutables (`SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome`, `QtValidationReport`) y la clase de servicio `QtDependencyValidator`.
   - Crear `apps/vigilio-browser/src/core/QtDependencyValidator.cpp` implementando el parsing de versiones, comparación semántica, validación de perfiles y diagnóstico de entorno en C++20 puro.
2. **Integración en el sistema de construcción CMake**:
   - En `apps/vigilio-browser/CMakeLists.txt`, incorporar `src/core/QtDependencyValidator.cpp` al target `vigilio_core`.
   - Definir la opción `option(VIGILIO_REQUIRE_QT "Require Qt 6.10+ dependencies strictly (fail build if missing)" OFF)`.
   - Refactorizar el bloque de detección de Qt6 para realizar comprobaciones granulares de `Core`, `Widgets`, `WebEngineWidgets` y `Test`, aplicando advertencia con degradación a headless o error fatal según el valor de `VIGILIO_REQUIRE_QT`.
   - Asegurar que los directorios de inclusión de Qt se traten con la directiva `SYSTEM`.
3. **Integración del protocolo de pre-validación en `app.cpp`**:
   - En `apps/vigilio-browser/src/app.cpp`, incluir `core/QtDependencyValidator.h` y ejecutar la comprobación de compatibilidad ABI (`qVersion()` vs `QT_VERSION_STR`) y servidor gráfico antes de inicializar `QApplication`.
4. **Adición de pruebas unitarias de validación en `CoreTests.cpp`**:
   - En `apps/vigilio-browser/tests/unit/CoreTests.cpp`, añadir casos de prueba deterministas que validen el parsing de versiones semánticas, los operadores de comparación y la lógica de decisión de `QtDependencyValidator`.
5. **Verificación integral de calidad y compilación**:
   - Ejecutar `apps/vigilio-browser/scripts/check.sh` asegurando compilación limpia con cero advertencias bajo `-Werror` y 100% de tests aprobados.
   - Verificar conformidad de formato de código con Clang-Format.
6. **Sincronización de documentación canónica**:
   - Actualizar el estado en `docs/rules/01-modules/foundations/micro/0010-qt-dependency-validation/task.md` pasando de `TARGET; NOT_EXECUTED` a `CURRENT; PASS`.

## Definición de terminado (Definition of Done)

- [x] Propuesta técnica integral para la etapa `implementation` de `Qt dependency validation` completamente especificada y estructurada.
- [x] Arquitectura de la capa C++20 (`vigilio::QtDependencyValidator`) diseñada con entidades fuertemente tipadas y desacopladas de Qt y de `BrowserWindow`.
- [x] Gobernanza de construcción en CMake proyectada con soporte para detección granular, degradación elegante a headless y conmutador `VIGILIO_REQUIRE_QT`.
- [x] Protocolo de pre-validación en arranque (runtime ABI y servidor de display) formalizado para `app.cpp`.
- [x] Desacoplamiento headless absoluto de `vigilio_core` y `vigilio_storage` garantizado al 100%.
- [x] Ausencia total de cambio visual directo documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Presupuestos de rendimiento (< 100 KB RAM, < 2ms arranque), seguridad y privacidad (local-first estricto) respetados.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0009` (etapa `domain` de Qt dependency validation, completada asegurando el modelo formal de especificación).
- **Dependencias posteriores**: `module.foundations.task0011` (etapa `ui` de Qt dependency validation, donde se auditará la ausencia de impacto visual directo).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de paquetes de desarrollo de Qt6 WebEngine en entornos de CI o contenedores Linux mínimos.
    *Mitigación*: Preservación estricta de la política headless-first: el proyecto compila y prueba 100% en modo headless (`VIGILIO_BUILD_GUI=OFF` o detección automática con degradación a `core-debug`) sin fallar la integración continua.
  - *Riesgo*: Falsos positivos por discrepancias menores de versión de compilación frente a bibliotecas compartidas de Qt en tiempo de ejecución.
    *Mitigación*: La verificación ABI solo rechaza discrepancias en la versión mayor (o versión menor menor a la mínima canónica 6.10.0), tolerando variaciones seguras en el número de parche de bibliotecas del sistema.
  - *Riesgo*: Acoplamiento accidental entre el servicio de validación y la interfaz gráfica.
    *Mitigación*: Ubicación estricta del validador en `src/core/` dentro de `vigilio_core`, sin dependencias ni inclusiones de módulos Qt, preservando el principio de inversión de dependencias y responsabilidad única.

## Commit propuesto

`feat(foundations): implementation qt-dependency-validation`
