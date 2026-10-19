# Execution Memory — module-ux-shell-task0064

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `domain` de la capacidad **title bar integration** (`module.ux-shell.task0064`), modelando de manera formal y desacoplada la taxonomía de la barra de título integrada, configuraciones canónicas de densidad visual (estándar, compacta y «Interfaz más fácil de usar» / Modo Senior), botones de control de ventana (minimizar, maximizar/restaurar, cerrar, toggle accesible), cálculo analítico del área de arrastre no-cliente, función de clasificación espacial de pruebas de impacto (*non-client hit testing*), validador estático geométrico y suite de 14 pruebas unitarias de especificación en CTest, manteniendo total independencia de widgets Qt y estricta conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0064-title-bar-integration/task.md`: Contrato vinculante de la microtarea canónica 0064.
  - `openspec/changes/module-ux-shell-task0064/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes de interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pruebas de dominio deterministas, rápidas y reproducibles en CTest; aserciones precisas sin mocks engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados (`enum class : uint8_t`), semántica de valor inmutable, const-correctness y exclusión de dependencias de GUI en el núcleo de dominio.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica en la barra superior.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), áreas de interacción táctiles y de ratón $\ge 48\times 48$ px, barra de título $\ge 48$ px de altura, foco visible con contraste adecuado y soporte de lector de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, presupuestos de memoria reducidos, cálculos analíticos en memoria (< 1 ms) y pruebas globales < 10 segundos en CTest.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, sanitización del título de ventana para no exponer URLs completas ni tokens privados.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Barra superior sobria integrada con azul oscuro de base y controles discretos.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles ampliados y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de cálculo geométrico y validación opera de forma 100% local en memoria, sin dependencias cloud ni telemetría remota.
  2. *Desacoplamiento estricto del núcleo de dominio*: `TitleBarTypes.h`, `TitleBarConfiguration` y `TitleBarValidator` permanecen en `vigilio_core` sin dependencias hacia `QWidget`, `QMainWindow`, `QApplication` ni APIs nativas del SO.
  3. *Invariante de no solapamiento de controles*: El algoritmo `calculateTitleBarLayout` garantiza formalmente que todos los botones y componentes asignados son mutuamente disjuntos ($\forall i \ne j, C_i \cap C_j = \emptyset$).
  4. *Invariante de suficiencia de área de arrastre*: El espacio horizontal disponible para mover la ventana física debe ser de al menos 120 px (estándar) o 160 px (Senior); cualquier distribución que asfixie este espacio se rechaza con `DraggableAreaInsufficient`.
  5. *Invariante de «Interfaz más fácil de usar»*: Tap targets y botones $\ge 48\times 48$ px, altura de barra de título $\ge 48$ px, márgenes protectores de 4 px entre controles.
  6. *Invariante de hit-testing determinista*: Cualquier punto dentro de la barra de título mapea a exactamente una zona válida, priorizando los controles interactivos y asignando el espacio libre a `DraggableSpace`.
  7. *Auditoría de lenguaje público*: Toda cadena o sugerencia de remediación emitida por `TitleBarValidator` debe formularse en español neutro sin términos internos proscritos.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`: Base cartesiana `LayoutRect` y zona `LayoutZone::TitleBar` reutilizada para coherencia global.
  - `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h`: Constante canónica `kCanonicalTitleBarHeight` sincronizada.
  - `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales oficiales de partida.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Acoplamiento de APIs de eventos nativos de Windows (`WM_NCHITTEST`) o Linux (Wayland XDG) en la capa de dominio.
    - *Mitigación*: La función analítica `hitTest(x, y)` clasifica puntos de manera puramente cartesiana, permitiendo a la capa de UI (`task0066`) mapear los resultados a valores nativos (`HTCAPTION`, `HTCLOSE`, etc.) sin contaminar el dominio.
  - *Riesgo 2*: Desbordamiento horizontal en pantallas pequeñas o ventanas angostas al activar la barra integrada.
    - *Mitigación*: Validación preventiva de anchura mínima ($400$ px estándar / $500$ px Senior) y degradación segura reportando `DimensionsBelowMinimum`.
  - *Riesgo 3*: Conflicto de posicionamiento entre controles a la izquierda y derecha.
    - *Mitigación*: Soporte formal mediante `TitleBarControlsPlacement`, invirtiendo el orden topológico de forma predecible sin solapamiento con el área de arrastre.

---

## Checklist evidence and handoff

- **Pipeline continuo en CMake / CTest**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 27/27 pruebas pasando exitosamente en CTest en ~5.55s (100% pass, exit code 0) en `apps/vigilio-browser/_build`.
- **Gobernanza con Prompt Builder CLI 0.6.3**:
  - `vigilio-prompt validate-references --project-root .`: 64 referencias canónicas validadas (`valid: true`, exit code 0).
  - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0190-module.ux-shell.task0064.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
  - `vigilio-prompt validate-decisions --project-root .`: Única decisión abierta `Q-007` (licencia jurídica), restringida a `release.public-release`, sin impacto sobre `module.ux-shell`.
- **Artefactos OpenSpec entregados para revisión humana**:
  - `openspec/changes/module-ux-shell-task0064/canonical-reference.yaml`
  - `openspec/changes/module-ux-shell-task0064/proposal.md`
  - `openspec/changes/module-ux-shell-task0064/design.md`
  - `openspec/changes/module-ux-shell-task0064/tasks.md`
  - `openspec/changes/module-ux-shell-task0064/execution-memory.md`
