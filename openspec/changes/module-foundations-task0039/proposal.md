# Change Proposal — module-foundations-task0039

Pack: `module.foundations.task0039`

Status: PROPOSED

## Outcome and scope

Este cambio formaliza la propuesta técnica integral para la etapa **ui** de la capacidad **test fixture foundation** en el módulo propietario `foundations` de Vigilio Browser (microtarea canónica `0039 — Implementar la interacción y estados visuales en Qt: test fixture foundation`).

Habiéndose completado y certificado en las etapas precedentes la especificación del modelo de dominio, taxonomías, validación estática de rutas temporales seguras y arnés RAII básico (`module.foundations.task0037`), así como la implementación física de la capa operativa de servicio desacoplada (`vigilio::TestFixtureService`) con cancelación cooperativa multihilo (`CancellationToken`), purga segura de fixtures huérfanos, reintentos con espera progresiva ante bloqueos transitorios de archivos, snapshots y rollback deterministas, y sembrado sintético de perfiles de navegador y bases de datos SQLite en C++20 (`module.foundations.task0038`), esta propuesta proyecta la **gobernanza de interacción en Qt Widgets, contratos visuales, tolerancia headless, arnés de pruebas de interfaz de usuario (`vigilio_ui_tests` / `WindowTests.cpp`) y verificación de estados visuales para test fixture foundation**.

El objetivo central de este pack es garantizar que la infraestructura de fixtures proporcione aislamiento, seguridad y aprovisionamiento determinista de estados para las suites de prueba de interfaz gráfica de usuario en Qt Widgets, manteniendo la estricta política de lenguaje público de producto `docs/UX-COPY-PUBLICO.md`, **sin acoplar lógica de pruebas a la ventana principal de navegación (`BrowserWindow`), sin introducir elementos de depuración en la interfaz pública del usuario final, y asegurando tolerancia completa a entornos sin servidor gráfico (Zero-Zombie)**.

