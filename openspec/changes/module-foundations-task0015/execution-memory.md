# Execution Memory — module-foundations-task0015

Status: PROPOSED

## Objective and authority

- **Objetivo**: Diseñar la propuesta técnica completa y rigurosa para la capacidad `WebEngine version compatibility` en su etapa `ui` (especificación de interacción en Qt Widgets, gestión de estados visuales `empty`, `loading`, `success`, `error`, `deferred`, contenedor nativo accesible ante fallos de carga con opciones de reintento, integración con la «Interfaz más fácil de usar» [Senior Mode] escalando controles a >= 48px/55px, atajos de teclado y gobernanza de pruebas de UI en CMake/CTest), preparando el cambio OpenSpec para su revisión y aprobación humana antes de la fase APPLY.
- **Autoridad canónica**:
  - `docs/rules/01-modules/foundations/README.md`
  - `docs/rules/01-modules/foundations/design.md`
  - `docs/rules/01-modules/foundations/screens.md`
  - `docs/rules/01-modules/foundations/business.md`
  - `docs/rules/01-modules/foundations/tests.md`
  - `docs/rules/01-modules/foundations/operations.md`
  - `docs/rules/01-modules/foundations/micro/0015-webengine-version-compatibility/task.md`
  - Baseline de fábrica: `docs/rules/factory-baseline.yaml`
  - Arquitectura transversal: `docs/rules/00-global/architecture.md`
  - Registro de decisiones humanas: `docs/rules/answers.md` (Q-001 local-first, Q-002 C++20/Qt 6.10+/WebEngine/SQLite/CMake, Q-004 multiplataforma)
  - Plan de ejecución: `docs/plans/full-project.yaml`
  - Prompt SHA-256: `ab42bff46e471125e9afa17481ef2542bc5673bf034b57eebbc8cbfe89b7bfda`
  - Lock SHA-256: `97574c4ca74815d0648e579f287c2f8d009e92df80024e9921de4266dab5cb77`
  - Versión de Prompt Builder requerida: `0.6.3`

## Skills and required references read

Se consultaron e incorporaron al 100% como norma vinculante las siguientes skills y directrices canónicas:
- `docs/skills/01-principios-de-ingenieria.md`: Principios SOLID aplicados a la capa de presentación: responsabilidad única en la gestión de estados visuales; desacoplamiento estricto entre el modelo de validación (`WebEngineVersionValidator`) y los widgets gráficos (`BrowserWindow`); observancia de DRY y KISS mediante la reutilización de componentes nativos de Qt Widgets y tokens visuales compartidos.
- `docs/skills/02-testing-senior.md`: Pirámide de pruebas: verificación de estados de interfaz mediante pruebas de UI en CTest (`vigilio_ui_tests`); aislamiento sin requerir servidor gráfico externo forzado; reporte honesto y explícito de `NOT_EXECUTED` ante la ausencia de bibliotecas gráficas en entornos headless; cero sleeps fijos.
- `docs/skills/03-codigo-cpp-qt.md`: Uso de C++20 moderno y buenas prácticas Qt: ownership estricto de widgets (`parent/child`), destrucción segura de objetos web con `deleteLater()`, inmutabilidad y `const-correctness`, ausencia de llamadas bloqueantes en el hilo principal de la interfaz (`main thread`), y separación limpia entre `src/core`, `src/storage` y `src/ui`.
- `docs/skills/04-ux-ui-senior.md`: Diseño sobrio, moderno y profesional: interfaz basada en Qt Widgets con tonos azul oscuro y superficies limpias; ausencia de destellos artificiales o colores discordantes; jerarquía visual clara y mensajes de error comprensibles para usuarios finales.
- `docs/skills/05-accesibilidad-y-senior-mode.md`: Enfoque accesible universal e integración con la «Interfaz más fácil de usar»: controles con áreas táctiles ampliadas (mínimo 48px para entradas y botones, 55px para barras), tipografía base en 14pt (escalable a 18pt), foco visible con alto contraste WCAG 2.2 nivel AA, navegación por teclado completa (`Tab`, flechas, atajos) y nombres accesibles (`setAccessibleName`) para lectores de pantalla.
- `docs/skills/06-rendimiento-y-ram.md`: Eficiencia de ejecución y gestión de memoria: latencia de cambio de pestañas < 16 ms; contención estricta de memoria mediante pestañas diferidas (`deferredUrl`) que evitan inicializar múltiples instancias de Chromium simultáneamente; consumo base en reposo inferior a 150 MB RAM; cero procesos zombis auxiliares de WebEngine.
- `docs/skills/07-seguridad-y-privacidad.md`: Filosofía local-first estricta (decisión Q-001): cero telemetría remota, cero consultas de red para comprobación de estados; aislamiento de datos de navegación en SQLite local (`browser.sqlite`); no persistir visitas fallidas en el historial del usuario.
- `docs/skills/08-observabilidad-calidad-linters.md`: Diagnóstico legible y estructurado; evidencia auditable con comandos reproducibles y códigos de salida explícitos; verificación de formateo estricto con Clang-Format.
- `docs/skills/09-prompt-builder-y-documentacion.md`: Separación estricta de fases del ciclo de vida (PROPOSE diseña la solución completa sin ampliar alcance y sin modificar código de producción; APPLY implementa únicamente el incremento del pack tras aprobación humana).
- `docs/skills/11-assets-svg-quality.md`: Calidad de iconografía vectorial SVG, respetando proporciones, alineación óptica y contrastes en temas claro y oscuro.
- `docs/UX-COPY-PUBLICO.md`: Prohibición terminante de exponer términos de ingeniería («Chromium», «WebEngine», «milestone», «MV3», «sandboxing», «renderProcessTerminated», «Senior Mode», «Modo Senior», «Prompt Builder», «OpenSpec», «DAG») en la interfaz visible; empleo exclusivo de «Interfaz más fácil de usar», «No se pudo cargar la página», «Reintentar», «Ir al inicio» y «Cargar pestañas cuando las necesites».
- `docs/UX-UI-SENIOR-CONTRATOS.md`: Especificación de geometría de controles, estados de interacción y accesibilidad.
- `docs/PROPOSE-APPLY-REVIEW-CONTRACT.md`: Contrato de gobernanza de fases; verificación reproducible y respeto absoluto a las restricciones de escritura en allowed paths.
- `/resultado/01-vigilio-home.png`, `/resultado/03-vigilio-settings-senior-mode.png` y `/resultado/06-logo-instalador-oficial.png`: Referencias visuales de la interfaz de usuario objetivo y del logotipo oficial del producto.

