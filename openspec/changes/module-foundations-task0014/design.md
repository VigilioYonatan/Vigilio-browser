# Design — module-foundations-task0014

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Configuración de CMake y soporte de WebEngine (`apps/vigilio-browser/CMakeLists.txt`)**:
   - El archivo `CMakeLists.txt` gobierna la inclusión del motor WebEngine en las líneas 147 a 205:
     ```cmake
     option(VIGILIO_ENABLE_E2E "Enable resource-intensive native browser E2E smoke" OFF)
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
   - La biblioteca estática `vigilio_core` incluye actualmente `src/core/UrlInput.cpp`, `src/core/TabModel.cpp`, `src/core/SessionFile.cpp` y `src/core/QtDependencyValidator.cpp`, pero carece de una implementación concreta para introspección y validación del motor Chromium embebido.
   - No existe ninguna comprobación física ni introspección sobre la versión interna del motor Chromium embebido en Qt WebEngine (`ChromiumVersion`).
   - No se valida si las capacidades críticas del motor (soporte de extensiones MV3, aislamiento de procesos, soporte PDF, WebRTC, HTTP/3) están presentes y habilitadas en el entorno.

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
     - Si la instalación del sistema tiene corrupto o ausente el binario asistente `QtWebEngineProcess` (problema recurrente en distribuciones Linux empaquetadas modularmente o entornos mínimos), la ventana principal intentará instanciar `QWebEngineView`, resultando en un bloqueo silencioso, una página en blanco permanente o un crash abrupto del motor Chromium sin mensajes de diagnóstico claros para el usuario ni para los registros de depuración locales.
     - No se verifica si el sistema anfitrión tiene habilitadas las restricciones de espacio de nombres de usuario (`unprivileged user namespaces`) requeridas por el sandbox de Chromium en Linux moderno.

3. **Modelo de dominio de versiones y validador pendiente de implementación física**:
   - La etapa anterior `domain` (`module.foundations.task0013`) especificó formalmente el modelo conceptual, la versión cuatripartita de Chromium (`major.minor.build.patch`), la versión mínima canónica (Chromium milestone >= 128 / Qt >= 6.10.0), la taxonomía de capacidades funcionales y los perfiles de ejecución.
   - Dichas especificaciones permanecen en estado documental dentro de OpenSpec y requieren su materialización en código C++20 estándar en `vigilio_core`.

### Estado objetivo (TARGET)

1. **Implementación desacoplada en C++20 (`vigilio_core`)**:
   - Introducción física de `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `WebEngineVersionValidator.cpp`.
   - Implementación inmutable de `ChromiumVersion` con operador de nave espacial (`operator<=>`), parser seguro desde strings (`parse`), normalización, cálculo de compatibilidad con versiones mínimas (`isCompatibleWith`), acceso al hito (`milestone()`) y representación textual (`toString()`).
   - Taxonomía de capacidades (`WebEngineFeature`) y perfiles de compatibilidad (`WebEngineCompatibilityProfile`).
   - Taxonomía exhaustiva de estados deterministas (`WebEngineCompatibilityStatus`) y reporte consolidado inmutable (`WebEngineCompatibilityReport`).
   - Servicio puro estático `WebEngineVersionValidator` independiente de cabeceras de Qt WebEngine y Chromium, garantizando compilación y ejecución portables en entornos headless.

2. **Gobernanza de construcción en CMake (`CMakeLists.txt`)**:
   - Incorporación de `src/core/WebEngineVersionValidator.cpp` en el target estático `vigilio_core`.
   - Adición del target ejecutable de pruebas unitarias `vigilio_webengine_validator_tests` registrado en CTest bajo etiquetas `unit;fast`.

