# Change Proposal — module-foundations-task0040

Pack: `module.foundations.task0040`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **tests** de la capacidad **test fixture foundation** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0040 — Probar casos positivos, negativos y recuperación: test fixture foundation`).

Habiéndose completado la especificación formal del modelo de dominio e invariantes de seguridad (`task0037`), la implementación del servicio operativo desacoplado con snapshots, sembrado y cancelación cooperativa (`task0038`), y la integración del arnés de fixtures en las pruebas de interfaz gráfica de usuario (`task0039`), el objetivo estricto de este pack es **diseñar y estructurar la suite exhaustiva de pruebas automatizadas, deterministas y de alta resiliencia para test fixture foundation**. La suite verifica casos positivos de ciclo de vida completo, rechazo de entradas inválidas y ataques de directory traversal profundo, concurrencia masiva multihilo sin colisiones, cancelación cooperativa inmediata, recuperación ante fallos de disco o contención de bloqueos transitorios, y regresión cero sin iniciar procesos Chromium ni servidores gráficos.

El alcance comprende:
- **Casos positivos y ciclo de vida integral de fixtures**:
  - Creación y verificación de fixtures aislados bajo `std::filesystem::temp_directory_path() / "vigilio_test_*"` mediante RAII y verificación de limpieza completa tras la destrucción.
  - Creación de subdirectorios multinivel y archivos sintéticos de texto y binarios con cálculo acumulado de bytes consumidos.
  - Generación, mutación destructiva y restauración atómica (`rollbackToSnapshot`) de instantáneas de estado mediante `TestFixtureService`.
  - Sembrado de bases de datos SQLite sintéticas (`seedMockDatabase`) y perfiles de navegador sintéticos (`seedBrowserProfile`) con verificación de integridad de esquema y tablas.
  - Anidamiento y restauración limpia de múltiples variables de entorno mediante `TestEnvironmentGuard`.
- **Casos negativos, entradas malformadas y ataques de path traversal**:
  - Detección y rechazo riguroso de rutas con secuencias de directory traversal (`../`, `../../`, `..\\`) en creación de subdirectorios, archivos e instantáneas.
  - Rechazo categórico de rutas fuera del directorio temporal autorizado (`/`, `/etc`, `/home`, `$TMPDIR` raíz).
  - Rechazo de configuraciones inválidas (`TestEnvironmentConfig` con prefijo vacío, prefijos con barras/separadores de ruta, cuotas de disco nulas o excesivas $> 500$ MB).
  - Verificación estricta de los límites de cuota de almacenamiento (`maxTempDirSizeBytes`): escritura en la frontera exacta, fallo inmediato al intentar superar la cuota por 1 byte, transición del estado a `TestEnvironmentState::Corrupted` con código `FixtureError::QuotaExceeded` y preservación de la integridad del fixture.
  - Operaciones de instantánea inválidas: rollback hacia snapshots inexistentes, creación de snapshots con nombre vacío o con secuencias maliciosas, e intento de crear instantáneas cuando la cuota está agotada.
  - Saneamiento de archivos SQLite (`cleanupSqliteWalShm`) ante archivos inexistentes, nombres con path traversal o carpetas en lugar de archivos de base de datos.
- **Concurrencia masiva, reentrancia y aislamiento entre procesos**:
  - Creación y destrucción paralela de fixtures gestionados desde 8 a 16 hilos concurrentes simultáneos, certificando ausencia total de carreras de datos, sincronización correcta y unicidad absoluta de identificadores (`fixtureId`) y rutas en el sistema de archivos (PID + steady_clock nanosegundos + contador atómico).
  - Aislamiento total en ejecuciones paralelas de CTest (`ctest -jN`), evitando interferencias cruzadas entre suites de pruebas ejecutándose en simultáneo.
- **Cancelación cooperativa y política Zero-Zombie**:
  - Interrupción inmediata y atómica de operaciones prolongadas (`createSnapshot`, `rollbackToSnapshot`, `seedBrowserProfile`, `scanAndPurgeOrphanFixtures`, `cleanWithRetry`) ante la activación de `CancellationToken`.
  - Verificación de idempotencia ante múltiples llamadas consecutivas a `requestCancellation()`.
  - Garantía de ausencia de procesos zombi o huérfanos y liberación inmediata de memoria y descriptores de archivo.
- **Recuperación ante fallos de disco, permisos y contención transitoria**:
  - Simulación de bloqueos de archivo transitorios y validación del mecanismo de reintento con retroceso exponencial (`cleanWithRetry`).
  - Resiliencia ante subdirectorios sin permisos de escritura/ejecución: detección adecuada del fallo de limpieza (`FixtureError::CleanupFailed`), transición al estado `Corrupted`, y recuperación limpia una vez restaurados los permisos.
  - Escáner y purga segura de fixtures huérfanos (`scanAndPurgeOrphanFixtures`) con límites temporales de antigüedad, certificando que nunca se alteren carpetas ajenas a `vigilio_test_*` ni se afecten archivos del sistema.
- **Desacoplamiento arquitectónico y ejecución sin Chromium**:
  - Certificación de ejecución 100% en CTest headless (< 100 ms) sin requerir `DISPLAY`, `WAYLAND_DISPLAY`, `QApplication` ni procesos WebEngine/GPU.
  - Aislamiento absoluto y política local-first constitucional (ADR-001 / Decisión Q-001): cero sockets de red externa, cero dependencias cloud y cero telemetría.
- **Auditoría de textos públicos y terminología**:
  - Verificación sistemática de ausencia de términos técnicos prohibidos de ingeniería («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain») en todas las funciones de diagnóstico y conversión de estado de fixtures (`fixtureStateToString`, `fixtureErrorToString`, `operationStatusToString`), garantizando cumplimiento estricto de `docs/UX-COPY-PUBLICO.md`.

---

## Out of scope

- Modificación de la interfaz gráfica pública del navegador (`BrowserWindow`) o adición de controles visuales de fixtures (prohibido taxativamente por `docs/UX-COPY-PUBLICO.md`; test fixture foundation es infraestructura interna de pruebas y se mantiene formalmente bajo el contrato de «sin cambio visual directo»).
- Modificación de contratos de negocio existentes o tipos de datos en `TestFixtureTypes.h`, `TestFixtureFoundation.h`, `TestFixtureValidator.h` o `TestFixtureService.h` (salvo que una prueba identifique una discrepancia formal con la especificación).
- Invocación de procesos reales de Qt WebEngine o Chromium durante la ejecución de las pruebas de test fixture foundation.
- Integración con servicios de nube, autenticación remota o telemetría (estrictamente denegado por ADR-001 / Decisión Q-001).
- Modificación de código fuente fuera de las rutas autorizadas para esta etapa.

---

## Contexto y evidencia

Se inspeccionó de forma exhaustiva la base de código y los artefactos existentes en `apps/vigilio-browser`:

1. **Dominio desacoplado (`apps/vigilio-browser/src/core/`)**:
   - `TestFixtureTypes.h`: Define formalmente las enumeraciones fuertemente tipadas `TestEnvironmentState` (`Uninitialized`, `Configured`, `Active`, `TornDown`, `Corrupted`), `FixtureCleanPolicy` (`AlwaysClean`, `RetainOnFailure`, `NeverClean`), `FixtureError` (`None`, `DirectoryCreationFailed`, `CleanupFailed`, `QuotaExceeded`, `EnvironmentOverrideFailed`, `InvalidConfiguration`, `DirectoryLocked`, `UnsafePathDetected`), `FixtureOperationStatus` (`Success`, `Cancelled`, `QuotaExceeded`, `PathUnsafe`, `IOError`, `LockContention`, `NotFound`, `Timeout`), token ligero de cancelación (`CancellationToken`), opciones configurables (`TestEnvironmentConfig`, `BrowserProfileSeedOptions`) y estructuras inmutables de resultados (`TestEnvironmentReport`, `FixtureOperationResult`, `OrphanPurgeReport`).
   - `TestFixtureValidator.h` / `.cpp`: Validador estático libre de dependencias gráficas, encargado de comprobar la seguridad de rutas temporales (`isSafeTestDirectory`), validar configuraciones de entorno (`validateConfig`), sanear nombres de archivos y proporcionar descripciones en español neutral conforme a `docs/UX-COPY-PUBLICO.md` (`fixtureStateToString`, `fixtureErrorToString`).
   - `TestFixtureFoundation.h` / `.cpp`: Primitiva RAII base que administra el ciclo de vida del directorio temporal (`rootPath`), creación segura de subdirectorios y archivos sintéticos, cálculo y verificación de cuota de almacenamiento, saneamiento de ficheros SQLite WAL/SHM, y protección de variables de entorno mediante `TestEnvironmentGuard`.
   - `TestFixtureService.h` / `.cpp`: Capa operativa de servicio que administra el registro de fixtures gestionados (`createManagedFixture`, `releaseFixture`, `releaseAllFixtures`), creación y reversión de instantáneas (`createSnapshot`, `rollbackToSnapshot`, `removeSnapshot`), purga segura de fixtures huérfanos (`scanAndPurgeOrphanFixtures`), reintentos con retroceso exponencial (`cleanWithRetry`), sembrado sintético de bases de datos (`seedMockDatabase`) y perfiles de navegador (`seedBrowserProfile`).
2. **Arnés de pruebas actual (`apps/vigilio-browser/tests/`)**:
   - `tests/unit/TestFixtureFoundationTests.cpp`: Contiene 10 pruebas unitarias centradas en la configuración canónica, ciclo de vida RAII, rechazo de rutas inseguras, subdirectorios y archivos, control de cuota, limpieza SQLite, guardián de variables de entorno, aislamiento de concurrencia, resiliencia de permisos y auditoría de textos públicos.
   - `tests/unit/TestFixtureServiceTests.cpp`: Contiene 12 pruebas de servicio centradas en registro y ciclo de vida, liberación en lote, cancelación cooperativa, purga de huérfanos, reintentos con retroceso exponencial, instantáneas y rollback, sembrado de perfiles, sembrado de SQLite, control de cuota en servicio, concurrencia multihilo, rechazo de rutas inseguras y auditoría de textos públicos.
   - `tests/ui/WindowTests.cpp`: Arnés de pruebas de interfaz de usuario de Qt que utiliza `TestFixtureFoundation` y `TestFixtureService` para validar los escenarios de recuperación ante caídas del navegador (terminación anormal, salida limpia, aislamiento en modo incógnito, bucle de caídas y descarte de aviso), verificando rutas seguras con `TestFixtureValidator::isSafeTestDirectory`.
3. **Pipeline de compilación y verificación (`apps/vigilio-browser/CMakeLists.txt` y `scripts/check.sh`)**:
   - `CMakeLists.txt` declara los ejecutables de prueba `vigilio_fixture_tests` y `vigilio_fixture_service_tests` con labels `unit;fast` y timeout de 15 segundos.
   - `scripts/check.sh` compila bajo C++20 con `-Werror`, ejecuta CTest con filtro `-L fast` y valida Clang-Format sobre la base de código.

---

## Alcance exacto

La microtarea `0040` (etapa `tests`) tiene como alcance exacto consolidar y expandir la suite de pruebas unitarias y de integración de `test fixture foundation` mediante tests focalizados en:

1. **Casos positivos de extremo a extremo**:
   - Orquestación completa de un ciclo de pruebas complejo: inicialización de fixture gestionado $\to$ creación de jerarquía profunda de subdirectorios $\to$ sembrado de perfil sintético y base de datos SQLite con datos simulados $\to$ captura de snapshot $\to$ mutación corrupta intencional $\to$ rollback atómico a snapshot $\to$ verificación de consistencia de datos $\to$ limpieza SQLite WAL/SHM $\to$ purga RAII con cero residuos en disco.
   - Verificación de anidamiento múltiple y preservación de estados en `TestEnvironmentGuard` ante excepciones simuladas o salidas prematuras de ámbito.
2. **Casos negativos de frontera y ataques de evasión**:
   - Inyección de secuencias complejas de directory traversal: `createSubdirectory("a/../../b")`, `createTestFile("../../../etc/shadow", ...)`, `createTestBinaryFile("..\\escape", ...)`.
   - Inyección de nombres nulos, rutas compuestas solo de separadores (`///`) y nombres con caracteres de control.
   - Pruebas de frontera exacta en cuota de almacenamiento: configuración de cuota mínima permitida (1024 bytes), escritura de 1024 bytes (PASS), intento de escritura adicional de 1 solo byte (FAIL con `QuotaExceeded`), validación de que el estado transiciona a `Corrupted` y que `checkQuota()` retorna `false`.
   - Pruebas de rechazo en operaciones de instantánea con nombres hostiles o inexistentes.
   - Comportamiento ante intentos de saneamiento de bases de datos SQLite con nombres inexistentes o que apuntan a directorios.
