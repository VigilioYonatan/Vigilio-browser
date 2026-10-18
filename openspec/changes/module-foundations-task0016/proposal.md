# Change Proposal — module-foundations-task0016

Pack: `module.foundations.task0016`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **WebEngine version compatibility** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en `module.foundations.task0013` la especificación del modelo de dominio, taxonomía de capacidades funcionales e invariantes de versión de Chromium (Chromium milestone >= 128 asociado al baseline Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`), implementado en `module.foundations.task0014` el validador desacoplado en C++20 (`vigilio::WebEngineVersionValidator`), la pre-validación de arranque en `app.cpp` y la suite base de pruebas unitarias en CTest (`webengine_version_validator_tests`), y proyectado en `module.foundations.task0015` los estados visuales y contratos de presentación en Qt Widgets (`BrowserWindow`), esta microtarea establece la arquitectura y suite de pruebas automatizadas deterministas, focalizadas y de alta resiliencia para certificar la compatibilidad del motor web ante casos normales, entradas inválidas, estrés concurrente, recuperación ante fallos y ausencia de regresiones.

El alcance concreto de este pack comprende:
- **Suite de pruebas unitarias y de estrés automatizadas (`apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`)**:
  - **Casos positivos y normales**:
    - Parsing y normalización exhaustiva de versiones de Chromium (`ChromiumVersion::parse`):
      - Formato estándar de 4 componentes numéricos (`128.0.6613.119`, `130.0.6723.58`).
      - Versiones base con componentes en cero (`128.0.0.0`, `132.0.0.0`).
      - Normalización de prefijos literales en minúscula y mayúscula (`v130.0.6723.58`, `V132.0.6834.15`).
      - Aceptación y recorte determinista de espacios en blanco perimetrales (`"   129.0.6668.70   \t"`).
      - Acceso exacto a hito principal (`milestone() == major`), componentes individuales (`major`, `minor`, `build`, `patch`) y serialización canónica (`toString()`).
    - Ordenamiento total, comparación estricta e invariantes de versión:
      - Operador de nave espacial (`operator<=>`) verificando reflexividad (`a == a`), antisimetría (`a < b` implica `b > a`) y transitividad (`a < b && b < c` implica `a < c`) entre versiones con diferencias en cualquiera de los cuatro campos numéricos.
      - Predicado de compatibilidad `isCompatibleWith`: una versión es compatible si su hito es mayor o igual al hito base y los campos secundarios satisfacen el umbral de baseline.
      - Invariante del baseline canónico de Chromium: `WebEngineVersionValidator::baselineChromiumVersion() == ChromiumVersion{128, 0, 0, 0}`.
    - Resolución de baseline de Chromium por versión de Qt (`minimumChromiumVersionForQt`):
      - Qt >= 6.10.0 (`6.10.0`, `6.10.2`, `6.11.0`, `7.0.0`) mapeado a Chromium 128.0.0.0.
      - Qt 6.9.x mapeado a Chromium 122.0.0.0.
      - Qt 6.8.x mapeado a Chromium 118.0.0.0.
      - Qt 6.7.x mapeado a Chromium 112.0.0.0.
    - Matriz de capacidades funcionales y perfiles de ejecución:
      - Hito >= 128: soporte completo de las 8 capacidades canónicas (`ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`).
      - Hito 122: soporte degradado (6 capacidades; carece de `ManifestV3Extensions` y `Http3Quic`).
      - Hito 118: soporte legacy (4 capacidades; carece de `ProcessSandboxing`).
      - Hito 0 / desconocido: lista vacía de capacidades.
      - Requerimientos por perfil: `FullDesktopBrowser` (exige `ManifestV3Extensions`, `PdfViewerInternal`, `ProcessSandboxing`, `LocalDevStorage`), `RestrictedSecureProfile` (exige `ProcessSandboxing`, `LocalDevStorage`), `HeadlessInspection` (exige `LocalDevStorage`), `MinimalCore` (cero capacidades requeridas).
    - Verificación y permisos del proceso ejecutable asistente (`isHelperExecutable`):
      - Creación de ejecutables sintéticos temporales en `/tmp` con RAII (`TempHelperFileGuard`).
      - Detección precisa de permisos de ejecución (`0755` -> true; `0644` -> false en sistemas POSIX).
      - Manejo determinista de symlinks válidos apuntando a ejecutables reales.
    - Validación integral de compatibilidad (`validateCompatibility`):
      - Perfil `FullDesktopBrowser` con Qt >= 6.10.0, Chromium >= 128, helper ejecutable y sandboxing activo -> `Compatible`.
      - Perfil `HeadlessInspection` con Qt >= 6.10.0, Chromium >= 128, sin exigir helper ejecutable ni aceleración gráfica -> `Compatible`.
      - Perfil `MinimalCore` con o sin dependencias disponibles -> `DegradedHeadless`.
  - **Casos negativos y rechazo de entradas inválidas**:
    - Parsing de cadenas malformadas en `ChromiumVersion::parse`:
      - Cadenas vacías (`""`), compuestas exclusivamente por espacios en blanco (`"   "`), o caracteres de control (`"\t\n"`).
      - Cadenas truncadas con número insuficiente de campos (`"128"`, `"128.0"`, `"128.0.6613"`).
      - Cadenas con exceso de campos (`"128.0.6613.119.1"`).
      - Cadenas con texto extra o sufijos de pre-lanzamiento (`"128.0.6613.119extra"`, `"128.0.6613.119-beta"`).
      - Caracteres alfanuméricos en posiciones numéricas (`"abc"`, `"128.x.6613.119"`, `"128.0.y.119"`, `"128.0.6613.z"`).
      - Puntuación errónea o puntos duplicados (`".128.0.6613.119"`, `"128..6613.119"`, `"128.0.6613."`).
      - Números negativos (`"-128.0.6613.119"`).
      - Protección contra desbordamiento entero en parsing de componentes numéricos grandes (`"99999999999999999999.0.0.0"`).
    - Taxonomía completa de estados de incompatibilidad:
      - Versión obsoleta o ausente de Qt: `IncompatibleQtVersion` (Qt 6.9.3 o `std::nullopt`).
      - Versión obsoleta o ausente de Chromium: `IncompatibleChromiumVersion` (Chromium 122 o `std::nullopt`).
      - Proceso asistente ausente o no ejecutable: `ProcessHelperMissing` (ruta inexistente, directorio, archivo regular sin flag de ejecución `0644`, o ruta vacía en perfil desktop).
      - Entorno mal configurado o sin sandboxing: `EnvironmentMisconfigured` (sandboxing reportado como falso en perfil `FullDesktopBrowser`).
  - **Pruebas de concurrencia multihilo, latencia y thread-safety**:
    - Validación masiva concurrente con 16 hilos simultáneos y 1000 iteraciones por hilo ejecutando parsing, validación de compatibilidad y formateo de reportes en paralelo.
    - Confirmación de pureza funcional: cero datos mutables globales, cero variables estáticas no sincronizadas, ausencia de condiciones de carrera y compatibilidad total con ThreadSanitizer (TSan).
    - Presupuesto de latencia y tiempo de CPU: ejecución unitaria de validación < 1 ms por llamada.
  - **Recuperación, degradación y resiliencia**:
    - Prueba del protocolo de pre-validación de arranque (`validateStartup`):
      - Modo interactivo desktop con parámetros válidos -> `Compatible`.
      - Modo interactivo desktop con helper inexistente -> `ProcessHelperMissing` con mensaje de mitigación.
      - Modo headless forzado (`isHeadless = true`) -> degradación limpia a `DegradedHeadless` sin excepciones ni accesos ilegales de memoria.
    - Manejo determinista de excepciones y punteros nulos: nunca provocar `SIGSEGV`, `SIGABRT` ni cierres anómalos.
  - **Formateo diagnóstico y observabilidad estructurada**:
    - Verificación biunívoca de nombres de capacidades (`featureName`), perfiles (`profileName`) y estados (`formatStatus`).
    - Verificación de formateo estructurado de reportes (`formatReport`): presencia garantizada del estado, versiones detectadas, ruta del asistente, capacidades faltantes y acción de mitigación recomendada.
- **Aislamiento de recursos y política zero-zombie**:
  - Las pruebas unitarias no inician procesos Chromium reales (`QtWebEngineProcess`) ni requieren display server (X11 / Wayland), ejecutándose 100% en modo headless sin dependencias de red ni procesos huérfanos.
  - Gestión determinista de archivos temporales mediante RAII (`TempHelperFileGuard`), garantizando que ningún archivo temporal permanezca tras la ejecución de las pruebas.
- **Integración y gobernanza en CTest (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Mantenimiento del target de prueba ejecutable `vigilio_webengine_validator_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`.
  - Registro de CTest como `webengine_version_validator_tests`, etiquetado como `unit;fast`, con timeout de 15 segundos y tiempo real de ejecución < 50 milisegundos.

## Out of scope

- Implementación de extensiones web bajo la API Manifest V3 o emulación de APIs de extensiones (reservado para el módulo `extensions`, microtareas 0705 a 0730).
- Modificación directa de la interfaz gráfica de usuario en `BrowserWindow` (diseñada en `module.foundations.task0015` y perteneciente a la etapa `ui`).
- Inclusión, configuración y compilación de recursos binarios vectoriales QRC (reservado a la capacidad `resource-qrc-inclusion`, microtareas 0017 a 0020).
- Pruebas de conexión de red externa, validaciones remotas en la nube o telemetría (prohibidas por la constitución local-first y la decisión canónica Q-001).
- Modificación directa de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida exclusivamente a `openspec/changes/module-foundations-task0016/**`).

## Contexto y evidencia

Se inspeccionó de forma rigurosa la base de código actual de `apps/vigilio-browser`, los contratos canónicos y las herramientas del entorno local:
1. **Infraestructura de validación existente (`src/core/WebEngineVersionValidator.h` y `.cpp`)**:
   - Componente central en la biblioteca estática `vigilio_core`.
   - Provee las entidades inmutables `ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus` y `WebEngineCompatibilityReport`.
   - Provee métodos desacoplados puros: `baselineChromiumVersion()`, `minimumChromiumVersionForQt()`, `supportedFeaturesForMilestone()`, `requiredFeaturesForProfile()`, `isHelperExecutable()`, `validateCompatibility()`, `validateStartup()` y formateadores de diagnósticos.
   - Pasa al 100% en CTest mediante la suite unitaria `webengine_version_validator_tests` (exit code 0).
2. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Integra la verificación de Qt en runtime (`QtDependencyValidator::validateRuntimeEnvironment`).
   - Integra la pre-validación de WebEngine (`WebEngineVersionValidator::validateStartup(report.foundVersion, "128.0.6613.119", "", false)`).
   - Maneja fallos de compatibilidad emitiendo diagnósticos claros y terminando con código 1 antes de instanciar `QApplication`.
3. **Capa visual y de estados (`src/ui/BrowserWindow.h` y `src/ui/BrowserWindow.cpp`)**:
   - Construida con Qt Widgets (`QMainWindow`, `QTabWidget`, `QLineEdit`, `QToolBar`).
   - Implementa estados visuales `empty`, `loading`, `success`, `error`, y pestañas diferidas `deferred` (`restoreSession`, `activateDeferredTab`).
   - Conmutador de «Interfaz más fácil de usar» con escalado de tipografías a 14pt y controles con alturas mínimas de 48px y 55px.
4. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Compila bajo C++20 con `-Werror` mediante `vigilio_compiler_warnings`.
   - Target estático `vigilio_core` desacoplado de dependencias gráficas de WebEngine.
   - Targets existentes de CTest: `core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`, `webengine_version_validator_tests`.
5. **Verificación de herramientas locales de desarrollo y pipelines**:
   - Compilador: GNU GCC 15.2.0 (`-std=c++20`).
   - CMake: versión 4.2.3.
   - Ninja: versión 1.13.2.
   - SQLite3: versión 3.46.1.
   - Pipeline de verificación local `bash apps/vigilio-browser/scripts/check.sh`: compilación limpia y 5/5 pruebas unitarias y de integración pasando al 100% en 0.05 segundos, exit code 0.
   - Verificación de formateo con Clang-Format: 100% de archivos en `src/` y `tests/` cumplen el estándar sin divergencias, exit code 0.
   - Prompt Builder CLI (`/config/Desktop/auto/prompt-builder/dist/cli.js`):
     - `validate-references --project-root .`: 16/16 referencias canónicas validadas con estado `valid: true`, exit code 0.
     - `verify-lock --lock-file generated/prompts/full-project/0046-module.foundations.task0016.propose.lock.json --project-root .`: 14/14 comprobaciones exitosas con estado `valid: true`, exit code 0.
     - `validate-decisions`: decisiones Q-001 a Q-006 y Q-008 resueltas; Q-007 permanece OPEN afectando exclusivamente el hito de empaquetado final `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0016`.
- **Capacidad**: `WebEngine version compatibility`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0016/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0016/proposal.md`
  - `openspec/changes/module-foundations-task0016/design.md`
  - `openspec/changes/module-foundations-task0016/tasks.md`
  - `openspec/changes/module-foundations-task0016/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0016/**`. Prohibido modificar `apps/**` o `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño integral de pruebas automatizadas deterministas para `WebEngine version compatibility` se articula en torno a la suite unitaria desacoplada `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` y su target CTest `webengine_version_validator_tests`:
1. **Casos de prueba exhaustivos en `WebEngineVersionValidatorTests.cpp`**:
   - `test_chromium_version_parsing_positive()`:
     - Parsing de versiones canónicas cuatripartitas (`128.0.6613.119`).
     - Versiones base con ceros (`128.0.0.0`).
     - Prefijos `'v'` y `'V'` (`v130.0.6723.58`, `V132.0.6834.15`).
     - Recorte de espacios y tabuladores perimetrales (`"   129.0.6668.70   \t"`).
     - Delegación a través de método estático `WebEngineVersionValidator::parseChromiumVersion`.
   - `test_chromium_version_parsing_negative()`:
     - Cadenas vacías, cadenas con solo espacios, saltos de línea.
     - Componentes numéricos faltantes (`"128"`, `"128.0"`, `"128.0.6613"`).
     - Componentes numéricos en exceso (`"128.0.6613.119.1"`).
     - Sufijos textuales y de pre-lanzamiento (`"128.0.6613.119extra"`, `"128.0.6613.119-beta"`).
     - Caracteres no numéricos intermedios o iniciales (`"abc"`, `"128.x.6613.119"`, `"128.0.y.119"`, `"128.0.6613.z"`).
     - Puntos erróneos (`".128.0.6613.119"`, `"128..6613.119"`, `"128.0.6613."`).
     - Signos negativos (`"-128.0.6613.119"`).
     - Desbordamiento numérico entero en componentes de versión.
   - `test_chromium_version_comparison_and_compatibility()`:
     - Operador de igualdad (`==`) y desigualdad (`!=`).
     - Operador de ordenamiento total (`<=>`, `<`, `<=`, `>`, `>=`) en cada posición (major, minor, build, patch).
     - Predicado `isCompatibleWith` validando monotonicidad frente al baseline.
     - Invariante de `baselineChromiumVersion()`.
   - `test_minimum_chromium_version_for_qt()`:
     - Mapeo de Qt 6.10.x, 6.11.x, 7.x a Chromium 128.0.0.0.
     - Mapeo de versiones anteriores: Qt 6.9 -> 122, Qt 6.8 -> 118, Qt 6.7 -> 112.
   - `test_feature_matrix_and_profiles()`:
     - Presencia de las 8 capacidades en hito 128.
     - Ausencia de `ManifestV3Extensions` y `Http3Quic` en hito 122.
     - Ausencia de `ProcessSandboxing` en hito 118.
     - Lista vacía para hito 0.
     - Validación de perfiles de requerimientos: `FullDesktopBrowser`, `RestrictedSecureProfile`, `HeadlessInspection`, `MinimalCore`.
   - `test_helper_executable_check()`:
     - Ruta vacía -> false.
     - Ruta inexistente -> false.
     - Directorio existente (`/tmp`) -> false.
     - Archivo sintético sin permisos de ejecución (`0644`) -> false.
     - Archivo sintético con permisos de ejecución (`0755`) -> true.
     - Symlinks válidos hacia ejecutables sintéticos -> true.
     - Limpieza automática de fixtures con RAII (`TempHelperFileGuard`).
   - `test_compatibility_validation()`:
     - Perfil `MinimalCore` -> `DegradedHeadless`.
     - Qt menor a 6.10 o nulo -> `IncompatibleQtVersion`.
     - Chromium menor a 128 o nulo -> `IncompatibleChromiumVersion`.
     - Proceso asistente faltante o no ejecutable -> `ProcessHelperMissing`.
     - Entorno sin sandboxing en modo escritorio -> `EnvironmentMisconfigured`.
     - Perfil `FullDesktopBrowser` con dependencias completas -> `Compatible`.
     - Perfil `HeadlessInspection` sin asistente -> `Compatible`.
   - `test_startup_validation()`:
     - Validación de pre-arranque en modo headless (`isHeadless = true`) -> `DegradedHeadless`.
     - Validación de pre-arranque en modo desktop con helper válido -> `Compatible`.
     - Validación de pre-arranque en modo desktop con helper ausente -> `ProcessHelperMissing`.
   - `test_formatting_and_introspection()`:
     - Nombres legibles de capacidades (`featureName`).
     - Nombres legibles de perfiles (`profileName`).
     - Nombres legibles de estados (`formatStatus`).
     - Reportes estructurados de diagnósticos (`formatReport`) conteniendo estado, versiones detectadas, helper, capacidades ausentes y remediación.
   - `test_concurrency_and_thread_safety()`:
     - 16 hilos simultáneos y 1000 iteraciones por hilo ejecutando parsing, evaluación de perfiles y formateo en paralelo.
     - Aserción de ausencia de fallos concurrentes mediante `std::atomic<bool>`.
2. **Gobernanza de compilación y CTest**:
   - Enlace exclusivo de `vigilio_webengine_validator_tests` con `vigilio_core` y `vigilio_compiler_warnings`.
   - Ejecución sin interfaz gráfica ni dependencias de Chromium en tiempo de ejecución.
   - Integración continua en el script `apps/vigilio-browser/scripts/check.sh`.

## Archivos afectados

Durante esta fase PROPOSE, la escritura está rigurosamente limitada a la raíz OpenSpec del cambio:
- `openspec/changes/module-foundations-task0016/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0016/proposal.md`
- `openspec/changes/module-foundations-task0016/design.md`
- `openspec/changes/module-foundations-task0016/tasks.md`
- `openspec/changes/module-foundations-task0016/execution-memory.md`

En la fase futura APPLY (tras aprobación humana), los archivos de código afectados serán:
- `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`: ampliación y endurecimiento de la suite de pruebas unitarias, cobertura de casos límite de parsing, desbordamientos numéricos, symlinks en helpers sintéticos, matrices de incompatibilidad combinadas y concurrencia multihilo.
- `apps/vigilio-browser/CMakeLists.txt`: verificación y preservación del target `vigilio_webengine_validator_tests` y del registro CTest `webengine_version_validator_tests`.

## Contrato UX/UI

- **Sin cambio visual directo**: Esta microtarea pertenece a la etapa `tests` y opera exclusivamente sobre la infraestructura de pruebas automatizadas y validación en espacio de usuario (`vigilio_core` y CTest).
- **Preservación estricta de contratos visuales existentes**:
  - No modifica la interfaz gráfica de `BrowserWindow` ni altera los widgets de Qt Widgets.
  - No introduce mensajes visibles ni diálogos emergentes en la interfaz de usuario.
  - Cumple estrictamente con `docs/UX-COPY-PUBLICO.md`: ningún término de ingeniería interno («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «renderProcessTerminated», «ABI mismatch», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec») se expone en cadenas visuales de usuario.
  - La referencia a la «Interfaz más fácil de usar» se mantiene conforme a las directrices de `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`, y los prototipos visuales canónicos en `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`.

## Reglas y fallos

1. **Determinismo y reproducibilidad**: Toda prueba en la suite debe producir resultados idénticos en cualquier orden de ejecución y en cualquier plataforma soportada (Linux x64, Windows x64, macOS arm64/x64). Prohibidos sleeps fijos, temporizadores arbitrarios o dependencias de reloj de pared.
2. **Aislamiento de procesos y memoria (Zero-Zombie)**: Ninguna prueba unitaria iniciará procesos Chromium reales (`QtWebEngineProcess`). Los helpers ejecutables sintéticos se crean como scripts mínimos temporales y se eliminan inmediatamente mediante RAII (`TempHelperFileGuard`).
3. **Manejo estricto de fallos e incompatibilidades**:
   - `IncompatibleQtVersion`: emitida cuando la versión de Qt es inferior a 6.10.0 o es nula en un perfil que requiere Qt.
   - `IncompatibleChromiumVersion`: emitida cuando el hito de Chromium es inferior a 128 o es nula.
   - `ProcessHelperMissing`: emitida cuando el binario `QtWebEngineProcess` no existe, es un directorio, o carece de permisos de ejecución `0755` en perfiles desktop.
   - `EnvironmentMisconfigured`: emitida cuando el entorno no admite aislamiento por procesos o espacios de nombres requeridos por Chromium.
   - `DegradedHeadless`: estado seguro de contingencia en perfil `MinimalCore` o entornos sin aceleración/display.
4. **Resiliencia ante entradas extremas**:
   - Cadenas vacías, caracteres alfanuméricos, desbordamientos de enteros de 64 bits en componentes de versión deben resolverse retornando `std::nullopt` sin arrojar excepciones no controladas ni provocar fallos de segmentación.
5. **Garantía local-first (Decisión Q-001)**: Prohibida toda petición de red, resolución de nombres remotos o telemetría en la suite de pruebas.

## Seguridad y privacidad

- **Límite de confianza y sandboxing**: La validación exige `ProcessSandboxing` para el perfil `FullDesktopBrowser`, garantizando que el motor web opere bajo aislamiento multiproceso con namespaces y seccomp-bpf en Linux.
- **Privacidad local estricta**:
  - No se registran URLs completas ni datos de navegación del usuario en diagnósticos o logs.
  - Los reportes emitidos por `formatReport` contienen exclusivamente metadatos técnicos mínimos (versión Qt, versión Chromium, ruta del binario asistente, capacidades soportadas y ausentes).
  - Cero persistencia en SQLite durante la ejecución de las pruebas unitarias de WebEngine.
- **Limpieza segura de recursos**: El uso de `TempHelperFileGuard` garantiza que no queden archivos sintéticos residuales en directorios temporales compartidos (`/tmp`).

## Matriz de pruebas

| Identificador | Criterio de validación | Fixture / Entrada | Comando / Ejecutable | Resultado esperado | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Parsing versiones válidas y canónicas | `"128.0.6613.119"`, `"128.0.0.0"`, `"v130.0.6723.58"`, `"  129.0.6668.70 \t"` | `ctest -R webengine_version_validator_tests` | `ChromiumVersion` válida con hito, componentes y serialización canónica exacta | APPLY |
| **TEST-002** | Rechazo de versiones inválidas y malformadas | `""`, `"   "`, `"128"`, `"128.0"`, `"128.0.6613"`, `"128.0.6613.119.1"`, `"128.x"`, `"-128"`, overflow | `ctest -R webengine_version_validator_tests` | `std::nullopt` en 100% de entradas malformadas | APPLY |
| **TEST-003** | Ordenamiento total y comparación de versiones | Comparación exhaustiva entre hitos 127, 128, 129 y variaciones en build/patch | `ctest -R webengine_version_validator_tests` | `operator<=>` satisface reflexividad, antisimetría y transitividad | APPLY |
| **TEST-004** | Compatibilidad semántica con baseline | Comparación frente a baseline 128.0.0.0 (`v128_base.isCompatibleWith`) | `ctest -R webengine_version_validator_tests` | Hito >= 128 retorna true; hito < 128 retorna false | APPLY |
| **TEST-005** | Mapeo de baseline Chromium por versión de Qt | Qt 6.10.x, 6.11.x, 7.x vs Qt 6.9, 6.8, 6.7 | `ctest -R webengine_version_validator_tests` | Qt >= 6.10 -> Cr 128; Qt 6.9 -> Cr 122; Qt 6.8 -> Cr 118 | APPLY |
| **TEST-006** | Matriz de capacidades funcionales por hito | Hito 128 (8 caps), Hito 122 (6 caps), Hito 118 (4 caps), Hito 0 (0 caps) | `ctest -R webengine_version_validator_tests` | Coincidencia exacta de capacidades por hito sin falsos positivos | APPLY |
| **TEST-007** | Requerimientos de perfiles de compatibilidad | Perfiles `FullDesktopBrowser`, `RestrictedSecureProfile`, `HeadlessInspection`, `MinimalCore` | `ctest -R webengine_version_validator_tests` | Mapeo determinista de capacidades obligatorias por perfil | APPLY |
| **TEST-008** | Verificación de helper ejecutable y permisos | Fixtures sintéticos en `/tmp`: vacío, directorio, archivo `0644`, archivo `0755`, symlink | `ctest -R webengine_version_validator_tests` | `isHelperExecutable` valida permisos `X_OK` y tipo regular | APPLY |
| **TEST-009** | Evaluación de compatibilidad integral | Combinaciones de Qt, Chromium, helper y flag de sandboxing | `ctest -R webengine_version_validator_tests` | Estados `Compatible`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `EnvironmentMisconfigured` | APPLY |
| **TEST-010** | Pre-validación de arranque y degradación | Invocación `validateStartup` con modo interactivo y headless | `ctest -R webengine_version_validator_tests` | `Compatible` en desktop válido; `DegradedHeadless` en headless; `ProcessHelperMissing` ante helper ausente | APPLY |
| **TEST-011** | Introspección y formateo diagnóstico | `featureName`, `profileName`, `formatStatus`, `formatReport` | `ctest -R webengine_version_validator_tests` | Cadenas no vacías con diagnósticos precisos y acciones de mitigación | APPLY |
| **TEST-012** | Concurrencia multihilo y estrés libre de data races | 16 hilos simultáneos, 1000 iteraciones por hilo ejecutando parsing y validación | `ctest -R webengine_version_validator_tests` | Cero fallos, thread-safety garantizado sin bloqueos | APPLY |
| **TEST-013** | Pruebas de integración visual de estados WebEngine en GUI | Instanciación de `BrowserWindow` y verificación de estados `empty`, `loading`, `error`, `deferred` | `ctest -R window_ui_tests` | Conmutación de estados y visualización de contenedor de error nativo | LATER |
| **TEST-014** | Smoke E2E pesado de navegación real con Chromium | Arranque de proceso WebEngine real cargando página local | `ctest -R browser_e2e_smoke` | Ejecución serial (`RUN_SERIAL TRUE`), `RESOURCE_LOCK browser_e2e`, bajo demanda (`VIGILIO_ENABLE_E2E=ON`) | LATER |
| **TEST-015** | Compatibilidad de red o sincronización en la nube | N/A (prohibido por principios arquitectónicos) | N/A | N/A (local-first constitucional) | NOT_APPLICABLE |

## Pasos APPLY

1. **Auditoría e inspección previa del arnés de pruebas (`tests/unit/WebEngineVersionValidatorTests.cpp`)**:
   - Verificar la estructura actual del archivo de pruebas y asegurar compatibilidad estricta con C++20 y `-Werror`.
2. **Endurecimiento de casos límite de parsing y desbordamiento en `WebEngineVersionValidatorTests.cpp`**:
   - Añadir pruebas de parsing para desbordamientos enteros grandes, ceros a la izquierda y caracteres de control perimetrales.
3. **Ampliación de pruebas de permisos y symlinks en `test_helper_executable_check()`**:
   - Incorporar comprobaciones de enlaces simbólicos que apuntan a ejecutables sintéticos y validar el comportamiento ante symlinks rotos.
4. **Endurecimiento de combinaciones de estados de incompatibilidad en `test_compatibility_validation()`**:
   - Cubrir casos donde múltiples condiciones anómalas coexisten (por ejemplo, Qt incompatible y helper faltante simultáneamente), verificando la precedencia determinista del reporte de errores.
5. **Ampliación del estrés multihilo en `test_concurrency_and_thread_safety()`**:
   - Incrementar la concurrencia a 16 hilos y 1000 iteraciones por hilo, midiendo tiempos de respuesta para asegurar que la latencia media sea inferior a 1 ms sin contención de locks.
6. **Verificación de la integración en CMakeLists.txt y CTest**:
   - Confirmar que el target `vigilio_webengine_validator_tests` y el test `webengine_version_validator_tests` cuentan con etiquetas `LABELS "unit;fast"` y timeout de 15 segundos.
7. **Ejecución y validación de calidad continua**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` comprobando que las 5 pruebas de CTest aprueban al 100% en tiempo récord (< 0.1 s).
   - Validar formato de código con `clang-format --dry-run --Werror`.
8. **Validación formal de referencias OpenSpec con Prompt Builder**:
   - Ejecutar `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` y certificar el estado `valid: true`.

## Definición de terminado

- [ ] Propuesta OpenSpec formalizada en `openspec/changes/module-foundations-task0016/{proposal.md,design.md,tasks.md,execution-memory.md}` con estado `PROPOSED` y referencias canónicas validadas.
- [ ] La suite de pruebas automatizadas `webengine_version_validator_tests` cubre exhaustivamente casos positivos, negativos, límites de parsing, matrices de capacidades, permisos de helpers, recuperación/degradación y concurrencia multihilo.
- [ ] 100% de las pruebas unitarias y de integración pasan con éxito en CTest (`apps/vigilio-browser/scripts/check.sh`) con exit code 0.
- [ ] Formateo de código verificado con Clang-Format sin advertencias ni diferencias.
- [ ] Aislamiento garantizado: cero llamadas de red externa, cero procesos zombis de Chromium iniciados, cero archivos temporales residuales.
- [ ] Respeto absoluto a la decisión canónica Q-001 (local-first) y ausencia de terminología técnica interna en interfaces públicas según `docs/UX-COPY-PUBLICO.md`.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0015`: Diseñó la capa de presentación, estados visuales y manejo accesible de errores en Qt Widgets para WebEngine.
  - Compilador C++20 (GCC 15+, Clang 18+, MSVC 2022+), CMake >= 3.24, Ninja >= 1.12.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Inestabilidad o flakiness en pruebas multihilo por contención de recursos del sistema anfitrión.
    *Mitigación*: Métodos de validación completamente puros y libres de estado mutable global; cada hilo opera con sus propias instancias inmutables.
  - *Riesgo*: Archivos temporales residuales en `/tmp` en caso de aserción fallida o interrupción prematura de la prueba.
    *Mitigación*: Uso estricto de RAII mediante `TempHelperFileGuard` en el constructor y destructor para asegurar la eliminación determinista de los archivos sintéticos.
  - *Riesgo*: Falsa sensación de cobertura si las pruebas ejecutan procesos Chromium reales que dependan del display server.
    *Mitigación*: Desacoplamiento arquitectónico absoluto en `vigilio_core`: las pruebas unitarias validan lógica, contratos y binarios sin instanciar componentes WebEngine ni requerir entorno gráfico.

## Commit propuesto

`test(foundations): tests webengine-version-compatibility`
