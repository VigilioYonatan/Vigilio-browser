# Observabilidad, calidad, linters y formateadores

## Formato y linters
- ClangFormat y Clang-Tidy para C++20.
- EditorConfig como base.
- Prettier/ESLint para JS o JSON cuando aplique.

## Observabilidad
- Logs locales por categoría.
- Sin telemetría remota obligatoria.
- Métricas de RAM, CPU, arranque y fallos en modo local.

## Calidad
- Cada cambio debe dejar evidencia de comandos ejecutados, tests y resultados.
- Si no se pudo ejecutar una verificación, documentar por qué.