3. **Concurrencia multihilo y estrés**:
   - Ejecución simultánea de 8 a 16 hilos realizando operaciones de creación de fixtures, escritura de archivos, toma de instantáneas, rollback y liberación, verificando que no existan colisiones de rutas ni identificadores repetidos, y que el contador de fixtures activos de `TestFixtureService` converja limpiamente a cero.
4. **Cancelación cooperativa**:
   - Activación de cancelación pre-operación y concurrente durante operaciones de sembrado, purga de huérfanos y reintento con retroceso exponencial, verificando que retornan inmediatamente con estado `Cancelled` y no dejan archivos residuales bloqueados.
5. **Recuperación ante fallos e inyección de errores**:
   - Simulación de fallos de E/S y contención de bloqueos temporales en `cleanWithRetry`, verificando que el retroceso exponencial respeta los reintentos configurados y reporta `LockContention` o `IOError` adecuadamente.
   - Pruebas de recuperación de permisos: simulación de fallo de limpieza sobre subdirectorios de solo lectura, verificación de código de error `CleanupFailed`, restauración de permisos y confirmación de limpieza exitosa posterior.
6. **Escáner de huérfanos**:
   - Pruebas de frontera en `scanAndPurgeOrphanFixtures`: umbral de antigüedad 0 segundos (purga inmediata de huérfanos seguros), umbral grande (preservación de fixtures recientes), y garantía de que directorios ajenos al prefijo `vigilio_test_` en el directorio temporal nunca son modificados ni eliminados.
