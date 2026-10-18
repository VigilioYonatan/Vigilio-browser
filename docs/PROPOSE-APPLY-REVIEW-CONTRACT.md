# Contrato de ejecución Senior — Prompt Builder Vigilio Browser

**Fuente de verdad**: `rules/answers.md` contiene decisiones humanas; `rules/00-global/` gobierna normas transversales; cada `rules/01-modules/<modulo>/` es owner técnico; `micro/<ID>/task.md` define el incremento exclusivo. Un plan es TARGET, no una funcionalidad implementada.

## PROPOSE: solución completa sin ampliación de alcance
En cada task, PROPOSE explora el repositorio y redacta una solución **implementable para su capacidad**, incluyendo clases/métodos, contratos, flujos, UI/UX, fallos, seguridad, fixtures, pruebas y criterios. Debe enumerar archivos existentes verificados frente a archivos nuevos propuestos; no inventar código ya escrito.

PROPOSE contiene un mapa de la solución completa de esa capacidad, pero solo autoriza **un cambio pequeño** correspondiente al pack actual. Cualquier dependencia no realizada queda explícita y se asigna a otro pack: no la implementa a escondidas.

## APPLY
Comprobar dependencias reales, modificar solo rutas permitidas y documentar cambios. Los errores deben aparecer en tests negativos. No falsear PASS, no desactivar tests ni suprimir sanitizers para pasar un gate. Evitar E2E masivo en PR; tests locales y repetibles.

## REVIEW
Revisar de forma independiente la intención, diffs, integración, seguridad, accesibilidad y evidencia. Un test compilado sin integración Qt no valida el navegador. Resultado por gate: PASS/FAIL/BLOCKED/NOT_EXECUTED/NOT_APPLICABLE. Commit solo después de verificar; proponer mensaje `tipo(modulo): descripción` sin incluir jerga de generación.

## UX/UI de producto
Qt Widgets de escritorio con tipografía Inter/sistema, Lucide SVG, zoom, enfoque por teclado, contraste WCAG 2.2 AA donde aplique, Modo Senior accesible sin retirar permisos, y sin superficies excesivamente luminosas. Revisar etiqueta, icono, placement, empty/loading/success/error, localización ES/EN y `reduced-motion`.

## Privacidad y distribución
C++20 + Qt6.10+ WebEngine, SQLite local; sin nube Vigilio, login Vigilio ni telemetría remota por defecto. Windows 10/11 x64, Linux x64 y macOS Intel/Apple Silicon son targets sujetos a pruebas reales. Licencia open source aprobada como concepto; la elección legal de MIT/Apache/MPL sigue OPEN hasta aprobación humana.

## Evidencia mínima
- Criterio → prueba → comando → exit code → evidencia, sin afirmaciones no demostradas.
- Qt WebEngine y pruebas de instalador solo PASS si se ejecutaron en sistema compatible.
- Todos los archivos, manifiestos, referencias y lotes deben resolver bajo `docs/` y `apps/`. No crear `openspec/`.


## Referencias obligatorias para diseño
Cuando un micro-prompt afecte interfaz, branding, Senior Mode, instalador, desinstalador o assets, PROPOSE, APPLY y REVIEW deben tomar como referencia la carpeta raíz `/resultado/` y la carpeta `docs/skills/`.


## Lenguaje público de producto
Consulte `docs/UX-COPY-PUBLICO.md`: nunca mostrar «Senior Mode», «Modo Senior», prompts o terminología interna en la UI pública. Usar «Interfaz más fácil de usar». La referencia oficial del logo del instalador es `resultado/06-logo-instalador-oficial.png`.
