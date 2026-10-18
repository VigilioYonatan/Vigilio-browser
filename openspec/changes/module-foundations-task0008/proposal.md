# Change Proposal — module-foundations-task0008

Pack: `module.foundations.task0008`

Status: PROPOSED

## Outcome and scope

- **Capacidad**: `compiler warnings`
- **Etapa**: `tests`
- **Módulo propietario**: `foundations`
- **Objetivo**: Diseñar y definir la suite integral de pruebas automatizadas deterministas para la capacidad `compiler warnings`, verificando de manera rigurosa:
  1. **Casos positivos (compilación limpia)**: Todos los componentes de producción y prueba de `vigilio-browser` (`vigilio_core`, `vigilio_storage`, `vigilio_core_tests`, `vigilio_storage_tests`, y condicionalmente `vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`) compilan con cero advertencias bajo flags hiperestrictos (GCC, Clang y MSVC) con `-Werror` / `/WX` habilitado por defecto (`VIGILIO_WARNINGS_AS_ERRORS=ON`).
  2. **Casos negativos (inducción y rechazo de diagnósticos)**: Validación de que el target `vigilio_compiler_warnings` intercepta y aborta la compilación ante infracciones de código reales: conversiones implícitas estrechantes o de signo (`-Wconversion`, `-Wsign-conversion`), sombreado de variables (`-Wshadow`), destructores no virtuales en clases polimórficas (`-Wnon-virtual-dtor`), conversiones de estilo C antiguo (`-Wold-style-cast`), variables o parámetros no utilizados (`-Wunused`), sentencias switch incompletas sin fallthrough explícito (`-Wimplicit-fallthrough`) y condiciones o ramas duplicadas (`-Wduplicated-cond`, `-Wduplicated-branches`).
  3. **Recuperación y alternancia**: Comprobación del conmutador de gobernanza `VIGILIO_WARNINGS_AS_ERRORS=OFF`, permitiendo que advertencias diagnósticas no bloqueen la compilación en flujos de experimentación o diagnóstico, así como degradación suave ante compiladores no reconocidos mediante aviso informativo `message(WARNING ...)`.
  4. **Concurrencia, límites de recursos y determinismo**: Verificación de que la compilación paralela con Ninja (`ninja -j`) es determinista, libre de condiciones de carrera en diagnósticos, y que la cancelación (SIGINT) no corrompe la configuración de CMake.
  5. **No regresión y compatibilidad headless**: Preservación del 100% de éxito en las suites de pruebas de dominio y persistencia (`core_units`, `storage_integration`) en modo headless (`VIGILIO_BUILD_GUI=OFF`) sin iniciar procesos de renderizado Chromium / WebEngine.

## Out of scope

- Modificación de la configuración de linters estáticos (`clang-tidy`, microtareas 0045-0048) o formateadores de código (`clang-format`, microtareas 0041-0044).
- Creación de nuevas pantallas, diálogos de usuario o controles visuales en la interfaz gráfica del navegador (las advertencias de compilador son infraestructura interna de ingeniería).
- Conexión a redes remotas, servicios de nube de Vigilio, autenticación o envío de telemetría (estrictamente prohibido por la constitución local-first de Vigilio).
- Modificación directa de código fuente bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (toda modificación de código se ejecutará exclusivamente en la fase APPLY tras aprobación).

## Contexto y evidencia

Se realizó una inspección exhaustiva de la base de código y del estado de construcción en `apps/vigilio-browser`:
1. **Configuración de construcción existente (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_compiler_warnings` implementado como target de interfaz (`INTERFACE`) que consolida las banderas estrictas para GNU GCC (`GCC_WARNINGS`), LLVM Clang (`CLANG_WARNINGS`) y MSVC (`MSVC_WARNINGS`).
   - `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)` inyecta `-Werror` (GCC/Clang) o `/WX` (MSVC) de forma predeterminada.
   - Enlace privado (`PRIVATE vigilio_compiler_warnings`) presente en `vigilio_core`, `vigilio_storage`, `vigilio_core_tests` y `vigilio_storage_tests`.
   - Bajo la guarda condicional `if(VIGILIO_BUILD_GUI)` y `find_package(Qt6 6.10 QUIET COMPONENTS Core Widgets WebEngineWidgets)`, los targets `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke` enlazan de forma privada `vigilio_compiler_warnings`.
2. **Suites de prueba existentes (`apps/vigilio-browser/tests/`)**:
   - `tests/unit/CoreTests.cpp`: prueba unitaria de `UrlInput`, `TabModel` y `SessionFile` con aserciones deterministas (`CHECK`).
   - `tests/integration/StorageTests.cpp`: prueba de integración de SQLite para marcadores e historial con transacciones y eliminación RAII de archivos efímeros.
   - `tests/ui/WindowTests.cpp`: prueba unitaria de widgets Qt para `BrowserWindow` (condicionada a Qt6).
   - `tests/e2e/BrowserSmoke.cpp`: prueba de humo gráfica con fixture HTML local (condicionada a `VIGILIO_ENABLE_E2E=ON`).
3. **Entorno de ejecución y validación de calidad**:
   - Compilador: GNU GCC 15.2.0 (C++20), CMake 4.2.3, Ninja 1.13.2 sobre Linux x86_64.
   - Ejecución exitosa de `bash apps/vigilio-browser/scripts/check.sh`: compilación Ninja limpia con cero advertencias y 100% de pruebas pasadas (`core_units` y `storage_integration`, exit code 0).
   - Validación de referencias canónicas con Prompt Builder: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` reportó 8 referencias válidas con exit code 0.
   - Verificación de integridad de lockfile con Prompt Builder: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0022-module.foundations.task0008.propose.lock.json` superó el 100% de las verificaciones con exit code 0.
   - Validación de decisiones humanas: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .` confirmó que Q-001 a Q-006 y Q-008 están resueltas, y que Q-007 solo bloquea `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0008`.