7. **Auditoría de cumplimiento de textos públicos**:
   - Auditoría exhaustiva mediante inspección algorítmica de todas las salidas de cadenas de texto generadas por `TestFixtureValidator` y `TestFixtureService`, asegurando el cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md` en español y cero términos técnicos de ingeniería interna.

---

## Diseño completo de la capacidad

El diseño de pruebas para **test fixture foundation** se fundamenta en los siguientes pilares de arquitectura de software y calidad de código:

### 1. Desacoplamiento estricto y ejecución sin servidor gráfico
La infraestructura de fixtures (`TestFixtureFoundation`, `TestFixtureValidator`, `TestFixtureService`, `TestEnvironmentGuard`) reside exclusivamente en `src/core`, compilada como parte de la biblioteca estática `vigilio_core`. No posee dependencias hacia `QWidget`, `QApplication` ni módulos de renderizado gráfico de Chromium o WebEngine. Por consiguiente, la suite de pruebas ejecuta íntegramente en consola / CTest en menos de 100 ms, sin requerir servidor gráfico ni variables `DISPLAY` o `WAYLAND_DISPLAY`, garantizando ejecución transparente en cualquier entorno de integración continua.

### 2. Gestión de recursos mediante RAII y contención de rutas
Todo directorio temporal creado por `TestFixtureFoundation` o gestionado por `TestFixtureService` se ubica de forma obligatoria bajo `std::filesystem::temp_directory_path()` y sigue el patrón canónico de nomenclatura:
$$\text{prefix} + \text{"\_"} + \text{PID} + \text{"\_"} + \text{nanoseconds} + \text{"\_"} + \text{atomic\_counter}$$
El destructor de `TestFixtureFoundation` ejecuta automáticamente la limpieza de la carpeta temporal conforme a la política configurada (`FixtureCleanPolicy::AlwaysClean`), asegurando que ningún artefacto sintético quede como residuo en el host del desarrollador o del sistema de CI.

### 3. Matriz de validación estática de seguridad (`TestFixtureValidator`)
Antes de crear carpetas o archivos, o al escanear huérfanos, el validador estático verifica que:
1. La ruta normalizada resida de forma estricta dentro de `std::filesystem::temp_directory_path()`.
2. La ruta no coincida con el directorio temporal raíz del sistema (`/tmp`).
3. El nombre base de la carpeta comience estrictamente con el prefijo autorizado (`vigilio_test_`).
4. La ruta normalizada no contenga componentes relativos de ascenso (`..`).
Cualquier violación a estas condiciones aborta la operación inmediatamente con el código de error `FixtureError::UnsafePathDetected` o `FixtureOperationStatus::PathUnsafe`.

### 4. Cancelación cooperativa atómica y control de concurrencia
La infraestructura utiliza `CancellationToken` (basado en `std::atomic<bool>`) y mutex de sincronización fina (`std::mutex`, `std::lock_guard`) en `TestFixtureService` para proteger el registro de fixtures activos y evitar condiciones de carrera en entornos multihilo. Las operaciones prolongadas consultan periódicamente `token->isCancelled()`, deteniendo de forma segura la ejecución ante cancelaciones del usuario o del arnés de pruebas.

---

## Archivos afectados

| Archivo | Rol | Operación prevista en APPLY |
| --- | --- | --- |
| `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp` | Arnés de pruebas unitarias del dominio base | Ampliación: Incorporar pruebas de casos negativos extremos, path traversal profundo, precisión de cuota en frontera, anidamiento de `TestEnvironmentGuard` y recuperación ante denegación de permisos. |
| `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp` | Arnés de pruebas de la capa de servicio | Ampliación: Incorporar pruebas de ciclo de vida integral, rollback de instantáneas, concurrencia masiva (8-16 hilos), estrés de cancelación atómica, recuperación ante bloqueos transitorios y límites de frontera en escáner de huérfanos. |
| `docs/rules/01-modules/foundations/micro/0040-test-fixture-foundation/task.md` | Documento canónico de la microtarea | Modificación: Sincronizar estado a `COMPLETED` tras la ejecución exitosa de la etapa `tests`. |
| `openspec/changes/module-foundations-task0040/proposal.md` | Propuesta de cambio OpenSpec | Creación: Especificación completa y detallada de la propuesta. |
| `openspec/changes/module-foundations-task0040/design.md` | Diseño técnico OpenSpec | Creación: Arquitectura de pruebas, invariantes y diseño de resiliencia. |
| `openspec/changes/module-foundations-task0040/tasks.md` | Tareas atómicas OpenSpec | Creación: Plan determinista y verificable de tareas de implementación. |
| `openspec/changes/module-foundations-task0040/execution-memory.md` | Memoria de ejecución OpenSpec | Creación: Registro de evidencia, decisiones, skills y handoff. |

---

## Contrato UX/UI

- **Declaración formal de «sin cambio visual directo»**:
  - `test fixture foundation` es una capacidad de infraestructura de pruebas y soporte de desarrollo; por consiguiente, **no introduce menús, botones, widgets ni modificaciones en la interfaz pública de usuario del navegador**.
  - La interfaz gráfica de usuario (`BrowserWindow`) permanece completamente desacoplada de la infraestructura de fixtures de prueba.
- **Cumplimiento estricto de textos públicos (`docs/UX-COPY-PUBLICO.md`)**:
  - Todas las cadenas de diagnóstico y mensajes de error devueltos por `TestFixtureValidator::fixtureStateToString`, `TestFixtureValidator::fixtureErrorToString` y `TestFixtureService::operationStatusToString` están redactadas en español neutro, claro y comprensible.
  - Prohibición absoluta de exhibir términos de ingeniería interna («Senior Mode», «Modo Senior», «Prompt Builder», «DAG», «OpenSpec», «toolchain»).
  - En cualquier superficie observable donde se haga referencia a la modalidad de interfaz accesible, se emplea exclusivamente la denominación oficial aprobada: **«Interfaz más fácil de usar»**.

---

## Reglas y fallos

| Escenario de fallo | Causa raíz | Comportamiento esperado y mitigación |
| --- | --- | --- |
| Intento de path traversal (`../../etc`) | Parámetro malicioso o malformado en subdirectorios o archivos | Rechazo estricto por `TestFixtureValidator::isSafeTestDirectory`; retorna ruta vacía o `FixtureOperationStatus::PathUnsafe`; el estado del fixture no se corrompe. |
| Agotamiento de cuota de disco | La suma de bytes escritos supera `maxTempDirSizeBytes` | La operación de escritura se aborta inmediatamente; `lastError()` se establece en `FixtureError::QuotaExceeded`; el estado transiciona a `TestEnvironmentState::Corrupted`; no se escriben datos parciales no autorizados. |
| Cancelación durante snapshot o rollback | `CancellationToken::requestCancellation()` invocado durante la operación | Detección atómica en el bucle de copia; aborto inmediato; la operación retorna `FixtureOperationStatus::Cancelled`; los recursos temporales parciales son limpiados. |
| Contención de bloqueo en limpieza | Fichero retenido temporalmente por otro proceso o hilo | `cleanWithRetry` ejecuta retroceso exponencial (backoff de 50 ms a 200 ms); si el bloqueo cede, la limpieza culmina con éxito; si persiste, retorna `FixtureOperationStatus::LockContention` de forma limpia sin arrojar excepciones. |
| Subdirectorio de solo lectura en limpieza | Permisos de sistema de archivos insuficientes (`owner_read` sin escritura) | `clean()` detecta el fallo; establece `lastError()` en `FixtureError::CleanupFailed` y estado en `Corrupted`; al restaurar permisos válidos, la limpieza subsiguiente concluye exitosamente. |
| Concurrencia masiva en creación de fixtures | Múltiples hilos solicitando fixtures simultáneamente | Generación determinista combinando PID + timestamp steady_clock + contador atómico; sincronización mediante `std::mutex` en el servicio; cero colisiones de ID o de directorio. |
| Rollback a snapshot inexistente | Identificador de snapshot no registrado en el subdirectorio `_snapshots` | Retorna `FixtureOperationStatus::NotFound` con mensaje descriptivo en español sin modificar el estado actual del fixture. |

---

## Seguridad y privacidad

- **Local-First Constitucional (ADR-001 / Decisión Q-001)**:
  - Cero dependencias de servicios externos, servidores en la nube o telemetría en los arneses de fixtures.
  - Todas las operaciones de sembrado y validación de archivos se ejecutan estrictamente en el almacenamiento temporal local del host (`temp_directory_path()`).
- **Higiene de Rutas y Prevención de Directory Traversal**:
  - Toda ruta de fixture es validada estáticamente y en tiempo de ejecución contra secuencias de escape de directorio (`..`).
  - Prohibición absoluta de manipular rutas del sistema o del usuario real (`~/.local/share/vigilio` o `%APPDATA%`).
- **Aislamiento de Sesiones y Datos Sensibles**:
  - Los datos sembrados para pruebas de recuperación ante caídas o perfiles sintéticos son puramente ficticios y locales.
  - Ninguna URL real o dato sensible de navegación se registra en los archivos de sesión o sentinelas de prueba.
- **Protección de la partición temporal del host**:
  - Límite estricto de cuota de almacenamiento por fixture (50 MB por defecto) para prevenir ataques de denegación de servicio local o saturación de disco durante la ejecución de pruebas automatizadas.

---

## Matriz de pruebas

| ID Prueba | Tipo | Fixture / Arnés | Resultado Esperado | Manejo de Fallo Negativo |
| --- | --- | --- | --- | --- |
| `TEST-FIXTURE-UNIT-011` | Caso Negativo | `TestFixtureFoundationTests` (path traversal profundo) | Rechazo categórico de secuencias `../`, `..\\`, rutas absolutas ajenas y componentes nulos. | Falla si alguna ruta no segura es aceptada o creada. |
| `TEST-FIXTURE-UNIT-012` | Cuota Frontera | `TestFixtureFoundationTests` (exact boundary quota) | Escritura exacta a 1024 bytes tiene éxito; escritura de 1025 bytes falla con `QuotaExceeded` y estado `Corrupted`. | Falla si permite sobrepasar la cuota o no registra el estado de error. |
| `TEST-FIXTURE-UNIT-013` | Guardián Entorno | `TestFixtureFoundationTests` (nested environment guard) | Múltiples instancias anidadas de `TestEnvironmentGuard` restauran con fidelidad los valores originales de las variables. | Falla si una variable queda alterada tras la destrucción del guardián. |
| `TEST-FIXTURE-UNIT-014` | Resiliencia Permisos | `TestFixtureFoundationTests` (read-only subtree cleanup) | Detección limpia de fallo de permisos con `CleanupFailed`; recuperación y limpieza completa al restaurar permisos. | Falla si arroja excepción no controlada o deja carpetas bloqueadas. |
| `TEST-FIXTURE-UNIT-015` | Saneamiento SQLite | `TestFixtureFoundationTests` (corrupted wal/shm cleanup) | `cleanupSqliteWalShm` maneja de forma segura ficheros inexistentes o rutas anómalas retornando falso limpiamente. | Falla si se produce violación de segmento o bloqueo indefinido. |
| `TEST-FIXTURE-SRV-013` | Integración E2E | `TestFixtureServiceTests` (full lifecycle e2e) | Ciclo completo: creación $\to$ sembrado $\to$ snapshot $\to$ mutación corrupta $\to$ rollback atómico $\to$ verificación $\to$ purga limpia. | Falla si los datos restaurados no coinciden con el snapshot original. |
| `TEST-FIXTURE-SRV-014` | Insumos Inválidos | `TestFixtureServiceTests` (invalid snapshots & profiles) | Rollback a snapshot inexistente retorna `NotFound`; nombres de snapshot con traversal retornan `PathUnsafe`. | Falla si se permite operar sobre nombres no sanitizados. |
| `TEST-FIXTURE-SRV-015` | Concurrencia Masiva | `TestFixtureServiceTests` (high-concurrency 16 threads) | 16 hilos creando, modificando y liberando fixtures simultáneamente sin colisiones de ID ni condiciones de carrera. | Falla si se detectan colisiones de ID o el contador activo no converge a 0. |
| `TEST-FIXTURE-SRV-016` | Cancelación Estrés | `TestFixtureServiceTests` (cancellation token stress) | Cancelación previa y concurrente sobre `cleanWithRetry` y `seedBrowserProfile` retorna `Cancelled` de inmediato. | Falla si la operación ignora el token o queda en ejecución infinita. |
| `TEST-FIXTURE-SRV-017` | Reintentos Backoff | `TestFixtureServiceTests` (exponential backoff retry) | `cleanWithRetry` ejecuta reintentos con retraso ante contención transitoria y reporta el diagnóstico con precisión. | Falla si no respeta el número máximo de reintentos o el backoff. |
| `TEST-FIXTURE-SRV-018` | Huérfanos Frontera | `TestFixtureServiceTests` (orphan purge edge cases) | Purga con umbral 0 purga fixtures de prueba pero no altera carpetas ajenas ni temporales del sistema. | Falla si elimina directorios fuera de `vigilio_test_*` o falla al purgar. |
| `TEST-FIXTURE-SRV-019` | Auditoría Copy | `TestFixtureServiceTests` (public copy compliance) | 100% de cadenas de diagnóstico auditadas sin términos prohibidos de ingeniería («Senior Mode», «Prompt Builder», etc.). | Falla si alguna cadena expone términos técnicos no autorizados. |
| `TEST-FIXTURE-REG-020` | Regresión CTest | Suite completa CTest (`scripts/check.sh`) | 15/15 suites de prueba pasando exitosamente en menos de 1 segundo (100% pass, exit code 0). | Falla si alguna suite de prueba previa sufre regresión. |

---

## Pasos APPLY

Cuando esta propuesta sea aprobada para la fase APPLY, se ejecutarán deterministamente los siguientes pasos:

1. **Ampliar `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp` con pruebas focalizadas de casos negativos y frontera**:
   - Incorporar `test_fixture_deep_path_traversal_rejection` para evaluar secuencias avanzadas de escape de directorios (`../../`, `..\\`), rutas absolutas no autorizadas y nombres con caracteres de control.
   - Incorporar `test_fixture_exact_boundary_quota` para validar el comportamiento en el límite exacto de la cuota de almacenamiento y la transición al estado `Corrupted` con `FixtureError::QuotaExceeded`.
   - Incorporar `test_fixture_nested_environment_guards` para validar el anidamiento múltiple y la restauración garantizada de variables de entorno ante excepciones y retornos anticipados.
   - Incorporar `test_fixture_sqlite_cleanup_edge_cases` para comprobar la resiliencia de `cleanupSqliteWalShm` ante ficheros inexistentes o malformados.
2. **Ampliar `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp` con pruebas focalizadas de resiliencia, concurrencia y recuperación**:
   - Incorporar `test_fixture_service_full_lifecycle_e2e` para validar la secuencia completa de creación de fixture, sembrado, snapshot, mutación corrupta, rollback atómico y purga sin residuos.
   - Incorporar `test_fixture_service_invalid_snapshot_and_profile_inputs` para verificar el rechazo de operaciones con instantáneas inexistentes o rutas no seguras.
   - Incorporar `test_fixture_service_massive_concurrency_stress` para ejecutar pruebas con 16 hilos simultáneos validando la unicidad de IDs y ausencia de carreras de datos.
   - Incorporar `test_fixture_service_cancellation_token_stress` para validar la respuesta inmediata y atómica ante cancelaciones en todas las operaciones del servicio.
   - Incorporar `test_fixture_service_orphan_purge_boundary_conditions` para validar los límites temporales del escáner de huérfanos y la protección incondicional de rutas ajenas.
3. **Auditar el cumplimiento de textos públicos (`docs/UX-COPY-PUBLICO.md`) en todas las funciones de diagnóstico**:
   - Revalidar mediante pruebas unitarias que todas las cadenas de texto expuestas cumplan con la guía de estilo en español y no contengan términos de ingeniería interna.
4. **Ejecutar el pipeline de compilación limpia y pruebas continuas**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh`, validando compilación limpia bajo C++20 con `-Werror`, cero advertencias y 100% de tests en CTest aprobados.
5. **Verificar gobernanza y sincronización de documentación**:
   - Ejecutar `validate-references` y `verify-lock` con Prompt Builder CLI.
   - Actualizar el estado de la microtarea `0040` en `docs/rules/01-modules/foundations/micro/0040-test-fixture-foundation/task.md` a `COMPLETED`.

