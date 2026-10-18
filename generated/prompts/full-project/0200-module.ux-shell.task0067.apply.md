# Project Context — APPLY

Pack: `module.ux-shell.task0067`
Change: `none`

## Phase instruction
Implement the active OpenSpec change. Stay within allowed paths, preserve invariants, use declared skills, and run required quality gates.

## Skills
- none declared

## Execution dependencies
- module.ux-shell.task0066

## Target profile
- none declared; generic pack

## Allowed paths
- apps/vigilio-browser/src/**
- apps/vigilio-browser/tests/**
- apps/vigilio-browser/assets/**
- apps/vigilio-browser/CMakeLists.txt
- docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md

## Forbidden paths
- **/generated/**
- **/openspec/**

## Required quality gates
- cmake-configure
- cmake-build
- ctest
- code-review
- documentation-sync

## Gates required before execution activation
- none declared

## Activation requirements
- none declared

## Stable project context

## Source: docs/rules/01-modules/ux-shell/README.md

# ux-shell — Responsabilidad y alcance

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/design.md

# ux-shell — Arquitectura y dependencias

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/screens.md

# ux-shell — Pantallas y estados

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/business.md

# ux-shell — Reglas, operaciones y errores

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/tests.md

# ux-shell — Matriz de pruebas

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/operations.md

# ux-shell — Observabilidad y recuperación

**Estado:** TARGET; los artefactos del prototipo son CURRENT solo si están comprobados.

## Capacidades del módulo
main window layout, title bar integration, tab strip density, toolbar layout, omnibox focus, navigation buttons, sidebar collapsed, sidebar expanded, responsive window minimum, context menu styling, modal focus restore, toast notifications, tooltips and labels, empty state views, loading and error states, keyboard shortcuts, high contrast appearance, reduced motion, theme switching, typography scaling, icon asset consistency, new tab page, wallpaper selection, window resize persistence.

## Límite
Este módulo es propietario de las capacidades enumeradas y no modifica identidades de otros perfiles, permisos ajenos o políticas de nube. UI nativa Qt Widgets, núcleo C++20, persistencia SQLite local según necesidad.

## Contratos
Definir entradas, salida, estados, fallos, reintentos y recuperación para cada capacidad en sus microtareas; evitar operaciones bloqueantes en UI. Eventos relevantes se validan por CTest, con E2E serial cuando aplique.

## UX/UI y Senior Mode
Diseño sobrio, Inter/sistema y Lucide SVG; iconos etiquetados, contraste WCAG aplicable, atajos, foco y tamaños grandes en Senior Mode. No mostrar cloud o login Vigilio.

## Seguridad y pruebas
Revisar límites entre procesos WebEngine, rutas locales y acceso de agentes. Registrar solo metadatos mínimos, sin URLs completas en logs. Pruebas unitarias e integración frecuentes; ASan/UBSan donde sea viable.


## Source: docs/rules/01-modules/ux-shell/micro/0067-title-bar-integration/task.md

# 0067 — Probar casos positivos, negativos y recuperación: title bar integration

- **Pack:** `module.ux-shell.task0067`
- **Módulo propietario:** `ux-shell`
- **Capacidad concreta:** title bar integration
- **Etapa:** tests
- **Estado:** TARGET; NOT_EXECUTED.
- **Dependencias:** `module.ux-shell.task0066`.

## Resultado verificable
Crear tests focalizados para title bar integration: caso normal, entrada inválida, concurrencia/cancelación cuando aplique, recuperación y regresión sin iniciar Chromium salvo necesidad.

## Alcance y secuencia
1. Leer los contratos fuente del módulo y las decisiones locales; no copiar la lógica de Vigilio Code ni añadir backend remoto.
2. Revisar el código existente antes de cambiar nombres, interfaces o rutas y definir el delta exacto de esta capacidad.
3. Probar casos positivos, negativos y recuperación de **title bar integration**, aislando el alcance de este pack frente a otros cambios.
4. Verificar permisos, recuperación y límites de RAM; no crear procesos WebEngine o MCP innecesarios.
5. Actualizar la documentación propietaria si cambia una decisión observable.

## UX/UI Senior
- Integración visual: componentes Qt existentes, Inter/sistema, iconos SVG Lucide, fondos discretos y sin brillo artificial.
- Interacción: foco visible, mensajes comprensibles, loading/no disponible/error, teclado y lector de pantalla.
- Modo Senior: área de clic amplia, texto escalable y acciones principales claramente etiquetadas; no ocultar permisos.
- Para tareas sin interfaz, documentar por qué no hay cambio visual directo.

## Condiciones y fallos
- No persistir datos de incógnito en historial ordinario; no conectar a cloud ni solicitar login Vigilio.
- Considerar entrada vacía/malformada, recurso inexistente, proceso reiniciado, operación cancelada y error de disco cuando aplique.
- Denegar privilegios no autorizados y evitar bloquear el hilo de interfaz.

## Verificación y commit
- Ejecutar ClangFormat y Clang-Tidy sobre archivos C++ modificados, CMake configure/build y CTest aplicable; registrar `NOT_EXECUTED` cuando falte Qt.
- Ejecutar E2E únicamente si el cambio afecta un flujo crítico observable; de uno en uno con fixture local.
- Evidencia exigida: diff pequeño, comandos y códigos de salida, resultado funcional, y revisión de accesibilidad/seguridad.
- **Commit sugerido:** `test(ux-shell): tests title-bar-integration`
- No realizar commit si tests obligatorios fallan. No modificar `generated/` ni código ajeno a la capacidad.

## Criterios de aceptación
- La función no contradice la política local-first; datos y permisos permanecen aislados.
- El comportamiento concreto de title bar integration se puede demostrar con una prueba o inspección reproducible.
- No existe regresión conocida sin ticket, y toda limitación no validada queda explicitada.

## Contrato obligatorio de PROPOSE — solución completa, ejecución incremental
**Resultado exigido:** proponer la solución integral de **title bar integration** para la etapa **tests** (pruebas automatizadas deterministas), pero limitar APPLY al único incremento de este pack. No reemplazar esa solución con una descripción genérica de metodología.

1. **Exploración comprobable**: listar los archivos y APIs verdaderamente existentes en `apps/vigilio-browser`, el contrato fuente del módulo y el estado CURRENT/TARGET/GAP. No inventar símbolos Qt o implementaciones.
2. **Diseño técnico específico**: describir qué entidades, métodos, estados y responsabilidades exactos se añadirán o modificarán para **title bar integration**; definir precondiciones, postcondiciones, invariantes y qué módulo es dueño de cada dato. Una sola fuente de tokens; densidad real de escritorio, no neón; foco visible, información clara y adaptación al Modo Senior.
3. **Contrato funcional**: definir actor/invocación, entrada/salida y validación, permiso, transiciones observables, persistencia si aplica, deshacer/cancelar, conflicto concurrente, errores claros y recuperación. Especificar explícitamente qué es N/A.
4. **Contrato de UI**: si esta etapa toca interfaz, detallar pantalla/menú, posición, label en español, icono SVG, dimensiones/tokens, navegación teclado, estados `idle/focus/hover/disabled/loading/empty/success/error`, Modo Senior y lector de pantalla. Si no toca UI, declarar **sin cambio visual** y verificar contratos existentes.
5. **Integración y seguridad**: escribir límite de confianza, perfil/incógnito, interacción Qt WebEngine (solo API disponible), datos en SQLite/archivo, concurrencia, presupuestos de memoria, aislamiento y privacidad local. No introducir cuentas Vigilio ni servicios cloud.
6. **Plan de APPLY mínimo**: detallar modificación por archivo realmente presente o archivo nuevo justificado, en orden determinista. La salida esperada de este incremento será **tests ejecutables con fixtures sintéticos, aserciones verificables y aislamiento de datos; E2E solo si afecta un recorrido crítico**. Dejar los pasos siguientes fuera de este APPLY aunque aparezcan en el diseño completo.
7. **Matriz de tests trazables**: asociar cada criterio a prueba concreta, fixture, comando de ejecución, resultado esperado y fallo negativo; incluir **casos feliz, inválido, cancelación, fallo inyectado y ausencia de regresión**. Distinguir unitario, integración, Qt UI, E2E crítico, accesibilidad, seguridad y benchmark como APPLY / LATER / NOT_APPLICABLE. E2E pesado serial, sin red externa, con procesos y perfiles temporales limpiados.
8. **Riesgos/decisiones**: enumerar bloqueos reales, alternativas y trade-offs. Una decisión humana no aprobada permanece OPEN; si no puede compilarse por dependencia ausente, `NOT_EXECUTED`, nunca PASS ficticio.

**Formato de PROPOSE obligatorio**: `Contexto y evidencia`, `Alcance exacto`, `Diseño completo de la capacidad`, `Archivos afectados`, `Contrato UX/UI`, `Reglas y fallos`, `Seguridad y privacidad`, `Matriz de pruebas`, `Pasos APPLY`, `Definición de terminado`, `Dependencias y riesgos`, `Commit propuesto`. Cada apartado debe contener información de **title bar integration** y no solo frases reutilizadas.

## Contrato de APPLY y REVIEW
- **APPLY** implementa solo este cambio y sus tests, verifica el diff, usa el código y API real, y no toca el código generado. Una dependencia ausente real debe registrarse, nunca sustituirse por un mock que finja la integración.
- **REVIEW** revisa independientemente cada aceptación y la evidencia: corre `format/lint/build/tests` aplicables, inspecciona interacción visual y permisos cuando corresponda, compara salida real con PROPOSE, rechaza regresiones.
- **Entrega de cada prompt**: resumen del comportamiento añadido, rutas cambiadas, resultados de comandos con exit code, limitaciones, riesgos residuales y **un único commit** descriptivo una vez aprobado; mensaje base: el indicado en esta tarea.
- **No se acepta como completo** un plan que solo diga «implementar siguiendo mejores prácticas». Debe exponer decisiones concretas del cambio y pruebas capaces de fallar.

## Referencias visuales y skills obligatorias
- Si este micro-prompt afecta UI, branding, accesibilidad, Modo Senior, instalador, desinstalador o assets, revisar primero la carpeta raíz `/resultado/` y citar la(s) imagen(es) relevantes en **PROPOSE** y **REVIEW**.
- Revisar también `docs/skills/` y aplicar sus reglas de ingeniería, testing, calidad, rendimiento, seguridad y UX/UI.
- **PROPOSE** debe describir cómo el cambio preserva o acerca el resultado visual objetivo y cómo se integra con Senior Mode.
- **APPLY** debe mantener consistencia con tokens, iconos, espaciados y comportamiento definidos; no introducir estilos ajenos ni apariencia genérica.
- **REVIEW** debe confirmar coherencia con `/resultado/`, accesibilidad aplicable y pruebas relevantes.


## Textos públicos y marca
En toda pantalla visible, aplicar `docs/UX-COPY-PUBLICO.md`. No mostrar «Senior Mode», «Modo Senior», «prompt», «skill» ni etiquetas de ingeniería; usar «Interfaz más fácil de usar». Para instalador e identidad visual, respetar `resultado/06-logo-instalador-oficial.png`. Si el cambio no modifica textos ni interfaz, dejar constancia en PROPOSE.


## OpenSpec context
No OpenSpec artifacts found for this invocation.
