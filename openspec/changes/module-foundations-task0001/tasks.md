# Tasks — module-foundations-task0001

Status: PROPOSED

- [ ] TASK-001: Formalizar la especificación de dominio de la topología de targets CMake para Vigilio Browser
  - *Criterio de verificación*: Documentar en el diseño la jerarquía de targets (`vigilio_core`, `vigilio-browser`, ejecutables de test) garantizando el desacoplamiento estricto del núcleo C++20 respecto a `Qt6::Widgets` y `Qt6::WebEngineWidgets`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`

- [ ] TASK-002: Definir el contrato de opciones de configuración y presets de compilación
  - *Criterio de verificación*: Establecer formalmente el comportamiento de `VIGILIO_BUILD_GUI`, `VIGILIO_ENABLE_E2E` y `BUILD_TESTING`, asegurando la capacidad de construcción headless en entornos CI mínimos.
  - *Comando*: Inspección documental de coherencia entre `CMakePresets.json` y `design.md`.

- [ ] TASK-003: Estandarizar la taxonomía de pruebas en CTest, etiquetas y políticas de contención de memoria
  - *Criterio de verificación*: Asignar propiedades obligatorias a las pruebas de CTest: labels `unit;fast` e `integration;fast` con timeout de 15s; label `e2e;heavy` con timeout de 60s, `RESOURCE_LOCK browser_e2e` y `RUN_SERIAL TRUE`.
  - *Comando*: Validación de la matriz de pruebas trazables en `design.md`.

- [ ] TASK-004: Validar coherencia con las reglas canónicas y preparar el handoff para la etapa de implementación
  - *Criterio de verificación*: Confirmar que todos los criterios de aceptación de `docs/rules/01-modules/foundations/micro/0001-cmake-targets/task.md` están cubiertos y documentados para el paso a `module.foundations.task0002`.
  - *Comando*: Verificación del lockfile con `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0001-module.foundations.task0001.propose.lock.json`.
