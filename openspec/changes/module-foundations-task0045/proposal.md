# Change Proposal — module-foundations-task0045

Pack: `module.foundations.task0045`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **clang-tidy configuration** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0045 — Definir el comportamiento y las invariantes: clang-tidy configuration`).

Habiéndose completado y certificado la capacidad precedente de *clang-format configuration* a lo largo de sus cuatro fases (`task0041` a `task0044`), este pack inaugura el diseño formal de la infraestructura de análisis estático y linteo de código C++20 con Clang-Tidy en el navegador.

El objetivo esencial de este pack es **especificar y formalizar el modelo de dominio, reglas, tipos inmutables, invariantes de seguridad y motor estático de validación para la configuración de Clang-Tidy (`.clang-tidy`)**, desacoplado de la interfaz gráfica y de cualquier proceso externo pesado.

El alcance concreto comprende:
- **Modelo de dominio fuertemente tipado en C++20 (`ClangTidyTypes.h`)**:
  - Definición de familias de comprobaciones canónicas (`ClangTidyCheckCategory`: Bugprone, ClangAnalyzer, Performance, Modernize, Readability, CppCoreGuidelines, Portability, Custom).
  - Taxonomía exhaustiva de estados de validación (`ClangTidyValidationState`: Unconfigured, Configured, Valid, Invalid, DiscrepancyDetected).
  - Clasificación fuertemente tipada de errores de configuración (`ClangTidyError`: None, FileNotFound, ParseError, EmptyChecks, InvalidHeaderFilterRegex, UnsafeFilePath, CorruptedRules, MissingMandatoryKey).
  - Estructuras inmutables de configuración (`ClangTidyOptions`), reporte de validación (`ClangTidyValidationReport`) e inspección de hallazgos (`ClangTidyIssueInspection`).
- **Validador estático desacoplado y parser YAML determinista (`ClangTidyValidator`)**:
  - Validación de coherencia de opciones (`validateOptions`): comprobación de que la cadena de checks no esté vacía, no deshabilite todas las reglas sin habilitar ninguna y no contenga caracteres de escape anómalos.
  - Validación sintáctica y de seguridad de expresiones regulares en `HeaderFilterRegex` (`isSafeRegex`), evitando fallos en tiempo de ejecución o expresiones catastróficas.
  - Parser ligero y seguro de archivos de configuración YAML (`parseConfig`) enfocado en las directivas canónicas del proyecto (`Checks`, `WarningsAsErrors`, `HeaderFilterRegex`, `FormatStyle`), con soporte para comentarios y delimitación por comillas simples o dobles.
  - Deserializador y particionador de listas de comprobaciones (`parseCheckTokens`), segregando reglas habilitadas (`+` o sin prefijo) de reglas deshabilitadas (`-`).
  - Serializador canónico determinista (`serializeConfig`) que genera el documento normalizado `.clang-tidy`.
  - Verificación rigurosa de seguridad de rutas (`isSafeConfigPath`) para prevenir ataques de Directory Traversal (`../`, `..\\`) o sondeo de rutas del sistema operativo.
  - Conversores de estados y errores a texto en español neutro, cumpliendo estrictamente con la política de copy público de `docs/UX-COPY-PUBLICO.md`.
- **Entidad de configuración canónica (`ClangTidyConfiguration`)**:
  - Provisión de la configuración de referencia del repositorio (`canonicalConfig()`), alineada exactamente con `apps/vigilio-browser/.clang-tidy`:
    `Checks: '-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements'`
    `WarningsAsErrors: ''`
    `HeaderFilterRegex: 'src/.*'`
  - Filtro oficial de archivos fuente inspeccionables (`isSupportedSourceExtension`: `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`).
  - Filtro oficial de directorios excluidos de análisis (`isExcludedDirectory`: `_build`, `build`, `generated`, `.git`, `.prompt-runner`, `assets`).
  - Verificación de invariantes fundamentales del proyecto (`verifyInvariants`).
- **Batería exhaustiva de pruebas unitarias de especificación (`ClangTidyConfigurationTests.cpp`)**:
  - 11 pruebas unitarias deterministas en CTest ejecutables en modo headless (< 50 ms), garantizando cero dependencias de red, sin instanciación de WebEngine y total aislamiento Local-First.

---

## Out of scope

- Implementación del servicio ejecutor o adaptador operativo (`ClangTidyService`), ejecución de subprocesos externos de Clang-Tidy con `compile_commands.json` o generación de reportes de diagnóstico sobre el árbol de código (reservado para la etapa `implementation` en `module.foundations.task0046`).
- Creación de diálogos de interfaz gráfica, componentes Qt Widgets, paneles de configuración en ventana principal o menús de linters para el usuario final (reservado para la etapa `screens` en `module.foundations.task0047`).
- Pruebas pesadas de estrés de linteo multihilo, concurrencia masiva y recuperación ante fallos de procesos (reservado para la etapa `tests` en `module.foundations.task0048`).
- Modificación de la ventana principal de navegación (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`) o adición de controles que saturen la interfaz o expongan conceptos de ingeniería interna (proscrito por `docs/UX-COPY-PUBLICO.md`).
- Conexión a redes externas, envío de telemetría de análisis o integración con servicios cloud de linteo (vetado taxativamente por ADR-001 / Decisión Q-001 Local-First).
- Ejecución de APPLY durante esta fase PROPOSE o modificación de código de aplicación fuera de las extensiones autorizadas.

