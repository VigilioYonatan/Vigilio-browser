# Change Proposal — module-foundations-task0032

Pack: `module.foundations.task0032`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **crash recovery policy** en el módulo propietario `foundations` de Vigilio Browser (microtarea `0032 — Probar casos positivos, negativos y recuperación: crash recovery policy`).

Tras haberse completado exitosamente la especificación de dominio (`task0029`), la implementación física desacoplada del coordinador y servicio operativo (`task0030`), y la integración visual accesible en Qt Widgets (`task0031`), el objetivo de este pack es **construir y ejecutar una batería exhaustiva de pruebas automatizadas, deterministas y de alta resiliencia para crash recovery policy**, cubriendo:
- **Casos positivos y ciclo de vida normal**: arranque limpio, persistencia atómica de centinela, latido periódico (`recordHeartbeat`), cierre ordenado (`finalizeShutdown`), recuperación de sesiones anómalas (`applyRecovery`), y detección de bucle de caídas repetidas con escalado a Modo Seguro.
- **Casos negativos y entradas malformadas**: centinelas inexistentes, vacíos (0 bytes), truncados, con campos faltantes, con JSON sintácticamente corrupto, esquemas desconocidos, PIDs/timestamps fuera de rango o negativos, y desbordamiento aritmético en conteo de caídas.
- **Corrupción y descarte de archivos de sesión**: archivos `session.json` vacíos, sin cabecera `VIGILIO_SESSION_V1`, con secuencias binarias o líneas defectuosas; verificación de archivado atómico a `.corrupt.bak` y `.discarded.bak`.
- **Saneamiento riguroso de URLs y límites de memoria (Bounded buffers)**: rechazo de esquemas no autorizados (`javascript:`, `data:`, `file:`, `blob:`, `about:`, `vbscript:`), descarte de URLs de longitud excesiva (> 8192 bytes), URLs vacías, secuencias contiguas duplicadas, y límite estricto de 1000 pestañas.
- **Concurrencia, reentrancia y transiciones de estado**: verificación de latidos concurrentes multihilo sin condición de carrera, llamadas repetidas e idempotentes a `initialize`, `discardRecovery`, `cancelRecovery` y `finalizeShutdown`, y comportamiento ante llamadas fuera de orden en la máquina de estados de `CrashRecoveryService`.
- **Inyección de fallos en el sistema de archivos**: simulación de rutas de solo lectura (`read-only`), directorios no escribibles, resiliencia ante fallos en reemplazo atómico (`rename` / `MoveFileExW`) y limpieza automática de archivos temporales `.tmp`.
- **Invariante de navegación privada (Modo Incógnito, Decisión Q-001)**: certificación de aislamiento total y cero operaciones de E/S en disco bajo sesiones de incógnito (`isIncognito == true`).
- **Eficiencia y regresión sin Chromium**: pruebas rápidas (< 500 ms en CTest) aisladas mediante fixtures RAII locales en `/tmp`, sin requerir el arranque de procesos Chromium o GPU.

---

## Out of scope

- Modificación de contratos de negocio existentes o tipos de dominio en `CrashRecoveryTypes.h` (salvo que una prueba revele un defecto no contemplado).
- Rediseño de componentes visuales en Qt Widgets (`BrowserWindow`), ya implementados y certificados en la etapa `ui` (`task0031`).
- Pruebas E2E pesadas con lanzamiento de Chromium y carga de red externa (reservado para smoke tests de empaquetado/distribución).
- Comunicación remota, telemetría o servicios cloud de recolección de fallos (estrictamente prohibido por la política local-first del navegador, Decisión Q-001).
- Modificación de archivos fuera de las rutas autorizadas para esta etapa.

---

## Contexto y evidencia

### Inspección del estado actual del repositorio

