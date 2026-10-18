# Auditoría de contratos PROPOSE por pack — Senior v3

Fecha: 2026-10-16

## Resultado comprobado
- Task Markdown inspeccionados y reforzados por script: **1000**.
- Task Markdown con contrato PROPOSE específico: **1000**.
- Packs YAML detectados: **1000**.
- Markdown inspeccionados mecánicamente: **1264**.
- Markdown vacíos: **0**.
- Bloques de código desequilibrados: **0**.
- IDs Q duplicados en `answers.md` después de reparación: **0**.
- Reglas por etapa detectadas: `{'domain': 260, 'implementation': 254, 'ui': 248, 'tests': 238}`.

## Hallazgo y reparación
- Problema: 1000 tareas compartían una plantilla de 41 líneas con PROPOSE implícito y no exigían evidencia de diseño completo. **Corregido:** nueva sección de contrato PROPOSE, APPLY y REVIEW por tarea, además de owner global.
- Problema: ID `Q-006` duplicado para licencia jurídica. **Corregido:** mantener `Q-007` como owner de licencia exacta OPEN.

## LIMITACIONES — no disfrazar como aprobado
- Revisión de significado humano individual de los 1265 Markdown: **NOT_EXECUTED**. El script verificó formato e inyectó requisitos específicos por operación/etapa, pero no demuestra que cada flujo de negocio esté completo.
- Prompt Builder 0.6.3 CLI/RESOLVER real: **NOT_EXECUTED** en esta pasada; requiere compatibilidad runtime y comandos reales. La validez del esquema registrada en versión previa no prueba generación de PROPOSE.
- Compilación Qt WebEngine/E2E multiplataforma y validación de UX por usuarios: **NOT_EXECUTED**.
- Licencia exacta: **OPEN**, por lo tanto no PUBLICATION_READY.
- El refuerzo aumenta la exigencia de PROPOSE, pero no sustituye contratos ejecutables detallados de cada herramienta ni demuestra ausencia de bugs.

## Próximo gate obligatorio
1. Ejecutar el CLI real compatible con Node 24 contra plan y registry.
2. Generar 3 PROPOSE de muestra (fundamentos, tabs, installer) y evaluar con evidencia concreta; después lote de 50.
3. Realizar revisión semántica humana de owners específicos, detectar contradicciones y refinar casos de aceptación antes de automatizar APPLY.

## Gate adicional de empaquetado v3
- 1000 packs YAML leídos y sus dependencias comprobadas: PASS (preflight propio).
- Documentos `required` localizados: PASS.
- Escrituras OpenSpec detectadas en `allowed_paths`: 0.
- Validador heredado `docs/validate_package.py`: FAIL por regla obsoleta que exige 56 prompts; no usarlo como evidencia de la versión 1000.
- Este preflight local **no** equivale al CLI de Prompt Builder.
