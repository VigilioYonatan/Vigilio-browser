# Execution Memory — module-ux-shell-task0065

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `implementation` de la capacidad **title bar integration** (`module.ux-shell.task0065`), modelando de manera formal y desacoplada la capa de servicio operativo en C++20 (`TitleBarService`), responsable del ciclo de vida, mutaciones deterministas de estado, ajuste por densidad visual («Interfaz más fácil de usar» / estándar / compacta), reconfiguración ante cambios de anchura de ventana, anclaje de controles (derecha/izquierda), persistencia atómica local en disco, recuperación ante archivos corruptos, soporte de cancelación cooperativa Zero-Zombie (`CancellationToken`), aislamiento estricto de navegación en modo incógnito (ADR-001 / Decisión Q-001) y observabilidad desacoplada con copy público estricto (`docs/UX-COPY-PUBLICO.md`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0065-title-bar-integration/task.md`: Contrato vinculante de la microtarea canónica 0065.
  - `openspec/changes/module-ux-shell-task0065/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

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
  2. *Desacoplamiento estricto del núcleo de dominio*: `TitleBarService` permanece en `vigilio_core` sin dependencias hacia `QWidget`, `QMainWindow`, `QApplication` ni APIs nativas del SO.
  3. *Invariante de concurrencia segura*: Todas las lecturas y mutaciones de estado en `TitleBarService` están sincronizadas mediante `std::mutex`.
  4. *Invariante de no solapamiento de controles*: El algoritmo garantiza formalmente que todos los botones y componentes asignados son mutuamente disjuntos ($\forall i \ne j, C_i \cap C_j = \emptyset$).
  5. *Invariante de «Interfaz más fácil de usar»*: Tap targets y botones $\ge 48\times 48$ px, altura de barra de título $\ge 48$ px, márgenes protectores de 4 px entre controles.
  6. *Invariante de aislamiento de incógnito*: En sesiones privadas, no se realiza ninguna operación de escritura en disco; toda persistencia es rechazada.
  7. *Invariante Zero-Zombie*: Operaciones de cálculo y disco verifican `cancel->isCancelled()` abortando de inmediato si la cancelación es solicitada.
  8. *Auditoría de lenguaje público*: Toda cadena o sugerencia de remediación emitida por `TitleBarService` debe formularse en español neutro sin términos internos proscritos.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/TitleBarTypes.h`: Tipos y estructuras canónicas de dominio.
  - `apps/vigilio-browser/src/core/TitleBarConfiguration.h/.cpp`: Constantes canónicas y verificación de invariantes.
  - `apps/vigilio-browser/src/core/TitleBarValidator.h/.cpp`: Algoritmo de cálculo de layout, hit-testing y funciones auxiliares.
  - `apps/vigilio-browser/src/core/MainWindowLayoutService.cpp`: Patrón de serialización atómica, parser JSON embebido y sincronización multihilo.
  - `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales oficiales de partida.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Corrupción del archivo de preferencias de barra ante cortes de energía o caídas de proceso.
    - *Mitigación*: Técnica de guardado atómico mediante archivo temporal `.tmp` y renombramiento seguro, combinada con recuperación automática al plan canónico ante errores de parseo.
  - *Riesgo 2*: Fuga de datos de navegación privada a disco.
    - *Mitigación*: Guardia estricta de `isIncognitoMode()` en `saveStateToFile`, certificada por pruebas unitarias.
  - *Riesgo 3*: Bloqueos o colgadas multihilo bajo la política Zero-Zombie.
    - *Mitigación*: Consulta explícita a `CancellationToken` en puntos clave de computación y E/S.

---

## Checklist evidence and handoff

- `TASK-001` a `TASK-008`: Declaración e implementación de `TitleBarService.h` y `TitleBarService.cpp`.
- `TASK-009` y `TASK-010`: Registro en `apps/vigilio-browser/CMakeLists.txt` de la biblioteca y el target de pruebas.
- `TASK-011` a `TASK-014`: Implementación y verificación de la suite de 14 pruebas unitarias en `TitleBarServiceTests.cpp`.
- `TASK-015`: Ejecución exitosa de `apps/vigilio-browser/scripts/check.sh` con 29/29 pruebas en CTest al 100%.
- `TASK-016`: Validación de referencias canónicas y lockfile con `vigilio-prompt`.
- Entrega prevista para fase APPLY bajo el commit: `feat(ux-shell): implementation title-bar-integration`.
