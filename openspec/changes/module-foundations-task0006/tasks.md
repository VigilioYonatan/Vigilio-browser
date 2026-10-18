# Tasks — module-foundations-task0006

Status: PROPOSED

- [ ] TASK-001: Implementar la biblioteca de interfaz `vigilio_compiler_warnings` y el catálogo canónico de advertencias en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: Declarar el target `add_library(vigilio_compiler_warnings INTERFACE)` con las listas canónicas de flags exhaustivas para GNU GCC (`GCC_WARNINGS`), LLVM Clang (`CLANG_WARNINGS`) y MSVC (`MSVC_WARNINGS`), configuradas según `CMAKE_CXX_COMPILER_ID` mediante `target_compile_options`.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-002: Implementar la opción configurable `VIGILIO_WARNINGS_AS_ERRORS` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: Definir la opción de CMake `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)` y asociar condicionalmente `-Werror` (Clang/GCC) o `/WX` (MSVC) al target `vigilio_compiler_warnings` cuando esté activa.
  - *Comando*: `grep -q "VIGILIO_WARNINGS_AS_ERRORS" apps/vigilio-browser/CMakeLists.txt`

- [ ] TASK-003: Vincular `vigilio_compiler_warnings` de forma privada (`PRIVATE`) a los targets `vigilio_core` y `vigilio_storage`, eliminando los bloques de flags duplicados
  - *Criterio de verificación*: `vigilio_core` y `vigilio_storage` vinculan `vigilio_compiler_warnings` vía `target_link_libraries(... PRIVATE vigilio_compiler_warnings)`, eliminando las sentencias redundantes de `target_compile_options` locales.
  - *Comando*: `cmake --build apps/vigilio-browser/_build --target vigilio_core --target vigilio_storage`

- [ ] TASK-004: Vincular `vigilio_compiler_warnings` a los ejecutables de pruebas unitarias e integración (`vigilio_core_tests` y `vigilio_storage_tests`)
  - *Criterio de verificación*: Los ejecutables `vigilio_core_tests` y `vigilio_storage_tests` enlazan de forma privada `vigilio_compiler_warnings`, asegurando que el código de pruebas esté sujeto al mismo rigor de advertencias que el código de producción.
  - *Comando*: `cmake --build apps/vigilio-browser/_build --target vigilio_core_tests --target vigilio_storage_tests`

- [ ] TASK-005: Vincular `vigilio_compiler_warnings` a los targets de interfaz gráfica (`vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`) con aislamiento de código generado por Qt
  - *Criterio de verificación*: Los targets gráficos enlazan de forma privada `vigilio_compiler_warnings`, y las cabeceras de Qt6 se mantienen aisladas como `SYSTEM` sin interferir con diagnósticos de macros internas de Qt.
  - *Comando*: `cmake --preset desktop-debug || true`

- [ ] TASK-006: Compilar y verificar el proyecto completo con Ninja en modo headless asegurando cero advertencias y paso del 100% de tests
  - *Criterio de verificación*: Ejecución limpia del script de comprobación local `apps/vigilio-browser/scripts/check.sh`, completando la construcción con Ninja y superando el 100% de las pruebas de CTest (`core_units` y `storage_integration`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-007: Sincronizar la documentación canónica de la microtarea y validar referencias con Prompt Builder
  - *Criterio de verificación*: El archivo `docs/rules/01-modules/foundations/micro/0006-compiler-warnings/task.md` actualiza su estado de ejecución tras la aprobación y pasa satisfactoriamente la validación de referencias canónicas.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
