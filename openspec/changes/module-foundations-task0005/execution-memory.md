# Execution Memory — module-foundations-task0005

Status: PROPOSED

## Objective and authority

- **Objetivo**: Elaborar la propuesta técnica formal (fase PROPOSE) para la microtarea `0005` del módulo `foundations` (`module.foundations.task0005`), correspondiente a la etapa **domain** de la capacidad **compiler warnings**.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/micro/0005-compiler-warnings/task.md`
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/00-global/architecture.md`
  - `docs/rules/00-global/constitution.md`
  - `docs/rules/00-global/delivery.md`
  - `docs/rules/00-global/error-handling.md`
  - `docs/rules/00-global/observability.md`
  - `docs/rules/00-global/security.md`
  - `docs/rules/00-global/testing.md`
  - `docs/rules/00-global/ux-ui.md`
  - `docs/rules/answers.md`
- **Identificadores criptográficos del prompt**:
  - Ejecución actual: `0013`
  - Pack: `module.foundations.task0005`
  - Phase: `propose`
  - Generated artifact: `0013-module.foundations.task0005.propose.md`
  - Prompt SHA-256: `367112aca56583bd894be69f93a33ee1eba307d40aff6c6ef07e375591658125`
  - Lock SHA-256: `bf465c014e6589f10b39d414c01e6cff5c9f4dfe86bca738c9e59d0b3ea36938`
  - Dependencias ejecutables: `module.foundations.task0004`
  - Prompt Builder requerido: `0.6.3`

## Skills and required references read