- **Capacidad**: `compiler warnings`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0008/proposal.md`
  - `openspec/changes/module-foundations-task0008/design.md`
  - `openspec/changes/module-foundations-task0008/tasks.md`
  - `openspec/changes/module-foundations-task0008/execution-memory.md`
  - `openspec/changes/module-foundations-task0008/canonical-reference.yaml`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0008/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. **Creación del arnés de pruebas de advertencias (`tests/unit/CompilerWarningsTests.cpp`)**:
   - Implementar pruebas unitarias deterministas que validen en tiempo de compilación y ejecución que las macros de diagnóstico estrictas están activadas, que la compatibilidad con el estándar C++20 está vigente (`__cplusplus >= 202002L`), y que no existen conversiones implícitas no intencionadas.
2. **Implementación de casos de prueba negativos de rechazo de advertencias**:
   - Incorporar fixtures de código sintético con infracciones controladas (conversión de estrechamiento, variables no utilizadas, sombra de variables, destructor no virtual) y un arnés automatizado de prueba en CMake / CTest (mediante sub-build o target de verificación) que demuestre que el compilador rechaza dichas infracciones con código de salida distinto de cero cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`.
3. **Validación del conmutador de recuperación (`VIGILIO_WARNINGS_AS_ERRORS=OFF`)**:
   - Comprobar que al desactivar `VIGILIO_WARNINGS_AS_ERRORS`, el compilador emite diagnósticos informativos a stderr sin abortar la construcción, preservando la capacidad de diagnóstico en desarrollo.
4. **Integración en `apps/vigilio-browser/CMakeLists.txt` y CTest**:
   - Registrar el nuevo target de prueba `vigilio_warnings_tests` bajo `BUILD_TESTING`, asignando etiquetas CTest taxonómicas (`unit;fast`), timeout estricto de 15 segundos y vinculación privada a `vigilio_compiler_warnings`.
5. **Verificación automatizada y no regresión**:
   - Ejecutar la suite completa mediante `bash apps/vigilio-browser/scripts/check.sh`, validando compilación Ninja limpia con cero advertencias bajo `-Werror` y 100% de pruebas aprobadas en CTest.
6. **Sincronización de documentación canónica**:
   - Actualizar el estado de ejecución en `docs/rules/01-modules/foundations/micro/0008-compiler-warnings/task.md` y revalidar referencias canónicas mediante Prompt Builder.

## Definición de terminado (Definition of Done)

- [x] Especificación integral de pruebas para la capacidad `compiler warnings` completada en la propuesta (casos positivos, negativos de inducción de advertencias, recuperación y no regresión).
- [x] Ausencia formal de cambio visual directo justificada documentalmente en cumplimiento de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Reglas de gobernanza de CTest (etiquetas `unit;fast`, timeouts estrictos y ejecución determinista sin procesos zombies) formalizadas en el diseño.
- [x] Preservación estricta de la compatibilidad headless (`VIGILIO_BUILD_GUI=OFF`) validada en el diseño y en los scripts de verificación local.
- [x] Matriz de pruebas trazable completa con clasificación explícita de cada escenario (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0007` (etapa `ui` de compiler warnings, completada exitosamente asegurando la integración con los targets de UI).
- **Dependencias posteriores**: `module.foundations.task0009` (inicio de la capacidad `qt-dependency-validation` en el roadmap de foundations).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Falsos positivos o divergencias entre compiladores (GCC vs Clang vs MSVC) en advertencias semánticas como `-Wuseless-cast` o `-Wduplicated-branches`.
    *Mitigación*: Mantener listas diferenciadas (`GCC_WARNINGS`, `CLANG_WARNINGS`, `MSVC_WARNINGS`) en `CMakeLists.txt` y verificar que solo se apliquen banderas soportadas nativamente por cada compilador.
  - *Riesgo*: Indisponibilidad de display gráfico en entornos de CI durante la ejecución de pruebas.
    *Mitigación*: Las pruebas de advertencias del compilador pertenecen a la suite headless (`unit;fast`), ejecutándose 100% en consola sin requerir X11/Wayland ni procesos Chromium.
  - *Riesgo*: Sobrecarga de tiempo de compilación al ejecutar múltiples sub-builds en pruebas negativas.
    *Mitigación*: Uso de snippets sintéticos diminutos (< 20 líneas) con timeouts de 15 segundos y ejecución acotada a nivel de target.

## Commit propuesto

`test(foundations): tests compiler-warnings`
