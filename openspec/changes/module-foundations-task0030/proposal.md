# Change Proposal — module-foundations-task0030

Pack: `module.foundations.task0030`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **implementation** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0030 — Implementar la operación en una capa propietaria: crash recovery policy`).

Habiendo formalizado en la microtarea precedente (`module.foundations.task0029`) el modelo de dominio, las enumeraciones tipadas (`CrashDetectionState`, `RecoveryAction`, `CrashReason`), las estructuras inmutables de centinela y reporte (`CrashSentinel`, `CrashRecoveryReport`, `CrashRecoveryConfig`), el algoritmo de detección de terminación anómala y la protección contra bucles de caídas repetidas (crash loops), esta propuesta proyecta la **implementación física, desacoplada y tipada en C++20 de la capa de servicio y adaptador de operaciones de recuperación tras caídas (`CrashRecoveryCoordinator` y su integración de servicio operativo `CrashRecoveryService`)**.

El objetivo central de esta etapa es proporcionar una capa operativa en `src/core` que orqueste la inicialización del centinela de sesión, el latido periódico (heartbeat), la gestión de decisiones de recuperación del usuario (restauración inmediata, restauración diferida, descarte o inicio limpio), el soporte para cancelación y rollback de recuperación, y la integración sincronizada con el ciclo de vida de la aplicación (`ApplicationLifetimeCoordinator`), **sin acoplar lógica de negocio a los widgets de interfaz gráfica ni añadir responsabilidades indebidas a `BrowserWindow`**.

El alcance concreto de este pack comprende:
- **Capa operativa de servicio y coordinación en C++20 (`CrashRecoveryCoordinator` / `CrashRecoveryService`)**:
  - Encapsulamiento del ciclo de vida de recuperación: evaluación inicial, inicio de sesión activa, latido periódico, aplicación de recuperación, descarte seguro de sesión y marcado de salida limpia.
  - Gestión transaccional y atómica de decisiones: si el navegador detecta una caída previa, la capa retiene las URLs recuperables en memoria de forma segura y provee operaciones tipadas:
    - `applyRecovery()`: devuelve las URLs saneadas validadas para su apertura y actualiza el centinela.
    - `discardRecovery()`: descarta las URLs de la sesión previa, elimina o archiva de forma segura el archivo de sesión (`session.json`) a `.discarded.bak` y marca el inicio limpio.
    - `cancelRecovery()`: revierte la solicitud de restauración manteniendo intacto el estado para decisiones posteriores sin bloquear la UI.
- **Integración con el coordinador de ciclo de vida (`ApplicationLifetimeCoordinator`)**:
  - Enlace con el flujo de apagado ordenado (`executeShutdown`): cuando `ApplicationLifetimeCoordinator` alcanza exitosamente el estado `AppLifetimeState::Terminated` con código de salida 0 y sin activación de watchdog timeout (`timeoutTriggered == false`), se ejecuta el marcado atómico de salida limpia (`markSessionClean`), cerrando el centinela con `clean_exit: true` y reseteando `consecutive_crashes` a 0.
  - En caso de aborto de emergencia (`EmergencyAbort`, `SIGSEGV`, `SIGABRT` o corte abrupto), el centinela deliberadamente **no** se marca como limpio, preservando el estado para que la próxima ejecución detecte la terminación anómala.
- **Protocolo de latido periódico (Heartbeat Monitoring)**:
  - Operación `updateHeartbeat` de baja sobrecarga (< 0.2 ms) para registrar marcas de tiempo vivas (`lastHeartbeatTime`) durante la sesión activa, permitiendo diagnósticos forenses locales sin enviar datos a la red.
- **Aislamiento estricto de Incógnito (Decisión Q-001)**:
  - Si `config.isIncognito == true`, toda la capa de crash recovery permanece inerte: no se crea, lee ni modifica ningún archivo centinela ni de sesión en disco; todas las operaciones devuelven éxito inmediato en memoria.
- **Protección contra Crash Loops y escalado a Modo Seguro**:
  - Detección determinista de $\ge 2$ caídas consecutivas sin sesión estable intercalada. La capa recomienda o activa automáticamente `SafeModeRecovery`, evitando que pestañas corruptas o extensiones defectuosas bloqueen el inicio del navegador.
- **Resiliencia de E/S atómica y bounded buffers**:
  - Manejo atómico con archivos temporales `.tmp` y reemplazo atómico (`MoveFileExW` en Windows, `std::filesystem::rename` en POSIX).
  - Límite estricto de 1000 pestañas y 8192 bytes por URL; esquemas restringidos a `http://`, `https://` y páginas internas `vigilio://`.
