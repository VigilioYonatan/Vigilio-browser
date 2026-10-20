# Design — module-ux-shell-task0071

Status: PROPOSED

## Current, target and gaps

### Estado actual (CURRENT)

1. **Estado del subsistema `tab strip density` tras las etapas precedentes (`task0068`, `task0069`, `task0070`)**:
   - `module.ux-shell.task0068` (commit `e05f659`) formalizó el modelo de dominio en `TabStripDensityTypes.h`, `TabStripDensityConfiguration.h/.cpp` y `TabStripDensityValidator.h/.cpp`, con 16 pruebas unitarias de invariantes geométricas básicas, compresión elástica preliminar y hit-testing elemental.
   - `module.ux-shell.task0069` (commit `ec06f94`) implementó la capa operativa desacoplada en `TabStripDensityService.h/.cpp`, proveyendo gestión multihilo del estado (`std::mutex`), conmutación de densidades (`setDensity`), mutaciones de la colección de pestañas (`addTab`, `insertTab`, `removeTab`, `moveTab`, `selectTab`, `setTabPinned`, `setTabMediaState`, `toggleTabAudioMute`, `setTabDiscardState`, `updateTabTitle`), redimensionamiento reactivo (`setAvailableWidth`), persistencia atómica local con tolerancia a fallos (`saveStateToFile`, `loadStateFromFile`), aislamiento de sesión privada de incógnito y cancelación cooperativa Zero-Zombie (`CancellationToken`), respaldado por 18 pruebas unitarias iniciales de servicio.
   - `module.ux-shell.task0070` integró la interacción física Qt Widgets en la ventana principal (`BrowserWindow.h/.cpp`), estructurando el contenedor de pestañas (`vigilio-tabs`), el botón de nueva pestaña (`vigilio-tab-new-button`), la captura reactiva de eventos nativos de redimensionamiento (`resizeEvent`), la conmutación bidireccional con la «Interfaz más fácil de usar» (`setSeniorMode(bool)`) y estilos visuales QSS sobrios sin brillo artificial.
2. **Limitaciones de cobertura en el arnés de pruebas actual**:
   - *Fronteras y casos negativos incompletos en el validador*:
     - No existen pruebas unitarias sistemáticas que exploren las fronteras exactas de dimensiones de tira sub-mínimas (e.g. ancho $399$ px, $200$ px, $0$ px y valores negativos patológicos).
     - No se validan exhaustivamente las violaciones de altura mínima en la densidad `Senior` ($47$ px, $40$ px, $36$ px) frente al código de error tipado `TabStripDensityError::SeniorModeMinimumHeightViolation`.
     - No se evalúan casos patológicos de alta densidad de pestañas (e.g. 100 pestañas en contenedor estrecho de $400$ px) para certificar que la compresión elástica y la activación de desbordamiento horizontal (`TabStripOverflowMode::Scrollable`) preservan la no intersección espacial sin colapsos de memoria ni coordenadas numéricas corruptas.
     - No se prueban escenarios de saturación por pestañas fijadas donde múltiples pestañas fijadas agotan el espacio disponible para pestañas regulares.
     - El hit-testing espacial actual evalúa únicamente puntos centrales o arbitrarios; falta verificar las coordenadas exactas de las cuatro esquinas de cada pestaña, icono, título, botón de cierre e indicador de audio, así como puntos a $\pm 1$ px fuera de los límites.
   - *Ausencia de pruebas de estrés bajo concurrencia masiva (16 hilos)*:
     - Las pruebas de servicio existentes evalúan concurrencia básica o secuencial, pero no someten la instancia compartida de `TabStripDensityService` a estrés mixto masivo con 16 hilos concurrentes ejecutando simultáneamente mutaciones en ráfaga de pestañas, redimensionamiento continuo de ventana, conmutación de densidades y consultas analíticas de plan y hit-test.
   - *Pruebas de fuzzing y recuperación de persistencia limitadas*:
     - Solo se evalúa un caso básico de archivo JSON corrupto; falta evaluar inyecciones sistemáticas de JSON truncado a la mitad, JSON con tipos dispares (strings en identificadores o dimensiones), JSON con campos numéricos negativos y simulación de fallos de disco por permisos de solo lectura (`perms::read_only`).
   - *Certificación formal de aislamiento en navegación privada (Incógnito)*:
     - Se requiere certificar formalmente que en modo incógnito (`isIncognitoMode() == true`), las llamadas a `saveStateToFile` y `loadStateFromFile` abortan de inmediato con 0 operaciones de E/S en disco, garantizando que ningún dato de navegación persista en el almacenamiento secundario local.
   - *Pruebas de ciclo de vida masivo de pestañas en ráfaga*:
     - Falta validar la estabilidad de la selección adyacente y la coherencia del estado bajo operaciones continuas y repetidas de inserción y borrado masivo de pestañas.

---

### Estado objetivo (TARGET)

