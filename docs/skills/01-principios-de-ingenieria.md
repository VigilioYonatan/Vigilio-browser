# Principios de ingeniería

## SOLID
- **S**: cada clase o componente Qt debe tener una responsabilidad clara.
- **O**: ampliar comportamiento mediante composición o nuevas clases, evitando romper APIs existentes.
- **L**: toda implementación derivada debe respetar contratos y no sorprender.
- **I**: interfaces pequeñas y enfocadas; no mezclar navegación, almacenamiento y UI en una sola abstracción.
- **D**: depender de abstracciones o servicios, no de concretos rígidos cuando el módulo crezca.

## DRY
- No duplicar lógica de validación, serialización o estilos.
- Compartir helpers de pruebas, builders de fixtures y tokens visuales.

## KISS
- Preferir soluciones simples, explícitas y mantenibles.
- No añadir procesos, hilos o estados extra sin justificación.

## YAGNI
- No implementar funciones no aprobadas por el roadmap o por el micro-prompt actual.

## Reglas de diseño del código
- Nombres claros y consistentes.
- Errores explícitos.
- APIs pequeñas.
- Comentarios solo donde aporten contexto.
