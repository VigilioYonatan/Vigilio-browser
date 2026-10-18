# Testing, formateadores, linters, performance y evidencia

## Prioridad y recursos
PR: format/lint/build, unit + Qt component + SQLite integration; E2E smoke solo si cambia flujo crítico y existe runner dedicado. Nightly: E2E ampliados, fuzz, sanitizers y matriz OS. Antes de release: visual regression, lectores de pantalla, instalador/desinstalador, backup/restore, web security, MCP permission negative paths y benchmarks.
E2E desktop pesado: un proceso navegador por runner, `RUN_SERIAL`, `RESOURCE_LOCK browser_e2e`, perfiles temporales; páginas offline fixture HTTP local sin llamadas a terceros; no sleeps fijos, usar condiciones/QSignalSpy y timeouts; matar procesos hijos y limpiar en finally. No medir benchmarks en ambiente compartido.
Unit: URL parse, tab state, permission policies, filter match, backup manifest, command registry. Integration: SQLite schema/migrations, per-profile isolation, Qt WebEngine ephemeral profiles, download dispatch, file viewers. E2E: navegador arranca, navega, cierra/recupera tab, importa/exporta, tema/senior, shields site override, consent MCP, instalación, desinstalación. Datos sintéticos sin PII.

## Toolchain
ClangFormat (style versionado) requerido; clang-tidy + Clang Static Analyzer, compiler warnings /W4 o -Wall -Wextra -Wconversion; EditorConfig. Prettier y ESLint solo para JS/JSON assets de visores, no imponer Node a app C++ final. CMake formatting/lint según herramienta validada. CI matriz Windows/Linux/macOS sólo cuando runners/Qt provisionados, no reclamar PASS remoto.
ASan+UBSan en builds distintos de TSAN (evitar incompatibilidad), fuzz parsers sobre corpus malicioso; análisis deps/SBOM/licencias. Gradualmente gates por módulo.

## Observabilidad local
`QLoggingCategory` categorías ui/navigation/storage/shields/mcp/downloads, rotación y TTL configurable, ningún URL full/contraseña/cookie/contenido en logs por defecto. Métricas opt-in en panel local y capturas de crashes revisables por usuario. 0 exportación remota automática. Medir memoria total proceso principal+subprocesos Qt WebEngine por OS, idle y 1/5/10/20 tabs, p50/p95 navegación en páginas reproducibles, consumo CPU en reposo; no afirmar 60% ahorro sin datos.

## Evidencia, gates y Definition of Done
Cada cambio: ID, paths, baseline test, implementación, nuevos tests, comandos+exit codes, capturas cuando UI, decisión rollback y commit con verbo de dominio; PASS solo con salida real. Contratos UX reproducibles en 100/150/200% y teclado. `generated/` en `.gitignore`; no mencionar docs/prompt-builder en comentarios de código ni mensaje de commit.
Evidencia actual: core unit/integration previamente PASS; Qt GUI, E2E, instaladores, SO matrix, runtime memory y CLI Prompt Builder NOT_EXECUTED.
