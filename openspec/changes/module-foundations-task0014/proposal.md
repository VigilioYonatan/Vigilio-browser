# Change Proposal — module-foundations-task0014

Pack: `module.foundations.task0014`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **WebEngine version compatibility** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado en `module.foundations.task0013` la especificación del modelo de dominio, la taxonomía de capacidades, los perfiles de requerimiento y las reglas de compatibilidad de versiones de Chromium (Chromium milestone >= 128 asociado al baseline Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), esta microtarea proyecta la implementación física, desacoplada y fuertemente tipada en C++20 de la validación de compatibilidad con el motor WebEngine, sin acoplar lógica de negocio a los widgets de interfaz ni añadir responsabilidades a `BrowserWindow`.

El alcance concreto de este pack comprende:
- **Implementación de la capa propietaria C++20 (`vigilio::WebEngineVersionValidator`)**:
  - Creación del componente de validación desacoplado en `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `WebEngineVersionValidator.cpp`, perteneciente a la biblioteca estática `vigilio_core`.
  - Estructuración inmutable y tipada en C++20 estándar de las entidades del dominio:
    - `ChromiumVersion`: estructura inmutable que modela la versión cuatripartita de Chromium (`major.minor.build.patch`), comparación total mediante el operador de nave espacial (`operator<=>`), parsing robusto desde cadenas textuales, cálculo de compatibilidad con versiones mínimas (`isCompatibleWith`), acceso al hito principal (`milestone()`) y formateo canónico (`toString()`).
    - `WebEngineFeature`: enum class fuertemente tipado que enumera las capacidades funcionales del motor web (`ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`).
    - `WebEngineCompatibilityProfile`: perfiles de compatibilidad declarativos (`FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`).
    - `WebEngineCompatibilityStatus`: taxonomía exhaustiva de estados deterministas de validación (`Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `FeatureUnsupported`, `EnvironmentMisconfigured`).
    - `WebEngineCompatibilityReport`: estructura inmutable que consolida el estado final (`status`), versiones detectadas de Qt y Chromium, lista de características soportadas y ausentes, ruta del ejecutable asistente, diagnóstico legible y acción de mitigación recomendada.
  - Implementación de métodos puros de servicio desacoplados:
    - `WebEngineVersionValidator::parseChromiumVersion`: parsing determinista de cadenas con rechazo de malformaciones.
    - `WebEngineVersionValidator::baselineChromiumVersion`: versión mínima requerida (Chromium 128.0.0.0).
    - `WebEngineVersionValidator::minimumChromiumVersionForQt`: resolución del baseline de Chromium asociado a la versión de Qt detectada.
    - `WebEngineVersionValidator::supportedFeaturesForMilestone`: matriz inmutable de capacidades garantizadas según el hito de Chromium.
    - `WebEngineVersionValidator::requiredFeaturesForProfile`: mapeo determinista entre perfiles de ejecución y características obligatorias.
    - `WebEngineVersionValidator::isHelperExecutable`: validación de la existencia y permisos de ejecución del binario asistente `QtWebEngineProcess`.
    - `WebEngineVersionValidator::validateCompatibility`: evaluación integral del estado de compatibilidad a partir del perfil solicitado, versiones detectadas, ruta del ejecutable asistente y soporte de aislamiento/sandboxing.
    - `WebEngineVersionValidator::validateStartup`: interfaz de pre-validación para el arranque de la aplicación en modo desktop o headless.
    - `WebEngineVersionValidator::formatStatus`, `formatReport`, `featureName`, `profileName`: formateadores de diagnósticos claros para observabilidad y logging.
- **Integración en la configuración de construcción de CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Incorporación de `src/core/WebEngineVersionValidator.cpp` en las fuentes del target estático `vigilio_core`.
  - Garantía de que `vigilio_core` compila limpiamente bajo C++20 con diagnósticos estrictos `-Werror` sin depender de cabeceras ni librerías de `Qt6::WebEngineWidgets` o Chromium.
- **Protocolo de pre-validación en tiempo de ejecución (Startup Bootstrap en `apps/vigilio-browser/src/app.cpp`)**:
  - Incorporación de la validación previa de WebEngine tras la comprobación de dependencias Qt (`QtDependencyValidator`):
    1. Si `VIGILIO_BUILD_GUI=ON`, antes de instanciar `QApplication` y `BrowserWindow`, se evalúa la compatibilidad del entorno con `WebEngineVersionValidator::validateStartup`.
    2. Si el estado resultante es incompatible o el proceso asistente `QtWebEngineProcess` no está disponible, se emite un error estructurado a `stderr` con la acción de mitigación recomendada y se termina limpiamente con código de error, evitando cuelgues o páginas en blanco permanentes.
    3. Si el entorno requiere degradación suave (`DegradedHeadless`), se registra la condición sin provocar caídas anormales de la aplicación.
