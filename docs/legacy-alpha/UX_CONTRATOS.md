# Contratos UX: primera vertical de Vigilio

Cada operación debe especificar estado, condición, error, recuperación, teclado y test. **Estos contratos no implican que la funcionalidad esté verificada en GUI.**

| ID | Control y ubicación | Acción | Estados/errores | Accesibilidad | Evidencia |
|---|---|---|---|---|---|
| UX-01 | Dirección / barra | Introducir URL o búsqueda, Enter | URL aceptada o advertencia de esquema rechazado | Nombre accesible; Ctrl+L | UNIT-URL PASS; GUI NOT_RUN |
| UX-02 | + / barra de pestañas | Abrir nueva pestaña | Home en blanco controlado; errores de proceso pendientes | Ctrl+T | UNIT-TAB PASS; GUI NOT_RUN |
| UX-03 | X / pestaña | Cerrar pestaña | No cerrar fijada; última pestaña crea nueva home | Ctrl+W | Modelo probado; GUI NOT_RUN |
| UX-04 | Favorito / toolbar | Guardar URL y título en DB local | Éxito/aviso; DB corrupta/error explícito | Ctrl+D | INT-SQLITE PASS; GUI NOT_RUN |
| UX-05 | Modo Senior / menú | Aumenta tamaño de fuente y objetivos de interacción | Persistencia local; posibilidad de desactivar | Etiquetas, teclado; lectores pendientes | GUI NOT_RUN |
| UX-06 | Exportar / menú | Guarda ajustes y favoritos en JSON local | Cancelar/error de escritura/confirmación | Diálogo nativo | GUI NOT_RUN |
| UX-07 | Restaurar / arranque | Carga sólo 1 WebView inicialmente, resto diferidas | Límites y restablecimiento previstos | Pestañas navegables por teclado | CORE PASS; GUI NOT_RUN |

**Pendiente UI**: estados de carga del WebEngine, drag/drop, vistas divididas, manejo de permisos sitio, control de foco con lector de pantalla, prueba de Modo Senior con usuarios. No fingir que la mockup es accesibilidad probada.
