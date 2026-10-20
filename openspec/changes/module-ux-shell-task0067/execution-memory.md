# Execution Memory — module-ux-shell-task0067

Status: PROPOSED

## Objective and authority

- **Objetivo**: Formalizar la propuesta técnica integral para la etapa **tests** de la capacidad **title bar integration** (`module.ux-shell.task0067`), estructurando una suite de pruebas automatizadas determinista, exhaustiva y de alta resiliencia que valide casos normales, fronteras dimensionales exactas, entradas no válidas y negativas, hit-testing de esquinas y bordes exteriores, concurrencia masiva multihilo con 16 hilos, cancelación cooperativa Zero-Zombie mediante `CancellationToken`, recuperación transparente ante persistencia corrupta (JSON truncado/inválido), simulación de fallos de disco por permisos de solo lectura, aislamiento estricto de sesiones en modo incógnito, alternancia continua de colocación de controles, e integración reactiva en `BrowserWindow` con tap targets $\ge 48\times 48$ px y estricto apego a `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md`: Contrato vinculante de la microtarea canónica 0067.
  - `openspec/changes/module-ux-shell-task0067/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única y separación limpia entre el servicio de barra de título (`vigilio_core`) y la ventana gráfica (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para lógica analítica y contratos, integración para persistencia local y concurrencia multihilo, y arnés UI rápido); uso de fixtures deterministas, aserciones verificables y ausencia de sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro en Qt mediante jerarquías de widgets, uso de RAII, tipos inmutables de C++20 y exclusión estricta de operaciones bloqueantes en el hilo principal de la interfaz.
- `docs/skills/04-ux-ui-senior.md`: Diseño sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica de la barra de título, controles y área de arrastre.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de la «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48\times 48$ px, altura de barra $\ge 48$ px, icono escalado a $24\times 24$ px, navegación por teclado, indicadores de foco de alto contraste y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, ejecución ultrarrápida en CTest (< 5 s en total) y prevención de instanciación innecesaria de componentes pesados de Chromium WebEngine durante pruebas unitarias y de servicio.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cero persistencia de datos de navegación o estado de barra en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato canónico con Clang-Format, compilación limpia con `-Werror` bajo GCC/Clang, y análisis estático con Clang-Tidy.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la barra de título superior sobria e integrada.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de pruebas opera 100% en local, sin llamadas de red ni sincronización remota.
  2. *Invariante de no solapamiento geométrico*: Las áreas asignadas a los controles interactivos de la barra de título son mutuamente excluyentes ($\forall i \ne j, \text{control}_i \cap \text{control}_j = \emptyset$) y la caja de controles no solapa el área ergonómica de arrastre ($\text{controlsBoxRect} \cap \text{draggableRect} = \emptyset$).
  3. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», la barra de título tiene altura $\ge 48$ px y todos los botones interactivos cumplen `rect.width >= 48` y `rect.height >= 48` px.
  4. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o persistir el estado de la barra de título en disco.
  5. *Invariante de resiliencia y recuperación*: Ante archivos de persistencia corruptos, truncados o no válidos, el servicio restablece de inmediato `canonicalDefaultTitleBar()` sin lanzar excepciones no controladas ni provocar caídas de proceso.
  6. *Invariante de lenguaje público*: Ninguna etiqueta visible, mensaje de error, tooltip o pista de remediación expone nombres internos de ingeniería.
  7. *Invariante Zero-Zombie*: Ninguna prueba genera procesos huérfanos o colgados esperando EOF; la cancelación cooperativa responde en < 10 ms.
- **Material reutilizable**:
  - `TitleBarTypes.h`, `TitleBarConfiguration` y `TitleBarValidator` de `src/core/`.
  - `TitleBarService` de `src/core/`.
  - `BrowserWindow` de `src/ui/`.
  - Arnés de pruebas de unidad e integración existente en `tests/unit/TitleBarIntegrationTests.cpp`, `tests/unit/TitleBarServiceTests.cpp` y `tests/ui/WindowTests.cpp`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    *Mitigación*: `WindowTests.cpp` detecta de forma determinista la ausencia de servidor gráfico y reporta `NOT_EXECUTED` de manera limpia, permitiendo que las pruebas de dominio y servicio en C++20 se ejecuten con éxito total.
  - *Riesgo*: Condiciones de carrera o contención excesiva en pruebas con 16 hilos.
    *Mitigación*: Sincronización thread-safe estricta en `TitleBarService` con `std::mutex` y `std::lock_guard`.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0067/canonical-reference.yaml`: Referencia canónica formal en modo `reference-only`.
  - `openspec/changes/module-ux-shell-task0067/proposal.md`: Propuesta integral cubriendo los 14 apartados canónicos requeridos.
  - `openspec/changes/module-ux-shell-task0067/design.md`: Diseño detallado de arquitectura de pruebas, estados y resiliencia.
  - `openspec/changes/module-ux-shell-task0067/tasks.md`: 17 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0067/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - `bash apps/vigilio-browser/scripts/check.sh`: Compilación limpia con `-Werror` y 29/29 pruebas pasando al 100% en CTest (~3.72 s).
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y validada en `openspec/changes/module-ux-shell-task0067/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