1. **Suite integral de pruebas de dominio y fronteras en `TabStripDensityTests.cpp`**:
   - Cobertura exhaustiva de todos los códigos de error definidos en `TabStripDensityError`:
     - `WidthBelowMinimum` verificado en los límites exactos ($399$ px, $200$ px, $0$ px, valores negativos).
     - `HeightBelowMinimum` verificado ante alturas $\le 0$ px.
     - `SeniorModeMinimumHeightViolation` verificado ante alturas $< 48$ px en modo accesible.
     - `SeniorModeTapTargetViolation` verificado ante dimensiones de botón de nueva pestaña $< 48\times 48$ px en modo Senior.
     - `AvailableWidthExhausted` verificado ante saturación extrema de pestañas fijadas.
   - Compresión elástica extrema evaluada con 100 pestañas en contenedor estrecho de $400$ px, certificando la activación determinista de `TabStripOverflowMode::Scrollable` y la invariante de no solapamiento espacial ($\forall i \ne j, \text{tabs}[i].\text{rect} \cap \text{tabs}[j].\text{rect} = \emptyset$).
   - Hit-testing espacial exhaustivo evaluando vértices exactos, fronteras y puntos exteriores (`TabStripHitTestZone::Nowhere`).
   - Sanitización robusta ante entradas hostiles (URLs completas con credenciales embebidas, parámetros sensibles, secuencias de escape y strings de longitud extrema).
   - Auditoría programática de todas las cadenas de texto, pistas de remediación, tooltips y etiquetas accesibles frente a `docs/UX-COPY-PUBLICO.md`.
2. **Suite integral de pruebas operativas, resiliencia y concurrencia en `TabStripDensityServiceTests.cpp`**:
   - Prueba de estrés multihilo de alta concurrencia con 16 hilos (`std::thread`) concurrentes ejecutando operaciones heterogéneas continuas (mutaciones de pestañas, redimensionamiento continuo, conmutación de densidad y consultas de plan y hit-test), validando la ausencia absoluta de carreras de datos, interbloqueos y corrupción de memoria (ThreadSanitizer-clean).
   - Prueba de cancelación cooperativa multihilo mediante `CancellationToken` disparado en instantes asíncronos arbitrarios, verificando respuesta inmediata (< 10 ms), preservación del estado previo y ausencia de recursos huérfanos (política Zero-Zombie).
   - Batería de pruebas de recuperación y resiliencia ante corrupción de archivos de persistencia:
     - Inyección de JSON truncado a la mitad.
     - Inyección de JSON con campos de tipos dispares.
     - Inyección de JSON con valores numéricos negativos deliberados.
     - Fallos simulados de escritura en disco por directorios de solo lectura (`std::filesystem::perms::read_only`).
     - Verificación del restablecimiento transparente del plan canónico por defecto (`canonicalDefaultDensity()`) y retorno de `false` sin caídas de proceso.
   - Verificación de aislamiento absoluto en modo incógnito: llamadas a persistencia abortadas inmediatamente con cero operaciones de E/S en disco.
   - Bucle de mutaciones masivas de pestañas en ráfaga (50 adiciones y eliminaciones alternadas), comprobando que la selección adyacente mantiene invariantes válidas sin excepciones ni punteros colgantes.
3. **Arnés de integración de interfaz gráfica en `WindowTests.cpp`**:
   - Validación de la integración reactiva entre `BrowserWindow` y `TabStripDensityService`:
     - Verificación de que `resizeEvent` sincroniza dimensiones con el servicio.
     - Verificación de que la activación de la «Interfaz más fácil de usar» impone altura de tira $\ge 48$ px y tap targets $\ge 48\times 48$ px en el botón de nueva pestaña y botones de cierre.
     - Verificación de navegación por teclado (`focusPolicy != Qt::NoFocus`) y atributos de accesibilidad (`accessibleName`, `accessibleDescription`).
     - Verificación de tolerancia headless determinista (reportando `NOT_EXECUTED` limpiamente si `DISPLAY`/`WAYLAND_DISPLAY` está ausente).
     - Auditoría estricta de textos accesibles y cadenas visibles frente a `docs/UX-COPY-PUBLICO.md`.

---

### Brechas (GAPS) y plan de cierre

