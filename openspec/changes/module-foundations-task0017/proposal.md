# Change Proposal — module-foundations-task0017

Pack: `module.foundations.task0017`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **resource qrc inclusion** en el módulo propietario `foundations` de Vigilio Browser. Habiendo formalizado en las microtareas precedentes (`module.foundations.task0009` a `task0016`) la validación de dependencias Qt (Qt >= 6.10.0 según la decisión Q-002 de `docs/rules/answers.md`) y la compatibilidad de versiones de Chromium embebido (Chromium milestone >= 128), esta microtarea formaliza el modelo de dominio, las entidades de valor, las invariantes semánticas y el catálogo canónico del sistema de inclusión de recursos compilados de Qt (`Qt Resource System` / `.qrc`).

El alcance concreto de este pack comprende:
- **Taxonomía de categorías de recursos embebidos (`ResourceCategory`)**:
  - `Theme`: Esquemas de color, superficies y variables de diseño en formato JSON (`themes/light.json`, `dark.json`, `midnight.json`, `forest.json`, `aurora.json`, `sunset.json`).
  - `Wallpaper`: Ilustraciones y fotografías de paisajes locales para pantalla de inicio en formato JPEG (`wallpapers/vigilio-landscape-01.jpg` a `06.jpg`).
  - `Branding`: Identidad visual del navegador y logotipo oficial vectorizado (`branding/vigilio-logo.svg`).
  - `DesignTokens`: Especificación formal de tokens de espaciado, tipografía, bordes y radios (`design-tokens/tokens.json`).
  - `NavigationIcon`: Iconografía funcional de la barra de navegación en formato SVG (`search`, `forward`, `settings`, `download`, `history`, `tabs`, `home`, `reload`, `back`, `bookmarks`).
  - `ToolIcon`: Iconografía de utilidades y herramientas integradas en formato SVG (`note`, `mcp`, `reader`, `camera`, `senior`, `tools`, `memory`, `extensions`, `split`, `qr`, `shield`).
- **Entidad de valor inmutable `ResourceDescriptor`**:
  - `alias`: Identificador canónico y relativo dentro del bundle (e.g. `branding/vigilio-logo.svg`, `icons/navigation/search.svg`).
  - `qrcPath`: Ruta virtual canónica con prefijo del sistema Qt (`:/vigilio/branding/vigilio-logo.svg`).
  - `physicalAssetPath`: Ruta física relativa en el árbol de fuentes (`assets/branding/vigilio-logo.svg`).
  - `category`: Categoría tipada (`ResourceCategory`).
  - `mimeType`: Tipo MIME canónico (`image/svg+xml`, `application/json`, `image/jpeg`).
  - `isCritical`: Booleano que señala si el recurso es indispensable para el arranque e inicialización básica de la interfaz.
- **Catálogo canónico de dominio (`ResourceQrcCatalog`) en `vigilio_core`**:
  - Catálogo inmutable y cerrado que inventaría con exactitud los **35 recursos aprobados** expuestos actualmente por `apps/vigilio-browser/resources.qrc` bajo el prefijo canónico `/vigilio`.
  - Métodos puros en C++20:
    - `canonicalPrefix()`: Retorna `":/vigilio"`.
    - `totalResourceCount()`: Retorna `35`.
    - `contains(std::string_view alias)`: Verificación de presencia en O(1) / lookup determinista.
    - `find(std::string_view alias)`: Retorna `std::optional<ResourceDescriptor>`.
    - `resolveQrcPath(std::string_view alias)`: Resuelve la ruta virtual `":/vigilio/" + alias`.
    - `isValidAlias(std::string_view alias)`: Validación sintáctica de alias.
    - `resourcesByCategory(ResourceCategory category)`: Lista inmutable de recursos filtrados por categoría.
    - `criticalResources()`: Lista inmutable de recursos marcados como críticos para el arranque.
    - `validateQrcConsistency(std::string_view qrcXmlContent)`: Validación estática de estructura XML y correspondencia 1:1 con el catálogo de dominio.
