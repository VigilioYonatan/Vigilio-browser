# Change Proposal — module-foundations-task0013

Pack: `module.foundations.task0013`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **WebEngine version compatibility** en el módulo propietario `foundations` de Vigilio Browser. Tras haber completado la validación de dependencias Qt (`module.foundations.task0009` a `task0012`) con la especificación de Qt >= 6.10.0 (decisión Q-002 de `docs/rules/answers.md`), su validador desacoplado en C++20 (`vigilio::QtDependencyValidator`) y su suite de pruebas unitarias exhaustivas en CTest, esta microtarea formaliza el modelo de dominio, las reglas de negocio, las invariantes de versión y el ciclo de vida de validación de compatibilidad con el motor embebido Chromium/WebEngine.

El alcance concreto de esta etapa **domain** comprende:
- **Modelo de dominio de versiones y capacidades de WebEngine (`WebEngine version compatibility`)**:
  - Definición de la entidad fuertemente tipada `ChromiumVersion` en C++20 para modelar la versión cuatripartita de Chromium (`major.minor.build.patch`), comparación total (`<=>`), normalización y parsing seguro de cadenas.
  - Especificación del estándar de versión canónico: Chromium milestone >= 128 (asociado al baseline Qt >= 6.10.0 y garantizando compatibilidad con Manifest V3 [MV3], isolation de procesos y mitigaciones de seguridad modernas).
  - Taxonomía de capacidades funcionales del motor (`WebEngineFeature`):
    - `ManifestV3Extensions`: soporte del motor para extensiones web bajo la especificación MV3 sin requerir APIs deprecadas de MV2.
    - `PdfViewerInternal`: visualizador integrado de documentos PDF aislado en proceso de renderizado no privilegiado.
    - `WebRTC`: soporte para comunicaciones multimedia en tiempo real sujeto a políticas de protección de direcciones IP locales.
    - `DevToolsFrontend`: protocolo de inspección remota e instrumentalización local de depuración.
    - `ProcessSandboxing`: aislamiento multiproceso mediante namespaces de usuario, seccomp-bpf en Linux y tokens de seguridad en Windows/macOS.
    - `HardwareAcceleration`: renderizado acelerado por GPU y soporte de primitivas WebGL/WebGPU seguras.
    - `LocalDevStorage`: particionamiento estricto de almacenamiento local (cookies, indexedDB, localStorage) respetando la privacidad del usuario.
    - `Http3Quic`: soporte nativo del protocolo de transporte HTTP/3 y QUIC.
  - Definición de perfiles de compatibilidad (`WebEngineCompatibilityProfile`):
    - *FullDesktopBrowser*: perfil completo de escritorio que exige Chromium >= 128, soporte de extensiones MV3, PDF seguro y aislamiento estricto de procesos.
    - *HeadlessInspection*: perfil de comprobación previa y auditoría de herramientas que verifica la compatibilidad de versiones sin exigir servidor gráfico ni aceleración GPU.
    - *RestrictedSecureProfile*: perfil de alta privacidad que desactiva WebRTC y características de fingerprinting manteniendo el motor Chromium en modo seguro.
    - *MinimalCore*: perfil de contingencia en modo degradado donde el motor WebEngine está inhabilitado y el navegador opera únicamente con componentes locales y almacenamiento SQLite.
  - Taxonomía exhaustiva de resultados de validación (`WebEngineCompatibilityStatus`):
    - `Compatible`: versión de Qt WebEngine y Chromium igual o superior al umbral mínimo (Qt >= 6.10.0, Chromium >= 128), proceso asistente `QtWebEngineProcess` localizado y ejecutable, y capacidades del perfil satisfechas.
    - `DegradedHeadless`: motor web inhabilitado o no disponible en el sistema; la arquitectura degrada limpiamente a modo núcleo headless sin provocar cierres inesperados.
    - `IncompatibleQtVersion`: versión de Qt menor a 6.10.0 detectada en runtime o en cabeceras de enlace.
    - `IncompatibleChromiumVersion`: hito de Chromium embebido inferior al umbral mínimo soportado (< 128), lo que implicaría riesgos de seguridad conocidos o ausencia de soporte MV3.
    - `ProcessHelperMissing`: binario ejecutable auxiliar `QtWebEngineProcess` no encontrado en la ruta esperada de librerías (`QLibraryInfo::LibraryExecutablesPath`) o sin permisos de ejecución (`X_OK`).
    - `FeatureUnsupported`: una capacidad crítica requerida por el perfil solicitado no está disponible en la compilación del motor.
    - `EnvironmentMisconfigured`: el entorno del sistema operativo carece de soporte para espacios de nombres de usuario o display server requerido para inicializar los procesos Chromium.
  - Estructura inmutable de reporte (`WebEngineCompatibilityReport`):
    - Agrupa el estado final (`status`), versiones detectadas de Qt y Chromium, listas de características soportadas y ausentes, ruta del ejecutable asistente, diagnóstico legible y acción de mitigación recomendada.