---

## Contexto y evidencia

1. **Configuración física canónica existente en el repositorio**:
   - En `apps/vigilio-browser/.clang-tidy` se encuentra definida la configuración canónica actual:
     ```yaml
     Checks: '-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements'
     WarningsAsErrors: ''
     HeaderFilterRegex: 'src/.*'
     ```
   - Esta configuración prioriza la detección de errores de programación (`bugprone-*`), análisis estático profundo (`clang-analyzer-*`), optimizaciones de rendimiento y copias innecesarias (`performance-*`), modernización idiomática hacia C++20 (`modernize-use-nullptr`) y consistencia en estructuras de control (`readability-braces-around-statements`), restringiendo el análisis de encabezados al código propio del navegador (`src/.*`) para no inspeccionar dependencias externas (Qt, SQLite).
2. **Infraestructura de verificación existente**:
   - En `apps/vigilio-browser/scripts/check.sh`:
     ```bash
     if command -v clang-tidy >/dev/null 2>&1; then
         echo 'clang-tidy available: run with compile_commands.json on supported compiler'
     else
         echo 'clang-tidy NOT_RUN (not installed)'
     fi
     ```
   - En el entorno de ejecución, Clang-Tidy se encuentra disponible (Ubuntu LLVM versión 21.1.8).
   - El pipeline actual de `check.sh` pasa al 100% con 17/17 pruebas en CTest (tiempo de ejecución total: 2.33s).
3. **Validación de referencias y lockfile**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0133-module.foundations.task0045.propose.lock.json --project-root .` confirma `valid: true` con todas las comprobaciones aprobadas.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` confirma `valid: true` sobre las 45 referencias canónicas del proyecto.
4. **Estado de la capacidad (CURRENT vs TARGET)**:
   - *CURRENT*: La configuración de Clang-Tidy existe únicamente como archivo estático plano en disco. No existe modelo de tipos en C++20, ni validador de seguridad, ni parser YAML, ni representación tipada en el código fuente de `vigilio_core`.
   - *TARGET*: Modelo de dominio completo, validador estático, parser determinista, filtros de archivos y suite de pruebas unitarias integradas en `vigilio_core` y CTest.

---

## Alcance exacto

