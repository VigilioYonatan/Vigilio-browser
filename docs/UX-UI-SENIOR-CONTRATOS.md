# Sistema UX/UI Senior — fuente de diseño

## Arquitectura visual
Ventana desktop sobria: fila tabs 34–40dp; barra de navegación 44–48dp; sidebar plegable 220–264dp abierta o 52dp compacta; viewport web ocupa el resto. No cards decorativas dentro de páginas web. Fondo panorámico opcional **solo nueva pestaña**. Sin resplandores/neón en superficies productivas.
Fuente Inter o fallback del SO; escala 12/13/14/16/20/24; interlineado 1.4–1.6; espaciado en escala 4/8/12/16/24/32; radio 6/8/12; iconos SVG 16/20/24; colores base light #F7F9FC/#FFFFFF/#172033 y dark #111827/#1B2535/#E5E7EB; acento #2563EB. Validar contraste, estados de foco y high contrast.

## Comportamiento de controles
Todo control debe tener ID semántico, etiqueta accesible, tooltip descriptivo si icon-only, atajo sólo si no colisiona, visible/enabled separados, states idle/hover/focus/pressed/loading/success/error/disabled, resultado verificable, ESC cancelar cuando reversible, deshacer cuando factible. Los menús contextuales solo muestran acciones válidas para selección y perfil.
Navegación por teclado: Tab/Shift+Tab orden lineal, Enter/Space activa, flechas navegan elementos de lista, Escape cierra popover, focus restored a disparador. Nunca bloquear el main thread en IO o parsing. Reducción de movimiento respeta preferencia SO.

## Modos
Normal es densidad balanceada; Senior muestra labels completos, tipografía configurable con default 18px, tap targets 44–48px, menú reducido con "Más opciones", contraste AA, lectura en voz alta opt-in y tutorial no intrusivo. Experto muestra operaciones avanzadas sin anular protección. Cambio disponible en onboarding, Ajustes > Apariencia y accesibilidad, `Ctrl+K`. Nunca inferir edad. Senior no modifica cookies ni permisos ni riesgo de acciones.

## Pantallas y flujos con owner
Nueva pestaña: logo discreto, search, favoritos, wallpaper optativo y control de personalización, sin métricas falsas de RAM/adblock.
Ajustes: árbol categoría a izquierda, formulario a derecha, búsqueda, cambios guardados localmente; preview de tema antes de aplicar.
Escudo: panel anclado al botón, contadores provenientes de datos reales, switch por sitio, explicar ruptura potencial, reset de excepciones.
Descargas: tabla compacta, progreso, ubicación, pausar/cancelar cuando el backend lo permita; mostrar errores y reintento sin duplicación.
Herramientas: lista ordenable y buscable por categoría; herramientas secundarias no precargadas.
Visores: preview y raw, export/salvar explícito, límite de archivo y parsing aislado; errores de archivo invalidado; zoom teclado.
MCP: off default, lista de agentes autorizados, permisos lectura/escritura, historial local, detener todo; consentimiento antes de interacción sensible.

## Verificación
Capturas en Windows y macOS/Linux, zoom 100/125/150/200, 1280x720 mínimo objetivo, temas, soporte Windows Narrator, macOS VoiceOver y Linux Orca. Prueba WCAG 2.2 AA donde aplique, sin atribuir conformidad sin test manual.


## Lenguaje público de producto
Consulte `docs/UX-COPY-PUBLICO.md`: nunca mostrar «Senior Mode», «Modo Senior», prompts o terminología interna en la UI pública. Usar «Interfaz más fácil de usar». La referencia oficial del logo del instalador es `resultado/06-logo-instalador-oficial.png`.