- **Servicio de dominio desacoplado (`vigilio::WebEngineVersionValidator`)**:
  - Clase de servicio puro en C++20 ubicada en `vigilio_core`, libre de cualquier dependencia de cabeceras de `Qt6::WebEngineWidgets` o Chromium.
  - Métodos inmutables y libres de efectos secundarios:
    - `parseChromiumVersion`: parsing determinista de cadenas de versión con rechazo de malformaciones.
    - `minimumChromiumVersionForQt`: resolución del baseline de Chromium asociado a una versión de Qt dada.
    - `baselineChromiumVersion`: obtención de la versión mínima soportada (Chromium 128.0.0.0).
    - `supportedFeaturesForMilestone`: matriz de capacidades garantizadas según el hito de Chromium.
    - `requiredFeaturesForProfile`: mapeo determinista entre perfiles de navegación y capacidades requeridas.
    - `validateCompatibility`: evaluación integral del estado de compatibilidad a partir de versiones, perfil y ruta de proceso asistente.
    - `formatStatus`, `formatReport`, `featureName`, `statusName`: generadores de diagnósticos claros para observabilidad y logs.
- **Reglas e invariantes de arquitectura y seguridad**:
  - *Desacoplamiento de Chromium*: el núcleo de dominio `vigilio_core` no incluye cabeceras de Chromium ni enlaza con librerías dinámicas de WebEngine, garantizando portabilidad y compilación en sistemas headless.
  - *Filosofía local-first*: la validación opera exclusivamente con metadatos locales y variables de entorno del sistema; prohibida cualquier consulta de red o verificación remota.
  - *Presupuesto de recursos*: evaluación en microsegundos (< 1 ms), consumo de memoria despreciable (< 50 KB), sin instanciar procesos WebEngine ni crear procesos zombies.
  - *Ausencia total de cambio visual directo*: la validación de versiones es infraestructura de dominio; no modifica widgets ni diálogos visibles, respetando estrictamente `docs/UX-COPY-PUBLICO.md`.

## Out of scope

- Implementación del validador de runtime en `src/core/WebEngineVersionValidator.cpp` (diferida a la etapa `implementation` en `module.foundations.task0014`).
- Integración visual de advertencias o estados de degradación en la ventana principal `BrowserWindow` (diferida a la etapa `ui` en `module.foundations.task0015`).
- Creación de ejecutables de prueba en CTest y fixtures sintéticos de Chromium (diferida a la etapa `tests` en `module.foundations.task0016`).
- Inicialización o configuración de cookies, caché HTTP o políticas de navegación de `QWebEngineProfile` (pertenecientes al módulo `security`/`privacy`).
- Modificación de código fuente en `apps/**` o reglas en `docs/rules/**` durante esta fase PROPOSE (restringida a `openspec/changes/module-foundations-task0013/**`).
- Descarga de binarios de Chromium o dependencias remotas desde la red.

## Contexto y evidencia

Se inspeccionó rigurosamente la base de código de `apps/vigilio-browser` y la autoridad canónica:
1. **Configuración de construcción (`apps/vigilio-browser/CMakeLists.txt`)**:
   - Gobierna la inclusión de `Qt6::WebEngineWidgets` condicionada a `VIGILIO_BUILD_GUI=ON` y versión mínima `6.10.0`.
   - Cuando Qt6 no está presente y `VIGILIO_REQUIRE_QT=OFF`, el sistema degrada a compilar `vigilio_core` y `vigilio_storage` en modo headless con éxito.
