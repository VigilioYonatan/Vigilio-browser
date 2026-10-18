# Execution Memory — module-foundations-task0039

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **ui** de la capacidad **test fixture foundation** en el módulo `foundations` (`module.foundations.task0039`, microtarea canónica `0039 — Implementar la interacción y estados visuales en Qt: test fixture foundation`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0039-test-fixture-foundation/task.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/factory-baseline.yaml`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0115`.
  - Pack: `module.foundations.task0039`.
  - Phase: `propose`.
  - Generated artifact: `0115-module.foundations.task0039.propose.md`.
  - Prompt SHA-256: `45eff409c76fe5af67cf4adfb3f733f37820ad4216cafb0928ecb0b919084878`.
  - Lock SHA-256: `ba1af7d6fbe9fdf4d0efdf0407229bc80b89a2e8b000c605362cf1ee3e891ab8`.
  - Dependencias ejecutables: `module.foundations.task0038`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad, separación estricta de responsabilidades (SOLID), KISS y YAGNI; desacoplamiento estricto entre la infraestructura de fixtures en `src/core` y la interfaz gráfica en `src/ui`.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas; integración de fixtures locales RAII deterministas en `WindowTests.cpp`, reporte honesto de `NOT_EXECUTED` ante ausencia de servidor gráfico y erradicación de sleeps artificiales.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), ownership claro en widgets Qt y aislamiento entre lógica del núcleo y renderizado gráfico.
- `docs/skills/04-ux-ui-senior.md`: Estilo moderno, sobrio y profesional sin brillos artificiales; alineación con las referencias visuales `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Controles amplios ($\ge 48$ px), barras $\ge 55$ px, banners $\ge 56$ px, iconos escalados (22×22 $\to$ 32×32 px), tipografía ampliada (+4 pt), navegación completa por teclado y atributos accesibles completos (`accessibleName`, `accessibleDescription`).
- `docs/skills/06-rendimiento-y-ram.md`: Presupuestos estrictos de recursos; cuota de disco limitada a 50 MB por fixture (`maxTempDirSizeBytes`), ejecución de CTest en < 1 segundo para la suite completa y cero procesos colgados (Zero-Zombie).
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes exteriores, prevención contra Directory Traversal (`..`) y validación rigurosa de rutas temporales con `isSafeTestDirectory`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad continua mediante Clang-Format, compilación con `-Werror` y cero advertencias.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación física tras aprobación humana).
- `docs/skills/10-release-instaladores.md`: Preservación de la integridad del empaquetado de producción sin filtración de artefactos de prueba temporales.
- `docs/skills/11-assets-svg-quality.md`: Respeto a los iconos SVG vectoriales compilados en `resources.qrc` con trazo y proporciones estándar.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Test Fixture», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec») en cadenas observables. Uso exclusivo de la denominación oficial «Interfaz más fácil de usar».

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la infraestructura de fixtures opera de forma estrictamente local en el almacenamiento temporal del host, sin conexiones remotas ni telemetría.
  2. *Contención estricta de rutas temporales (`isSafeTestDirectory`)*: Cualquier carpeta administrada o purgada debe residir obligatoriamente en `std::filesystem::temp_directory_path()`, comenzar con `vigilio_test_` y no coincidir con `/tmp` ni contener `..`. Queda terminantemente prohibido borrar rutas fuera de este confinamiento.
  3. *Ausencia formal de cambio visual en la UI pública*: `test fixture foundation` es una capacidad de desarrollo e infraestructura de pruebas; por consiguiente, la interfaz pública del navegador (`BrowserWindow`) no expone menús, botones ni diálogos de fixtures.
  4. *Tolerancia honesta a entornos headless*: Si `DISPLAY` y `WAYLAND_DISPLAY` están ausentes, `WindowTests.cpp` emite `NOT_EXECUTED` con código 0, previniendo cuelgues o falsos positivos en integración continua.
  5. *Aislamiento y concurrencia CTest*: Generación de rutas combinando PID + steady_clock nanosegundos + contador atómico, garantizando aislamiento total en ejecuciones paralelas (`ctest -jN`).
  6. *Presupuestos estrictos de recursos*: Cuota máxima de 50 MB por fixture y limpieza automática garantizada mediante destructores RAII.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/TestFixtureFoundation.h` y `.cpp`: Primitivas RAII del dominio base.
  - `apps/vigilio-browser/src/core/TestFixtureValidator.h` y `.cpp`: Motor de validación estática de seguridad de rutas.
  - `apps/vigilio-browser/src/core/TestFixtureService.h` y `.cpp`: Capa operativa de servicio y sembrado sintético.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Arnés de pruebas de UI en Qt Test.
  - `apps/vigilio-browser/scripts/check.sh`: Arnés local de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Acoplamiento accidental de dependencias de testing dentro del código de producción de `BrowserWindow`.
    *Mitigación*: Desacoplamiento estricto; `vigilio_core` no incluye widgets y `BrowserWindow` solo recibe rutas inyectadas mediante configuración.
  - *Riesgo 2*: Fuga de terminología interna en etiquetas visuales visibles al usuario final.
    *Mitigación*: Cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md` y auditoría de cadenas en `BrowserWindow`.
  - *Riesgo 3*: Fallo de pruebas de UI en servidores headless de integración continua.
    *Mitigación*: Detección preliminar de display en `WindowTests.cpp` retornando `NOT_EXECUTED` con código 0.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 15/15 pruebas pasando exitosamente en CTest en 0.75s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.09s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.06s)
    6. `resource_qrc_catalog_tests`: Passed (0.06s)
    7. `resource_inclusion_service_tests`: Passed (0.05s)
    8. `resource_qrc_inclusion_tests`: Passed (0.07s)
    9. `startup_bootstrap_tests`: Passed (0.08s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.00s)
    15. `test_fixture_service_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 39 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0115-module.foundations.task0039.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js check-eligibility --pack module.foundations.task0039 --project-root .`: 10 comprobaciones aprobadas con estado `eligible: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0039` en ejecución `0116`)**:
  - Se formalizaron en `openspec/changes/module-foundations-task0039/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 11 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente formulada, verificada y lista para revisión humana previa a APPLY.