El delta exacto de la capacidad en la etapa **domain** comprende:
1. `apps/vigilio-browser/src/core/ClangTidyTypes.h`: Archivo de cabecera nuevo con la taxonomía de tipos, enumeraciones fuertemente tipadas y estructuras inmutables.
2. `apps/vigilio-browser/src/core/ClangTidyValidator.h`: Archivo de cabecera nuevo con la declaración de la clase utilitaria estática `ClangTidyValidator`.
3. `apps/vigilio-browser/src/core/ClangTidyValidator.cpp`: Implementación del parser YAML ligero, validación de reglas, validación de regex, seguridad de rutas y cadenas de copy público.
4. `apps/vigilio-browser/src/core/ClangTidyConfiguration.h` y `ClangTidyConfiguration.cpp`: Declaración e implementación de la configuración canónica, catálogo de extensiones soportadas y exclusión de carpetas.
5. `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp`: Batería exhaustiva de 11 pruebas unitarias trazables de especificación.
6. `apps/vigilio-browser/CMakeLists.txt`: Registro de los nuevos fuentes en la biblioteca `vigilio_core`, definición del ejecutable de pruebas `vigilio_clang_tidy_tests` y vinculación del test `clang_tidy_configuration_tests` con labels `unit;fast`.
7. `docs/rules/01-modules/foundations/micro/0045-clang-tidy-configuration/task.md`: Sincronización documental registrando la culminación de la etapa domain.

---

## Diseño completo de la capacidad

### 1. Entidades de datos y responsabilidades
- **`ClangTidyOptions`**: Representa la configuración inmutable de Clang-Tidy. Encapsula las directivas `checks`, `warningsAsErrors`, `headerFilterRegex` y `formatStyle`. Implementa `operator==` para comprobaciones de igualdad por valor.
- **`ClangTidyValidationReport`**: Encapsula el resultado de la validación de un archivo o contenido de configuración: validez booleana (`isValid`), código de error fuertemente tipado (`ClangTidyError`), mensaje descriptivo legible, lista de comprobaciones activadas (`enabledChecks`), lista de comprobaciones desactivadas (`disabledChecks`) y avisos (`warnings`).
- **`ClangTidyValidator`**: Proveedor de lógica estática pura sin estado mutable. No retiene memoria ni descriptores de archivo. Provee métodos para validar estructuras en memoria (`validateOptions`), validar archivos en disco (`validateConfigurationFile`), validar texto YAML (`validateConfigContent`), parsear YAML (`parseConfig`), serializar a YAML normalizado (`serializeConfig`), validar contención de rutas (`isSafeConfigPath`) y validar expresiones regulares (`isSafeRegex`).
- **`ClangTidyConfiguration`**: Proveedor centralizado de la configuración canónica del proyecto (`canonicalConfig()`), discriminador de extensiones fuente C++ (`isSupportedSourceExtension`) y discriminador de rutas ignoradas (`isExcludedDirectory`).

### 2. Contrato funcional
- **Actor/Invocación**: Invocable programáticamente desde pruebas unitarias, servicios internos de calidad y arneses de integración mediante llamadas estáticas en C++20.
- **Entradas**: Rutas de sistema de archivos (`std::filesystem::path`), vistas de cadenas de texto (`std::string_view`) y estructuras de opciones (`ClangTidyOptions`).
- **Salidas**: Estructuras inmutables con semántica de valor (`ClangTidyValidationReport`, `ClangTidyError`, `std::string`, `bool`).
- **Validación**:
  - `checks` no puede ser una cadena vacía ni estar compuesta únicamente por espacios en blanco o reglas de deshabilitación (`-*`) sin ninguna regla habilitada.
  - `headerFilterRegex` debe ser una expresión regular sintácticamente válida y segura.
  - `configPath` debe ser una ruta existente, accesible para lectura y libre de Directory Traversal (`..`).
- **Recuperación**: Toda anomalía es reportada mediante un código tipado en `ClangTidyError` y un mensaje descriptivo en lenguaje natural; nunca se lanzan excepciones no controladas ni se produce aborto del proceso.

---

## Archivos afectados

