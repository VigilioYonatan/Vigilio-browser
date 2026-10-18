# Constitución técnica y de producto
1. Vigilio es local-first; jamás introducir login, cloud, sync ni telemetría propia sin un cambio humano explícito.
2. Código abierto aprobado; licencia exacta OPEN (Q-007). No publicar como licenciado todavía.
3. C++20, Qt Widgets 6.10+, Qt WebEngine (Chromium), SQLite, CMake. Cambiar stack exige ADR y autorización.
4. Modos Normal, Senior y Experto. Senior ≠ calidad de ingeniería Senior; la accesibilidad es transversal.
5. UI visual auténtica: densidad productiva, sombras mínimas, jerarquía consistente, SVG reales, foco visible, sin neón ornamental.
6. Asset runtime owner: apps/vigilio-browser/assets. Mockups conceptuales no van a release.
7. Ninguna herramienta pesada residente si está desactivada; lazy-load, no trabajo bloqueante en hilo de UI.
8. Cada mutación tiene estado, permiso, error, cancelación/deshacer cuando sea posible, auditoría local sin datos sensibles y tests.
9. E2E solo flujos verticales críticos; ejecutar serial/por resource lock. No presentar cifras de RAM hasta medición real.
10. Distribución Win10/11 x64 Linux x64 macOS Intel/arm64 target a verificar; updates manuales por usuario.
11. Cada change tiene PROPOSE/APPLY/REVIEW, micro-commit comprobado, memoria de ejecución OpenSpec. No editar generated/ y mantenerlo excluido de Git.
12. Definitions: CURRENT código presente; TARGET contrato futuro; GAP pendiente; PASS evidencia ejecutada; NOT_EXECUTED nunca se transforma en PASS.
