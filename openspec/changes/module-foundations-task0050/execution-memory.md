# Execution Memory — module-foundations-task0050

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **implementation** de la capacidad **CI Linux job** en el módulo propietario `foundations` (`module.foundations.task0050`, microtarea canónica `0050 — Implementar la operación en una capa propietaria: CI Linux job`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0050-ci-linux-job/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0148`.
  - Pack: `module.foundations.task0050`.
  - Phase: `propose`.
  - Generated artifact: `0148-module.foundations.task0050.propose.md`.
  - Prompt SHA-256: `043ce709888e82f60dc620086a9d5bdc8f249acebce57f6537d6ff10087e4d5b`.
  - Lock SHA-256: `f491806fa0dfe282ae2a377a8a3e1b7bcafc6d8464072d0296731a925be4fcf8`.
  - Dependencias ejecutables: `module.foundations.task0049`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Separación estricta de responsabilidades (SOLID, KISS, YAGNI); desacoplamiento total entre la lógica del núcleo (`src/core/`), el almacenamiento y la interfaz gráfica; diseño de interfaces pequeñas y mantenibles.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas (unitarias para contratos de configuración y servicios en memoria, integración para sistema de archivos y respaldos, CTest headless), aserciones deterministas, eliminación de retardos fijos (`sleep`), fixtures con datos locales y reporte transparente.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar (`std::filesystem`, `std::chrono`, `std::thread`, `std::atomic`, `std::mutex`), const-correctness, tipado fuerte (`CiLinuxExecutionMode`, `CiLinuxJobStatus`, `CiLinuxPipelineStep`), encapsulamiento de datos inmutables y ausencia de dependencias hacia GUI en el núcleo de negocio.
- `docs/skills/04-ux-ui-senior.md`: Contrato de «sin cambio visual directo en la interfaz pública» para subsistemas de infraestructura de integración continua y calidad de compilación; preservación de los estados visuales en capas de diagnóstico.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior) en los componentes de interfaz, tap targets $\ge 48$ px, barra de herramientas $\ge 55$ px, escalado de iconos (22×22 a 32×32 px) y compatibilidad con lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, cuotas por trabajo (`maxMemoryBytes`, 4 GB de presupuesto máximo), límites de tiempo (`totalTimeoutSeconds`, 600 s) y tiempos de ejecución en CTest < 50 ms.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, contención en rutas y rechazo categórico de Directory Traversal (`..`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio y equilibrado.
  - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar con controles amplios y legibles.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de planificación, validación de etapas y ejecución de CI Linux opera de manera estrictamente local en el equipo o runner propio, sin dependencias cloud ni telemetría.
  2. *Ausencia formal de cambio visual directo en la interfaz pública*: La capacidad `CI Linux job` en la etapa `implementation` es infraestructura de automatización de pruebas y compilación en Linux; por consiguiente, no agrega widgets, botones ni menús en `BrowserWindow`.
  3. *Desacoplamiento estricto*: `vigilio_core` permanece 100% independiente de `QWidget` y `QApplication`, ejecutándose limpiamente en CTest sin necesidad de servidor gráfico (`DISPLAY`).
  4. *Tolerancia headless y política Zero-Zombie*: El arnés de pruebas y el orquestador de CI operan con timeouts finitos explícitos, sin subprocesos zombis, sin colgarse indefinidamente y liberando recursos inmediatamente al completarse o cancelarse.
  5. *Precedencia estricta de etapas (DAG)*: No se permite compilar sin previa configuración, ni ejecutar pruebas sin compilación previa exitosa. Las comprobaciones de formato y análisis estático preceden a la compilación.
  6. *Política Fail-Fast*: Si una etapa obligatoria falla, las subsiguientes se omiten ordenadamente salvo que se declare explícitamente `continueOnError`.
  7. *Seguridad de rutas y confinamiento*: Toda ruta de directorio de construcción debe ser relativa y validarse contra Directory Traversal (`..`) antes de cualquier acceso o borrado en el sistema de archivos.
  8. *Presupuesto de recursos*: Límite estricto de memoria de 4 GB (`maxMemoryBytes`) por trabajo y límite de 15 segundos por prueba unitaria en CTest.

- **Material reutilizable existente en el proyecto**:
  - `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`: Base sólida con las 8 etapas canónicas, estados de etapa, errores tipados, planes inmutables y token de cancelación cooperativa.
  - `apps/vigilio-browser/src/core/CiLinuxJobConfiguration.h/.cpp`: Planes canónicos oficiales (`canonicalDebugPlan` y `canonicalReleasePlan`).
  - `apps/vigilio-browser/src/core/CiLinuxJobValidator.h/.cpp`: Validador formal de planes, secuencias topológicas, rutas y copy público.
  - Patrón de arquitectura operativa probado en `CMakePresetsService`, `TestFixtureService` y `ClangTidyService`.

- **Riesgos y mitigaciones**:
  - *Riesgo*: Modificación o borrado accidental de archivos fuera del directorio de construcción durante el saneamiento.
    *Mitigación*: Validación obligatoria con `CiLinuxJobValidator::isSafeBuildPath`, impidiendo terminantemente `..` o rutas absolutas peligrosas.
  - *Riesgo*: Procesos zombis o bucles colgados durante la orquestación.
    *Mitigación*: Verificación periódica obligatoria de `CancellationToken*` antes de cada etapa e iteración, y timeouts explícitos por paso y globales.
  - *Riesgo*: Desbordamiento de memoria por asignaciones masivas en reportes.
    *Mitigación*: Límite estricto de 4 GB (`maxMemoryBytes`) verificado antes de iniciar la ejecución del plan.

---

## Checklist evidence and handoff

### Validación de compuertas documentales y canónicas
- `validate-references`: `valid: true` (50 referencias canónicas validadas).
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority":"docs-rules","references":50,"valid":true}`
- `verify-lock`: `valid: true` sobre `0148-module.foundations.task0050.propose.lock.json`.
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0148-module.foundations.task0050.propose.lock.json --project-root .`
  - Salida: `{"lockfile":"generated/prompts/full-project/0148-module.foundations.task0050.propose.lock.json","valid":true,"checks":[...]}`
- Estado del arnés de compilación y pruebas actual:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Resultado: 20/20 pruebas pasando exitosamente en 2.77 segundos.

### Tareas preparadas para APPLY (Fase Siguiente)
- `TASK-001` a `TASK-012` han quedado completamente especificadas con criterios medibles y comandos de verificación exactos en `tasks.md`.
- No existen decisiones `Q-*` abiertas ni dependencias bloqueadas.
- La propuesta técnica respeta escrupulosamente los límites de `foundations`, Local-First (ADR-001) y `docs/UX-COPY-PUBLICO.md`.

### Handoff para fase APPLY
Una vez aprobada esta propuesta por el usuario, la fase APPLY procederá con:
1. Actualización de `apps/vigilio-browser/src/core/CiLinuxJobTypes.h`.
2. Creación de `apps/vigilio-browser/src/core/CiLinuxJobService.h` y `CiLinuxJobService.cpp`.
3. Creación de `apps/vigilio-browser/tests/unit/CiLinuxJobServiceTests.cpp` (12 tests unitarios).
4. Actualización de `apps/vigilio-browser/CMakeLists.txt` con el nuevo servicio y test.
5. Ejecución del pipeline `check.sh` alcanzando 21/21 pruebas en verde.
6. Commit sugerido: `feat(foundations): implementation ci-linux-job`.