1. **Capa de Dominio y Servicio (`src/core`)**:
   - `apps/vigilio-browser/src/core/CrashRecoveryTypes.h`: Define las enumeraciones tipadas `CrashDetectionState`, `RecoveryAction`, `CrashReason`, `RecoveryServiceState`, `RecoveryDecision`, y las estructuras `CrashSentinel`, `CrashRecoveryConfig`, `CrashRecoveryReport`, `RecoveryResolutionResult`.
   - `apps/vigilio-browser/src/core/CrashRecoveryCoordinator.h` & `.cpp`: Implementan la lógica de inspección (`inspectSessionState`), mutaciones atómicas (`markSessionActive`, `markSessionClean`, `updateHeartbeat`), saneamiento de URLs (`sanitizeRecoverableUrls`), archivado seguro (`archiveCorruptedSession`, `archiveDiscardedSession`), y el servicio con estado `CrashRecoveryService`.
2. **Capa de Interfaz de Usuario (`src/ui`)**:
   - `apps/vigilio-browser/src/ui/BrowserWindow.h` & `.cpp`: Integran el banner accesible `vigilio-crash-recovery-banner`, los botones «Restaurar pestañas», «Empezar de cero», «Cerrar aviso», el temporizador de latido `heartbeatTimer_` (30s), la carga diferida de pestañas (`lazy loading`), y el escalado para la «Interfaz más fácil de usar» (Modo Senior).
3. **Suite de Pruebas Existente**:
   - `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp`: Contiene 17 casos de prueba unitarios base (`test_crash_recovery_no_previous_session` a `test_crash_recovery_service_incognito`).
   - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Contiene verificaciones de UI para terminación anómala, salida limpia, aislamiento en incógnito, crash loop y atajo Escape.
4. **Verificación de Calidad y Pipeline (`apps/vigilio-browser/scripts/check.sh`)**:
   - Ejecución de Ninja y CTest: 11 de 11 tests pasando exitosamente (exit code 0 en 0.41s).
   - Validaciones de Prompt Builder: 32 referencias canónicas en `valid: true`, lockfile de 0094 verificado en `valid: true`.

---

## Alcance exacto

El alcance técnico de este pack está focalizado en la suite de pruebas unitarias y de integración de `crash recovery policy`, implementando:

1. **Batería de Pruebas de Centinela Malformado y Casos Límite**:
   - Archivo centinela con tamaño 0 bytes (fichero vacío).
   - Archivo centinela truncado abruptamente en mitad de clave o valor JSON.
   - Archivo centinela con JSON válido pero esquema desconocido (`schema_version != 1` o valor negativo/cero).
   - Archivo centinela con tipos de datos invertidos (e.g., cadenas en lugar de booleanos o números en lugar de cadenas).
   - Archivo centinela con PIDs y timestamps negativos o extremos.
   - Protección contra saturación aritmética en `consecutive_crashes` ($UINT32\_MAX$).
2. **Batería de Pruebas de Sesión Corrupta y Saneamiento Avanzado**:
   - Sesión con cabecera ausente o incorrecta.
   - Sesión vacía y sesión con miles de líneas (> 1000 pestañas) verificando límite estricto `kMaxTabs = 1000`.
   - URLs con esquemas maliciosos o no soportados (`javascript:alert(1)`, `data:text/html,...`, `file:///etc/passwd`, `blob:...`, `about:blank`, `vbscript:...`).
   - URLs gigantescas (> 8192 bytes) y cadenas con caracteres de control (`\0`, `\r`, `\n`).
3. **Batería de Pruebas de Concurrencia y Máquina de Estados**:
   - Latidos concurrentes multihilo (`updateHeartbeat` y `recordHeartbeat`) ejecutados en paralelo sin corromper el centinela en disco.
   - Verificación de llamadas fuera de orden en `CrashRecoveryService`: invocar `applyRecovery()` o `discardRecovery()` en estados `Uninitialized`, `SessionClean` o `SessionActive`.
   - Idempotencia en `cancelRecovery()`, `discardRecovery()` y `finalizeShutdown()`.
   - Re-inicialización segura de un servicio `CrashRecoveryService` previamente activo.
