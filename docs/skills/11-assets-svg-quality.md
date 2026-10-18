# Calidad de assets SVG y marca — contrato verificable

- Los iconos de interfaz usan viewBox `0 0 24 24`, trazos de 1.8 px y terminaciones redondas; no combinar estilos incompatibles.
- El globo con órbita es un elemento de identidad, no se sustituye por símbolos de código ni imágenes genéricas.
- Cada SVG debe tener XML bien formado y se verificará su renderizado a 16, 20, 24, 32 y 48 px, en claro y oscuro.
- El SVG es recurso vectorial; la ruta de Qt (`resources.qrc`) debe coincidir con el asset real y probarse en Qt.
- Mantener el área de seguridad del icono y asegurar alineación óptica; no confundir SVG válido con iconografía aprobada visualmente.
- No incluir un icono solo porque exista: tiene que representar una acción o categoría concreta.
- Las imágenes de `resultado/` son referencias visuales conceptuales; no son UI ya implementada ni contratos literales de texto.
- Para las imágenes finales, conservar origen y no redistribuir fuentes u otros assets de terceros sin permiso.
- Verificar manualmente legibilidad, contraste y posición en plataformas reales.
