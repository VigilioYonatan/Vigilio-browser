# Project Context — REVIEW

Pack: `module.mcp.task0747`
Change: `none`

## Phase instruction
Review the implementation against stable rules, OpenSpec artifacts, security, observability, tests, migrations, contracts and operational evidence. Report concrete gaps; do not invent passing evidence.

## Skills
- none declared

## Execution dependencies
- module.mcp.task0746

## Target profile
- none declared; generic pack

## Allowed paths
- apps/vigilio-browser/src/**
- apps/vigilio-browser/tests/**
- apps/vigilio-browser/assets/**
- apps/vigilio-browser/CMakeLists.txt
- docs/rules/01-modules/mcp/micro/0747-list-tabs-tool/task.md

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

## Source: docs/rules/01-modules/mcp/README.md

# MCP y agentes — bounded context
**Estado**: TARGET salvo funcionalidades explícitamente existentes en starter. **Owner canónico**: `docs/rules/01-modules/mcp`. **Propósito**: read tabs/page; navigate; click/type after permission. **Actores**: usuario local, Modo Senior/Normal/Experto, y agente MCP solo cuando el módulo lo permita. **Responsabilidades propias**: isolated stdio bridge + authenticated IPC. **No posee**: cloud, login o sincronización Vigilio, telemetría remota ni permiso global para acceder a datos de otros módulos. **Contrato público**: interfaz C++ tipada entre capa de dominio y Qt UI, sin lógica del negocio incrustada en widgets. **Dependencias**: constitution.md, security.md, testing.md y contrato de UI común. **Integración**: publicar comandos a CommandRegistry cuando aplique, persistir solo con repositorios locales autorizados.

**Funciones objetivo**: read tabs/page; navigate; click/type after permission. **Criterio de aceptación**: Disabled by default; no agent process without opt-in. **Fuera de scope inicial**: cualquier dependencia no implementada o no validada debe quedar GAP y con tests NOT_EXECUTED.


## Source: docs/rules/01-modules/mcp/design.md

# Diseño de MCP y agentes
## Límites y capas
El módulo **mcp** expone modelos y comandos sin widgets. Controller transfiere estado desde servicios `isolated stdio bridge + authenticated IPC` hacia UI Qt; persistencia usa SQLite/QStandardPaths solo si procede y los secretos no se guardan en texto plano. Para cada cambio de estado: validar argumentos y permisos, emitir progreso, actualizar modelo en hilo apropiado, persistir de forma atómica, notificar UI y permitir recuperación. No bloquear el hilo UI, no iniciar componentes pesados cuando la función no está abierta.
## Dependencias y restricciones
read tabs/page; navigate; click/type after permission. La capa de interfaz no llama directamente a SQL, WebEngine o IPC sin adapter. Se adoptan guardas cuando se cierra pestaña/ventana durante operación; referencias Qt weak (`QPointer`) cuando proceda; cancelación explícita y disconnect limpio.
## Concurrencia y seguridad
El estado del modelo debe tener propietario claro; comandos serializados o transacciones según el agregado. Proteger del contenido de web no confiable y del agente sin permisos. Nunca filtrar datos privados entre perfiles. Persistir identificadores estables versionados. Eventos no propagados a nube.
## Evolución
Fase 1 contrato + estado visual y tests; fase 2 integración real; fase 3 error/recovery/a11y/security; fase 4 performance y documentación final. Revertir mediante feature flag local y restaurar schema/archivos desde backup validado.


## Source: docs/rules/01-modules/mcp/screens.md

# Pantallas y controles — MCP y agentes
**Entrada**: menú contextual, barra de navegación, barra lateral o `Herramientas` según la capacidad. Icono Lucide SVG y etiqueta visible; en modo Experto comandos Ctrl+K; en Modo Senior entrada en menú simplificado accesible cuando la función es común. **Layout**: superficie plana con bordes sutiles, jerarquía título→explicación→control primario→secundarios; no glow ni gradientes en ventanas internas. **Operación principal**: read tabs/page; navigate; click/type after permission.

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

**Accesibilidad**: Qt AccessibleName/Description, orden tab lógico, contrast AA, shortcuts remapeables, manejo de zoom DPI, objetivos Senior 44–48 px, texto 18px, labels explícitas, reduced motion y screenreader. **Navegación y teclado**: Escape cancela o cierra diálogo no destructivo; Enter confirma control enfocado; no trap de foco. **Test IDs**: `vigilio.mcp.open`, `.primary`, `.cancel`, `.error`, `.status`; no se exponen como URL pública.


## Source: docs/rules/01-modules/mcp/business.md

# Caso principal — MCP y agentes
**Acción**: read tabs/page; navigate; click/type after permission.
**Precondiciones**: el usuario puede acceder al módulo y su perfil local está disponible. Para páginas remotas, verificar existencia/vigencia de pestaña, origen y permisos. Para operación del agente, require explicit client capability grant.
**Flujo**: (1) usuario invoca desde UI/command palette (2) controlador valida tipo/estado/permiso y límites (3) presenta preview/confirmación si afecta datos (4) adaptador ejecuta async sin bloquear UI (5) emite estado y persiste atómicamente cuando es necesario (6) UI anuncia resultado y ruta de recuperación.
**Invariantes**: sin nube/login, sin fuga interperfil, no mutación silenciosa, idempotency keys cuando el commit sea ambiguo. **Conflictos**: pestaña cerrada en paralelo se descarta de forma segura; cambio de perfil invalida comandos antiguos; doble activación se deduplica por id; datos obsoletos se revalidan justo antes de commit. **Errores**: entrada vacía/malformada, duplicados, permisos denegados, disco lleno, timeout antes/después de commit, reinicio inesperado, archivo grande, problema de renderer. **Recuperación**: cancelar tareas sin eliminar estado anterior; reintentar solo si acción idempotente; preservar backup y mostrar error accionable. **Audit**: guardar evento local sin payload sensible únicamente cuando fue una acción protegida. **Postcondición**: Disabled by default; no agent process without opt-in.


## Source: docs/rules/01-modules/mcp/tests.md

# Matriz de prueba — MCP y agentes
Evidencia inicial: **NOT_EXECUTED** para este módulo completo salvo fuente histórica verificable. No proclamar PASS por existencia de mocks.
| Tipo | Caso obligatorio | Gate |
|---|---|---|
| Unit | argumentos vacíos, malformed, duplicate, state machine y lifecycle | PR |
| Integration | adapter real, persistencia, error injection, cancellation, rollback | PR de módulo |
| Component Qt | hover/focus/keyboard/screenreader, error/status y mode Senior | UI PR |
| Contract | API/IPC o repositorio cuando aplique, versiones y tipos | PR |
| Security | forbidden origin/profile, secrets, injection, path traversal | PR/release |
| E2E crítico | Disabled by default; no agent process without opt-in a través de Qt WebEngine real si cruza capas | release; smoke selectivo |
| Performance | comparar CPU/RAM/latencia con baseline, procesos totales | nightly/release |
| Recovery | kill/restart, disco lleno, timeout ambiguo, restore/deduplicate | release |

E2E pesados serializados, servidor fixture local, sin red pública, profile temp aislado, tests con esperas por estado, no sleeps, teardown de subprocesos. Registrar resultados PASS/FAIL/BLOCKED/NOT_EXECUTED por caso con comando/hardware/fecha/log hash.


## Source: docs/rules/01-modules/mcp/operations.md

# Operación / recuperación — MCP y agentes
**Dependencias**: isolated stdio bridge + authenticated IPC. **Modo degradado**: conservar navegación fundamental cuando la capacidad opcional falla y desactivar solo ese módulo con explicación visible. **Diagnóstico**: códigos de error `VIG-MCP-*`; contador local de fallos con usuario autorizado, sin datos de páginas. **Límites de capacidad**: budget de memoria por subcomponente configurable en bench, cancelación al cerrar la UI, backpressure en importaciones y tareas recurrentes. **Alertas**: local; usuarios podrán consultar y borrar registros. **Runbook**: reproducir, preservar copia antes de tocar datos, desactivar capacidad afectada, validar permisos, corregir root cause, tests específicos, rollback desde backup verificado. **SLO**: definir tras medición, nunca inventado. **Owner**: ingeniero de módulo; **security incident**: aislar capacidad, revocar permisos MCP si aplica; nunca cargar dump sensible a terceros automáticamente.


## Source: docs/rules/01-modules/mcp/micro/0747-list-tabs-tool/task.md

# 0747 — Definir el comportamiento y las invariantes: list tabs tool

- **Pack:** `module.mcp.task0747`
- **Módulo propietario:** `mcp`
- **Capacidad concreta:** list tabs tool
- **Etapa:** domain
- **Estado:** TARGET; NOT_EXECUTED.
- **Dependencias:** `module.mcp.task0746`.

## Resultado verificable
Especificar las reglas de list tabs tool, los límites de mcp, entradas/salidas, ciclo de vida, datos y políticas de privacidad. Esta tarea modifica exclusivamente contratos y pruebas de especificación.

## Alcance y secuencia
1. Leer los contratos fuente del módulo y las decisiones locales; no copiar la lógica de Vigilio Code ni añadir backend remoto.
2. Revisar el código existente antes de cambiar nombres, interfaces o rutas y definir el delta exacto de esta capacidad.
3. Definir el comportamiento y las invariantes de **list tabs tool**, aislando el alcance de este pack frente a otros cambios.
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
- **Commit sugerido:** `docs(mcp): domain list-tabs-tool`
- No realizar commit si tests obligatorios fallan. No modificar `generated/` ni código ajeno a la capacidad.

## Criterios de aceptación
- La función no contradice la política local-first; datos y permisos permanecen aislados.
- El comportamiento concreto de list tabs tool se puede demostrar con una prueba o inspección reproducible.
- No existe regresión conocida sin ticket, y toda limitación no validada queda explicitada.

## Contrato obligatorio de PROPOSE — solución completa, ejecución incremental
**Resultado exigido:** proponer la solución integral de **list tabs tool** para la etapa **domain** (modelo de dominio, reglas e invariantes), pero limitar APPLY al único incremento de este pack. No reemplazar esa solución con una descripción genérica de metodología.

1. **Exploración comprobable**: listar los archivos y APIs verdaderamente existentes en `apps/vigilio-browser`, el contrato fuente del módulo y el estado CURRENT/TARGET/GAP. No inventar símbolos Qt o implementaciones.
2. **Diseño técnico específico**: describir qué entidades, métodos, estados y responsabilidades exactos se añadirán o modificarán para **list tabs tool**; definir precondiciones, postcondiciones, invariantes y qué módulo es dueño de cada dato. El agente no hereda acceso al perfil privado; consentimiento por cliente/acción/origen, IPC local autenticado y veto a operaciones sensibles.
3. **Contrato funcional**: definir actor/invocación, entrada/salida y validación, permiso, transiciones observables, persistencia si aplica, deshacer/cancelar, conflicto concurrente, errores claros y recuperación. Especificar explícitamente qué es N/A.
4. **Contrato de UI**: si esta etapa toca interfaz, detallar pantalla/menú, posición, label en español, icono SVG, dimensiones/tokens, navegación teclado, estados `idle/focus/hover/disabled/loading/empty/success/error`, Modo Senior y lector de pantalla. Si no toca UI, declarar **sin cambio visual** y verificar contratos existentes.
5. **Integración y seguridad**: escribir límite de confianza, perfil/incógnito, interacción Qt WebEngine (solo API disponible), datos en SQLite/archivo, concurrencia, presupuestos de memoria, aislamiento y privacidad local. No introducir cuentas Vigilio ni servicios cloud.
6. **Plan de APPLY mínimo**: detallar modificación por archivo realmente presente o archivo nuevo justificado, en orden determinista. La salida esperada de este incremento será **clases, estados, ownership, errores tipados, límites transaccionales y tests unitarios sin Qt WebEngine**. Dejar los pasos siguientes fuera de este APPLY aunque aparezcan en el diseño completo.
7. **Matriz de tests trazables**: asociar cada criterio a prueba concreta, fixture, comando de ejecución, resultado esperado y fallo negativo; incluir **caso válido, inválido, repetición, conflicto y límites de estado**. Distinguir unitario, integración, Qt UI, E2E crítico, accesibilidad, seguridad y benchmark como APPLY / LATER / NOT_APPLICABLE. E2E pesado serial, sin red externa, con procesos y perfiles temporales limpiados.
8. **Riesgos/decisiones**: enumerar bloqueos reales, alternativas y trade-offs. Una decisión humana no aprobada permanece OPEN; si no puede compilarse por dependencia ausente, `NOT_EXECUTED`, nunca PASS ficticio.

**Formato de PROPOSE obligatorio**: `Contexto y evidencia`, `Alcance exacto`, `Diseño completo de la capacidad`, `Archivos afectados`, `Contrato UX/UI`, `Reglas y fallos`, `Seguridad y privacidad`, `Matriz de pruebas`, `Pasos APPLY`, `Definición de terminado`, `Dependencias y riesgos`, `Commit propuesto`. Cada apartado debe contener información de **list tabs tool** y no solo frases reutilizadas.

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