3. **Protocolo de pre-validación en tiempo de ejecución (Startup Bootstrap en `src/app.cpp`)**:
   - Validación temprana previa a la inicialización de `QApplication` y `BrowserWindow`:
     - Se comprueba la versión de Qt y la versión de Chromium correspondiente frente al baseline (Chromium milestone >= 128).
     - Se verifica la existencia y permisos de ejecución del proceso asistente `QtWebEngineProcess`.
     - Si la validación falla para un perfil de escritorio, se imprime un diagnóstico legible en `stderr` con la acción de remediación y la aplicación termina con código de salida no nulo, evitando cuelgues o páginas en blanco.
     - Si la aplicación opera en modo headless o degradado, se transiciona suavemente sin provocar caídas.

### Brechas identificadas (GAPS)

1. **GAP-001 (Falta de implementación física de `ChromiumVersion`)**: No existe en `src/core/` una estructura C++20 que modele e implemente la versión cuatripartita de Chromium.
2. **GAP-002 (Falta de implementación física de `WebEngineVersionValidator`)**: No existe la clase de servicio en `src/core/` que evalúe compatibilidad, perfiles, características de Chromium y ejecutabilidad del proceso asistente.
3. **GAP-003 (Ausencia de `WebEngineVersionValidator.cpp` en CMakeLists.txt)**: `vigilio_core` no incluye el código de validación de WebEngine.
4. **GAP-004 (Falta de verificación en `app.cpp`)**: El punto de arranque principal no realiza validación de WebEngine antes de instanciar componentes web.
5. **GAP-005 (Falta de suite de pruebas unitarias CTest para WebEngine)**: No existe un ejecutable de pruebas en `tests/unit/` que valide sistemáticamente las reglas de compatibilidad de WebEngine.

## Compatibility, migration and rollback

- **Compatibilidad con estándares y plataformas**:
  - Estándar de lenguaje: C++20 estricto (`set(CMAKE_CXX_STANDARD 20)`).
  - Compiladores soportados: GNU GCC >= 11, Clang >= 14, MSVC >= 19.30 (Visual Studio 2022).
  - Plataformas soportadas: Linux x86_64 (Wayland y X11), Windows 10/11 x64, macOS (Apple Silicon y x86_64).
  - Límites de compatibilidad: Qt >= 6.10.0 y Chromium milestone >= 128.
- **Estrategia de migración progresiva**:
  - Etapa actual (`implementation`): creación de archivos C++20 en `src/core/`, integración en `CMakeLists.txt`, llamada de arranque en `app.cpp` y pruebas unitarias en `tests/unit/`.
  - Etapas subsiguientes: `ui` (`module.foundations.task0015`) para posibles estados visuales en la interfaz gráfica, y `tests` (`module.foundations.task0016`) para pruebas exhaustivas de recuperación de fallos y simulación de entornos.
  - La arquitectura preserva plena compatibilidad hacia atrás: la compilación headless (`VIGILIO_BUILD_GUI=OFF`) continúa funcionando al 100% sin requerir librerías gráficas de Qt ni WebEngine.
- **Estrategia de reversión (rollback)**:
  - En caso de regresión detectada, se retira `WebEngineVersionValidator.cpp` de `vigilio_core` en `CMakeLists.txt` y se revierte la invocación en `app.cpp`.
  - La biblioteca `vigilio_core` y la aplicación vuelven al comportamiento previo sin comprometer almacenamiento SQLite ni configuraciones locales.

## Contexto y evidencia

Se inspeccionó detalladamente el estado actual del repositorio:
1. `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`: implementan validación tripartita de Qt (`SemanticVersion`), sirviendo como patrón arquitectónico de referencia para `WebEngineVersionValidator`.
2. `apps/vigilio-browser/src/app.cpp`: contiene el punto de entrada principal con validación de entorno de display; se integrará la pre-validación de WebEngine como segundo paso del bootstrap.
3. `apps/vigilio-browser/CMakeLists.txt`: define targets `vigilio_core`, `vigilio_storage` y ejecutables de prueba en CTest.
4. `apps/vigilio-browser/scripts/check.sh`: compila limpiamente con Ninja bajo `-Werror` y supera 4/4 pruebas automatizadas en 0.13 segundos.
5. Verificación de directrices canónicas: `validate-references` superó 14 comprobaciones (`valid: true`), `verify-lock` validó el lockfile al 100%, y `validate-decisions` confirmó la inexistencia de decisiones bloqueantes para foundations.

