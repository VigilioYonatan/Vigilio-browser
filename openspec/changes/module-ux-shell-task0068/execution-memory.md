# Execution Memory — module-ux-shell-task0068

Status: PROPOSED

## Objective and authority

- **Objetivo**: Formalizar la propuesta técnica integral para la etapa **domain** de la capacidad **tab strip density** (`module.ux-shell.task0068`), estructurando un modelo de dominio declarativo, desacoplado y fuertemente tipado en C++20 puro que defina la taxonomía formal (`TabStripDensityTypes.h`), la configuración canónica e invariantes geométricas por densidad (`TabStripDensityConfiguration`), el algoritmo determinista de distribución analítica con compresión elástica y desbordamiento horizontal, zonificación espacial de pruebas de impacto (*hit-testing*), sanitización de privacidad y funciones de lenguaje público y accesibilidad en español neutro cumpliendo estrictamente con `docs/UX-COPY-PUBLICO.md` (`TabStripDensityValidator`), respaldado por una suite de 16 pruebas unitarias trazables en CTest.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0068-tab-strip-density/task.md`: Contrato vinculante de la microtarea canónica 0068.
  - `openspec/changes/module-ux-shell-task0068/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única y separación limpia entre el modelo de dominio analítico (`vigilio_core`) y los widgets gráficos de Qt (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para lógica analítica, contratos e invariantes geométricas); fixtures deterministas, aserciones verificables y ausencia de sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro, uso riguroso de RAII, tipos inmutables de C++20 (`constexpr`, `enum class : uint8_t`, semántica de valor) y exclusión total de operaciones bloqueantes en el hilo de interfaz.
- `docs/skills/04-ux-ui-senior.md`: Diseño sobrio, profesional y de escritorio; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica entre la pestaña activa, pestañas inactivas y controles auxiliares.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de la «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48\times 48$ px, altura de tira $\ge 48$ px, icono de pestaña escalado a $24\times 24$ px, navegación por teclado, indicadores de foco continuo de 2 px de alto contraste y legibilidad tipográfica sin compresión extrema (mínimo ineludible de 140 px por pestaña).
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, ejecución de cálculo en una sola pasada $O(N)$, ejecución ultrarrápida en CTest (< 50 ms) y prevención de instanciación innecesaria de Chromium WebEngine en pruebas de dominio.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y sanitización proactiva de títulos y URLs de pestañas en modo incógnito (cero filtración de credenciales o tokens en logs y UI).
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato canónico con Clang-Format, compilación limpia con `-Werror` bajo GCC/Clang, y análisis estático con Clang-Tidy.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Criterios de release y empaquetado seguro.
- `docs/skills/11-assets-svg-quality.md`: Calidad de assets vectoriales Lucide SVG (viewBox `0 0 24 24`, trazos de 1.8 px).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**, **«Liberar memoria de pestañas inactivas»** / **«Pestaña en reposo»**, **«Ahorro de memoria»** y **«Cargar pestañas cuando las necesites»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la tira de pestañas superior sobria e integrada.
  - `/resultado/02-vigilio-tools-productivity.png`: Distribución de pestañas y herramientas de productividad.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de cálculo geométrico opera 100% en local en memoria C++20, sin llamadas de red ni sincronización remota.
  2. *Invariante de no solapamiento geométrico*: Las áreas asignadas a las pestañas son mutuamente excluyentes ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$) y los elementos internos de cada pestaña no colisionan entre sí ($\text{favicon} \cap \text{title} = \emptyset$, $\text{title} \cap \text{close} = \emptyset$).
  3. *Invariante de precedencia de pestañas fijadas*: Todas las pestañas fijadas (*pinned*) preceden en coordenadas $X$ a las pestañas regulares ($x_{\text{pinned}} < x_{\text{regular}}$) y conservan su ancho canónico sin sufrir compresión elástica.
  4. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», la tira de pestañas tiene altura $\ge 48$ px y todos los controles individuales cumplen `rect.width >= 48` y `rect.height >= 48` px en su área de captura.
  5. *Invariante de compresión elástica y desbordamiento*: Las pestañas regulares se comprimen uniformemente hasta `minTabWidth`; si la compresión excede este límite, se activa el modo de desbordamiento horizontal (`TabStripOverflowMode::Scrollable`).
  6. *Invariante de protección de cierre*: En densidades estándar y compacta, las pestañas inactivas angostas (< 90 px) ocultan el botón de cierre para evitar cierres accidentales; en Modo Senior, el botón de cierre permanece siempre visible y protegido.
  7. *Invariante de privacidad en incógnito*: Los títulos de las pestañas son sanitizados eliminando credenciales embebidas, tokens y hashes sensibles.
  8. *Invariante de lenguaje público*: Ninguna etiqueta visible, mensaje de error, tooltip o pista de remediación expone nombres internos de ingeniería.
- **Material reutilizable**:
  - `MainWindowLayoutTypes.h` (`LayoutDensity`, `LayoutRect`, `LayoutZone::TabStrip`).
  - `TabModel.h` (`struct Tab`).
  - `TitleBarConfiguration` y `TitleBarValidator` como patrones canónicos de diseño geométrico desacoplado.
  - Arnés de pruebas de CTest en `apps/vigilio-browser/CMakeLists.txt`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Acoplamiento con el framework de widgets Qt en la capa de dominio.
    *Mitigación*: Implementación en C++20 puro sin cabeceras gráficas de Qt ni WebEngine en `vigilio_core`.
  - *Riesgo*: Compresión extrema de pestañas provocando ilegibilidad.
    *Mitigación*: Imposición de límites mínimos estrictos (`minTabWidth`) y activación automática de desbordamiento horizontal.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0068/canonical-reference.yaml`: Referencia canónica formal en modo `reference-only`.
  - `openspec/changes/module-ux-shell-task0068/proposal.md`: Propuesta integral cubriendo todos los apartados canónicos requeridos.
  - `openspec/changes/module-ux-shell-task0068/design.md`: Diseño detallado de arquitectura de dominio, estados, compresión y desbordamiento.
  - `openspec/changes/module-ux-shell-task0068/tasks.md`: 19 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0068/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - Ejecución de pruebas previas en CTest: 29/29 pruebas pasando al 100% con exit code 0 (~3.66 s).
  - Verificación de ausencia de tecnicismos proscritos en la documentación del cambio conforme a `docs/UX-COPY-PUBLICO.md`.
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y validada en `openspec/changes/module-ux-shell-task0068/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
