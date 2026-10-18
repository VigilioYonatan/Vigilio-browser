# Change Proposal — module-foundations-task0004

Pack: `module.foundations.task0004`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **cmake targets** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado la especificación de dominio (`module.foundations.task0001`), la modularización de bibliotecas estáticas (`module.foundations.task0002`) y la gobernanza del ejecutable de interfaz gráfica y recursos (`module.foundations.task0003`), este pack establece la estrategia y cobertura completa de pruebas automatizadas y deterministas para los targets de CMake.

El alcance concreto comprende:
- **Estrategia y cobertura exhaustiva de pruebas para CMake targets**:
  - **Casos positivos / normales**:
    - Validación determinista del grafo acíclico dirigido (DAG) de dependencias entre targets: `vigilio_core` (dominio puro C++20) -> `vigilio_storage` (persistencia SQLite) -> `vigilio-browser` (aplicación de escritorio nativa Qt Widgets).
    - Compilación, enlace y verificación en modo headless estricto (`VIGILIO_BUILD_GUI=OFF` / preset `core-debug`) para entornos de integración continua sin servidor gráfico ni paquetes Qt.
    - Compilación condicional de la interfaz gráfica (`VIGILIO_BUILD_GUI=ON` / preset `desktop-debug`) sujeta a la disponibilidad real de Qt6 (`Qt6_FOUND`), con inclusión verificable del bundle de recursos `resources.qrc`.
    - Clasificación taxonómica en CTest con etiquetas (`LABELS "unit;fast"`, `"integration;fast"`, `"ui;fast"`, `"e2e;heavy"`) y tiempos límite estrictos (`TIMEOUT`).
  - **Casos negativos y entradas inválidas**:
    - Comprobación de que `vigilio_core` rechaza enlaces indebidos o inclusiones accidentales de cabeceras de Qt Widgets o Chromium WebEngine (aislamiento de frontera arquitectónica).
    - Detección y manejo elegante de dependencias ausentes en CMake: `SQLite3` (error fatal informativo temprano) y `Qt6` (degradación elegante con advertencia informativa `message(WARNING ...)` manteniendo operativos los targets de core).
    - Comprobación de que `rcc` falla la compilación si un recurso declarado en `resources.qrc` no existe en disco, impidiendo la generación de binarios con recursos rotos.
    - Pruebas negativas a nivel de lógica de targets: rechazo de URLs malformadas, esquemas peligrosos (`javascript:`, `file:///etc/passwd`, URLs con credenciales embebidas `user:pass@host`), inyección SQL en almacenamiento SQLite y rutas no autorizadas.
  - **Concurrencia, cancelación y presupuestos de memoria (RAM)**:
    - Gobernanza estricta en CMake para pruebas pesadas de navegador (`vigilio_e2e_smoke`): ejecución estrictamente serializada mediante `RUN_SERIAL TRUE` y exclusión mutua de procesos mediante `RESOURCE_LOCK browser_e2e`.
    - Contención de memoria: verificación de que los tests unitarios y de integración no levanten subprocesos de Chromium ni agentes MCP innecesarios.
    - Timeouts acotados (15 segundos para unit/integration, 20 segundos para UI, 60 segundos para E2E) para prevenir procesos huérfanos o zombies ("ZERO-ZOMBIE").
  - **Recuperación y prevención de regresiones**:
    - Pruebas de recuperación ante archivo de sesión corrupto o inexistente (`SessionFile::loadSession`), garantizando que la aplicación inicie con página por defecto limpia.
    - Pruebas de reapertura y recuperación de base de datos SQLite tras cierre anómalo, validando integridad de esquemas y transacciones.
    - Limpieza automática garantizada de fixtures temporales de prueba mediante RAII (`std::filesystem::remove`, `QTemporaryDir`), asegurando que no queden archivos residuales (`-wal`, `-shm`, temporales) tras la ejecución de las suites.
    - Verificación de ausencia de regresiones sobre los targets preexistentes (`core_units` y `storage_integration`).

## Out of scope

- Implementación de nuevas funcionalidades de negocio o módulos ajenos a `foundations` (como filtros AdBlock de `shields`, gestor de `downloads`, herramientas de `tools` o paneles de `settings`).
- Modificación de los niveles de advertencia del compilador (`compiler-warnings`, microtareas 0005-0008) o reglas de linters (`clang-tidy`, microtareas 0045-0048).
- Ejecución obligatoria de pruebas E2E pesadas con Chromium durante la validación estándar de CI headless (las pruebas E2E requieren entorno gráfico y se ejecutan selectivamente bajo `VIGILIO_ENABLE_E2E=ON`).
- Conexión a redes externas, APIs en la nube, servidores de telemetría o servicios con login (prohibición estricta de la constitución local-first de Vigilio).
- Modificación directa de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE (toda alteración de código se reserva a la fase APPLY tras aprobación humana).

## Contexto y evidencia

Se inspeccionó de forma exhaustiva el repositorio `apps/vigilio-browser` y las herramientas de validación:
1. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Define los targets de biblioteca `vigilio_core` (C++20, estática) y `vigilio_storage` (C++20, estática, enlace privado a `SQLite::SQLite3`).
   - Define los ejecutables de prueba `vigilio_core_tests` (etiqueta `unit;fast`, timeout 15s) y `vigilio_storage_tests` (etiqueta `integration;fast`, timeout 15s).
   - Bloque condicional `VIGILIO_BUILD_GUI=ON` que busca `Qt6 6.10` y define `vigilio-browser`, con inclusión de `resources.qrc` y guardas para `vigilio_ui_tests` y `vigilio_e2e_smoke`.
