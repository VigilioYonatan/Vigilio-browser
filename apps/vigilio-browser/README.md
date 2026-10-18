# Vigilio Browser — Starter 0.1 ALPHA (proyecto de código fuente)

**No es un navegador publicado ni un .exe listo para instalar.** Este paquete es el primer incremento comprobable para corregir la ausencia de código, tests y trazabilidad detectada en la auditoría de la Super Skill Senior 3.0.

## Decisiones confirmadas
- Windows / Linux / macOS; C++20 + Qt 6.10+ Widgets + Qt WebEngine + SQLite + CMake.
- Sin login, cuentas Vigilio, cloud ni sincronización propia. Datos y ajustes locales. Exportación manual. MCP inactivo/no incluido en esta alpha.
- Diseño oscuro/claro sobrio, Modo Senior inicial, sin interfaz recargada.

## Qué contiene REALMENTE
- **Núcleo compilable sin Qt**: validación básica de direcciones, modelo de pestañas, snapshot local de sesión, repositorio SQLite para favoritos e historial.
- **UI Qt en código fuente**: ventana con pestañas, barra de direcciones, navegación, favoritos, tema claro/oscuro, Modo Senior básico, exportación JSON de ajustes y favoritos, restauración de pestañas diferidas. Todavía no compilada ni ejecutada en este entorno porque falta Qt WebEngine.
- **Tests:** unitarios e integración SQLite ejecutados satisfactoriamente en Linux (también bajo ASan/UBSan); prueba E2E Qt de navegación local preparada pero no ejecutada.
- **Calidad:** CTest, `.clang-format`, `.clang-tidy`, `.editorconfig`, matriz de requisitos, ADR, checklists y un flujo de CI propuesto, sin resultados de CI remoto.

## Compilar el núcleo con pruebas (Linux)
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVIGILIO_BUILD_GUI=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```
Requiere CMake ≥3.24, compilador C++20 y SQLite3 con headers y librería.

## Compilar la UI (requiere Qt 6.10+)
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVIGILIO_BUILD_GUI=ON
cmake --build build
```
Asegurar que CMake encuentre `Qt6::Widgets` y `Qt6::WebEngineWidgets`. Si no encuentra Qt, solo compilará el núcleo y mostrará una advertencia. **No confundirlo con una UI comprobada.**

### E2E (cuando Qt y GUI estén disponibles)
```sh
cmake -S . -B build -DVIGILIO_BUILD_GUI=ON -DVIGILIO_ENABLE_E2E=ON
cmake --build build
ctest --test-dir build -L e2e --output-on-failure --parallel 1
```
Puede requerir un display y sandbox correcto. No ejecutar como root sin protecciones; no pasar flags para desactivar el sandbox del motor en producción.

## No implementado en esta entrega
Vigilio Shields/AdBlock, extensiones MV3, MCP, perfiles aislados, modo privado, importación completa, respaldo cifrado, desinstalador con exportación, visores Markdown/Mermaid/JSON, split view y otras microherramientas. Todas permanecen en el roadmap, **no en este binario**.

Consulta `docs/REVISION_FINAL.md`, `docs/REQUISITOS.csv` y `docs/PRUEBAS_Y_OBSERVABILIDAD.md` para resultados y riesgos.
