# Tasks — module-foundations-task0009

Status: PROPOSED

- [ ] TASK-001: Formalizar el modelo de dominio de dependencias Qt (`QtModuleId`, `SemanticVersion`, `QtRequirementProfile`, `QtValidationOutcome`, `QtValidationReport`) en las especificaciones del módulo foundations
  - *Criterio de verificación*: Las entidades de dominio, tipos inmutables de versiones semánticas y la taxonomía de resultados de validación están rigurosamente definidos en la propuesta, cumpliendo los principios de diseño desacoplado de C++20 y arquitectura modular.
  - *Comando*: `grep -A 25 "Modelo de Dominio: Qt Dependency Validation" openspec/changes/module-foundations-task0009/design.md`

- [ ] TASK-002: Formalizar las invariantes de versión mínima canónica (Qt >= 6.10.0), compatibilidad de compilador C++20 y gobernanza de configuración en CMake
  - *Criterio de verificación*: Las especificaciones de construcción definen la política de descubrimiento granular de componentes (`Core`, `Widgets`, `WebEngineWidgets`, `Test`), la diferenciación entre modo permisivo con degradación elegante a headless (`VIGILIO_REQUIRE_QT=OFF`) y modo estricto de release (`VIGILIO_REQUIRE_QT=ON`), así como el aislamiento obligatorio de cabeceras de terceros como `SYSTEM`.
  - *Comando*: `grep -E "VIGILIO_REQUIRE_QT|R-001|R-003|R-004" openspec/changes/module-foundations-task0009/design.md`

- [ ] TASK-003: Formalizar el protocolo de validación en tiempo de arranque (Startup Bootstrap) para compatibilidad binaria ABI y disponibilidad de backend gráfico QPA
  - *Criterio de verificación*: El diseño técnico formaliza el punto de pre-validación en `app.cpp` previo a la instanciación de `QApplication` y `BrowserWindow`, garantizando la comparación entre `qVersion()` y `QT_VERSION_STR`, la verificación de servidor de display y la ruta del asistente `QtWebEngineProcess` sin fallos no controlados.
  - *Comando*: `grep -A 15 "FASE DE ARRANQUE EN RUNTIME" openspec/changes/module-foundations-task0009/design.md`

- [ ] TASK-004: Verificar el desacoplamiento estricto y la preservación de la arquitectura headless-first en `vigilio_core` y `vigilio_storage`
  - *Criterio de verificación*: El núcleo de dominio y persistencia local no contiene ninguna dependencia directa o indirecta hacia módulos de Qt, compilando y pasando satisfactoriamente sus pruebas unitarias en modo headless (`VIGILIO_BUILD_GUI=OFF`).
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF && cmake --build apps/vigilio-browser/_build --target vigilio_core_tests`

- [ ] TASK-005: Ejecutar la verificación automatizada integral mediante el script de comprobación local
  - *Criterio de verificación*: Compilación limpia con Ninja bajo opciones estrictas `-Werror` y 100% de pruebas pasadas en CTest (`core_units`, `storage_integration`, `vigilio_warnings_tests`) con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-006: Sincronizar la documentación canónica de la microtarea y verificar el árbol de referencias canónicas con Prompt Builder
  - *Criterio de verificación*: El archivo canónico `docs/rules/01-modules/foundations/micro/0009-qt-dependency-validation/task.md` actualiza su estado tras la aprobación humana y pasa satisfactoriamente la validación de referencias canónicas de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