| Archivo | Rol | Estado | Descripción de cambio |
| --- | --- | --- | --- |
| `apps/vigilio-browser/src/core/ClangTidyTypes.h` | Dominio | Nuevo (APPLY) | Tipos, enumeraciones `ClangTidyCheckCategory`, `ClangTidyValidationState`, `ClangTidyError`, estructuras `ClangTidyOptions`, `ClangTidyValidationReport`. |
| `apps/vigilio-browser/src/core/ClangTidyValidator.h` | Dominio | Nuevo (APPLY) | Interfaz pública estática del validador, parser YAML, analizador de checks y copy público. |
| `apps/vigilio-browser/src/core/ClangTidyValidator.cpp` | Dominio | Nuevo (APPLY) | Implementación de parsing YAML, validación de regex, seguridad de rutas y textos legibles. |
| `apps/vigilio-browser/src/core/ClangTidyConfiguration.h` | Dominio | Nuevo (APPLY) | Declaración de `canonicalConfig()`, filtros de extensiones y carpetas excluidas. |
| `apps/vigilio-browser/src/core/ClangTidyConfiguration.cpp` | Dominio | Nuevo (APPLY) | Implementación de la configuración canónica y reglas de inclusión/exclusión. |
| `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp` | Pruebas | Nuevo (APPLY) | 11 pruebas unitarias trazables (`TEST-TIDY-001` a `TEST-TIDY-011`). |
| `apps/vigilio-browser/CMakeLists.txt` | Build | Modificado (APPLY) | Registro de fuentes en `vigilio_core`, definición de target ejecutable `vigilio_clang_tidy_tests` y test CTest `clang_tidy_configuration_tests`. |
| `docs/rules/01-modules/foundations/micro/0045-clang-tidy-configuration/task.md` | Documentación | Sincronizado (APPLY) | Actualización de estado y trazabilidad canónica. |

---

## Contrato UX/UI

- **Sin cambio visual directo en la interfaz de navegación**:
  - De acuerdo con la arquitectura del módulo `foundations` y la política de diseño en `resultado/01-vigilio-home.png` y `docs/UX-COPY-PUBLICO.md`, esta etapa de especificación de dominio no introduce botones, ventanas ni elementos gráficos en el navegador de usuario.
- **Cumplimiento estricto de textos públicos (UX Copy)**:
  - Todas las cadenas legibles emitidas por `ClangTidyValidator::tidyStateToString` y `ClangTidyValidator::tidyErrorToString` están redactadas en español neutro, claro y accesible para el usuario.
  - Se prohíbe taxativamente la inclusión de términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - Mapeo de cadenas públicas:
    - `Unconfigured` $\rightarrow$ `"Análisis de calidad no configurado"`
    - `Configured` $\rightarrow$ `"Análisis de calidad configurado"`
    - `Valid` $\rightarrow$ `"Configuración de análisis válida"`
    - `Invalid` $\rightarrow$ `"Configuración de análisis no válida"`
    - `DiscrepancyDetected` $\rightarrow$ `"Discrepancias detectadas en el análisis"`
    - `None` $\rightarrow$ `"Sin errores en la configuración"`
    - `FileNotFound` $\rightarrow$ `"No se encontró el archivo de configuración"`
    - `ParseError` $\rightarrow$ `"Error al interpretar el archivo de configuración"`
    - `EmptyChecks` $\rightarrow$ `"No se han especificado reglas de análisis activas"`
    - `InvalidHeaderFilterRegex` $\rightarrow$ `"La expresión de filtrado de archivos no es válida"`
    - `UnsafeFilePath` $\rightarrow$ `"La ruta del archivo de configuración no es segura"`
    - `CorruptedRules` $\rightarrow$ `"Las reglas de análisis contienen valores corruptos"`
    - `MissingMandatoryKey` $\rightarrow$ `"Faltan directivas obligatorias en la configuración"`

---

## Reglas y fallos