4. **Batería de Pruebas de Inyección de Fallos en Sistema de Archivos**:
   - Rutas en directorios de solo lectura (simulación de permisos restringidos donde no es posible crear archivos temporales o directorios padre).
   - Manejo seguro de fallos en `markSessionActive`, `markSessionClean` y `updateHeartbeat` devolviendo `false` sin lanzar excepciones no controladas.
   - Limpieza garantizada de archivos `.tmp` huérfanos tras fallos de escritura atómica.
   - Reemplazo ordenado de backups previos cuando ya existen archivos `.corrupt.bak` o `.discarded.bak`.
5. **Certificación de Aislamiento en Modo Incógnito (Q-001)**:
   - Verificación estricta de que bajo `config.isIncognito = true`, ninguna operación de `CrashRecoveryCoordinator` o `CrashRecoveryService` realiza llamadas al sistema de archivos ni altera rutas en disco.

---

## Diseño completo de la capacidad

### 1. Fixture de Aislamiento Determinista (`TempDirectoryFixture`)
Todas las pruebas operan sobre directorios temporales creados con marcas de tiempo de alta resolución (`std::chrono::steady_clock`), aislados de la configuración real del usuario y destruidos automáticamente al finalizar la ejecución mediante destructores RAII.

### 2. Patrón de Fallo Inyectado para Permisos de Archivos
Para probar el comportamiento ante errores de disco sin requerir privilegios de superusuario:
- Se crean subdirectorios dentro del fixture temporal.
- Se configuran permisos de solo lectura usando `std::filesystem::permissions(p, std::filesystem::perms::all, std::filesystem::perm_options::remove)` o rutas inexistentes/inválidas.
- Se comprueba que los métodos devuelven `false` de manera limpia y que la aplicación no entra en pánico ni aborta.

### 3. Modelo de Prueba de Concurrencia Multihilo
- Se ejecutan múltiples hilos concurrentes que invocan `recordHeartbeat` con marcas de tiempo progresivas.
- Al finalizar la sincronización (`join`), se valida que el centinela final en disco es sintácticamente válido, contiene una marca de tiempo coherente y no presenta estados a medio escribir gracias al reemplazo atómico (`writeSentinelAtomic`).

### 4. Cobertura de la Máquina de Estados de `CrashRecoveryService`
Validación de todos los arcos de transición válidos e inválidos:
- `Uninitialized` $\to$ `initialize` $\to$ `AwaitingDecision` (si hubo caída) o `SessionActive` (arranque limpio/recuperado).
- `AwaitingDecision` $\to$ `applyRecovery` $\to$ `SessionActive`.
- `AwaitingDecision` $\to$ `discardRecovery` $\to$ `SessionActive`.
- `AwaitingDecision` $\to$ `cancelRecovery` $\to$ `Inspected`.
- `Inspected` $\to$ `applyRecovery` $\to$ `SessionActive`.
- `SessionActive` $\to$ `finalizeShutdown(true)` $\to$ `SessionClean`.
- `SessionActive` $\to$ `finalizeShutdown(false)` $\to$ `SessionActive` (marcado sucio preservado).
- Verificación de que transiciones anómalas (e.g. invocar `applyRecovery` sobre `Uninitialized`) no causan UB ni excepciones no controladas.

---

## Archivos afectados

| Archivo | Acción | Justificación |
| :--- | :--- | :--- |
| `apps/vigilio-browser/tests/unit/CrashRecoveryTests.cpp` | Modificar | Incorporar las nuevas suites de pruebas especializadas: casos límite de centinela, fallos de sesión, concurrencia multihilo, fallos inyectados de sistema de archivos, y validación exhaustiva de estados. |
| `apps/vigilio-browser/tests/ui/WindowTests.cpp` | Modificar | Ampliar aserciones de regresión sobre el comportamiento del banner y su aislamiento frente a fallos inyectados en UI. |
| `docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md` | Modificar | Sincronizar el estado de la microtarea de `NOT_EXECUTED` a completado en la fase APPLY correspondiente. |
| `openspec/changes/module-foundations-task0032/*` | Crear/Modificar | Especificación completa OpenSpec del cambio (`proposal.md`, `design.md`, `tasks.md`, `execution-memory.md`). |

