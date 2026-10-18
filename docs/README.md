# Documentación de Vigilio Browser

## Entradas
- [Resumen del producto](PROJECT-SUMMARY.md)
- [Índice de 1000 tareas](PROMPT-BUILDER-1000-INDEX.md)
- [Plan completo](plans/full-project.yaml) y [lotes secuenciales](plans/batches/)
- [Decisiones humanas](rules/answers.md)
- [Registro de packs](rules/registry.yaml)
- [Reporte de verificación](VALIDACION-1000.md)

**Estado:** paquete documental de implementación, no aplicación completa. `CURRENT` se limita al prototipo Qt/C++ y pruebas locales; `TARGET` comprende las funciones enumeradas. El CLI real de Prompt Builder debe ejecutarse en Node 24.

La política es sin cuenta, login o cloud de Vigilio, actualizaciones manuales, datos locales y código abierto con licencia exacta pendiente de confirmación. En `apps/vigilio-browser/assets/` están los recursos del ejecutable. No existe `openspec/` por decisión del usuario.


## Referencias visuales y skills
- Revisa la carpeta `/resultado/` para la visión visual final del navegador.
- Revisa `docs/skills/` para las buenas prácticas Senior obligatorias (SOLID, DRY, testing, accesibilidad, observabilidad, seguridad, release).
- Los micro-prompts con impacto visual deben citar las imágenes relevantes y mantener consistencia con ellas.