1. **Regla de presencia y consistencia de directivas (`Checks`)**:
   - El documento YAML debe contener la clave obligatoria `Checks`. Si la clave está ausente o su valor es vacío/nulo, la validación falla inmediatamente con `ClangTidyError::MissingMandatoryKey` o `ClangTidyError::EmptyChecks`.
   - Si la cadena de comprobaciones únicamente desactiva reglas (e.g. `Checks: '-*'`) sin incluir ninguna regla habilitada con posterioridad, la configuración es rechazada con `ClangTidyError::EmptyChecks`, ya que no proporciona valor analítico.
2. **Regla de seguridad y corrección en `HeaderFilterRegex`**:
   - La directiva `HeaderFilterRegex` define el patrón de inclusión de encabezados para el análisis estático.
   - Debe pasar la validación estática de `isSafeRegex`: la expresión debe poder compilarse mediante `std::regex` sin excepciones (`std::regex_error`), no debe superar una longitud de 256 caracteres y no debe contener secuencias maliciosas orientadas a denegación de servicio por retroceso exponencial (ReDoS).
   - Ante error de sintaxis, se retorna `ClangTidyError::InvalidHeaderFilterRegex`.
3. **Regla de prevención de Directory Traversal y contención de rutas**:
   - `isSafeConfigPath` comprueba que la ruta no contenga separadores hacia directorios superiores (`..`), caracteres nulos (`\0`) ni apunte a ubicaciones críticas del sistema operativo (`/etc`, `/usr`, `/root`). Si falla, se genera `ClangTidyError::UnsafeFilePath`.
4. **Regla de archivos ausentes o no legibles**:
   - Si la ruta no existe o es un directorio, `validateConfigurationFile` retorna limpiamente `ClangTidyError::FileNotFound` sin provocar fallos de segmentación.
5. **Regla de idempotencia y roundtrip en serialización**:
   - La serialización de un objeto `ClangTidyOptions` a través de `serializeConfig` y su posterior análisis mediante `parseConfig` produce un objeto idéntico (`options == roundtripOptions`), garantizando preservación exacta de todas las directivas.

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Todo el análisis de configuración y parsing de Clang-Tidy opera de forma 100% local en la máquina del usuario. No se abren puertos de red, no se envían datos a ningún servidor externo ni se solicita telemetría.
- **Aislamiento estricto de memoria y CPU**:
  - El parsing se implementa mediante vistas eficientes `std::string_view`, evitando asignaciones dinámicas superfluas.
  - El tiempo de validación de un archivo `.clang-tidy` típico es inferior a 1 milisegundo y el consumo de memoria temporal es inferior a 32 KB.
- **Sin acoplamiento con subprocesos ni Chromium**:
  - Ninguna clase del dominio de Clang-Tidy instancia procesos de Chromium, Qt WebEngine o componentes MCP. El código compila limpiamente en entornos sin pantalla (`VIGILIO_BUILD_GUI=OFF`).

---

## Matriz de pruebas