- **Taxonomía de resultados y fallos de validación (`ResourceInclusionStatus`)**:
  - `Valid`: Recurso o catálogo plenamente conforme y consistente.
  - `InvalidAliasFormat`: El alias contiene caracteres ilegales, secuencias de escape no válidas, barras duplicadas o intentos de salto de directorio (`../`).
  - `ResourceNotFound`: El alias solicitado no forma parte del catálogo canónico aprobado.
  - `CategoryMismatch`: La ruta del alias no corresponde a la convención estructural de su categoría.
  - `PrefixMismatch`: El archivo XML de recursos utiliza un prefijo distinto de `/vigilio`.
  - `DuplicateAlias`: Se detecta un alias repetido dentro del mismo archivo de recursos.
  - `MissingCriticalResource`: Uno o más recursos indispensables para el arranque están ausentes en el catálogo o en el archivo `.qrc`.
  - `ExcludedPathIncluded`: Intento de incluir rutas expresamente prohibidas (tales como `reference/`, `installer/` o `fonts/`).
- **Invariantes arquitectónicas y políticas de inclusión**:
  - *Prefijo único inmutable*: Todo recurso del navegador se expone exclusivamente bajo el espacio de nombres virtual `:/vigilio/`.
  - *Local-First absoluto (Decisión canónica Q-001)*: Todo recurso visual o de configuración reside localmente embebido en el binario; prohibida cualquier dependencia de CDNs remotas, telemetría o descarga de assets por red.
  - *Reglas de exclusión estricta*: Las carpetas `assets/reference/` (maquetas e infografías), `assets/installer/` (recursos exclusivos del asistente de instalación desacoplado) y `assets/fonts/` (fuentes sujetas a revisión de licenciamiento legal) quedan expresamente excluidas del compilador de recursos `resources.qrc` del navegador.
  - *Estándar de calidad SVG*: Todos los iconos deben cumplir con la especificación de `docs/skills/11-assets-svg-quality.md` (viewBox `0 0 24 24`, trazo 1.8 px, terminaciones redondeadas, ausencia total de scripts o entidades externas).
  - *Desacoplamiento de C++20*: Las definiciones de dominio residen en `vigilio_core`, permitiendo su compilación y testeo unitario en entornos headless sin necesidad de inicializar `QApplication` ni enlazar con bibliotecas gráficas pesadas.

## Out of scope

- Implementación del validador y catálogo en código fuente C++ (`src/core/ResourceQrcCatalog.cpp`), diferida a la etapa `implementation` en `module.foundations.task0018`.
- Carga de imágenes en memoria (`QIcon`, `QPixmap`) y aplicación de hojas de estilo o tokens en widgets de `BrowserWindow`, diferida a la etapa `ui` en `module.foundations.task0019`.
- Creación de arneses de prueba automatizados y suites CTest para recursos QRC, diferida a la etapa `tests` en `module.foundations.task0020`.
- Compilación del binario instalador o inclusión de assets de empaquetado (`assets/installer/**`), reservado a las capacidades de empaquetado e instalador (`module.foundations.task0053` a `task0056` y módulo `release`).
- Modificación de archivos fuente en `apps/**` o reglas en `docs/rules/**` durante esta fase PROPOSE (restringida a `openspec/changes/module-foundations-task0017/**`).
- Cualquier intento de conexión a redes externas o servicios en la nube para actualizar recursos (prohibido constitucionalmente).

## Contexto y evidencia

Se inspeccionó rigurosamente el estado actual del repositorio, los archivos de recursos, la configuración de compilación y las herramientas locales de desarrollo:
1. **Archivo de recursos existente (`apps/vigilio-browser/resources.qrc`)**:
   - Define el bloque `<qresource prefix="/vigilio">` conteniendo exactamente 35 elementos `<file alias="...">assets/...</file>`.
   - Distribución canónica de los 35 elementos:
     - 6 temas: `themes/sunset.json`, `light.json`, `dark.json`, `aurora.json`, `forest.json`, `midnight.json`.
     - 6 fondos: `wallpapers/vigilio-landscape-01.jpg` a `vigilio-landscape-06.jpg`.
     - 1 branding: `branding/vigilio-logo.svg`.
     - 1 tokens: `design-tokens/tokens.json`.
     - 10 iconos de navegación: `icons/navigation/search.svg`, `forward.svg`, `settings.svg`, `download.svg`, `history.svg`, `tabs.svg`, `home.svg`, `reload.svg`, `back.svg`, `bookmarks.svg`.
     - 11 iconos de herramientas: `icons/tools/note.svg`, `mcp.svg`, `reader.svg`, `camera.svg`, `senior.svg`, `tools.svg`, `memory.svg`, `extensions.svg`, `split.svg`, `qr.svg`, `shield.svg`.