El alcance concreto comprende:
- **Declaración contractual de UX/UI y «sin cambio visual directo en la interfaz pública»**:
  - Declaración formal vinculante de que la capacidad **test fixture foundation** es un subsistema de infraestructura de desarrollo, aseguramiento de la calidad y arneses de pruebas automatizadas; por consiguiente, **no introduce widgets, botones de herramientas, menús de depuración, diálogos ni etiquetas técnicas de fixtures en la interfaz visible de navegación de producción (`BrowserWindow`)**.
  - Cumplimiento riguroso de `docs/UX-COPY-PUBLICO.md`: proscripción absoluta de exponer términos de ingeniería interna como «Test Fixture», «Fixture Foundation», «Orphan Purge», «Snapshots», «Rollback», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» en menús principales, barras de herramientas o ventanas visibles al usuario común.
  - Verificación contractual de que todos los controles de `apps/vigilio-browser/src/ui/BrowserWindow.cpp` (pestañas, barra de navegación, barra de direcciones, menús, accesibilidad y páginas de inicio/error) respeten rigurosamente las directrices de diseño de `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y las referencias visuales oficiales de `/resultado/01-vigilio-home.png` y `/resultado/03-vigilio-settings-senior-mode.png`.
- **Integración de `TestFixtureFoundation` y `TestFixtureService` en el arnés de pruebas de interfaz gráfica (`WindowTests.cpp`)**:
  - Sustitución planificada de la estructura local ad-hoc `TempDirFixture` en `apps/vigilio-browser/tests/ui/WindowTests.cpp` por las primitivas canónicas de `vigilio_core` (`vigilio::TestFixtureFoundation` y `vigilio::TestFixtureService`):
    - Confinamiento garantizado de rutas temporales mediante `TestFixtureValidator::isSafeTestDirectory`, evitando cualquier riesgo de eliminación involuntaria de directorios del sistema o del árbol del repositorio.
    - Generación atómica y unívoca de nombres de directorio (`vigilio_test_<scope>_<pid>_<timestamp>_<counter>`) que garantiza aislamiento absoluto en ejecuciones concurrentes y paralelas (`ctest -jN`).
    - Aprovechamiento del sembrado sintético de perfil (`seedBrowserProfile`) y de bases de datos SQLite (`seedMockDatabase`) para aprovisionar los escenarios de prueba visuales de `WindowTests` (como recuperación ante caídas, sesiones previas, sentinelas de crash loop y pestañas restaurables) sin duplicar código de escritura de archivos.
    - Utilización de snapshots y rollback (`createSnapshot`, `rollbackToSnapshot`) para evaluar transiciones de estado visual en la ventana (conmutación de accesibilidad, visualización y descarte de banners) restaurando el estado prístino entre bloques de aserción sin recrear carpetas desde cero.
- **Auditoría y preservación de contratos visuales en `BrowserWindow` bajo entornos de prueba**:
  - Verificación integral de los estados interactivos en los componentes visuales:
    - `idle`: presentación limpia y sobria según `/resultado/01-vigilio-home.png`.
    - `hover`: realce sutil en botones y pestañas sin efectos estroboscópicos ni brillos artificiales.
    - `focus`: contorno visible accesible de 2 px para navegación por teclado.
    - `disabled`: atenuación visual accesible en acciones sin historial previo o siguiente.
    - `loading`: indicador en pestaña («Cargando…») y en barra de estado («Cargando…»).
    - `empty`: página de inicio nativa (`newHomePage` / widget `vigilio-home`) con logo vectorial `branding/vigilio-logo.svg`, título y buscador centralizado, sin inicializar instancias innecesarias de `QWebEngineView`.
    - `error`: contenedor nativo accesible (`vigilio-page-error`) con icono Lucide de advertencia/escudo, título «No se pudo cargar la página», mensaje descriptivo y botones de acción primaria («Reintentar») y secundaria («Ir al inicio»).
- **Preservación y escalado dinámico para «Interfaz más fácil de usar» (Modo Senior)**:
  - Verificación de la adaptación visual accesible en todos los componentes de la ventana:
    - Escalado dinámico de iconos de barra de herramientas (de 22×22 px a 32×32 px).
    - Dimensiones mínimas de toque y clic (tap targets $\ge 48$ px en botones `QPushButton` y campos `QLineEdit`; barra de herramientas $\ge 55$ px; banner de recuperación $\ge 56$ px).
    - Aumento tipográfico (+4 pt) manteniendo legibilidad sin solapamientos ni truncamientos de texto.
  - Mapeo universal de atajos de teclado accesibles: `Ctrl+T`, `Ctrl+W`, `Ctrl+L`, `Ctrl+D`, `Ctrl+B`, `Ctrl+Q`, `Alt+Home`, `Alt+F`.
  - Atributos completos `accessibleName` y `accessibleDescription` en lenguaje natural en español neutro para lectores de pantalla (Orca, NVDA, Narrator).
- **Tolerancia a entornos headless y garantía Zero-Zombie**:
  - Detección determinista en tiempo de ejecución de la presencia de servidor gráfico (`DISPLAY` / `WAYLAND_DISPLAY` en Linux), reportando con transparencia y rigor técnico `NOT_EXECUTED` en entornos de integración continua headless en lugar de provocar `SIGSEGV` o generar mocks que falseen la integración visual.
  - Tiempos de ejecución acotados (< 200 ms) para suites de interfaz en CTest, liberación inmediata de recursos temporales al terminar y garantía de cero procesos huérfanos.
- **Desacoplamiento arquitectónico estricto**:
  - La infraestructura de fixtures (`TestFixtureFoundation`, `TestFixtureValidator`, `TestFixtureService`) permanece 100% confinada en `src/core`, sin dependencias hacia `QWidget`, `QMainWindow` ni `QApplication`.
  - La capa gráfica (`BrowserWindow`) permanece en `src/ui`, dedicada a la experiencia de usuario y ajena a la orquestación de fixtures de prueba.
  - El arnés de prueba `WindowTests.cpp` en `tests/ui` actúa como consumidor de ambas capas, orquestando los fixtures de `core` para validar los widgets de `ui`.

---

## Out of scope

- Implementación física del código de producción y edición de archivos bajo `apps/**` o `docs/rules/**` durante esta fase PROPOSE (restringida taxativamente a `openspec/changes/module-foundations-task0039/**`). La implementación concreta se ejecutará en la fase subsiguiente APPLY tras aprobación humana.
- Creación de paneles gráficos o diálogos de administración de fixtures en la interfaz visible del navegador (terminantemente proscrito por `docs/UX-COPY-PUBLICO.md`).
- Baterías de pruebas de estrés E2E multiproceso con Chromium WebEngine navegando hacia dominios de red externa (reservado para la microtarea `0040` y suites E2E especializadas).
- Conexión con servicios en la nube, sincronización de fixtures remotos o telemetría (incompatible con la política Local-First constitucional, Decisión Q-001).

---

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código actual de `apps/vigilio-browser`, los módulos del núcleo, la persistencia, la interfaz gráfica y el sistema de pruebas:

1. **Especificación de dominio e implementación de servicio completadas (`task0037` y `task0038`)**:
   - `src/core/TestFixtureFoundation.h` y `.cpp`: arnés RAII básico con guardián de entorno `TestEnvironmentGuard`.
   - `src/core/TestFixtureValidator.h` y `.cpp`: motor estático de validación de seguridad de rutas (`isSafeTestDirectory`), confinamiento en `temp_directory_path()`, prevención de traversal (`..`) y copia pública en español neutro.
   - `src/core/TestFixtureService.h` y `.cpp`: servicio desacoplado en C++20 con soporte para cancelación cooperativa (`CancellationToken`), purga de huérfanos (`scanAndPurgeOrphanFixtures`), reintentos resilientes (`cleanWithRetry`), instantáneas atómicas (`createSnapshot` / `rollbackToSnapshot`) y sembrado sintético (`seedBrowserProfile`, `seedMockDatabase`).
   - `src/core/TestFixtureTypes.h`: taxonomía fuerte de tipos (`FixtureOperationStatus`, `FixtureOperationResult`, `OrphanScanReport`, `BrowserProfileSeedOptions`).
   - `tests/unit/TestFixtureFoundationTests.cpp`: 10 pruebas unitarias aprobadas.
   - `tests/unit/TestFixtureServiceTests.cpp`: 12 pruebas unitarias operativas aprobadas.
2. **Arnés de pruebas de interfaz gráfica existente (`apps/vigilio-browser/tests/ui/WindowTests.cpp`)**:
   - 499 líneas de código en CTest (`vigilio_ui_tests` / `window_ui_tests`).
   - Verifica: inicialización de ventana, recuento de pestañas diferidas (lazy loading), logo corporativo SVG compilado en QRC, acciones de navegación con iconos Lucide, metadatos de accesibilidad (`accessibleName`), atajos de teclado, «Interfaz más fácil de usar» con escalado de iconos y controles, contenedor de error nativo (`vigilio-page-error`), y banner de recuperación ante caídas (`vigilio-crash-recovery-banner`).
   - *Hallazgo de brecha técnica*: las líneas 270-282 de `WindowTests.cpp` definen una estructura local ad-hoc `TempDirFixture` para las pruebas de recuperación ante caídas:
     ```cpp
     struct TempDirFixture {
         std::filesystem::path path;
         explicit TempDirFixture(const std::string& prefix) {
             const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
             path = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(now));
             std::error_code ec;
             std::filesystem::create_directories(path, ec);
         }
         ~TempDirFixture() {
             std::error_code ec;
             std::filesystem::remove_all(path, ec);
         }
     };
     ```
     Esta estructura local no valida `isSafeTestDirectory`, no cuenta con protección contra Directory Traversal, no implementa reintentos ante contención de bloqueos ni utiliza las facilidades de sembrado sintético de `vigilio::TestFixtureService`.
3. **Punto de integración actual del sistema de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_ui_tests` enlaza `vigilio_core`, `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test`.
   - Dado que `vigilio_core` ya contiene `TestFixtureFoundation`, `TestFixtureValidator` y `TestFixtureService`, el arnés de UI tiene acceso directo e inmediato a la infraestructura de fixtures sin requerir nuevas dependencias de enlace.
4. **Comprobación del pipeline continuo local (`apps/vigilio-browser/scripts/check.sh`)**:
   - Compilación limpia con GNU GCC 15.2.0 bajo C++20 con `-Werror` y cero advertencias.
   - 15 de 15 pruebas pasando exitosamente en CTest con 100% de éxito en 0.75s (exit code 0):
     - `core_units`: Passed (0.00s)
     - `storage_integration`: Passed (0.00s)
     - `vigilio_warnings_tests`: Passed (0.09s)
     - `qt_dependency_validation_tests`: Passed (0.00s)
     - `webengine_version_validator_tests`: Passed (0.06s)
     - `resource_qrc_catalog_tests`: Passed (0.06s)
     - `resource_inclusion_service_tests`: Passed (0.05s)
     - `resource_qrc_inclusion_tests`: Passed (0.07s)
     - `startup_bootstrap_tests`: Passed (0.08s)
     - `application_lifetime_tests`: Passed (0.14s)
     - `crash_recovery_tests`: Passed (0.03s)
     - `cmake_presets_tests`: Passed (0.04s)
     - `cmake_presets_service_tests`: Passed (0.10s)
     - `test_fixture_foundation_tests`: Passed (0.00s)
     - `test_fixture_service_tests`: Passed (0.01s)
5. **Comprobación de gobernanza con Prompt Builder CLI**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 39 referencias canónicas validadas con estado `valid: true` (exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0115-module.foundations.task0039.propose.lock.json --project-root .`: 13 de 13 comprobaciones pasadas exitosamente (`valid: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js check-eligibility --pack module.foundations.task0039 --project-root .`: 10 de 10 comprobaciones aprobadas (`eligible: true`, exit code 0).
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: Confirma decisiones Q-001 a Q-006 y Q-008 resueltas (Q-007 solo bloquea `release.public-release`).
6. **Referencias visuales y UX**:
   - `/resultado/01-vigilio-home.png`: Ventana principal de navegación, layout sobrio sin saturación.
   - `/resultado/03-vigilio-settings-senior-mode.png`: Interfaz más fácil de usar, botones amplios y tipografía legible.
   - `resultado/06-logo-instalador-oficial.png`: Referencia oficial de marca e icono corporativo.
   - `docs/UX-COPY-PUBLICO.md`: Proscripción de jerga técnica («Test Fixture», «Senior Mode», «Prompt Builder», «DAG») en toda interfaz pública.

---

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `ui`.
- **Pack**: `module.foundations.task0039`.
- **Capacidad**: `test fixture foundation`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0039/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0039/proposal.md`
  - `openspec/changes/module-foundations-task0039/design.md`
  - `openspec/changes/module-foundations-task0039/tasks.md`
  - `openspec/changes/module-foundations-task0039/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0039/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase PROPOSE.

---

## Diseño completo de la capacidad

### 1. Topología de Integración de Fixtures con la Interfaz de Usuario y Pruebas Qt Test

La relación arquitectónica entre el núcleo de fixtures, las pruebas de interfaz y la ventana principal se articula en el siguiente esquema:

```
+-----------------------------------------------------------------------------+
|                                vigilio_core                                 |
|                                                                             |
|  [TestFixtureTypes] <---> [TestFixtureValidator]                            |
|          ^                         ^                                        |
|          |                         |                                        |
|  [TestFixtureFoundation] <-> [TestFixtureService]                           |
|  - Confinamiento /tmp        - Sembrado sintético perfiles                  |
|  - Aislamiento PID+Clock     - Sembrado sintético SQLite                    |
|  - Limpieza segura RAII      - Instantáneas y Rollback                      |
+------------------------------------+----------------------------------------+
                                     |
               (Solo en tiempo de pruebas - Inyección desacoplada)
                                     v
+-----------------------------------------------------------------------------+
|                               vigilio_ui_tests                              |
|                          (tests/ui/WindowTests.cpp)                         |
|                                                                             |
|  - Entorno de prueba aislado mediante TestFixtureFoundation                 |
|  - Sembrado determinista de crash loop y sesiones previas                   |
|  - Evaluación de estados visuales y accesibilidad                           |
|  - Detección de display (DISPLAY / WAYLAND_DISPLAY) -> NOT_EXECUTED         |
+------------------------------------+----------------------------------------+
                                     |
                         (Instanciación en memoria)
                                     v
+-----------------------------------------------------------------------------+
|                                 src/ui/                                     |
|                                                                             |
|  [BrowserWindow] (Producción - Cero dependencias de TestFixture)            |
|  - Sin widgets, menús o diálogos de fixtures (docs/UX-COPY-PUBLICO.md)      |
|  - Página de inicio nativa (vigilio-home) con logo vectorial                |
|  - Contenedor nativo de error (vigilio-page-error) accesible                |
|  - Banner de recuperación ante caídas (vigilio-crash-recovery-banner)       |
|  - Conmutador dinámico «Interfaz más fácil de usar» (Modo Senior)          |
|  - Aislamiento de sesiones de incógnito (Decisión Q-001)                    |
+-----------------------------------------------------------------------------+
```

### 2. Comparativa entre Arnés Ad-hoc y Arnés Canónico en Pruebas de UI

| Atributo / Garantía | Arnés Ad-hoc (`TempDirFixture` en `WindowTests.cpp`) | Arnés Canónico (`vigilio::TestFixtureFoundation` / `TestFixtureService`) |
| --- | --- | --- |
| **Validación de seguridad de ruta** | Inexistente (`remove_all` directo de ruta arbitraria). | Validación estricta con `TestFixtureValidator::isSafeTestDirectory`. |
| **Protección contra Traversal** | Ninguna. | Rechazo explícito de `..` y prefijo `vigilio_test_` mandatorio. |
| **Concurrencia en CTest (`-jN`)** | Dependiente solo de timestamp; riesgo de colisión. | Unicidad triple: PID del proceso + reloj nanosegundos + contador atómico. |
| **Presupuesto de recursos** | Sin límite de cuota; riesgo de saturar `/tmp`. | Presupuesto estricto de 50 MB por fixture (`maxTempDirSizeBytes`). |
| **Sembrado de perfiles de prueba** | Creación manual y dispersa de JSON en código de test. | `TestFixtureService::seedBrowserProfile` estandarizado. |
| **Sembrado de base de datos** | Inexistente o cadenas SQL directas sin WAL. | `TestFixtureService::seedMockDatabase` sintético con modo WAL. |
| **Puntos de control (Snapshots)** | Inexistente (requiere recrear carpetas completas). | `createSnapshot` y `rollbackToSnapshot` rápidos y atómicos. |
| **Cumplimiento de lenguaje público** | Textos dispersos en código de prueba. | Validación centralizada con `docs/UX-COPY-PUBLICO.md`. |

### 3. Contratos de Sembrado Sintético para Pruebas de UI

Para validar los componentes interactivos de `BrowserWindow` que dependen del almacenamiento (tales como la recuperación de caídas y la sesión de usuario), el arnés de pruebas utiliza `TestFixtureService` para sembrar:
1. **Directorio de perfil aislado**:
   ```
   <fixture_path>/profiles/default/
       ├── preferences.json
       ├── cache/
       └── storage/local/
   ```
2. **Archivos de control de sesión y recuperación**:
   - `sentinel.json`: metadatos estructurados del ciclo de vida (PID, estado de salida limpia, recuento de caídas consecutivas, motivo).
   - `session.json`: URLs previas para restaurar pestañas tras cierre anormal.
3. **Base de datos sintética de prueba**:
   - `browser.sqlite`: cabecera SQLite válida de 100 bytes con `journal_mode=WAL` para verificar que la ventana no se bloquee ni falle al inicializar el repositorio local.

### 4. Contratos de Aislamiento y Desacoplamiento

- **Independencia del Núcleo**:
  - `TestFixtureFoundation`, `TestFixtureValidator` y `TestFixtureService` se compilan en `vigilio_core`.
  - Cero inclusión de `<QWidget>`, `<QApplication>`, `<QMainWindow>` o `<QWebEngineView>`.
  - Permite compilación headless perfecta y ejecución de pruebas unitarias en servidores sin servidor gráfico.
- **Independencia de la Ventana Principal**:
  - `BrowserWindow` no conoce ni instancia `TestFixtureService`.
  - Recibe sus rutas de almacenamiento mediante `StartupDiagnosticReport` o parámetros de configuración en el constructor, manteniendo desacoplamiento absoluto de la infraestructura de pruebas.
- **Consumo Exclusivo en Pruebas**:
  - `WindowTests.cpp` es el punto de convergencia donde se instancia el arnés de fixtures para proporcionar un entorno seguro y temporal a `BrowserWindow`.

---

## Archivos afectados

Durante la subsiguiente fase APPLY de este pack, los cambios se concentrarán en los siguientes archivos autorizados:

1. `apps/vigilio-browser/tests/ui/WindowTests.cpp`:
   - Reemplazo de la estructura ad-hoc `TempDirFixture` por `vigilio::TestFixtureFoundation` y `vigilio::TestFixtureService`.
   - Adopción de `isSafeTestDirectory` para validar la seguridad de los directorios temporales de prueba antes y después de cada bloque de aserción.
   - Utilización del sembrado sintético de `TestFixtureService` para pruebas de recuperación de sesión y caídas.
   - Auditoría y revalidación de las aserciones de interacción visual: recuento de pestañas, branding corporativo, iconografía QRC, metadatos de accesibilidad, atajos de teclado, estados de error y conmutación de «Interfaz más fácil de usar».
   - Preservación de la detección honesta de entorno de display (`DISPLAY` / `WAYLAND_DISPLAY`), reportando `NOT_EXECUTED` si falta servidor gráfico.
2. `apps/vigilio-browser/CMakeLists.txt`:
   - Verificación de que `vigilio_ui_tests` mantenga su enlace privado a `vigilio_core` (que incluye la infraestructura de fixtures), `vigilio_storage`, `vigilio_compiler_warnings` y `Qt6::Test`.
   - Preservación de la guarda `if(TARGET Qt6::Test)` para compilación condicional limpia.
3. `docs/rules/01-modules/foundations/micro/0039-test-fixture-foundation/task.md`:
   - Actualización del estado de la microtarea de `NOT_EXECUTED` a `EXECUTED` tras la aprobación y ejecución de APPLY.
4. `openspec/changes/module-foundations-task0039/`:
   - Artefactos OpenSpec de propuesta, diseño, tareas y memoria de ejecución.

---

## Contrato UX/UI

### 1. Ausencia Formal de Cambio Visual Directo en la Interfaz Pública
- En estricto cumplimiento de `docs/UX-COPY-PUBLICO.md`:
  - **No se agregan menús, opciones, botones, ventanas de diálogo ni tooltips de «Test Fixtures» en la ventana principal `BrowserWindow`**.
  - Los términos «Test Fixture», «Fixture Foundation», «Fixture Service», «Orphan Purge», «Snapshots», «Rollback», «Senior Mode», «Modo Senior», «Prompt Builder», «DAG» u «OpenSpec» están terminantemente prohibidos en cualquier superficie gráfica visible al usuario final.
  - La razón técnica es terminante: Vigilio Browser es un navegador para el público general; los arneses de fixtures son herramientas de testing internas de ingeniería que jamás deben contaminar la interfaz de usuario de producción.

### 2. Preservación de Contratos de Presentación en la Ventana Principal (`BrowserWindow`)
Todo entorno visual probado con los arneses de fixtures respeta rigurosamente:
- **Dirección artística y referencias visuales oficiales**:
  - Coherencia con `/resultado/01-vigilio-home.png` (página de bienvenida nativa `vigilio-home` con logo corporativo `branding/vigilio-logo.svg`, barra de navegación limpia, barra de direcciones sobria sin brillos artificiales).
  - Coherencia con `/resultado/03-vigilio-settings-senior-mode.png` (interfaz accesible, botones de gran formato y espaciado holgado).
  - Respeto a `resultado/06-logo-instalador-oficial.png` como referencia de marca e icono corporativo.
- **Iconografía vectorial SVG compilada en QRC**:
  - Barra de herramientas con iconos Lucide SVG (viewBox `0 0 24 24`, trazo 1.8 px): Atrás (`back.svg`), Adelante (`forward.svg`), Recargar (`reload.svg`), Inicio (`home.svg`), Favorito (`bookmarks.svg`), Lista de favoritos (`bookmarks.svg`), Nueva pestaña (`tabs.svg`) y Menú (`settings.svg`).
  - Menú principal con opciones «Interfaz más fácil de usar», «Tema oscuro», «Exportar configuración local…» y «Cerrar navegador».
- **Estados interactivos de widgets**:
  - `idle`: presentación base sobria.
  - `hover`: realce visual sutil sin animaciones intrusivas.
  - `focus`: contorno visible de 2 px para navegación clara por teclado.
  - `disabled`: atenuación visual accesible cuando no hay historial de navegación previo o siguiente.
  - `loading`: indicador en pestaña («Cargando…») y barra de estado («Cargando…»).
  - `empty`: página de inicio nativa (`vigilio-home`) con cero instancias activas de WebEngine.
  - `error`: contenedor nativo accesible (`vigilio-page-error`) con icono de escudo, título «No se pudo cargar la página», mensaje explicativo, botón primario «Reintentar» y secundario «Ir al inicio».
- **Adaptabilidad dinámica para «Interfaz más fácil de usar» (Senior Mode)**:
  - Activación mediante el menú principal (`seniorAction_`) o persistencia local en `QSettings`.
  - Escalado automático de iconos de barra de herramientas: 22×22 px en modo estándar $\to$ 32×32 px en modo accesible.
  - Tap targets mínimos $\ge 48$ px de altura en botones (`QPushButton`) y campos de entrada (`QLineEdit`).
  - Altura mínima de barra de herramientas $\ge 55$ px; banner de recuperación $\ge 56$ px.
  - Tipografía aumentada (+4 pt) manteniendo legibilidad y evitando truncados.
- **Accesibilidad asistiva y atajos de teclado**:
  - Atributos completos `accessibleName` y `accessibleDescription` en todos los controles interactivos.
  - Atajos canónicos: `Ctrl+T` (Nueva pestaña), `Ctrl+W` (Cerrar pestaña), `Ctrl+L` (Barra de direcciones), `Ctrl+D` (Guardar favorito), `Ctrl+B` (Ver favoritos), `Ctrl+Q` (Cerrar navegador), `Alt+Home` (Inicio), `Alt+F` (Menú).

---

## Reglas y fallos

1. **Confinamiento estricto y saneamiento de rutas temporales**:
   - Todo directorio de fixture utilizado en las pruebas de UI debe validar positivamente con `TestFixtureValidator::isSafeTestDirectory`.
   - Se prohíbe taxativamente la eliminación de directorios que no comiencen con `vigilio_test_` o que no residan bajo `std::filesystem::temp_directory_path()`.
2. **Degradación controlada ante entornos sin display gráfico**:
   - Si se ejecutan pruebas de interfaz en un entorno sin servidor X11 o Wayland (`DISPLAY` ni `WAYLAND_DISPLAY`), `WindowTests.cpp` detecta la condición en tiempo de ejecución y retorna exitosamente con código 0 y mensaje descriptivo `NOT_EXECUTED`, evitando fallos espurios en pipelines de integración continua headless.
3. **Resguardo de memoria y recursos (Política Zero-Zombie)**:
   - Las pruebas de UI no crean procesos WebEngine o MCP innecesarios en segundo plano.
   - Todo directorio temporal se purga automáticamente al salir del ámbito RAII mediante el destructor de `TestFixtureFoundation`.
   - Cuota máxima de 50 MB por fixture para impedir que ejecuciones repetitivas agoten el almacenamiento en disco.
4. **Aislamiento de sesiones de incógnito (Decisión Q-001)**:
   - En ventanas con `isIncognitoSession() == true`, no se instancian servicios de recuperación de caídas, no se construyen banners de crash recovery, no se escribe historial en SQLite y se destruye el contexto de sesión al cerrar la ventana.
5. **Resiliencia ante bloqueos transitorios en el sistema de archivos**:
   - En caso de contención de descriptores de archivos durante la limpieza de fixtures de UI en plataformas Windows o bajo antivirus locales, se utiliza el protocolo de reintentos con espera progresiva (`cleanWithRetry`).

---

## Seguridad y privacidad

- **Local-First Constitucional (Decisión Q-001)**:
  - Cero dependencias de servicios externos, servidores en la nube o telemetría en los arneses de fixtures o en la ventana principal.
  - Todas las operaciones de sembrado y validación de archivos se ejecutan estrictamente en el almacenamiento temporal local del host.
- **Higiene de Rutas y Prevención de Directory Traversal**:
  - Toda ruta de fixture es validada estáticamente y en tiempo de ejecución contra secuencias de escape de directorio (`..`).
  - Prohibición absoluta de manipular rutas del sistema o del usuario real (`~/.local/share/vigilio` o `%APPDATA%`).
- **Aislamiento de Sesiones y Datos Sensibles**:
  - Los datos sembrados para pruebas de recuperación ante caídas son puramente sintéticos y locales.
  - Ninguna URL real o dato sensible de navegación se registra en los archivos de sesión o sentinelas de prueba.

---

## Matriz de pruebas

| ID Prueba | Tipo | Fixture / Comando | Resultado Esperado | Manejo de Fallo Negativo |
| --- | --- | --- | --- | --- |
| `TEST-FIXTURE-UI-001` | Integración Fixture | `WindowTests` con `TestFixtureFoundation` | Fixture temporal creado bajo `/tmp/vigilio_test_*` y validado con `isSafeTestDirectory`. | Falla si la ruta es insegura o no se puede crear el directorio. |
| `TEST-FIXTURE-UI-002` | Tolerancia Headless | `ctest -R window_ui_tests` (sin DISPLAY) | Retorna código 0 con mensaje estructurado `NOT_EXECUTED`. | Falla si arroja `SIGSEGV` o cuelgue indefinido en CI headless. |
| `TEST-FIXTURE-UI-003` | Interacción Ventana | `vigilio_ui_tests` en entorno gráfico | Pestañas, buscador, branding corporativo, iconografía Lucide y atajos validados al 100%. | Falla si algún control carece de icono, nombre accesible o atajo. |
| `TEST-FIXTURE-UI-004` | Accesibilidad y Modo Senior | `vigilio_ui_tests` (Senior toggle) | Iconos escalan 22$\to$32 px, botones $\ge 48$ px, toolbar $\ge 55$ px, tipografía aumentada. | Falla si no se respetan las dimensiones mínimas de tap target. |
| `TEST-FIXTURE-UI-005` | Página de Error Nativa | `vigilio_ui_tests` (createErrorPage) | Contenedor `vigilio-page-error` con icono Lucide, textos comprensibles y botones funcionales. | Falla si muestra terminología técnica o falta botón Reintentar. |
| `TEST-FIXTURE-UI-006` | Crash Recovery Banner | `vigilio_ui_tests` (sembrado fixture) | Banner de recuperación visible ante terminación anormal; oculto tras restaurar o descartar. | Falla si el banner no aparece o no limpia el estado. |
| `TEST-FIXTURE-UI-007` | Crash Loop Guard | `vigilio_ui_tests` (sembrado loop) | Banner advierte sobre caídas repetidas; botón «Empezar de cero» es la acción por defecto. | Falla si no alerta sobre caídas repetidas o acción no es default. |
| `TEST-FIXTURE-UI-008` | Aislamiento Incógnito | `vigilio_ui_tests` (sesión incógnito) | Ventana privada con insignia visual, sin banner de recuperación y sin persistencia (Q-001). | Falla si se crea banner o se escribe en SQLite en incógnito. |
| `TEST-FIXTURE-UI-009` | Purga RAII Limpia | Verificación post-test en `/tmp` | El directorio temporal del fixture es eliminado automáticamente al salir del ámbito. | Falla si quedan carpetas huérfanas residuales tras la prueba. |
| `TEST-FIXTURE-UI-010` | Auditoría de Lenguaje | Inspección grep en `src/ui/` | Cero menciones a «Test Fixture», «Senior Mode», «Prompt Builder», «DAG» u «OpenSpec». | Falla si existe terminología interna en cadenas de UI visible. |

---

## Pasos APPLY

Cuando esta propuesta sea aprobada para la fase APPLY, se ejecutarán deterministamente los siguientes pasos:

1. **Refactorizar `apps/vigilio-browser/tests/ui/WindowTests.cpp` para integrar el arnés canónico de fixtures**:
   - Reemplazar la definición local de `struct TempDirFixture` por `#include "core/TestFixtureFoundation.h"`, `#include "core/TestFixtureService.h"` y `#include "core/TestFixtureValidator.h"`.
   - Utilizar `vigilio::TestFixtureFoundation` para los bloques de prueba de recuperación ante caídas (`vigilio_ui_crash_abnormal`, `vigilio_ui_crash_clean`, `vigilio_ui_crash_incognito`, `vigilio_ui_crash_loop`, `vigilio_ui_crash_dismiss`).
   - Validar la contención de rutas temporales con `vigilio::TestFixtureValidator::isSafeTestDirectory`.
2. **Revalidar la suite de pruebas de interfaz de usuario con `vigilio_ui_tests`**:
   - Ejecutar la suite de pruebas Qt Test, verificando inicialización de ventana, recuento de pestañas, branding corporativo, iconografía QRC, accesibilidad, Modo Senior, contenedor de error y banners de recuperación.
   - Confirmar el reporte transparente de `NOT_EXECUTED` cuando no haya display gráfico disponible.
3. **Auditar el cumplimiento de lenguaje público (`docs/UX-COPY-PUBLICO.md`) en todas las superficies visuales**:
   - Confirmar mediante inspección estática que ningún archivo bajo `src/ui/` exponga terminología técnica («Test Fixture», «Senior Mode», etc.).
4. **Ejecutar el pipeline de compilación y pruebas continuas**:
   - Ejecutar `bash apps/vigilio-browser/scripts/check.sh`, validando compilación limpia bajo C++20 con `-Werror`, cero advertencias y 100% de tests en CTest (15/15 tests).
5. **Verificar gobernanza y sincronización de documentación**:
   - Ejecutar `validate-references` y `verify-lock` con Prompt Builder CLI.
   - Actualizar el estado de la microtarea `0039` en `docs/rules/01-modules/foundations/micro/0039-test-fixture-foundation/task.md`.

---

## Definición de terminado

La capacidad se considerará completa en la fase APPLY cuando:
1. `apps/vigilio-browser/tests/ui/WindowTests.cpp` utilice formalmente `vigilio::TestFixtureFoundation` y `vigilio::TestFixtureValidator` en lugar de la estructura ad-hoc `TempDirFixture`.
2. Se declare formalmente el contrato de «sin cambio visual directo» para test fixture foundation en la UI pública conforme a `docs/UX-COPY-PUBLICO.md`.
3. Todos los controles y estados de interfaz de usuario de `BrowserWindow` mantengan su coherencia con `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y las directrices de accesibilidad.
4. Las pruebas de interfaz gestionen de forma limpia y honesta la ausencia de servidor gráfico emitiendo `NOT_EXECUTED`.
5. `apps/vigilio-browser/scripts/check.sh` complete con exit code 0 y 100% de pruebas aprobadas en CTest.
6. Se verifique la gobernanza documental mediante Prompt Builder CLI.

---

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0038`: Implementación previa de la capa de servicio `vigilio::TestFixtureService`.
  - `module.foundations.task0037`: Especificación del modelo de dominio y `vigilio::TestFixtureFoundation`.
  - Qt 6.10+ (`Core`, `Widgets`, `WebEngineWidgets`, `Test`): Dependencia condicional para perfiles de interfaz.
  - CMake 3.24+ y Ninja: Entorno de generación y construcción de perfiles.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Acoplamiento indebido de código de fixtures de prueba dentro de `BrowserWindow`.
    - *Mitigación*: Desacoplamiento estricto; `TestFixtureFoundation` y `TestFixtureService` residen exclusivamente en `vigilio_core` sin incluir cabeceras de Qt Widgets. `BrowserWindow` solo recibe rutas de almacenamiento inyectadas en su constructor.
  - *Riesgo*: Exposición accidental de términos técnicos en la interfaz pública.
    - *Mitigación*: Cumplimiento vinculante de `docs/UX-COPY-PUBLICO.md`; test fixture foundation es una capacidad interna de testing sin cambio visual en la UI pública.
  - *Riesgo*: Fallo de pruebas de UI en entornos CI sin servidor gráfico.
    - *Mitigación*: Detección en tiempo de ejecución de `DISPLAY` / `WAYLAND_DISPLAY` en `WindowTests.cpp`, reportando honestamente `NOT_EXECUTED`.

---

## Commit propuesto

- **Mensaje sugerido**: `feat(foundations): ui test-fixture-foundation`
- **Alcance**: Contrato de UI, integración de arnés de fixtures en WindowTests, tolerancia headless y verificación de accesibilidad para test fixture foundation.