| Identificador | Nombre de la prueba | Tipo | Comando / Fixture | Resultado esperado | Criterio de fallo negativo |
| --- | --- | --- | --- | --- | --- |
| `TEST-TIDY-001` | `test_clang_tidy_canonical_options` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | `canonicalConfig()` retorna checks canónicos (`-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-braces-around-statements`), `warningsAsErrors=""`, y `headerFilterRegex="src/.*"`. | Falla si algún campo difiere de la especificación física en `apps/vigilio-browser/.clang-tidy`. |
| `TEST-TIDY-002` | `test_clang_tidy_validator_valid_options` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | `validateOptions` sobre la configuración canónica retorna `ClangTidyError::None`. | Falla si una configuración válida es erróneamente rechazada. |
| `TEST-TIDY-003` | `test_clang_tidy_validator_empty_checks` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Opciones con `checks=""`, solo espacios en blanco o únicamente `"-*"` retornan `ClangTidyError::EmptyChecks`. | Falla si se aceptan configuraciones sin reglas activas. |
| `TEST-TIDY-004` | `test_clang_tidy_validator_invalid_header_filter` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Expresiones regulares no balanceadas o corruptas en `headerFilterRegex` (e.g. `"[a-z("`) retornan `ClangTidyError::InvalidHeaderFilterRegex`. | Falla si se aceptan expresiones regulares malformadas. |
| `TEST-TIDY-005` | `test_clang_tidy_parse_canonical_yaml` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Parsea exitosamente el contenido real de `apps/vigilio-browser/.clang-tidy` produciendo las opciones esperadas. | Falla si no interpreta correctamente el archivo YAML físico del repositorio. |
| `TEST-TIDY-006` | `test_clang_tidy_serialize_roundtrip` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | `serializeConfig` genera un documento YAML que, al re-parsearse con `parseConfig`, resulta en un objeto idéntico por valor. | Falla si se pierde fidelidad en el ciclo roundtrip. |
| `TEST-TIDY-007` | `test_clang_tidy_missing_mandatory_keys` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Documentos YAML sin la clave `Checks` o con sintaxis ilegible retornan `ClangTidyError::MissingMandatoryKey` o `ClangTidyError::ParseError`. | Falla si se da por válido un archivo incompleto. |
| `TEST-TIDY-008` | `test_clang_tidy_unsafe_path_rejection` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Rutas con `../../etc/.clang-tidy`, `..\\windows\\tidy` o `/etc/shadow` son rechazadas por `isSafeConfigPath`. | Falla si se permite el procesamiento de rutas inseguras con directory traversal. |
| `TEST-TIDY-009` | `test_clang_tidy_file_extensions_and_exclusions` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | Extensiones `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp` son admitidas; extensiones `.txt`, `.json`, `.qrc` son rechazadas; carpetas `_build`, `build`, `generated`, `.git` son excluidas. | Falla si se admiten archivos no C++ o si no se filtran directorios ignorados. |
| `TEST-TIDY-010` | `test_clang_tidy_token_extraction` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | `parseCheckTokens` clasifica correctamente `-*` como deshabilitado y `bugprone-*`, `performance-*`, `modernize-use-nullptr`, `readability-braces-around-statements` como habilitados. | Falla si no se discriminan adecuadamente los prefijos de habilitación y deshabilitación. |
| `TEST-TIDY-011` | `test_clang_tidy_public_copy_compliance` | Unitario (APPLY) | `vigilio_clang_tidy_tests` | `tidyStateToString` y `tidyErrorToString` devuelven mensajes en español sin términos prohibidos por `docs/UX-COPY-PUBLICO.md`. | Falla si aparecen términos técnicos internos prohibidos en el copy público. |

- *Clasificación de pruebas*:
  - **APPLY**: Pruebas unitarias puras (`TEST-TIDY-001` a `TEST-TIDY-011`) ejecutables en CTest headless (< 50 ms).
  - **LATER**: Pruebas de integración del servicio de ejecución (`task0046`), pruebas de diálogo/UI (`task0047`) y pruebas de concurrencia y resiliencia de procesos (`task0048`).
  - **NOT_APPLICABLE**: Pruebas con navegación WebEngine viva, perfiles de Chromium o dependencias de red externa.

---

## Pasos APPLY