2. **Árbol de assets físico (`apps/vigilio-browser/assets/`)**:
   - Se verificó que los 35 archivos mapeados en `resources.qrc` existen físicamente en sus rutas respectivas bajo `apps/vigilio-browser/assets/` y cuentan con permisos regulares de lectura.
   - El archivo `apps/vigilio-browser/assets/README.md` explicita la regla de oro: la única ruta canónica es `assets/`; los archivos bajo `reference/` son maquetas y no deben compilarse en QRC; `resources.qrc` expone recursos a `:/vigilio/...`; y las fuentes bajo `fonts/` no deben distribuirse sin revisión jurídica de licencias.
3. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Configura `CMAKE_AUTORCC ON` cuando `Qt6` y `VIGILIO_BUILD_GUI=ON` están presentes (líneas 163-164).
   - Enlaza `resources.qrc` a `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke`.
   - Sin embargo, `vigilio_core` no cuenta con un modelo de dominio formal para inspeccionar, validar y catalogar los recursos QRC de forma desacoplada antes de que la GUI intente acceder a ellos, creando una brecha entre la definición física del archivo XML y la arquitectura de software.
4. **Validación de entorno y herramientas locales**:
   - Compilador: GNU GCC 15.2.0 (`-std=c++20`).
   - CMake: versión 4.2.3 con Ninja 1.13.2.
   - Pipeline de verificación continua (`bash apps/vigilio-browser/scripts/check.sh`): 5/5 pruebas unitarias e integración pasando al 100% (exit code 0).
   - Formateo: Clang-Format sin discrepancias en el código existente.
   - Prompt Builder CLI (`/config/Desktop/auto/prompt-builder/dist/cli.js`):
     - `validate-references --project-root .`: 17/17 referencias canónicas con estado `valid: true` (exit code 0).
     - `verify-lock --lock-file generated/prompts/full-project/0049-module.foundations.task0017.propose.lock.json --project-root .`: 14/14 comprobaciones aprobadas con estado `valid: true` (exit code 0).
     - `validate-decisions`: Confirma que Q-001 a Q-006 y Q-008 están resueltas; Q-007 solo bloquea el hito final de lanzamiento público.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0017`.
- **Capacidad**: `resource qrc inclusion`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0017/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0017/proposal.md`
  - `openspec/changes/module-foundations-task0017/design.md`
  - `openspec/changes/module-foundations-task0017/tasks.md`
  - `openspec/changes/module-foundations-task0017/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0017/**`. Queda terminantemente prohibido modificar `apps/**` o `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

El diseño técnico integral de **resource qrc inclusion** para la etapa **domain** se articula en torno a los siguientes componentes arquitectónicos en el espacio de nombres `vigilio`:
1. **Enumeraciones y entidades de valor (`ResourceTypes.h`)**:
   - `enum class ResourceCategory : uint8_t { Theme, Wallpaper, Branding, DesignTokens, NavigationIcon, ToolIcon };`
   - `enum class ResourceInclusionStatus : uint8_t { Valid, InvalidAliasFormat, ResourceNotFound, CategoryMismatch, PrefixMismatch, DuplicateAlias, MissingCriticalResource, ExcludedPathIncluded };`
   - `struct ResourceDescriptor`:
     - `std::string_view alias`: Identificador relativo canónico (e.g. `"icons/navigation/search.svg"`).
     - `std::string_view qrcPath`: Ruta virtual completa (e.g. `":/vigilio/icons/navigation/search.svg"`).
     - `std::string_view physicalAssetPath`: Ruta física relativa en el árbol de desarrollo (e.g. `"assets/icons/navigation/search.svg"`).
     - `ResourceCategory category`: Clasificación semántica.
     - `std::string_view mimeType`: Tipo MIME asociado (`"image/svg+xml"`, `"application/json"`, `"image/jpeg"`).
     - `bool isCritical`: Indicador de criticidad para inicialización mínima de UI.
     - Operadores de comparación `operator==` y ordenamiento `operator<=>` para posibilitar búsquedas binarias eficientes en colecciones ordenadas en tiempo de compilación.
2. **Catálogo canónico inmutable (`ResourceQrcCatalog.h`)**:
   - Proveedor de dominio desacoplado de Qt Widgets, basado en tablas estáticas inmutables (`std::array` de `ResourceDescriptor` con evaluación `constexpr` o inicialización determinista).
   - Constantes de dominio:
     - `kCanonicalPrefix`: `":/vigilio"`.
     - `kExpectedResourceCount`: `35`.
   - Métodos públicos del catálogo:
     - `static constexpr std::string_view canonicalPrefix() noexcept;`
     - `static constexpr size_t totalResourceCount() noexcept;`
     - `static bool contains(std::string_view alias) noexcept;`
     - `static std::optional<ResourceDescriptor> find(std::string_view alias) noexcept;`
     - `static std::string resolveQrcPath(std::string_view alias);`
     - `static bool isValidAlias(std::string_view alias) noexcept;`
     - `static std::vector<ResourceDescriptor> resourcesByCategory(ResourceCategory category);`
     - `static std::vector<ResourceDescriptor> criticalResources();`
     - `static ResourceInclusionStatus validateQrcConsistency(std::string_view qrcXmlContent);`
     - `static std::string_view categoryName(ResourceCategory category) noexcept;`
     - `static std::string_view statusName(ResourceInclusionStatus status) noexcept;`
3. **Reglas de validación sintáctica de alias**:
   - Un alias es válido si:
     - No está vacío.
     - No comienza ni termina con una barra diagonal `/`.
     - No contiene barras diagonales duplicadas `//`.
     - No contiene secuencias de salto de directorio `..` o `../` o `/..`.
     - Contiene únicamente caracteres alfanuméricos ASCII en minúscula, guiones medios `-`, guiones bajos `_`, puntos `.` y barras simples `/`.
     - Posee una extensión soportada según su categoría (`.svg`, `.json`, `.jpg`).