---

## Contrato UX/UI

- **Sin cambio visual directo en la aplicación**: Esta etapa corresponde estrictamente a pruebas automatizadas de lógica, integración y resiliencia de la capacidad.
- **Verificación de regresión de contratos visuales existentes**:
  - Banner accesible `vigilio-crash-recovery-banner`: se mantiene no intrusivo y posicionado en la cabecera.
  - Copias públicas aprobadas en español conforme a `docs/UX-COPY-PUBLICO.md`:
    - Caída anómala estándar: «El navegador no se cerró correctamente. ¿Quieres restaurar tus pestañas anteriores?».
    - Bucle de caídas: «El navegador se cerró varias veces de forma inesperada. Se recomienda abrir una sesión limpia para mayor estabilidad.».
  - Escalado en «Interfaz más fácil de usar» (Modo Senior): botones con altura $\ge 48$ px y contenedor $\ge 56$ px.
  - Controles de teclado: atajo `Escape` y foco cíclico garantizados.

---

## Reglas y fallos

1. **Centinela inexistente o vacío**:
   - Tratado de forma determinista como `NoPreviousSession` o inicio limpio si no hay sesión previa. Si el centinela tiene 0 bytes o está roto, se archiva a `.corrupt.bak` y se reporta `AbnormalTermination` para proteger la sesión del usuario.
2. **Sesión corrupta**:
   - Si `session.json` no contiene el encabezado `VIGILIO_SESSION_V1`, se archiva atómicamente a `session.json.corrupt.bak` y se recomienda `DiscardCorrupted`, evitando lecturas infinitas o parseos erróneos.
3. **Fallo de escritura en disco (Read-only o Permiso Denegado)**:
   - Toda función de mutación (`markSessionActive`, `markSessionClean`, `updateHeartbeat`, `writeSentinelAtomic`) comprueba errores de E/S. Ante un fallo, limpian archivos temporales `.tmp` y retornan `false` de forma segura sin arrojar excepciones no capturadas.
4. **Límites de memoria y URLs (Bounded Buffers)**:
   - Máximo de 1000 pestañas recuperables.
   - Longitud máxima de 8192 bytes por URL.
   - Filtrado estricto de esquemas peligrosos (`javascript:`, `data:`, `file:`, etc.).
5. **Aislamiento en Incógnito (Q-001)**:
   - En modo privado, no se genera ni se lee rastro alguno en disco.

---

## Seguridad y privacidad

- **Local-First Constitucional**: Cero llamadas remotas, telemetría o servicios cloud. Todos los diagnósticos de fallo permanecen estrictamente en la máquina local.
- **Aislamiento de Incógnito**: Prohibición de persistir URLs de navegación privada en centinelas o sesiones en disco.
- **Sanitización contra Inyección**: Rechazo de URLs ejecutables (`javascript:`, `data:`) que pudieran haberse inyectado en archivos manipulados por terceros.
- **Permisos de Archivo**: Creación y archivado con permisos estándar de usuario, evitando permisos de ejecución o exposición indebida.

---

## Matriz de pruebas

La siguiente matriz define las pruebas especializadas de la etapa `tests` para crash recovery policy:

