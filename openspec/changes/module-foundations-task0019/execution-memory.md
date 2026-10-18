# Execution Memory — module-foundations-task0019

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa, rigurosa y formal para la capacidad `resource qrc inclusion` en su etapa `ui` dentro del módulo propietario `foundations` de Vigilio Browser. El alcance define la construcción de la interacción Qt Widgets, estados visuales, iconografía vectorial SVG de Lucide resuelta de forma segura mediante `ResourceInclusionService::resolveIconWithFallback()`, escalado dinámico para «Interfaz más fácil de usar» (Modo Senior), accesibilidad asistiva para lectores de pantalla con etiquetas en español natural (`docs/UX-COPY-PUBLICO.md`), atajos de teclado y la suite de pruebas automatizadas en `tests/ui/WindowTests.cpp`, preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0019-resource-qrc-inclusion/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 soporte multiplataforma x64/arm64)
  - Plan de ejecución del proyecto: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `a0e4551ecbb27a4317596838baa5927e9f0a6b4e3a7f2e92bf291b797be3ed96`
  - Lock SHA-256: `d8a18045f904deebbd94cb438265023194e5afdc7ce93d751e0d02a3cce8fefe`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes directrices y skills canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID y DRY en la capa visual; encapsulamiento de la provisión de iconos a través de `makeResourceIcon`, evitando el acoplamiento directo a rutas de disco o cadenas frágiles de recursos.
