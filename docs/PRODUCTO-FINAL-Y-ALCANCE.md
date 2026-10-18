# Vigilio Browser — producto final y límites contractuales

## DECISIONES CONFIRMADAS
- Desktop Windows 10/11 x64, Linux x64, macOS Intel x64 y Apple Silicon arm64 **objetivo sujeto a verificaciones Qt**.
- C++20, Qt >=6.10, Qt Widgets, Qt WebEngine, SQLite, CMake; iconografía SVG tipo Lucide, Inter con fallback.
- **Código abierto confirmado, licencia exacta NO elegida**: MIT recomendada, no presentar como aprobada ni publicar LICENSE inventada. Auditar licencias de Qt, Chromium, SVG y demás dependencias.
- NO cuentas ni login de Vigilio, NO cloud, NO sync propio, NO telemetría remota involuntaria. El usuario sí puede iniciar sesión en webs ajenas. Actualización manual bajo solicitud.
- Perfil, historial, marcadores, notas y backups: almacenamiento local. El usuario debe autorizar borrados, permisos especiales y acceso MCP.

## Superficies target de producto
1. Shell: tabs horizontales o verticales, grupos, pins, workspaces, múltiples ventanas, atajos, historial en árbol y lista de lectura.
2. Barra de direcciones: búsquedas, autocompletado y comandos `@tabs`, `@history`, `@bookmarks`, `Ctrl+K`.
3. Navegación avanzada: split 2-4, link preview, focus, mouse gestures, acciones encadenadas, recarga programada.
4. Privacidad: Shields (ads/tracking y filtros cosméticos), navegación privada, permisos por origen, clean URL, perfil/containers.
5. Rendimiento: Turbo (caché y prioridades respetando Chromium), Memory Saver frozen/discarded, audio/llamadas/descargas protegidos.
6. Utilidades: screenshot, QR, lectura, notas, generador de passwords, traducción con consentimiento explícito, resource inspector y devtools responsive.
7. Visores: MD, Mermaid, JSON/JSONL, YAML/TOML/INI, XML, CSV/TSV, LOG/TXT, DIFF/PATCH, SVG, PDF/EPUB, HAR/HTTP/ICS. Priorizar por versión.
8. Extensiones Manifest V3 mediante APIs reales de Qt WebEngine >=6.10, **no afirmar Chrome Web Store full-compatible**.
9. Integración MCP opt-in vía bridge local, permisos por operación/agente y confirmaciones para mutaciones.
10. Normal, Senior (accesible) y Experto; la ingeniería UX/UI Senior es estándar en todos los modos.
11. Instalador y desinstalador completos: sin login, importación opcional, backups locales, retener o borrar perfil únicamente por elección informada.

## CURRENT frente a TARGET
CURRENT: starter C++ con core tabs/URLs/sesiones/SQLite, shell Qt aún sin verificación y 2 pruebas core en la auditoría anterior.
TARGET: todos los módulos arriba; pruebas multiplataforma y release firmadas aún NO_EXECUTED. **No inferir que una maqueta representa código real.**
