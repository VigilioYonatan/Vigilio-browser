# Lenguaje público — Vigilio Browser

## Alcance

Vigilio es un **navegador público**. La documentación técnica puede utilizar nombres de ingeniería, pero los textos de la aplicación, configuraciones, instalador, desinstalador, ayudas y diálogos deben utilizar lenguaje natural y sencillo.

## Terminología para el público

| Concepto interno | Etiqueta pública aprobada |
| --- | --- |
| `Senior Mode` / `Modo Senior` | **Interfaz más fácil de usar** |
| `Memory Saver` | **Ahorro de memoria** |
| `Vigilio Shields` | **Protección contra anuncios y rastreadores** (en el panel; la marca puede aparecer en «Acerca de») |
| `MCP` | **Conexión con asistentes** (MCP se puede explicar en «Opciones avanzadas») |
| `Lazy loading` | **Cargar pestañas cuando las necesites** |
| `Tab discard` | **Liberar memoria de pestañas inactivas** |
| `Prompt Builder` / `PROPOSE` / `APPLY` / `REVIEW` | Nunca aparece en la UI pública |
| `Skill` / `micro-prompt` / `DAG` | Nunca aparece en la UI pública |
| `OpenSpec` | Nunca aparece en la UI pública |

## Ejemplos de textos visibles

- Menú: **Interfaz más fácil de usar**.
- Ajustes → **Accesibilidad** → **Interfaz más fácil de usar**.
- Estado: **Interfaz fácil activada** / **Interfaz fácil desactivada**.
- Ayuda: **Aumenta el tamaño del texto, los botones y el espacio entre elementos.**
- Instalador: **¿Quieres usar una interfaz con letras y botones más grandes?**
- Desinstalador: **Exportar mis datos**, **Conservar mis datos en este equipo**, **Eliminar mis datos del equipo**.
- Privacidad: **Bloqueo de anuncios y rastreadores**.
- Descargas: **Descargas**.

## Política de UX writing

1. Sustantivos y verbos conocidos para usuarios generales; evitar jerga técnica en acciones primarias.
2. En etiquetas, describir la acción o resultado concreto, no la tecnología.
3. No usar palabras como «senior», «prompt», «skill» ni «agente MCP» en los flujos comunes.
4. Permitir términos técnicos en opciones avanzadas cuando sean necesarios para una configuración experta, acompañados de explicación.
5. Mantener textos breves, consistentes y traducibles; sin promesas de rendimiento no medidas.
6. Probar labels con navegación por teclado, lectores de pantalla, localización y escalado.

## Límite

Este documento establece textos objetivo; no demuestra la implementación de todas las pantallas. Hay que auditar cualquier nueva cadena de UI al implementarla.
