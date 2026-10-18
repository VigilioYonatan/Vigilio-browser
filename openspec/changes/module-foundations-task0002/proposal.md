# Change Proposal — module-foundations-task0002

Pack: `module.foundations.task0002`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica completa para la etapa **implementation** de la capacidad **cmake targets** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado la especificación de dominio e invariantes arquitectónicas en `module.foundations.task0001`, este cambio proyecta la estructuración e implementación física de los targets en `apps/vigilio-browser/CMakeLists.txt`.

El alcance concreto de este pack comprende:
- Modularización de targets de CMake desacoplando estrictamente el núcleo de dominio (`vigilio_core`) del adaptador de persistencia en base de datos (`vigilio_storage`):
  - Target `vigilio_core`: biblioteca estática en C++20 con lógica de negocio pura (`UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`), headers públicos en `src/`, sin dependencias directas de SQLite ni de bibliotecas de interfaz gráfica.
  - Target `vigilio_storage`: biblioteca estática en C++20 que encapsula el repositorio local SQLite (`LocalRepository.cpp`), enlazando `SQLite::SQLite3` y vinculándose con `vigilio_core`.
  - Target `vigilio-browser`: ejecutable nativo de escritorio (`app.cpp`, `BrowserWindow.cpp`, `resources.qrc`), condicionado a la disponibilidad de Qt6 (`Core`, `Widgets`, `WebEngineWidgets`) y `VIGILIO_BUILD_GUI=ON`.
- Estandarización de directivas de compilación y visibilidad:
  - Uso estricto de `PUBLIC` / `PRIVATE` para include directories y dependencias de enlace (`target_include_directories`, `target_link_libraries`).
  - Fijación obligatoria del estándar C++20 vía `target_compile_features(vigilio_core PUBLIC cxx_std_20)`.
  - Flags estrictos de diagnóstico (`-Wall -Wextra -Wpedantic -Wconversion` en GCC/Clang, `/W4 /permissive-` en MSVC) aplicados a nivel de target para evitar contaminación global.
- Configuración de targets y suites de pruebas en CTest:
  - `vigilio_core_tests`: ejecutable de pruebas unitarias (`tests/unit/CoreTests.cpp`) enlazado exclusivamente a `vigilio_core`, etiquetado como `unit;fast` con timeout de 15s.
  - `vigilio_storage_tests`: ejecutable de pruebas de integración (`tests/integration/StorageTests.cpp`) enlazado a `vigilio_storage`, etiquetado como `integration;fast` con timeout de 15s.
  - `vigilio_e2e_smoke`: ejecutable opcional condicionado a `VIGILIO_ENABLE_E2E=ON` y Qt6, etiquetado como `e2e;heavy` con timeout de 60s, `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE` para contención de memoria RAM.
- Garantía de compatibilidad headless total: la bandera `VIGILIO_BUILD_GUI=OFF` compila e instala la suite completa de core y pruebas sin exigir bibliotecas gráficas en el entorno del sistema.

## Out of scope

- Implementación o alteración de componentes de interfaz de usuario Qt Widgets (reservado a la etapa `ui` en `module.foundations.task0003` o documentado sin cambio visual cuando proceda).
- Suite avanzada de pruebas negativas, inyección de fallos de disco o simulaciones de fallo de proceso (reservado a la etapa `tests` en `module.foundations.task0004`).
- Configuración avanzada de advertencias de compilador adicionales y linters automatizados (reservados a las capacidades `compiler-warnings` en task0005-0008 y `clang-tidy` en task0045-0048).
- Modificación de lógica de negocio o esquemas de base de datos existentes; este pack se enfoca puramente en la capa de targets de compilación y su aislamiento modular.
- Modificación de archivos bajo `apps/**` o `docs/rules/**` durante esta fase de propuesta (la implementación se ejecutará en la fase APPLY una vez aprobada la propuesta).

## Contexto y evidencia

Se inspeccionó detalladamente la base de código existente en `apps/vigilio-browser`:
1. `apps/vigilio-browser/CMakeLists.txt`: agrupa actualmente `UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp` y `LocalRepository.cpp` bajo un único target `vigilio_core` que enlaza incondicionalmente a `SQLite::SQLite3`.
2. `apps/vigilio-browser/CMakePresets.json`: presets estandarizados `core-debug` (headless, `VIGILIO_BUILD_GUI=OFF`) y `desktop-debug` (`VIGILIO_BUILD_GUI=ON`).
3. `apps/vigilio-browser/scripts/check.sh`: automatización local que verifica la compilación con Ninja y ejecuta pruebas rápidas con `-L fast`, finalizando con exit code 0.
4. Entorno de compilación: verificado CMake 4.2.3, Ninja 1.13.2 y GCC 15.2.0 operativos.
5. Autoridad canónica: validado contra `docs/rules/01-modules/foundations/micro/0002-cmake-targets/task.md`, `README.md`, `design.md`, `constitution.md` y `architecture.md`.
6. Registro de decisiones: comprobado `docs/rules/answers.md` donde Q-001 a Q-006 y Q-008 están resueltas, y Q-007 (licencia) permanece OPEN bloqueando exclusivamente `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0002/proposal.md`
  - `openspec/changes/module-foundations-task0002/design.md`
  - `openspec/changes/module-foundations-task0002/tasks.md`
  - `openspec/changes/module-foundations-task0002/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0002/**`.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán los siguientes pasos:
1. Refactorizar `apps/vigilio-browser/CMakeLists.txt` para separar los targets `vigilio_core` y `vigilio_storage`, estableciendo dependencias explícitas `PUBLIC`/`PRIVATE` y `target_compile_features`.
2. Actualizar las definiciones de los ejecutables de prueba `vigilio_core_tests` y `vigilio_storage_tests` para enlazar únicamente sus dependencias directas.
3. Configurar y compilar en modo headless con el preset `core-debug` (`cmake --preset core-debug` y `cmake --build --preset core-debug`).
4. Ejecutar la suite de pruebas mediante CTest (`ctest --preset core-debug`).
5. Sincronizar el estado en `docs/rules/01-modules/foundations/micro/0002-cmake-targets/task.md` pasando el estado a `CURRENT; PASS`.

## Definición de terminado (Definition of Done)

- [x] Especificación de implementación de `cmake targets` completada con aislamiento estricto entre lógica de dominio y capa de persistencia.
- [x] Contrato funcional y técnico detallado sin acoplar lógica a widgets Qt ni a `BrowserWindow`.
- [x] Conservación del soporte headless-first mediante `VIGILIO_BUILD_GUI=OFF` validado para entornos de CI.
- [x] Matriz de pruebas trazables que asigna etiquetas CTest (`unit;fast`, `integration;fast`, `e2e;heavy`), timeouts y locks de recursos.
- [x] Ausencia de impacto visual directo documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Tareas de implementación atómicas y verificables redactadas con IDs estables en `tasks.md`.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0001` (etapa `domain` de cmake targets, completada).
- **Dependencias posteriores**: `module.foundations.task0003` (etapa `ui` de cmake targets).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Ruptura de includes en archivos existentes si se altera la estructura de directorios.
    *Mitigación*: Mantener `target_include_directories(... PUBLIC src)` en las librerías base, preservando las rutas de inclusión relativas existentes (`core/...`, `storage/...`).
  - *Riesgo*: Entornos sin Qt6 o WebEngine disponibles en CI.
    *Mitigación*: Mantener la verificación condicional `find_package(Qt6 ... QUIET)` con mensaje informativo y fallback a core-only sin fallar la compilación headless.

## Commit propuesto

`feat(foundations): implementation cmake-targets`