| ID de Prueba | Ámbito | Componente / Fixture | Entrada / Condición | Resultado Esperado | Modo de Fallo Negativo |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `TEST-REC-001` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Centinela con tamaño 0 bytes (vacío) | Detecta centinela roto, archiva `.corrupt.bak`, reporta `AbnormalTermination` | Crash por parseo o reporte limpio falso |
| `TEST-REC-002` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Centinela con JSON truncado sin cierre | Archiva a `.corrupt.bak`, reporta `sessionCorrupted = true` | Excepción no capturada |
| `TEST-REC-003` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Centinela con `schema_version = 99` o `0` | Rechaza esquema no compatible, archiva `.corrupt.bak` | Aceptación de esquema inválido |
| `TEST-REC-004` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Centinela con tipos invertidos (cadena en `clean_exit`) | Parseo falla de forma segura, retorna `false`, backup creado | Lectura errónea de tipo |
| `TEST-REC-005` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | `consecutive_crashes` con valor extremo ($UINT32\_MAX$) | Incremento seguro sin overflow ni comportamiento indefinido | Desbordamiento o wrap-around a 0 |
| `TEST-REC-006` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Sesión `session.json` vacía (0 bytes) | Detecta cabecera ausente, archiva `.corrupt.bak`, `DiscardCorrupted` | Bucle infinito o parseo de URLs nulas |
| `TEST-REC-007` | Unitario | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Sesión con 1500 URLs válidas | Sanitización trunca a exactamente 1000 URLs (`kMaxTabs`) | Carga de más de 1000 pestañas en RAM |
| `TEST-REC-008` | Seguridad | `CrashRecoveryCoordinator::sanitizeRecoverableUrls` | URLs con `javascript:`, `data:`, `file:`, `blob:` | Solo conserva `http://`, `https://`, `vigilio://`; descarta el resto | Inyección de script o acceso a archivos locales |
| `TEST-REC-009` | Seguridad | `CrashRecoveryCoordinator::sanitizeRecoverableUrls` | URL de más de 8192 bytes | Descarta la URL gigante preservando las válidas | Consumo no acotado de memoria |
| `TEST-REC-010` | Concurrencia| `CrashRecoveryCoordinator` / Hilos múltiples | 10 hilos ejecutando `updateHeartbeat` concurrentemente | Cero corrupción de archivo, centinela final consistente y parseable | Condición de carrera o centinela corrupto |
| `TEST-REC-011` | Máquina Estados | `CrashRecoveryService` | Invocación de `applyRecovery` en estado `Uninitialized` | Retorna vector vacío sin mutar disco ni lanzar excepción | Violación de segmento o estado corrupto |
| `TEST-REC-012` | Máquina Estados | `CrashRecoveryService` | Múltiples llamadas consecutivas a `discardRecovery()` | Primera llamada archiva sesión; subsecuentes retornan éxito sin error | Fallo por archivo inexistente |
| `TEST-REC-013` | Máquina Estados | `CrashRecoveryService` | Re-inicialización de servicio activo con nueva configuración | Reinicia estado de forma coherente y actualiza configuración | Fuga de recursos o estado incongruente |
| `TEST-REC-014` | Fallo Inyectado | `CrashRecoveryCoordinator` / Directorio de solo lectura | `markSessionActive` sobre directorio sin permisos de escritura | Retorna `false` de forma limpia; elimina `.tmp` temporal | Crash o bloqueo del proceso |
| `TEST-REC-015` | Fallo Inyectado | `CrashRecoveryCoordinator` / `TempDirectoryFixture` | Archivación a `.corrupt.bak` cuando ya existe un backup previo | Sobrescribe ordenadamente el backup viejo sin error | Fallo de renombrado o bloqueo |
| `TEST-REC-016` | Rendimiento | Pipeline `check.sh` / CTest | Ejecución completa de suite de pruebas unitarias | 100% de tests aprobados en < 500 ms sin procesos Chromium | Pruebas lentas o flaky |

---

## Pasos APPLY

Plan de ejecución secuencial y determinista para la fase APPLY (`module.foundations.task0032` en ejecución `0095`):

1. **Diseño e Implementación de Casos de Prueba Límite en `CrashRecoveryTests.cpp`**:
   - Implementar `test_crash_recovery_corrupted_sentinel_variations()`: validación de centinelas vacíos, truncados, esquemas inválidos y campos con tipos incongruentes.
   - Implementar `test_crash_recovery_corrupted_session_variations()`: archivos de sesión vacíos, cabeceras rotas y líneas corruptas con verificación de backup `.corrupt.bak`.
   - Implementar `test_crash_recovery_bounds_and_overflow()`: límites estrictos de 1000 pestañas, URLs de longitud excesiva (> 8192 bytes) y saturación de conteo de caídas.
