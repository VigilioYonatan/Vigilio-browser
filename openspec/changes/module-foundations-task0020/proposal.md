# Change Proposal — module-foundations-task0020

Pack: `module.foundations.task0020`

Status: PROPOSED

## Outcome and scope

Formalizar y proponer la arquitectura integral de pruebas automatizadas, deterministas y de alta resiliencia para la capacidad **resource qrc inclusion** en su etapa **tests** (microtarea `0020 — Probar casos positivos, negativos y recuperación: resource qrc inclusion`).

El alcance abarca:
- **Verificación exhaustiva de casos positivos e integridad física de activos**:
  - Inspección del catálogo completo de 35 recursos canónicos en sus 6 categorías (`Branding`, `DesignTokens`, `Theme`, `Wallpaper`, `NavigationIcon`, `ToolIcon`).
  - Validación de existencia física y tamaño no nulo de cada archivo en `apps/vigilio-browser/assets/**`.
  - Comprobación de formato y estructura interna: sintaxis JSON válida en tokens (`tokens.json`) y temas (`*.json`), estructura XML y especificación vectorial conforme a `11-assets-svg-quality.md` (`viewBox="0 0 24 24"`, trazos redondos) en todos los SVG, y encabezados de archivo JPEG válidos (`0xFF, 0xD8, 0xFF`) en fondos de pantalla.
  - Validación de correspondencia entre tipos MIME declarados y extensiones físicas de archivo.
  - Verificación de consistencia del manifiesto real `resources.qrc` frente al catálogo compilado en `ResourceQrcCatalog`.
- **Casos negativos y pruebas de robustez/seguridad (fuzzing y entradas maliciosas)**:
  - Rechazo determinista de intentos de Directory Traversal (`../`, `../../`, secuencias escapadas y rutas absolutas de sistema operativo).
  - Rechazo de alias malformados: cadenas vacías, espacios perimetrales o intercalados, caracteres en mayúsculas, signos no autorizados (`@`, `$`, `#`, `!`) y extensiones no soportadas (`.exe`, `.sh`, `.bin`, `.py`).
  - Detección y bloqueo de rutas explícitamente excluidas (`reference/`, `installer/`, `fonts/` no auditadas) con código `AccessDenied` o `ExcludedPathIncluded`.
  - Detección de corrupción sintáctica y estructural en manifiestos XML QRC: prefijos incorrectos (`/other`), ausencia de prefijo, etiquetas no cerradas, aliases duplicados y referencias a recursos ajenos al catálogo.
  - Aislamiento de fallos ante la ausencia individual y combinada de cualquiera de los 8 recursos críticos (`branding/vigilio-logo.svg`, `design-tokens/tokens.json`, `themes/light.json`, `themes/dark.json`, `icons/navigation/search.svg`, `icons/navigation/back.svg`, `icons/navigation/forward.svg`, `icons/navigation/reload.svg`).
- **Recuperación determinista, fallbacks y auto-reparación**:
  - Resolución resiliente de iconos desconocidos o corruptos hacia el icono canónico de respaldo `":/vigilio/icons/tools/tools.svg"`.
  - Resolución resiliente de temas anómalos o no encontrados hacia el tema base `"themes/light.json"`.
  - Recuperación en lectura de texto con `allowFallback = true`: retorno exitoso con indicador `hasFallback == true` y carga del activo supletorio.
  - Comportamiento estricto con `allowFallback = false`: reporte de error `ResourceReadError::NotFound` con datos nulos.
  - Diagnóstico estructurado y accionable: generación de mensajes de remediación comprensibles ante estados `MissingCritical` o `CatalogCorruption`.
- **Concurrencia multihilo, cancelación cooperativa y control de recursos**:
  - Prueba de estrés masivo con 16 hilos simultáneos y 2000 iteraciones por hilo ejecutando consultas de catálogo, resolución de rutas y validación de arranque sin cerrojos (`lock-free`), demostrando ausencia total de condiciones de carrera (cero data races bajo TSan) y latencias < 1 ms.
  - Cancelación cooperativa determinista mediante `CancellationToken`: aborto inmediato de operaciones de lectura binaria sin fugas de descriptores de archivo ni memoria huérfana.
  - Presupuesto de memoria estricta (< 30 KB en metadatos) y tiempo de inicialización inferior a 2 milisegundos.
