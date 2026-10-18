# Catálogo operativo — cada función, ubicación y etapa

Estados de implementación: CURRENT_PARTIAL no implica probado GUI; TARGET pendiente; EXPERIMENTAL requiere flag/consent. Los IDs aquí conectan a `rules/01-modules/*`.

| ID | Herramienta | Dónde aparece / cuándo | Acción y resultado esperado | Estado |
|---|---|---|---|---|
| NAV | Barra de direcciones | Toolbar siempre | buscar URL/consulta; validación esquema/feedback | CURRENT_PARTIAL |
| TAB | Pestañas, grupos, pins, mute | Tabbar y menú derecho | nueva derecha, grupo, ventana, split, duplicar, pin, mute sitio/tab, otros, derecha, cerrar repetidas, reabrir | CURRENT_PARTIAL |
| WRK | Workspaces | Sidebar personal/trabajo/estudios | selección, guardado local, aislamientos opt-in | TARGET |
| SPL | Vista dividida 2–4 | Menú pestaña + comando | docks redimensionables con mínimo legible | TARGET |
| CMP | Ctrl+K y prefijos | Atajo global dentro de Vigilio | tabs/history/bookmarks/comandos | TARGET |
| HIS | Historial árbol | Sidebar Historial | nodos por navegación y búsqueda | TARGET |
| PRF | Perfiles e identidades | Menú perfil / sitio | cookies y storage por perfiles independientes | TARGET |
| BKM | Marcadores, lectura y sesiones | Sidebar y menú página | guardar/restaurar/reabrir sin pérdida | CURRENT_PARTIAL |
| DNL | Descargas | Toolbar y sidebar | progreso, ruta, cancelación, limpieza segura | TARGET |
| SHD | Shields AdBlock | Escudo por sitio | listas filtros, reglas, estadísticas reales, toggle | TARGET |
| TUR | Turbo / Memory Saver | Ajustes > Rendimiento | caché, congelar/descartar, excepciones audio | TARGET |
| EXT | Extensiones MV3 limitadas Qt | Puzzle > Extensiones | instalar local manifest; permisos y límites API | TARGET |
| RDR | Markdown, Mermaid, JSON, PDF y visores | Herramientas > Visores o abrir archivo | previsualizar, raw/tree, exportar local; errores | TARGET |
| NOT | Notas rápidas | Sidebar > Notas | captura vinculada opcional, autosave local | TARGET |
| SCR | Capturas y QR | Herramientas o clic derecho | imagen visible/completa + QR válido | TARGET |
| REA | Modo lectura | Menú página cuando artículo detectable | ocultar ruido sin editar web original | TARGET |
| TRN | Traductor opcional | Menú selección | consentir proveedor; advertir red externo | TARGET |
| DEV | Responsive inspector/HAR/media | Herramientas > Desarrollo | ver tamaño y recursos, sin sandbox off | TARGET |
| MCP | Agentes MCP | Ajustes > Avanzado | bridge off por defecto, autorización y stop | TARGET |
| SNR | Modo Senior | Onboarding/Ajustes/Ctrl+K | 18px, hitbox grande, labels, AA y lectores | TARGET |
| INS | Instalador/desinstalador | Entradas OS | backup/retener/borrar con consentimiento | TARGET |
| BKP | Backup local | Ajustes > Datos locales | cifrar/restaurar sin passwords en claro | TARGET |
| THM | Temas y wallpapers | Ajustes > Apariencia | Light/Dark/Aurora/Midnight/Forest/Sunset | TARGET |
| DIA | Diagnósticos | Herramientas > Diagnósticos | RAM/CPU/crash logs local, redacción | TARGET |

Todas las operaciones requieren estados vacíos, carga, cancelación, error y seguridad descritos en UX-UI-SENIOR-CONTRATOS.md.
