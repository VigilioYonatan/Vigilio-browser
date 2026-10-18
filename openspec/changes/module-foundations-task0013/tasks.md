# Tasks — module-foundations-task0013

Status: PROPOSED

- [ ] TASK-001: Formalizar el modelo de dominio de WebEngine version compatibility (`ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, `WebEngineCompatibilityReport`) en las especificaciones del módulo foundations
  - *Criterio de verificación*: Las entidades de dominio en C++20, la estructura cuatripartita inmutable de versión de Chromium, las capacidades funcionales del motor y la taxonomía exhaustiva de resultados de validación están rigurosamente especificadas en la propuesta OpenSpec.
  - *Comando*: `grep -A 30 "Entidad de valor \`ChromiumVersion\`" openspec/changes/module-foundations-task0013/design.md`

- [ ] TASK-002: Formalizar las invariantes de versión mínima canónica (Chromium milestone >= 128 / Qt >= 6.10.0), requerimientos de Manifest V3 y mitigaciones de seguridad
  - *Criterio de verificación*: La propuesta establece que el perfil de navegador completo exige Qt >= 6.10.0 y Chromium milestone >= 128 para soporte de extensiones MV3, aislamiento de procesos y protección contra vulnerabilidades conocidas, documentando el baseline determinista.
  - *Comando*: `grep -E "Chromium milestone >= 128|ManifestV3Extensions|R-001|R-002" openspec/changes/module-foundations-task0013/design.md`

- [ ] TASK-003: Formalizar el protocolo de validación de arranque para el proceso asistente `QtWebEngineProcess`, sandboxing y degradación suave a modo headless
  - *Criterio de verificación*: El diseño técnico formaliza el punto de pre-validación previo a la instanciación de vistas web, garantizando la verificación de ejecutabilidad de `QtWebEngineProcess`, el soporte de espacios de nombres para sandboxing y la transición controlada a `DegradedHeadless` sin caídas de la aplicación.
  - *Comando*: `grep -A 20 "Contrato funcional y ciclo de vida" openspec/changes/module-foundations-task0013/design.md`

- [ ] TASK-004: Verificar el desacoplamiento estricto del núcleo de dominio frente a cabeceras de Chromium y bibliotecas de interfaz gráfica
  - *Criterio de verificación*: Las especificaciones de diseño garantizan que `vigilio_core` y `vigilio_storage` compilen y operen limpiamente en C++20 sin incluir `#include <QWebEngineView>` ni enlaces a librerías de Chromium en modo headless.
  - *Comando*: `cmake -S apps/vigilio-browser -B apps/vigilio-browser/_build -G Ninja -DVIGILIO_BUILD_GUI=OFF && cmake --build apps/vigilio-browser/_build --target vigilio_core_tests`

- [ ] TASK-005: Ejecutar la suite de pruebas local automatizada y verificar compilación limpia bajo opciones estrictas `-Werror`
  - *Criterio de verificación*: Compilación exitosa con Ninja y 100% de pruebas pasadas en CTest (`core_units`, `storage_integration`, `vigilio_warnings_tests`, `qt_dependency_validation_tests`) sin advertencias de compilación y con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-006: Sincronizar la documentación canónica de la microtarea y validar las referencias canónicas del árbol OpenSpec con Prompt Builder
  - *Criterio de verificación*: El archivo de referencia canónica `canonical-reference.yaml` y la propuesta OpenSpec pasan satisfactoriamente la validación automatizada de referencias canónicas mediante la herramienta Prompt Builder sin discrepancias.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