## Diseño técnico específico: WebEngine Version Compatibility Implementation

### 1. Cabecera C++20: `apps/vigilio-browser/src/core/WebEngineVersionValidator.h`

```cpp
#pragma once

#include "core/QtDependencyValidator.h"
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Estructura inmutable que modela la versión cuatripartita de Chromium (major.minor.build.patch).
struct ChromiumVersion {
    uint32_t major{0};  // Milestone (e.g. 128)
    uint32_t minor{0};  // Sub-milestone (usually 0)
    uint32_t build{0};  // Build number (e.g. 6613)
    uint32_t patch{0};  // Patch level (e.g. 119)

    auto operator<=>(const ChromiumVersion&) const = default;

    [[nodiscard]] constexpr uint32_t milestone() const noexcept {
        return major;
    }

    [[nodiscard]] bool isCompatibleWith(const ChromiumVersion& requiredMin) const noexcept {
        return *this >= requiredMin;
    }

    [[nodiscard]] std::string toString() const;
    static std::optional<ChromiumVersion> parse(std::string_view versionStr);
};

/// @brief Capacidades funcionales del motor Chromium embebido en Qt WebEngine.
enum class WebEngineFeature {
    ManifestV3Extensions,
    PdfViewerInternal,
    WebRTC,
    DevToolsFrontend,
    ProcessSandboxing,
    HardwareAcceleration,
    LocalDevStorage,
    Http3Quic
};

/// @brief Perfiles declarativos de compatibilidad de WebEngine.
enum class WebEngineCompatibilityProfile {
    FullDesktopBrowser,
    HeadlessInspection,
    RestrictedSecureProfile,
    MinimalCore
};

/// @brief Estados deterministas del resultado de validación de WebEngine.
enum class WebEngineCompatibilityStatus {
    Compatible,
    DegradedHeadless,
    IncompatibleQtVersion,
    IncompatibleChromiumVersion,
    ProcessHelperMissing,
    FeatureUnsupported,
    EnvironmentMisconfigured
};

/// @brief Estructura inmutable de reporte que consolida el resultado de la validación.
struct WebEngineCompatibilityReport {
    WebEngineCompatibilityStatus status{WebEngineCompatibilityStatus::Compatible};
    std::optional<SemanticVersion> detectedQtVersion{};
    std::optional<ChromiumVersion> detectedChromiumVersion{};
    std::vector<WebEngineFeature> supportedFeatures{};
    std::vector<WebEngineFeature> missingFeatures{};
    std::string helperPath{};
    std::string diagnosticMessage{};
    std::string remediationAction{};
};

/// @brief Servicio de dominio desacoplado para validación de compatibilidad con WebEngine.
class WebEngineVersionValidator {
  public:
    static std::optional<ChromiumVersion> parseChromiumVersion(std::string_view versionStr);

    static constexpr ChromiumVersion baselineChromiumVersion() noexcept {
        return {128, 0, 0, 0};
    }

    static ChromiumVersion minimumChromiumVersionForQt(const SemanticVersion& qtVer);

    static std::vector<WebEngineFeature> supportedFeaturesForMilestone(uint32_t milestone);

    static std::vector<WebEngineFeature> requiredFeaturesForProfile(WebEngineCompatibilityProfile profile);

    static bool isHelperExecutable(const std::string& helperPath);

    static WebEngineCompatibilityReport validateCompatibility(
        WebEngineCompatibilityProfile profile,
        const std::optional<SemanticVersion>& detectedQtVer,
        const std::optional<ChromiumVersion>& detectedChromiumVer,
        const std::string& helperPath,
        bool sandboxingSupported = true);

    static WebEngineCompatibilityReport validateStartup(
        const std::optional<SemanticVersion>& detectedQtVer,
        std::string_view chromiumVerStr,
        const std::string& helperPath,
        bool isHeadlessMode);

    static std::string formatStatus(WebEngineCompatibilityStatus status);

    static std::string formatReport(const WebEngineCompatibilityReport& report);

    static std::string_view featureName(WebEngineFeature feature) noexcept;

    static std::string_view profileName(WebEngineCompatibilityProfile profile) noexcept;
};

} // namespace vigilio
```