Se consultaron y aplicaron al 100% como norma vinculante las siguientes directrices, contratos y habilidades del proyecto:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID aplicados a la encapsulación del target `vigilio_compiler_warnings` (Single Responsibility para gobernanza de flags, Interface Segregation mediante `INTERFACE library`); principio DRY para eliminar la duplicación de bloques de flags en `CMakeLists.txt`; principios KISS y YAGNI para evitar scripts o macros innecesariamente complejas en CMake.
- `docs/skills/02-testing-senior.md`: Pruebas de compilación negativas y deterministas; reporte honesto de estados (`PASS`, `NOT_EXECUTED`); aseguramiento de que los flags de advertencia refuercen la pirámide de pruebas sin introducir inestabilidad.
- `docs/skills/03-codigo-cpp-qt.md`: Código idiomático C++20 con const-correctness, tipos estándar del lenguaje, RAII estricto y total independencia del núcleo de dominio frente a la interfaz gráfica.
- `docs/skills/04-ux-ui-senior.md`: Estilo sobrio, fondos limpios con contraste controlado; declaración explícita de ausencia de cambio visual directo para una capacidad puramente de infraestructura de compilación.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal y verificación de «Interfaz más fácil de usar»; documentación justificada de ausencia de controles o pantallas nuevas en esta etapa.
- `docs/skills/06-rendimiento-y-ram.md`: Verificación de que la capacidad opera exclusivamente en tiempo de traducción; cero impacto en consumo de memoria RAM o rendimiento en tiempo de ejecución.
- `docs/skills/07-seguridad-y-privacidad.md`: Modelo local-first estricto; prevención temprana de vulnerabilidades de memoria y desbordamiento de enteros mediante `-Wconversion`, `-Wsign-conversion` y `-Wnull-dereference`.
- `docs/skills/08-observabilidad-calidad-linters.md`: Integración armónica entre advertencias del compilador, ClangFormat y Clang-Tidy; trazabilidad de comandos con códigos de salida explícitos.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de responsabilidades entre fases (PROPOSE diseña la solución integral sin modificar código de aplicación bajo `apps/**`; APPLY ejecuta el incremento; REVIEW verifica independientemente).
- `docs/skills/10-release-instaladores.md`: Garantía de que los binarios generados para release en Linux, Windows y macOS compilen con cero defectos latentes.
- `docs/skills/11-assets-svg-quality.md`: Confirmación de que la capacidad de advertencias no altera ni afecta los assets vectoriales ni el archivo de recursos `resources.qrc`.
- `docs/UX-COPY-PUBLICO.md`: Prohibición total de exponer términos de compilación o ingeniería en la interfaz pública visible del navegador.
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Contratos de accesibilidad y visuales de referencia.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales conceptuales de la interfaz de usuario objetivo y del panel de accesibilidad.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Cero advertencias como estándar continuo*: Todo el código fuente del proyecto bajo `src/**` y `tests/**` debe compilar sin advertencias en los compiladores objetivo (GCC 11+, Clang 14+, MSVC 19.30+).
  2. *Desacoplamiento y no-fuga transitiva*: La política de advertencias se encapsula en el target `INTERFACE` `vigilio_compiler_warnings` y se enlaza con visibilidad `PRIVATE`, asegurando que no se propague a dependientes externos.
  3. *Inmunidad frente a dependencias externas*: Las cabeceras de terceros (`SQLite3`, `Qt6`) deben marcarse con `SYSTEM` en CMake para que sus advertencias internas no aborten la compilación bajo `-Werror` / `/WX`.
  4. *Aislamiento del núcleo*: Los flags de advertencia de `vigilio_core` no dependen ni asumen la presencia de Qt ni de SQLite.
  5. *Determinismo multiplataforma*: Los flags se seleccionan condicionalmente según `CMAKE_CXX_COMPILER_ID` (`GNU`, `Clang`, `AppleClang`, `MSVC`) manteniendo la equivalencia de rigor en Linux, macOS y Windows.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/CMakePresets.json`
  - `apps/vigilio-browser/scripts/check.sh`
  - `apps/vigilio-browser/src/core/UrlInput.h` / `UrlInput.cpp`
  - `apps/vigilio-browser/src/core/TabModel.h` / `TabModel.cpp`
  - `apps/vigilio-browser/src/core/SessionFile.h` / `SessionFile.cpp`
  - `apps/vigilio-browser/src/storage/LocalRepository.h` / `LocalRepository.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` / `BrowserWindow.cpp`
  - `apps/vigilio-browser/src/app.cpp`
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Falsos positivos en macros y código generado por Qt (`moc_*.cpp`, `qrc_*.cpp`) al activar advertencias estrictas como `-Wold-style-cast` o `-Wuseless-cast`.
    *Mitigación*: En la arquitectura de CMake, aislar los archivos generados o excluir flags que entren en conflicto directo con las expansiones de macros de Qt, manteniendo las comprobaciones estrictas sobre el código fuente del proyecto.
  - *Riesgo*: Advertencias provenientes de cabeceras de terceros (`sqlite3.h`, cabeceras Qt) al compilar con `-Werror` / `/WX`.
    *Mitigación*: Uso mandatorio de la directiva `SYSTEM` en las rutas de inclusión de bibliotecas externas en CMake (`target_include_directories(... SYSTEM ...)`), instruyendo a los compiladores a suprimir advertencias en esas cabeceras.
  - *Riesgo*: Divergencias de comportamiento entre versiones de compiladores (GCC vs Clang vs MSVC).
    *Mitigación*: Pruebas de configuración condicionales por `CMAKE_CXX_COMPILER_ID` y validación de compatibilidad en matrices de CI.

## Checklist evidence and handoff

- **Validación de referencias canónicas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
  - Salida: `{"authority": "docs-rules", "references": 5, "valid": true}`
  - Código de salida: `0`
- **Verificación de integridad del lockfile**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0013-module.foundations.task0005.propose.lock.json`
  - Salida: `{"lockfile": "generated/prompts/full-project/0013-module.foundations.task0005.propose.lock.json", "valid": true, "checks": [...]}` (13 checks aprobados)
  - Código de salida: `0`
- **Validación de decisiones humanas**:
  - Comando: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`
  - Salida: Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN (bloquea exclusivamente el hito final de publicación `release.public-release`, no bloquea `module.foundations.task0005`).
- **Verificación de suite headless existente (baseline)**:
  - Comando: `bash apps/vigilio-browser/scripts/check.sh`
  - Salida: 100% de tests aprobados (`core_units` y `storage_integration`) en 0.01s con Ninja; formato verificado; exit code 0.
- **Estado de artefactos OpenSpec**:
  - `openspec/changes/module-foundations-task0005/canonical-reference.yaml`: válido y verificado.
  - `openspec/changes/module-foundations-task0005/proposal.md`: completado formalmente con el alcance exhaustivo de la etapa `domain`.
  - `openspec/changes/module-foundations-task0005/design.md`: completado formalmente con el modelo de dominio, catálogo de flags, política de warnings-as-errors, aislamiento y matriz de trazabilidad.
  - `openspec/changes/module-foundations-task0005/tasks.md`: completado formalmente con tareas atómicas y verificables TASK-001 a TASK-005.
  - `openspec/changes/module-foundations-task0005/execution-memory.md`: completado formalmente con registro de autoridad, skills, evidencias y handoff.
- **Handoff**:
  - La propuesta para `module.foundations.task0005` queda completamente formalizada, verificada y lista para revisión humana previa a la fase APPLY.
  - No se modificó ningún archivo fuera de `openspec/changes/module-foundations-task0005/**`.
