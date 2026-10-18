# Change Proposal — module-foundations-task0012

Pack: `module.foundations.task0012`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **Qt dependency validation** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en `module.foundations.task0009` la especificación de dominio y taxonomía de dependencias Qt (Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), implementado en `module.foundations.task0010` el validador desacoplado en C++20 (`vigilio::QtDependencyValidator`) y la guarda en CMake (`VIGILIO_REQUIRE_QT`), y auditado en `module.foundations.task0011` la integración de arranque seguro sin impacto visual adverso en la interfaz de usuario, esta microtarea diseña la suite completa de pruebas automatizadas deterministas para garantizar la máxima confiabilidad, robustez y resiliencia de la validación de dependencias.

El alcance concreto de este pack comprende:
- **Suite de pruebas unitarias focalizadas y exhaustivas (`apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`)**:
  - **Casos positivos y normales**:
    - Parsing y normalización de versiones semánticas (`SemanticVersion::parse`): formato estándar 3 componentes (`6.10.0`, `6.10.2`), formato de 2 componentes con normalización a patch 0 (`6.10` -> `6.10.0`), cadenas con espacios en blanco perimetrales (`"  6.10.0  "`), prefijos literales (`"v6.12.3"`), y cadenas con sufijos de pre-lanzamiento o metadatos de compilación (`"6.10.1-beta"`, `"6.10.0+build.2026"`).
    - Representación canónica en cadena (`SemanticVersion::toString`).
    - Operadores de comparación y ordenamiento total (`<=>`, `==`, `<`, `>`) garantizando reflexividad, antisimetría y transitividad.
    - Predicado de compatibilidad binaria y semántica de Qt6 (`isCompatibleWith`): compatibilidad estricta dentro del mismo major (`major == 6`) requiriendo versión igual o superior a la mínima requerida (Qt >= 6.10.0).
    - Módulos requeridos por perfil de ejecución (`QtDependencyValidator::requiredModules`): `HeadlessCore` (conjunto vacío), `MinimalGui` (`{Core, Widgets}`), `FullBrowser` (`{Core, Widgets, WebEngineWidgets}`) y `DeveloperTest` (`{Core, Widgets, WebEngineWidgets, Test}`).
    - Evaluación de perfiles de dependencias (`validateProfile`): resolución exitosa (`Satisfied`) para cada perfil con versiones válidas e insensibilidad al orden de declaración de módulos.
    - Validación de entorno de ejecución en runtime (`validateRuntimeEnvironment`): validación exitosa ante servidor de despliegue X11 (`DISPLAY=":0"`), Wayland (`WAYLAND_DISPLAY="wayland-0"`), ambos configurados simultáneamente, o plataforma QPA headless/offscreen (`QT_QPA_PLATFORM=offscreen` / `"minimal"`).
    - Formateo de resultados y diagnósticos (`formatOutcome`, `formatReport`, `moduleName`): correspondencia biunívoca entre enumeradores y cadenas legibles para el reporte estructurado en terminal/logs.
  - **Casos negativos y rechazo de entradas inválidas**:
    - Rechazo de cadenas malformadas en `SemanticVersion::parse`: cadenas vacías (`""`), solo espacios en blanco (`"   "`), valores no numéricos (`"abc"`, `"qt6"`), puntos aislados o incompletos (`"6."`, `"..6.10"`, `"6..10"`), caracteres intermedios inválidos (`"6.abc.0"`), y protección contra desbordamiento numérico entero.
    - Rechazo de perfiles incompletos (`MissingComponent`): detección exacta e individualizada de módulos faltantes (por ejemplo, falta `Widgets` en `MinimalGui`, falta `WebEngineWidgets` en `FullBrowser`, falta `Test` en `DeveloperTest`), así como ausencia de múltiples módulos simultáneamente.
    - Rechazo de versiones obsoletas o incompatibles (`IncompatibleVersion`): detección estricta de Qt < 6.10.0 (por ejemplo `6.9.3`) y rechazo de versiones con major distinto (`5.15.2` y `7.0.0`).
    - Rechazo de entorno runtime anómalo (`EnvironmentError`): cadenas de versión no parseables, desajuste mayor de versión ABI entre la versión compilada (`QT_VERSION_STR`) y la enlazada dinámicamente en runtime (`qVersion()`), y ausencia total de servidor de despliegue gráfico en Linux (`DISPLAY` y `WAYLAND_DISPLAY` ausentes o vacíos sin plataforma offscreen).
  - **Recuperación, degradación y resiliencia**:
    - Manejo determinista de punteros nulos (`nullptr`) y cadenas vacías en variables de entorno sin fallos de segmentación ni accesos ilegales de memoria.
    - Simulación de entornos sintéticos sin mutar ni corromper las variables de entorno del sistema operativo anfitrión.
    - Validación del protocolo de fallo controlado en `app.cpp`: ante resultado distinto de `Satisfied`, la aplicación termina con código 1 y mensaje diagnóstico en `stderr` antes de instanciar `QApplication` o `BrowserWindow`.
  - **Concurrencia, límites de recursos y determinismo**:
    - Invocación concurrente multihilo (`std::jthread` / `std::thread`): confirmación de que `validateProfile` y `validateRuntimeEnvironment` son funciones puras libres de efectos secundarios, mutaciones de estado global o condiciones de carrera.
    - Límite estricto de memoria y CPU: suite de pruebas ejecutada 100% en espacio de usuario, con consumo de RAM inferior a 100 KB y tiempo total de ejecución inferior a 15 milisegundos.
    - Cero procesos Chromium / WebEngine iniciados: las pruebas no instancian motores web pesados ni procesos auxiliares.
