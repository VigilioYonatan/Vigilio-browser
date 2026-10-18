# Change Proposal — module-foundations-task0001

Pack: `module.foundations.task0001`

Status: PROPOSED

## Outcome and scope

Este cambio define la especificación de dominio, reglas e invariantes arquitectónicas para la capacidad **cmake targets** en el módulo propietario `foundations` de Vigilio Browser. Establece la topología formal de targets de compilación en CMake, garantizando el desacoplamiento estricto entre el núcleo de dominio (`vigilio_core`) y la interfaz gráfica de usuario con motor Chromium (`vigilio-browser` con Qt6 WebEngine).

El alcance de este pack se circunscribe a la etapa **domain**:
- Especificar formalmente la taxonomía y jerarquía de targets en CMake (`vigilio_core`, ejecutables de pruebas unitarias/integración, ejecutable de escritorio `vigilio-browser` y smoke E2E).
- Formalizar los contratos de desacoplamiento e invariantes de aislamiento: el núcleo de dominio permanece C++20 puro sin dependencias de `Qt6::Widgets` ni `Qt6::WebEngineWidgets`, garantizando capacidades headless-first.
- Definir la política de flags de compilación rigurosa (-Wall -Wextra -Wpedantic -Wconversion en GCC/Clang y /W4 /permissive- en MSVC).
- Establecer la categorización y etiquetado de pruebas en CTest (`unit;fast`, `integration;fast`, `e2e;heavy`) con serialización obligatoria (`RESOURCE_LOCK browser_e2e`) para salvaguardar el presupuesto de memoria RAM.
- Garantizar la plena observancia del modelo local-first y privacidad: sin telemetría remota, sin servicios en la nube y sin cuentas de Vigilio.

## Out of scope

- Implementación del refactor del archivo `apps/vigilio-browser/CMakeLists.txt` (diferida a la etapa `implementation` en `module.foundations.task0002`).
- Creación o modificación de componentes visuales Qt (diferida a la etapa `ui` en `module.foundations.task0003`, o declarada no aplicable para targets puramente de compilación/infraestructura).
- Ejecución de pruebas funcionales y de recuperación en tiempo de ejecución (diferida a la etapa `tests` en `module.foundations.task0004`).
- Modificación de otros módulos del navegador (como `memory`, `shields`, `ux-shell`, `installer` o `mcp`).
- Modificación de archivos fuera de `openspec/changes/module-foundations-task0001/**` durante esta fase de propuesta.

## Contexto y evidencia

Se inspeccionó exhaustivamente la base de código existente en `apps/vigilio-browser`:
1. `apps/vigilio-browser/CMakeLists.txt`: define el proyecto `VigilioBrowser VERSION 0.1.0` con C++20 requerido, `vigilio_core` estático que agrupa `UrlInput`, `TabModel`, `SessionFile` y `LocalRepository`, enlace a SQLite3, y condicionales `VIGILIO_BUILD_GUI` y `VIGILIO_ENABLE_E2E`.
2. `apps/vigilio-browser/CMakePresets.json`: presets estandarizados `core-debug` (headless, `VIGILIO_BUILD_GUI=OFF`) y `desktop-debug` (`VIGILIO_BUILD_GUI=ON`).
3. `apps/vigilio-browser/scripts/check.sh`: script de comprobación local que compila en modo headless con Ninja y ejecuta pruebas con filtro `-L fast`.
4. `docs/rules/00-global/architecture.md` y `docs/rules/00-global/constitution.md`: establecen la arquitectura modular C++20/Qt6, prohibición de cloud/telemetría propia y control estricto del uso de memoria.
5. `docs/rules/answers.md`: decisiones Q-001 a Q-006 resueltas; Q-007 (licencia legal exacta) permanece OPEN bloqueando únicamente `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Artefactos producidos**:
  - `openspec/changes/module-foundations-task0001/proposal.md`
  - `openspec/changes/module-foundations-task0001/design.md`
  - `openspec/changes/module-foundations-task0001/tasks.md`
  - `openspec/changes/module-foundations-task0001/execution-memory.md`
- **Restricción de escritura**: Ningún archivo bajo `apps/**` o `docs/rules/**` es modificado en esta fase.

## Pasos APPLY

Cuando este change sea aprobado por un revisor humano, la fase APPLY ejecutará:
1. Verificación del baseline y referencias canónicas contra `docs/rules/01-modules/foundations/micro/0001-cmake-targets/task.md`.
2. Validación de coherencia de las invariantes de dominio en el modelo de CMake targets.
3. Comprobación documental y sincronización de contratos previa al paso a `module.foundations.task0002`.

## Definición de terminado (Definition of Done)

- [x] Invariantes arquitectónicas de `cmake targets` definidas y documentadas sin ambigüedad.
- [x] Topología de targets (core headless vs UI WebEngine) delimitada conforme a las directrices de `docs/skills/01-principios-de-ingenieria.md` y `03-codigo-cpp-qt.md`.
- [x] Matriz de pruebas y categorización CTest trazada conforme a `docs/skills/02-testing-senior.md`.
- [x] Ausencia de impacto visual directo documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Requisitos de seguridad, privacidad y memoria auditados según `docs/skills/06-rendimiento-y-ram.md` y `07-seguridad-y-privacidad.md`.
- [x] Tareas ordenadas y verificables redactadas con IDs estables en `tasks.md`.

## Dependencias y riesgos

- **Dependencias previas**: Ninguna (`none; root execution pack`).
- **Dependencias posteriores**: `module.foundations.task0002` (etapa `implementation` de cmake targets).
- **Riesgos y mitigaciones**:
  - Ausencia de librerías del sistema (p. ej. `SQLite3` o `Qt6`) en entornos CI/contenedores mínimos: Mitigado al aislar `vigilio_core` de Qt y mantener el preset `core-debug` con opciones headless (`VIGILIO_BUILD_GUI=OFF`).
  - Falsos positivos o negativos en tests: Regla obligatoria de registrar `NOT_EXECUTED` si falta la dependencia de runtime, sin crear mocks falsos que simulen éxito.

## Commit propuesto

`docs(foundations): domain cmake-targets`