2. **Suites de prueba existentes**:
   - `tests/unit/CoreTests.cpp`: prueba resolución de entradas URL normales y de búsqueda, rechazo de entradas peligrosas (`javascript:`, `file:///`, credenciales embebidas), operaciones sobre `TabModel` y persistencia básica de sesión en disco.
   - `tests/integration/StorageTests.cpp`: prueba creación de base de datos SQLite, inserción/actualización de marcadores, resistencia a inyecciones SQL en títulos, rechazo de URLs inválidas y conteo de historial tras reapertura.
   - `tests/ui/WindowTests.cpp`: prueba inicialización básica de ventana Qt, conteo de pestañas y presencia de `QLineEdit` y `QToolBar`.
   - `tests/e2e/BrowserSmoke.cpp`: prueba de humo de ciclo de vida completo con fixture HTML local, condicionado a `VIGILIO_ENABLE_E2E=ON`.
3. **Verificación de ejecución local**:
   - Script `apps/vigilio-browser/scripts/check.sh` ejecutado con éxito: configuración con Ninja, compilación completa y 100% de tests aprobados (`core_units` y `storage_integration`, exit code 0).
4. **Verificación de gobernanza y Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 4 referencias canónicas validadas, estado `valid: true`, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0010-module.foundations.task0004.propose.lock.json`: validación al 100% aprobada sobre todos los documentos y baseline, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito final de publicación `release.public-release`.
5. **Entorno de compilación**:
   - CMake 4.2.3, Ninja 1.13.2, GCC 15.2.0, Qt 6.10.2 instalados en el sistema operativo Linux.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0004`.
- **Capacidad**: `cmake targets`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0004/proposal.md`
  - `openspec/changes/module-foundations-task0004/design.md`
  - `openspec/changes/module-foundations-task0004/tasks.md`
  - `openspec/changes/module-foundations-task0004/execution-memory.md`
  - `openspec/changes/module-foundations-task0004/canonical-reference.yaml`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0004/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. **Refinamiento de pruebas unitarias (`tests/unit/CoreTests.cpp`)**:
   - Incorporar casos negativos adicionales: cadenas de longitud extrema, caracteres de control no imprimibles, esquemas no estándar, y pruebas de robustez en deserialización de archivos de sesión truncados o corruptos.
   - Añadir aserciones de invariantes de `TabModel` ante operaciones de cierre masivo concurrente o IDs inexistentes.
2. **Refinamiento de pruebas de integración de persistencia (`tests/integration/StorageTests.cpp`)**:
   - Añadir casos de prueba para manejo de errores de base de datos bloqueada o archivo de solo lectura, verificando que las excepciones se capturen y reporten limpiamente.
   - Validar que las transacciones incompletas se reviertan de forma segura (rollback) sin dejar registros corruptos.
   - Asegurar el borrado determinista de todos los artefactos SQLite generados durante los tests (`.sqlite`, `-wal`, `-shm`).
3. **Formalización de gobernanza de targets en `apps/vigilio-browser/CMakeLists.txt`**:
   - Verificar y consolidar las propiedades CTest de todos los targets de prueba (`TIMEOUT`, `LABELS`, `RESOURCE_LOCK`, `RUN_SERIAL`).
   - Garantizar que la compilación headless (`VIGILIO_BUILD_GUI=OFF`) continúe pasando el 100% de las pruebas registradas sin requerir dependencias gráficas.
4. **Verificación integral de calidad**:
   - Ejecutar el script `apps/vigilio-browser/scripts/check.sh` asegurando compilación limpia con Ninja, paso de todas las pruebas rápidas de CTest y verificación de formato con ClangFormat.
5. **Sincronización documental canónica**:
   - Actualizar el estado de la tarea en `docs/rules/01-modules/foundations/micro/0004-cmake-targets/task.md` y revalidar referencias canónicas mediante la herramienta de Prompt Builder.

## Definición de terminado (Definition of Done)

- [x] Estrategia y cobertura de pruebas para `cmake targets` completamente formalizada en la propuesta (casos normales, negativos, concurrencia, límites de RAM, recuperación y no regresión).
- [x] Ausencia de cambio visual directo justificada documentalmente en cumplimiento de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Reglas de gobierno de CTest (etiquetado taxonómico `unit;fast`, `integration;fast`, `ui;fast`, `e2e;heavy`, timeouts y serialización `RUN_SERIAL`) detalladas en el diseño.
- [x] Preservación obligatoria de compatibilidad headless (`VIGILIO_BUILD_GUI=OFF`) validada en el diseño y verificable en `scripts/check.sh`.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0003` (etapa `ui` de cmake targets, completada con éxito).
- **Dependencias posteriores**: `module.foundations.task0005` (inicio de la capacidad `compiler-warnings` en el roadmap).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Indisponibilidad de display gráfico en entornos de CI sin X11/Wayland al ejecutar pruebas de interfaz.
    *Mitigación*: Mantener la suite de interacción de widgets (`vigilio_ui_tests`) desacoplada del renderizado pesado de WebEngine y acondicionada a entornos con soporte Qt, registrando `NOT_EXECUTED` honesto cuando falte el entorno adecuado, mientras la suite de core/almacenamiento se ejecuta 100% headless.
  - *Riesgo*: Retención de procesos Chromium o archivos de fixture huérfanos tras fallos de prueba.
    *Mitigación*: Uso obligatorio de RAII (`QTemporaryDir`, limpieza en bloques `try...catch` / destructores) y serialización forzada en CMake (`RESOURCE_LOCK browser_e2e`, `RUN_SERIAL TRUE`).
  - *Riesgo*: Contaminación de dependencias entre targets de core y bibliotecas externas.
    *Mitigación*: Pruebas de compilación que verifican que `vigilio_core` no incluye cabeceras de Qt ni SQLite, preservando la independencia de capas.

## Commit propuesto

`test(foundations): tests cmake-targets`
