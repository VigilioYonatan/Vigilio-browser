# ux-shell — Responsabilidad y alcance

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.
