# Backlog funcional Vigilio Browser — conforme a lo acordado

Estado: **planificado** salvo funciones señaladas como **parciales** en `REQUISITOS.csv`. Este archivo no es prueba de ejecución.

## Primera vertical — starter v0.1
- Navegación HTTP/HTTPS y búsquedas (núcleo PASS, UI pendiente de compilar).
- Pestañas: abrir/cerrar/duplicar/fijar/silenciar/cerrar otras (núcleo separado probado; UI parcial).
- Favoritos, historial local SQLite (integración DB PASS; UI no probada).
- Menú contextual básico de pestañas (código Qt no probado).
- Temas claro/oscuro sin glow, Modo Senior básico (código Qt no probado; sin revisión de lectores de pantalla).
- Exportación local de ajustes y favoritos JSON (código Qt no probado); restauración diferida de pestañas (código Qt no probado).

## Iteraciones previstas
- Privacidad: Vigilio Shields/AdBlock, listas regionales, filtros cosméticos, bloqueador de rastreadores, excepciones, permisos de sitio, modo privado y perfiles separados.
- Rendimiento: Vigilio Turbo, políticas de caché, gestión de DNS/conexiones mediante Chromium, lazy loading, freeze/discard, RAM/CPU total medido.
- Organización: espacios de trabajo, listas de lectura, grupos, vista dividida, pestañas verticales, pin, recarga programada, copia de enlaces, historial en árbol, sesiones y recuperación.
- Herramientas: navegación mediante teclado, vista previa de enlaces, modo concentración, aplicaciones laterales, detector de recursos, modo desarrollador, comandos universales, acciones configurables, notas, QR, capturas, lectura, traductor opcional, contraseñas, enlaces limpios, reglas por sitio.
- Visores: Markdown, Mermaid, JSON/JSONL, YAML, TOML, INI, CSV/TSV, XML, TXT/LOG, DIFF/PATCH, SVG, PDF, EPUB, HAR, ICS, imágenes/audio/vídeo compatibles.
- Integraciones: extensiones Manifest V3 **según APIs realmente soportadas por Qt WebEngine**; MCP local opt-in como puente de procesos, permisos granulares y registro local mínimo, no activos al inicio.
- Copias: importar favoritos, exportar e importar ajustes, backup local cifrado, exportación desde el desinstalador, sin cloud ni cuentas de Vigilio.
- Packaging: .exe firmado cuando sea posible para Windows, AppImage/.deb según distribución para Linux, .dmg firmado y notarizado para Mac; actualizar con comprobación de integridad y opción de rollback.
- Calidad: CI/CMake/ClangFormat/Clang-Tidy/Unit/Integration/E2E mínimo/Benchmark real/observabilidad local/seguridad/fuzzing/accesibilidad Senior con usuarios.

## Decisiones de UX
- Logo: globo orbital azul/violeta; nativo, sin brillo ni apariencia artificial.
- Tipografía: Inter con fallback nativo. Iconos: SVG de estilo Lucide; no usar emojis como iconos.
- Modo Normal, Modo Senior opcional desde configuración inicial o ajustes, Modo Experto; accesibilidad base transversal.
- No mostrar todas las herramientas al mismo tiempo: función visible mediante contexto o buscador; carga solo al activarla.

## No objetivos
- No login Vigilio, no cuentas propias, no sincronización cloud, no telemetría remota por defecto.
- No afirmar todas las extensiones Chrome compatibles por defecto.
- No reclamar 10/10, RAM mínima o seguridad total sin evidencias de pruebas.