2. **Infraestructura de validación existente (`src/core/QtDependencyValidator.h` y `.cpp`)**:
   - Implementa `SemanticVersion`, `QtModuleId`, `QtRequirementProfile`, `QtValidationOutcome` y `QtValidationReport`.
   - Ha demostrado la viabilidad y elegancia del patrón de validación desacoplada en C++20 con suite de pruebas unitarias (`tests/unit/QtDependencyValidatorTests.cpp`) pasando 100% en CTest.
3. **Punto de arranque de la aplicación (`src/app.cpp`)**:
   - Ejecuta `QtDependencyValidator::validateRuntimeEnvironment` antes de inicializar `QApplication`.
   - Actualmente carece de una verificación de compatibilidad de versión de Chromium y existencia del asistente `QtWebEngineProcess`, lo que expone al sistema a fallos no controlados si `QtWebEngineProcess` está ausente o si el entorno ejecuta una versión no compatible de Chromium.
4. **Decisiones canónicas (`docs/rules/answers.md` y `docs/rules/00-global/architecture.md`)**:
   - Q-001: Local-first, sin cuentas ni nube de Vigilio; validación sin red.
   - Q-002: Base tecnológica C++20 / Qt 6.10+ / WebEngine / SQLite / CMake.
   - Q-004: Soporte multiplataforma Windows, Linux, macOS.
   - Arquitectura: el núcleo de dominio no depende de Widgets ni de Chromium; WebEngine trata URLs y contenido remoto como no confiable; isolation de procesos y presupuestos de memoria estrictos.