4. **Reglas de exclusión de rutas no autorizadas**:
   - El validador detecta y rechaza expresamente cualquier intento de referenciar rutas que contengan:
     - `assets/reference/` o `reference/` (maquetas gráficas descartadas de instalación).
     - `assets/installer/` o `installer/` (recursos del empaquetador del instalador).
     - `assets/fonts/` o `fonts/` (fuentes sin licencia auditada).
     - Rutas absolutas del sistema de archivos (`/usr/`, `C:\`, `/tmp/`).
5. **Inventario cerrado de los 35 recursos canónicos**:
   - Registro exhaustivo de los 6 temas, 6 wallpapers, 1 branding (`vigilio-logo.svg`), 1 design tokens (`tokens.json`), 10 iconos de navegación y 11 iconos de herramientas.
   - Recursos catalogados como `isCritical = true`:
     - `branding/vigilio-logo.svg` (identidad esencial).
     - `design-tokens/tokens.json` (mecanismo base de diseño y escalado).
     - `themes/light.json` y `themes/dark.json` (temas fundamentales del sistema).
     - `icons/navigation/search.svg`, `icons/navigation/back.svg`, `icons/navigation/forward.svg`, `icons/navigation/reload.svg` (navegación básica mínima).

## Archivos afectados

Durante esta fase PROPOSE, la escritura está rigurosamente limitada a la raíz OpenSpec del cambio:
- `openspec/changes/module-foundations-task0017/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0017/proposal.md`
- `openspec/changes/module-foundations-task0017/design.md`
- `openspec/changes/module-foundations-task0017/tasks.md`
- `openspec/changes/module-foundations-task0017/execution-memory.md`

En la fase futura APPLY (tras aprobación humana en `module.foundations.task0018`):
- `apps/vigilio-browser/src/core/ResourceTypes.h`: Declaración de tipos, enumeraciones y descriptores de recursos.
- `apps/vigilio-browser/src/core/ResourceQrcCatalog.h`: Interfaz del catálogo de dominio de recursos QRC.
- `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`: Implementación de la tabla canónica, búsquedas y validador de consistencia.
- `apps/vigilio-browser/tests/unit/ResourceQrcCatalogTests.cpp`: Suite de pruebas unitarias de especificación de dominio en C++20.
- `apps/vigilio-browser/CMakeLists.txt`: Incorporación de los nuevos archivos fuente en la biblioteca estática `vigilio_core` y registro del nuevo test en CTest.

## Contrato UX/UI

- **Declaración formal**: **Sin cambio visual directo**.
- **Justificación**: Esta microtarea pertenece a la etapa `domain` del ciclo de desarrollo y formaliza el modelo conceptual, catálogo y validadores del sistema de recursos QRC en la capa de lógica interna (`vigilio_core`). No instancia widgets de Qt, no altera `BrowserWindow` y no renderiza imágenes en pantalla.
- **Preservación y coherencia con la visión de diseño objetivo**:
  - Garantiza la disponibilidad e integridad de los recursos necesarios para construir la experiencia visual canónica detallada en:
    - `/resultado/01-vigilio-home.png`: Sidebar con iconos vectoriales Lucide, buscador central y tarjetas informativas.
    - `/resultado/02-vigilio-tools-productivity.png`: Iconografía de herramientas especializadas (`reader`, `note`, `camera`, `memory`, `split`, `shield`).
    - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz accesible con «Interfaz más fácil de usar», conmutadores ampliados y tokens de contraste adecuados.
    - `/resultado/06-logo-instalador-oficial.png`: Referencia oficial para la marca e identidad gráfica del producto (globo terráqueo con órbita continua estilizada).
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Ningún identificador interno de recurso, término de ingeniería («QRC», «RCC», «viewBox», «DOM», «Prompt Builder», «DAG», «OpenSpec», «Senior Mode») se expone en la interfaz de usuario.
  - La iconografía y los temas catalogados alimentarán exclusivamente textos legibles y accesibles en español («Interfaz más fácil de usar», «Ahorro de memoria», «Protección contra anuncios y rastreadores»).

## Reglas y fallos

- **R-001 (Espacio de nombres y prefijo canónico)**: Todo recurso del navegador debe registrarse bajo el prefijo virtual `/vigilio`. Cualquier archivo `.qrc` que declare un prefijo diferente es clasificado como `PrefixMismatch`.
- **R-002 (Inmutabilidad del catálogo canónico)**: El inventario de recursos aprobados consta de exactamente 35 elementos. La introducción de nuevos recursos requiere actualizar el catálogo formal de dominio y documentar su procedencia y licencia.
- **R-003 (Rechazo estricto de Directory Traversal)**: Cualquier alias que contenga secuencias `..` o componentes que intenten acceder a rutas superiores al bundle virtual debe ser rechazado inmediatamente con `InvalidAliasFormat`.
- **R-004 (Prohibición de rutas excluidas)**: Ningún recurso procedente de `assets/reference/`, `assets/installer/` o `assets/fonts/` puede ser incluido en el catálogo del navegador o en el archivo `resources.qrc`. Su detección emite `ExcludedPathIncluded`.
- **R-005 (Presencia obligatoria de recursos críticos)**: El catálogo debe garantizar la presencia de los recursos identificados como críticos (`branding/vigilio-logo.svg`, `design-tokens/tokens.json`, temas `light`/`dark` y los 4 iconos de navegación esenciales). Si alguno falta, la validación falla con `MissingCriticalResource`.
- **R-006 (Formato e integridad SVG)**: Todo icono clasificado como `NavigationIcon` o `ToolIcon` debe poseer extensión `.svg`, tipo MIME `image/svg+xml`, viewBox `0 0 24 24`, trazo 1.8 px y ausencia absoluta de etiquetas `<script>` o entidades XML externas.
- **R-007 (Pureza y concurrencia)**: Los métodos del catálogo son funciones libres de efectos secundarios, thread-safe y ejecutables simultáneamente desde múltiples hilos sin locks ni contención de memoria.
- **R-008 (Filosofía Local-First)**: Prohibida toda carga remota o descarga de activos por red (Decisión Q-001). Todo activo requerido debe ser provisto de forma local y pre-empaquetada en el binario.

## Seguridad y privacidad

- **Aislamiento de recursos locales**: Los recursos compilados en QRC forman parte del segmento de solo lectura (`.rodata`) del ejecutable; son inmunes a manipulaciones en tiempo de ejecución o ataques de sustitución de archivos en disco.
- **Prevención de vulnerabilidades XML/SVG (XXE / XSS)**:
  - Los archivos SVG se validan sintácticamente garantizando la ausencia de scripts embebidos (`<script>`), atributos de eventos (`onload`, `onclick`) o llamadas a entidades externas (XXE), evitando vectores de ataque al renderizar iconos.
- **Privacidad absoluta**:
  - Los recursos no contienen balizas de seguimiento, peticiones de telemetría ni identificadores únicos de usuario.
  - Ninguna operación de lectura o validación de recursos interactúa con el almacenamiento de historial ni deja trazas en bases de datos SQLite.
- **Presupuesto de memoria y rendimiento**:
  - El catálogo de dominio en `vigilio_core` ocupa menos de 20 KB de memoria estática.
  - Las consultas de existencia y búsqueda por alias operan en tiempo constante O(1) o logarítmico determinista sin asignaciones dinámicas en el montón (`heap`).

## Matriz de pruebas

| Identificador | Criterio de validación | Fixture / Entrada | Comando / Ejecutable | Resultado esperado | Clasificación |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Totalidad e integridad del catálogo canónico | Consulta a `ResourceQrcCatalog::totalResourceCount()` | `ctest -R resource_qrc_catalog_tests` | Retorna exactamente 35 recursos registrados | APPLY |
| **TEST-002** | Consulta de recursos válidos por alias | `"branding/vigilio-logo.svg"`, `"design-tokens/tokens.json"`, `"icons/navigation/search.svg"` | `ctest -R resource_qrc_catalog_tests` | `contains` retorna true; `find` retorna `ResourceDescriptor` con metadatos exactos | APPLY |
| **TEST-003** | Resolución canónica de rutas virtuales QRC | `"themes/midnight.json"`, `"icons/tools/shield.svg"` | `ctest -R resource_qrc_catalog_tests` | `resolveQrcPath` genera `":/vigilio/themes/midnight.json"` y `":/vigilio/icons/tools/shield.svg"` | APPLY |
| **TEST-004** | Rechazo de alias no existentes en catálogo | `"icons/unknown.svg"`, `"themes/nonexistent.json"`, `"malicious/payload.exe"` | `ctest -R resource_qrc_catalog_tests` | `contains` retorna false; `find` retorna `std::nullopt` | APPLY |
| **TEST-005** | Validación sintáctica y rechazo de Directory Traversal | `"../etc/passwd"`, `"icons/../../secret"`, `"/branding/logo.svg"`, `"themes//light.json"` | `ctest -R resource_qrc_catalog_tests` | `isValidAlias` retorna false; validador emite `InvalidAliasFormat` | APPLY |
| **TEST-006** | Filtrado correcto de recursos por categoría | Categorías `Theme`, `Wallpaper`, `NavigationIcon`, `ToolIcon` | `ctest -R resource_qrc_catalog_tests` | Conteo exacto: 6 temas, 6 wallpapers, 10 nav icons, 11 tool icons | APPLY |
| **TEST-007** | Verificación de lista de recursos críticos | Consulta a `ResourceQrcCatalog::criticalResources()` | `ctest -R resource_qrc_catalog_tests` | Retorna descriptores de logo, tokens, temas light/dark e iconos de navegación esenciales | APPLY |
| **TEST-008** | Validación de estructura XML de `resources.qrc` | Contenido real de `apps/vigilio-browser/resources.qrc` | `ctest -R resource_qrc_catalog_tests` | `validateQrcConsistency` retorna `ResourceInclusionStatus::Valid` | APPLY |
| **TEST-009** | Detección de prefijo incorrecto en QRC | Documento XML sintético con `<qresource prefix="/other">` | `ctest -R resource_qrc_catalog_tests` | `validateQrcConsistency` retorna `PrefixMismatch` | APPLY |
| **TEST-010** | Detección de inclusión de rutas prohibidas | Documento XML sintético incluyendo `assets/reference/mockups/...` o `installer/...` | `ctest -R resource_qrc_catalog_tests` | `validateQrcConsistency` retorna `ExcludedPathIncluded` | APPLY |
| **TEST-011** | Concurrencia multihilo y seguridad de lectura | 16 hilos consultando simultáneamente `find` y `contains` en 2000 iteraciones | `ctest -R resource_qrc_catalog_tests` | 100% de consultas concurrentes exitosas, cero data races, latencia < 0.1 ms | APPLY |
| **TEST-012** | Carga real de pixmaps y renderizado en widgets Qt | Instanciación de `BrowserWindow` y carga de `QIcon(":/vigilio/icons/navigation/search.svg")` | `ctest -R window_ui_tests` | Carga exitosa en GUI con tamaño escalado según modo | LATER |
| **TEST-013** | Validación de assets de instalador en paquete final | Despliegue de instalador en paquete binario | Script de release / empaquetado | Inclusión de assets propios del instalador | NOT_APPLICABLE |

## Pasos APPLY

1. **Creación de tipos y descriptores de valor en `apps/vigilio-browser/src/core/ResourceTypes.h`**:
   - Definir `ResourceCategory`, `ResourceInclusionStatus` y el struct inmutable `ResourceDescriptor` con operadores de igualdad y ordenamiento.
2. **Definición de la interfaz del catálogo en `apps/vigilio-browser/src/core/ResourceQrcCatalog.h`**:
   - Declarar la clase pura `ResourceQrcCatalog` con métodos estáticos para consulta, validación de alias y consistencia de QRC.
3. **Implementación del catálogo canónico en `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`**:
   - Codificar la tabla estática inmutable conteniendo los 35 descriptores canónicos de `resources.qrc`.
   - Implementar el algoritmo de validación de sintaxis de alias y el parser estático de consistencia XML.
4. **Implementación de la suite de pruebas unitarias en `apps/vigilio-browser/tests/unit/ResourceQrcCatalogTests.cpp`**:
   - Codificar pruebas deterministas en C++20 que verifiquen conteo de recursos, búsquedas, filtrado por categoría, validación de alias, rechazo de traversals y concurrencia multihilo.
5. **Actualización de `apps/vigilio-browser/CMakeLists.txt`**:
   - Añadir `src/core/ResourceQrcCatalog.cpp` a la biblioteca estática `vigilio_core`.
   - Registrar el ejecutable `vigilio_resource_catalog_tests` y el test de CTest `resource_qrc_catalog_tests` bajo `LABELS "unit;fast"`.
6. **Ejecución y verificación del pipeline local**:
   - Compilar con Ninja y ejecutar `bash apps/vigilio-browser/scripts/check.sh` verificando 100% de tests pasando en CTest con exit code 0.
   - Validar formato de código con `clang-format --dry-run --Werror`.
7. **Certificación documental y de referencias canónicas**:
   - Ejecutar `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` y certificar estado `valid: true`.

## Definición de terminado

- [ ] Propuesta OpenSpec formalizada en `openspec/changes/module-foundations-task0017/{proposal.md,design.md,tasks.md,execution-memory.md}` con estado `PROPOSED` y referencias canónicas validadas.
- [ ] Especificación formal del catálogo de 35 recursos aprobados, delimitando rigurosamente categorías, criticidad, tipos MIME y rutas QRC.
- [ ] Reglas estrictas de validación de alias y exclusión de rutas no autorizadas (`reference/`, `installer/`, `fonts/`) documentadas y trazadas a pruebas unitarias.
- [ ] Desacoplamiento arquitectónico garantizado en `vigilio_core`, permitiendo compilación y validación en modo headless sin dependencias de GUI ni WebEngine.
- [ ] Respeto absoluto a la decisión Q-001 (filosofía local-first) y al manual de estilo `docs/UX-COPY-PUBLICO.md`.
- [ ] Pipeline local y herramientas de Prompt Builder (`validate-references`, `verify-lock`) ejecutados con resultado limpio (exit code 0).

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0016`: Finalizó la suite de pruebas unitarias y de estrés de compatibilidad de WebEngine.
  - Cadena de herramientas C++20 (GCC 15+, Clang 18+, MSVC 2022+), CMake >= 3.24, Ninja >= 1.12.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Desincronización entre el archivo `apps/vigilio-browser/resources.qrc` en disco y el catálogo de dominio `ResourceQrcCatalog` en C++.
    *Mitigación*: Se diseña la función `validateQrcConsistency` para validar programáticamente en pruebas unitarias que todo alias en `resources.qrc` exista en el catálogo de dominio y viceversa.
  - *Riesgo*: Inclusión accidental de maquetas pesadas de `assets/reference/mockups/` en el bundle compilado, incrementando el tamaño del ejecutable.
    *Mitigación*: Regla estricta de exclusión R-004 validada en tiempo de compilación y en pruebas unitarias que rechaza cualquier ruta que contenga `reference/`.
  - *Riesgo*: Dependencias de inicialización de `QApplication` en entornos de servidor o CI headless.
    *Mitigación*: El modelo de dominio en `vigilio_core` opera exclusivamente con tipos estándar de C++20 (`std::string_view`, `std::array`, `std::optional`), sin llamadas a la API de Qt Core o Qt GUI en la capa de especificación.

## Commit propuesto

`docs(foundations): domain resource-qrc-inclusion`
