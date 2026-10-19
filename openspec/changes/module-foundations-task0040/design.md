# Design — module-foundations-task0040

Status: PROPOSED

## Current, target and gaps

### Current (Estado actual)
En las etapas previas del módulo `foundations` para la capacidad **test fixture foundation**:
- `task0037` (domain): Definió formalmente las estructuras del dominio (`TestEnvironmentConfig`, `TestEnvironmentState`, `FixtureError`, `TestEnvironmentReport`, `TestFixtureFoundation`, `TestFixtureValidator` en `src/core/`), validando configuraciones, ciclos de vida RAII, cuotas de almacenamiento y rechazo de rutas inseguras con una suite base de 10 pruebas unitarias en `tests/unit/TestFixtureFoundationTests.cpp`.
- `task0038` (implementation): Desarrolló la capa de servicio operativo desacoplada (`TestFixtureService` en `src/core/`), dotándola de registro y ciclo de vida de fixtures gestionados, creación y reversión de instantáneas (`createSnapshot`, `rollbackToSnapshot`), detección y purga segura de carpetas huérfanas (`scanAndPurgeOrphanFixtures`), reintentos con retroceso exponencial (`cleanWithRetry`), sembrado sintético de bases de datos SQLite y perfiles de navegador, y cancelación cooperativa multihilo vía `CancellationToken`, con 12 pruebas unitarias en `tests/unit/TestFixtureServiceTests.cpp`.
- `task0039` (ui): Integró el arnés de fixtures en la suite de pruebas de interfaz de usuario de Qt (`tests/ui/WindowTests.cpp`), erradicando estructuras ad-hoc anteriores (`TempDirFixture`), asegurando la validación estática de rutas seguras con `isSafeTestDirectory`, y verificando las interacciones y escalado accesible de los banners de recuperación ante caídas del navegador.

### Target (Estado objetivo)
- Consolidar una **batería integral, exhaustiva y determinista de pruebas de resiliencia y casos de frontera** para `test fixture foundation` en la etapa `tests`.
- Cubrir de forma metódica:
  1. *Casos normales de extremo a extremo (E2E sintético)*: Flujo completo de ciclo de vida con sembrado de perfil, persistencia SQLite, instantánea, alteración destructiva, rollback y limpieza RAII.
  2. *Casos negativos extremos*: Inyección de secuencias complejas de directory traversal (`../../`, `..\\`), rutas absolutas no autorizadas, insumos nulos o malformados, cuotas rebasadas en frontera exacta (1 byte sobre el límite), y operaciones de instantánea con nombres hostiles o inexistentes.
  3. *Concurrencia masiva*: Estrés de creación y liberación concurrente desde 8 a 16 hilos simultáneos, garantizando unicidad de identificadores (`fixtureId`), ausencia total de carreras de datos (`data races`) y convergencia del conteo de fixtures activos a cero.
  4. *Cancelación cooperativa y política Zero-Zombie*: Interrupción atómica e inmediata de operaciones prolongadas mediante `CancellationToken`, liberación inmediata de descriptores de archivo y eliminación de riesgos de procesos zombi o huérfanos.
  5. *Recuperación ante fallos y resiliencia de E/S*: Simulación de bloqueos de archivo transitorios y validación del retroceso exponencial en `cleanWithRetry`, manejo de subdirectorios de solo lectura y recuperación ordenada tras restaurar permisos.
  6. *Protección estricta de rutas ajenas en el escáner de huérfanos*: Comprobación de que `scanAndPurgeOrphanFixtures` con umbral cero purga exclusivamente directorios que cumplan `isSafeTestDirectory` y pertenezcan al prefijo `vigilio_test_`, sin tocar jamás directorios del sistema o carpetas temporales de otras aplicaciones.
  7. *Desacoplamiento y ejecución headless*: Suite 100% ejecutable en CTest bajo consola (< 100 ms) sin requerir servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`) ni invocar procesos Chromium o WebEngine.
  8. *Auditoría de lenguaje*: 100% de cumplimiento con `docs/UX-COPY-PUBLICO.md`, prohibiendo términos internos de ingeniería en textos observables.

### Gaps (Brechas identificadas a resolver en APPLY)
- Brecha 1: `TestFixtureFoundationTests.cpp` cubre casos de rechazo de rutas estándar, pero carece de pruebas de secuencias de directory traversal complejas multinivel, intentos de inyección con separadores mixtos (`..\\..//`), y pruebas de precisión de cuota en el límite de frontera exacta (escritura de $N$ bytes exitosa vs. $N+1$ bytes rechazada con `QuotaExceeded`).
- Brecha 2: `TestFixtureFoundationTests.cpp` requiere una prueba de robustez para el anidamiento múltiple y restauración de variables de entorno en `TestEnvironmentGuard` ante flujos excepcionales o salidas prematuras de ámbito.
- Brecha 3: `TestFixtureServiceTests.cpp` no cuenta con una prueba integral de extremo a extremo que orqueste en una sola secuencia: creación de fixture gestionado $\to$ sembrado de base de datos SQLite y perfil sintético $\to$ captura de instantánea $\to$ corrupción intencional $\to$ restauración atómica con rollback $\to$ verificación de consistencia $\to$ limpieza completa.
- Brecha 4: `TestFixtureServiceTests.cpp` cuenta con una prueba de concurrencia básica de 4 hilos, pero carece de una prueba de estrés con 16 hilos que ejecute de forma combinada creación, escritura, instantáneas y liberación simultáneas para someter a prueba la sincronización por mutex y la unicidad de IDs.
- Brecha 5: `TestFixtureServiceTests.cpp` debe validar la frontera temporal de `scanAndPurgeOrphanFixtures`, asegurando que carpetas creadas recientemente no sean purgadas cuando el umbral de antigüedad sea superior, y que carpetas no-Vigilio presentes en la carpeta temporal nunca sean alteradas.