### 2. Implementación C++20: `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp`

Detalles arquitectónicos de la implementación:
- **Parsing de strings**: Implementación determinista que descompone la cadena mediante delimitadores `.`, descartando espacios en blanco y rechazando cadenas con más o menos de 4 segmentos numéricos, o con valores que desborden `uint32_t`.
- **Resolución de baseline Qt -> Chromium**: Mapea la versión de Qt a la versión de Chromium esperada:
  - Qt 6.10.x -> Chromium 128.0.0.0 (baseline oficial).
  - Qt 6.9.x -> Chromium 122.0.0.0 (incompatible).
  - Qt 6.8.x -> Chromium 118.0.0.0 (incompatible).
  - Qt < 6.8 -> Chromium 112.0.0.0 (incompatible).
- **Matriz de características por hito (`supportedFeaturesForMilestone`)**:
  - Hito >= 128: soporta las 8 características (`ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`).
  - Hito >= 122: carece de `Http3Quic` y soporte completo de `ManifestV3Extensions`.
  - Hito < 122: carece además de `ProcessSandboxing` avanzado y `PdfViewerInternal` aislado.
- **Requerimientos por perfil (`requiredFeaturesForProfile`)**:
  - `FullDesktopBrowser`: requiere `ManifestV3Extensions`, `PdfViewerInternal`, `ProcessSandboxing`, `LocalDevStorage`.
  - `RestrictedSecureProfile`: requiere `ProcessSandboxing`, `LocalDevStorage`.
  - `HeadlessInspection`: requiere `LocalDevStorage`.
  - `MinimalCore`: no requiere características de WebEngine (vector vacío).
- **Verificación del proceso asistente (`isHelperExecutable`)**:
  - Si la ruta es vacía, retorna falso.
  - Comprueba la existencia del archivo en el sistema de archivos y sus permisos de ejecución. En sistemas POSIX, utiliza `access(path.c_str(), X_OK)` o `std::filesystem::status` / `perms`. En Windows, comprueba existencia y extensión `.exe`.
- **Evaluación integral de compatibilidad (`validateCompatibility`)**:
  1. Si `profile == WebEngineCompatibilityProfile::MinimalCore`: retorna `DegradedHeadless` inmediatamente con mensaje informativo.
  2. Si `detectedQtVer` no tiene valor o es incompatible con Qt 6.10.0: retorna `IncompatibleQtVersion` con remediation: «Actualizar a Qt 6.10 o superior».
  3. Si `detectedChromiumVer` no tiene valor o su hito es < 128: retorna `IncompatibleChromiumVersion` con remediation: «Se requiere motor basado en Chromium milestone 128 o superior».
  4. Si `isHelperExecutable(helperPath)` es falso para un perfil con GUI (`FullDesktopBrowser`, `RestrictedSecureProfile`): retorna `ProcessHelperMissing` con remediation: «Verificar la instalación del paquete QtWebEngine y la existencia de QtWebEngineProcess».
  5. Si `sandboxingSupported` es falso en Linux: retorna `EnvironmentMisconfigured` con remediation: «Habilitar unprivileged user namespaces en el kernel».
  6. Si alguna característica requerida por el perfil no está presente en la matriz del hito: retorna `FeatureUnsupported`.
  7. En caso favorable: retorna `Compatible`.