- **Suite completa de pruebas unitarias y de resiliencia (`CrashRecoveryTests.cpp`)**:
  - Pruebas exhaustivas de casos normales, errores de disco, corrupción, cancelación, idempotencia, latido y concurrencia.

---

## Out of scope

- Construcción de los widgets Qt visuales, componentes de banner accesible (`vigilio-crash-recovery-banner`), botones de restauración, diálogos modales y atajos de teclado (reservado para la etapa `ui` en `module.foundations.task0031`).
- Pruebas E2E automatizadas con inyección de señales del sistema operativo (`kill -9`, `kill -SEGV`) o procesos reales de Chromium WebEngine (reservado para la etapa `tests` en `module.foundations.task0032`).
- Recolección de volcados de memoria binarios (core dumps / minidumps de WebEngine), los cuales son administrados por el sistema operativo.
- Modificación directa de archivos de código fuente de producción bajo `apps/**` o reglas bajo `docs/rules/**` durante esta fase PROPOSE (restringida rigurosamente a `openspec/changes/module-foundations-task0030/**`).
- Cualquier comunicación por red, comprobación de actualizaciones en la nube, autenticación o telemetría remota (terminantemente prohibido por la política local-first, decisión Q-001).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia y la suite de pruebas:
1. **Especificación de dominio aprobada (`openspec/changes/module-foundations-task0029/`)**:
   - Formalizó la máquina de estados de detección de caídas (`CrashDetectionState`), las acciones de resolución (`RecoveryAction`), las causas tipadas (`CrashReason`), el formato del centinela `CrashSentinel`, la configuración `CrashRecoveryConfig` y el reporte consolidado `CrashRecoveryReport`.
2. **Archivos existentes en `apps/vigilio-browser/src/core/`**:
   - `CrashRecoveryTypes.h`: Define los tipos de valor inmutables, enums y estructuras del dominio de recuperación.
   - `CrashRecoveryCoordinator.h` y `.cpp`: Provee métodos estáticos desacoplados para inspeccionar el estado previo de sesión (`inspectSessionState`), marcar sesión activa (`markSessionActive`), marcar salida limpia (`markSessionClean`), actualizar latido (`updateHeartbeat`), sanear URLs (`sanitizeRecoverableUrls`) y archivar sesiones corruptas (`archiveCorruptedSession`).
   - `SessionFile.h` y `.cpp`: Persistencia atómica de URLs de pestañas en `session.json` con cabecera `VIGILIO_SESSION_V1`, límite de 1000 pestañas y 8192 bytes por URL.
   - `ApplicationLifetimeCoordinator.h` y `.cpp`: Coordinador de ciclo de vida con 9 estados (`Uninitialized` hasta `Terminated`), temporizador de seguridad de 3000 ms y método `executeShutdown`.
   - `StartupBootstrapCoordinator.h` y `.cpp`: Coordinador de bootstrap que analiza la línea de comandos (`--safe-mode`, `--incognito`, `--headless`) y prepara el entorno.
3. **Punto de integración de UI actual (`apps/vigilio-browser/src/ui/BrowserWindow.cpp`)**:
   - Actualmente, `BrowserWindow::BrowserWindow` invoca `restoreSession()` de forma pasiva leyendo de `QSettings`, sin inspeccionar `sentinel.json` ni consultar `CrashRecoveryCoordinator`.
   - En `BrowserWindow::closeEvent`, `lifetimeCoordinator_->executeShutdown` guarda la sesión en `session.json`, pero no coordina el marcado de salida limpia en `sentinel.json`.
   - Esto evidencia la necesidad de una capa de servicio unificada en `core` que conecte la inicialización, la recuperación, el latido y el apagado limpio sin introducir lógica de negocio dentro de `BrowserWindow`.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación Ninja limpia bajo C++20 con `-Werror` y cero advertencias.
   - Ejecución de `bash apps/vigilio-browser/scripts/check.sh`: 11/11 pruebas pasadas exitosamente con 100% de éxito en 0.60s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.06s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.05s)
     - `resource_qrc_catalog_tests`: Passed (0.01s)
     - `resource_inclusion_service_tests`: Passed (0.03s)
     - `resource_qrc_inclusion_tests`: Passed (0.15s)
     - `startup_bootstrap_tests`: Passed (0.13s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.00s)
5. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0088-module.foundations.task0030.propose.lock.json --project-root .`: 13/13 comprobaciones aprobadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 30/30 referencias canónicas validadas (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma que las decisiones Q-001 a Q-006 están resueltas (la única pendiente Q-007 solo bloquea `release.public-release`).
6. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal con pestañas y barra de navegación.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar.
   - `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («crash dump», «SIGSEGV», «watchdog abort», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces públicas o mensajes visibles al usuario. Uso obligado de «Interfaz más fácil de usar».

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0030`.
- **Capacidad**: `crash recovery policy`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0030/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0030/proposal.md`
  - `openspec/changes/module-foundations-task0030/design.md`
  - `openspec/changes/module-foundations-task0030/tasks.md`
  - `openspec/changes/module-foundations-task0030/execution-memory.md`

---

## Diseño completo de la capacidad

La solución integral de **crash recovery policy** en su etapa **implementation** estructura la arquitectura operativa en los siguientes componentes modulares:

### 1. Servicio operativo de recuperación (`CrashRecoveryService`)
Ubicado en `src/core/`, este servicio encapsula el estado transaccional de recuperación de sesión:
- **Estados operativos (`RecoveryServiceState`)**:
  - `Uninitialized`: Servicio instanciado pero no inicializado.
  - `Inspected`: Inspección de inicio completada; reporte disponible.
  - `AwaitingDecision`: Se detectó caída anómala; las URLs recuperables están retenidas en memoria esperando decisión del usuario (en etapa UI).
  - `SessionRestored`: Las URLs recuperables fueron transferidas y aceptadas.
  - `SessionDiscarded`: El usuario o la política descartó la sesión previa; el archivo de sesión fue archivado o limpiado.
  - `SessionActive`: El centinela actual está activo con `clean_exit: false`.
  - `SessionClean`: El centinela actual fue marcado como cerrado ordenadamente.
- **Operaciones principales**:
  - `initialize(config, pid, timestamp)`: Ejecuta la inspección atómica; si no hay caídas previas o la salida fue limpia, marca la sesión como activa de inmediato y pasa a `SessionActive`. Si se detecta caída anómala, pasa a `AwaitingDecision` y retiene las URLs sin invocar WebEngine.
  - `applyRecovery()`: Aplica la restauración de sesión, retorna las URLs saneadas, y marca la sesión como activa en disco.
  - `discardRecovery()`: Descarta las URLs recuperables, renombra `session.json` a `.discarded.bak`, inicializa una sesión limpia y marca la sesión como activa.
  - `cancelRecovery()`: Cancela la operación de recuperación manteniendo el estado accesible para decisiones posteriores sin bloquear la interfaz.
  - `recordHeartbeat(timestamp)`: Actualiza la marca de tiempo en el centinela atómicamente.
  - `finalizeShutdown(cleanExit, reason)`: Cierra la sesión activa en el centinela; si `cleanExit == true`, invoca `markSessionClean`.

### 2. Integración simbiótica con `ApplicationLifetimeCoordinator`
- `ApplicationLifetimeCoordinator::executeShutdown` se coordina con la política de recuperación:
  - En la fase final de apagado ordenado (`AppLifetimeState::Terminated`), se confirma el marcado de salida limpia (`markSessionClean`).
  - Si ocurre un apagado anormal o aborto de emergencia (`EmergencyAbort`, por ejemplo por `SIGINT`/`SIGTERM` repetidos o por temporizador watchdog superado > 3000 ms), el centinela mantiene `clean_exit = false` para que la subsiguiente invocación del navegador identifique el fallo.

### 3. Aislamiento constitucional de Incógnito
- En modo incógnito (`isIncognito == true`), el servicio opera enteramente en modo de simulación en memoria:
  - Cero operaciones de lectura o escritura en disco.
  - `initialize` retorna inmediatamente `CrashDetectionState::NoPreviousSession` y `RecoveryAction::StartClean`.
  - Ninguna URL ni marca de tiempo se persiste en ningún momento.

---

## Archivos afectados

### Durante PROPOSE (etapa actual)
- `openspec/changes/module-foundations-task0030/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0030/proposal.md`
- `openspec/changes/module-foundations-task0030/design.md`
- `openspec/changes/module-foundations-task0030/tasks.md`
- `openspec/changes/module-foundations-task0030/execution-memory.md`

### Durante APPLY (próxima etapa `0089`)
- `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Refinamiento de tipos operativos y estados de servicio.
- `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h`: Declaración de la interfaz operativa del servicio y helpers de coordinación.
- `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`: Implementación de la lógica operativa, transiciones de decisión, cancelación y callbacks de recuperación.
- `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`: Ampliación de la suite de pruebas unitarias para certificar transiciones de servicio, cancelación, descarte de sesión y robustez de latido.
- `apps/vigilio-browser/CMakeLists.txt`: Mantenimiento y verificación del target `vigilio_crash_recovery_tests`.
- `docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md`: Sincronización del estado a ejecutado tras APPLY.

---

## Contrato UX/UI

- **Impacto visual en esta etapa**: **Sin cambio visual directo**.
  - Esta etapa (`implementation`) implementa exclusivamente la capa de servicio C++20 desacoplada en `src/core`, sin crear ni modificar widgets de interfaz gráfica.
  - La construcción del banner visual de recuperación (`vigilio-crash-recovery-banner`), sus botones interactivos, foco accesible y adaptación a la «Interfaz más fácil de usar» (Modo Senior) se realizarán estrictamente en la etapa `ui` (`module.foundations.task0031`).
- **Garantías hacia la UI**:
  - La capa de servicio en `core` expone estados y señales/callbacks desacoplados (`AwaitingDecision`, URLs retenidas) para que la capa UI pueda consumir los datos sin tener que gestionar lógica de archivos ni parseos.
  - Cero operaciones bloqueantes de disco en el hilo principal: lecturas y escrituras atómicas ultrarrápidas (< 1 ms).
- **Cumplimiento de `docs/UX-COPY-PUBLICO.md`**:
  - Los mensajes estructurados de diagnóstico (`diagnosticMessage`) no contienen jerga técnica visible («SIGSEGV», «minidump», «watchdog abort»). Toda información dirigida a la UI se formula en lenguaje claro y comprensible.

---

## Reglas y fallos

1. **Detección de Caída Anómala**:
   - Condición: `sentinel.json` existe con `clean_exit == false`.
   - Diagnóstico: Terminación anómala detectada (`AbnormalTermination`).
   - Acción: No restaurar pestañas automáticamente; retener URLs y recomendar `PromptRestore`.
2. **Protección contra Bucle de Caídas (Crash Loop)**:
   - Condición: `sentinel.consecutive_crashes >= config.crashLoopThreshold` ($\ge 2$).
   - Diagnóstico: Caídas repetidas consecutivas (`RepeatedCrashes`).
   - Acción: Recomendar inicio en modo seguro (`SafeModeRecovery`) o inicio limpio para proteger al usuario.
3. **Archivo de Sesión o Centinela Corrupto**:
   - Condición: JSON malformado, truncado o cabecera inválida.
   - Diagnóstico: `CorruptedSession`.
   - Recuperación: Archivar archivo defectuoso a `.corrupt.bak` y reanudar con perfil limpio sin crashear.
4. **Fallo de Escritura en Disco / Permisos Denegados**:
   - Condición: Sistema de archivos de solo lectura o sin espacio libre.
   - Recuperación: Operar en memoria de forma degradada, retornar `false` en operaciones de persistencia sin provocar caída del proceso.
5. **Aislamiento de Incógnito**:
   - Condición: `config.isIncognito == true`.
   - Invariante: Cero operaciones de E/S en disco; reportar siempre `StartClean`.

---

## Seguridad y privacidad

- **Local-First Constitucional (Decisión Q-001)**:
  - Toda la persistencia y diagnóstico de caídas es 100% local en la carpeta del perfil del usuario.
  - Jamás se contacta ningún servidor remoto, endpoint de telemetría o servicio cloud para reportar caídas.
- **Aislamiento estricto de Incógnito**:
  - Las sesiones privadas jamás dejan rastro en `sentinel.json` ni en `session.json`.
- **Saneamiento riguroso de URLs**:
  - Se descartan esquemas peligrosos (`javascript:`, `data:`, `file:`) y cadenas que superen 8192 bytes o listas mayores a 1000 pestañas, evitando desbordamientos de memoria o inyecciones de código.
- **Resiliencia de E/S atómica**:
  - Escrituras atómicas con `.tmp` y reemplazo atómico para evitar corrupción por cortes de corriente.

---

## Matriz de pruebas

| ID | Escenario | Fixture | Comando / Target | Resultado Esperado | Clasificación |
|---|---|---|---|---|---|
| TC-01 | Arranque limpio sin sesión previa | Perfil vacío | `vigilio_crash_recovery_tests` | `NoPreviousSession`, `StartClean`, 0 caídas | APPLY |
| TC-02 | Salida previa limpia ordenada | `sentinel.json` con `clean_exit: true` | `vigilio_crash_recovery_tests` | `CleanExit`, `RestoreSession`, 0 caídas | APPLY |
| TC-03 | Caída única anómala | `sentinel.json` con `clean_exit: false` | `vigilio_crash_recovery_tests` | `AbnormalTermination`, `PromptRestore`, 1 caída | APPLY |
| TC-04 | Bucle de caídas repetidas ($\ge 2$) | 2 fallos consecutivos | `vigilio_crash_recovery_tests` | `RepeatedCrashes`, `SafeModeRecovery`, 2 caídas | APPLY |
| TC-05 | Aislamiento absoluto de Incógnito | `isIncognito = true` | `vigilio_crash_recovery_tests` | Cero I/O en disco, tamaño de archivo invariable | APPLY |
| TC-06 | Centinela corrupto / JSON trunco | Archivo con JSON inválido | `vigilio_crash_recovery_tests` | Archiva a `.corrupt.bak`, recupera sin crash | APPLY |
| TC-07 | Archivo de sesión corrupto | Archivo con cabecera inválida | `vigilio_crash_recovery_tests` | `CorruptedSession`, archiva a `.corrupt.bak` | APPLY |
| TC-08 | Saneamiento estricto de URLs | URLs con `javascript:`, largos > 8K | `vigilio_crash_recovery_tests` | Solo URLs válidas admitidas, sin duplicados contiguos | APPLY |
| TC-09 | Ciclo de vida y marcado limpio | `markSessionActive` + `markSessionClean` | `vigilio_crash_recovery_tests` | Transición atómica correcta en disco | APPLY |
| TC-10 | Actualización periódica de latido | `updateHeartbeat` con timestamp | `vigilio_crash_recovery_tests` | Marca de tiempo persistida sin alterar `clean_exit` | APPLY |
| TC-11 | Descarte y cancelación de recuperación | `discardRecovery` / `cancelRecovery` | `vigilio_crash_recovery_tests` | Limpieza segura de URLs y persistencia estable | APPLY |
| TC-12 | Pipeline continuo local | Suite completa CMake | `bash apps/vigilio-browser/scripts/check.sh` | 100% pruebas pasando, exit code 0 | APPLY |

---

## Pasos APPLY

1. **Paso 1**: Refinar la capa de tipos en `apps/vigilio-browser/src/core/CrashRecoveryTypes.h` para soportar estados de servicio y operaciones transaccionales de recuperación.
2. **Paso 2**: Declarar los métodos operativos y de servicio en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` (o servicio complementario desacoplado).
3. **Paso 3**: Implementar la lógica de servicio, descarte seguro de sesión y soporte de cancelación en `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.cpp`.
4. **Paso 4**: Ampliar las pruebas unitarias en `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp` para validar el descarte, la cancelación y la orquestación del ciclo de vida.
5. **Paso 5**: Verificar el target `vigilio_crash_recovery_tests` en `apps/vigilio-browser/CMakeLists.txt`.
6. **Paso 6**: Ejecutar `bash apps/vigilio-browser/scripts/check.sh` y certificar compilación limpia bajo C++20, `-Werror`, Clang-Format y 100% tests aprobados.
7. **Paso 7**: Validar lockfile y referencias canónicas con Prompt Builder CLI.
8. **Paso 8**: Sincronizar documentación de microtarea en `docs/rules/01-modules/foundations/micro/0030-crash-recovery-policy/task.md`.

---

## Definición de terminado

- La capa operativa de crash recovery policy está implementada en C++20 puro, desacoplada de widgets y WebEngine.
- El centinela durable gestiona adecuadamente sesiones activas, salidas limpias, caídas anómalas y bucles de caídas.
- El modo incógnito garantiza cero accesos a disco (invariante constitucional Q-001).
- Toda la suite de pruebas unitarias pasa al 100% bajo CTest.
- Compilación Ninja limpia con `-Werror` sin advertencias.
- Prompt Builder valida referencias canónicas y lockfile con éxito.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0029`: Dominio y especificación base de crash recovery policy (completada y certificada).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Corrupción de archivos de estado ante pérdidas repentinas de alimentación eléctrica.
    *Mitigación*: Reemplazo atómico con archivos `.tmp` y renombrado transaccional. Si un archivo está corrupto, se archiva de forma segura a `.corrupt.bak` y el navegador arranca en limpio sin colapsar.
  - *Riesgo*: Bucle infinito de caídas por URLs maliciosas.
    *Mitigación*: Escalado automático a `SafeModeRecovery` ante $\ge 2$ caídas consecutivas.

---

## Commit propuesto

`feat(foundations): implementation crash-recovery-policy`