- `docs/skills/02-testing-senior.md`: Pruebas de interfaz en `WindowTests.cpp` deterministas, sin sleeps y con fixtures locales, tolerantes ante la ausencia de servidor gráfico en CI headless.
- `docs/skills/03-codigo-cpp-qt.md`: C++20 moderno (`std::string_view`, punteros inteligentes, señales y slots seguros), separación entre núcleo y presentación gráfica, sin operaciones bloqueantes en el hilo principal de eventos de Qt.
- `docs/skills/04-ux-ui-senior.md`: Interfaz sobria, moderna y profesional; azul oscuro como base, superficies limpias, contraste controlado, iconografía SVG consistente sin emojis ni brillos artificiales.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Integración con «Interfaz más fácil de usar»: escalado dinámico de iconos en la barra (22×22 a 32×32 px), controles con áreas táctiles amplias (>= 48 px), navegación secuencial por teclado y atributos `accessibleName` claros.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia en UI: huella de iconos en memoria < 150 KB mediante `QPixmapCache`, tiempo de resolución de iconos < 1 ms en arranque.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first (decisión Q-001): todos los recursos gráficos residen compilados en `.rodata`; cero peticiones de red o telemetría.
- `docs/skills/08-observabilidad-calidad-linters.md`: Formato estricto verificado con Clang-Format, compilación con `-Werror` bajo C++20 y pruebas de regresión en CTest.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación rigurosa entre PROPOSE (diseño integral de la interacción y contratos visuales) y APPLY (implementación del incremento específico en la siguiente fase).
- `docs/skills/11-assets-svg-quality.md`: Contrato de calidad de assets SVG: iconografía vectorial Lucide, viewBox `0 0 24 24`, trazos de 1.8 px, terminaciones redondas y renderizado nítido en diferentes escalas.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería interna («QRC», «RCC», «viewBox», «DOM», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec») en la interfaz visible. Uso obligado de «Interfaz más fácil de usar».
- `/resultado/01-vigilio-home.png`, `/resultado/02-vigilio-tools-productivity.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz gráfica objetivo, distribución de la barra de herramientas, panel de accesibilidad y logotipo oficial de la marca.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. *Resolución determinista de iconos*: Toda acción visual resuelve su icono a través de `ResourceInclusionService::resolveIconWithFallback`, asegurando que ante cualquier error se emplee el fallback canónico `icons/tools/tools.svg`.
  2. *Cumplimiento estricto de textos públicos*: Cero menciones de «Senior Mode» o términos técnicos internos en etiquetas, tooltips o anuncios de lectores de pantalla; uso exclusivo de «Interfaz más fácil de usar».
  3. *Dimensiones mínimas en Senior Mode*: Todo control interactivo garantiza altura >= 48 px y barra >= 55 px con iconos de 32×32 px al activar la accesibilidad avanzada.
  4. *Accesibilidad universal asistiva*: Toda acción interactiva con icono posee `accessibleName` no vacío y atajo de teclado configurado.
  5. *Local-First constitucional (Decisión Q-001)*: Cero descargas remotas de activos; todo recurso proviene del binario compilado.
  6. *Tolerancia Headless*: En ausencia de servidor gráfico (`DISPLAY`/`WAYLAND_DISPLAY`), las pruebas de interfaz retornan `NOT_EXECUTED` con exit code 0 sin crashear.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/ResourceInclusionService.h` y `.cpp`: Servicio de resolución segura de recursos y fallbacks.
  - `apps/vigilio-browser/src/core/ResourceQrcCatalog.h` y `.cpp`: Catálogo canónico de 35 recursos compilados.
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`: Ventana principal de la interfaz de usuario.
  - `apps/vigilio-browser/tests/ui/WindowTests.cpp`: Suite de pruebas Qt UI para verificación automatizada.
  - `apps/vigilio-browser/resources.qrc` y `apps/vigilio-browser/assets/**`: 35 activos SVG, JSON y JPG aprobados.
  - `apps/vigilio-browser/scripts/check.sh`: Pipeline de integración y pruebas automatizadas.
- **Riesgos y mitigaciones**:
  - *Riesgo*: Caída en la ejecución de pruebas UI en servidores CI sin servidor gráfico (sin X11/Wayland).
    *Mitigación*: Verificación dinámica de variables `DISPLAY` / `WAYLAND_DISPLAY` en `WindowTests.cpp`, finalizando con éxito limpio (`NOT_EXECUTED`, exit code 0).
  - *Riesgo*: Degeneración del rendimiento de renderizado por re-rasterización recurrente de SVG.
    *Mitigación*: `QIcon` y `QPixmapCache` gestionan automáticamente el cache en memoria de las representaciones rasterizadas según el tamaño solicitado.
  - *Riesgo*: Fallos de legibilidad de iconos sobre fondos de distinto contraste (temas claro/oscuro).
    *Mitigación*: Los tokens de diseño en `design-tokens/tokens.json` y las hojas de estilo QSS adaptan dinámicamente los colores y fondos para satisfacer WCAG AA/AAA.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Pipeline de compilación y pruebas: `bash apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (7/7 tests pasados en 0.11s, exit code 0):
    - `core_units`: Passed (0.00s)
    - `storage_integration`: Passed (0.00s)
    - `vigilio_warnings_tests`: Passed (0.05s)
    - `qt_dependency_validation_tests`: Passed (0.00s)
    - `webengine_version_validator_tests`: Passed (0.03s)
    - `resource_qrc_catalog_tests`: Passed (0.01s)
    - `resource_inclusion_service_tests`: Passed (0.02s)
  - Validación de formato: Clang-Format verificado sin advertencias ni modificaciones pendientes.
  - Validación de referencias OpenSpec: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .` validó 19/19 referencias con estado `valid: true` (exit code 0).
  - Verificación de lockfile: `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0055-module.foundations.task0019.propose.lock.json --project-root .` superó 13/13 comprobaciones con estado `valid: true` (exit code 0).
  - Registro de decisiones humanas: `validate-decisions` confirmó que no existen decisiones abiertas que bloqueen foundations ni resource qrc inclusion (Q-001 a Q-006 y Q-008 resueltas; Q-007 solo aplica al hito final de lanzamiento público).
- **Handoff**:
  - La propuesta técnica para la etapa `ui` de `module.foundations.task0019` queda formalizada, coherente y lista en `openspec/changes/module-foundations-task0019/` para su revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY (`module.foundations.task0019.apply`).