---

## Compatibility, migration and rollback

### Compatibilidad hacia atrás y portable
- **C++20 y bibliotecas estándar**: Todas las pruebas y aserciones utilizan estrictamente características estándar de C++20 (`std::filesystem`, `std::chrono`, `std::atomic`, `std::unique_ptr`, `std::string`, `std::vector`) sin depender de extensiones específicas de compilador ni de bibliotecas externas pesadas.
- **Portabilidad multiplataforma**: Compatibilidad total probada en entornos POSIX (Linux, macOS) y Windows, utilizando separadores de ruta genéricos (`std::filesystem::path::preferred_separator`), detección condicional de entorno headless (`DISPLAY` / `WAYLAND_DISPLAY`), y abstracción de variables de entorno (`setenv`/`unsetenv` en POSIX vs. `_putenv_s` en Windows).
- **Compatibilidad con suites existentes**: Las nuevas pruebas se integran de forma natural en los ejecutables existentes `vigilio_fixture_tests` y `vigilio_fixture_service_tests`, sin romper ni modificar las aserciones de las 22 pruebas preexistentes.

### Estrategia de migración
- No se requiere migración de esquemas de datos ni conversión de archivos de producción, dado que la capacidad `test fixture foundation` opera exclusivamente en el ámbito de desarrollo y pruebas automatizadas.
- La incorporación de las nuevas funciones de prueba en `TestFixtureFoundationTests.cpp` y `TestFixtureServiceTests.cpp` se realiza de forma aditiva y determinista, manteniendo la estructura canónica de los archivos.

### Plan de rollback
- En caso de detectar fallos de compilación o inestabilidad en alguna prueba, el rollback es inmediato y seguro:
  1. Revertir las modificaciones en `tests/unit/TestFixtureFoundationTests.cpp` y `tests/unit/TestFixtureServiceTests.cpp` mediante `git checkout`.
  2. Recompilar la suite con `apps/vigilio-browser/scripts/check.sh` y certificar el retorno al estado verde anterior.
  3. No existen artefactos persistentes ni bases de datos de producción afectadas.

---

## Arquitectura de pruebas y matriz de resiliencia

### 1. Desacoplamiento de componentes bajo prueba
La infraestructura de fixtures sigue una arquitectura modular en capas claramente delimitadas:

