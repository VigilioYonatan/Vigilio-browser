# Tasks — module-foundations-task0008

Status: PROPOSED

- [ ] TASK-001: Implementar el ejecutable de pruebas de higiene C++20 y advertencias del compilador (`vigilio_warnings_tests`) en `apps/vigilio-browser/tests/unit/CompilerWarningsTests.cpp`
  - *Criterio de verificación*: El código de prueba valida en tiempo de compilación (`static_assert`) y ejecución que el estándar C++20 está activo, que la alineación de tipos es estricta, y que se aplican correctamente los tipos estándar sin conversiones implícitas no intencionadas.
  - *Comando*: `ctest --test-dir apps/vigilio-browser/_build -R vigilio_warnings_tests`

- [ ] TASK-002: Implementar pruebas negativas de rechazo de advertencias mediante fixtures sintéticos con infracciones controladas
  - *Criterio de verificación*: Los fixtures sintéticos que inducen violaciones de advertencias (`-Wconversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`) son efectivamente interceptados y rechazados por el compilador con código de salida distinto de cero cuando `VIGILIO_WARNINGS_AS_ERRORS=ON`.
  - *Comando*: `cmake --build apps/vigilio-browser/_build --target vigilio_warnings_tests`

- [ ] TASK-003: Registrar y configurar el target `vigilio_warnings_tests` en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: El target `vigilio_warnings_tests` está declarado bajo la guarda `if(BUILD_TESTING)`, enlaza de forma privada `vigilio_compiler_warnings`, tiene asignadas etiquetas CTest taxonómicas (`unit;fast`) y timeout estricto de 15 segundos.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-004: Verificar el conmutador de recuperación `VIGILIO_WARNINGS_AS_ERRORS=OFF` y la compatibilidad headless
  - *Criterio de verificación*: La configuración con `-DVIGILIO_WARNINGS_AS_ERRORS=OFF` compila exitosamente permitiendo la emisión de advertencias diagnósticas a `stderr` sin abortar la construcción, preservando la capacidad de depuración y desarrollo sin romper el modo headless.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build_permissive -G Ninja -DVIGILIO_WARNINGS_AS_ERRORS=OFF -DVIGILIO_BUILD_GUI=OFF && cmake --build apps/vigilio-browser/_build_permissive`

- [ ] TASK-005: Ejecutar la verificación automatizada integral mediante el script de comprobación local
  - *Criterio de verificación*: Compilación limpia con Ninja en modo headless y paso del 100% de las pruebas rápidas registradas en CTest (`core_units`, `storage_integration` y `vigilio_warnings_tests`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-006: Sincronizar la documentación canónica de la microtarea y verificar el árbol de referencias canónicas con Prompt Builder
  - *Criterio de verificación*: El archivo canónico `docs/rules/01-modules/foundations/micro/0008-compiler-warnings/task.md` actualiza su estado de ejecución tras la aprobación y pasa satisfactoriamente la validación de referencias de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
