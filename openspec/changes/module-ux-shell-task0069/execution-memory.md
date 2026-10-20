# Execution Memory — module-ux-shell-task0069

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `implementation` de la capacidad **tab strip density** (`module.ux-shell.task0069`), modelando de manera formal y desacoplada la capa de servicio operativo en C++20 (`TabStripDensityService`), responsable del ciclo de vida, mutaciones deterministas de la colección de pestañas, ajuste por densidad visual («Interfaz más fácil de usar» / estándar / compacta), reconfiguración ante cambios de anchura de ventana, precedencia de pestañas fijadas ($x_{\text{pinned}} < x_{\text{regular}}$), selección contigua inteligente al cerrar pestañas, control de sonido y silenciamiento, transiciones de ahorro de memoria («Liberar memoria de pestañas inactivas» / «Pestaña en reposo»), desbordamiento horizontal y desplazamiento, persistencia atómica local en disco, recuperación ante archivos corruptos, soporte de cancelación cooperativa Zero-Zombie (`CancellationToken`), aislamiento estricto de navegación en modo incógnito (ADR-001 / Decisión Q-001) y observabilidad desacoplada con copy público estricto (`docs/UX-COPY-PUBLICO.md`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0069-tab-strip-density/task.md`: Contrato vinculante de la microtarea canónica 0069.
  - `openspec/changes/module-ux-shell-task0069/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes de interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pruebas de dominio deterministas, rápidas y reproducibles en CTest; aserciones precisas sin mocks engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados (`enum class : uint8_t`), semántica de valor inmutable, const-correctness y exclusión de dependencias de GUI en el núcleo de dominio.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración armónica en la tira de pestañas.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), áreas de interacción táctiles y de ratón $\ge 48\times 48$ px, tira de pestañas $\ge 48$ px de altura, foco visible con contraste adecuado, separación generosa (4 px) y soporte de lector de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, presupuestos de memoria reducidos, cálculos analíticos en memoria (< 1 ms), compresión elástica lineal $\mathcal{O}(N)$ y pruebas globales < 10 segundos en CTest.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, sanitización de títulos y URLs de pestañas para no exponer contraseñas ni tokens privados.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**, **«Liberar memoria de pestañas inactivas»** / **«Pestaña en reposo»**, **«Ahorro de memoria»** y **«Cargar pestañas cuando las necesites»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Tira de pestañas superior sobria integrada con azul oscuro de base y controles discretos.
  - `/resultado/02-vigilio-tools-productivity.png`: Distribución de pestañas y herramientas de productividad.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles ampliados, botones $\ge 48\times 48$ px y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de ciclo de vida de pestañas y cálculo espacial opera de forma 100% local en memoria C++20, sin dependencias cloud ni telemetría remota.
  2. *Desacoplamiento estricto del núcleo de dominio*: `TabStripDensityService` permanece en `vigilio_core` sin dependencias hacia `QWidget`, `QTabBar`, `QTabWidget`, `BrowserWindow` ni subprocesos Chromium WebEngine.
  3. *Invariante de concurrencia segura*: Todas las lecturas y mutaciones de estado en `TabStripDensityService` están sincronizadas mediante `std::mutex`.
  4. *Invariante de precedencia de pestañas fijadas*: Toda pestaña fijada precede en el orden de disposición a las pestañas regulares ($\forall i \in \text{pinned}, \forall j \in \text{regular}, x_i < x_j$) y conserva su ancho canónico sin sufrir compresión elástica.
  5. *Invariante de no solapamiento de componentes*: Las áreas calculadas de las pestañas son mutuamente excluyentes ($\forall i \ne j, \text{tab}_i \cap \text{tab}_j = \emptyset$) y los elementos internos no colisionan.
  6. *Invariante de «Interfaz más fácil de usar»*: Tap targets y botones de acción $\ge 48\times 48$ px, altura de tira $\ge 48$ px, margen protector de 4 px entre pestañas y ancho mínimo de pestaña regular de 140 px para impedir compresión ilegible.
  7. *Invariante de aislamiento de incógnito*: En sesiones privadas, no se realiza ninguna operación de escritura en disco; toda persistencia es rechazada inmediatamente.
  8. *Invariante Zero-Zombie*: Operaciones de cálculo y persistencia en disco verifican `cancel->isCancelled()`, abortando de inmediato si la cancelación es solicitada.
  9. *Auditoría de lenguaje público*: Toda cadena o sugerencia de remediación emitida por `TabStripDensityService` debe formularse en español neutro sin términos internos proscritos.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/TabStripDensityTypes.h`: Tipos y estructuras canónicas de dominio.
  - `apps/vigilio-browser/src/core/TabStripDensityConfiguration.h/.cpp`: Constantes canónicas y verificación de invariantes.
  - `apps/vigilio-browser/src/core/TabStripDensityValidator.h/.cpp`: Algoritmo de cálculo de layout, hit-testing y funciones auxiliares de textos accesibles en español neutro.
  - `apps/vigilio-browser/src/core/TitleBarService.cpp`: Patrón de serialización atómica, parser JSON embebido y sincronización multihilo.
  - `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales oficiales de partida.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Corrupción del archivo de preferencias de pestañas ante cortes de energía o caídas de proceso.
    - *Mitigación*: Técnica de guardado atómico mediante archivo temporal `.tmp` y renombramiento seguro, combinada con recuperación automática al plan canónico por defecto ante errores de parseo.
  - *Riesgo 2*: Fuga de datos de navegación privada a disco en sesiones de incógnito.
    - *Mitigación*: Comprobación ineludible de `isIncognitoSession` antes de cualquier operación de escritura, abortando de inmediato sin tocar el sistema de archivos.
  - *Riesgo 3*: Degradación del hilo de interfaz ante recálculos continuos de compresión elástica.
    - *Mitigación*: Algoritmo determinista de complejidad lineal $\mathcal{O}(N)$ que completa la distribución espacial en microsegundos y soporta cancelación cooperativa mediante `CancellationToken`.

---

## Checklist evidence and handoff

- **Evidencia documental y validaciones ejecutadas**:
  - `canonical-reference.yaml` validado: referencia canónica en modo `reference-only` apuntando a `module.ux-shell.task0069`.
  - `proposal.md` completado: especificación formal exhaustiva de `TabStripDensityService`, interfaces, invariantes, UX/UI, seguridad, matriz de 17 pruebas unitarias y commit sugerido `feat(ux-shell): implementation tab-strip-density`.
  - `design.md` completado: análisis CURRENT/TARGET/GAPS, diagrama de arquitectura de 3 capas (UI, Servicio Operativo, Dominio Matemático), compatibilidad C++20, Local-First y plan de rollback.
  - `tasks.md` completado: 18 tareas ordenadas y verificables con identificadores `TASK-001` a `TASK-018`, criterios de verificación unívocos y comandos ejecutables.
  - `execution-memory.md` completado: registro riguroso de objetivos, autoridades canónicas, skills leídas, invariantes, material reutilizable y gestión de riesgos.
- **Estado de las pruebas y build actual**:
  - `apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total: 30 de 30 pruebas pasando al 100% en CTest en ~3.68 segundos con `-Werror` activado.
- **Handoff a la fase subsiguiente**:
  - La fase PROPOSE para `module.ux-shell.task0069` ha concluido de forma limpia y rigurosa sin tocar código de producción en `apps/**`, respetando estrictamente las restricciones de escritura.
  - El change en `openspec/changes/module-ux-shell-task0069` queda en estado `PROPOSED`, listo para revisión humana y posterior aprobación para la fase APPLY.
