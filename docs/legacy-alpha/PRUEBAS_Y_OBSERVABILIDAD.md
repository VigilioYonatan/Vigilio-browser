# Estrategia de QA, E2E y observabilidad local

## Reglas
1. Núcleo y SQLite: tests rápidos por cada cambio, sin navegador Chromium.
2. E2E: **solo navegación local esencial, recuperación de sesión, descargas y exportación**, mediante un navegador a la vez (`RUN_SERIAL`, `RESOURCE_LOCK`, timeouts), con fixtures HTTP/local sin internet. La prueba `BrowserSmoke.cpp` es una primera plantilla, NO_EJECUTADA.
3. E2E completos por OS solo en runners con Qt WebEngine actualizado, display, dependencias y sandbox configurados. No usar pruebas de front-end para medir RAM.
4. Benchmarks separados: cold start, 1/5/20 pestañas, RSS total de procesos, p50/p95 de navegación, CPU y tiempo de recuperación. Guardar hardware, OS, motor, condición y repeticiones; no hay métricas reales todavía.
5. Seguridad: fuzz de URL, sesiones, backups, Mermaid/JSON/SVG; sanitizers en binarios de pruebas; pruebas de permisos para MCP.
6. Accesibilidad: automatización donde sea posible y pruebas manuales con Narrador, VoiceOver, Orca y teclado.

## Observabilidad
- `QLoggingCategory` y registros estructurados por categoría en el futuro; este prototipo usa patrón básico de logging local.
- **No registrar URLs completas, cookies, cabeceras, contraseñas, tokens, formularios, prompts ni contenido web**.
- Sin envío remoto por defecto, sin login, sin nube, sin telemetría oculta.
- Diagnóstico local opcional con retención limitada, borrado y exportación revisada.
- Simular fallos para verificar qué se registra y qué NO se filtra.

## Estado al 2026-10-16
- Compilación de núcleo C++20: PASS en entorno Linux.
- Tests núcleo bajo AddressSanitizer + UndefinedBehaviorSanitizer: PASS (2/2, proceso separado, no habilitado en producto).
- Unitarios `core_units`: PASS.
- Integración SQLite `storage_integration`: PASS.
- GUI Qt WebEngine: NOT_RUN (Qt no disponible).
- E2E: NOT_RUN.
- Accesibilidad manual: NOT_RUN.
- Benchmark RAM/velocidad: NOT_RUN.
- Compilaciones/instaladores multiplataforma: NOT_RUN.
- ClangFormat/Clang-Tidy: NOT_RUN (herramientas no instaladas).