```
+-------------------------------------------------------------------+
|                        Arnés de Pruebas                           |
|  - TestFixtureFoundationTests.cpp (Pruebas unitarias de dominio)  |
|  - TestFixtureServiceTests.cpp    (Pruebas de servicio y estrés)  |
|  - WindowTests.cpp                (Integración con interfaz Qt)   |
+---------------------------------+---------------------------------+
                                  |
                                  v
+---------------------------------+---------------------------------+
|                       vigilio_core (C++20)                        |
|                                                                   |
|  +------------------------+      +-----------------------------+  |
|  |   TestFixtureService   |----->|    TestFixtureFoundation    |  |
|  | - Registro y lifecycle |      | - Gestión RAII de carpetas  |  |
|  | - Snapshots y rollback |      | - Control de cuota (bytes)  |  |
|  | - Purga de huérfanos   |      | - Limpieza SQLite WAL/SHM   |  |
|  | - Sembrado de perfiles |      | - TestEnvironmentGuard      |  |
|  +------------------------+      +--------------+--------------+  |
|              |                                  |                 |
|              +------------------+---------------+                 |
|                                 |                                 |
|                                 v                                 |
|                  +-----------------------------+                  |
|                  |    TestFixtureValidator     |                  |
|                  | - isSafeTestDirectory       |                  |
|                  | - validateConfig            |                  |
|                  | - fixtureStateToString      |                  |
|                  | - fixtureErrorToString      |                  |
|                  +-----------------------------+                  |
+-------------------------------------------------------------------+
```

- **Aislamiento de `vigilio_core`**: Ninguna clase de fixtures incluye cabeceras de `QWidget`, `QMainWindow` ni `QApplication`.
- **Independencia de Chromium**: Las pruebas no inicializan `QWebEngineView`, perfiles de Chromium ni procesos de renderizado GPU.

### 2. Jerarquía y cobertura de pruebas de fixtures

La suite ampliada para `module.foundations.task0040` se estructura en tres niveles complementarios:

#### Nivel 1: Dominio base y seguridad de rutas (`TestFixtureFoundationTests.cpp`)
- `test_fixture_canonical_config`: Valida la configuración por defecto (50 MB, AlwaysClean, prefijo vigilio_test).
- `test_fixture_lifecycle_raii`: Valida creación, existencia y purga automática RAII.
- `test_fixture_unsafe_path_rejection`: Valida rechazo de rutas del sistema y prefijos no seguros.
- `test_fixture_subdirectories_and_files`: Valida creación de carpetas y archivos con cálculo de bytes.
- `test_fixture_quota_enforcement`: Valida control básico de cuota de almacenamiento.
- `test_fixture_sqlite_cleanup`: Valida purga coordinada de ficheros `.sqlite`, `-wal` y `-shm`.
- `test_fixture_environment_guard`: Valida establecimiento y restauración de variables de entorno.
- `test_fixture_concurrency_isolation`: Valida aislamiento entre 10 instancias secuenciales.
- `test_fixture_permission_resilience`: Valida respuesta ante subdirectorios sin permisos.
- `test_fixture_public_copy_audit`: Valida cadenas en español y ausencia de términos prohibidos.
- **NUEVA `test_fixture_deep_path_traversal_rejection`**: Valida secuencias complejas `a/../../b`, `..\\escape`, rutas absolutas hostiles en creación de subdirectorios y archivos.
- **NUEVA `test_fixture_exact_boundary_quota`**: Valida escritura en frontera exacta (1024 bytes pasan, 1025 bytes fallan con `QuotaExceeded`, estado `Corrupted`).
- **NUEVA `test_fixture_nested_environment_guards`**: Valida anidamiento en tres niveles de `TestEnvironmentGuard`, con mutaciones sucesivas y restauración garantizada en orden inverso.
- **NUEVA `test_fixture_sqlite_cleanup_edge_cases`**: Valida que `cleanupSqliteWalShm` maneje con robustez ficheros inexistentes, rutas relativas y carpetas.

