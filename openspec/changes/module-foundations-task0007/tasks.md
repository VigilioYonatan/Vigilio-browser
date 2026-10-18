# Tasks — module-foundations-task0007

Status: PROPOSED

- [ ] TASK-001: Auditar y validar la integración de `vigilio_compiler_warnings` en los targets de interfaz gráfica (`vigilio-browser` y `vigilio_ui_tests`) en `apps/vigilio-browser/CMakeLists.txt`
  - *Criterio de verificación*: Los targets `vigilio-browser` y `vigilio_ui_tests` enlazan de forma privada (`PRIVATE`) la biblioteca de interfaz `vigilio_compiler_warnings` bajo la guarda condicional `VIGILIO_BUILD_GUI=ON` y `Qt6_FOUND`, con directorios de inclusión de terceros tratados como `SYSTEM`.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF`

- [ ] TASK-002: Auditar la higiene de código C++20 y ausencia de advertencias en `apps/vigilio-browser/src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: El código de presentación de la ventana principal cumple con los estándares estrictos de C++20 sin conversiones implícitas estrechantes ni de signo (`-Wconversion`, `-Wsign-conversion`), destructores virtuales bien formados y comprobaciones de punteros nulos antes de desreferenciar widgets hijos.
  - *Comando*: `find apps/vigilio-browser/src/ui -name '*.cpp' -o -name '*.h' | xargs -n 1 test -f`

- [ ] TASK-003: Auditar la conformidad de textos y etiquetas públicas en `src/ui/BrowserWindow.cpp` contra `docs/UX-COPY-PUBLICO.md`
  - *Criterio de verificación*: Ninguna cadena visible al usuario contiene términos técnicos de ingeniería («compiler warnings», «warnings», «-Werror», «Senior Mode», «Modo Senior», «CMake», «DAG», «OpenSpec»), empleando de forma rigurosa la etiqueta aprobada «Interfaz más fácil de usar» y lenguaje natural claro.
  - *Comando*: `! grep -E -i "Senior Mode|Modo Senior|compiler warnings" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Ejecutar la verificación automatizada del proyecto mediante el script de comprobación local
  - *Criterio de verificación*: Compilación limpia con Ninja en modo headless y paso del 100% de las pruebas rápidas registradas (`core_units` y `storage_integration`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-005: Sincronizar la documentación canónica de la microtarea y verificar el árbol de referencias con Prompt Builder
  - *Criterio de verificación*: El archivo canónico `docs/rules/01-modules/foundations/micro/0007-compiler-warnings/task.md` actualiza su estado de ejecución tras la aprobación y pasa satisfactoriamente la validación de referencias de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
