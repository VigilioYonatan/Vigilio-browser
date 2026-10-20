# Execution Memory — module-ux-shell-task0066

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `ui` de la capacidad **title bar integration** (`module.ux-shell.task0066`), diseñando la integración física de la jerarquía Qt Widgets en `BrowserWindow`, la orquestación desacoplada con `TitleBarService`, la incorporación del widget superior `vigilio-titlebar` (altura 32 px estándar / 48 px en modo accesible), la caja de controles de ventana (minimizar, maximizar/restaurar, cerrar y acceso rápido a la interfaz accesible), la zona de arrastre ergonómica con soporte de doble clic, la máquina de estados interactivos (normal, hover, pressed, disabled, activo/inactivo), la adaptación reactiva a la «Interfaz más fácil de usar» (Modo Senior), la navegación accesible por teclado, soporte de lectores de pantalla y la suite de pruebas automatizadas en `WindowTests.cpp`, en rigurosa conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y las directrices de `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0066-title-bar-integration/task.md`: Contrato vinculante de la microtarea canónica 0066.
  - `openspec/changes/module-ux-shell-task0066/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única por widget y separación limpia entre el servicio de barra de título (`vigilio_core`) y la ventana gráfica (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pruebas de interfaz en Qt deterministas; aserciones precisas de jerarquía de widgets, geometrías y accesibilidad sin sleeps arbitrarios ni mocks fingidos.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro de objetos en Qt mediante jerarquías de padres/hijos, gestión RAII, tipos de valor inmutables y exclusión de operaciones bloqueantes en el hilo principal de la UI.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro (`#141D2B`) y superficies limpias; integración sobria en la barra superior del navegador.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48\times 48$ px, barra de título $\ge 48$ px de altura, iconos escalados a $24\times 24$ px, navegación completa por teclado, indicadores de foco de alto contraste (borde de 2 px `#2563EB`) y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, renderizado vectorial de iconos Lucide SVG vía QRC sin accesos a disco repetitivos en el hilo de UI.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales, sanitización del título de ventana y cero persistencia de datos de navegación o estado de la barra en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC (viewBox `0 0 24 24`, trazo 1.8 px).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**, **«Minimizar ventana»**, **«Maximizar ventana»**, **«Restaurar ventana»** y **«Cerrar ventana»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la ventana principal con barra superior integrada sobria y moderna.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser (`branding/vigilio-logo.svg`).

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de interacción visual opera 100% en local, sin llamadas de red ni telemetría remota.
  2. *Desacoplamiento del servicio de barra de título*: `TitleBarService` mantiene la autoridad sobre el estado geométrico y las densidades; `BrowserWindow` sincroniza sus widgets reactivamente sin lógica duplicada.
  3. *Invariante de no solapamiento geométrico*: Las áreas asignadas a los componentes de la barra (`iconRect`, `titleRect`, `draggableRect`, `controlsBoxRect`) son mutuamente excluyentes ($\forall i \ne j, C_i \cap C_j = \emptyset$) y garantizan un área de arrastre mínima de al menos 120 px (160 px en modo accesible).
  4. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», todos los botones de la barra de título cumplen `minimumHeight() >= 48` y `minimumWidth() >= 48` px.
  5. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o persistir el estado de la barra en disco.
  6. *Invariante de lenguaje público*: Ninguna etiqueta visible o descripción accesible expone nombres internos de ingeniería.
- **Material reutilizable**:
  - `TitleBarService`, `TitleBarConfiguration` y `TitleBarValidator` de `src/core/`.
  - Iconografía Lucide SVG integrada en `resources.qrc` y logotipo oficial `branding/vigilio-logo.svg`.
  - Infraestructura de pruebas existente en `tests/ui/WindowTests.cpp`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    - *Mitigación*: `WindowTests.cpp` detecta la ausencia de servidor gráfico y marca la ejecución interactiva como `NOT_EXECUTED` de manera limpia, mientras que las pruebas de análisis estático y de servicio continúan verificándose.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0066/proposal.md`: Propuesta integral cubriendo los 12 apartados requeridos.
  - `openspec/changes/module-ux-shell-task0066/design.md`: Diseño detallado de arquitectura, estados y compatibilidad.
  - `openspec/changes/module-ux-shell-task0066/tasks.md`: 15 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0066/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - `apps/vigilio-browser/scripts/check.sh`: Compilación limpia con `-Werror`, Clang-Format validado y 29/29 pruebas de CTest pasando al 100%.
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y certificada en `openspec/changes/module-ux-shell-task0066/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
