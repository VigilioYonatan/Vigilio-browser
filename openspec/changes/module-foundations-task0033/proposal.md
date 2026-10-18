# Change Proposal — module-foundations-task0033

Pack: `module.foundations.task0033`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **CMake presets** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0033 — Definir el comportamiento y las invariantes: CMake presets`).

Habiendo implementado y certificado en las capacidades precedentes la modularización de targets CMake (`task0001`-`task0004`), el target de interfaz y política de warnings estrictos (`task0005`-`task0008`), la validación desacoplada de dependencias Qt6 (`task0009`-`task0012`), la compatibilidad de versiones de Chromium WebEngine (`task0013`-`task0016`), la inclusión y catálogo de recursos QRC (`task0017`-`task0020`), el arranque y bootstrap de la aplicación (`task0021`-`task0024`), el ciclo de vida y apagado ordenado (`task0025`-`task0028`), y la política integral de recuperación tras caídas (`task0029`-`task0032`), esta propuesta formaliza el **modelo de dominio, máquina de estados, tipos de valor inmutables, catálogo canónico de perfiles de compilación, motor de validación estática de esquemas, reglas de integridad referencial entre presets y directivas de seguridad para la capacidad CMake presets**.

El alcance concreto comprende:
- **Taxonomía formal de presets de CMake (`CMakePresetKind`)**:
  - `Configure`: Presets que definen el generador (Ninja), directorio de compilación binario (`binaryDir`), variables de caché (`cacheVariables`) y variables de entorno del toolchain.
  - `Build`: Presets de compilación vinculados deterministamente a un `configurePreset` específico, controlando targets, concurrencia y flags de build.
  - `Test`: Presets de ejecución de CTest asociados a un `configurePreset`, gobernando filtros de etiquetas (`LABELS`), tiempos de espera (`TIMEOUT`), concurrencia y volcado de diagnósticos (`outputOnFailure`).
  - `Package`: Presets de empaquetado y generación de instaladores locales mediante CPack.
  - `Workflow`: Flujos encadenados atómicos que orquestan de forma determinista las secuencias configure $\to$ build $\to$ test $\to$ package.
- **Catálogo canónico de perfiles de compilación para Vigilio Browser**:
  - `core-debug`: Compilación headless del núcleo C++20 con SQLite3 y suite de pruebas unitarias (`VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `CMAKE_BUILD_TYPE=Debug`). Diseñado para ciclos de retroalimentación inmediata (< 1s) en desarrollo local e integración continua sin dependencias gráficas.
  - `core-release`: Compilación optimizada del núcleo headless (`VIGILIO_BUILD_GUI=OFF`, `BUILD_TESTING=ON`, `CMAKE_BUILD_TYPE=Release`) para benchmarks de rendimiento, verificación de consumo de memoria y ejecución de pruebas de estrés.
  - `desktop-debug`: Configuración completa de la aplicación de escritorio con interfaz gráfica Qt6 y soporte de motor WebEngine (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=ON`, `VIGILIO_ENABLE_E2E=OFF`, `CMAKE_BUILD_TYPE=Debug`), habilitando símbolos de depuración y diagnósticos en tiempo de ejecución.
  - `desktop-release`: Configuración de producción para empaquetado final de escritorio (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=OFF`, `VIGILIO_ENABLE_E2E=OFF`, `CMAKE_BUILD_TYPE=Release`), optimizada con `-O3` / `/O2` y desprovista de sobrecostes de instrumentación.
  - `desktop-e2e`: Configuración especializada para pruebas de humo end-to-end nativas (`VIGILIO_BUILD_GUI=ON`, `BUILD_TESTING=ON`, `VIGILIO_ENABLE_E2E=ON`), ejecutada en serie con aislamiento de recursos (`RESOURCE_LOCK browser_e2e`).
  - `ci-linux-debug`: Perfil estricto de integración continua en contenedores Linux, activando advertencias como errores (`VIGILIO_WARNINGS_AS_ERRORS=ON`) y compilación determinista con Ninja.
  - `ci-linux-release`: Perfil de release en CI para validación de artefactos empaquetables y verificación de enlaces dinámicos.
  - `sanitizer-asan`: Perfil de instrumentación de seguridad con AddressSanitizer (`-fsanitize=address`) para detección automatizada de accesos fuera de límites, use-after-free y fugas de memoria.
  - `sanitizer-ubsan`: Perfil de instrumentación con UndefinedBehaviorSanitizer (`-fsanitize=undefined`) para certificación de operaciones aritméticas, conversiones y alineación de memoria.