- **Suite de pruebas unitarias de implementación (`apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`)**:
  - Creación de pruebas unitarias exhaustivas en CTest (`webengine_version_validator_tests`), etiquetadas como `unit;fast`, cubriendo parsing de versiones válidas e inválidas, comparación de hitos, matriz de características, perfiles de requerimientos, comprobación de binario asistente, evaluación de reportes y formateo de diagnósticos.
- **Garantías de aislamiento, rendimiento y privacidad**:
  - *Desacoplamiento headless absoluto*: `vigilio_core` (incluido `WebEngineVersionValidator`) no incluye cabeceras de Chromium ni enlaza con librerías dinámicas de WebEngine; compila y ejecuta 100% en C++20 estándar en cualquier entorno.
  - *Filosofía local-first*: cero peticiones de red, cero descargas de binarios, cero telemetría remota y sin servicios cloud (decisión Q-001).
  - *Presupuesto de recursos*: tiempo de ejecución del validador < 1 ms, huella de memoria RAM < 50 KB, cero procesos Chromium WebEngine iniciados durante la verificación.
  - *Ausencia total de cambio visual directo*: la validación es infraestructura técnica de backend y arranque sin alteración de widgets ni diálogos gráficos, preservando estrictamente `docs/UX-COPY-PUBLICO.md`, `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`.

## Out of scope

- Modificación visual de pantallas, barras de herramientas, menús contextuales o diálogos en la ventana principal `BrowserWindow` (diferido a la etapa `ui` en `module.foundations.task0015`).
- Creación de arneses de prueba de integración pesados o simulación exhaustiva de fallos de recuperación del proceso Chromium (diferido a la etapa `tests` en `module.foundations.task0016`).
- Inicialización o configuración de políticas de privacidad, aislamiento de perfiles de navegación, gestión de cookies o almacenamiento particionado en `QWebEngineProfile` (pertenecientes al módulo `security`/`privacy`, microtareas `0921-0924`).
- Descarga o empaquetado de binarios de Chromium desde la red.
- Modificación directa de código fuente bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida a `openspec/changes/module-foundations-task0014/**`).

## Contexto y evidencia

Se inspeccionó minuciosamente la base de código existente en `apps/vigilio-browser` y las herramientas del sistema:
1. **Infraestructura de validación existente (`src/core/QtDependencyValidator.h` y `.cpp`)**:
   - Implementa `SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome` y `QtValidationReport`.
   - Ha demostrado la viabilidad y robustez del patrón de validación desacoplada en C++20 con suite de pruebas unitarias (`tests/unit/QtDependencyValidatorTests.cpp`) pasando 100% en CTest.
2. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Actualmente ejecuta `QtDependencyValidator::validateRuntimeEnvironment(qVersion(), QT_VERSION_STR, display, wayland)`.
   - Carece de comprobación de compatibilidad con el motor Chromium y no valida la disponibilidad del binario asistente `QtWebEngineProcess`, lo que provoca fallos silenciosos o cuelgues si el motor WebEngine está corrupto o desalineado.
3. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Compila la biblioteca estática `vigilio_core` con C++20 (`cxx_std_20`) y diagnósticos estrictos de `vigilio_compiler_warnings` (`-Werror`).
   - Define opciones `VIGILIO_BUILD_GUI`, `VIGILIO_REQUIRE_QT` y suites de prueba en CTest.