1. **Crear tipos y modelos de datos**: Implementar `apps/vigilio-browser/src/core/ClangTidyTypes.h` definiendo las enumeraciones `ClangTidyCheckCategory`, `ClangTidyValidationState`, `ClangTidyError`, y las estructuras inmutables `ClangTidyOptions`, `ClangTidyValidationReport` y `ClangTidyIssueInspection`.
2. **Declarar interfaz del validador y parser**: Crear `apps/vigilio-browser/src/core/ClangTidyValidator.h` con las firmas estáticas de validación, parsing, serialización, extracción de tokens, seguridad de rutas y UX copy.
3. **Implementar validador y parser YAML**: Crear `apps/vigilio-browser/src/core/ClangTidyValidator.cpp` implementando el análisis de pares clave-valor YAML, validación de regex con `std::regex`, verificación de rutas con `std::filesystem`, y mapeo de textos legibles en español.
4. **Declarar e implementar configuración canónica**: Crear `apps/vigilio-browser/src/core/ClangTidyConfiguration.h` y `apps/vigilio-browser/src/core/ClangTidyConfiguration.cpp` con `canonicalConfig()`, filtros de extensiones soportadas y directorios excluidos.
5. **Implementar batería de pruebas unitarias**: Crear `apps/vigilio-browser/tests/unit/ClangTidyConfigurationTests.cpp` con los 11 escenarios trazables de especificación.
6. **Actualizar `CMakeLists.txt`**: Incorporar `ClangTidyValidator.cpp` y `ClangTidyConfiguration.cpp` en `vigilio_core`, definir el ejecutable `vigilio_clang_tidy_tests` vinculado a `ClangTidyConfigurationTests.cpp`, configurar la macro `VIGILIO_CLANG_TIDY_PATH` y registrar el test `clang_tidy_configuration_tests` con labels `unit;fast`.
7. **Verificar compilación limpia y tests**: Ejecutar `bash apps/vigilio-browser/scripts/check.sh` certificando compilación Ninja con `-Werror`, cero advertencias y 18/18 pruebas aprobadas en CTest.
8. **Validar referencias canónicas y lockfile**: Ejecutar `validate-references` y `verify-lock` con Prompt Builder CLI certificando integridad total.
9. **Sincronizar documentación de microtarea**: Actualizar `docs/rules/01-modules/foundations/micro/0045-clang-tidy-configuration/task.md` formalizando el cumplimiento de la etapa domain.

---

## Definición de terminado

- [ ] Tipos de datos, enumeraciones y estructuras inmutables de Clang-Tidy formalizados en `ClangTidyTypes.h`.
- [ ] Validador estático, parser determinista de YAML y serializador implementados en `ClangTidyValidator.h` y `ClangTidyValidator.cpp`.
- [ ] Entidad de configuración canónica y políticas de filtrado implementadas en `ClangTidyConfiguration.h` y `ClangTidyConfiguration.cpp`.
- [ ] 11 pruebas unitarias de especificación implementadas en `ClangTidyConfigurationTests.cpp` pasando al 100% en CTest.
- [ ] Target `vigilio_core` y test `clang_tidy_configuration_tests` registrados en `CMakeLists.txt`.
- [ ] Cero advertencias bajo `-Wall -Wextra -Wpedantic -Werror` y formato C++20 validado.
- [ ] Cumplimiento de textos públicos y copy en español conforme a `docs/UX-COPY-PUBLICO.md`.
- [ ] Aprobación de `validate-references` y `verify-lock` en Prompt Builder CLI.

---

## Dependencias y riesgos

- **Dependencias ejecutables**: `module.foundations.task0044` (completada y certificada).
- **Riesgos identificados y mitigaciones**:
  - *Riesgo 1*: Desincronización entre el archivo físico `.clang-tidy` y la entidad canónica en C++.
    *Mitigación*: La prueba `TEST-TIDY-005` analiza directamente el archivo físico del repositorio y valida su coincidencia exacta con `canonicalConfig()`.
  - *Riesgo 2*: Complejidad o lentitud en expresiones regulares de `HeaderFilterRegex`.
    *Mitigación*: `isSafeRegex` limita la longitud del patrón, rechaza caracteres nulos y valida la compilación estricta con `std::regex`, garantizando tiempos de verificación $< 1$ ms.
  - *Riesgo 3*: Introducción involuntaria de jerga técnica interna en textos de error.
    *Mitigación*: `TEST-TIDY-011` audita programáticamente que ninguna cadena generada contenga palabras restringidas por la política de copy público.

---

## Commit propuesto

`docs(foundations): domain clang-tidy-configuration`