#### Nivel 2: Capa de servicio, instantáneas y concurrencia (`TestFixtureServiceTests.cpp`)
- `test_fixture_service_lifecycle`: Valida registro y búsqueda de fixtures gestionados.
- `test_fixture_service_release_batch`: Valida liberación masiva con `releaseAllFixtures`.
- `test_fixture_service_cancellation`: Valida interrupción básica vía `CancellationToken`.
- `test_fixture_service_orphan_purge`: Valida detección y purga de carpetas simuladas.
- `test_fixture_service_retry_backoff`: Valida ejecución de reintentos con retroceso.
- `test_fixture_service_snapshots`: Valida creación y rollback básico de instantáneas.
- `test_fixture_service_seed_profile`: Valida sembrado de archivos de preferencias y estructura de perfil.
- `test_fixture_service_seed_sqlite`: Valida creación y consulta de base de datos SQLite sintética.
- `test_fixture_service_quota_enforcement`: Valida control de cuota al crear snapshots.
- `test_fixture_service_concurrency`: Valida concurrencia básica de 4 hilos.
- `test_fixture_service_unsafe_path_rejection`: Valida rechazo de rutas maliciosas en snapshots y SQLite.
- `test_fixture_service_public_copy_audit`: Valida textos de diagnóstico en español.
- **NUEVA `test_fixture_service_full_lifecycle_e2e`**: Valida el ciclo completo de vida E2E: registro $\to$ sembrado de SQLite y perfil $\to$ snapshot $\to$ corrupción intencional $\to$ rollback atómico $\to$ verificación de consistencia $\to$ purga limpia.
- **NUEVA `test_fixture_service_invalid_snapshot_and_profile_inputs`**: Valida rollback a snapshots inexistentes (`NotFound`), creación con nombres vacíos o con secuencias traversal (`PathUnsafe`), e intento de rollback con token cancelado.
- **NUEVA `test_fixture_service_massive_concurrency_stress`**: Valida 16 hilos concurrentes realizando operaciones cruzadas simultáneas sin condiciones de carrera y convergiendo a 0 fixtures activos.
- **NUEVA `test_fixture_service_cancellation_token_stress`**: Valida cancelación inmediata y atómica en todas las operaciones del servicio (`seedBrowserProfile`, `scanAndPurgeOrphanFixtures`, `cleanWithRetry`).
- **NUEVA `test_fixture_service_orphan_purge_boundary_conditions`**: Valida que el escáner de huérfanos con umbral de tiempo preserve fixtures recientes y nunca elimine carpetas ajenas en el directorio temporal.

#### Nivel 3: Integración de UI y recuperación ante caídas (`WindowTests.cpp`)
- Valida la orquestación de `TestFixtureFoundation` en la interfaz gráfica para pruebas de recuperación ante caídas, modo incógnito y escalado accesible en Senior Mode, reportando transparentemente `NOT_EXECUTED` ante la ausencia de servidor gráfico.

### 3. Matriz de aserciones y comportamiento determinista

| Vector de prueba | Entrada / Invocación | Estado resultante | Diagnóstico devuelto | Aserción verificada |
| --- | --- | --- | --- | --- |
| Path Traversal profundo | `fixture.createSubdirectory("../evil")` | `Active` (sin corrupción) | Retorna ruta vacía | `CHECK(subPath.empty())` |
| Cuota exacta (frontera) | Escritura de 1024 bytes en cuota de 1024 | `Active` | Retorna ruta válida | `CHECK(!file.empty()) && CHECK(fixture.checkQuota())` |
| Cuota rebasada (+1 byte) | Escritura de 1025 bytes en cuota de 1024 | `Corrupted` | `FixtureError::QuotaExceeded` | `CHECK(file.empty()) && CHECK(fixture.lastError() == FixtureError::QuotaExceeded)` |
| Guardián anidado | Nivel 1: A, Nivel 2: B, Nivel 3: C | Variable restaurada | Valor inicial previo | `CHECK(std::getenv(var) == originalValue)` |
| Rollback snapshot inexistente | `service.rollbackToSnapshot(f, "inexistente")` | Inalterado | `FixtureOperationStatus::NotFound` | `CHECK(res.status == FixtureOperationStatus::NotFound)` |
| Cancelación activa | Invocación con `token.requestCancellation()` | Cancelado | `FixtureOperationStatus::Cancelled` | `CHECK(res.status == FixtureOperationStatus::Cancelled)` |
| Concurrencia (16 hilos) | 16 hilos $\times$ 4 iteraciones de creación/liberación | `0` fixtures activos | Éxito en todas las liberaciones | `CHECK(service.activeFixtureCount() == 0)` |
| Huérfanos ajenos | Carpeta `/tmp/otra_app_123` | Preservada intacta | Escáner no la toca | `CHECK(std::filesystem::exists(otraAppPath))` |
| Textos públicos | `fixtureStateToString`, `operationStatusToString` | Cadenas válidas en español | Cero términos internos | `CHECK(copy.find("Senior Mode") == npos)` |

### 4. Resiliencia y saneamiento garantizado del sistema de archivos
- **Política Zero-Zombie**: Toda prueba de CTest cuenta con timeout finito (15 segundos configurados en CMakeLists.txt) y ejecuta localmente en memoria y almacenamiento temporal sin subprocesos huérfanos.
- **Saneamiento post-prueba**: Los bloques de prueba aseguran la eliminación completa de los directorios creados tanto mediante destructores RAII automáticos como mediante llamadas explícitas de limpieza en bloques `catch` para evitar acumulaciones en `/tmp`.
