# Tasks — module-foundations-task0018

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos de error (`ResourceReadError`), resultado de lectura (`ResourceReadResult<T>`), estados de arranque (`ResourceStartupStatus`), reporte de arranque (`ResourceStartupReport`) y token de cancelación (`CancellationToken`) en `apps/vigilio-browser/src/core/ResourceInclusionService.h`
  - *Criterio de verificación*: Declaración de `enum class ResourceReadError`, `struct ResourceReadResult<T>` con `isSuccess()`, `enum class ResourceStartupStatus`, `struct ResourceStartupReport` y la clase atómica cooperativa `CancellationToken` con métodos `isCancelled()` y `requestCancellation()`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ResourceInclusionService.h && grep -E "enum class ResourceReadError|class CancellationToken" apps/vigilio-browser/src/core/ResourceInclusionService.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del servicio de inclusión de recursos (`ResourceInclusionService`) en `apps/vigilio-browser/src/core/ResourceInclusionService.h`
  - *Criterio de verificación*: Declaración de la clase `ResourceInclusionService` con constantes canónicas `kDefaultThemeAlias`, `kFallbackIconAlias`, y métodos estáticos `validateStartupResources`, `loadTextResource`, `loadBinaryResource`, `resolveIconWithFallback`, `resolveThemeWithFallback`, `defaultThemeAlias`, `fallbackIconAlias`, `errorName` y `startupStatusName`.
  - *Comando*: `grep -E "class ResourceInclusionService|validateStartupResources|resolveIconWithFallback" apps/vigilio-browser/src/core/ResourceInclusionService.h`

- [ ] TASK-003: Implementar la lógica de pre-validación de arranque de recursos críticos (`validateStartupResources`) y formateo de diagnósticos en `apps/vigilio-browser/src/core/ResourceInclusionService.cpp`
  - *Criterio de verificación*: `validateStartupResources` verifica la disponibilidad de los 8 recursos críticos de `ResourceQrcCatalog::criticalResources()`; si todos están presentes retorna `ResourceStartupStatus::Ready`; si falta alguno retorna `MissingCritical` con la lista de faltantes y acción de mitigación; `errorName` y `startupStatusName` retornan descripciones textuales legibles.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ResourceInclusionService.cpp && grep -E "validateStartupResources|criticalVerified" apps/vigilio-browser/src/core/ResourceInclusionService.cpp`

- [ ] TASK-004: Implementar las funciones de carga de recursos de texto (`loadTextResource`) y recursos binarios (`loadBinaryResource`) con cancelación cooperativa en `apps/vigilio-browser/src/core/ResourceInclusionService.cpp`
  - *Criterio de verificación*: `loadTextResource` lee contenido o sustituye por fallback a `themes/light.json` si `allowFallback == true`; rechaza rutas excluidas con `AccessDenied`; `loadBinaryResource` comprueba periódicamente `CancellationToken` y retorna `ResourceReadError::Cancelled` si se solicita la parada.
  - *Comando*: `grep -E "loadTextResource|loadBinaryResource|ResourceReadError::Cancelled" apps/vigilio-browser/src/core/ResourceInclusionService.cpp`

- [ ] TASK-005: Implementar los métodos de resolución segura de iconos (`resolveIconWithFallback`) y temas (`resolveThemeWithFallback`) en `apps/vigilio-browser/src/core/ResourceInclusionService.cpp`
  - *Criterio de verificación*: `resolveIconWithFallback` retorna la ruta canónica virtual para iconos válidos o `":/vigilio/icons/tools/tools.svg"` ante cualquier alias desconocido o malformado; `resolveThemeWithFallback` retorna el alias solicitado si existe o `"themes/light.json"` en caso contrario.
  - *Comando*: `grep -E "resolveIconWithFallback|resolveThemeWithFallback|icons/tools/tools.svg" apps/vigilio-browser/src/core/ResourceInclusionService.cpp`

- [ ] TASK-006: Integrar la pre-validación de recursos QRC críticos en el bootstrap de arranque de la aplicación en `apps/vigilio-browser/src/app.cpp`
  - *Criterio de verificación*: Invocación a `ResourceInclusionService::validateStartupResources()` tras la validación de WebEngine y antes de instanciar `BrowserWindow`; salida controlada con código 1 y mensaje en `stderr` si el estado es `MissingCritical` o `CatalogCorruption`.
  - *Comando*: `grep -E "ResourceInclusionService::validateStartupResources|MissingCritical" apps/vigilio-browser/src/app.cpp`

- [ ] TASK-007: Implementar la suite de pruebas unitarias automatizadas en `apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp`
  - *Criterio de verificación*: Cobertura exhaustiva de pre-validación normal (`Ready`), detección de recurso crítico ausente (`MissingCritical`), carga de texto con/sin fallback, rechazo de rutas prohibidas (`AccessDenied`), cancelación cooperativa binaria (`Cancelled`), resolución de iconos seguros y prueba de estrés concurrente con 16 hilos.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp && grep -E "test_startup_validation_success|test_cancellation" apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp`

- [ ] TASK-008: Integrar el servicio en el target `vigilio_core`, configurar el test `resource_inclusion_service_tests` en `apps/vigilio-browser/CMakeLists.txt` y validar con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia con `-Werror` bajo C++20; registro del ejecutable `vigilio_resource_inclusion_tests` y test de CTest `resource_inclusion_service_tests` bajo `LABELS "unit;fast"`; ejecución de `check.sh` con 100% de tests pasando en tiempo récord.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh && ctest --test-dir apps/vigilio-browser/_build -R resource_inclusion_service_tests --verbose`

- [ ] TASK-009: Certificar formalmente las referencias canónicas OpenSpec y el lockfile con la herramienta Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` para todas las referencias OpenSpec y `verify-lock` valida el lockfile `0052-module.foundations.task0018.propose.lock.json`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0052-module.foundations.task0018.propose.lock.json --project-root .`
