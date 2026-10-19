# Execution Memory — module-ux-shell-task0063

Status: PROPOSED

## Objective and authority

- **Objetivo**: Formalizar la propuesta técnica integral para la etapa **tests** de la capacidad **main window layout** (`module.ux-shell.task0063`), estructurando una suite de pruebas automatizadas determinista, exhaustiva y de alta resiliencia que valide casos normales, fronteras dimensionales exactas, entradas no válidas y negativas, concurrencia masiva multihilo con 16 hilos, cancelación cooperativa Zero-Zombie mediante `CancellationToken`, recuperación transparente ante persistencia corrupta (JSON truncado/inválido), simulación de fallos de disco por permisos de solo lectura, aislamiento estricto de sesiones en modo incógnito, anclaje derecho de barra lateral, e integración reactiva en `BrowserWindow` con tap targets $\ge 48$ px y estricto apego a `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0063-main-window-layout/task.md`: Contrato vinculante de la microtarea canónica 0063.
  - `openspec/changes/module-ux-shell-task0063/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, responsabilidad única y separación limpia entre el servicio de layout (`vigilio_core`) y la ventana gráfica (`vigilio_ui`).
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para lógica y contratos, integración para persistencia local y concurrencia multihilo, y arnés UI rápido); uso de fixtures deterministas, aserciones verificables y ausencia de sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Ownership claro en Qt mediante jerarquías de widgets, uso de RAII, tipos inmutables de C++20 y exclusión estricta de operaciones bloqueantes en el hilo principal de la interfaz.
- `docs/skills/04-ux-ui-senior.md`: Diseño sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica de barra lateral, omnibox y tarjetas.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Directrices de la «Interfaz más fácil de usar» (Modo Senior), tap targets $\ge 48$ px, barra de navegación $\ge 56$ px, iconos escalados a $32\times 32$ px (28 px en barra lateral), navegación por teclado, indicadores de foco de alto contraste y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta en el uso de memoria RAM, ejecución ultrarrápida en CTest (< 5 s en total) y prevención de instanciación innecesaria de componentes pesados de Chromium WebEngine durante pruebas unitarias y de servicio.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cero persistencia de datos de navegación o estado de layout en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato canónico con Clang-Format, compilación limpia con `-Werror` bajo GCC/Clang, y análisis estático con Clang-Tidy.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes visibles o accesibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la ventana principal con barra lateral, barra de herramientas y buscador centrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios, tipografía escalada y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de pruebas opera 100% en local, sin llamadas de red ni sincronización remota.
  2. *Invariante de no solapamiento geométrico*: Las áreas asignadas a las zonas de interfaz (`Sidebar`, `ToolBar`, `TabStrip`, `Banner`, `Content`, `StatusBar`) son mutuamente excluyentes ($\forall i \ne j, Z_i \cap Z_j = \emptyset$) y garantizan un área central útil de al menos $400\times 250$ px.
  3. *Invariante de accesibilidad y tap targets*: En la «Interfaz más fácil de usar», todos los controles interactivos primarios cumplen `minimumHeight() >= 48` px.
  4. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o persistir el estado de la ventana en disco.
  5. *Invariante de resiliencia y recuperación*: Ante archivos de layout corruptos, truncados o no válidos, el servicio restablece de inmediato `canonicalDefaultLayout()` sin lanzar excepciones no controladas ni provocar caídas de proceso.
  6. *Invariante de lenguaje público*: Ninguna etiqueta visible, mensaje de error o pista de remediación expone nombres internos de ingeniería.
  7. *Invariante Zero-Zombie*: Ninguna prueba genera procesos huérfanos o colgados esperando EOF.
- **Material reutilizable**:
  - `MainWindowLayoutTypes.h`, `MainWindowLayoutConfiguration` y `MainWindowLayoutValidator` de `src/core/`.
  - `MainWindowLayoutService` de `src/core/`.
  - `BrowserWindow` de `src/ui/`.
  - Arnés de pruebas de unidad e integración existente en `tests/unit/MainWindowLayoutTests.cpp`, `tests/unit/MainWindowLayoutServiceTests.cpp` y `tests/ui/WindowTests.cpp`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Entorno de CI sin servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY` ausente).
    *Mitigación*: `WindowTests.cpp` detecta de forma determinista la ausencia de servidor gráfico y reporta `NOT_EXECUTED` de manera limpia, permitiendo que las pruebas de dominio y servicio en C++20 se ejecuten con éxito total.
  - *Riesgo*: Condiciones de carrera o contención excesiva en pruebas con 16 hilos.
    *Mitigación*: Sincronización thread-safe estricta en `MainWindowLayoutService` con `std::mutex` y `std::lock_guard`.

---

## Checklist evidence and handoff

- **Evidencia documental**:
  - `openspec/changes/module-ux-shell-task0063/proposal.md`: Propuesta integral cubriendo los 14 apartados canónicos requeridos.
  - `openspec/changes/module-ux-shell-task0063/design.md`: Diseño detallado de arquitectura de pruebas, estados y resiliencia.
  - `openspec/changes/module-ux-shell-task0063/tasks.md`: 18 tareas ordenadas y verificables para la fase APPLY.
  - `openspec/changes/module-ux-shell-task0063/execution-memory.md`: Memoria de ejecución y handoff formal.
- **Validaciones automáticas ejecutadas**:
  - `vigilio-prompt validate-references --project-root .`: Exit code 0 (`valid: true`, 63 referencias).
  - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0187-module.ux-shell.task0063.propose.lock.json --project-root .`: Exit code 0 (`valid: true`, 13 checks passed).
  - `ctest --test-dir apps/vigilio-browser/build-ci-debug`: 27/27 pruebas pasando al 100%.
- **Handoff a fase APPLY**:
  - Con la propuesta formalizada y validada en `openspec/changes/module-ux-shell-task0063/`, el cambio queda en estado `READY` para revisión humana previa a la ejecución de `apply`.