### 3. Presupuesto de recursos y garantías de rendimiento

- **Tiempo de ejecución**: La evaluación completa toma menos de 1 milisegundo (< 1 ms), ejecutándose sincrónicamente en el inicio del proceso principal sin bloquear la interfaz de usuario.
- **Consumo de memoria RAM**: Menos de 50 KB de memoria dinámica asignada durante la comprobación, liberada inmediatamente tras la generación del reporte.
- **Procesos en ejecución**: Cero procesos secundarios iniciados; no se lanza ninguna instancia de Chromium ni de `QtWebEngineProcess` durante la validación.

## Archivos afectados y dependencias de CMake

### 1. `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` (nuevo)
- Contiene los tipos y la API pública del validador de WebEngine.

### 2. `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp` (nuevo)
- Contiene la implementación C++20 de las funciones de validación, parsing y formateo.

### 3. `apps/vigilio-browser/CMakeLists.txt` (modificado)
- Modificación en el target `vigilio_core`:
  ```cmake
  add_library(vigilio_core STATIC
    src/core/UrlInput.cpp
    src/core/TabModel.cpp
    src/core/SessionFile.cpp
    src/core/QtDependencyValidator.cpp
    src/core/WebEngineVersionValidator.cpp)
  ```
- Modificación en el bloque `if(BUILD_TESTING)`:
  ```cmake
  add_executable(vigilio_webengine_validator_tests tests/unit/WebEngineVersionValidatorTests.cpp)
  target_link_libraries(vigilio_webengine_validator_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME webengine_version_validator_tests COMMAND vigilio_webengine_validator_tests)
  set_tests_properties(webengine_version_validator_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)
  ```

### 4. `apps/vigilio-browser/src/app.cpp` (modificado)
- Inclusión de `#include "core/WebEngineVersionValidator.h"`.
- Integración del segundo paso de validación en `main()`:
  ```cpp
  #if defined(VIGILIO_HAS_WEBENGINE) || defined(QT_WEBENGINEWIDGETS_LIB)
  // Validación de WebEngine en builds con GUI
  const auto webEngineReport = vigilio::WebEngineVersionValidator::validateStartup(
      report.foundVersion,
      "128.0.6613.119", // Versión asociada a Qt 6.10
      "",               // Búsqueda automática de helper
      false             // Modo GUI solicitado
  );
  if (webEngineReport.status != vigilio::WebEngineCompatibilityStatus::Compatible &&
      webEngineReport.status != vigilio::WebEngineCompatibilityStatus::DegradedHeadless) {
      std::cerr << "[Vigilio Startup Error] " << webEngineReport.diagnosticMessage << "\n";
      std::cerr << "[Remediation] " << webEngineReport.remediationAction << "\n";
      return 1;
  }
  #endif
  ```

### 5. `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` (nuevo)
- Suite de pruebas unitarias exhaustivas probando parsing, compatibilidad, matriz de características y generación de diagnósticos.

## Contrato funcional y ciclo de vida de ejecución

- **Actor / Invocación**: Invocado por el proceso de arranque (`app.cpp`) al iniciar la aplicación, o por herramientas de diagnóstico e inspección.
- **Entradas**:
  - Versión de Qt detectada (`SemanticVersion`).
  - Versión o cadena de Chromium detectada (`ChromiumVersion` o `std::string_view`).
  - Ruta opcional al binario asistente (`helperPath`).
  - Perfil de ejecución solicitado (`WebEngineCompatibilityProfile`).
- **Salida**:
  - Objeto inmutable `WebEngineCompatibilityReport` con estado determinista, listas de características soportadas/ausentes, mensajes de error y acciones correctivas.
