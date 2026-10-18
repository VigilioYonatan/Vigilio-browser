# Pantallas y controles — Traducción opt-in
**Entrada**: menú contextual, barra de navegación, barra lateral o `Herramientas` según la capacidad. Icono Lucide SVG y etiqueta visible; en modo Experto comandos Ctrl+K; en Modo Senior entrada en menú simplificado accesible cuando la función es común. **Layout**: superficie plana con bordes sutiles, jerarquía título→explicación→control primario→secundarios; no glow ni gradientes en ventanas internas. **Operación principal**: local or third party upon explicit consent.

| Estado | Especificación |
|---|---|
| idle | mostrar controles aplicables con permiso efectivo y atajos visibles |
| hover/focus | resaltado moderado y focus ring perceptible para teclado |
| loading | indicador acotado, botón deshabilitado si duplicación causaría corrupción |
| empty | explicar ausencia de contenido y cómo empezar |
| success | notificar resultado, ubicación del dato y deshacer cuando posible |
| error | error humano, código diagnóstico redactado, recuperar/reintentar si seguro |
| forbidden | explicar permiso y ajuste relevante sin conceder acceso automático |
| offline/degraded | operar local cuando sea posible y declarar limitación de web/external |

**Accesibilidad**: Qt AccessibleName/Description, orden tab lógico, contrast AA, shortcuts remapeables, manejo de zoom DPI, objetivos Senior 44–48 px, texto 18px, labels explícitas, reduced motion y screenreader. **Navegación y teclado**: Escape cancela o cierra diálogo no destructivo; Enter confirma control enfocado; no trap de foco. **Test IDs**: `vigilio.translation.open`, `.primary`, `.cancel`, `.error`, `.status`; no se exponen como URL pública.