| ID Brecha | Componente | Descripción de la brecha | Plan de cierre técnico |
| :--- | :--- | :--- | :--- |
| **GAP-001** | `TabStripDensityTests.cpp` | Ausencia de pruebas de frontera sub-mínimas ($399$ px, $0$ px, negativos) | Implementar `testSubMinimumDimensionsExhaustive` verificando `WidthBelowMinimum` |
| **GAP-002** | `TabStripDensityTests.cpp` | Ausencia de pruebas de violación de altura en modo Senior ($47$ px, $36$ px) | Implementar `testSeniorModeHeightViolations` verificando `SeniorModeMinimumHeightViolation` |
| **GAP-003** | `TabStripDensityTests.cpp` | Falta de prueba de estrés elástico con 100 pestañas en $400$ px | Implementar `testExtremeOverflowAndElasticCompression` verificando scroll e invariantes |
| **GAP-004** | `TabStripDensityTests.cpp` | Falta de prueba de saturación por pestañas fijadas | Implementar `testPinnedTabsSaturationAndSpaceExhaustion` verificando `AvailableWidthExhausted` |
| **GAP-005** | `TabStripDensityTests.cpp` | Hit-testing incompleto en las 4 esquinas de pestañas y botones | Implementar `testExhaustiveHitTestingBoundaries` evaluando vértices y puntos exteriores |
| **GAP-006** | `TabStripDensityTests.cpp` | Falta de validación de sanitización hostil de URLs complejas | Implementar `testHostileTitleSanitizationAndPrivacy` con URLs con credenciales y bytes nulos |
| **GAP-007** | `TabStripDensityTests.cpp` | Falta de auditoría programática completa de textos públicos | Implementar `testPublicCopyExhaustiveAudit` verificando conformidad con `docs/UX-COPY-PUBLICO.md` |
| **GAP-008** | `TabStripDensityServiceTests.cpp` | Falta de prueba de estrés multihilo de 16 hilos compitiendo | Implementar `testMultiThreadedHighStressConcurrency16Threads` ($\ge 200$ iters/hilo) |
| **GAP-009** | `TabStripDensityServiceTests.cpp` | Falta de prueba de cancelación cooperativa asíncrona | Implementar `testCooperativeCancellationStress` con `CancellationToken` y timeout < 10 ms |
| **GAP-010** | `TabStripDensityServiceTests.cpp` | Falta de fuzzing de archivos JSON truncados o corruptos | Implementar `testCorruptedPersistenceFuzzingAndRecovery` con restablecimiento a factoría canónica |
| **GAP-011** | `TabStripDensityServiceTests.cpp` | Falta de prueba de simulación de permisos de solo lectura | Implementar `testDiskIoFailureHandling` verificando retorno limpio de `false` |
| **GAP-012** | `TabStripDensityServiceTests.cpp` | Falta de certificación formal de 0 escrituras en modo incógnito | Implementar `testIncognitoStrictPrivacyIsolation` verificando aislamiento total en disco |
| **GAP-013** | `TabStripDensityServiceTests.cpp` | Falta de prueba de ráfaga continua de mutaciones de pestañas | Implementar `testContinuousTabMutationsBurst` con 50 operaciones sucesivas |
| **GAP-014** | `WindowTests.cpp` | Falta de aserciones de integración de tira de pestañas y tap targets | Refinar `WindowTests.cpp` con verificación de `tabStripDensityService()` y tap targets $\ge 48$ px |

---

## Compatibility, migration and rollback

### Compatibilidad hacia atrás

- **Contratos inmutables de C++20**:
  - Las nuevas pruebas consumen exclusivamente las APIs públicas y métodos existentes de `TabStripDensityConfiguration`, `TabStripDensityValidator` y `TabStripDensityService`. No se alteran firmas de métodos, estructuras de datos (`TabSpec`, `TabItemLayout`, `TabStripDensityPlan`) ni enumeraciones (`TabStripDensityError`, `LayoutDensity`).
- **Compatibilidad binaria y de ABI**:
  - Al tratarse de adiciones exclusivas en los archivos de arnés de pruebas (`tests/unit/` y `tests/ui/`), el código del núcleo `libvigilio_core.a` permanece $100\%$ compatible a nivel de ABI y enlace con el resto de la aplicación.
- **Tolerancia en plataformas heterogéneas**:
  - Las pruebas operativas y de dominio son independientes del entorno gráfico y se ejecutan limpiamente en Linux, Windows y macOS.
  - Las pruebas de interfaz en `WindowTests.cpp` detectan automáticamente la disponibilidad de display gráfico (`DISPLAY` / `WAYLAND_DISPLAY`), retornando `NOT_EXECUTED` de manera determinista cuando se ejecutan en entornos CI/CD sin servidor X11/Wayland, previniendo falsos fallos.

### Migración y coexistencia

- **Sustitución no disruptiva del arnés**:
  - Las pruebas añadidas amplían la cobertura de los ejecutables `vigilio_tab_strip_density_tests` y `vigilio_tab_strip_density_service_tests` sin invalidar los casos de prueba precedentes de las tareas 0068 y 0069.
  - La ejecución bajo CTest se mantiene unificada (`ctest --test-dir build -L fast`), reportando tiempos de ejecución contenidos (< 5 segundos para la suite completa).
- **Archivos de prueba y datos sintéticos**:
  - Toda prueba de persistencia o E/S opera exclusivamente sobre rutas temporales generadas mediante `std::filesystem::temp_directory_path()` con prefijo `vigilio_test_tabstrip_` y limpieza determinista al finalizar mediante bloques RAII (`std::filesystem::remove`).

### Estrategia de reversión (Rollback)

- **Aislamiento en Git**:
  - Todo el incremento de este pack se confina a un único commit descriptivo `test(ux-shell): tests tab-strip-density`.
  - En caso de detectarse alguna regresión o inestabilidad, la reversión se efectúa limpiamente mediante `git revert HEAD` sin afectar el código de producción ni la persistencia local del usuario.
- **Ausencia de efectos secundarios persistentes**:
  - Al no ejecutarse migraciones de esquema de base de datos ni modificaciones en configuraciones de producción, la reversión del commit restaura el estado del repositorio al commit `ec06f94` sin necesidad de pasos de saneamiento adicionales.
