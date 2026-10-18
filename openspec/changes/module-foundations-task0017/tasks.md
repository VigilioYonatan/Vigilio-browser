# Tasks — module-foundations-task0017

Status: PROPOSED

- [ ] TASK-001: Definir los tipos de datos de valor, enumeraciones y descriptores inmutables de recursos QRC en `apps/vigilio-browser/src/core/ResourceTypes.h`
  - *Criterio de verificación*: Declaración de `ResourceCategory` (`Theme`, `Wallpaper`, `Branding`, `DesignTokens`, `NavigationIcon`, `ToolIcon`), `ResourceInclusionStatus` (`Valid`, `InvalidAliasFormat`, `ResourceNotFound`, `CategoryMismatch`, `PrefixMismatch`, `DuplicateAlias`, `MissingCriticalResource`, `ExcludedPathIncluded`), y `struct ResourceDescriptor` con `operator==` y `operator<=>`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ResourceTypes.h && grep -E "enum class ResourceCategory|struct ResourceDescriptor" apps/vigilio-browser/src/core/ResourceTypes.h`

- [ ] TASK-002: Declarar la interfaz pública desacoplada del catálogo canónico de recursos QRC en `apps/vigilio-browser/src/core/ResourceQrcCatalog.h`
  - *Criterio de verificación*: Declaración de la clase `ResourceQrcCatalog` con constantes `kCanonicalPrefix` (`":/vigilio"`), `kExpectedResourceCount` (35), y métodos estáticos puros `contains`, `find`, `resolveQrcPath`, `isValidAlias`, `isExcludedPath`, `resourcesByCategory`, `criticalResources`, `validateQrcConsistency`, `categoryName` y `statusName`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ResourceQrcCatalog.h && grep -E "class ResourceQrcCatalog|validateQrcConsistency" apps/vigilio-browser/src/core/ResourceQrcCatalog.h`

- [ ] TASK-003: Implementar el inventario canónico inmutable de los 35 recursos aprobados en `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`
  - *Criterio de verificación*: Tabla estática inmutable conteniendo con exactitud los 35 descriptores (6 temas, 6 wallpapers, 1 branding, 1 tokens, 10 iconos de navegación y 11 iconos de herramientas) concordantes con `apps/vigilio-browser/resources.qrc`.
  - *Comando*: `test -f apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp && grep -E "kApprovedResources|branding/vigilio-logo.svg" apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`

- [ ] TASK-004: Implementar las funciones de validación sintáctica de alias y rechazo de Directory Traversal en `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`
  - *Criterio de verificación*: `isValidAlias` rechaza cadenas vacías, barras iniciales o finales, barras dobles, secuencias `..`, caracteres no admitidos y extensiones incompatibles.
  - *Comando*: `grep -E "isValidAlias" apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`

- [ ] TASK-005: Implementar las reglas de exclusión de rutas no permitidas (`reference/`, `installer/`, `fonts/`) en `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`
  - *Criterio de verificación*: `isExcludedPath` retorna `true` para cualquier ruta que incluya carpetas de maquetas, empaquetado del instalador o fuentes sin licencia.
  - *Comando*: `grep -E "isExcludedPath|reference/|installer/|fonts/" apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`

- [ ] TASK-006: Implementar el motor de validación estática de consistencia para archivos XML `.qrc` en `apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`
  - *Criterio de verificación*: `validateQrcConsistency` valida que la raíz declare el prefijo `/vigilio`, verifica duplicados, detecta inclusiones prohibidas y certifica que todos los recursos críticos (`isCritical == true`) estén presentes.
  - *Comando*: `grep -E "validateQrcConsistency|MissingCriticalResource" apps/vigilio-browser/src/core/ResourceQrcCatalog.cpp`

- [ ] TASK-007: Implementar la suite de pruebas unitarias automatizadas en `apps/vigilio-browser/tests/unit/ResourceQrcCatalogTests.cpp`
  - *Criterio de verificación*: Cobertura de casos positivos (conteo exacto 35, búsquedas exitosas, resolución de ruta virtual), casos negativos (alias malformados, traversals `../`, alias inexistentes), filtrado por categoría, lista de recursos críticos, validación de consistencia XML con `resources.qrc` real y fixtures sintéticos anómalos, y estrés multihilo con 16 hilos.
  - *Comando*: `test -f apps/vigilio-browser/tests/unit/ResourceQrcCatalogTests.cpp && grep -E "test_catalog_total_count|test_concurrency" apps/vigilio-browser/tests/unit/ResourceQrcCatalogTests.cpp`

- [ ] TASK-008: Integrar el catálogo en `vigilio_core`, configurar el target CTest en `apps/vigilio-browser/CMakeLists.txt` y validar con `apps/vigilio-browser/scripts/check.sh`
  - *Criterio de verificación*: Compilación limpia con `-Werror` bajo C++20; registro del test `resource_qrc_catalog_tests` bajo `LABELS "unit;fast"`; ejecución de `check.sh` con 100% de tests pasando en tiempo récord.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh && ctest --test-dir apps/vigilio-browser/_build -R resource_qrc_catalog_tests --verbose`

- [ ] TASK-009: Certificar formalmente las referencias canónicas OpenSpec y lockfiles con la herramienta Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` confirma `valid: true` para todas las referencias OpenSpec y `verify-lock` valida el lockfile `0049-module.foundations.task0017.propose.lock.json`.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0049-module.foundations.task0017.propose.lock.json --project-root .`
