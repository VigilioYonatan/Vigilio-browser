# Change Proposal — module-foundations-task0018

Pack: `module.foundations.task0018`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **resource qrc inclusion** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en la microtarea precedente (`module.foundations.task0017`) la especificación del modelo de dominio, la taxonomía de categorías (`ResourceCategory`), los estados de inclusión (`ResourceInclusionStatus`), la entidad inmutable `ResourceDescriptor` y el catálogo canónico de los 35 recursos aprobados (`ResourceQrcCatalog`), esta microtarea proyecta la **implementación física, desacoplada y fuertemente tipada en C++20 de la capa de servicio y adaptador de inclusión de recursos**, garantizando provisión segura, manejo de errores observables, soporte de cancelación cooperativa y recuperación con fallbacks locales, sin acoplar lógica de negocio a los widgets de interfaz ni añadir responsabilidades a `BrowserWindow`.

El alcance concreto de este pack comprende:
- **Implementación del servicio propietario C++20 (`vigilio::ResourceInclusionService`)**:
  - Creación del componente de servicio y adaptador desacoplado en `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `ResourceInclusionService.cpp`, integrado en la biblioteca estática `vigilio_core`.
  - Estructuración inmutable y tipada en C++20 estándar de las entidades de la capa de operación y servicio:
    - `ResourceReadError`: enum class fuertemente tipado que categoriza exhaustivamente los fallos de lectura (`None`, `NotFound`, `AccessDenied`, `InvalidFormat`, `CorruptedContent`, `Cancelled`, `IOError`).
    - `ResourceReadResult<T>`: plantilla struct inmutable con semántica de valor que encapsula el código de error (`error`), los datos opcionales leídos (`std::optional<T> data`), el mensaje de diagnóstico explicativo (`errorMessage`) y la bandera de indicación de fallback (`hasFallback`).
    - `ResourceStartupStatus`: enum class determinista de evaluación de arranque (`Ready`, `Degraded`, `MissingCritical`, `CatalogCorruption`).
    - `ResourceStartupReport`: estructura inmutable que consolida el estado final de verificación en arranque (`status`), conteo de recursos verificados (`totalVerified`, `criticalVerified`), listas de recursos faltantes o advertencias (`missingCritical`, `warnings`), diagnóstico estructurado legible (`diagnosticMessage`) y acción de mitigación recomendada (`remediationAction`).
    - `CancellationToken`: mecanismo ligero, no bloqueante y thread-safe para soporte de cancelación cooperativa en cargas asíncronas o diferidas de recursos potencialmente pesados (como fondos JPEG de alta resolución).
  - Implementación de métodos puros de servicio y provisión desacoplada:
    - `ResourceInclusionService::validateStartupResources(...)`: valida en tiempo de arranque que todos los recursos críticos (`ResourceQrcCatalog::criticalResources()`) estén accesibles e íntegros antes de inicializar la interfaz gráfica.
    - `ResourceInclusionService::loadTextResource(...)`: provee lectura tipada de recursos textuales (esquemas JSON de temas y tokens de diseño) con opción de recuperación automática mediante fallback determinista al tema predeterminado (`themes/light.json`).
    - `ResourceInclusionService::loadBinaryResource(...)`: provee lectura tipada de recursos binarios (vectores de bytes para SVG o JPEG) con soporte de cancelación cooperativa mediante `CancellationToken`.
    - `ResourceInclusionService::resolveIconWithFallback(...)`: resuelve la ruta virtual canónica de un icono; si el alias es inválido o no existe, retorna una ruta de fallback segura (`:/vigilio/icons/tools/tools.svg`) evitando estados en blanco o caídas de UI.
    - `ResourceInclusionService::resolveThemeWithFallback(...)`: resuelve el alias de un tema con fallback automático a `themes/light.json`.
    - `ResourceInclusionService::defaultThemeAlias(...)`: retorna de forma inmutable el identificador del tema base (`"themes/light.json"`).
    - `ResourceInclusionService::errorName(...)` y `startupStatusName(...)`: formateadores estáticos de diagnósticos y estados para observabilidad y logging estructurado.
- **Protocolo de pre-validación en tiempo de ejecución (Startup Bootstrap en `apps/vigilio-browser/src/app.cpp`)**:
  - Incorporación de la validación previa de recursos QRC tras la verificación de dependencias Qt (`QtDependencyValidator`) y la compatibilidad de WebEngine (`WebEngineVersionValidator`):
    1. Antes de instanciar `BrowserWindow` y los widgets principales, se evalúa la integridad y disponibilidad de recursos mediante `ResourceInclusionService::validateStartupResources()`.
    2. Si el estado es `MissingCritical` o `CatalogCorruption`, se emite un error estructurado a `stderr` con la acción de mitigación y se termina de forma limpia y controlada con código de salida 1, previniendo excepciones no controladas, punteros nulos o corrupción de memoria en la interfaz.
    3. Si el estado es `Degraded`, se registra la advertencia informativa en el log local y se permite el arranque seguro activando los fallbacks predeterminados.
- **Suite de pruebas unitarias de implementación (`apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp`)**:
  - Creación de pruebas unitarias exhaustivas en CTest (`resource_inclusion_service_tests`), etiquetadas bajo `LABELS "unit;fast"`, cubriendo:
    - Validación de arranque exitosa (todos los recursos críticos presentes -> `Ready`).
    - Detección y reporte estructurado ante ausencia simulada de recursos críticos (`MissingCritical`).
    - Carga de recursos textuales con y sin fallback automático.
    - Carga de recursos binarios con cancelación cooperativa exitosa vía `CancellationToken`.
    - Resolución de iconos y temas con sustitución por fallback determinista ante identificadores malformados o ausentes.
    - Estrés multihilo de consultas concurrentes para garantizar pureza y thread-safety sin contención de cerrojos.
- **Gobernanza de compilación en CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Incorporación de `src/core/ResourceInclusionService.cpp` en el target estático `vigilio_core`.
  - Registro del ejecutable de prueba `vigilio_resource_inclusion_tests` y del test `resource_inclusion_service_tests` en CTest.
  - Aseguramiento de que `vigilio_core` compila limpiamente bajo C++20 con `-Werror` sin depender de bibliotecas gráficas pesadas ni de `QApplication`.
- **Garantías de aislamiento, rendimiento y privacidad**:
  - *Desacoplamiento headless absoluto*: la capa de servicio compila y ejecuta 100% en C++20 estándar; opera tanto en modo desktop con GUI como en servidores CI sin servidor gráfico.
  - *Filosofía local-first (Decisión Q-001)*: cero peticiones de red, cero descargas externas de activos, sin servicios en la nube ni telemetría remota.
  - *Presupuesto de recursos*: verificación de arranque < 2 ms, huella de memoria RAM < 30 KB, sin hilos en segundo plano ni procesos zombis.
  - *Ausencia total de cambio visual directo*: la implementación provee el servicio técnico de acceso a recursos; la integración visual directa en widgets de `BrowserWindow` se reserva para la etapa `ui` (`module.foundations.task0019`).

## Out of scope

- Instanciación de widgets `QIcon` o renderizado de pixmaps en menús, barras de herramientas o pestañas de `BrowserWindow`, diferido a la etapa `ui` en `module.foundations.task0019`.
- Arneses pesados de integración con servidor gráfico o pruebas E2E visuales de renderizado de SVG, diferido a la etapa `tests` en `module.foundations.task0020`.
- Empaquetado del instalador o inclusión de recursos exclusivos del instalador (`assets/installer/**`), reservado para el módulo de empaquetado y release (`module.foundations.task0053` a `task0056`).
- Modificación directa de archivos de código fuente de producción bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida rigurosamente a `openspec/changes/module-foundations-task0018/**`).
- Cualquier comunicación por red, descarga remota de fuentes o temas dinámicos desde internet (prohibido por arquitectura).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código existente en `apps/vigilio-browser`, los módulos de dominio previos y las herramientas locales:
1. **Modelo de dominio de recursos existente (`src/core/ResourceTypes.h`, `ResourceQrcCatalog.h`, `ResourceQrcCatalog.cpp`)**:
   - Creado y validado en la microtarea precedente `module.foundations.task0017`.
   - Modela `ResourceCategory`, `ResourceInclusionStatus`, `ResourceDescriptor` y el catálogo inmutable `ResourceQrcCatalog` conteniendo con exactitud los 35 recursos aprobados bajo el prefijo `/vigilio`.
   - Cuenta con una suite unitaria determinista (`ResourceQrcCatalogTests.cpp`) que pasa al 100% en CTest.
2. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Actualmente valida el entorno en tiempo de ejecución con `QtDependencyValidator::validateRuntimeEnvironment` (líneas 14-20) y la compatibilidad con `WebEngineVersionValidator::validateStartup` (líneas 22-32).
   - Sin embargo, inmediatamente después ejecuta `QApplication app(argc, argv)` e instancia `BrowserWindow window` sin validar si los recursos compilados indispensables (branding, tokens, temas light/dark, iconos básicos) están disponibles, arriesgando fallos silenciosos de UI o caídas si el bundle QRC está dañado o desalineado.
3. **Ausencia de capa de servicio y adaptadores en el estado actual**:
   - No existe un servicio que exponga operaciones de carga con tipado seguro, fallbacks controlados ante recursos ausentes o soporte de cancelación cooperativa.
   - Cualquier cliente futuro que intente cargar un tema o un icono tendría que implementar su propia lógica ad-hoc de resolución de rutas y gestión de errores, duplicando código y violando los principios SOLID y DRY de `docs/skills/01-principios-de-ingenieria.md`.
4. **Validación de entorno y pipeline de pruebas**:
   - Pipeline de integración `bash apps/vigilio-browser/scripts/check.sh`: 6/6 pruebas unitarias e integración pasando al 100% en 0.13 segundos (exit code 0).
   - Formateo de código: Clang-Format verificado sin discrepancias.
   - Herramienta Prompt Builder CLI (`/config/Desktop/auto/prompt-builder/dist/cli.js`):
     - `validate-references --project-root .`: 18/18 referencias canónicas validadas con estado `valid: true` (exit code 0).
     - `verify-lock --lock-file generated/prompts/full-project/0052-module.foundations.task0018.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
     - `validate-decisions`: Confirma que Q-001 a Q-006 y Q-008 están resueltas (Q-007 solo aplica al hito final de lanzamiento público).

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0018`.
- **Capacidad**: `resource qrc inclusion`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0018/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0018/proposal.md`
  - `openspec/changes/module-foundations-task0018/design.md`
  - `openspec/changes/module-foundations-task0018/tasks.md`
  - `openspec/changes/module-foundations-task0018/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0018/**`. No se autoriza modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de la capa de implementación para **resource qrc inclusion** se estructura en el espacio de nombres `vigilio`:
1. **Modelo de errores y resultados (`ResourceInclusionTypes.h` o sección tipada de `ResourceInclusionService.h`)**:
   - `enum class ResourceReadError : uint8_t { None, NotFound, AccessDenied, InvalidFormat, CorruptedContent, Cancelled, IOError };`
   - `template <typename T> struct ResourceReadResult { ResourceReadError error{ResourceReadError::None}; std::optional<T> data{std::nullopt}; std::string errorMessage{}; bool hasFallback{false}; bool isSuccess() const noexcept { return error == ResourceReadError::None && data.has_value(); } };`
   - `enum class ResourceStartupStatus : uint8_t { Ready, Degraded, MissingCritical, CatalogCorruption };`
   - `struct ResourceStartupReport { ResourceStartupStatus status{ResourceStartupStatus::Ready}; size_t totalVerified{0}; size_t criticalVerified{0}; std::vector<std::string> missingCritical{}; std::vector<std::string> warnings{}; std::string diagnosticMessage{}; std::string remediationAction{}; };`
   - `class CancellationToken`:
     - Token atómico y cooperativo para abortar de manera no bloqueante operaciones de lectura de recursos en segundo plano.
     - `explicit CancellationToken(bool initial = false) noexcept;`
     - `void requestCancellation() noexcept;`
     - `[[nodiscard]] bool isCancelled() const noexcept;`
2. **Servicio desacoplado de inclusión de recursos (`ResourceInclusionService`)**:
   - Métodos estáticos puros y thread-safe:
     - `[[nodiscard]] static ResourceStartupReport validateStartupResources(std::string_view qrcContent = {}) noexcept;`
     - `[[nodiscard]] static ResourceReadResult<std::string> loadTextResource(std::string_view alias, bool allowFallback = true) noexcept;`
     - `[[nodiscard]] static ResourceReadResult<std::vector<uint8_t>> loadBinaryResource(std::string_view alias, const CancellationToken* cancelToken = nullptr) noexcept;`
     - `[[nodiscard]] static std::string resolveIconWithFallback(std::string_view alias) noexcept;`
     - `[[nodiscard]] static std::string resolveThemeWithFallback(std::string_view themeAlias) noexcept;`
     - `[[nodiscard]] static constexpr std::string_view defaultThemeAlias() noexcept { return "themes/light.json"; }`
     - `[[nodiscard]] static constexpr std::string_view fallbackIconAlias() noexcept { return "icons/tools/tools.svg"; }`
     - `[[nodiscard]] static std::string_view errorName(ResourceReadError error) noexcept;`
     - `[[nodiscard]] static std::string_view startupStatusName(ResourceStartupStatus status) noexcept;`
3. **Mecanismo de fallback determinista y recuperación**:
   - Ante la petición de un tema inexistente, corrupto o malformado, `loadTextResource` o `resolveThemeWithFallback` devuelven el tema predeterminado `themes/light.json` marcando `hasFallback = true`.
   - Ante la petición de un icono no reconocido o malformado, `resolveIconWithFallback` devuelve `":/vigilio/icons/tools/tools.svg"`.
   - Se asegura que ningún componente de UI quede en estado roto o con puntero nulo.
4. **Integración en `src/app.cpp`**:
   - Integración de `validateStartupResources()` en el flujo de arranque antes de inicializar la interfaz gráfica, garantizando que el proceso falle rápidamente y con diagnósticos accionables en caso de daño catastrófico del empaquetado.

## Archivos afectados

Durante esta fase PROPOSE, la escritura está rigurosamente limitada a la raíz OpenSpec del cambio:
- `openspec/changes/module-foundations-task0018/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0018/proposal.md`
- `openspec/changes/module-foundations-task0018/design.md`
- `openspec/changes/module-foundations-task0018/tasks.md`
- `openspec/changes/module-foundations-task0018/execution-memory.md`

En la fase futura APPLY (tras aprobación humana en `module.foundations.task0018`):
- `apps/vigilio-browser/src/core/ResourceInclusionService.h`: Declaración de tipos de operación, errores, resultados, token de cancelación e interfaz del servicio de inclusión de recursos.
- `apps/vigilio-browser/src/core/ResourceInclusionService.cpp`: Implementación de los métodos de pre-validación de arranque, carga de recursos de texto y binarios, fallbacks deterministas y formateadores de diagnósticos.
- `apps/vigilio-browser/src/app.cpp`: Integración de la pre-validación de recursos en la secuencia de inicialización del proceso.
- `apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp`: Suite de pruebas unitarias automatizadas en C++20 para la capa de servicio de inclusión de recursos.
- `apps/vigilio-browser/CMakeLists.txt`: Incorporación de `src/core/ResourceInclusionService.cpp` al target estático `vigilio_core` y registro del ejecutable de test `vigilio_resource_inclusion_tests` en CTest.

## Contrato UX/UI

- **Declaración formal**: **Sin cambio visual directo**.
- **Justificación**: Esta microtarea pertenece a la etapa `implementation` y provee la capa de servicio C++20, validación en arranque, mecanismos de cancelación y recuperación ante fallos para la gestión de recursos. No altera layouts de ventanas, no añade widgets a `BrowserWindow` y no renderiza directamente en pantalla.
- **Preservación y soporte de la experiencia visual objetivo**:
  - Garantiza que los componentes de la interfaz puedan obtener de forma confiable los activos visuales estipulados en:
    - `/resultado/01-vigilio-home.png`: Sidebar con iconos Lucide y fondos de paisaje.
    - `/resultado/02-vigilio-tools-productivity.png`: Iconos vectoriales de herramientas de productividad.
    - `/resultado/03-vigilio-settings-senior-mode.png`: Icono específico `icons/tools/senior.svg` para «Interfaz más fácil de usar» y tokens de contraste y escalado.
    - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial de marca y logotipo corporativo (`branding/vigilio-logo.svg`).
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - No se exponen etiquetas internas de ingeniería («QRC», «RCC», «viewBox», «DOM», «Senior Mode», «Prompt Builder», «OpenSpec») en ningún mensaje visible al usuario general.
  - Los mensajes de diagnóstico generados por el servicio se dirigen exclusivamente a la consola de inicio (`stderr`) o registros de observabilidad interna para facilitar la depuración técnica.

## Reglas y fallos

- **R-001 (Verificación mandatoria de recursos críticos en arranque)**: Si `validateStartupResources()` detecta que falta alguno de los 8 recursos críticos (`branding/vigilio-logo.svg`, `design-tokens/tokens.json`, `themes/light.json`, `themes/dark.json`, `icons/navigation/search.svg`, `back.svg`, `forward.svg`, `reload.svg`), el estado resultante es `MissingCritical` y el arranque debe abortarse con código de salida 1.
- **R-002 (Recuperación elegante con fallbacks locales)**: Si un recurso secundario o no crítico falla al ser consultado por alias, el servicio debe sustituirlo de forma transparente por el recurso de fallback canónico (`themes/light.json` para temas, `icons/tools/tools.svg` para iconos), marcando `hasFallback = true` y evitando crasheos de la interfaz.
- **R-003 (Soporte de cancelación cooperativa no destructiva)**: Operaciones de lectura sobre recursos que reciban una señal activa de cancelación vía `CancellationToken` deben interrumpir el procesamiento de inmediato y retornar `ResourceReadError::Cancelled` sin fugas de memoria ni estados inconsistentes.
- **R-004 (Aislamiento frente a rutas prohibidas)**: El servicio valida que ninguna petición intente resolver o leer rutas bajo `reference/`, `installer/`, `fonts/` o rutas absolutas de disco, rechazándolas de inmediato con `ResourceReadError::AccessDenied`.
- **R-005 (Filosofía Local-First estricta - Decisión Q-001)**: Queda terminantemente prohibido cualquier mecanismo de descarga remota de activos, reintentos por red o sincronización con servidores en la nube.
- **R-006 (Thread-Safety y pureza funcional)**: Todos los métodos estáticos del servicio son seguros para ser invocados de manera concurrente desde múltiples hilos sin contención de locks ni condiciones de carrera.

## Seguridad y privacidad

- **Segmento de solo lectura y resistencia a manipulación**: Los recursos servidos provienen del catálogo compilado en `.rodata` o del sistema virtual `:/vigilio/`, impidiendo ataques de sustitución de DLLs o inyección de archivos maliciosos en disco.
- **Aislamiento de historial y navegación privada**: La lectura y provisión de recursos no guarda ningún registro en base de datos SQLite ni interactúa con la sesión o historial de navegación.
- **Sanitización de rutas y prevención de Directory Traversal**: Se reutiliza la validación estricta de alias de `ResourceQrcCatalog::isValidAlias` para neutralizar intentos de escape de directorio (`../`).
- **Eficiencia y presupuesto de memoria**: Huella estática menor a 30 KB en `vigilio_core`, tiempo de verificación en arranque inferior a 2 milisegundos y cero consumo de hilos de CPU en reposo (política Zero-Zombie).

## Matriz de pruebas

| Identificador | Criterio de validación | Fixture / Entrada | Comando / Ejecutable | Resultado esperado | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Verificación exitosa de arranque con catálogo canónico | Invocación a `validateStartupResources()` sin XML anómalo | `ctest -R resource_inclusion_service_tests` | Retorna `ResourceStartupStatus::Ready`, criticalVerified = 8, missingCritical vacío | APPLY |
| **TEST-002** | Detección y fallo ante falta de recurso crítico | XML sintético que omite `branding/vigilio-logo.svg` | `ctest -R resource_inclusion_service_tests` | Retorna `MissingCritical`, lista `branding/vigilio-logo.svg` y diagnóstico explicativo | APPLY |
| **TEST-003** | Carga de recurso de texto existente | Alias `"themes/light.json"` | `ctest -R resource_inclusion_service_tests` | Retorna `ResourceReadResult` con `error == None`, `hasFallback == false` y contenido válido | APPLY |
| **TEST-004** | Carga de tema inexistente con fallback habilitado | Alias `"themes/nonexistent.json"`, `allowFallback = true` | `ctest -R resource_inclusion_service_tests` | Retorna `hasFallback == true`, datos correspondientes al tema light por defecto | APPLY |
| **TEST-005** | Carga de tema inexistente con fallback deshabilitado | Alias `"themes/nonexistent.json"`, `allowFallback = false` | `ctest -R resource_inclusion_service_tests` | Retorna `error == ResourceReadError::NotFound`, `data == nullopt` | APPLY |
| **TEST-006** | Rechazo de acceso a rutas excluidas | Alias `"reference/mockup.png"` o `"installer/setup.exe"` | `ctest -R resource_inclusion_service_tests` | Retorna `error == ResourceReadError::AccessDenied`, `data == nullopt` | APPLY |
| **TEST-007** | Cancelación cooperativa de lectura binaria | Carga de wallpaper con `CancellationToken` activado | `ctest -R resource_inclusion_service_tests` | Retorna `error == ResourceReadError::Cancelled` sin completar lectura | APPLY |
| **TEST-008** | Resolución de iconos con fallback ante alias inválido | Alias `"icons/navigation/invalid.svg"` o malformado | `ctest -R resource_inclusion_service_tests` | Retorna la ruta virtual segura `":/vigilio/icons/tools/tools.svg"` | APPLY |
| **TEST-009** | Concurrencia multihilo en consultas masivas | 16 hilos ejecutando concurrentemente 2000 lecturas y resoluciones | `ctest -R resource_inclusion_service_tests` | 100% de consultas exitosas, cero data races en ThreadSanitizer, tiempo < 100 ms | APPLY |
| **TEST-010** | Carga real de pixmaps y renderizado en widgets Qt | Instanciación de `BrowserWindow` y pintado de icono | `ctest -R window_ui_tests` | Renderizado exitoso en widgets de interfaz | LATER |
| **TEST-011** | Empaquetado binario y assets del instalador | Verificación de instalador generado | Script de release / empaquetado | Generación limpia de instalador oficial | NOT_APPLICABLE |

## Pasos APPLY

1. **Definición de tipos e interfaz de servicio en `apps/vigilio-browser/src/core/ResourceInclusionService.h`**:
   - Declarar `ResourceReadError`, `ResourceReadResult<T>`, `ResourceStartupStatus`, `ResourceStartupReport`, `CancellationToken` y la clase de servicio `ResourceInclusionService`.
2. **Implementación de la lógica de servicio en `apps/vigilio-browser/src/core/ResourceInclusionService.cpp`**:
   - Codificar `validateStartupResources`, `loadTextResource`, `loadBinaryResource`, `resolveIconWithFallback`, `resolveThemeWithFallback` y los formateadores de diagnóstico.
3. **Integración en la secuencia de inicio de la aplicación en `apps/vigilio-browser/src/app.cpp`**:
   - Invocar `ResourceInclusionService::validateStartupResources()` tras la validación de WebEngine; si el estado es `MissingCritical`, emitir diagnóstico y salir con código 1.
4. **Implementación de la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/ResourceInclusionServiceTests.cpp`**:
   - Crear casos de prueba exhaustivos que validen casos normales, errores de lectura, fallbacks, cancelación cooperativa y estrés concurrente.
5. **Actualización de la configuración de compilación en `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/ResourceInclusionService.cpp` al target `vigilio_core`.
   - Registrar el ejecutable `vigilio_resource_inclusion_tests` y el test `resource_inclusion_service_tests` bajo `LABELS "unit;fast"`.
6. **Ejecución y validación del pipeline continuo**:
   - Compilar con Ninja y ejecutar `bash apps/vigilio-browser/scripts/check.sh`, verificando 100% de pruebas aprobadas con exit code 0.
   - Validar formato estricto con Clang-Format.
7. **Certificación documental y de referencias OpenSpec**:
   - Ejecutar `validate-references` y `verify-lock` con la CLI de Prompt Builder asegurando estado `valid: true`.

## Definición de terminado

- [ ] Propuesta OpenSpec formalizada en `openspec/changes/module-foundations-task0018/{proposal.md,design.md,tasks.md,execution-memory.md}` con estado `PROPOSED` y referencias canónicas validadas.
- [ ] Especificación completa de la capa de servicio `ResourceInclusionService`, definiendo contratos fuertemente tipados en C++20 para pre-validación de arranque, carga de recursos, cancelación cooperativa y fallbacks.
- [ ] Mecanismos deterministas de recuperación ante errores (temas e iconos por defecto) documentados y trazados a la matriz de pruebas.
- [ ] Protocolo de pre-validación en arranque integrado en el flujo de inicialización (`src/app.cpp`).
- [ ] Desacoplamiento arquitectónico preservado en `vigilio_core` sin añadir dependencias de GUI ni WebEngine.
- [ ] Cumplimiento de la decisión local-first Q-001 y del manual de estilo `docs/UX-COPY-PUBLICO.md`.
- [ ] Pipeline local y herramientas de Prompt Builder (`validate-references`, `verify-lock`) ejecutados con resultado limpio (exit code 0).

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0017`: Definió el modelo de dominio y catálogo canónico (`ResourceQrcCatalog`, `ResourceTypes.h`).
  - Cadena de herramientas C++20 estándar (GCC 15+, Clang 18+, MSVC 2022+), CMake >= 3.24, Ninja >= 1.12.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Impacto en el tiempo de arranque de la aplicación al verificar los recursos críticos.
    *Mitigación*: La verificación de los 8 recursos críticos sobre estructuras inmutables se ejecuta en menos de 2 ms, despreciable frente a los tiempos de carga del servidor gráfico.
  - *Riesgo*: Lecturas concurrentes bloqueantes desde hilos secundarios al solicitar iconos o temas.
    *Mitigación*: Métodos puros y libres de efectos secundarios; las tablas estáticas inmutables permiten consultas totalmente libres de cerrojos (`lock-free`).
  - *Riesgo*: Comportamientos impredecibles si un archivo de tema está truncado o dañado en compilaciones experimentales.
    *Mitigación*: El servicio implementa fallback determinista inmediato al tema predeterminado `themes/light.json` con reporte explícito en `hasFallback`.

## Commit propuesto

`feat(foundations): implementation resource-qrc-inclusion`