- **Tipos de valor inmutables y estructuras de dominio (`CMakePresetsTypes.h`)**:
  - `CMakePresetKind`: Enumeración fuertemente tipada que distingue los tipos de presets (`Configure`, `Build`, `Test`, `Package`, `Workflow`).
  - `CMakeBuildType`: Enumeración con los tipos canónicos de construcción (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`).
  - `CMakeGenerator`: Enumeración de generadores compatibles (`Ninja`, `UnixMakefiles`, `NinjaMultiConfig`, `VisualStudiov17`).
  - `CMakePresetDefinition`: Estructura inmutable que encapsula nombre único, descripción legible, generador, ruta de binarios normalizada, preset de configuración padre (`configurePreset`), variables de caché tipadas y directivas de salida.
  - `CMakePresetValidationOutcome`: Taxonomía exhaustiva de resultados de validación (`Valid`, `InvalidSchemaVersion`, `MissingRequiredPreset`, `OrphanConfigurePresetReference`, `DuplicatePresetName`, `UnsafeBinaryDir`, `ConflictingVariables`, `MalformedJson`).
  - `CMakePresetsValidationReport`: Estructura consolidada de reporte con estado de evaluación, métricas de presets detectados, lista de incidencias encontradas y recomendaciones accionables.
- **Validador desacoplado de dominio (`vigilio::CMakePresetsValidator`)**:
  - Componente de lógica pura en C++20 ubicado en `vigilio_core`, libre de acoplamientos con `QApplication`, widgets o servicios WebEngine.
  - Validador estático de archivos `CMakePresets.json` y buffers JSON en memoria:
    - Validación estricta de versión de esquema (requiere versión $\ge 3$, estándar canónico versión 6).
    - Verificación de completitud del catálogo mínimo obligatorio (`core-debug` y `desktop-debug` deben existir inexorablemente).
    - Verificación de integridad referencial: todo preset de construcción (`buildPreset`) o prueba (`testPreset`) debe referenciar un preset de configuración (`configurePreset`) válido y existente en la misma especificación.
    - Detección de colisiones de nombres dentro de la misma categoría de preset.
    - Directiva de higiene de rutas: rechazo categórico de Directory Traversal (`..`), rutas absolutas fijadas al sistema de archivos del host (`/home/...`, `C:\...`) o destinos fuera del árbol de construcción del proyecto (`${sourceDir}/build-*`).
    - Verificación de coherencia semántica en `cacheVariables`: consistencia entre flags booleanos (`ON`/`OFF`), tipos de compilación válidos y prevención de combinaciones antagónicas (ej. `VIGILIO_BUILD_GUI=OFF` concurrente con `VIGILIO_ENABLE_E2E=ON`).
- **Invariantes constitucionales de calidad, portabilidad y privacidad**:
  - *Local-First Constitucional (Decisión Q-001)*: Cero dependencia de servicios en la nube, servidores remotos o telemetría en tiempo de configuración o construcción; los presets operan exclusivamente sobre herramientas locales (CMake, Ninja, GCC/Clang/MSVC, SQLite3).
  - *Portabilidad Multiplataforma (Decisión Q-004)*: Soporte estandarizado y reproducible en Linux, Windows y macOS mediante generador Ninja y variables canónicas `${sourceDir}`.
  - *Independencia del Núcleo respecto a WebEngine*: La capacidad de validación de presets reside íntegramente en `vigilio_core` y se compila sin requerir bibliotecas gráficas de Qt ni Chromium.
  - *Presupuestos estrictos de recursos*: Evaluación en memoria instantánea (< 2 ms), consumo de RAM despreciable (< 2 MB) y cero procesos en segundo plano.

---

## Out of scope

- Implementación del código fuente C++20 en `apps/vigilio-browser/src/core/CMakePresetsTypes.h`, `CMakePresetsValidator.h` y `CMakePresetsValidator.cpp` (reservado para la etapa `implementation` en `module.foundations.task0034`).
- Edición física o reemplazo del archivo `apps/vigilio-browser/CMakePresets.json` durante esta fase PROPOSE.
- Modificación directa del script `apps/vigilio-browser/CMakeLists.txt` o creación de nuevos targets en el build system durante PROPOSE.
- Creación de componentes visuales en Qt Widgets (la capacidad es infraestructura de construcción pura, sin interfaz gráfica directa).
- Ejecución de pruebas unitarias o de integración automatizadas en CTest para presets (reservado para la etapa `tests` en `module.foundations.task0036`).
- Modificación de cualquier archivo bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida a `openspec/changes/module-foundations-task0033/**`).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los contratos de foundations y las herramientas locales del sistema:

1. **Configuración de presets existente (`apps/vigilio-browser/CMakePresets.json`)**:
   - Actualmente define el esquema versión 6 con 2 configure presets (`core-debug`, `desktop-debug`), 2 build presets (`core-debug`, `desktop-debug`) y 1 test preset (`core-debug`).
   - Evidencia:
     ```json
     {
       "version": 6,
       "configurePresets": [
         {
           "name": "core-debug",
           "generator": "Ninja",
           "binaryDir": "${sourceDir}/build-core-debug",
           "cacheVariables": {
             "CMAKE_BUILD_TYPE": "Debug",
             "VIGILIO_BUILD_GUI": "OFF",
             "BUILD_TESTING": "ON"
           }
         },
         {
           "name": "desktop-debug",
           "generator": "Ninja",
           "binaryDir": "${sourceDir}/build-desktop-debug",
           "cacheVariables": {
             "CMAKE_BUILD_TYPE": "Debug",
             "VIGILIO_BUILD_GUI": "ON",
             "BUILD_TESTING": "ON",
             "VIGILIO_ENABLE_E2E": "OFF"
           }
         }
       ],
       "buildPresets": [
         { "name": "core-debug", "configurePreset": "core-debug" },
         { "name": "desktop-debug", "configurePreset": "desktop-debug" }
       ],
       "testPresets": [
         { "name": "core-debug", "configurePreset": "core-debug", "output": { "outputOnFailure": true } }
       ]
     }
     ```
   - Diagnóstico: La especificación existente es funcional pero incompleta: carece de presets para `core-release`, `desktop-release`, `desktop-e2e`, entornos CI (`ci-linux-debug`, `ci-linux-release`) y sanitizadores de seguridad (`sanitizer-asan`, `sanitizer-ubsan`). Asimismo, no existe un validador en C++20 que certifique programáticamente la integridad referencial y seguridad de los presets.
2. **Script de construcción principal (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Gobierna los conmutadores `VIGILIO_BUILD_GUI`, `VIGILIO_REQUIRE_QT`, `VIGILIO_ENABLE_E2E`, `BUILD_TESTING` y `VIGILIO_WARNINGS_AS_ERRORS`.
   - Incorpora `vigilio_compiler_warnings`, `vigilio_core`, `vigilio_storage` y 11 ejecutables de pruebas unitarias e integración en CTest.
3. **Pipeline de verificación continua local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Ejecución de Ninja y CTest: 11 de 11 tests pasando exitosamente (exit code 0 en 0.68s):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.07s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.05s)
     - `resource_qrc_catalog_tests`: Passed (0.02s)
     - `resource_inclusion_service_tests`: Passed (0.08s)
     - `resource_qrc_inclusion_tests`: Passed (0.12s)
     - `startup_bootstrap_tests`: Passed (0.16s)
     - `application_lifetime_tests`: Passed (0.15s)
     - `crash_recovery_tests`: Passed (0.02s)
4. **Verificación de herramientas del host**:
   - CMake: versión `4.2.3` (soporta nativamente Presets Schema versión 6 y comandos `cmake --list-presets=all`).
   - Compilador: GNU GCC `15.2.0` con estándar C++20.
   - Generador: Ninja `1.13.2`.
5. **Gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 33 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0097-module.foundations.task0033.propose.lock.json --project-root .`: 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 resueltas (Q-007 solo bloquea `release.public-release`).

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0033`.
- **Capacidad**: `CMake presets`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0033/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0033/proposal.md`
  - `openspec/changes/module-foundations-task0033/design.md`
  - `openspec/changes/module-foundations-task0033/tasks.md`
  - `openspec/changes/module-foundations-task0033/execution-memory.md`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0033/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase PROPOSE.

---

## Diseño completo de la capacidad

### 1. Modelo de Datos y Taxonomía de Presets (`CMakePresetsTypes.h`)
- `CMakePresetKind`: `Configure`, `Build`, `Test`, `Package`, `Workflow`.
- `CMakeBuildType`: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`.
- `CMakeGenerator`: `Ninja`, `UnixMakefiles`, `NinjaMultiConfig`, `VisualStudiov17`.
- `CMakePresetDefinition`:
  ```cpp
  struct CMakePresetDefinition {
      CMakePresetKind kind{CMakePresetKind::Configure};
      std::string name;
      std::string displayName;
      std::string description;
      std::string generator{"Ninja"};
      std::string binaryDir;
      std::string configurePreset; // Requerido para build y test presets
      std::map<std::string, std::string> cacheVariables;
      std::map<std::string, std::string> environment;
      bool outputOnFailure{true};
  };
  ```

### 2. Catálogo Canónico y Estandarización de Presets
| Preset | Tipo | Generador | BinaryDir | Variables de Caché Clave | Propósito |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `core-debug` | Configure / Build / Test | Ninja | `${sourceDir}/build-core-debug` | `BUILD_GUI=OFF`, `TESTING=ON`, `Debug` | Desarrollo rápido headless C++20 |
| `core-release` | Configure / Build / Test | Ninja | `${sourceDir}/build-core-release` | `BUILD_GUI=OFF`, `TESTING=ON`, `Release` | Benchmarks y pruebas de estrés |
| `desktop-debug` | Configure / Build / Test | Ninja | `${sourceDir}/build-desktop-debug` | `BUILD_GUI=ON`, `TESTING=ON`, `E2E=OFF`, `Debug` | Depuración de GUI Qt6 WebEngine |
| `desktop-release` | Configure / Build | Ninja | `${sourceDir}/build-desktop-release` | `BUILD_GUI=ON`, `TESTING=OFF`, `E2E=OFF`, `Release` | Artefacto final para producción |
| `desktop-e2e` | Configure / Build / Test | Ninja | `${sourceDir}/build-desktop-e2e` | `BUILD_GUI=ON`, `TESTING=ON`, `E2E=ON`, `Debug` | Smoke tests end-to-end serializados |
| `ci-linux-debug` | Configure / Build / Test | Ninja | `${sourceDir}/build-ci-debug` | `WARNINGS_AS_ERRORS=ON`, `TESTING=ON` | Validación estricta en pipeline CI Linux |
| `ci-linux-release`| Configure / Build | Ninja | `${sourceDir}/build-ci-release` | `WARNINGS_AS_ERRORS=ON`, `Release` | Verificación de empaquetado en CI |
| `sanitizer-asan` | Configure / Build / Test | Ninja | `${sourceDir}/build-asan` | `-fsanitize=address`, `Debug` | Detección de fugas y corrupción de memoria |
| `sanitizer-ubsan`| Configure / Build / Test | Ninja | `${sourceDir}/build-ubsan` | `-fsanitize=undefined`, `Debug` | Detección de comportamiento indefinido |

### 3. Validador Desacoplado de Dominio (`vigilio::CMakePresetsValidator`)
- Métodos públicos estáticos en C++20:
  - `validatePresetsJson(const std::string& jsonContent)`: Analiza el contenido textual, valida la versión del esquema, la unicidad de nombres, la integridad de referencias cruzadas y la higiene de rutas.
  - `validatePresetsFile(const std::filesystem::path& path)`: Carga el archivo desde el sistema de archivos e invoca la validación interna.
  - `isSafeBinaryDir(const std::string& binaryDir)`: Verifica que la ruta de salida utilice el prefijo `${sourceDir}/build-*` o `_build-*`, no contenga secuencias de escape `..` ni rutas absolutas forzadas.
  - `canonicalPresetNames()`: Devuelve la lista canónica inmutable de presets mínimos obligatorios.
  - `outcomeToString(CMakePresetValidationOutcome outcome)`: Traduce códigos de error a cadenas legibles en español técnico conforme a `docs/UX-COPY-PUBLICO.md`.

---

## Archivos afectados

| Archivo | Acción | Justificación |
| :--- | :--- | :--- |
| `openspec/changes/module-foundations-task0033/canonical-reference.yaml` | Crear / Formalizar | Especificación de metadatos de gobernanza OpenSpec para task0033. |
| `openspec/changes/module-foundations-task0033/proposal.md` | Crear / Formalizar | Propuesta técnica exhaustiva con arquitectura, alcance, reglas y matrices. |
| `openspec/changes/module-foundations-task0033/design.md` | Crear / Formalizar | Documento de diseño arquitectónico con CURRENT/TARGET/GAPS, esquemas y modelos C++20. |
| `openspec/changes/module-foundations-task0033/tasks.md` | Crear / Formalizar | Lista de tareas atómicas, ordenadas y verificables para la fase APPLY. |
| `openspec/changes/module-foundations-task0033/execution-memory.md` | Crear / Formalizar | Registro de ejecución, skills vinculantes, invariantes y handoff. |
| `apps/vigilio-browser/src/core/CMakePresetsTypes.h` | Crear (en APPLY) | Declaración de enumeraciones, estructuras y tipos inmutables del dominio de presets. |
| `apps/vigilio-browser/src/core/CMakePresetsValidator.h` | Crear (en APPLY) | Declaración del validador desacoplado de presets en C++20. |
| `apps/vigilio-browser/src/core/CMakePresetsValidator.cpp` | Crear (en APPLY) | Implementación de las reglas de validación sintáctica, referencial y de seguridad. |
| `apps/vigilio-browser/CMakePresets.json` | Modificar (en APPLY) | Ampliación del catálogo de presets canónicos para soportar perfiles completos. |
| `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp` | Crear (en APPLY) | Suite de pruebas unitarias para validación de presets. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificar (en APPLY) | Registro de fuentes en `vigilio_core` y target de prueba `vigilio_presets_tests`. |
| `docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md` | Modificar (en APPLY) | Sincronización del estado de ejecución de la microtarea. |

---

## Contrato UX/UI

- **Sin cambio visual directo en la aplicación**: Esta etapa corresponde estrictamente a la infraestructura de compilación, presets de CMake y especificación de dominio técnico.
- **Justificación de ausencia visual**: Las herramientas de construcción operan a nivel de toolchain, scripts de empaquetado y terminal de desarrollo; no añaden ventanas, menús, botones ni alertas en la interfaz gráfica del navegador.
- **Conformidad con directrices públicas**: No se introducen términos de ingeniería («CMake», «Presets», «Toolchain», «Prompt Builder») en ningún componente visual visible al usuario final, respetando estrictamente `docs/UX-COPY-PUBLICO.md`, `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.

---

## Reglas y fallos

1. **Versión de esquema incompatible**:
   - Condición: El campo `"version"` es inferior a 3 o no numérico.
   - Resultado: Retorna `CMakePresetValidationOutcome::InvalidSchemaVersion` con diagnóstico accionable.
2. **Presupuesto mínimo de presets insatisfecho**:
   - Condición: El archivo carece de `core-debug` o `desktop-debug`.
   - Resultado: Retorna `CMakePresetValidationOutcome::MissingRequiredPreset`, impidiendo configuraciones truncadas.
3. **Referencias huérfanas entre presets**:
   - Condición: Un `buildPreset` o `testPreset` define `"configurePreset": "inexistente"`.
   - Resultado: Retorna `CMakePresetValidationOutcome::OrphanConfigurePresetReference`, identificando el preset y la referencia rota.
4. **Colisión de nombres**:
   - Condición: Dos presets en la misma categoría comparten el mismo `"name"`.
   - Resultado: Retorna `CMakePresetValidationOutcome::DuplicatePresetName`.
5. **Ruta de binarios no segura (Path Traversal / Absolute Host Path)**:
   - Condición: El campo `binaryDir` incluye `..` o rutas absolutas fijadas a directorios privados del sistema.
   - Resultado: Retorna `CMakePresetValidationOutcome::UnsafeBinaryDir`, garantizando que todo artefacto de construcción resida dentro de `${sourceDir}/build-*`.
6. **Variables de caché conflictivas**:
   - Condición: Combinación de `VIGILIO_BUILD_GUI=OFF` con `VIGILIO_ENABLE_E2E=ON` (los tests E2E requieren la interfaz gráfica).
   - Resultado: Retorna `CMakePresetValidationOutcome::ConflictingVariables`.
7. **JSON sintácticamente malformado**:
   - Condición: Archivo vacío, caracteres no imprimibles o JSON truncado.
   - Resultado: Retorna `CMakePresetValidationOutcome::MalformedJson`.

---

## Seguridad y privacidad

- **Local-First Constitucional (Decisión Q-001)**: Todos los presets se resuelven exclusivamente mediante compiladores y librerías instaladas en el sistema local. Prohibida cualquier descarga implícita de toolchains remotos o llamadas de telemetría a servidores cloud.
- **Aislamiento de Rutas y Privacidad de Desarrolladores**: Ningún preset puede incorporar rutas duras de usuarios (`/home/usuario`, `C:\Users\...`) para evitar fugas de nombres de usuario o estructuras de directorios privadas en repositorios públicos.
- **Prevención de Ataques de Path Traversal**: Validación estricta de que los artefactos generados no puedan escapar del árbol del proyecto mediante manipulaciones en `binaryDir`.
- **Presupuesto de Recursos de Construcción**: Separación de presets livianos (`core-debug`, < 1s) frente a presets pesados de navegador completo (`desktop-debug`), garantizando entornos amigables con máquinas de recursos acotados.

---

## Matriz de pruebas

| ID de Prueba | Ámbito | Componente / Fixture | Entrada / Condición | Resultado Esperado | Modo de Fallo Negativo | Ciclo de Vida |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `TEST-PRESET-001` | Unitario | `CMakePresetsValidator` / Memoria | JSON canónico con los 9 presets válidos | `outcome == Valid`, reporte limpio | Falso positivo de validación | APPLY |
| `TEST-PRESET-002` | Unitario | `CMakePresetsValidator` / Memoria | JSON con `"version": 2` | `outcome == InvalidSchemaVersion` | Aceptación de versión antigua | APPLY |
| `TEST-PRESET-003` | Unitario | `CMakePresetsValidator` / Memoria | JSON sin preset `core-debug` | `outcome == MissingRequiredPreset` | Omisión de preset crítico | APPLY |
| `TEST-PRESET-004` | Unitario | `CMakePresetsValidator` / Memoria | Build preset con `configurePreset` inexistente | `outcome == OrphanConfigurePresetReference` | Referencia rota no detectada | APPLY |
| `TEST-PRESET-005` | Unitario | `CMakePresetsValidator` / Memoria | Test preset con `configurePreset` inexistente | `outcome == OrphanConfigurePresetReference` | Test preset huérfano permitido | APPLY |
| `TEST-PRESET-006` | Unitario | `CMakePresetsValidator` / Memoria | Dos configure presets con nombre `"core-debug"` | `outcome == DuplicatePresetName` | Nombres duplicados tolerados | APPLY |
| `TEST-PRESET-007` | Seguridad | `CMakePresetsValidator` / Memoria | `binaryDir: "${sourceDir}/../../etc"` | `outcome == UnsafeBinaryDir` | Vulnerabilidad de Directory Traversal | APPLY |
| `TEST-PRESET-008` | Seguridad | `CMakePresetsValidator` / Memoria | `binaryDir: "/var/tmp/build"` (absoluta fija) | `outcome == UnsafeBinaryDir` | Fuga de ruta absoluta | APPLY |
| `TEST-PRESET-009` | Unitario | `CMakePresetsValidator` / Memoria | `BUILD_GUI=OFF` con `ENABLE_E2E=ON` | `outcome == ConflictingVariables` | Configuración imposible aceptada | APPLY |
| `TEST-PRESET-010` | Unitario | `CMakePresetsValidator` / Memoria | Cadena vacía o JSON truncado | `outcome == MalformedJson` | Excepción o fallo de segmentación | APPLY |
| `TEST-PRESET-011` | Integración | `CMakePresetsValidator` / Archivo | Lectura directa de `CMakePresets.json` del proyecto | `outcome == Valid`, lista 100% de presets requeridos | Desincronización entre JSON y código | APPLY |
| `TEST-PRESET-012` | Rendimiento | Pipeline `check.sh` / CTest | Validación de 50 presets en bucle | Tiempo total < 50 ms, RAM < 2 MB | Degradación de tiempo de ejecución | APPLY |

---

## Pasos APPLY

Plan secuencial y determinista para la ejecución de la fase APPLY (`module.foundations.task0033` en ejecución `0098`):

1. **Definición de Tipos del Dominio en `CMakePresetsTypes.h`**:
   - Declarar `enum class CMakePresetKind`, `enum class CMakeBuildType`, `enum class CMakeGenerator`, `enum class CMakePresetValidationOutcome`.
   - Definir `struct CMakePresetDefinition` y `struct CMakePresetsValidationReport`.
2. **Declaración del Validador Desacoplado en `CMakePresetsValidator.h`**:
   - Declarar `class CMakePresetsValidator` con métodos estáticos `validatePresetsJson`, `validatePresetsFile`, `isSafeBinaryDir`, `canonicalPresetNames` y `outcomeToString`.
3. **Implementación de la Lógica de Validación en `CMakePresetsValidator.cpp`**:
   - Implementar parser liviano de JSON para CMake Presets v6 sin dependencias externas pesadas.
   - Implementar comprobación de versiones, detección de duplicados, validación de referencias huérfanas y reglas de higiene de rutas.
4. **Estandarización y Ampliación de `apps/vigilio-browser/CMakePresets.json`**:
   - Incorporar los presets canónicos: `core-debug`, `core-release`, `desktop-debug`, `desktop-release`, `desktop-e2e`, `ci-linux-debug`, `ci-linux-release`, `sanitizer-asan`, `sanitizer-ubsan`.
5. **Implementación de Pruebas Unitarias en `tests/unit/CMakePresetsTests.cpp`**:
   - Crear suite de pruebas exhaustiva para validar los 12 escenarios de la matriz de pruebas (`TEST-PRESET-001` a `TEST-PRESET-012`).
6. **Configuración en el Build System (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Incorporar `src/core/CMakePresetsValidator.cpp` en `vigilio_core`.
   - Añadir ejecutable `vigilio_presets_tests` vinculado a `vigilio_core` y `vigilio_compiler_warnings`.
   - Registrar test `cmake_presets_tests` bajo CTest con labels `unit;fast` y timeout de 15 segundos.
7. **Verificación del Pipeline de Compilación y Calidad**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` garantizando compilación con `-Werror`, Clang-Format impecable y 100% de tests pasando en CTest.
8. **Validación de Gobernanza y Cierre Documental**:
   - Validar referencias canónicas y lockfile con Prompt Builder CLI.
   - Sincronizar `docs/rules/01-modules/foundations/micro/0033-cmake-presets/task.md`.

---

## Definición de terminado

La etapa `domain` de la capacidad **CMake presets** se considerará completada cuando:
1. El archivo `openspec/changes/module-foundations-task0033/proposal.md` defina exhaustivamente la arquitectura, taxonomía, contratos y matrices de pruebas requeridas.
2. El archivo `openspec/changes/module-foundations-task0033/design.md` formalice la comparación CURRENT/TARGET/GAPS, modelos C++20, diagramas de transición e invariantes de seguridad.
3. El archivo `openspec/changes/module-foundations-task0033/tasks.md` especifique 9 tareas atómicas, ordenadas y verificables para la fase APPLY.
4. El archivo `openspec/changes/module-foundations-task0033/execution-memory.md` documente la autoridad canónica, skills vinculantes, hechos compactos y handoff.
5. El catálogo canónico de 9 presets cubra la totalidad de los escenarios de desarrollo, CI y auditoría de seguridad del proyecto.
6. La validación con Prompt Builder CLI (`validate-references` y `verify-lock`) confirme el 100% de coherencia documental.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0032`: Completada satisfactoriamente (batería de pruebas de crash recovery policy pasando al 100%).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Discrepancias de formato en rutas relativas de `binaryDir` entre Windows (`\`) y sistemas POSIX (`/`).
    *Mitigación*: Uso estricto de barras diagonales `/` soportadas universalmente por CMake en todas las plataformas y normalización mediante `std::filesystem::path::generic_string()`.
  - *Riesgo*: Presets huérfanos generados durante el desarrollo que provoquen fallos silenciosos en entornos CI.
    *Mitigación*: Validación estricta en tiempo de pruebas unitarias (`TEST-PRESET-004` y `TEST-PRESET-005`) certificando que no existan referencias rotas.
  - *Riesgo*: Sobrecarga en tiempo de ejecución de las pruebas unitarias.
    *Mitigación*: El validador opera en memoria sin llamadas a procesos externos, garantizando tiempos de prueba inferiores a 50 ms.

---

## Commit propuesto

`docs(foundations): domain cmake-presets`
