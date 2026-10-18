# Recursos ejecutables de Vigilio Browser

**Única ruta canónica:** `apps/vigilio-browser/assets/`. Archivos bajo `reference/` son maquetas, NO recursos para instalar. `resources.qrc` expone recursos de UI a `:/vigilio/...` en Qt una vez compilado. Qt no fue compilado en esta revisión.

- `branding/`: globo con órbita, variantes SVG, colores documentados.
- `icons/`: iconos SVG de navegación/herramientas. Inspeccionar licencias de iconos originales frente a Lucide.
- `wallpapers/`: fotos/ilustraciones de inicio, nunca como fondo de páginas web o diálogos.
- `themes/` y `design-tokens/`: colores, dimensiones, radii y controles por modo.
- `fonts/`: notas de Inter y alternativas. **No distribuir fuentes binarías sin revisar licencia.**
- `installer/`: textos/guía y recursos, distinto de un instalador construido.
- `reference/mockups/`: referencias gráficas únicamente. Algunas ilustraciones antiguas muestran sync/login por error: NO expresan la política del producto.

Reglas: archivo original/permiso y SHA256 en catálogo; ≤ recursos útiles; no telemetría, login, cloud, sincronización; empaquetar sólo lo necesario.


## Final targets
La subcarpeta `reference/final-targets/` contiene la visión final de la UI para home, tools, settings, instalador y desinstalador.