- **Aislamiento headless y política Zero-Zombie**:
  - Ejecución 100% determinista sin requerir servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`), sin inicializar procesos Chromium ni WebEngine, y sin conexiones de red o telemetría (filosofía local-first constitucional, decisión Q-001).

## Out of scope

- Modificación de la lógica interna de `ResourceQrcCatalog` o `ResourceInclusionService`, consolidadas y verificadas en las etapas anteriores.
- Alteración de la presentación gráfica en `BrowserWindow` o cambios de layout visual en Qt Widgets (etapa `ui` completada en microtarea 0019).
- Modificación de assets binarios existentes en `apps/vigilio-browser/assets/**`.
- Ejecución de procesos pesados de Chromium o pruebas de renderizado en GPU (reservado para E2E serial bajo demanda).
- Integración de servicios cloud, autenticación o almacenamiento remoto.

## Contexto y evidencia

- **Exploración comprobable de componentes existentes en `apps/vigilio-browser`**:
  - `apps/vigilio-browser/src/core/ResourceTypes.h`: Define `ResourceCategory`, `ResourceDescriptor`, `ResourceReadError`, `ResourceStartupStatus`, `ResourceStartupReport` y `CancellationToken`.
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo inmutable de 35 recursos canónicos, mapeo de prefijo `:/vigilio`, reglas de exclusión y validación de manifiestos QRC.
  - `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `.cpp`: Servicio desacoplado de pre-validación en arranque, lectura de texto/binaria, fallbacks deterministas y diagnóstico.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Integración de iconos en la interfaz gráfica, escalado dinámico en «Interfaz más fácil de usar» (Senior Mode), accesibilidad y páginas de error.
  - `apps/vigilio-browser/resources.qrc`: Manifiesto canónico XML que empaqueta los 35 activos bajo el prefijo `/vigilio`.
  - `apps/vigilio-browser/assets/**`: 35 activos organizados en `branding/`, `design-tokens/`, `themes/`, `wallpapers/` e `icons/` (`navigation/` y `tools/`).
- **Estado de suites de prueba existentes**:
  - `tests/unit/ResourceQrcCatalogTests.cpp`: Valida conteos básicos, lookups, categorías y consistencia preliminar de XML.
  - `tests/unit/ResourceInclusionServiceTests.cpp`: Valida flujo básico de arranque, lectura de texto, fallbacks y concurrencia.
  - `tests/ui/WindowTests.cpp`: Valida iconos de ventana, botones de navegación, escalado y nombres accesibles en presencia de display.
- **Evidencia de ejecución del pipeline continuo (`check.sh`)**:
  - Ejecución de `bash apps/vigilio-browser/scripts/check.sh` pasando 7/7 pruebas unitarias e integración en 0.15s (exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.06s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.04s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.03s)
- **Evidencia documental y Prompt Builder**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 20 referencias validadas con estado `valid: true`.
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0058-module.foundations.task0020.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true`.

## Alcance exacto

El alcance exacto de la fase APPLY para `module.foundations.task0020` consiste en:
1. Crear la suite unitaria e integradora focalizada `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`, dedicada a certificar integralmente los contratos positivos, negativos, de recuperación y estrés concurrente de la capacidad `resource qrc inclusion`.
2. Registrar en `apps/vigilio-browser/CMakeLists.txt` el ejecutable `vigilio_resource_qrc_inclusion_tests` y el test de CTest `resource_qrc_inclusion_tests` bajo etiquetas `LABELS "unit;fast"` con timeout de 15 segundos.
3. Actualizar y sincronizar el estado de la microtarea en `docs/rules/01-modules/foundations/micro/0020-resource-qrc-inclusion/task.md` documentando la evidencia de pruebas.

## Diseño completo de la capacidad

El diseño de la capacidad de pruebas automatizadas comprende 10 módulos de verificación determinista integrados en `ResourceQrcInclusionTests.cpp`:
1. `test_physical_assets_integrity()`: Recorre los 35 recursos de `ResourceQrcCatalog::allResources()`, verifica la presencia de cada archivo en el sistema de archivos local (`assets/**`), valida que el tamaño sea estrictamente > 0 bytes y que los permisos de lectura estén concedidos.
2. `test_asset_content_structure()`: Inspección profunda de formato:
   - Valida que `design-tokens/tokens.json` y los 6 temas JSON contengan estructuras JSON sintácticamente válidas y claves obligatorias.
   - Valida que los 22 archivos SVG posean encabezados XML/SVG correctos, `viewBox="0 0 24 24"` y ausencia de scripts maliciosos embebidos.
   - Valida que los 6 fondos de pantalla JPG comiencen con el número mágico JPEG (`0xFF 0xD8 0xFF`).
3. `test_mime_type_consistency()`: Verifica que cada recurso en el catálogo posea un MIME type exactamente correspondiente a su formato (`image/svg+xml`, `application/json`, `image/jpeg`).
4. `test_negative_alias_syntax_fuzzing()`: Somete a `ResourceQrcCatalog::isValidAlias` a un conjunto exhaustivo de cadenas anómalas: secuencias de escape (`../`, `..\`, `..//`, `/..`), barras duplicadas, caracteres en mayúsculas, espacios, caracteres de control, caracteres especiales y extensiones binarias peligrosas (`.exe`, `.sh`).
5. `test_excluded_paths_defense()`: Confirma el bloqueo infalible de rutas de referencia, instaladores, fuentes no licenciadas y rutas absolutas de sistema operativo en `isExcludedPath`, `loadTextResource` y `loadBinaryResource`.
6. `test_synthetic_qrc_manifest_corruption()`: Valida la respuesta del parser ante XML sintético malformado: etiquetas mal anidadas, prefijos erróneos (`/custom`), ausencia de prefijo, alias repetidos y alias no inventariados en el catálogo.
7. `test_critical_resources_isolation_and_remediation()`: Prueba la ausencia individual de cada uno de los 8 recursos críticos, asegurando que `validateStartupResources` retorne `MissingCritical`, identifique el recurso faltante exacto y proporcione instrucciones de remediación claras.
8. `test_fallback_recovery_mechanisms()`: Verifica la auto-reparación determinista:
   - Resolución de iconos inválidos hacia `":/vigilio/icons/tools/tools.svg"`.
   - Resolución de temas inválidos hacia `"themes/light.json"`.
   - Carga con `allowFallback = true` retornando `hasFallback == true` y contenido de emergencia.
   - Carga con `allowFallback = false` retornando `ResourceReadError::NotFound`.
9. `test_cancellation_cooperative()`: Verifica el ciclo de vida de `CancellationToken`, validando el aborto de lecturas binarias antes y durante la operación sin generar fugas de recursos ni bloqueos.
10. `test_high_concurrency_stress_and_thread_safety()`: Ejecución concurrente con 16 hilos y 2000 iteraciones por hilo de consultas continuas de catálogo, validación y formateo diagnóstico, certificando cero data races y estabilidad bajo C++20.

## Archivos afectados

| Archivo | Tipo de cambio | Justificación técnica |
| :--- | :--- | :--- |
| `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp` | Creación | Nueva suite exhaustiva de pruebas unitarias y de integración para certificar casos normales, inválidos, recuperación, integridad de disco y concurrencia de resource qrc inclusion. |
| `apps/vigilio-browser/CMakeLists.txt` | Modificación | Declaración del target ejecutable `vigilio_resource_qrc_inclusion_tests` y registro en CTest como `resource_qrc_inclusion_tests` con etiquetas `unit;fast` y timeout de 15s. |
| `docs/rules/01-modules/foundations/micro/0020-resource-qrc-inclusion/task.md` | Modificación | Sincronización del estado de ejecución de la microtarea a `EXECUTED` tras la fase APPLY con registro de comandos y resultados. |
| `openspec/changes/module-foundations-task0020/**` | Creación | Artefactos OpenSpec formalizados (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`). |

## Contrato UX/UI

- **Declaración formal**: La etapa `tests` **no introduce cambios visuales directos** en la interfaz gráfica de usuario.
- **Verificación de contratos visuales existentes**: La suite de pruebas verifica y protege los contratos visuales aprobados en la microtarea 0019:
  - Garantiza que el icono oficial de la marca (`branding/vigilio-logo.svg`) y el icono de seguridad/aviso (`icons/tools/shield.svg`) se encuentren físicamente íntegros y listos para su pintado en `BrowserWindow`.
  - Asegura que ante cualquier error de resolución en tiempo de ejecución, el mecanismo de fallback provea `":/vigilio/icons/tools/tools.svg"`, evitando botones transparentes o controles en blanco.
  - Verifica que los textos de diagnóstico y error respeten estrictamente `docs/UX-COPY-PUBLICO.md`, prohibiendo términos internos de ingeniería en mensajes públicos y validando la denominación «Interfaz más fácil de usar».

## Reglas y fallos

- **R-001 (Integridad de catálogo)**: Todo recurso expuesto por `ResourceQrcCatalog` debe tener una correspondencia exacta 1:1 con un archivo físico existente en `apps/vigilio-browser/assets/**` y con una entrada válida en `resources.qrc`.
- **R-002 (Determinismo ante entradas inválidas)**: Cualquier alias malformado o ruta de escape debe ser rechazado inmediatamente con error explícito (`InvalidAliasFormat`, `AccessDenied`, `NotFound`), sin invocar operaciones de disco innecesarias.
- **R-003 (Recuperación y resiliencia)**: La ausencia de recursos opcionales o referencias a iconos inexistentes nunca debe abortar o crashear la aplicación; se debe restituir inmediatamente el control mediante el recurso de fallback establecido.
- **R-004 (Pre-validación en arranque)**: La falta de cualquiera de los 8 recursos críticos impide el inicio normal del navegador con estado `ResourceStartupStatus::MissingCritical`, generando un reporte diagnóstico accionable.
- **R-005 (Invariante de concurrencia y thread-safety)**: Todas las operaciones de consulta y lectura de catálogo deben ser seguras para ejecución concurrente desde múltiples hilos sin cerrojos (`lock-free`) y sin efectos secundarios mutables.
- **R-006 (Zero-Zombie y límites de ejecución)**: Las pruebas no deben dejar procesos colgados, temporizadores activos ni descriptores de archivo abiertos; el timeout máximo de ejecución en CTest es de 15 segundos y el tiempo real esperado es < 100 milisegundos.

## Seguridad y privacidad

- **Local-First constitucional (Decisión Q-001)**: Todos los activos residen localmente en el repositorio y se compilan en el ejecutable; cero dependencias de red, cero descargas en segundo plano y cero peticiones de telemetría.
- **Defensa contra Path Traversal y LFI (Local File Inclusion)**: Validación sintáctica rigurosa previa a cualquier operación de apertura de archivos (`isValidAlias`), neutralizando vectores de ataque basados en `../` o rutas absolutas de sistema.
- **Inmutabilidad y protección de memoria**: Los metadatos de recursos y las estructuras de catálogo residen en memoria de solo lectura (`.rodata`), previniendo corrupción de memoria o inyección maliciosa en tiempo de ejecución.
- **Presupuesto de recursos y RAM**: La suite completa se ejecuta consumiendo menos de 5 MB de memoria RAM pico y no realiza asignaciones dinámicas descontroladas en el bucle principal.

## Matriz de pruebas

| Identificador | Criterio de validación | Fixture / Entrada | Comando / Ejecutable | Resultado esperado | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Integridad y presencia física de los 35 recursos en disco | Directorio `assets/**` y catálogo canónico | `ctest -R resource_qrc_inclusion_tests` | 100% de los 35 archivos existen con tamaño > 0 y permisos de lectura | APPLY |
| **TEST-002** | Validación estructural de JSON en tokens y temas | `tokens.json` y los 6 temas en `assets/themes/` | `ctest -R resource_qrc_inclusion_tests` | JSON sintácticamente válido con claves de diseño obligatorias | APPLY |
| **TEST-003** | Validación de especificación vectorial SVG | 22 archivos SVG en `assets/` | `ctest -R resource_qrc_inclusion_tests` | XML bien formado, `viewBox="0 0 24 24"`, sin `<script>` | APPLY |
| **TEST-004** | Validación de números mágicos JPEG en wallpapers | 6 archivos JPG en `assets/wallpapers/` | `ctest -R resource_qrc_inclusion_tests` | Bytes iniciales coinciden con `0xFF 0xD8 0xFF` | APPLY |
| **TEST-005** | Fuzzing y rechazo de sintaxis de alias anómalos | Cadenas vacías, traversal `../`, mayúsculas, espacios, símbolos | `ctest -R resource_qrc_inclusion_tests` | `isValidAlias` retorna false para 100% de entradas inválidas | APPLY |
| **TEST-006** | Bloqueo estricto de rutas excluidas del empaquetado | `reference/`, `installer/`, `fonts/`, rutas absolutas | `ctest -R resource_qrc_inclusion_tests` | `isExcludedPath` retorna true; carga denegada con `AccessDenied` | APPLY |
| **TEST-007** | Corrupción sintáctica de manifiestos QRC sintéticos | XML con prefijo `/custom`, sin prefijo, duplicados, no catalogados | `ctest -R resource_qrc_inclusion_tests` | Detección determinista con estados de corrupción correspondientes | APPLY |
| **TEST-008** | Detección de falta de recursos críticos y reporte | Manifiestos omitiendo individualmente cada uno de los 8 críticos | `ctest -R resource_qrc_inclusion_tests` | Retorna `MissingCritical` listando el activo faltante exacto y remediación | APPLY |
| **TEST-009** | Recuperación y fallback de iconos y temas | Solicitud de iconos y temas desconocidos o malformados | `ctest -R resource_qrc_inclusion_tests` | Retorna deterministamente `tools.svg` y `light.json` respectivamente | APPLY |
| **TEST-010** | Carga de texto con fallback habilitado vs deshabilitado | Lectura de recurso inexistente con `allowFallback = true` y `false` | `ctest -R resource_qrc_inclusion_tests` | `hasFallback == true` con datos en fallback; `NotFound` sin fallback | APPLY |
| **TEST-011** | Cancelación cooperativa de lecturas binarias | `CancellationToken` pre-cancelado y cancelado durante lectura | `ctest -R resource_qrc_inclusion_tests` | Retorna `ResourceReadError::Cancelled` sin fugas de descriptores | APPLY |
| **TEST-012** | Concurrencia masiva multihilo libre de data races | 16 hilos simultáneos, 2000 iteraciones por hilo | `ctest -R resource_qrc_inclusion_tests` | 100% de operaciones exitosas, cero data races bajo TSan, tiempo < 100 ms | APPLY |
| **TEST-013** | Pruebas de renderizado de iconos en widgets Qt | Instanciación de `BrowserWindow` y verificación de `iconSize` | `ctest -R window_ui_tests` | Iconos no nulos y escalado 22x22 a 32x32 verificado | LATER |
| **TEST-014** | Smoke E2E de sesión completa con Chromium | Flujo completo de navegación con WebEngine | `ctest -R browser_e2e_smoke` | Ejecución serial bajo demanda (`VIGILIO_ENABLE_E2E=ON`) | LATER |
| **TEST-015** | Integración con servicios de red o nube | N/A (prohibido por principios arquitectónicos) | N/A | N/A (local-first constitucional) | NOT_APPLICABLE |

## Pasos APPLY

1. **Creación de la suite focalizada de pruebas en `apps/vigilio-browser/tests/unit/ResourceQrcInclusionTests.cpp`**:
   - Implementar el macro de aserción determinista `CHECK(condition)`.
   - Implementar módulos de prueba: integridad de archivos físicos en disco, verificación profunda de formatos (JSON, SVG, JPEG), fuzzing negativo de alias, exclusión de rutas, corrupción de manifiestos sintéticos, aislamiento de recursos críticos, fallbacks y recuperación, cancelación cooperativa y concurrencia multihilo.
2. **Actualización de la configuración de compilación en `apps/vigilio-browser/CMakeLists.txt`**:
   - Declarar el ejecutable de prueba `vigilio_resource_qrc_inclusion_tests` enlazado con `vigilio_core` y `vigilio_compiler_warnings`.
   - Definir macros de ruta `VIGILIO_ASSETS_DIR` y `VIGILIO_RESOURCES_QRC_PATH`.
   - Registrar la prueba en CTest: `add_test(NAME resource_qrc_inclusion_tests COMMAND vigilio_resource_qrc_inclusion_tests)` con propiedades `LABELS "unit;fast"` y `TIMEOUT 15`.
3. **Ejecución y validación continua local con `check.sh`**:
   - Compilar en modo headless (`-DVIGILIO_BUILD_GUI=OFF`) con Ninja.
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` verificando el paso del 100% de las pruebas (8/8 pruebas con la nueva incorporación) en menos de 0.2 segundos con exit code 0.
   - Auditar formato de código con Clang-Format asegurando cero desviaciones de estilo.
4. **Sincronización documental canónica**:
   - Actualizar el estado de la microtarea en `docs/rules/01-modules/foundations/micro/0020-resource-qrc-inclusion/task.md` de `TARGET; NOT_EXECUTED` a `EXECUTED`.
5. **Certificación con herramientas de Prompt Builder**:
   - Validar referencias con `validate-references` certificando `valid: true`.
   - Validar el lockfile correspondiente con `verify-lock`.

## Definición de terminado

- [ ] Propuesta OpenSpec formalizada en `openspec/changes/module-foundations-task0020/{proposal.md,design.md,tasks.md,execution-memory.md}` con estado `PROPOSED` y referencias canónicas validadas.
- [ ] La suite de pruebas automatizadas `resource_qrc_inclusion_tests` cubre exhaustivamente casos positivos, negativos, integridad física en disco, estructura de contenidos, recuperación por fallback, cancelación y estrés concurrente.
- [ ] 100% de las pruebas unitarias y de integración pasan con éxito en CTest (`apps/vigilio-browser/scripts/check.sh`) con exit code 0.
- [ ] Formato de código verificado con Clang-Format sin advertencias ni diferencias.
- [ ] Aislamiento garantizado: cero llamadas de red externa, cero procesos zombis de Chromium iniciados, cero archivos temporales residuales.
- [ ] Respeto absoluto a la decisión canónica Q-001 (local-first) y al manual de estilo `docs/UX-COPY-PUBLICO.md`.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0019`: Diseñó e implementó la integración visual de iconos y accesibilidad en `BrowserWindow`.
  - Cadena de herramientas C++20 estándar (GCC 15+, Clang 18+, MSVC 2022+), CMake >= 3.24, Ninja >= 1.12.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Inestabilidad o falsos negativos en pruebas multihilo por contención de recursos en entornos CI limitados.
    *Mitigación*: Las consultas sobre el catálogo y los métodos de servicio operan sobre estructuras inmutables en memoria de solo lectura (`.rodata`), garantizando un comportamiento totalmente libre de bloqueos (`lock-free`).
  - *Riesgo*: Rutas relativas inconsistentes al acceder a `assets/` o `resources.qrc` desde diferentes directorios de trabajo de ejecución de pruebas.
    *Mitigación*: Inyección determinista de macros de compilación (`VIGILIO_ASSETS_DIR`, `VIGILIO_RESOURCES_QRC_PATH`) mediante CMake con rutas absolutas canónicas, complementadas con resolución fallback relativa controlada.
  - *Riesgo*: Dependencia inadvertida de servidores gráficos o display en entornos de integración continua headless.
    *Mitigación*: La suite `ResourceQrcInclusionTests.cpp` forma parte del núcleo desacoplado `vigilio_core`, operando puramente sobre la lógica de C++20 estándar sin instanciar componentes de ventana ni requerir X11/Wayland.

## Commit propuesto

`test(foundations): tests resource-qrc-inclusion`
