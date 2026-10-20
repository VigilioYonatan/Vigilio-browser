# Execution Memory — module-ux-shell-task0071

Status: PROPOSED

## Objective and authority

- **Objetivo**: Formalizar la propuesta técnica integral para la etapa **tests** de la capacidad **tab strip density** (`module.ux-shell.task0071`), estructurando una suite de pruebas automatizadas determinista, exhaustiva y de alta resiliencia que valide casos normales, fronteras dimensionales exactas ($399$ px, $200$ px, $0$ px, valores negativos), violaciones de altura mínima en la «Interfaz más fácil de usar» ($< 48$ px) y tap targets insuficientes, compresión elástica extrema (100 pestañas en contenedor estrecho de $400$ px) con activación de desbordamiento horizontal (`TabStripOverflowMode::Scrollable`), saturación por pestañas fijadas, hit-testing cartesiano de vértices y bordes exteriores, sanitización hostil de títulos con credenciales embebidas y secuencias de control, concurrencia masiva multihilo con 16 hilos compitiendo ($\ge 200$ iters/hilo), cancelación cooperativa Zero-Zombie mediante `CancellationToken`, recuperación transparente ante persistencia corrupta (fuzzing de JSON truncado/inválido), simulación de fallos de disco por permisos de solo lectura, aislamiento estricto de sesiones en modo incógnito (0 escrituras en disco), mutaciones de pestañas en ráfaga (50 operaciones alternadas), e integración reactiva en `BrowserWindow` con tap targets $\ge 48\times 48$ px en el botón de nueva pestaña y estricto apego a `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0071-tab-strip-density/task.md`: Contrato vinculante de la microtarea canónica 0071.
  - `openspec/changes/module-ux-shell-task0071/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única y separación limpia entre el servicio de tira de pestañas (`vigilio_core`) y la ventana gráfica (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para lógica analítica y contratos, integración para persistencia local y concurrencia multihilo, y arnés UI rápido); uso de fixtures deterministas, aserciones verificables y ausencia de sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro en Qt mediante jerarquías de widgets, uso de RAII, tipos inmutables de C++20 y exclusión estricta de operaciones bloqueantes en el hilo principal de la interfaz.
- `docs/skills/04-ux-ui-senior.md`: Diseño sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica de la tira de pestañas, pestañas fijadas, pestañas regulares y botón de nueva pestaña.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de la «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48\times 48$ px en el botón de nueva pestaña, altura de tira $\ge 48$ px, botones de cierre interactivos con área expandida $\ge 24\times 24$ px, navegación por teclado, indicadores de foco de alto contraste y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, ejecución ultrarrápida en CTest (< 5 s en total) y prevención de instanciación innecesaria de componentes pesados de Chromium WebEngine durante pruebas unitarias y de servicio.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cero persistencia de datos de navegación o estado de tira de pestañas en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato canónico con Clang-Format, compilación limpia con `-Werror` bajo GCC/Clang, y análisis estático con Clang-Tidy.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**, **«Nueva pestaña»**, **«Cerrar pestaña»**, **«Silenciar pestaña»**, **«Pestaña en reposo»**, **«Cargando…»** y **«No se pudo cargar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la tira de pestañas superior integrada, pestañas sobrias e isotipo oficial.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de pruebas opera 100% en local, sin llamadas de red ni sincronización remota.
  2. *Invariante de no solapamiento geométrico*: Las áreas asignadas a pestañas adyacentes son mutuamente excluyentes ($\forall i \ne j \implies \text{tabs}[i].\text{rect} \cap \text{tabs}[j].\text{rect} = \emptyset$) y ninguna pestaña solapa el botón de nueva pestaña ($\text{tabs}[i].\text{rect} \cap \text{newTabButtonRect} = \emptyset$).
  3. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», la tira de pestañas tiene altura $\ge 48$ px y el botón de nueva pestaña cumple `newTabButtonRect.width >= 48` y `newTabButtonRect.height >= 48` px.
  4. *Invariante de precedencia de pestañas fijadas*: Toda pestaña fijada (`TabPinState::Pinned`) precede estrictamente a cualquier pestaña no fijada en el orden de visualización horizontal.
  5. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o persistir el estado de la tira de pestañas en disco (0 llamadas de E/S a almacenamiento secundario).
  6. *Invariante de resiliencia y recuperación*: Ante archivos de persistencia corruptos, truncados o no válidos, el servicio restablece de inmediato `canonicalDefaultDensity()` sin lanzar excepciones no controladas ni provocar caídas de proceso.
  7. *Invariante de lenguaje público*: Ninguna etiqueta visible, mensaje de error, tooltip o pista de remediación expone nombres internos de ingeniería.
  8. *Invariante Zero-Zombie*: Ninguna prueba genera procesos huérfanos o colgados esperando EOF; la cancelación cooperativa responde en < 10 ms.
- **Material reutilizable**:
  - `TabStripDensityTypes.h`, `TabStripDensityConfiguration` y `TabStripDensityValidator` de `src/core/`.
  - `TabStripDensityService` de `src/core/`.
  - `BrowserWindow` de `src/ui/`.
  - Arnés de pruebas de unidad e integración existente en `tests/unit/TabStripDensityTests.cpp`, `tests/unit/TabStripDensityServiceTests.cpp` y `tests/ui/WindowTests.cpp`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    *Mitigación*: `WindowTests.cpp` detecta de forma determinista la ausencia de servidor gráfico y reporta `NOT_EXECUTED` de manera limpia, permitiendo que las pruebas de dominio y servicio en C++20 se ejecuten con éxito total.
  - *Riesgo*: Condiciones de carrera o contención excesiva en pruebas con 16 hilos compitiendo.
    *Mitigación*: Sincronización thread-safe estricta en `TabStripDensityService` con `std::mutex` y `std::lock_guard`.
  - *Riesgo*: Cuelgues o flakiness en pruebas de cancelación cooperativa.
    *Mitigación*: Uso de `CancellationToken` con aserciones deterministas de tiempo finito (< 10 ms) y verificación de consistencia del plan.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0071/canonical-reference.yaml`: Referencia canónica formal en modo `reference-only`.
  - `openspec/changes/module-ux-shell-task0071/proposal.md`: Propuesta integral cubriendo los 14 apartados canónicos requeridos.
  - `openspec/changes/module-ux-shell-task0071/design.md`: Diseño detallado de arquitectura de pruebas, estados y resiliencia.
  - `openspec/changes/module-ux-shell-task0071/tasks.md`: 18 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0071/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - `ctest --test-dir build --output-on-failure`: 31/31 pruebas pasando al 100% en CTest (~3.81 s).
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y validada en `openspec/changes/module-ux-shell-task0071/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
