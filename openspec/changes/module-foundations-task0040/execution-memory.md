# Execution Memory — module-foundations-task0040

Status: PROPOSED

## Objective and authority

- **Objetivo**: Formular, estructurar y verificar la propuesta técnica completa (fase PROPOSE) para la etapa **tests** de la capacidad **test fixture foundation** en el módulo `foundations` de Vigilio Browser, documentada canónicamente en `docs/rules/01-modules/foundations/micro/0040-test-fixture-foundation/task.md`.
- **Autoridad canónica**:
  - `docs/rules/` y `docs/rules/01-modules/foundations/` constituyen la única fuente de autoridad normativa y de diseño.
  - Prompt Builder CLI (`v0.6.3`) rige la validación de locks, referencias y elegibilidad.
  - No se añadieron dependencias remotas, telemetría ni lógica en la nube (estricto cumplimiento de ADR-001 / Decisión Q-001).
- **Metadatos de ejecución**:
  - Secuencia de ejecución: `0118`.
  - Pack: `module.foundations.task0040`.
  - Fase: `propose`.
  - Prompt SHA-256: `6cb542e83a362d7b4e9ae0613dc0e209061e9e7ef1d1b3716ba2005980ef218a`.
  - Lock SHA-256: `47736f35a1bf629b102c3b0bff881866bfeeb8b5334166f07d3e53db2e12f179`.
  - Dependencias ejecutables: `module.foundations.task0039`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad, separación estricta de responsabilidades (SOLID), KISS y YAGNI; desacoplamiento estricto entre la infraestructura de fixtures en `src/core` y la interfaz gráfica en `src/ui`.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; integración de fixtures locales RAII deterministas, reporte honesto de `NOT_EXECUTED` ante ausencia de servidor gráfico y erradicación de sleeps artificiales mediante espera observable.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), gestión de memoria determinista y aislamiento entre lógica del núcleo y renderizado gráfico.
- `docs/skills/04-ux-ui-senior.md`: Estilo moderno, sobrio y profesional sin brillos artificiales; alineación con las referencias visuales `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Respeto estricto a las pautas de accesibilidad y soporte de escalado dinámico en la denominación oficial «Interfaz más fácil de usar».
- `docs/skills/06-rendimiento-y-ram.md`: Presupuestos estrictos de recursos; cuota de disco limitada a 50 MB por fixture (`maxTempDirSizeBytes`), ejecución de CTest en < 1 segundo para la suite completa y cero procesos colgados (Zero-Zombie).
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes exteriores, prevención contra Directory Traversal (`..`) y validación rigurosa de rutas temporales con `isSafeTestDirectory`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad continua mediante Clang-Format, compilación con `-Werror` y cero advertencias.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación física tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Preservación de la integridad del empaquetado de producción sin filtración de artefactos de prueba temporales.
- `docs/skills/11-assets-svg-quality.md`: Respeto a los iconos SVG vectoriales compilados en `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Test Fixture», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de la denominación oficial «Interfaz más fácil de usar».
- `docs/rules/03-adr/ADR-001-local-only.md`: Local-First constitucional; prohibición de telemetría y sincronización cloud no solicitada.
- `docs/rules/03-adr/ADR-002-qt-stack.md`: Núcleo en C++20 y GUI desacoplada en Qt Widgets.
- `docs/rules/03-adr/ADR-005-e2e-serial.md`: Pruebas pesadas en serie y suites unitarias/rápidas independientes de Chromium.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la infraestructura de fixtures opera de forma estrictamente local en el almacenamiento temporal del host, sin conexiones remotas ni telemetría.
  2. *Contención estricta de rutas temporales (`isSafeTestDirectory`)*: Cualquier carpeta administrada o purgada debe residir obligatoriamente en `std::filesystem::temp_directory_path()`, comenzar con `vigilio_test_` y no coincidir con `/tmp` ni contener `..`. Queda terminantemente prohibido borrar rutas fuera de este confinamiento.
  3. *Ausencia formal de cambio visual en la UI pública*: `test fixture foundation` es una capacidad de desarrollo e infraestructura de pruebas; por consiguiente, la interfaz pública del navegador (`BrowserWindow`) no expone menús, botones ni diálogos de fixtures.
  4. *Tolerancia honesta a entornos headless*: La suite de pruebas de fixtures ejecuta 100% en CTest bajo consola (< 100 ms) sin requerir servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`) ni procesos WebEngine/GPU.
  5. *Aislamiento y concurrencia CTest*: Generación de rutas combinando PID + steady_clock nanosegundos + contador atómico, garantizando aislamiento total en ejecuciones paralelas (`ctest -jN`).
  6. *Presupuestos estrictos de recursos*: Cuota máxima de 50 MB por fixture y limpieza automática garantizada mediante destructores RAII.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/TestFixtureFoundation.h` y `.cpp`: Primitivas RAII del dominio base.
  - `apps/vigilio-browser/src/core/TestFixtureValidator.h` y `.cpp`: Motor de validación estática de seguridad de rutas.
  - `apps/vigilio-browser/src/core/TestFixtureService.h` y `.cpp`: Capa operativa de servicio y sembrado sintético.
  - `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp`: Arnés de pruebas unitarias existente (10 pruebas).
  - `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp`: Arnés de pruebas de servicio existente (12 pruebas).
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Arnés de pruebas de UI en Qt Test.
  - `apps/vigilio-browser/scripts/check.sh`: Arnés local de verificación continua con CMake y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Colisión de rutas temporales en ejecuciones multihilo masivas.
    *Mitigación*: Fórmula determinista de ruta con PID, timestamp nanosegundo y contador atómico secuencial.
  - *Riesgo 2*: Fuga de artefactos residuales en `/tmp` tras pruebas abortadas intencionalmente.
    *Mitigación*: Destructores RAII y mecanismo preventivo `scanAndPurgeOrphanFixtures`.
  - *Riesgo 3*: Saturación de disco o agotamiento de inodos durante pruebas de estrés.
    *Mitigación*: Validación rigurosa de cuota de disco (`maxTempDirSizeBytes`) y liberación inmediata de instantáneas.

---

## Checklist evidence and handoff

- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 40 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0118-module.foundations.task0040.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js check-eligibility --pack module.foundations.task0040 --project-root .`: 10 comprobaciones aprobadas con estado `eligible: true` (exit code 0).
- **Verificación de precondiciones y dependencias ejecutables**:
  - `module.foundations.task0039` completado y verificado.
  - Base de código limpia y sincronizada en `main`.
- **Handoff a la fase APPLY (`module.foundations.task0040` en ejecución `0119`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0040/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 14 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
