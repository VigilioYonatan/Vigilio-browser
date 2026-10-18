# foundations — Reglas, operaciones y errores

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
cmake targets, compiler warnings, Qt dependency validation, WebEngine version compatibility, resource qrc inclusion, startup bootstrap, application lifetime, crash recovery policy, CMake presets, test fixture foundation, clang-format configuration, clang-tidy configuration, CI Linux job, CI Windows job, CI macOS job, dependency inventory, SBOM generation, license attribution inventory, resource loading, filesystem permissions, feature flags, config schema, test labels, cross platform paths, reproducible build metadata.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.