## Invariants, reusable inputs and risks

- **Invariantes arquitectónicas**:
  1. Estado vacío (`empty`): una nueva pestaña no crea vistas web ni consume procesos Chromium; se limita a renderizar el widget local `newHomePage()`.
  2. Estado de error (`error`): ante `loadFinished(false)` o desconexión del motor web, la vista se sustituye por un contenedor nativo accesible en Qt Widgets con botones "Reintentar" e "Ir al inicio".
  3. Lenguaje público de producto: estricta proscripción de nombres técnicos de ingeniería en textos visibles de usuario.
  4. «Interfaz más fácil de usar» (Senior Mode): los campos de entrada y botones interactivos deben cumplir `minimumHeight >= 48px` y barras `minimumHeight >= 55px` cuando la opción esté activada.
  5. Carga diferida de pestañas: en sesiones restauradas, solo la pestaña activa inicializa una instancia viva de `QWebEngineView`, manteniendo las demás en reposo bajo la propiedad `deferredUrl`.
  6. Filosofía local-first: cero peticiones de red para comprobación de estados o reportes de caídas.
- **Material reutilizable inspeccionado**:
  - `apps/vigilio-browser/src/core/WebEngineVersionValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/core/QtDependencyValidator.h` y `.cpp`
  - `apps/vigilio-browser/src/ui/BrowserWindow.h` y `.cpp`
  - `apps/vigilio-browser/src/app.cpp`
  - `apps/vigilio-browser/CMakeLists.txt`
  - `apps/vigilio-browser/scripts/check.sh`
- **Riesgos y mitigaciones**:
  - *Riesgo*: Caída súbita del proceso de renderizado Chromium (`renderProcessTerminated`) provocando un área de visualización en blanco.
    *Mitigación*: Captura de señales en `QWebEnginePage` y despliegue inmediato del contenedor de error nativo en Qt Widgets con botón de reintento.
  - *Riesgo*: Saturación de memoria RAM al restaurar sesiones con decenas de pestañas abiertas.
    *Mitigación*: Retención estricta del patrón de pestañas diferidas (`deferredUrl`) que previene la creación de instancias WebEngine hasta su activación directa por el usuario.

## Checklist evidence and handoff

- **Evidencia de calidad y gobernanza**:
  - Compilación local: Ninja configurado con éxito (`CMakeLists.txt`, C++20, `-Werror`).
  - Suite de pruebas automatizadas: `apps/vigilio-browser/scripts/check.sh` ejecutado con éxito total (5/5 tests pasados en 0.14s, exit code 0).
  - Formato de código: Clang-Format verificado sin advertencias ni diferencias en todos los archivos de `src/` y `tests/` (exit code 0).
  - Validación de referencias OpenSpec: `vigilio-prompt validate-references` superó 15/15 comprobaciones canónicas con estado `valid: true`.
  - Verificación de lockfile: `verify-lock` confirmó 13/13 comprobaciones exitosas para el lockfile `0043-module.foundations.task0015.propose.lock.json`.
  - Registro de decisiones: `validate-decisions` confirmó que ninguna decisión bloquea foundations ni webengine.
- **Handoff**:
  - La propuesta para `module.foundations.task0015` queda completa, coherente y lista en `openspec/changes/module-foundations-task0015/` para revisión y aprobación humana.
  - Siguiente paso en el flujo: revisión y aprobación humana para habilitar la fase APPLY.
