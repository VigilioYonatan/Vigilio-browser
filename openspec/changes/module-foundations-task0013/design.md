# Design — module-foundations-task0013

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Configuración de CMake y soporte de WebEngine (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Actualmente, el archivo `CMakeLists.txt` gobierna la inclusión del motor WebEngine en las líneas 151 a 205:
     ```cmake
     option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
     option(VIGILIO_REQUIRE_QT "Require Qt 6.10+ dependencies strictly (fail build if missing)" OFF)

     if(VIGILIO_BUILD_GUI)
       set(QT_MINIMUM_VERSION "6.10.0")
       find_package(Qt6 ${QT_MINIMUM_VERSION} QUIET COMPONENTS Core Widgets WebEngineWidgets)
       if(Qt6_FOUND)
         ...
       else()
         ...
       endif()
     endif()
     ```
   - Aunque la directiva requiere `Qt6` versión `>= 6.10.0`, únicamente valida la presencia de los paquetes de CMake de Qt a nivel macro (`Qt6Core`, `Qt6Widgets`, `Qt6WebEngineWidgets`).
   - No existe ninguna comprobación ni introspección sobre la versión interna del motor Chromium embebido en Qt WebEngine.
   - No se valida si las capacidades críticas del motor (soporte de extensiones MV3, aislamiento de procesos, soporte PDF) están presentes y habilitadas en el binario compilado.
2. **Punto de arranque de la aplicación (`apps/vigilio-browser/src/app.cpp`)**:
   - `app.cpp` invoca la pre-validación de entorno de Qt:
     ```cpp
     const auto report = vigilio::QtDependencyValidator::validateRuntimeEnvironment(
         qVersion(), QT_VERSION_STR, display, wayland);
     if (report.outcome != vigilio::QtValidationOutcome::Satisfied) {
         std::cerr << "[Vigilio Startup Error] " << report.diagnosticMessage << "\n";
         return 1;
     }
     ```
   - Sin embargo, no existe ninguna verificación previa sobre la compatibilidad de WebEngine o la disponibilidad del ejecutable asistente `QtWebEngineProcess`:
     - Si la instalación del sistema tiene corrupto o ausente el binario asistente `QtWebEngineProcess` (problema común en distribuciones Linux o empaquetados incompletos), la ventana principal intentará instanciar `QWebEngineView`, resultando en un bloqueo silencioso, una página en blanco permanente o un crash abrupto del motor Chromium sin mensajes de diagnóstico claros para el usuario ni para los registros de depuración locales.
     - No se verifica si el sistema anfitrión tiene habilitadas las restricciones de espacio de nombres de usuario (`unprivileged user namespaces`) requeridas por el sandbox de Chromium en Linux contemporáneo.
3. **Ausencia de modelo de dominio para Chromium y WebEngine**:
   - La biblioteca `vigilio_core` contiene `QtDependencyValidator.h` con `SemanticVersion`, pero está limitada a la semántica tripartita de Qt (`major.minor.patch`).
   - No existe un tipo de datos en C++20 para modelar las versiones cuatripartitas de Chromium (`major.minor.build.patch`), ni una taxonomía tipada de capacidades del motor web (`WebEngineFeature`), perfiles de ejecución (`WebEngineCompatibilityProfile`) o resultados estructurados de validación de WebEngine (`WebEngineCompatibilityReport`).

### Estado objetivo (TARGET)

1. **Modelo de dominio fuertemente tipado en C++20 (`vigilio_core`)**:
   - Introducción de `ChromiumVersion` para modelar de forma inmutable la versión cuatripartita de Chromium (`major.minor.build.patch`), con parsing robusto, comparación total mediante `<=>`, formateo canónico y validación de compatibilidad con el baseline mínimo establecido (Chromium milestone >= 128).
   - Taxonomía completa de capacidades (`WebEngineFeature`): `ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`.
   - Perfiles de compatibilidad declarativos (`WebEngineCompatibilityProfile`): `FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`.
   - Taxonomía exhaustiva de estados de validación (`WebEngineCompatibilityStatus`): `Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `FeatureUnsupported`, `EnvironmentMisconfigured`.
   - Objeto inmutable de reporte (`WebEngineCompatibilityReport`) que consolida diagnósticos contextuales y recomendaciones de solución.
2. **Servicio puro desacoplado (`WebEngineVersionValidator`)**:
   - Servicio estático en `vigilio_core` independiente de bibliotecas de interfaz gráfica y libre de cabeceras de Chromium.
   - Provee funciones puras de evaluación para relacionar versiones de Qt con hitos de Chromium, validar perfiles de requerimientos y verificar la salud de los procesos asistentes antes de instanciar componentes de navegación web.
3. **Desacoplamiento arquitectónico y portabilidad**:
   - El núcleo `vigilio_core` y `vigilio_storage` compilan limpiamente en modo headless en Linux, Windows y macOS sin requerir librerías gráficas ni Chromium instalado.
   - La validación es 100% determinista, no realiza llamadas a red ni telemetría, y se ejecuta en microsegundos (< 1 ms).

### Brechas identificadas (GAPS)

1. **GAP-001 (Falta de tipo de datos para versiones de Chromium)**: `SemanticVersion` solo soporta 3 componentes (`major.minor.patch`), mientras que Chromium utiliza 4 componentes (`major.minor.build.patch`).
2. **GAP-002 (Ausencia de taxonomía de capacidades de WebEngine)**: No existen tipos enumerados para describir qué características del motor web son requeridas o soportadas en el navegador.
3. **GAP-003 (Falta de reglas e invariantes de baseline de Chromium)**: No están formalizadas en código las reglas que establecen que Qt >= 6.10.0 exige Chromium milestone >= 128 para soporte de MV3 y seguridad.
4. **GAP-004 (Falta de validación de proceso asistente)**: El arranque en `app.cpp` no valida la presencia ni ejecutabilidad de `QtWebEngineProcess`, dejando abierta la posibilidad de fallos silenciosos.
5. **GAP-005 (Falta de suite de pruebas de especificación de WebEngine)**: No existen pruebas unitarias en `apps/vigilio-browser/tests/` que verifiquen las reglas de compatibilidad de Chromium y degradación a headless.

## Compatibility, migration and rollback

- **Compatibilidad con estándares y plataformas**:
  - Estándar de lenguaje: C++20 estricto (`set(CMAKE_CXX_STANDARD 20)`).
  - Compiladores soportados: GNU GCC >= 11, Clang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Sistemas operativos objetivo: Linux (x86_64, Wayland y X11), Windows 10/11 (x64), macOS (Apple Silicon y x86_64).
  - Baseline de versión: Qt >= 6.10.0 y Chromium milestone >= 128.
- **Estrategia de migración progresiva**:
  - Etapa actual (`domain`): formalización en la propuesta OpenSpec del modelo conceptual, entidades de valor, reglas e invariantes.
  - Etapas subsiguientes (`implementation`, `ui`, `tests`): adición de cabeceras en `src/core/`, integración en `app.cpp`, y registro de suites unitarias en CTest. La arquitectura preserva plena retrocompatibilidad con compilaciones headless (`VIGILIO_BUILD_GUI=OFF`).
- **Estrategia de reversión (rollback)**:
  - En la fase PROPOSE, la reversión se efectúa simplemente eliminando o descartando los archivos generados bajo `openspec/changes/module-foundations-task0013/`.
  - La base de código y la configuración existente no experimentan mutaciones irreversibles.

## Contexto y evidencia

Se inspeccionó detalladamente el estado actual del repositorio:
1. `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`: implementan validación tripartita de Qt.
2. `apps/vigilio-browser/src/app.cpp`: contiene el punto de entrada principal con validación de entorno de display.
3. `apps/vigilio-browser/CMakeLists.txt`: define targets `vigilio_core`, `vigilio_storage` y ejecutables de prueba en CTest.
4. `apps/vigilio-browser/scripts/check.sh`: compila limpiamente con Ninja bajo `-Werror` y supera 4/4 pruebas automatizadas en 0.18 segundos.
5. Verificación de directrices canónicas: `validate-references` superó 13 comprobaciones (`valid: true`), `verify-lock` validó el lockfile `0037-module.foundations.task0013.propose.lock.json` al 100%, y `validate-decisions` confirmó la inexistencia de decisiones bloqueantes para foundations.

## Diseño técnico específico: WebEngine Version Compatibility Domain Model

### 1. Entidad de valor `ChromiumVersion` (C++20)

```cpp
namespace vigilio {

struct ChromiumVersion {
    uint32_t major{0};  // Milestone (e.g. 128)
    uint32_t minor{0};  // Sub-milestone (usually 0)
    uint32_t build{0};  // Build number (e.g. 6613)
    uint32_t patch{0};  // Patch level (e.g. 120)

    auto operator<=>(const ChromiumVersion&) const = default;

    [[nodiscard]] bool isCompatibleWith(const ChromiumVersion& requiredMin) const noexcept {
        // En Chromium, la compatibilidad arquitectónica se rige por el milestone mayor
        if (major < requiredMin.major) {
            return false;
        }
        return *this >= requiredMin;
    }

    [[nodiscard]] std::string toString() const;
    static std::optional<ChromiumVersion> parse(std::string_view versionStr);
};

} // namespace vigilio
```

- **Invariantes de parsing**:
  - Cadenas canónicas: `"128.0.6613.120"` -> `major=128, minor=0, build=6613, patch=120`.
  - Normalización de 3 componentes: `"128.0.6613"` -> `patch=0`.
  - Normalización de 2 componentes: `"128.0"` -> `build=0, patch=0`.
  - Tolerancia a prefijos: `"v128.0.6613.120"` o `"Chromium/128.0.6613.120"`.
  - Rechazo estricto (`std::nullopt`): cadenas vacías, caracteres alfabéticos intermedios, puntos duplicados (`"128..0"`), números que desborden `uint32_t`.

### 2. Taxonomía de capacidades `WebEngineFeature`

```cpp
namespace vigilio {

enum class WebEngineFeature {
    ManifestV3Extensions,  // Extensiones web MV3 con Service Workers y declarativas
    PdfViewerInternal,     // Visor seguro de PDF integrado
    WebRTC,                // Comunicaciones de audio/video en tiempo real
    DevToolsFrontend,      // Soporte para protocolo de inspección remota
    ProcessSandboxing,     // Aislamiento multiproceso de renderers
    HardwareAcceleration,  // Aceleración GPU y soporte WebGL
    LocalDevStorage,       // Almacenamiento local SQLite/LevelDB aislado
    Http3Quic              // Transporte HTTP/3 y protocolos modernos
};

} // namespace vigilio
```

### 3. Perfiles de compatibilidad `WebEngineCompatibilityProfile`

```cpp
namespace vigilio {

enum class WebEngineCompatibilityProfile {
    FullDesktopBrowser,      // Exige Chromium >= 128, MV3, PDF, Sandboxing, LocalStorage
    HeadlessInspection,      // Exige Chromium >= 128, sin exigir display server ni GPU
    RestrictedSecureProfile, // Exige Chromium >= 128, Sandboxing estricto, sin WebRTC
    MinimalCore              // Modo degradado sin WebEngine (opera con core headless)
};

} // namespace vigilio
```

### 4. Estados y resultados de compatibilidad `WebEngineCompatibilityStatus`

```cpp
namespace vigilio {

enum class WebEngineCompatibilityStatus {
    Compatible,                  // Todas las comprobaciones satisfechas exitosamente
    DegradedHeadless,            // WebEngine ausente o desactivado; núcleo opera normalmente
    IncompatibleQtVersion,       // Qt < 6.10.0 detectado
    IncompatibleChromiumVersion, // Chromium milestone < 128
    ProcessHelperMissing,        // QtWebEngineProcess no encontrado o no ejecutable
    FeatureUnsupported,          // Capacidad crítica requerida ausente en el binario
    EnvironmentMisconfigured     // Fallo en configuración del SO o display server
};

} // namespace vigilio
```

### 5. Estructura de reporte `WebEngineCompatibilityReport`

```cpp
namespace vigilio {

struct WebEngineCompatibilityReport {
    WebEngineCompatibilityStatus status{WebEngineCompatibilityStatus::Compatible};
    std::optional<SemanticVersion> detectedQtVersion{};
    std::optional<ChromiumVersion> detectedChromiumVersion{};
    std::vector<WebEngineFeature> supportedFeatures{};
    std::vector<WebEngineFeature> missingFeatures{};
    std::string helperProcessPath{};
    std::string diagnosticMessage{};
    std::string remediationSuggestion{};
};

} // namespace vigilio
```

### 6. Servicio puro de dominio `WebEngineVersionValidator`

```cpp
namespace vigilio {

class WebEngineVersionValidator {
  public:
    // Parsing seguro de versiones de Chromium
    static std::optional<ChromiumVersion> parseChromiumVersion(std::string_view versionStr);

    // Baseline canónico de versión de Chromium
    static constexpr ChromiumVersion baselineChromiumVersion() noexcept {
        return ChromiumVersion{.major = 128, .minor = 0, .build = 0, .patch = 0};
    }

    // Mapeo entre versión de Qt y versión de Chromium esperada
    static ChromiumVersion minimumChromiumVersionForQt(const SemanticVersion& qtVersion);

    // Capacidades garantizadas según el hito de Chromium
    static std::vector<WebEngineFeature> supportedFeaturesForMilestone(uint32_t chromiumMilestone);

    // Capacidades requeridas por perfil de ejecución
    static std::vector<WebEngineFeature> requiredFeaturesForProfile(WebEngineCompatibilityProfile profile);

    // Validación integral de compatibilidad
    static WebEngineCompatibilityReport validateCompatibility(
        const SemanticVersion& qtVersion,
        const ChromiumVersion& chromiumVersion,
        WebEngineCompatibilityProfile profile,
        std::string_view helperProcessPath = "");

    // Formateo de diagnósticos y observabilidad
    static std::string formatStatus(WebEngineCompatibilityStatus status);
    static std::string formatReport(const WebEngineCompatibilityReport& report);
    static std::string_view featureName(WebEngineFeature feature) noexcept;
    static std::string_view statusName(WebEngineCompatibilityStatus status) noexcept;
};

} // namespace vigilio
```

## Contrato funcional y ciclo de vida

### 1. Invocación y actores
- **Actor principal**: La función de arranque de la aplicación (`src/app.cpp`) y el punto de composición de la ventana del navegador (`BrowserWindow`).
- **Punto de invocación**: Tras la validación básica de dependencias Qt (`QtDependencyValidator::validateRuntimeEnvironment`), antes de crear vistas web (`QWebEngineView`) o inicializar perfiles de motor (`QWebEngineProfile`).

### 2. Precondiciones e invariantes
- Las funciones de validación son puras, libres de efectos secundarios y no realizan asignaciones de recursos globales ni E/S de red.
- Si la versión de Qt es menor a 6.10.0, la evaluación de WebEngine aborta de inmediato con estado `IncompatibleQtVersion`.
- Si el hito de Chromium es menor a 128, se emite `IncompatibleChromiumVersion`.

### 3. Postcondiciones y transiciones
- Si el reporte es `Compatible`, la aplicación procede con la inicialización gráfica estándar.
- Si el reporte es `DegradedHeadless`, la aplicación puede optar por operar en modo terminal o notificar al usuario que la interfaz web no está disponible sin generar crashes de memoria.
- Si el reporte es un error crítico (`IncompatibleChromiumVersion`, `ProcessHelperMissing`), la aplicación imprime el reporte estructurado en `stderr` y termina de forma segura con código 1.

### 4. Errores y recuperación
- **Proceso asistente no ejecutable (`ProcessHelperMissing`)**: el reporte emite la ruta exacta esperada y la instrucción de configuración de paquetes de sistema (e.g. `libqt6webengine6-data` o reinstalación del paquete de navegador).
- **Entorno no compatible (`EnvironmentMisconfigured`)**: ante fallos en namespaces de Linux, el reporte sugiere la activación de `kernel.unprivileged_userns_clone=1` o el uso de sandboxing alternativo.

## Contrato de UI

- **Sin cambio visual directo**: Esta microtarea pertenece a la etapa `domain` del módulo `foundations` y no añade componentes gráficos, botones ni diálogos a la interfaz.
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Ningún término de ingeniería interna («Chromium», «WebEngine», «milestone», «MV3», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») será mostrado en la interfaz visible de la aplicación.
  - En etapas posteriores de UI, si se requiere informar al usuario de la degradación del motor web, se empleará un lenguaje sencillo: *"El visor de páginas web no está disponible en este momento. Puede consultar sus archivos locales y marcadores."*
- **Preservación del diseño visual**: Se garantiza coherencia con `resultado/01-vigilio-home.png` y `resultado/03-vigilio-settings-senior-mode.png`.

## Integración y seguridad

- **Filosofía Local-First (decisión Q-001)**:
  - Cero dependencias de servidores en la nube de Vigilio; cero verificación remota de firmas o actualizaciones automáticas ocultas.
- **Límites de procesos y sandboxing**:
  - WebEngine opera en arquitectura multiproceso donde los procesos de renderizado se ejecutan en espacios de nombres no privilegiados.
  - La verificación de compatibilidad asegura que no se desactive el sandbox (`--no-sandbox` está expresamente prohibido en producción).
- **Aislamiento de almacenamiento**:
  - Los datos locales se gestionan a través de `QStandardPaths` y bases de datos SQLite locales; los perfiles de incógnito no escriben en el almacenamiento permanente.
- **Presupuestos de recursos (RAM y CPU)**:
  - La evaluación del modelo de dominio se completa en menos de 1 milisegundo y consume menos de 50 KB de memoria.
  - No se lanzan procesos secundarios ni instancias de Chromium durante la validación de dominio.

## Matriz de pruebas trazables

| ID Prueba | Tipo | Criterio / Comportamiento | Fixture / Comando | Resultado Esperado | Clasificación |
| --- | --- | --- | --- | --- | --- |
| **TEST-001** | Unit | Parsing válido de versión Chromium cuatripartita | `ChromiumVersion::parse("128.0.6613.120")` | Objeto válido con `major=128, minor=0, build=6613, patch=120` | APPLY (en ciclo de implementación) |
| **TEST-002** | Unit | Parsing y normalización de versión de 2 y 3 componentes | `ChromiumVersion::parse("128.0.6613")` | Objeto normalizado con `patch=0` | APPLY |
| **TEST-003** | Unit | Rechazo de cadenas malformadas, vacías o alfabéticas | `ChromiumVersion::parse("abc")`, `parse("")`, `parse("128..0")` | `std::nullopt` sin excepciones | APPLY |
| **TEST-004** | Unit | Comparación total y ordenamiento (`<=>`, `==`, `<`, `>`) | Casos de versiones iguales, superiores e inferiores | Ordenamiento transitivo y consistente | APPLY |
| **TEST-005** | Unit | Predicado de compatibilidad con versión mínima (milestone >= 128) | `isCompatibleWith({128, 0, 0, 0})` con versiones 128, 130 y 118 | `true` para >= 128; `false` para < 128 | APPLY |
| **TEST-006** | Unit | Resolución de baseline de Chromium para Qt 6.10+ | `minimumChromiumVersionForQt({6, 10, 0})` | Versión Chromium 128.0.0.0 | APPLY |
| **TEST-007** | Unit | Mapeo de capacidades por hito (`ManifestV3Extensions`, etc.) | `supportedFeaturesForMilestone(128)` | Vector conteniendo MV3, PDF, Sandboxing, LocalDevStorage | APPLY |
| **TEST-008** | Unit | Validación de perfil `FullDesktopBrowser` positivo | Versión Qt 6.10.0 + Chromium 128 + proceso asistente | Estado `Compatible` | APPLY |
| **TEST-009** | Unit | Detección de versión incompatible de Chromium | Versión Chromium 118 (< 128) | Estado `IncompatibleChromiumVersion` | APPLY |
| **TEST-010** | Unit | Detección de proceso asistente ausente | Ruta vacía o archivo inexistente para perfil desktop | Estado `ProcessHelperMissing` | APPLY |
| **TEST-011** | Unit | Degradación limpia a modo headless (`MinimalCore`) | Perfil `MinimalCore` | Estado `DegradedHeadless` | APPLY |
| **TEST-012** | Unit | Invocación concurrente multihilo (`std::jthread`) | 10 hilos concurrentes evaluando validaciones | Cero condiciones de carrera, ejecución determinista | APPLY |
| **TEST-013** | Integración | Protocolo de arranque en `app.cpp` ante fallo | Simulación de reporte no compatible en pre-main | Salida de diagnóstico en `stderr` y salida limpia con código 1 | LATER |
| **TEST-014** | UI / Accesibilidad | Inspección de impacto visual en ventana principal | Inspección visual y lectores de pantalla | Cero alteración de interfaz gráfica existente | NOT_APPLICABLE |
| **TEST-015** | E2E | Prueba de humo de navegación web desktop | Ejecución serial con fixture local | Flujo crítico de navegación validado | NOT_APPLICABLE (en esta etapa) |

## Decisiones técnicas y trade-offs

1. **Desacoplamiento de Chromium frente a inclusión directa de cabeceras**:
   - *Alternativa A (Seleccionada)*: Modelar `ChromiumVersion` y el validador como código C++20 puro en `vigilio_core`, sin `#include <QWebEngineView>` ni cabeceras de Chromium.
   - *Alternativa B*: Usar APIs directas de Qt WebEngine (`QQuickWebEngineProfile`, etc.).
   - *Trade-off*: La alternativa A garantiza que `vigilio_core` y sus suites de prueba puedan compilarse y ejecutarse en cualquier entorno headless o servidor CI sin requerir paquetes gráficos ni motores de renderizado pesados, cumpliendo estrictamente con la arquitectura modular de Vigilio Browser.
2. **Definición del baseline en Chromium milestone 128**:
   - *Razón*: Qt 6.8 LTS y Qt 6.10+ basan su motor en Chromium 128+, el cual garantiza la implementación madura de Manifest V3 para extensiones web, aislamiento de procesos reforzado y mitigación de vulnerabilidades de seguridad conocidas en versiones previas.