- **Gobernanza e integración en CTest (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Declaración del target ejecutable de pruebas `vigilio_qt_validator_tests` asociado a `tests/unit/QtDependencyValidatorTests.cpp`.
  - Enlace privado estricto con `vigilio_core` y `vigilio_compiler_warnings`.
  - Registro formal como prueba de CTest mediante `add_test(NAME qt_dependency_validation_tests COMMAND vigilio_qt_validator_tests)`.
  - Configuración de propiedades CTest: etiquetas `LABELS "unit;fast"` y timeout determinista de 15 segundos.
  - Ejecución automática dentro del pipeline de verificación local `apps/vigilio-browser/scripts/check.sh`.

## Out of scope

- Modificación de la implementación o interfaces de `vigilio::QtDependencyValidator` (establecidas en `module.foundations.task0010` y probadas como estables).
- Pruebas de compatibilidad específica de versiones de Chromium embebido (reservadas a la capacidad `webengine-version-compatibility`, tareas 0013 a 0016).
- Pruebas de empaquetado de recursos gráficos QRC (reservadas a `resource-qrc-inclusion`, tareas 0017 a 0020).
- Modificación de código de producción en `apps/**` o reglas en `docs/rules/**` durante esta fase PROPOSE (restringida exclusivamente a `openspec/changes/module-foundations-task0012/**`).
- Pruebas de conexión de red, servicios en la nube o telemetría (prohibidas por el principio local-first y la decisión canónica Q-001).

## Contexto y evidencia

Se inspeccionó de forma exhaustiva el repositorio `apps/vigilio-browser` y las decisiones canónicas:
1. **Implementación de validación de dependencias (`apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`)**:
   - Ofrece tipos fuertemente tipados: `SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome` y `QtValidationReport`.
   - Ofrece métodos puros: `parseVersion`, `requiredModules`, `validateProfile`, `validateRuntimeEnvironment`, `formatOutcome`, `formatReport` y `moduleName`.
   - Desacoplado al 100% de la biblioteca Qt: compila y corre de forma totalmente autónoma en entornos headless.
2. **Punto de arranque de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - Ejecuta `vigilio::QtDependencyValidator::validateRuntimeEnvironment(qVersion(), QT_VERSION_STR, display, wayland)`.
   - Si el resultado no es `Satisfied`, emite el reporte en `stderr` y retorna código 1 antes de instanciar `QApplication`.
3. **Configuración de CMake y pruebas (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Targets existentes de CTest: `core_units`, `storage_integration`, `vigilio_warnings_tests`.
   - Banderas estrictas de compilación con `vigilio_compiler_warnings` y `-Werror`.
4. **Verificación del entorno local de ejecución**:
   - `bash apps/vigilio-browser/scripts/check.sh` ejecutado exitosamente con 3/3 tests pasados (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.23 segundos bajo Ninja y GCC 15.2.0 (C++20).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 12 referencias canónicas con resultado `valid: true`.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0034-module.foundations.task0012.propose.lock.json` superó 13/13 comprobaciones con resultado `valid: true`.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .` confirmó que Q-001 a Q-006 y Q-008 están resueltas; Q-007 permanece OPEN afectando únicamente el hito de release público.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0012`.
- **Capacidad**: `Qt dependency validation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0012/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0012/proposal.md`
  - `openspec/changes/module-foundations-task0012/design.md`
  - `openspec/changes/module-foundations-task0012/tasks.md`
  - `openspec/changes/module-foundations-task0012/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0012/**`. Prohibido modificar `apps/**` o `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

La solución integral de pruebas para **Qt dependency validation** se materializa en una suite C++20 desacoplada de la interfaz gráfica (`tests/unit/QtDependencyValidatorTests.cpp`) y un target de CTest (`qt_dependency_validation_tests`):
1. **Módulo de pruebas unitarias (`tests/unit/QtDependencyValidatorTests.cpp`)**:
   - Macro de aserción determinista `CHECK(condition)` con diagnóstico contextual explícito.
   - 8 suites lógicas de prueba:
     - `test_version_parsing_positive()`: parsing de versiones canónicas, variantes y metadatos.
     - `test_version_parsing_negative()`: rechazo de entradas malformadas, cadenas vacías y desbordamiento.
     - `test_version_comparison_and_compatibility()`: ordenamiento total (`<=>`), igualdad, compatibilidad semántica Qt6 (`isCompatibleWith`).
     - `test_profile_evaluation_positive()`: perfiles `HeadlessCore`, `MinimalGui`, `FullBrowser`, `DeveloperTest` satisfechos.
     - `test_profile_evaluation_negative()`: detección precisa de módulos faltantes y versiones incompatibles.
     - `test_runtime_environment_validation()`: servidores de display X11/Wayland, duales, QPA offscreen, variables vacías, desajustes de versión mayor ABI y errores de entorno.
     - `test_concurrency_and_thread_safety()`: invocaciones concurrentes desde múltiples hilos sin bloqueo ni estado compartido.
     - `test_diagnostic_formatting()`: verificación de salida de `formatOutcome`, `formatReport` y `moduleName`.
2. **Target CTest y política de ejecución**:
   - Compilación con Ninja y banderas estrictas `-Werror` mediante `vigilio_compiler_warnings`.
   - Etiquetado `LABELS "unit;fast"` con timeout de 15 segundos para integración en `check.sh`.
   - Ejecutable en modo headless sin requerir servidor X11 o paquetes GUI de Qt.

## Archivos afectados

- **En fase PROPOSE (este paso)**:
  - `openspec/changes/module-foundations-task0012/canonical-reference.yaml` (autoridad de especificación canónica)
  - `openspec/changes/module-foundations-task0012/proposal.md` (propuesta formal de alcance, arquitectura y DoD)
  - `openspec/changes/module-foundations-task0012/design.md` (diseño técnico detallado, matrices de prueba y análisis de riesgos)
  - `openspec/changes/module-foundations-task0012/tasks.md` (plan de tareas ordenadas y verificables TASK-001 a TASK-007)
  - `openspec/changes/module-foundations-task0012/execution-memory.md` (memoria de ejecución, referencias leídas e invariantes)
- **En fase APPLY (tras aprobación humana)**:
  - `apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp` (creación del nuevo archivo de pruebas unitarias)
  - `apps/vigilio-browser/CMakeLists.txt` (adición del target ejecutable y registro del test en CTest)
  - `docs/rules/01-modules/foundations/micro/0012-qt-dependency-validation/task.md` (actualización de estado canónico a EXECUTED)

## Contrato UX/UI

- **Sin cambio visual directo**: La capacidad `Qt dependency validation` en su etapa `tests` es puramente de infraestructura, aseguramiento de calidad y pruebas automatizadas. No agrega controles gráficos, ventanas ni componentes en la interfaz de usuario.
- **Conformidad con `docs/UX-COPY-PUBLICO.md`**: No expone términos técnicos («Qt», «ABI mismatch», «QPA», «CMake», «DAG», «Senior Mode») en ninguna superficie de usuario final.
- **Preservación de la «Interfaz más fácil de usar»**: Garantiza que los componentes de la interfaz de usuario continúen operando sobre entornos Qt previamente validados, preservando tamaños mínimos de tap target (48px en entradas, 55px en barras), tipografía legible de 14pt y contraste WCAG.

## Reglas y fallos

- **Regla 1 (Validación semántica estricta)**: La versión mínima de Qt soportada es Qt 6.10.0. Cualquier versión menor a 6.10.0 o con major diferente a 6 produce un resultado `IncompatibleVersion` o `EnvironmentError`.
- **Regla 2 (Diagnóstico explícito sin excepciones no controladas)**: Las funciones de validación devuelven estructuras de reporte (`QtValidationReport`) o valores opcionales (`std::optional<SemanticVersion>`) sin lanzar excepciones no capturadas ni causar terminaciones anormales (`SIGSEGV`).
- **Regla 3 (Aislamiento de entorno)**: Las pruebas de entorno runtime (`validateRuntimeEnvironment`) aceptan punteros a cadenas sintéticas de display (`const char* displayEnv`, `const char* waylandEnv`), evitando invocar `setenv`/`putenv` y garantizando pruebas deterministas y libres de interferencias entre hilos.
- **Regla 4 (Prevención de procesos zombies)**: La ejecución de las pruebas unitarias no lanza subprocesos del navegador ni de Chromium/WebEngine, garantizando terminación limpia instantánea.

## Seguridad y privacidad

- **Filosofía local-first**: Las pruebas no realizan ninguna llamada de red, no acceden a servidores remotos ni recopilan datos de telemetría.
- **Aislamiento de datos**: Las pruebas operan en memoria o sobre rutas temporales efímeras gestionadas mediante RAII.
- **Límite de privilegios**: Las pruebas no requieren privilegios elevados (root/sudo) y se ejecutan bajo cuentas de usuario sin privilegios.

## Matriz de pruebas

| Identificador | Nivel | Propósito de la prueba | Fixture / Parámetros | Resultado esperado | Clasificación |
| --- | --- | --- | --- | --- | --- |
| `TEST-001` | Unit | Parsing de versiones positivas estándar y variantes | `"6.10.0"`, `"6.10.2"`, `"6.10"`, `"  6.10.0  "`, `"v6.12.3"` | `SemanticVersion` válido con major, minor y patch correctos | APPLY |
| `TEST-002` | Unit | Rechazo de versiones malformadas o vacías | `""`, `"   "`, `"abc"`, `"6."`, `"..6.10"`, `"6..10"` | `std::nullopt` | APPLY |
| `TEST-003` | Unit | Ordenamiento total y comparación de versiones | Comparación `<=>`, `==`, `<`, `>` entre `6.9.3`, `6.10.0`, `6.10.2`, `7.0.0` | Cumplimiento de orden estricto débil | APPLY |
| `TEST-004` | Unit | Predicado de compatibilidad `isCompatibleWith` | Versión detectada vs mínima `6.10.0` | True para `>= 6.10.0` con major 6; False para `< 6.10.0` o major != 6 | APPLY |
| `TEST-005` | Unit | Evaluación de perfiles de dependencias válidos | Perfiles `HeadlessCore`, `MinimalGui`, `FullBrowser`, `DeveloperTest` con módulos requeridos | `outcome == Satisfied`, `missingModules` vacío | APPLY |
| `TEST-006` | Unit | Detección de módulos faltantes por perfil | `MinimalGui` sin Widgets, `FullBrowser` sin WebEngineWidgets, `DeveloperTest` sin Test | `outcome == MissingComponent`, lista exacta de faltantes | APPLY |
| `TEST-007` | Unit | Validación de entorno de ejecución X11 / Wayland | `display=":0"`, `wayland="wayland-0"`, o ambos | `outcome == Satisfied` | APPLY |
| `TEST-008` | Unit | Detección de ausencia de servidor de display en Linux | `display=nullptr` y `wayland=nullptr` sin QPA offscreen | `outcome == EnvironmentError` | APPLY |
| `TEST-009` | Unit | Detección de desajuste de versión mayor ABI | Versión de compilación `6.10.0` vs runtime `7.0.0` | `outcome == EnvironmentError` | APPLY |
| `TEST-010` | Unit | Detección de versión runtime obsoleta | Runtime `6.9.1` con compilación `6.9.1` | `outcome == IncompatibleVersion` | APPLY |
| `TEST-011` | Unit | Seguridad multihilo y concurrencia | 8 hilos ejecutando 100 validaciones simultáneas | Cero carreras de datos, 100% de reportes válidos | APPLY |
| `TEST-012` | Unit | Formateo diagnóstico de reportes y nombres de módulos | Reportes válidos, reportes con errores y módulos desconocidos | Cadenas formateadas no vacías y comprensibles | APPLY |
| `TEST-013` | Integración | Pipeline de calidad local y suite CTest | `bash apps/vigilio-browser/scripts/check.sh` | 100% tests pasados con exit code 0 | APPLY |

## Pasos APPLY

1. **Crear archivo de pruebas unitarias (`apps/vigilio-browser/tests/unit/QtDependencyValidatorTests.cpp`)**:
   - Implementar los casos de prueba exhaustivos agrupados en funciones modulares.
   - Definir punto de entrada `main()` con captura de excepciones y reporte claro de éxito/fracaso.
2. **Actualizar configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Declarar el ejecutable `vigilio_qt_validator_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar la prueba en CTest: `add_test(NAME qt_dependency_validation_tests COMMAND vigilio_qt_validator_tests)`.
   - Asignar propiedades `LABELS "unit;fast"` y timeout de 15 segundos.
3. **Verificar compilación limpia y ejecución de pruebas**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` confirmando compilación sin advertencias bajo `-Werror` y ejecución exitosa de CTest.
4. **Verificar formato y conformidad con linters**:
   - Validar conformidad de estilo con Clang-Format.
5. **Sincronizar documentación canónica**:
   - Actualizar el estado en `docs/rules/01-modules/foundations/micro/0012-qt-dependency-validation/task.md` a `EXECUTED`.

## Definición de terminado (DoD)

- [ ] Todas las pruebas unitarias de `QtDependencyValidatorTests.cpp` compilan limpiamente sin advertencias con GCC/Clang bajo `-Werror`.
- [ ] La prueba `qt_dependency_validation_tests` se ejecuta dentro del comando `ctest` y finaliza con código de salida 0 en menos de 100 ms.
- [ ] La suite de pruebas se ejecuta de forma independiente y autónoma en modo headless (`VIGILIO_BUILD_GUI=OFF`).
- [ ] Los scripts de verificación (`check.sh`) y linters (`clang-format`) pasan sin errores.
- [ ] Los comandos de validación de Prompt Builder (`validate-references`, `verify-lock`, `validate-decisions`) confirman la consistencia del árbol canónico.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0011` completada y verificada.
  - Dependencias externas: ninguna requerida para compilar o ejecutar los tests unitarios (autónomos en C++20 sobre `vigilio_core`).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Falsos positivos o fallos intermitentes en la prueba de servidor de pantalla si el entorno anfitrión tiene o no `DISPLAY`.
    *Mitigación*: La función `validateRuntimeEnvironment` recibe los valores de pantalla por parámetro en lugar de leer variables globales directamente, garantizando determinismo absoluto e inmunidad ante el estado del entorno anfitrión.
  - *Riesgo*: Consumo excesivo de recursos o bloqueos por pruebas concurrentes.
    *Mitigación*: La prueba multihilo utiliza un número acotado de iteraciones (100 por hilo) y sincronización estándar con `std::jthread`, completándose en menos de 5 ms sin asignaciones dinámicas pesadas.

## Commit propuesto

`test(foundations): tests qt-dependency-validation`