2. **Implementación de Pruebas de Seguridad y Sanitización en `CrashRecoveryTests.cpp`**:
   - Implementar `test_crash_recovery_security_schemes()`: verificación exhaustiva contra esquemas no permitidos (`javascript:`, `data:`, `file:`, `blob:`, `about:`, `vbscript:`).
3. **Implementación de Pruebas de Concurrencia y Resiliencia en `CrashRecoveryTests.cpp`**:
   - Implementar `test_crash_recovery_concurrent_heartbeats()`: concurrencia multihilo ejecutando actualizaciones atómicas simultáneas y validando la consistencia final del centinela.
4. **Implementación de Pruebas de Máquina de Estados y Fallos Inyectados en `CrashRecoveryTests.cpp`**:
   - Implementar `test_crash_recovery_service_invalid_transitions()`: invocaciones fuera de orden (`applyRecovery`, `discardRecovery` antes de `initialize`).
   - Implementar `test_crash_recovery_filesystem_permission_fault()`: directorio de solo lectura y verificación de retorno `false` seguro con limpieza de `.tmp`.
   - Implementar `test_crash_recovery_backup_overwriting()`: archivado sucesivo sobre backups existentes (`.corrupt.bak`, `.discarded.bak`).
5. **Verificación y Cobertura de UI en `WindowTests.cpp`**:
   - Añadir aserciones de regresión para asegurar que la UI no intente restaurar sesiones corruptas cuando el servicio reporta `sessionCorrupted = true`.
6. **Ejecución y Verificación de Calidad**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh` asegurando compilación limpia con `-Werror`, formato Clang-Format impecable y 100% de tests pasando en CTest.
   - Validar referencias canónicas y lockfile mediante Prompt Builder CLI.
7. **Sincronización Documental**:
   - Actualizar `docs/rules/01-modules/foundations/micro/0032-crash-recovery-policy/task.md` reflejando el estado de pruebas implementadas.

---

## Definición de terminado

La etapa de pruebas de la capacidad **crash recovery policy** se considerará completa cuando:
1. La suite de pruebas unitarias `vigilio_crash_recovery_tests` contemple y apruebe todos los casos definidos en la matriz de pruebas (`TEST-REC-001` a `TEST-REC-016`).
2. Se demuestre que ante centinelas o sesiones corruptas, vacías o malformadas, el sistema nunca aborta ni entra en bucles indefinidos, archivando adecuadamente los datos dañados.
3. Se demuestre el rechazo total de esquemas URL no autorizados (`javascript:`, `data:`, `file:`, etc.) y el truncado estricto a 1000 pestañas.
4. Se verifique la ausencia de condiciones de carrera bajo concurrencia multihilo en el latido periódico.
5. Se certifique la resiliencia ante errores de permisos en disco con retorno `false` seguro.
6. El 100% de los tests en `check.sh` se ejecuten con código de salida 0 en < 1 segundo total.
7. Las validaciones de Prompt Builder (`validate-references`, `verify-lock`) aprueben todas las comprobaciones.

---

## Dependencias y riesgos

- **Dependencias ejecutables**:
  - `module.foundations.task0031`: Completada satisfactoriamente, proveyendo la integración visual y las pruebas de interfaz de usuario.
- **Riesgos identificados y mitigaciones**:
  - *Riesgo*: Flakiness en pruebas de concurrencia multihilo.
    *Mitigación*: Uso de sincronización determinista (`std::thread::join`), marcas de tiempo controladas y verificación de integridad de archivo final sin depender de sleeps artificiales.
  - *Riesgo*: Problemas de portabilidad en pruebas de permisos de disco en plataformas Windows vs POSIX.
    *Mitigación*: Empleo de `std::filesystem::permissions` portable y verificación de rutas deliberadamente no existentes o inaccesibles, con fallback adecuado por plataforma.
  - *Riesgo*: Aumento en el tiempo de ejecución de las pruebas unitarias.
    *Mitigación*: Mantener todas las pruebas basadas en I/O sintético local y operaciones en memoria, garantizando tiempo total de suite < 500 ms.

---

## Commit propuesto

`test(foundations): tests crash-recovery-policy`