---

## Definición de terminado

La capacidad se considerará completa en la fase APPLY cuando:
1. `apps/vigilio-browser/tests/unit/TestFixtureFoundationTests.cpp` incluya las nuevas pruebas focalizadas de casos negativos, límites de cuota, anidamiento de guardián de entorno y resiliencia de SQLite.
2. `apps/vigilio-browser/tests/unit/TestFixtureServiceTests.cpp` incluya las nuevas pruebas focalizadas de ciclo de vida completo E2E, insumos inválidos, concurrencia masiva (16 hilos), estrés de cancelación y límites de huérfanos.
3. La suite completa de pruebas ejecute en CTest en menos de 1 segundo sin requerir servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`) ni iniciar procesos WebEngine o Chromium.
4. No exista ninguna regresión en las pruebas preexistentes del navegador (15/15 suites en PASS).
5. Se verifique la ausencia total de términos técnicos prohibidos en las cadenas públicas de diagnóstico conforme a `docs/UX-COPY-PUBLICO.md`.
6. El pipeline `apps/vigilio-browser/scripts/check.sh` complete con exit code 0.
7. Se verifique la gobernanza documental mediante Prompt Builder CLI y se actualice `task.md`.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0039`: Integración previa del arnés de fixtures en las pruebas de UI (`WindowTests.cpp`).
  - `module.foundations.task0038`: Implementación del servicio operativo `TestFixtureService`.
  - `module.foundations.task0037`: Especificación del modelo de dominio `TestFixtureFoundation`.
  - CMake 3.24+ y Ninja / Make: Entorno de generación y construcción de perfiles.
  - Compilador C++20 (`gcc` / `clang`) con soporte completo de `std::filesystem`.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Condiciones de carrera o colisión de nombres de carpetas en ejecuciones multihilo de CTest.
    - *Mitigación*: Generación determinista combinando PID + timestamp steady_clock + contador atómico, garantizando unicidad y aislamiento total en `ctest -jN`.
  - *Riesgo*: Fuga de carpetas temporales en `/tmp` tras ejecuciones abortadas o fallos inyectados.
    - *Mitigación*: Gestión RAII limpia en destructores y mecanismo de escaneo y purga de huérfanos (`scanAndPurgeOrphanFixtures`).
  - *Riesgo*: Saturación de memoria o almacenamiento en pruebas de estrés.
    - *Mitigación*: Presupuesto estricto de cuota (50 MB) y limpieza inmediata de instantáneas y perfiles temporales al finalizar cada caso de prueba.
  - *Riesgo*: Dependencia accidental de servidor gráfico o Chromium.
    - *Mitigación*: Aislamiento de las pruebas en `vigilio_core`, sin enlace a `Qt6::Widgets` ni `Qt6::WebEngineWidgets`.

---

## Commit propuesto

- **Mensaje sugerido**: `test(foundations): tests test-fixture-foundation`
- **Alcance**: Batería exhaustiva de pruebas unitarias y de integración para casos normales, negativos, concurrencia, cancelación y recuperación en test fixture foundation.
