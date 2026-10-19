# Execution Memory — module-ux-shell-task0061

Status: PROPOSED

## Objective and authority

- **Objetivo**: Proponer la solución técnica integral para la etapa `implementation` de la capacidad **main window layout** (`module.ux-shell.task0061`), diseñando la capa de servicio operativo desacoplada (`MainWindowLayoutService`), la máquina de estados de layout, transiciones de densidad visual (estándar, compacta y «Interfaz más fácil de usar» / Modo Senior), gestión de barra lateral, persistencia local atómica con resiliencia ante datos corruptos, aislamiento estricto de navegación en modo incógnito, soporte de cancelación cooperativa bajo la política Zero-Zombie y suite de pruebas unitarias en CTest, sin acoplamiento a widgets de GUI ni a Chromium WebEngine, en rigurosa conformidad con la arquitectura Local-First (ADR-001 / Decisión Q-001) y `docs/UX-COPY-PUBLICO.md`.
- **Autoridad canónica**:
  - `docs/rules/01-modules/ux-shell/README.md`: Responsabilidad y alcance del módulo ux-shell.
  - `docs/rules/01-modules/ux-shell/design.md`: Arquitectura, dependencias e interfaces desacopladas.
  - `docs/rules/01-modules/ux-shell/screens.md`: Pantallas y estados.
  - `docs/rules/01-modules/ux-shell/business.md`: Reglas de negocio, operaciones y fallos.
  - `docs/rules/01-modules/ux-shell/tests.md`: Matriz de pruebas y criterios de verificación.
  - `docs/rules/01-modules/ux-shell/operations.md`: Observabilidad, telemetría local y recuperación.
  - `docs/rules/01-modules/ux-shell/micro/0061-main-window-layout/task.md`: Contrato vinculante de la microtarea canónica 0061.
  - `openspec/changes/module-ux-shell-task0061/canonical-reference.yaml`: Declaración formal de autoridad canónica en modo `reference-only`.

---

## Skills and required references read

