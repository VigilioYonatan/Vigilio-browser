# Instalación, primer inicio y desinstalación — contrato visual detallado

## Installer desktop nativo
Windows: diálogo nativo sobrio de ancho ~760–900px, sin wallpaper total ni textos marketing largos; logotipo globo/órbita 32px, barra de progreso y breadcrumb de pasos. Linux: DEB/AppImage respetando gestor de paquetes. macOS: DMG drag-to-Applications y primera ejecución; **no prometer wizard idéntico** en todos.
Estados: Welcome → Ubicación y alcance de instalación (si el tipo de instalador lo permite) → Componentes permitidos (acceso directo, asociaciones opcionales) → Revisar → Instalar → Finalizar. Botones Atrás, Siguiente y Cancelar con focus; detectar permisos insuficientes, disco lleno, archivos bloqueados y fallo de firmas. Nunca cambiar navegador por defecto sin confirmación explícita, ni instalar extensiones invisibles.
Al finalizar abrir onboarding dentro de app: tema Claro/Oscuro/Sistema, modo Normal/Senior/Experto, opción importar bookmarks (sin contraseñas plaintext), escoger buscador, pantalla de privacidad. Sin formulario Vigilio cuenta, sincronización ni telemetría cloud. Aceptación: cancelar instalador no destruye perfil previo; rollback si falla; ruta/permiso/error en lenguaje claro.

## Desinstalar sin pérdida por sorpresa
Paso 1: cerrar app de forma cooperativa, confirmar aplicaciones abiertas y exportación opcional. Paso 2: elegir **Conservar datos del navegador (por defecto)** o **Eliminar datos locales** (requiere confirmación adicional con lista y alcance); elegir destino de backup `.vigilio-backup` cifrado cuando se incluya material sensible. Paso 3: ejecutar y mostrar estado, errores, ayuda y resultado. Limpieza de caché es herramienta distinta, no opción de desinstalación.
No incluir contraseñas en JSON plano, no borrar carpetas externas arbitrarias, no eliminar datos de otras instalaciones/perfiles; validar symlinks/rutas, traversal, permisos, export atómico fsync+rename. No usar botón "exportar y desinstalar" si export falló: detener y preguntar.

## Controles a testear
Botones Next/Back/Cancel/Install/Finish; progreso; Browse location; import skip; Senior preview; respaldar/browse destination; conservar/eliminar data; dialog confirm; recuperación ante disco lleno; instalación previa; versión incompatible; desinstalación con app abierta; verificación por OS y accesibilidad de teclado/lector.

## Material gráfico
Usar SVG `assets/branding`, ilustraciones mínimas en instalación, fotografía **solo** splash/onboarding opcional. Maquetas `assets/reference/mockups/` no representan builds y pueden contener datos visuales obsoletos.


## Lenguaje público de producto
Consulte `docs/UX-COPY-PUBLICO.md`: nunca mostrar «Senior Mode», «Modo Senior», prompts o terminología interna en la UI pública. Usar «Interfaz más fácil de usar». La referencia oficial del logo del instalador es `resultado/06-logo-instalador-oficial.png`.