5. **Verificación de herramientas del entorno**:
   - Compilación y pruebas locales: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (4/4 tests pasados en 0.18s).
   - Validación de referencias OpenSpec: `validate-references` arrojó 13 referencias validadas con `valid: true`.
   - Verificación de lockfile: `verify-lock` sobre `0037-module.foundations.task0013.propose.lock.json` superó 13/13 comprobaciones (`valid: true`).
   - Registro de decisiones: `validate-decisions` confirmó que ninguna decisión bloquea foundations ni webengine.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0013`.
- **Capacidad**: `WebEngine version compatibility`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0013/canonical-reference.yaml`
  - `openspec/changes/module-foundations-task0013/proposal.md`
  - `openspec/changes/module-foundations-task0013/design.md`
  - `openspec/changes/module-foundations-task0013/tasks.md`
  - `openspec/changes/module-foundations-task0013/execution-memory.md`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0013/**`. No modificar `apps/**` ni `docs/rules/**` durante esta fase.

## Diseño completo de la capacidad

La solución integral de **WebEngine version compatibility** para la etapa **domain** se estructura en:
1. **Entidad de valor `ChromiumVersion`**:
   - Componentes numéricos inmutables: `major` (hito/milestone), `minor`, `build`, `patch` (`uint32_t`).
   - Comparación total con operador de nave espacial (`<=>`) y operadores de igualdad estándar C++20.
   - Predicado `isCompatibleWith(const ChromiumVersion& requiredMin)`: verifica que el hito mayor sea compatible y que la versión sea igual o superior a la requerida.
   - Métodos utilitarios: `toString()` (formato canónico `X.Y.Z.W`) y `parse(std::string_view)` con rechazo seguro de caracteres no numéricos, desbordamientos y cadenas malformadas.
2. **Taxonomía de capacidades `WebEngineFeature`**:
   - Modelado tipado de características dependientes del motor web: `ManifestV3Extensions`, `PdfViewerInternal`, `WebRTC`, `DevToolsFrontend`, `ProcessSandboxing`, `HardwareAcceleration`, `LocalDevStorage`, `Http3Quic`.
3. **Perfiles de requerimiento `WebEngineCompatibilityProfile`**:
   - Mapeo declarativo de los requisitos del motor según el contexto de ejecución: `FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`.
4. **Estados y resultados `WebEngineCompatibilityStatus`**:
   - Enum class que cubre exhaustivamente todas las condiciones de evaluación: `Compatible`, `DegradedHeadless`, `IncompatibleQtVersion`, `IncompatibleChromiumVersion`, `ProcessHelperMissing`, `FeatureUnsupported`, `EnvironmentMisconfigured`.
5. **Estructura de reporte `WebEngineCompatibilityReport`**:
   - Consolidación de diagnóstico, versiones detectadas, capacidades soportadas y faltantes, ruta del asistente y acción de remediación sugerida.
6. **Servicio puro de dominio `WebEngineVersionValidator`**:
   - Provee funciones estáticas inmutables para el análisis y verificación de compatibilidad, con total desacoplamiento de librerías GUI y WebEngine.

## Archivos afectados

En esta fase **PROPOSE**, los únicos archivos creados/modificados se encuentran bajo:
- `openspec/changes/module-foundations-task0013/canonical-reference.yaml`
- `openspec/changes/module-foundations-task0013/proposal.md`
- `openspec/changes/module-foundations-task0013/design.md`
- `openspec/changes/module-foundations-task0013/tasks.md`
- `openspec/changes/module-foundations-task0013/execution-memory.md`

En fases futuras de este ciclo (`implementation`, `ui`, `tests`):
- `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` (nuevo archivo de dominio).
- `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp` (implementación del servicio).
- `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` (suite de pruebas unitarias).
- `apps/vigilio-browser/CMakeLists.txt` (inclusión de nuevas fuentes y target de CTest).

## Contrato UX/UI

- **Declaración formal**: **Sin cambio visual directo**.
- **Justificación**: Esta capacidad corresponde a la infraestructura interna de dominio y validación de compatibilidad de la arquitectura del navegador. No introduce componentes visuales, ventanas, botones ni menús en la interfaz de usuario.
- **Cumplimiento de textos públicos**: Se respeta de manera absoluta `docs/UX-COPY-PUBLICO.md`. Queda terminantemente prohibido mostrar términos técnicos internos («Chromium», «WebEngine», «milestone», «sandboxing», «MV3», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en interfaces visibles al usuario común.
- **Integración con Interfaz más fácil de usar**: Cuando el modo con controles grandes y simplificados esté activo, cualquier error o advertencia a nivel de sistema se transmitirá mediante mensajes comprensibles y accesibles a través de los canales estándar de la ventana principal definidos en etapas de UI, sin alterar el diseño ni la geometría especificada en `resultado/01-vigilio-home.png` y `resultado/03-vigilio-settings-senior-mode.png`.

## Reglas y fallos

- **R-001 (Versión mínima canónica)**: Todo entorno de ejecución de escritorio con WebEngine activado debe cumplir Qt >= 6.10.0 y Chromium milestone >= 128.
- **R-002 (Incompatibilidad de versión de Chromium)**: Si la versión detectada de Chromium es menor a 128 (e.g. Chromium 108 de Qt 6.5 LTS o Chromium 118 de Qt 6.7), el validador emite `IncompatibleChromiumVersion` detallando la ausencia de mitigaciones de seguridad modernas y soporte MV3.
- **R-003 (Proceso asistente obligatorio)**: En sistemas Linux, Windows y macOS, la ruta del ejecutable `QtWebEngineProcess` debe verificarse en el arranque. Si el binario no existe o no tiene permisos de ejecución, se emite `ProcessHelperMissing`.
- **R-004 (Degradación elegante a modo headless)**: Si el motor WebEngine no está compilado o se desactiva explícitamente (`MinimalCore`), el sistema emite `DegradedHeadless`, permitiendo que el núcleo y almacenamiento continúen operativos sin cierres abruptos (`SIGSEGV`/`std::abort`).
- **R-005 (Entrada malformada o vacía)**: Cadenas de versión nulas, vacías o que contengan caracteres no numéricos o desbordamientos deben retornar `std::nullopt` sin generar excepciones ni fugas de memoria.
- **R-006 (Determinismo y pureza)**: Las funciones de validación son puras y seguras para ejecución concurrente desde múltiples hilos (`thread-safe`), sin modificar variables globales ni el entorno del sistema operativo.

## Seguridad y privacidad

- **Filosofía Local-First (Q-001)**: Cero peticiones a redes externas o servicios en la nube para comprobación de versiones o descarga de binarios.
- **Aislamiento de procesos Chromium**: La validación de WebEngine confirma que las opciones de sandbox de Chromium permanezcan activas y que los procesos de renderizado se ejecuten con mínimos privilegios.
- **Aislamiento de perfiles**: No se permite persistir datos de perfiles privados/incógnito en bases de datos SQLite ordinarias.
- **Privacidad de observabilidad**: Los diagnósticos emitidos por el validador no registran URLs, cookies, datos de navegación ni información personal sensible del usuario; únicamente reportan versiones y estados de componentes del sistema.

## Matriz de pruebas

La verificación integral de la capacidad se distribuye en:
1. **Pruebas unitarias de dominio (`tests/unit/WebEngineVersionValidatorTests.cpp`)** — *Planificadas para etapas de implementación y pruebas*:
   - Parsing válido de versiones Chromium: estándar 4 componentes (`"128.0.6613.120"`), 3 componentes normalizados (`"128.0.6613"` -> patch 0), espacios en blanco y prefijos (`"v130.0.0.1"`).
   - Rechazo de cadenas inválidas: vacías, caracteres alfabéticos, puntos dobles, desbordamiento numérico.
   - Comparación y ordenamiento: operadores `<=>`, `==`, `<`, `>`, compatibilidad con versión mínima.
   - Perfiles de compatibilidad: resolución de `FullDesktopBrowser`, `HeadlessInspection`, `RestrictedSecureProfile`, `MinimalCore`.
   - Diagnóstico estructurado: consistencia de mensajes legibles generados por `formatStatus` y `formatReport`.
   - Seguridad en concurrencia: invocaciones multihilo concurrentes sin colisiones ni condiciones de carrera.
2. **Pruebas de integración de runtime (`app.cpp`)** — *Etapa LATER*:
   - Verificación de arranque controlado: ante incompatibilidad de Chromium, la aplicación finaliza con código de error 1 y diagnóstico en stderr antes de instanciar ventanas web.
3. **Pruebas de UI y accesibilidad** — *NOT_APPLICABLE para etapa domain*.
4. **Pruebas E2E** — *NOT_APPLICABLE para etapa domain*.

## Pasos APPLY

El plan de trabajo incremental para materializar esta propuesta comprende:
1. **Paso 1**: Crear la cabecera de dominio `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` conteniendo las entidades `ChromiumVersion`, `WebEngineFeature`, `WebEngineCompatibilityProfile`, `WebEngineCompatibilityStatus`, `WebEngineCompatibilityReport` y la clase `WebEngineVersionValidator`.
2. **Paso 2**: Implementar el servicio puro en `apps/vigilio-browser/src/core/WebEngineVersionValidator.cpp`.
3. **Paso 3**: Registrar las nuevas fuentes en `vigilio_core` dentro de `apps/vigilio-browser/CMakeLists.txt`.
4. **Paso 4**: Crear la suite de pruebas unitarias `apps/vigilio-browser/tests/unit/WebEngineVersionValidatorTests.cpp` y su target ejecutable en CTest.
5. **Paso 5**: Ejecutar `apps/vigilio-browser/scripts/check.sh` y validar compilación limpia bajo `-Werror` y 100% de pruebas aprobadas.

## Definición de terminado

La capacidad se considerará terminada para la etapa **domain** cuando:
1. La propuesta OpenSpec contenga especificaciones exhaustivas, sin marcadores pendientes ni texto provisional, coherentes con la autoridad canónica.
2. Las invariantes de versión mínima (Chromium >= 128 / Qt >= 6.10.0), perfiles y taxonomía de errores estén unívocamente formalizados.
3. Se garantice el desacoplamiento estricto del núcleo de dominio frente a cabeceras de Chromium y Qt WebEngine.
4. Las referencias canónicas de OpenSpec sean validadas al 100% por la herramienta Prompt Builder (`validate-references`).
5. La suite de comprobación local `check.sh` mantenga su estado 100% aprobado sin regresiones.

## Dependencias y riesgos

- **Dependencias**:
  - `module.foundations.task0012`: Pruebas de validación de dependencias Qt (completada y aprobada).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Complejidad en la determinación del hito exacto de Chromium en tiempo de compilación sin incluir cabeceras pesadas de WebEngine.
    *Mitigación*: Definición de una matriz determinista en `WebEngineVersionValidator` que correlaciona versiones conocidas de Qt6 (>= 6.10.0) con sus respectivos hitos de Chromium, complementada con detección en runtime cuando las cabeceras estén disponibles.
  - *Riesgo*: Falsos positivos en la verificación del binario `QtWebEngineProcess` en entornos empaquetados o contenedores.
    *Mitigación*: Soporte para rutas configurables y consulta a través de `QLibraryInfo` respetando la estructura de instalación estándar de Qt.

## Commit propuesto

`docs(foundations): domain webengine-version-compatibility`
