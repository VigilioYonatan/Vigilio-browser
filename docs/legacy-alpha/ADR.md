# Decisiones de arquitectura — Vigilio Browser 0.1

- **ADR-001 (confirmada):** C++20, Qt 6.10+, Qt Widgets, Qt WebEngine, CMake, SQLite. No Electron ni fork propio de Chromium.
- **ADR-002 (confirmada):** Sin cuenta Vigilio, sin cloud, sin backend propio y sin telemetría remota por defecto. Perfiles y configuraciones en disco local.
- **ADR-003 (implementada parcialmente):** Separar núcleo de UI para poder compilar y probar navegación, pestañas y SQLite sin WebEngine.
- **ADR-004 (implementada parcialmente):** Persistencia SQLite con sentencias preparadas; preferencias por QSettings en UI; exportación en JSON vía QSaveFile. Falta importación y backup cifrado.
- **ADR-005 (implementada parcialmente):** Restaurar sesión con vistas WebEngine diferidas; pestañas secundarias no se cargan hasta seleccionarlas. Faltan benchmarks y robustez de recuperación tras fallos.
- **ADR-006 (pendiente):** MCP solo como puente local, con autorizaciones por agente, inactivo por defecto, no integrado en 0.1.
- **ADR-007 (pendiente):** Extensiones Manifest V3, AdBlock, VPN no incluida, navegacion privada aislada: requieren análisis de APIs/licencias y seguridad.
- **ADR-008 (pendiente):** Empaquetado firmado por SO e instalador/desinstalador con exportación; no simular disponibilidad de instalador.

## Riesgos activos

P0: El prototipo no debe anunciarse como navegador seguro y listo para uso habitual; falta revisar aislamiento, permisos, WebEngine actualizado y pruebas de integración reales.
P1: Qt 6.10+ y sus APIs no verificadas en este entorno. P1: cierres, interrupciones de I/O, importación y accesibilidad en las tres plataformas sin probar.
P2: Diseños y componentes existentes son propuestas UX, no una biblioteca implementada y auditada.
