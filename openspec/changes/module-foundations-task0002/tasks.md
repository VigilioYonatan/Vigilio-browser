# Tasks — module-foundations-task0002

Status: PROPOSED

- [ ] TASK-001: Implementar la separación modular de targets en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: Declarar los targets de librería estática `vigilio_core` (para `UrlInput.cpp`, `TabModel.cpp`, `SessionFile.cpp`) y `vigilio_storage` (para `LocalRepository.cpp` con `SQLite::SQLite3`), configurando `target_include_directories(... PUBLIC src)` y `target_compile_features(... PUBLIC cxx_std_20)`.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-002: Actualizar los ejecutables de pruebas unitarias e integración con sus dependencias mínimas requeridas
  - *Criterio de verificación*: `vigilio_core_tests` debe enlazar únicamente `vigilio_core`, y `vigilio_storage_tests` debe enlazar `vigilio_storage`. Ambos targets deben conservar sus propiedades de CTest con etiquetas `unit;fast` e `integration;fast` y timeout de 15 segundos.
  - *Comando*: `cmake --build apps/vigilio-browser/_build`

- [ ] TASK-003: Adaptar las dependencias de los targets gráficos `vigilio-browser` y `vigilio_e2e_smoke`
  - *Criterio de verificación*: `vigilio-browser` y `vigilio_e2e_smoke` enlazan de forma privada tanto `vigilio_core` como `vigilio_storage` junto a las bibliotecas de Qt6 requeridas (`Core`, `Widgets`, `WebEngineWidgets`).
  - *Comando*: Inspección de consistencia sintáctica y resolución de dependencias en `apps/vigilio-browser/CMakeLists.txt`.

- [ ] TASK-004: Ejecutar la verificación automatizada del proyecto mediante el script de comprobación local
  - *Criterio de verificación*: Compilación limpia con Ninja en modo headless y paso del 100% de las pruebas con etiqueta `fast` (`core_units` y `storage_integration`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-005: Actualizar la documentación canónica de la microtarea y validar referencias
  - *Criterio de verificación*: El archivo `docs/rules/01-modules/foundations/micro/0002-cmake-targets/task.md` actualiza su estado de ejecución y pasa la validación canónica de referencias de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
