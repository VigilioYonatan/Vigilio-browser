# Change Proposal — module-foundations-task0028

Pack: `module.foundations.task0028`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **application lifetime** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0028 — Probar casos positivos, negativos y recuperación: application lifetime`).

Habiendo implementado en las microtareas precedentes el modelo de tipos y sesión atómica (`task0025`), el coordinador desacoplado de C++20 con watchdog de 3000 ms y deduplicación atómica (`task0026`), y la integración física en Qt Widgets (`task0027`), esta propuesta proyecta la **estrategia de pruebas automatizadas deterministas, exhaustivas y de alta velocidad** para certificar el ciclo de vida de la aplicación frente a todas las condiciones operativas reales: apagado limpio, entradas inválidas, excepciones en callbacks, saltos de estado ilegales, concurrencia multi-hilo, cancelación de salida, señales POSIX/Windows (`SIGINT`, `SIGTERM`), timeouts de watchdog, tolerancia a fallos de almacenamiento y garantía constitucional de cero trazas en modo incógnito (decisión Q-001).

El alcance concreto de este pack comprende:
- **Diseño e implementación de una suite exhaustiva de pruebas unitarias y de integración para el ciclo de vida (`apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`)**:
  - **Casos positivos (Positive Cases)**:
    - Validación del ciclo de vida estándar completo: `Uninitialized -> Starting -> Running -> ShutdownRequested -> FlushingState -> TerminatingSubsystems -> Terminated`.
    - Verificación de códigos de salida canónicos: 0 para salida normal (`UserWindowClose`, `UserExplicitQuit`), 130 para `SignalInterrupt` (`SIGINT`), 143 para `SignalTerminate` (`SIGTERM`), y 1 para fallos o abortos de emergencia (`EmergencyAbort`, `MemoryExhaustion`, watchdog timeout).
    - Métricas de temporización de alta resolución: recolección de microsegundos transcurridos en vaciado de sesión (`sessionFlushElapsedMicroseconds`) y cierre de subsistemas (`teardownElapsedMicroseconds`).
    - Verificación del predicado `report.isCleanExit()`, que debe devolver `true` exclusivamente cuando el estado final es `Terminated`, el código de salida es 0 y `timeoutTriggered` es `false`.
    - Comportamiento de sesión en modo seguro (`isSafeMode`): propagación transparente sin interferencia con la secuencia de salida ordenada.
  - **Casos negativos y condiciones de frontera (Negative & Boundary Cases)**:
    - Transiciones ilegales directas en la máquina de estados: rechazo de saltos inválidos (p. ej. `Uninitialized -> Terminated`, `Running -> Terminated`, `FlushingState -> Running`, `Terminated -> Starting`).
    - Estado terminal inmutable: verificación de que ningún estado puede suceder a `AppLifetimeState::Terminated`.
    - Solicitudes directas de `executeShutdown` desde estados iniciales sin pasar previamente por `requestShutdown`: validación de que el coordinador fuerza la transición segura a `ShutdownRequested` o `EmergencyAbort` sin invocar comportamientos indefinidos.
    - Manejo de fallos en callbacks de subsistemas:
      - Simulación de fallo en `sessionSaver` (retorno `false`): el coordinador registra `report.sessionSaved = false`, no interrumpe el flujo, prosigue al cierre ordenado de base de datos y alcanza `Terminated`.
      - Inyección de callback nulo (`sessionSaver = nullptr` o `dbCloser = nullptr`): ejecución segura sin desreferenciación de punteros inválidos.
      - Simulación de fallo en `dbCloser` (retorno `false`): el coordinador registra `report.databaseClosedCleanly = false` y finaliza ordenadamente.
    - Parámetros extremos en `LifetimeConfig`: timeout de 0 ms (disparo inmediato de timeout ante cualquier trabajo), timeout máximo (`UINT32_MAX`), y rutas nulas o vacías de sesión y perfil.
  - **Concurrencia, carreras críticas y cancelación (Concurrency & Cancellation)**:
    - **Flujo completo de cancelación de apagado**:
      - Simulación del rechazo del usuario ante el diálogo de confirmación de cierre de múltiples pestañas: invocación de `requestShutdown(UserWindowClose)` (transición a `ShutdownRequested`), seguida de reversión voluntaria con `transitionTo(AppLifetimeState::Running)`.
      - Verificación de que `isShutdownRequested()` vuelve inmediatamente a `false`, permitiendo la operatividad plena y aceptando futuras solicitudes de apagado sin bloqueos por contadores residuales.
    - **Estrés multi-hilo**:
      - Pruebas de contención con decenas de hilos invocando simultáneamente `requestShutdown` con distintos motivos.
      - Invocación concurrente de `executeShutdown`: la bandera atómica `shutdownInProgress_` asegura que solo el hilo ganador orquesta el vaciado y cierre de subsistemas, evitando carreras críticas sobre `sessionSaver` y `dbCloser`.
    - **Escalación de señales repetidas a aborto de emergencia**:
      - Primera señal -> `ProceedGraceful`.
      - Segunda señal durante apagado en curso -> `IgnoreRequest` (apagado ya iniciado).
      - Tercera señal consecutiva -> `ProceedImmediate`, transicionando a `EmergencyAbort` con salida rápida (código 130 o 143) para garantizar respuesta ante usuarios impacientes (`Ctrl+C` repetido).
  - **Recuperación y tolerancia a fallos inyectados (Recovery & Fault Tolerance)**:
    - **Disparo determinista del Watchdog de seguridad**:
      - Inyección de bloqueo artificial en `sessionSaver` superior a `shutdownTimeoutMs`.
      - Verificación de que el coordinador aborta el vaciado, marca `timeoutTriggered = true`, transiciona a `EmergencyAbort`, cierra la base de datos para prevenir corrupción y finaliza en `Terminated` con código 1.
    - **Tolerancia a fallos de almacenamiento (`ShutdownReason::StorageFailure`)**:
      - Activación de salida de emergencia ante fallos irrecuperables de disco, registrando el motivo en el reporte.
    - **Agotamiento crítico de memoria (`ShutdownReason::MemoryExhaustion`)**:
      - Activación de salida rápida (`ProceedImmediate`) omitiendo el vaciado de estado para evitar asignaciones de memoria adicionales que provoquen un crash violento por OOM, ejecutando únicamente el cierre de SQLite.
  - **Aislamiento constitucional de incógnito (Q-001)**:
    - En modo incógnito (`isIncognito = true`), `executeShutdown` garantiza que `sessionSaver` jamás es invocado, `report.sessionSaved` es `false`, y no se genera ningún archivo temporal ni permanente en el sistema de archivos.
  - **Aislamiento de procesos y eficiencia de ejecución (Zero-Chromium & Zero-Zombie)**:
    - Pruebas 100% aisladas del motor WebEngine y de procesos Chromium, ejecutables en entornos headless de CI sin servidor X11 o Wayland.
    - Tiempo de ejecución total de la suite de pruebas inferior a 250 ms, con fixtures sintéticos y limpieza estricta mediante guardias RAII (`TempFileGuard`).
- **Gobernanza de CTest y CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
  - Mantenimiento del target de prueba `vigilio_lifetime_tests` asociado a `application_lifetime_tests` con etiquetas `"unit;fast"` y timeout de 15 segundos.
- **Sincronización documental en `docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md`**:
  - Actualización del estado a `CURRENT` una vez implementadas y aprobadas las pruebas en APPLY.

## Out of scope

- Modificación de la arquitectura o la interfaz visual de `BrowserWindow` implementada en `task0027` (salvo que una prueba revele un defecto funcional imperativo).
- Pruebas E2E pesadas con procesos gráficos reales de Chromium (`browser_e2e_smoke`), reservadas para perfiles dedicados y flujos visuales completos.
- Políticas de recuperación post-crash, journaling o reconstrucción de pestañas tras fallos inesperados del proceso (reservado para la capacidad `crash recovery policy`, tareas `task0029`-`task0032`).
- Modificación de código de producción bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida rigurosamente a `openspec/changes/module-foundations-task0028/**`).
- Cualquier llamada de red o telemetría remota (prohibido por la decisión Q-001).

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia y la suite de pruebas:
1. **Coordinador de ciclo de vida existente (`apps/vigilio-browser/src/core/ApplicationLifetimeCoordinator.h` y `.cpp`)**:
   - Implementado en `task0026`. Cuenta con una máquina de estados de 9 fases (`AppLifetimeState`), control atómico de concurrencia mediante `std::atomic`, watchdog de seguridad de 3000 ms y gestión de señales.
2. **Integración en UI (`apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`)**:
   - Completada en `task0027`. Gestiona el evento de cierre de ventana `closeEvent`, evalúa el número de pestañas abiertas, ofrece confirmación accesible mediante `confirmCloseMultipleTabs`, e invoca `executeShutdown` coordinando el vaciado de sesión y el cierre ordenado de `LocalRepository`.
3. **Persistencia atómica de sesión (`apps/vigilio-browser/src/core/SessionFile.h` y `.cpp`)**:
   - Implementa `saveSession` y `loadSession` con validación estricta de esquemas, límite de 1000 pestañas, longitud máxima de 8192 caracteres por URL, y reemplazo atómico en disco mediante archivo temporal (`.tmp`) y renombrado atómico (`MoveFileExW` / `std::filesystem::rename`).
4. **Estado actual de la suite de pruebas de ciclo de vida (`apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp`)**:
   - Contiene 11 casos de prueba básicos (416 líneas de código). Cubre el camino feliz, la matriz básica de transiciones, incógnito, manejo elemental de señales, watchdog sintético y conversiones a string.
   - **Brecha detectada**: Faltan pruebas focalizadas para:
     - Cancelación de apagado (reversión de `ShutdownRequested` a `Running` y verificación de reinicio limpio).
     - Manejo de excepciones o retornos fallidos en callbacks (`sessionSaver` fallido, `dbCloser` fallido, callbacks nulos).
     - Inyección de fallos de disco en `saveSession` durante el ciclo de apagado.
     - Pruebas de borde para `LifetimeConfig` (timeouts extremos de 0 ms o máximos).
     - Salidas forzadas por agotamiento de memoria (`MemoryExhaustion`) o fallo de almacenamiento (`StorageFailure`).
     - Concurrencia sobre `executeShutdown` desde múltiples hilos concurrentes.
5. **Pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con C++20 y `-Werror` bajo CMake y Ninja.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: 10/10 pruebas pasadas en 0.30s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.05s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.03s)
     - `resource_qrc_catalog_tests`: Passed (0.02s)
     - `resource_inclusion_service_tests`: Passed (0.03s)
     - `resource_qrc_inclusion_tests`: Passed (0.03s)
     - `startup_bootstrap_tests`: Passed (0.02s)
     - `application_lifetime_tests`: Passed (0.11s)
6. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0082-module.foundations.task0028.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 28/28 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Decisiones de foundations resueltas (Q-001 a Q-006, Q-008 resueltas; Q-007 solo bloquea `release.public-release`).

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `tests`.
- **Pack**: `module.foundations.task0028`.
- **Capacidad**: `application lifetime`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0028/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0028/proposal.md`
  - `openspec/changes/module-foundations-task0028/design.md`
  - `openspec/changes/module-foundations-task0028/tasks.md`
  - `openspec/changes/module-foundations-task0028/execution-memory.md`

## Diseño completo de la capacidad

El diseño completo para la etapa `tests` estructura una matriz de pruebas ortogonales dividida en 6 grupos de verificación funcional:

1. **Grupo 1 — Ciclo de vida positivo y métricas deterministas**:
   - Transiciones nominales de arranque y parada.
   - Validación de códigos de salida (0, 130, 143, 1).
   - Verificación de no-negatividad y coherencia de tiempos transcurridos (`sessionFlushElapsedMicroseconds`, `teardownElapsedMicroseconds`).
   - Evaluación exhaustiva de `report.isCleanExit()`.
2. **Grupo 2 — Restricciones de máquina de estados y transiciones inválidas**:
   - Verificación exhaustiva de que `canTransition` y `transitionTo` rechazan todos los saltos ilegales hacia atrás o entre estados no contiguos.
   - Inmutabilidad estricta del estado terminal `Terminated`.
   - Invocación de `executeShutdown` desde estados irregulares (`Starting`, `Running`) validando autocorrecion de estado.
3. **Grupo 3 — Manejo de errores y fallos en callbacks inyectados**:
   - Comportamiento resiliente ante `sessionSaver` que devuelve `false`: el guardado falla pero el ciclo continúa sin abortar abruptamente.
   - Tolerancia a punteros de función nulos en `sessionSaver` y `dbCloser`.
   - Comportamiento ante `dbCloser` que devuelve `false`: reporte fiel del estado sin generar fallos de segmentación.
4. **Grupo 4 — Cancelación y reactivación de ciclo de vida**:
   - Secuencia de cancelación voluntaria: `requestShutdown` transiciona a `ShutdownRequested`, pero al cancelar (diálogo de usuario), se invoca `transitionTo(AppLifetimeState::Running)`.
   - Verificación de que `isShutdownRequested()` se restablece y el sistema puede continuar operando indefinidamente.
   - Solicitud posterior de apagado procesada con éxito tras una cancelación previa.
5. **Grupo 5 — Resiliencia ante fallos externos y watchdog**:
   - Watchdog de 3000 ms activado por timeout inducido: confirmación de que la aplicación nunca se congela indefinidamente.
   - Fallo de almacenamiento (`ShutdownReason::StorageFailure`) y agotamiento de memoria (`ShutdownReason::MemoryExhaustion`).
   - Guardado de sesión integrado con fallo inducido en sistema de archivos (directorio protegido o ruta inválida).
6. **Grupo 6 — Concurrencia y contención multi-hilo**:
   - Solicitudes concurrentes masivas a `requestShutdown` desde múltiples hilos de ejecución.
   - Solicitudes concurrentes masivas a `executeShutdown`, garantizando que las tareas de vaciado se ejecutan exactamente una vez mediante exclusión mutua atómica.
   - Manejo de señales repetidas escalando a aborto de emergencia (`ProceedImmediate`).

## Archivos afectados

- **Durante PROPOSE**:
  - `openspec/changes/module-foundations-task0028/canonical-reference.yaml` (existente)
  - `openspec/changes/module-foundations-task0028/proposal.md` (creado/refinado)
  - `openspec/changes/module-foundations-task0028/design.md` (creado/refinado)
  - `openspec/changes/module-foundations-task0028/tasks.md` (creado/refinado)
  - `openspec/changes/module-foundations-task0028/execution-memory.md` (creado/refinado)
- **Durante APPLY (futuro incremento)**:
  - `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp` (ampliación de la suite con los nuevos casos de prueba)
  - `docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md` (sincronización de estado a CURRENT)

## Contrato UX/UI

- **Sin cambio visual directo**: Esta microtarea pertenece a la etapa **tests** y se centra exclusivamente en pruebas automatizadas unitarias, de integración, concurrencia y recuperación en C++20 para la capacidad `application lifetime`. No añade ni modifica pantallas, estilos CSS, widgets Qt, iconos ni componentes visuales.
- **Conformidad con `docs/UX-COPY-PUBLICO.md`**:
  - Ninguna prueba, mensaje de log visible o reporte de error público expone terminología técnica prohibida como «application lifetime», «graceful teardown», «kill signal», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» o «OpenSpec».
  - Las aserciones de pruebas que validan los textos de diálogos de confirmación o badges de ventana verifican taxativamente el uso de las etiquetas públicas aprobadas: «Interfaz más fácil de usar», «Cerrar navegador», «Navegación privada», «Cerrar todas», «Cancelar».

## Reglas y fallos

- **Aislamiento de incógnito (Q-001)**: Terminantemente prohibido guardar sesión o dejar rastros en disco cuando `isIncognito == true`.
- **Límite de tiempo estricto**: Todo vaciado de estado durante el apagado está acotado por `shutdownTimeoutMs` (3000 ms por defecto). Si se excede, el watchdog aborta el proceso sin dejar la aplicación colgada.
- **Zero-Zombie**: Ninguna prueba puede ejecutar bucles infinitos, esperas activas sin timeout (`sleep` indefinido), procesos colgados o dependencias de red externa.
- **Idempotencia y seguridad de hilos**: Múltiples solicitudes de apagado concurrentes o consecutivas no deben provocar condiciones de carrera, doble liberación de memoria ni corrupción de estado.
- **Gestión de fallos de disco**: Si el archivo de sesión no puede ser escrito por permisos o falta de espacio, `saveSession` devuelve `false` y el coordinador prosigue al cierre ordenado de la base de datos sin crashear.

## Seguridad y privacidad

- **Local-first constitucional (decisión Q-001)**: Cero peticiones de red o telemetría durante el ciclo de vida o las pruebas.
- **Privacidad de incógnito**: Ausencia verificable de persistencia de datos en perfiles privados.
- **Sanitización de rutas y memoria**: Las pruebas utilizan directorios sintéticos temporales creados bajo el directorio temporal del sistema y limpiados automáticamente por destructores RAII.
- **Aislamiento de procesos**: No se inician procesos de renderizado WebEngine ni servicios de red durante las pruebas unitarias de ciclo de vida.

## Matriz de pruebas

| Identificador | Nivel | Enfoque | Escenario probado | Resultado esperado | Comando de ejecución |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TEST-001** | Unitario | Positivo | Ciclo completo nominal (`Uninitialized` -> `Running` -> `Terminated`) | Transiciones válidas, código de salida 0, `isCleanExit() == true` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-002** | Unitario | Positivo | Códigos de salida para señales POSIX (`SIGINT` = 130, `SIGTERM` = 143) | Códigos 130 y 143 registrados fielmente en `LifetimeReport` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-003** | Unitario | Negativo | Intentos de transiciones inválidas y reversas en la matriz de estados | `canTransition` y `transitionTo` devuelven `false` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-004** | Unitario | Negativo | Inmutabilidad de estado terminal `Terminated` | Ninguna transición posterior es permitida | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-005** | Integración | Resiliencia | Fallo inducido en callback `sessionSaver` (retorna `false`) | `sessionSaved == false`, el flujo prosigue y la base de datos se cierra limpiamente | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-006** | Unitario | Robustez | Punteros nulos (`nullptr`) en `sessionSaver` y `dbCloser` | Ejecución segura sin crash ni violaciones de acceso | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-007** | Integración | Resiliencia | Fallo inducido en callback `dbCloser` (retorna `false`) | `databaseClosedCleanly == false`, coordinador transiciona a `Terminated` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-008** | Integración | Recuperación | Cancelación voluntaria de apagado (`ShutdownRequested` -> `Running`) | `isShutdownRequested() == false`, operatividad restaurada, posterior apagado exitoso | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-009** | Concurrencia | Estrés | 32 hilos concurrentes invocando `requestShutdown` simultáneamente | Exactamente 1 solicitud `ProceedGraceful`, 31 `IgnoreRequest`, sin data races | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-010** | Concurrencia | Estrés | Hilos concurrentes compitiendo en `executeShutdown` | Solo el primer hilo ejecuta los callbacks; el estado final es consistente | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-011** | Unitario | Emergencia | Señales repetidas durante apagado acelerando a `EmergencyAbort` | 3ª señal genera `ProceedImmediate`, transiciona a `EmergencyAbort`, salida 130 | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-012** | Integración | Recuperación | Watchdog timeout: `sessionSaver` excede `shutdownTimeoutMs` | `timeoutTriggered == true`, aborto de emergencia, salida 1, base de datos cerrada | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-013** | Unitario | Frontera | Configuración de timeout de 0 ms en `LifetimeConfig` | Cualquier tiempo transcurrido activa inmediatamente `timeoutTriggered` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-014** | Unitario | Privacidad | Invariante de incógnito (Q-001): `isIncognito == true` | Cero llamadas a `sessionSaver`, cero escrituras en disco, `sessionSaved == false` | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-015** | Integración | Recuperación | Fallo crítico por `MemoryExhaustion` y `StorageFailure` | Transición a `EmergencyAbort`, vaciado omitido, cierre limpio de DB si es posible | `ctest -R application_lifetime_tests --output-on-failure` |
| **TEST-016** | Unitario | Consistencia | Conversión bidireccional a texto de estados, motivos y acciones | Coincidencia exacta de cadenas textuales representativas | `ctest -R application_lifetime_tests --output-on-failure` |

## Pasos APPLY

1. **APPLY-TASK-001**: Implementar los nuevos casos de prueba en `apps/vigilio-browser/tests/unit/ApplicationLifetimeTests.cpp` agrupados por funciones de test focalizadas:
   - `test_lifetime_cancellation_and_recovery()`: cubre TEST-008.
   - `test_lifetime_callback_failures_resilience()`: cubre TEST-005, TEST-006, TEST-007.
   - `test_lifetime_direct_execute_shutdown()`: cubre ejecución directa sin solicitud previa.
   - `test_lifetime_extreme_timeouts()`: cubre TEST-013 y timeouts máximos.
   - `test_lifetime_concurrent_execute_shutdown()`: cubre TEST-010.
   - `test_lifetime_critical_reasons()`: cubre TEST-015 (`MemoryExhaustion`, `StorageFailure`).
   - `test_lifetime_timing_metrics()`: cubre recolección de microsegundos en el reporte.
2. **APPLY-TASK-002**: Ejecutar la suite ampliada de pruebas con `bash apps/vigilio-browser/scripts/check.sh` y certificar compilación limpia bajo C++20 con `-Werror` y 100% de tests pasando en CTest.
3. **APPLY-TASK-003**: Formatear el código modificado con Clang-Format según `apps/vigilio-browser/.clang-format`.
4. **APPLY-TASK-004**: Sincronizar la documentación de la microtarea en `docs/rules/01-modules/foundations/micro/0028-application-lifetime/task.md` actualizando su estado a CURRENT.

## Definición de terminado

- La suite de pruebas de `ApplicationLifetimeTests.cpp` cubre todos los casos positivos, negativos, concurrentes, de cancelación y recuperación descritos en la matriz.
- El 100% de las pruebas pasa en CTest con exit code 0 y tiempo de ejecución < 0.5s.
- No se introducen dependencias externas ni de red (local-first constitucional).
- Cero advertencias del compilador bajo `-Werror` con C++20.
- Código formateado estrictamente con Clang-Format.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0027`: Completada y verificada. La integración en UI ya está disponible en `BrowserWindow`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Pruebas de concurrencia inestables o propensas a flakiness.
    *Mitigación*: Uso estricto de tipos atómicos de C++20 (`std::atomic`) con semántica de memoria explícita (`acquire`, `release`, `relaxed`), barreras de sincronización (`std::thread::join()`), y ausencia total de `sleep` fijos para sincronización lógica.
  - *Riesgo*: Pruebas que dejan procesos o archivos temporales huérfanos.
    *Mitigación*: Enfoque Zero-Zombie, RAII estricto en guardias de archivos (`TempFileGuard`) y ejecución 100% en el mismo proceso sin invocar ejecutables externos.

## Commit propuesto

`test(foundations): tests application-lifetime`