4. **Verificación de herramientas del entorno local**:
   - Compilador: GNU GCC 15.2.0 (C++20, `-std=c++20`).
   - CMake: versión 4.2.3.
   - Ninja: versión 1.13.2.
   - SQLite3: versión 3.46.1.
   - Ejecución del script de comprobación local `apps/vigilio-browser/scripts/check.sh`: compilación limpia bajo `-Werror` con Ninja y 100% de pruebas aprobadas (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`) en 0.13 segundos, exit code 0.
   - Verificación de formateo con Clang-Format: 100% de archivos C++ cumplen el estándar sin diferencias, exit code 0.
5. **Verificación de gobernanza con Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 14 referencias canónicas validadas, estado `valid: true`, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando exclusivamente el hito `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0014`.
- **Capacidad**: `WebEngine version compatibility`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0014/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0014/proposal.md`
  - `openspec/changes/module-foundations-task0014/design.md`
  - `openspec/changes/module-foundations-task0014/tasks.md`
  - `openspec/changes/module-foundations-task0014/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0014/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico completo de la implementación para `WebEngine version compatibility` se estructura en:
1. **Entidades de dominio C++20 (`src/core/WebEngineVersionValidator.h`)**:
   - `ChromiumVersion`: versión cuatripartita (`major.minor.build.patch`) con spaceship operator (`<=>`), parsing robusto con control de desbordamiento, predicado `isCompatibleWith(const ChromiumVersion& minVer)` que comprueba `major >= minVer.major`, y método `milestone()`.
   - `WebEngineFeature`: enum class con las 8 capacidades funcionales del motor (`ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`).
   - `WebEngineCompatibilityProfile`: perfiles `FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`.
   - `WebEngineCompatibilityStatus`: estados `Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `FeatureUnsupported`, `EnvironmentMisconfigured`.
   - `WebEngineCompatibilityReport`: estructura con diagnóstico, versiones detectadas, características faltantes y acción recomendada.
2. **Servicio puro C++20 (`src/core/WebEngineVersionValidator.cpp`)**:
   - Funciones estáticas puras sin efectos secundarios sobre GUI o red.
   - Evaluación de compatibilidad basada en el baseline Chromium 128.0.0.0 y Qt >= 6.10.0.
   - Verificación de ejecutabilidad del proceso asistente `QtWebEngineProcess` mediante comprobación de archivo y permisos locales.
3. **Punto de arranque (Startup Bootstrap en `src/app.cpp`)**:
   - Invocación segura previa a `QApplication` para validar el entorno y fallar de forma descriptiva si los componentes del motor web no están disponibles.
4. **Target CMake y suite de pruebas (`tests/unit/WebEngineVersionValidatorTests.cpp`)**:
   - Integración de `WebEngineVersionValidator.cpp` en `vigilio_core`.
   - Nueva suite de pruebas unitarias CTest para garantizar cobertura completa de casos normales, límites y de error.

## Archivos afectados

Para la subsiguiente fase APPLY (tras aprobación humana de esta propuesta), los archivos a modificar/crear son:
1. `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` (nuevo): declaración de tipos de dominio y del servicio validador en C++20.
2. `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp` (nuevo): implementación de la lógica de parsing, tablas de capacidades y evaluación de compatibilidad.
3. `apps/vigilio-browser/CMakeLists.txt` (modificado): adición de `src/core/WebEngineVersionValidator.cpp` a las fuentes de `vigilio_core`, y registro del ejecutable `vigilio_webengine_validator_tests` en CTest.
4. `apps/vigilio-browser/src/app.cpp` (modificado): adición de la validación previa de WebEngine en el punto de arranque.
5. `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` (nuevo): suite de pruebas unitarias que validan la lógica de compatibilidad, parsing, perfiles y formateo de diagnósticos.

## Contrato UX/UI

- **Declaración de cambio visual**: **Sin cambio visual directo**.
- **Justificación**: Esta microtarea implementa lógica de infraestructura de dominio y arranque en C++20 (`vigilio_core` y `app.cpp`). No agrega widgets, menús, barras ni pantallas visibles al usuario final.
- **Conformidad con `docs/UX-COPY-PUBLICO.md`**:
  - Los términos técnicos de ingeniería («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») están estrictamente restringidos al código C++, diagnósticos de `stderr` para desarrolladores y logs de depuración locales.
  - No se exponen en ninguna interfaz gráfica visible al público. En caso de requerir mención visual en microtareas futuras, se empleará exclusivamente «Interfaz más fácil de usar».
- **Integración con Modo Senior**: Al ser infraestructura técnica subyacente, garantiza la estabilidad, predictibilidad y rendimiento del navegador para todas las modalidades de uso, sin interferir con la navegación por teclado ni lectores de pantalla.
- **Referencias visuales y de marca**: Se respetan las directrices de `resultado/06-logo-instalador-oficial.png` y `resultado/01-vigilio-home.png`.

## Reglas y fallos

- **R-001 (Baseline de Chromium)**: El perfil de navegador de escritorio (`FullDesktopBrowser`) exige Chromium milestone >= 128 (asociado canónicamente a Qt >= 6.10.0 según la decisión Q-002). Cualquier versión inferior resulta en `IncompatibleChromiumVersion`.
- **R-002 (Baseline de Qt)**: Se requiere Qt versión mayor 6 y menor >= 10 (`>= 6.10.0`). Cualquier versión de Qt incompatible resulta en `IncompatibleQtVersion`.
- **R-003 (Disponibilidad de proceso asistente)**: El ejecutable auxiliar `QtWebEngineProcess` debe existir y contar con permisos de ejecución antes de inicializar vistas web. Su ausencia o falta de permisos produce `ProcessHelperMissing`.
- **R-004 (Degradación elegante)**: Si la aplicación se ejecuta en modo headless (`HeadlessInspection` o `MinimalCore`), la ausencia de WebEngine resulta en `DegradedHeadless`, permitiendo continuar operaciones locales sin bloqueos ni caídas.
- **R-005 (Local-first estricto)**: Toda validación opera exclusivamente mediante inspección local de metadatos y sistema de archivos. Queda estrictamente prohibida cualquier consulta de red o servicio externo.
- **Manejo de entradas malformadas**: Cadenas de versión vacías, incompletas, con caracteres no numéricos o desbordamientos de enteros son rechazadas deterministamente retornando `std::nullopt`.
- **Comprobación de sandboxing**: En entornos Linux sin soporte para `unprivileged user namespaces`, se reporta `EnvironmentMisconfigured` indicando la acción correctiva del sistema.

## Seguridad y privacidad

- **Límites de confianza**: La validación de WebEngine forma parte de la línea de defensa de arranque de Vigilio Browser, garantizando que el motor de renderizado cuente con las mitigaciones de seguridad modernas (Chromium >= 128: mitigaciones V8, aislamiento estricto de procesos por sitio y extensiones bajo Manifest V3).
- **Protección de privacidad y datos locales**: Cumplimiento estricto de la decisión Q-001. No se realiza telemetría remota, ni sincronización en la nube, ni autenticación de cuentas Vigilio.
- **Sanitización de logs**: Los mensajes de diagnóstico generados por `formatReport` registran únicamente números de versión y rutas locales de binarios; no se registran datos personales ni URLs del usuario.
- **Seguridad de memoria**: Código C++20 fuertemente tipado, inmutabilidad por defecto (`const`, `constexpr`), ausencia de punteros crudos y gestión de recursos mediante RAII.
- **Prevención de procesos zombies**: La validación no invoca ni spawnea procesos Chromium en segundo plano; únicamente verifica rutas y metadatos estáticos locales.

## Matriz de pruebas

| Identificador | Criterio de prueba | Tipo | Comando de ejecución | Resultado esperado |
| :--- | :--- | :--- | :--- | :--- |
| `TEST-001` | Parsing de versiones Chromium válidas (e.g. "128.0.6613.119", "130.0.0.0") | Unitario | `ctest -R qt_dependency_validation_tests` / `webengine_version_validator_tests` | PASS: parsing exacto de 4 componentes, `milestone() == 128` |
| `TEST-002` | Parsing de cadenas inválidas, vacías, con letras o componentes faltantes | Unitario | `ctest -R webengine_version_validator_tests` | PASS: retorno determinista de `std::nullopt` |
| `TEST-003` | Comparación de versiones cuatripartitas mediante `operator<=>` | Unitario | `ctest -R webengine_version_validator_tests` | PASS: ordenamiento correcto por major, minor, build, patch |
| `TEST-004` | Verificación de baseline mínimo (`ChromiumVersion{128, 0, 0, 0}`) | Unitario | `ctest -R webengine_version_validator_tests` | PASS: `isCompatibleWith(baseline)` verdadero para milestone >= 128, falso para < 128 |
| `TEST-005` | Resolución de matriz de características para Chromium milestone >= 128 | Unitario | `ctest -R webengine_version_validator_tests` | PASS: presencia de `ManifestV3Extensions`, `PdfViewerInternal`, `ProcessSandboxing` |
| `TEST-006` | Detección de características faltantes para Chromium milestone < 128 | Unitario | `ctest -R webengine_version_validator_tests` | PASS: reporte identifica ausencia de `ManifestV3Extensions` |
| `TEST-007` | Verificación de requerimientos según perfiles de compatibilidad | Unitario | `ctest -R webengine_version_validator_tests` | PASS: `FullDesktopBrowser` exige MV3 y sandboxing; `MinimalCore` no exige WebEngine |
| `TEST-008` | Validación de ejecutabilidad del proceso asistente `QtWebEngineProcess` | Unitario | `ctest -R webengine_version_validator_tests` | PASS: ruta válida retorna compatible; ruta inexistente o vacía reporta `ProcessHelperMissing` |
| `TEST-009` | Evaluación de degradación suave en modo headless (`MinimalCore`) | Unitario | `ctest -R webengine_version_validator_tests` | PASS: estado `DegradedHeadless`, sin interrupción anormal de ejecución |
| `TEST-010` | Formateo estructurado de diagnósticos y reportes de estado | Unitario | `ctest -R webengine_version_validator_tests` | PASS: cadenas legibles, descriptivas y sin jerga técnica prohibida en UI |

## Pasos APPLY

Tras la aprobación humana de esta propuesta, la fase APPLY ejecutará deterministamente los siguientes pasos:
1. **Paso 1**: Crear el archivo de cabecera `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` con las entidades de dominio y la API pública del validador.
2. **Paso 2**: Crear el archivo de implementación `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp` con la lógica pura de parsing, tablas de características, verificación de binarios y evaluación de compatibilidad.
3. **Paso 3**: Modificar `apps/vigilio-browser/CMakeLists.txt` para agregar `src/core/WebEngineVersionValidator.cpp` al target `vigilio_core` y registrar el nuevo target de pruebas `vigilio_webengine_validator_tests` en CTest con las etiquetas `unit;fast`.
4. **Paso 4**: Modificar `apps/vigilio-browser/src/app.cpp` integrando la llamada a `WebEngineVersionValidator::validateStartup` en el punto de arranque de la aplicación.
5. **Paso 5**: Crear la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` cubriendo la matriz completa de casos.
6. **Paso 6**: Ejecutar la suite completa de calidad local: `bash apps/vigilio-browser/scripts/check.sh`, Clang-Format y CTest, verificando compilación limpia con `-Werror` y 100% de pruebas aprobadas.
7. **Paso 7**: Verificar la sincronización documental y la validación de referencias canónicas con Prompt Builder (`vigilio-prompt validate-references`).

## Definición de terminado

Se considerará terminado el cambio cuando:
1. Las entidades de dominio (`ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, `WebEngineCompatibilityReport`) y el servicio `WebEngineVersionValidator` estén implementados en C++20 estándar en `vigilio_core`.
2. `vigilio_core` compile sin advertencias bajo `-Werror` y sin incluir cabeceras de `Qt6::WebEngineWidgets` ni Chromium.
3. El punto de arranque en `app.cpp` ejecute la pre-validación de WebEngine con degradación controlada y emisión de diagnósticos claros en caso de fallo.
4. La suite de pruebas unitarias `vigilio_webengine_validator_tests` esté registrada en CTest y pase al 100%.
5. Las verificaciones automáticas de `check.sh`, Clang-Format y `validate-references` concluyan con exit code 0.
6. Se mantenga estricta conformidad con las políticas local-first (Q-001) y lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`).

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0013`: etapa domain que formalizó el modelo conceptual y las invariantes de versión.
  - Estándar C++20 y compilador compatible (GCC >= 11, Clang >= 14, MSVC >= 19.30).
  - Herramientas locales: CMake >= 3.24, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Variabilidad en la ruta de instalación del ejecutable asistente `QtWebEngineProcess` entre diferentes distribuciones Linux (e.g. `/usr/lib/qt6/libexec/`, `/usr/libexec/`, `/usr/lib/x86_64-linux-gnu/qt6/libexec/`).
    *Mitigación*: Implementar una búsqueda jerárquica que consulte `QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)` cuando Qt esté disponible, variable de entorno `QTWEBENGINEPROCESS_PATH`, y rutas estándar del sistema operativo con fallback seguro.
  - *Riesgo*: Entornos de integración continua o contenedores mínimos donde el proceso asistente no esté instalado.
    *Mitigación*: La verificación soporta el perfil `HeadlessInspection` y `MinimalCore` que transiciona limpiamente a `DegradedHeadless` sin abortos anormales de ejecución.

## Commit propuesto

- **Mensaje base**: `feat(foundations): implementation webengine-version-compatibility`
- **Ámbito**: Módulo `foundations`, capacidad `WebEngine version compatibility`, etapa `implementation`.