- `docs/skills/01-principios-de-ingenieria.md`: Modularidad estricta, alta cohesión, bajo acoplamiento, C++20 portable y separación limpia entre `vigilio_core` y componentes de interfaz gráfica.
- `docs/skills/02-testing-senior.md`: Pruebas de servicio deterministas, rápidas y reproducibles en CTest; aserciones precisas sin mocks engañosos ni sleeps arbitrarios.
- `docs/skills/03-codigo-cpp-qt.md`: Tipos fuertemente tipados, semántica de valor inmutable, const-correctness y exclusión de dependencias de GUI en el núcleo de servicios.
- `docs/skills/04-ux-ui-senior.md`: Principio de diseño moderno, sobrio y profesional; sin brillos artificiales ni aspecto genérico; tonos azul oscuro y superficies limpias; integración con barra lateral, omnibox y tarjetas.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Mantenimiento y verificación de las directrices de «Interfaz más fácil de usar» (Modo Senior), áreas de interacción táctiles y de ratón $\ge 48$ px, barra de navegación $\ge 56$ px, iconos escalados a $32\times 32$ px, navegación por teclado y legibilidad sin solapamientos.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta, presupuestos de memoria reducidos (< 100 KB heap), cálculos geométricos analíticos en memoria (< 1 ms) y pruebas globales < 10 segundos en CTest.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, aislamiento de datos locales y cero persistencia de datos de navegación en modo incógnito.
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de formato con Clang-Format, compilación limpia con `-Werror` y ejecución estricta en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación en la etapa subsiguiente tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad y renderizado de recursos gráficos e iconografía vectorial Lucide SVG en QRC.
- `docs/skills/README.md`: Buenas prácticas transversales de ingeniería, calidad y diseño.
- `docs/UX-COPY-PUBLICO.md`: Proscripción terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain», «runner», «Linter») en mensajes de diagnóstico o pantallas visibles. Uso obligatorio de **«Interfaz más fácil de usar»**.
- Referencias visuales oficiales:
  - `/resultado/01-vigilio-home.png`: Layout estructural de la ventana principal con barra lateral, barra de herramientas y buscador.
  - `/resultado/03-vigilio-settings-senior-mode.png`: «Interfaz más fácil de usar» con controles amplios y alta legibilidad.
  - `/resultado/06-logo-instalador-oficial.png`: Isotipo y logotipo oficial de Vigilio Browser.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de orquestación, cálculo y persistencia opera 100% en local, sin llamadas de red ni sincronización remota.
  2. *Desacoplamiento estricto del servicio*: `MainWindowLayoutService` reside en `vigilio_core` sin incluir cabeceras de Qt GUI (`QWidget`, `QMainWindow`, `QApplication`).
  3. *Thread-safety estricto*: Las lecturas y mutaciones concurrentes del plan de layout están sincronizadas mediante `std::mutex`.
  4. *Invariante de persistencia atómica*: La escritura a disco se realiza a través de un archivo temporal `.tmp` seguido de renombramiento atómico.
  5. *Invariante de aislamiento de modo incógnito*: Ninguna sesión privada o ventana en modo incógnito puede escribir o actualizar el archivo de estado de layout en disco.
  6. *Invariante de resiliencia y recuperación*: Si el archivo de estado en disco está truncado, ausente o corrupto, el servicio restaura automáticamente `canonicalDefaultLayout()` con el código recuperable `LayoutPersistenceCorrupted`.
  7. *Invariante de cancelación cooperativa (Zero-Zombie)*: Las operaciones pesadas aceptan `CancellationToken*` y abortan inmediatamente ante señalización.
  8. *Auditoría de lenguaje público*: Todo texto o diagnóstico emitido por el servicio utiliza la terminología aprobada en `docs/UX-COPY-PUBLICO.md` («Interfaz más fácil de usar», «Estándar», «Compacta»).
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/MainWindowLayoutTypes.h`: Taxonomía y tipos inmutables de layout establecidos en `task0060`.
  - `apps/vigilio-browser/src/core/MainWindowLayoutConfiguration.h/.cpp`: Factorías canónicas e invariantes.
  - `apps/vigilio-browser/src/core/MainWindowLayoutValidator.h/.cpp`: Algoritmo analítico de cálculo de áreas `calculateLayoutAreas`.
  - `apps/vigilio-browser/src/core/ClangTidyService.h/.cpp`, `CiMacOsJobService.h/.cpp`: Patrones canónicos de servicios desacoplados en el proyecto.
  - `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`: Proporciones y parámetros de interfaz objetivo.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Corrupción de archivos de configuración ante cortes abruptos de energía.
    - *Mitigación*: Mecanismo de guardado en dos fases con archivo temporal `.tmp` y renombramiento atómico (`std::filesystem::rename`).
  - *Riesgo 2*: Fugas de privacidad en sesiones de incógnito.
    - *Mitigación*: Validación preventiva de `isIncognitoSession == true` en todos los métodos de escritura en disco, abortando antes de tocar el sistema de archivos.
  - *Riesgo 3*: Estados intermedios inconsistentes durante transiciones de densidad.
    - *Mitigación*: Mutación atómica dentro del lock de mutex con validación previa de umbrales dimensionales y rechazo sin efectos secundarios en caso de fallo.
  - *Riesgo 4*: Interrupciones multihilo y procesos zombis.
    - *Mitigación*: Soporte estricto de `CancellationToken` bajo la política Zero-Zombie.

---

## Checklist evidence and handoff

- **Pipeline continuo en CMake / CTest**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 26/26 pruebas pasando exitosamente en CTest en ~5.39s (100% pass, exit code 0):
    1. `core_units`: Passed (0.01s)
    2. `storage_integration`: Passed (0.01s)
    3. `vigilio_warnings_tests`: Passed (0.15s)
    4. `qt_dependency_validation_tests`: Passed (0.01s)
    5. `webengine_version_validator_tests`: Passed (0.20s)
    6. `resource_qrc_catalog_tests`: Passed (0.09s)
    7. `resource_inclusion_service_tests`: Passed (0.19s)
    8. `resource_qrc_inclusion_tests`: Passed (0.19s)
    9. `startup_bootstrap_tests`: Passed (0.18s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.03s)
    12. `cmake_presets_tests`: Passed (0.04s)
    13. `cmake_presets_service_tests`: Passed (0.10s)
    14. `test_fixture_foundation_tests`: Passed (0.01s)
    15. `test_fixture_service_tests`: Passed (0.11s)
    16. `clang_format_configuration_tests`: Passed (0.00s)
    17. `clang_format_service_tests`: Passed (1.98s)
    18. `clang_tidy_configuration_tests`: Passed (0.01s)
    19. `clang_tidy_service_tests`: Passed (0.28s)
    20. `ci_linux_job_configuration_tests`: Passed (0.02s)
    21. `ci_linux_job_service_tests`: Passed (0.18s)
    22. `ci_windows_job_configuration_tests`: Passed (0.01s)
    23. `ci_windows_job_service_tests`: Passed (0.76s)
    24. `ci_macos_job_configuration_tests`: Passed (0.01s)
    25. `ci_macos_job_service_tests`: Passed (0.67s)
    26. `main_window_layout_tests`: Passed (0.00s)
- **Gobernanza con Prompt Builder CLI**:
  - `vigilio-prompt validate-references --project-root .`: 61 referencias canónicas validadas (`valid: true`, exit code 0).
  - `vigilio-prompt verify-lock --lock-file generated/prompts/full-project/0181-module.ux-shell.task0061.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
  - `vigilio-prompt validate --pack module.ux-shell.task0061 --phase propose --project-root .`: Validación de pack OK (exit code 0).
