# Project Context — PROPOSE

Pack: `module.installer.task0853`
Change: `none`

## Phase instruction
Create or refine the root OpenSpec change under `openspec/changes/<change-id>/` using the supplied stable context. PROPOSE is an authoring and feasibility phase: missing backend modules, migrations, OpenAPI, UI components or executable test evidence are expected implementation gaps and MUST become explicit tasks, risks and acceptance gates; their absence alone is not a reason to block PROPOSE. Do not implement application code or silently override permanent rules.

If `Change` is `none`, derive the deterministic change id from the pack by replacing dots with hyphens. Produce substantive `proposal.md`, `design.md` and `tasks.md` in the project root OpenSpec directory. Human approval happens after the proposal is ready and before APPLY.

Return BLOCKED only when a proposal cannot be made safely because required canonical authority is absent/contradictory, a material human decision is necessary to choose the design, the declared repository/data/cloud source cannot be inspected with approved read-only access, or the proposed scope would escape allowed paths. Do not demand APPLY evidence during PROPOSE.

## Skills
- none declared

## Execution dependencies
- module.installer.task0852

## Target profile
- none declared; generic pack

## Allowed paths
- apps/vigilio-browser/src/**
- apps/vigilio-browser/tests/**
- apps/vigilio-browser/assets/**
- apps/vigilio-browser/CMakeLists.txt
- docs/rules/01-modules/installer/micro/0853-windows-component-selection/task.md
- openspec/changes/module-installer-task0853/**

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

## Source: docs/rules/01-modules/installer/README.md

# Instalación/desinstalación — bounded context
**Estado**: TARGET salvo funcionalidades explícitamente existentes en starter. **Owner canónico**: `docs/rules/01-modules/installer`. **Propósito**: Win exe, Linux deb/AppImage, macOS dmg, first-run, preserve/export/delete. **Actores**: usuario local, Modo Senior/Normal/Experto, y agente MCP solo cuando el módulo lo permita. **Responsabilidades propias**: platform packaging. **No posee**: cloud, login o sincronización Vigilio, telemetría remota ni permiso global para acceder a datos de otros módulos. **Contrato público**: interfaz C++ tipada entre capa de dominio y Qt UI, sin lógica del negocio incrustada en widgets. **Dependencias**: constitution.md, security.md, testing.md y contrato de UI común. **Integración**: publicar comandos a CommandRegistry cuando aplique, persistir solo con repositorios locales autorizados.

**Funciones objetivo**: Win exe, Linux deb/AppImage, macOS dmg, first-run, preserve/export/delete. **Criterio de aceptación**: No silent profile deletion; signed release subject to keys. **Fuera de scope inicial**: cualquier dependencia no implementada o no validada debe quedar GAP y con tests NOT_EXECUTED.


## Source: docs/rules/01-modules/installer/design.md

# Diseño de Instalación/desinstalación
## Límites y capas
El módulo **installer** expone modelos y comandos sin widgets. Controller transfiere estado desde servicios `platform packaging` hacia UI Qt; persistencia usa SQLite/QStandardPaths solo si procede y los secretos no se guardan en texto plano. Para cada cambio de estado: validar argumentos y permisos, emitir progreso, actualizar modelo en hilo apropiado, persistir de forma atómica, notificar UI y permitir recuperación. No bloquear el hilo UI, no iniciar componentes pesados cuando la función no está abierta.
## Dependencias y restricciones
Win exe, Linux deb/AppImage, macOS dmg, first-run, preserve/export/delete. La capa de interfaz no llama directamente a SQL, WebEngine o IPC sin adapter. Se adoptan guardas cuando se cierra pestaña/ventana durante operación; referencias Qt weak (`QPointer`) cuando proceda; cancelación explícita y disconnect limpio.
## Concurrencia y seguridad
El estado del modelo debe tener propietario claro; comandos serializados o transacciones según el agregado. Proteger del contenido de web no confiable y del agente sin permisos. Nunca filtrar datos privados entre perfiles. Persistir identificadores estables versionados. Eventos no propagados a nube.
## Evolución
Fase 1 contrato + estado visual y tests; fase 2 integración real; fase 3 error/recovery/a11y/security; fase 4 performance y documentación final. Revertir mediante feature flag local y restaurar schema/archivos desde backup validado.


## Source: docs/rules/01-modules/installer/screens.md

# Pantallas y controles — Instalación/desinstalación
**Entrada**: menú contextual, barra de navegación, barra lateral o `Herramientas` según la capacidad. Icono Lucide SVG y etiqueta visible; en modo Experto comandos Ctrl+K; en Modo Senior entrada en menú simplificado accesible cuando la función es común. **Layout**: superficie plana con bordes sutiles, jerarquía título→explicación→control primario→secundarios; no glow ni gradientes en ventanas internas. **Operación principal**: Win exe, Linux deb/AppImage, macOS dmg, first-run, preserve/export/delete.

| Estado | Especificación |
|---|---|
| idle | mostrar controles aplicables con permiso efectivo y atajos visibles |
| hover/focus | resaltado moderado y focus ring perceptible para teclado |
| loading | indicador acotado, botón deshabilitado si duplicación causaría corrupción |
| empty | explicar ausencia de contenido y cómo empezar |
| success | notificar resultado, ubicación del dato y deshacer cuando posible |
| error | error humano, código diagnóstico redactado, recuperar/reintentar si seguro |
| forbidden | explicar permiso y ajuste relevante sin conceder acceso automático |
| offline/degraded | operar local cuando sea posible y declarar limitación de web/external |

**Accesibilidad**: Qt AccessibleName/Description, orden tab lógico, contrast AA, shortcuts remapeables, manejo de zoom DPI, objetivos Senior 44–48 px, texto 18px, labels explícitas, reduced motion y screenreader. **Navegación y teclado**: Escape cancela o cierra diálogo no destructivo; Enter confirma control enfocado; no trap de foco. **Test IDs**: `vigilio.installer.open`, `.primary`, `.cancel`, `.error`, `.status`; no se exponen como URL pública.


## Source: docs/rules/01-modules/installer/business.md

# Caso principal — Instalación/desinstalación
**Acción**: Win exe, Linux deb/AppImage, macOS dmg, first-run, preserve/export/delete.
**Precondiciones**: el usuario puede acceder al módulo y su perfil local está disponible. Para páginas remotas, verificar existencia/vigencia de pestaña, origen y permisos. Para operación del agente, require explicit client capability grant.
**Flujo**: (1) usuario invoca desde UI/command palette (2) controlador valida tipo/estado/permiso y límites (3) presenta preview/confirmación si afecta datos (4) adaptador ejecuta async sin bloquear UI (5) emite estado y persiste atómicamente cuando es necesario (6) UI anuncia resultado y ruta de recuperación.
**Invariantes**: sin nube/login, sin fuga interperfil, no mutación silenciosa, idempotency keys cuando el commit sea ambiguo. **Conflictos**: pestaña cerrada en paralelo se descarta de forma segura; cambio de perfil invalida comandos antiguos; doble activación se deduplica por id; datos obsoletos se revalidan justo antes de commit. **Errores**: entrada vacía/malformada, duplicados, permisos denegados, disco lleno, timeout antes/después de commit, reinicio inesperado, archivo grande, problema de renderer. **Recuperación**: cancelar tareas sin eliminar estado anterior; reintentar solo si acción idempotente; preservar backup y mostrar error accionable. **Audit**: guardar evento local sin payload sensible únicamente cuando fue una acción protegida. **Postcondición**: No silent profile deletion; signed release subject to keys.


## Source: docs/rules/01-modules/installer/tests.md

# Matriz de prueba — Instalación/desinstalación
Evidencia inicial: **NOT_EXECUTED** para este módulo completo salvo fuente histórica verificable. No proclamar PASS por existencia de mocks.
| Tipo | Caso obligatorio | Gate |
|---|---|---|
| Unit | argumentos vacíos, malformed, duplicate, state machine y lifecycle | PR |
| Integration | adapter real, persistencia, error injection, cancellation, rollback | PR de módulo |
| Component Qt | hover/focus/keyboard/screenreader, error/status y mode Senior | UI PR |
| Contract | API/IPC o repositorio cuando aplique, versiones y tipos | PR |
| Security | forbidden origin/profile, secrets, injection, path traversal | PR/release |
| E2E crítico | No silent profile deletion; signed release subject to keys a través de Qt WebEngine real si cruza capas | release; smoke selectivo |
| Performance | comparar CPU/RAM/latencia con baseline, procesos totales | nightly/release |
| Recovery | kill/restart, disco lleno, timeout ambiguo, restore/deduplicate | release |

E2E pesados serializados, servidor fixture local, sin red pública, profile temp aislado, tests con esperas por estado, no sleeps, teardown de subprocesos. Registrar resultados PASS/FAIL/BLOCKED/NOT_EXECUTED por caso con comando/hardware/fecha/log hash.


## Source: docs/rules/01-modules/installer/operations.md

# Operación / recuperación — Instalación/desinstalación
**Dependencias**: platform packaging. **Modo degradado**: conservar navegación fundamental cuando la capacidad opcional falla y desactivar solo ese módulo con explicación visible. **Diagnóstico**: códigos de error `VIG-INSTALLER-*`; contador local de fallos con usuario autorizado, sin datos de páginas. **Límites de capacidad**: budget de memoria por subcomponente configurable en bench, cancelación al cerrar la UI, backpressure en importaciones y tareas recurrentes. **Alertas**: local; usuarios podrán consultar y borrar registros. **Runbook**: reproducir, preservar copia antes de tocar datos, desactivar capacidad afectada, validar permisos, corregir root cause, tests específicos, rollback desde backup verificado. **SLO**: definir tras medición, nunca inventado. **Owner**: ingeniero de módulo; **security incident**: aislar capacidad, revocar permisos MCP si aplica; nunca cargar dump sensible a terceros automáticamente.


## Source: docs/rules/01-modules/installer/micro/0853-windows-component-selection/task.md

# 0853 — Probar casos positivos, negativos y recuperación: Windows component selection

- **Pack:** `module.installer.task0853`
- **Módulo propietario:** `installer`
- **Capacidad concreta:** Windows component selection
- **Etapa:** tests
- **Estado:** TARGET; NOT_EXECUTED.
- **Dependencias:** `module.installer.task0852`.

## Resultado verificable
Crear tests focalizados para Windows component selection: caso normal, entrada inválida, concurrencia/cancelación cuando aplique, recuperación y regresión sin iniciar Chromium salvo necesidad.

## Alcance y secuencia
1. Leer los contratos fuente del módulo y las decisiones locales; no copiar la lógica de Vigilio Code ni añadir backend remoto.
2. Revisar el código existente antes de cambiar nombres, interfaces o rutas y definir el delta exacto de esta capacidad.
3. Probar casos positivos, negativos y recuperación de **Windows component selection**, aislando el alcance de este pack frente a otros cambios.
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
- **Commit sugerido:** `test(installer): tests windows-component-selection`
- No realizar commit si tests obligatorios fallan. No modificar `generated/` ni código ajeno a la capacidad.

## Criterios de aceptación
- La función no contradice la política local-first; datos y permisos permanecen aislados.
- El comportamiento concreto de Windows component selection se puede demostrar con una prueba o inspección reproducible.
- No existe regresión conocida sin ticket, y toda limitación no validada queda explicitada.

## Contrato obligatorio de PROPOSE — solución completa, ejecución incremental
**Resultado exigido:** proponer la solución integral de **Windows component selection** para la etapa **tests** (pruebas automatizadas deterministas), pero limitar APPLY al único incremento de este pack. No reemplazar esa solución con una descripción genérica de metodología.

1. **Exploración comprobable**: listar los archivos y APIs verdaderamente existentes en `apps/vigilio-browser`, el contrato fuente del módulo y el estado CURRENT/TARGET/GAP. No inventar símbolos Qt o implementaciones.
2. **Diseño técnico específico**: describir qué entidades, métodos, estados y responsabilidades exactos se añadirán o modificarán para **Windows component selection**; definir precondiciones, postcondiciones, invariantes y qué módulo es dueño de cada dato. No borrar perfil en desinstalación por defecto; copia/exportación voluntaria, rollback si falla y firma/verificación por plataforma.
3. **Contrato funcional**: definir actor/invocación, entrada/salida y validación, permiso, transiciones observables, persistencia si aplica, deshacer/cancelar, conflicto concurrente, errores claros y recuperación. Especificar explícitamente qué es N/A.
4. **Contrato de UI**: si esta etapa toca interfaz, detallar pantalla/menú, posición, label en español, icono SVG, dimensiones/tokens, navegación teclado, estados `idle/focus/hover/disabled/loading/empty/success/error`, Modo Senior y lector de pantalla. Si no toca UI, declarar **sin cambio visual** y verificar contratos existentes.
5. **Integración y seguridad**: escribir límite de confianza, perfil/incógnito, interacción Qt WebEngine (solo API disponible), datos en SQLite/archivo, concurrencia, presupuestos de memoria, aislamiento y privacidad local. No introducir cuentas Vigilio ni servicios cloud.
6. **Plan de APPLY mínimo**: detallar modificación por archivo realmente presente o archivo nuevo justificado, en orden determinista. La salida esperada de este incremento será **tests ejecutables con fixtures sintéticos, aserciones verificables y aislamiento de datos; E2E solo si afecta un recorrido crítico**. Dejar los pasos siguientes fuera de este APPLY aunque aparezcan en el diseño completo.
7. **Matriz de tests trazables**: asociar cada criterio a prueba concreta, fixture, comando de ejecución, resultado esperado y fallo negativo; incluir **casos feliz, inválido, cancelación, fallo inyectado y ausencia de regresión**. Distinguir unitario, integración, Qt UI, E2E crítico, accesibilidad, seguridad y benchmark como APPLY / LATER / NOT_APPLICABLE. E2E pesado serial, sin red externa, con procesos y perfiles temporales limpiados.
8. **Riesgos/decisiones**: enumerar bloqueos reales, alternativas y trade-offs. Una decisión humana no aprobada permanece OPEN; si no puede compilarse por dependencia ausente, `NOT_EXECUTED`, nunca PASS ficticio.

**Formato de PROPOSE obligatorio**: `Contexto y evidencia`, `Alcance exacto`, `Diseño completo de la capacidad`, `Archivos afectados`, `Contrato UX/UI`, `Reglas y fallos`, `Seguridad y privacidad`, `Matriz de pruebas`, `Pasos APPLY`, `Definición de terminado`, `Dependencias y riesgos`, `Commit propuesto`. Cada apartado debe contener información de **Windows component selection** y no solo frases reutilizadas.

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
