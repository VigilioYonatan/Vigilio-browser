# Execution Memory — module-foundations-task0036

Status: READY_FOR_REVIEW

## Objective and authority

- **Objetivo**: Diseñar y formalizar la propuesta OpenSpec integral para la etapa **tests** de la capacidad **CMake presets** en el módulo `foundations` (`module.foundations.task0036`, microtarea canónica `0036 — Probar casos positivos, negativos y recuperación: CMake presets`).
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0036-cmake-presets/task.md`
- **Metadatos de ejecución del runner**:
  - Ejecución actual: `0106`.
  - Pack: `module.foundations.task0036`.
  - Fase: `propose`.
  - Generated artifact: `0106-module.foundations.task0036.propose.md`.
  - Prompt SHA-256: `9426c9c77571a4726ef4b42eea146f59fd30aa41758f3007db834c016520212b`.
  - Lock SHA-256: `3e271e4d9b582562a8f849c6da70c03595575dc03b38bb322dc700906f78d597`.
  - Dependencias ejecutables: `module.foundations.task0035`.
  - Paso de fase anterior: `none`.
  - Versión de Prompt Builder requerida: `0.6.3`.

---

## Skills and required references read

Se consultaron y aplicaron rigurosamente como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Modularidad y separación estricta de responsabilidades en C++20; desacoplamiento entre el núcleo de presets y la capa gráfica.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas enfocada en pruebas unitarias y de integración deterministas en `CMakePresetsTests.cpp` y `CMakePresetsServiceTests.cpp`; uso de fixtures locales RAII en `/tmp`, ausencia de sleeps artificiales y cobertura de casos positivos, negativos, extremos y recuperación ante fallos.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 estándar, const-correctness, tipado fuerte (`enum class`), manejo seguro de errores sin excepciones no controladas y ausencia de dependencias hacia `QApplication` o `QWidget` en la capa de build del núcleo.
- `docs/skills/04-ux-ui-senior.md` y `docs/skills/05-accesibilidad-y-senior-mode.md`: Verificación de que la capacidad de CMake Presets mantiene el contrato de «sin cambio visual directo» en la interfaz pública de usuario.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia estricta y presupuestos acotados; búferes acotados a 64 KB (`maxOutputBytes = 65536`) y tiempos de ejecución en CTest inferiores a 250 ms sin lanzar instancias de Chromium.
- `docs/skills/07-seguridad-y-privacidad.md`: Local-First estricto (ADR-001 / Decisión Q-001); cero llamadas a redes externas, prevención contra inyección de comandos shell en nombres de presets y contención contra directory traversal en rutas binarias (`isSafeBinaryDir`).
- `docs/skills/08-observabilidad-calidad-linters.md`: Verificación de calidad mediante Clang-Format, compilación con `-Werror` y ejecución limpia en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa de fases entre PROPOSE (autoría documental y diseño en `openspec/changes/`) y APPLY (implementación del código de prueba en la etapa subsiguiente).
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en cadenas observables. Uso exclusivo de descripciones en lenguaje natural en español comprensible.

---

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Local-First constitucional (ADR-001 / Q-001)*: Toda la lógica de validación, síntesis y ejecución de CMake Presets opera de manera estrictamente local en el equipo del usuario, sin componentes cloud ni telemetría.
  2. *Contención estricta de rutas (`isSafeBinaryDir`)*: El directorio de compilación debe residir obligatoriamente en subdirectorios locales con prefijo `build-*` o `_build*`, rechazando cualquier intento de escape o ruta absoluta fuera del proyecto.
  3. *Zero-Zombie y Bounded Buffers*: Captura de salida truncada a un límite máximo de 64 KB y timeouts estrictos ($\le 60$ s) para evitar bloqueos indefinidos o consumo excesivo de RAM.
  4. *Cancelación cooperativa multihilo*: `CancellationToken` garantiza paradas seguras e idempotentes en transiciones de estado con reporte ordenado de estado `Cancelled`.
  5. *Ejecución Headless sin Chromium*: El 100% de las suites de prueba de CMake Presets se ejecuta en CTest sin requerir display gráfico (X11/Wayland) ni procesos WebEngine/GPU.
  6. *Resiliencia de E/S en disco*: `cleanBinaryDir` y demás métodos de mantenimiento manejan errores de permisos y rutas bloqueadas limpiamente, retornando `false` y un diagnóstico claro sin propagar excepciones no controladas.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakePresets.json`: Especificación canónica v6 con 9 perfiles canónicos.
  - `apps/vigilio-browser/src/core/CMakePresetsTypes.h`: Definiciones de tipos y estructuras inmutables de dominio.
  - `apps/vigilio-browser/src/core/CMakePresetsValidator.h` & `.cpp`: Validador estático desacoplado.
  - `apps/vigilio-browser/src/core/CMakePresetsService.h` & `.cpp`: Servicio operativo con estado.
  - `apps/vigilio-browser/tests/unit/CMakePresetsTests.cpp`: Suite base del validador.
  - `apps/vigilio-browser/tests/unit/CMakePresetsServiceTests.cpp`: Suite base del servicio.
  - `apps/vigilio-browser/scripts/check.sh`: Arnés local de verificación continua con CMake, Ninja y CTest.
