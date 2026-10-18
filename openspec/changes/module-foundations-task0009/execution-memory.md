# Execution Memory — module-foundations-task0009

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `Qt dependency validation` en su etapa `domain` (modelo de dominio de dependencias, versión mínima canónica Qt >= 6.10.0, perfiles de requerimiento, reglas de validación en tiempo de configuración y runtime, y protocolo de degradación suave a headless), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0009-qt-dependency-validation/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `40764d725bcacd8dfbca3f01b4c85111f2ff515b3cd1945015616c06f1d47f8c`
  - Lock SHA-256: `e43e1de917fed96e4901bf647c534c943161168790187210f42dea557107ddec`
  - Versión de Prompt Builder: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y especificaciones del repositorio:
- `docs/skills/01-principios-de-ingenieria.md`: Aplicación de SOLID: responsabilidad única en la validación de dependencias; interfaz segregada por perfiles de requerimiento; inversión de dependencias desacoplando el núcleo de dominio de bibliotecas de interfaz gráfica; principios DRY, KISS y YAGNI.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: pruebas unitarias puras en tiempo de compilación y ejecución para la lógica de versiones; aislamiento headless; aserciones precisas; timeouts estrictos para prevenir bloqueos en CI; reporte honesto de `PASS`, `FAIL` y `NOT_EXECUTED`.
- `docs/skills/03-codigo-cpp-qt.md`: Buenas prácticas C++20; tipos estándar; inmutabilidad y const-correctness; separación estricta de capas (`src/core`, `src/storage`, `src/ui`); ownership claro de recursos; validación de entradas y rutas.
- `docs/skills/04-ux-ui-senior.md`: Coherencia visual sobria; ausencia formal de cambio visual directo para la capacidad `Qt dependency validation`; garantía de que la infraestructura de compilación no altera la apariencia ni los controles de la aplicación.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal; documentación explícita de impacto cero en la interfaz gráfica; garantía de que no se exponen términos técnicos en la UI del navegador; preservación de «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de construcción y pruebas; huella de memoria mínima (< 100 KB RAM); cero procesos de Chromium WebEngine iniciados durante la verificación de dependencias; prevención estricta de procesos zombies.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta; ausencia absoluta de descargas remotas de binarios, telemetría o servicios cloud; validación confinada a variables y rutas del sistema local.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico riguroso de compilación; mensajes estructurados en CMake y stderr en runtime; evidencia auditable de comandos ejecutados y códigos de salida.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación).
- `docs/skills/10-release-instaladores.md`: Criterios de empaquetado y validación de prerrequisitos de sistema para instaladores.
- `docs/skills/11-assets-svg-quality.md`: Preservación de la iconografía vectorial SVG y referencias de `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer nombres técnicos («Qt dependency validation», «Qt6», «QPA», «WebEngineWidgets», «Senior Mode», «Modo Senior», «CMake», «DAG», «Prompt Builder», «OpenSpec») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de controles, estados de interacción y accesibilidad.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Contrato de gobernanza de fases; verificación reproducible y respeto absoluto a las restricciones de escritura en allowed paths.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Versión mínima requerida: Qt >= 6.10.0 para todos los componentes de interfaz gráfica de usuario desktop (`Core`, `Widgets`, `WebEngineWidgets`).
  2. Desacoplamiento headless absoluto: `vigilio_core` y `vigilio_storage` tienen cero dependencias hacia Qt, garantizando compilación y pruebas 100% en entornos sin servidor gráfico.
  3. Gobernanza de configuración: conmutador `VIGILIO_REQUIRE_QT=OFF` permite degradación suave a build headless cuando falta Qt en desarrollo local; `VIGILIO_REQUIRE_QT=ON` fuerza error fatal en entornos de release/packaging.
  4. Inmunización de cabeceras externas: todos los directorios de inclusión de Qt se declaran como `SYSTEM` en CMake, impidiendo que advertencias de terceros interfieran con `-Werror`.
  5. Ausencia formal de cambio visual directo: la capacidad no añade componentes gráficos ni expone jerga técnica a los usuarios finales (cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`).
  6. Filosofía local-first estricta: cero conexiones de red para descarga de binarios o telemetría.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - `apps/vigilio-browser/src/core/**`
  - `apps/vigilio-browser/src/storage/**`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de paquetes de Qt6 WebEngine en entornos de CI headless o contenedores mínimos.
    *Mitigación*: Preservación estricta de la ruta de compilación headless (`VIGILIO_BUILD_GUI=OFF` y preset `core-debug`), asegurando que las suites de prueba de dominio y persistencia pasen al 100% sin requerir Qt.
  - *Riesgo*: Incompatibilidad binaria (ABI mismatch) entre la versión de cabeceras compilada y las bibliotecas dinámicas compartidas del sistema.
    *Mitigación*: Verificación en tiempo de ejecución de `qVersion()` frente a `QT_VERSION_STR` en `app.cpp` antes de instanciar ventanas, abortando con mensaje claro en `stderr` si existe divergencia mayor.
  - *Riesgo*: Lanzamiento accidental de la GUI en entornos Linux sin servidor gráfico (sin `DISPLAY`/`WAYLAND_DISPLAY`), provocando crash de Qt.
    *Mitigación*: Detección temprana en runtime de variables de display y manejo estructurado de error con código de salida 1.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 9, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad de lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0025-module.foundations.task0009.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0025-module.foundations.task0009.propose.lock.json", "valid": true, "checks": [...]}` (13/13 comprobaciones pasadas exitosamente)
  - Código de salida: `0`
- **Registro de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006, Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando exclusivamente el hito final de empaquetado `release.public-release`.
- **Verificación de construcción y pruebas headless**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: Compilación Ninja limpia sin advertencias bajo `-Werror`, 3/3 tests pasados (`core_units`, `storage_integration`, `vigilio_warnings_tests`) en 0.13s.
  - Código de salida: `0`
- **Estado de artefactos OpenSpec (`openspec/changes/module-foundations-task0009/`)**:
  - `canonical-reference.yaml`: validado conforme al esquema v1 de Prompt Builder.
  - `proposal.md`: redactado y completado con alcance, out of scope, contexto, pasos APPLY, DoD y commit propuesto.
  - `design.md`: redactado y completado con CURRENT/TARGET/GAPS, diseño de dominio, reglas, fallos y matriz de pruebas trazables.
  - `tasks.md`: redactado y completado con tareas atómicas TASK-001 a TASK-006 con criterios de verificación y comandos.
  - `execution-memory.md`: redactado y completado con memoria técnica y handoff.
- **Handoff**:
  - La propuesta para `module.foundations.task0009` (etapa `domain` de `Qt dependency validation`) está completamente elaborada, es coherente, respeta todos los contratos canónicos y está lista para la revisión y aprobación humana antes de proceder a la fase APPLY.