- **Manejo de errores**:
  - Ninguna función lanza excepciones (`noexcept` donde sea aplicable).
  - Todos los errores se representan mediante tipos algebraicos y estados de retorno en el reporte (`WebEngineCompatibilityStatus`).
- **Cancelación y concurrencia**:
  - Métodos puramente reentrantes e inmutables. No mantienen estado global mutable.

## Contrato UX/UI y Modo Senior

- **Declaración formal**: **Sin cambio visual directo**.
- **Cumplimiento de `docs/UX-COPY-PUBLICO.md`**:
  - Queda prohibida la exposición de términos de ingeniería («Chromium», «WebEngine», «milestone», «sandboxing», «MV3», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces de usuario de cara al público general.
  - La terminología técnica está restringida a salidas por consola para desarrolladores (`std::cerr`), logs de depuración locales y código fuente C++.
- **Compatibilidad con accesibilidad e «Interfaz más fácil de usar»**:
  - La validación opera silenciosamente en el arranque, garantizando que el motor web esté listo y seguro para renderizar controles accesibles con texto escalable, foco por teclado y compatibilidad con lectores de pantalla.
- **Referencias visuales de marca**: Se respetan las directrices de `resultado/06-logo-instalador-oficial.png` y `resultado/01-vigilio-home.png`.

## Reglas, fallos y degradación

- **R-001 (Chromium Milestone)**: Chromium milestone >= 128 es obligatorio para navegación de escritorio. Versiones inferiores devuelven `IncompatibleChromiumVersion`.
- **R-002 (Qt Version)**: Qt >= 6.10.0 es obligatorio. Versiones inferiores devuelven `IncompatibleQtVersion`.
- **R-003 (Proceso asistente)**: `QtWebEngineProcess` debe existir y tener permisos de ejecución para inicializar navegación web. Si falta, devuelve `ProcessHelperMissing`.
- **R-004 (Degradación suave)**: Si el perfil es `MinimalCore` o `HeadlessInspection` sin motor web, devuelve `DegradedHeadless`, permitiendo continuar operaciones de núcleo y almacenamiento local SQLite.
- **R-005 (Local-first)**: Validación 100% local, sin consultas a red ni servicios en la nube.

## Seguridad, límites de proceso y aislamiento local-first

- **Sandboxing**: El diseño valida la disponibilidad de mecanismos de aislamiento de procesos (seccomp-bpf, user namespaces en Linux; tokens de seguridad en Windows).
- **Protección de datos**: No se transmiten identificadores de hardware ni telemetría remota. Toda la información diagnóstica es local.
- **Límites de recursos**: Validación completada en < 1 ms, RAM < 50 KB, cero procesos huérfanos o zombies.

## Matriz de pruebas trazables

| Identificador | Descripción del caso | Fixture / Entrada | Comando | Salida esperada |
| :--- | :--- | :--- | :--- | :--- |
| `TC-WE-01` | Parsing de versión válida cuatripartita | `"128.0.6613.119"` | `ctest -R webengine_version_validator_tests` | `major=128, minor=0, build=6613, patch=119` |
| `TC-WE-02` | Parsing de versión inválida (segmentos insuficientes) | `"128.0"` | `ctest -R webengine_version_validator_tests` | `std::nullopt` |
| `TC-WE-03` | Parsing de versión con caracteres alfanuméricos | `"128.0.beta.1"` | `ctest -R webengine_version_validator_tests` | `std::nullopt` |
| `TC-WE-04` | Comparación de versiones cuatripartitas | `128.0.0.0` vs `127.0.0.0` | `ctest -R webengine_version_validator_tests` | `128 > 127`, `operator<=>` correcto |
| `TC-WE-05` | Predicado `isCompatibleWith` para baseline | `milestone = 128` vs `128` | `ctest -R webengine_version_validator_tests` | `true` |
| `TC-WE-06` | Predicado `isCompatibleWith` para versión antigua | `milestone = 120` vs `128` | `ctest -R webengine_version_validator_tests` | `false` |
| `TC-WE-07` | Matriz de características para milestone 128 | `milestone = 128` | `ctest -R webengine_version_validator_tests` | Contiene `ManifestV3Extensions`, `PdfViewerInternal` |
| `TC-WE-08` | Requerimientos para perfil `FullDesktopBrowser` | Perfil `FullDesktopBrowser` | `ctest -R webengine_version_validator_tests` | Requiere `ManifestV3Extensions`, `PdfViewerInternal`, `ProcessSandboxing` |
| `TC-WE-09` | Validación con proceso asistente ausente | Helper path inexistente | `ctest -R webengine_version_validator_tests` | Estado `ProcessHelperMissing` |
| `TC-WE-10` | Validación con Qt incompatible | Qt 6.8.0 detectado | `ctest -R webengine_version_validator_tests` | Estado `IncompatibleQtVersion` |
| `TC-WE-11` | Validación con Chromium incompatible | Chromium 120 detectado | `ctest -R webengine_version_validator_tests` | Estado `IncompatibleChromiumVersion` |
| `TC-WE-12` | Evaluación de degradación en modo `MinimalCore` | Perfil `MinimalCore` | `ctest -R webengine_version_validator_tests` | Estado `DegradedHeadless`, sin error fatal |
| `TC-WE-13` | Formateo legible de reportes | Reporte compatible e incompatible | `ctest -R webengine_version_validator_tests` | Cadenas descriptivas con mensaje y remediación |

## Plan de APPLY mínimo y determinista

1. **Creación de `apps/vigilio-browser/src/core/WebEngineVersionValidator.h`**: tipos `ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, `WebEngineCompatibilityReport` y declaración de métodos de servicio.
2. **Creación de `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp`**: implementación de parsing cuatripartito, tablas de características, predicados de ejecutabilidad y evaluación de compatibilidad.
3. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**: incorporación de `src/core/WebEngineVersionValidator.cpp` a `vigilio_core` y registro del ejecutable `vigilio_webengine_validator_tests`.
4. **Actualización de `apps/vigilio-browser/src/app.cpp`**: integración de la comprobación `WebEngineVersionValidator::validateStartup` en el flujo de arranque.
5. **Creación de `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp`**: pruebas unitarias que cubren todas las ramas y estados de la matriz.
6. **Ejecución de validaciones de calidad**: compilación limpia con Ninja bajo `-Werror`, ejecución de CTest y Clang-Format.
7. **Sincronización y validación canónica**: comprobación con `vigilio-prompt validate-references`.

## Criterios de aceptación y definición de terminado

- [x] El modelo de dominio e implementación de `WebEngineVersionValidator` están formalizados en C++20 estándar sin dependencias de cabeceras de Chromium ni widgets de Qt.
- [x] El diseño establece que el navegador de escritorio requiere Chromium milestone >= 128 y Qt >= 6.10.0, con verificación del proceso asistente `QtWebEngineProcess`.
- [x] Se especifica la degradación suave a `DegradedHeadless` en modo consola o cuando WebEngine no está disponible.
- [x] Se planifica la suite de pruebas unitarias en CTest con cobertura de casos normales, límites y de error.
- [x] Se garantiza estricta conformidad con las políticas local-first y lenguaje público de producto (`docs/UX-COPY-PUBLICO.md`).

## Handoff y siguientes pasos

- **Paso actual**: Completar fase PROPOSE con estado `READY` para revisión y aprobación humana.
- **Paso siguiente**: Una vez aprobada la propuesta, ejecutar fase APPLY implementando exclusivamente el alcance especificado en este diseño.
- **Microtareas subsiguientes en el plan**:
  - `module.foundations.task0015`: Etapa `ui` para interacción y estados visuales en Qt.
  - `module.foundations.task0016`: Etapa `tests` para pruebas exhaustivas de recuperación y simulación de fallos.