- **Riesgos y mitigaciones**:
  - *Riesgo 1*: Comportamiento inconsistente de permisos del sistema de archivos (`chmod` / `std::filesystem::permissions`) en contenedores o montajes virtuales con usuario root.
    *Mitigación*: Implementación de validaciones previas de compatibilidad de permisos en el fixture y fallback determinista que comprueba el manejo de errores sin depender exclusivamente de atributos de sistema operativo.
  - *Riesgo 2*: Condiciones de carrera entre el hilo de prueba y la solicitud de cancelación en `test_service_cancellation_stress_and_idempotence`.
    *Mitigación*: Uso de variables atómicas de sincronización para asegurar que la solicitud de cancelación ocurra durante la ventana activa de ejecución simulada.

---

## Checklist evidence and handoff

- **Pipeline continuo (`apps/vigilio-browser/scripts/check.sh`)**:
  - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
  - 13/13 pruebas pasando exitosamente en CTest en 0.44s (100% pass, exit code 0):
    1. `core_units`: Passed (0.00s)
    2. `storage_integration`: Passed (0.00s)
    3. `vigilio_warnings_tests`: Passed (0.04s)
    4. `qt_dependency_validation_tests`: Passed (0.00s)
    5. `webengine_version_validator_tests`: Passed (0.04s)
    6. `resource_qrc_catalog_tests`: Passed (0.01s)
    7. `resource_inclusion_service_tests`: Passed (0.02s)
    8. `resource_qrc_inclusion_tests`: Passed (0.03s)
    9. `startup_bootstrap_tests`: Passed (0.07s)
    10. `application_lifetime_tests`: Passed (0.14s)
    11. `crash_recovery_tests`: Passed (0.02s)
    12. `cmake_presets_tests`: Passed (0.03s)
    13. `cmake_presets_service_tests`: Passed (0.01s)
- **Validación documental y Prompt Builder CLI**:
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 36 referencias canónicas validadas con estado `valid: true` (exit code 0).
  - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0106-module.foundations.task0036.propose.lock.json --project-root .`: 13 comprobaciones aprobadas con estado `valid: true` (exit code 0).
- **Handoff a la fase APPLY (`module.foundations.task0036` en ejecución `0107`)**:
  - Se crearon y formalizaron en `openspec/changes/module-foundations-task0036/` los archivos `proposal.md`, `design.md`, `tasks.md`, `execution-memory.md` y `canonical-reference.yaml`.
  - Se definieron 11 tareas atómicas y verificables en `tasks.md` con criterios, comandos y orden determinista.
  - La propuesta está completamente lista y es apta para revisión humana previa a APPLY.
