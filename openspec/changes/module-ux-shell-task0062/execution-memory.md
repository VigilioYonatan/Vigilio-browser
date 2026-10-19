# Execution Memory — module-ux-shell-task0062

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `ui` de la capacidad **main window layout** (`module.ux-shell.task0062`), diseñando la integración física de la jerarquía Qt Widgets en `BrowserWindow`, la orquestación desacoplada con `MainWindowLayoutService`, la incorporación del panel lateral `vigilio-sidebar` (colapsado 48 px / expandido 240 px), la máquina de estados visuales (vacío, cargando, error), la adaptación reactiva a la «Interfaz más fácil de usar» (Modo Senior), la navegación accesible por teclado, soporte de lectores de pantalla y la suite de pruebas automatizadas en `WindowTests.cpp`, en rigurosa conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices de `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0062-main-window-layout/task.md`: Contrato vinculante de la microtarea canónica 0062.
  - `openspec/changes/module-ux-shell-task0062/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única por widget y separación limpia entre el servicio de layout (`vigilio_core`) y la ventana gráfica (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pruebas de interfaz en Qt deterministas; aserciones precisas de jerarquía de widgets, geometrías y accesibilidad sin sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro de objetos en Qt mediante jerarquías de padres/hijos, gestión RAII, tipos de valor inmutables y exclusión de operaciones bloqueantes en el hilo principal de la UI.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración con barra lateral, omnibox y tarjetas.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48$ px, barra de navegación $\ge 56$ px, iconos escalados a $32\times 32$ px, navegación completa por teclado, indicadores de foco de alto contraste y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, renderizado vectorial de iconos Lucide SVG vía QRC sin accesos a disco repetitivos en el hilo de UI.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cero persistencia de datos de navegación o estado de layout en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC (viewBox `0 0 24 24`, trazo 1.8 px).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la ventana principal con barra lateral, barra de herramientas y buscador centrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de interacción visual opera 100% en local, sin llamadas de red ni sincronización remota.
  2. *Desacoplamiento del servicio de layout*: `MainWindowLayoutService` mantiene la autoridad sobre el estado geométrico y las densidades; `BrowserWindow` sincroniza sus widgets reactivamente sin lógica duplicada.
  3. *Invariante de no solapamiento geométrico*: Las áreas asignadas a las zonas de interfaz (`Sidebar`, `ToolBar`, `TabStrip`, `Banner`, `Content`, `StatusBar`) son mutuamente excluyentes ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y garantizan un área central útil de al menos $400\times 250$ px.
  4. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», todos los controles interactivos primarios cumplen `minimumHeight() >= 48` px.
  5. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o persistir el estado de la ventana en disco.
  6. *Invariante de lenguaje público*: Ninguna etiqueta visible o descripción accesible expone nombres internos de ingeniería.
- **Material reutilizable**:
  - `MainWindowLayoutService`, `MainWindowLayoutConfiguration` y `MainWindowLayoutValidator` de `src/core/`.
  - Iconografía Lucide SVG integrada en `resources.qrc`.
  - Infraestructura de pruebas existente en `tests/ui/WindowTests.cpp`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    *Mitigación*: `WindowTests.cpp` detecta la ausencia de servidor gráfico y marca la ejecución interactiva como `NOT_EXECUTED` de manera limpia, mientras que las pruebas de análisis estático y de servicio continúan verificándose.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0062/proposal.md`: Propuesta integral cubriendo los 12 apartados requeridos.
  - `openspec/changes/module-ux-shell-task0062/design.md`: Diseño detallado de arquitectura, estados y compatibilidad.
  - `openspec/changes/module-ux-shell-task0062/tasks.md`: 15 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0062/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - `vigilio-prompt validate-references --project-root .`: Exit code 0 (`valid: true`, 62 referencias).
  - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0184-module.ux-shell.task0062.propose.lock.json --project-root .`: Exit code 0 (`valid: true`, 13 checks passed).
  - `ctest --test-dir apps/vigilio-browser/build-ci-debug`: 27/27 pruebas pasando al 100%.
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y certificada en `openspec/changes/module-ux-shell-task0062/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
