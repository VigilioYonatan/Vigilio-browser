# Vigilio Browser — PROJECT-SUMMARY
**Tipo**: navegador desktop, código abierto (licencia por aprobar), modo BROWNFIELD + TARGET greenfield por módulos. **Usuarios**: usuarios generales, profesionales, personas con necesidad de accesibilidad y operadores de agentes locales. **Estado**: starter alpha; todo módulo no verificado indicado TARGET.

## Resultado final
Navegador de ventanas y pestañas con Qt WebEngine/Chromium, UI Qt Widgets nativa sobria; pestañas verticales, grupos, workspaces, split hasta 4, perfiles y contenedores aislados, historial, descargas, favoritos, lista lectura, búsqueda universal, comandos/gestos, sesión persistente, vistas previas, lectura, screenshot, QR, notas, traducción opcional, generador contraseñas, bloqueo trackers y anuncios, optimización tab freezing/discard, visores Markdown/Mermaid/JSON/CSV/XML/PDF/EPUB y otros por fases, extensiones MV3 dentro de límites Qt, MCP local opcional, backups locales. Modos Normal, Senior y Experto.

## Arquitectura
C++20 + Qt 6.10+ Widgets + Qt WebEngine; SQLite local, QStandardPaths, CMake; JSON settings, MD4C y Mermaid local cuando se integren. Módulos UI/core/engine/privacy/storage/performance/viewers/tools/mcp/diagnostics. No backend remoto Vigilio. Extensiones Chrome Web Store no garantizadas. Build target Windows 10/11 x64, Linux x64, macOS Intel y Apple Silicon sujeto a pruebas reales.

## Identidad / privacidad
**Sin cuenta Vigilio, login, cloud, sync ni telemetría remota**. Logins en webs ajenas se permiten. Perfiles son locales aislados; controles sobre secretos/cookies y backups cifrados cuando incluyan datos sensibles. MCP desactivado y proceso auxiliar no iniciado por defecto, permisos por cliente y site, acciones sensibles confirmadas.

## Pantallas
Ventana principal, nueva pestaña, ajustes, temas, Senior Mode, privacidad/site info, descargas, historial, marcadores, workspaces, split panes, command palette, herramientas, visores, extensiones, permisos/MCP, diagnostics, first-run, import/export, installer y uninstaller.

## Datos / desarrollo / distribución
SQLite para historial/bookmarks y metadatos; Qt WebEngine profiles para cookies/cache por perfil; settings versionados. Migraciones transaccionales, backup/restore con rollback. Compilación local Windows/Linux/macOS, jobs CI matrix. Installer EXE, Linux DEB/AppImage, macOS app/DMG; firmas y notarización sujetas a recursos reales. Actualizaciones manuales, sin consulta oculta por defecto.

## Testing / seguridad / operabilidad
Unitarios y componentes cada PR, integración con SQLite/WebEngine y fixture HTTP local; E2E smoke serial en cambios relevantes y completos de noche/release. Fuzz parsers, ASan/UBSan/TSan, benchmark multiproceso RAM/CPU/startup, WCAG 2.2 AA donde aplique; pruebas Narrator/Orca/VoiceOver. ClangFormat/Clang-Tidy/CTest y ESLint/Prettier únicamente para JS. Logs QLoggingCategory locales y sanitizados, métricas opt-in locales; sin red para diagnósticos.

## Aceptación
No producción hasta: builds nativos y firma revisados, permisos/aislamiento revisados, adblock medido, backup probado, E2E y accesibilidad aprobados, regresiones de memoria analizadas, exploración funcional de herramientas, OpenSpec y documentos sincronizados, decisiones materiales cerradas.

## Brechas conocidas
Qt y E2E no ejecutados aquí; MCP y extensiones con límites de compatibilidad; licencia específica sin ratificación; certificados de firma sin aprovisionar; branding visual solo conceptual; mediciones de RAM no obtenidas. Ver answers.md, AUDIT.md y docs/rules/05-reference/traceability.md.


## Ejecución incremental
- **1000 micro-packs** bajo `rules/01-modules/*/micro/`; registrar `NOT_EXECUTED` hasta ejecutar Prompt Builder y los cambios.
- Orden total explícito en `plans/full-project.yaml` y cadenas `depends_on`; 20 lotes en `plans/batches/`.
- Se elimina OpenSpec del alcance por decisión posterior; no se genera ni modifica.
- Los assets reales son propiedad de `apps/vigilio-browser/assets/`, diferenciando mockups de recursos ejecutables.
- Política sin cloud/login/sync; actualizaciones manuales; licencia exacta aún abierta antes de publicar.


## Skills Senior y resultado visual
- `docs/skills/`: buenas prácticas obligatorias de ingeniería, testing, rendimiento, UX/UI, seguridad y release.
- `/resultado/`: referencia visual final del navegador, tools, Modo Senior, instalador y desinstalador.


## Lenguaje público de producto
Consulte `docs/UX-COPY-PUBLICO.md`: nunca mostrar «Senior Mode», «Modo Senior», prompts o terminología interna en la UI pública. Usar «Interfaz más fácil de usar». La referencia oficial del logo del instalador es `resultado/06-logo-instalador-oficial.png`.
