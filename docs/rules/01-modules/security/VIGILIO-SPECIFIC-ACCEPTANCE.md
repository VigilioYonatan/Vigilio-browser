# Security — contrato específico Vigilio

**Owner:** `security`. **Estado actual:** TARGET salvo código explicitado como CURRENT por evidencia de compilación.

## Apariencia y descubrimiento
entrada contextual de security desde Herramientas/Ajustes según necesidad. Titular la acción en español natural, icono SVG, foco teclado accesible y ayuda corta. Modo Senior: controles >=44px, texto configurable >=18px en UI, no depender solo de color. Modo Experto: accesos adicionales sin omitir confirmaciones.

## Acción y reglas
realizar la operación observable de security sin activar módulos no usados. Iniciar solo cuando el usuario invoque la operación; validate input y scope local. UI muestra pending, success, recoverable-error, cancelled, empty y denied. No cloud/login/telemetría propia.

## Casos difíciles
datos vacíos, operación cancelada, recurso fuera de alcance y reinicio durante security. Simular permisos insuficientes, fallos IO, estado anterior obsoleto, multi-ventana, escape/cancelación y perfil privado. Log local sin secretos. Cada operación que borre datos pide consentimiento reversible cuando sea posible.

## Pruebas de aceptación
- Unit: regla específica security, entradas inválidas y transiciones imposibles.
- Integration: integración con Qt/SQLite según módulo, perfil temporal y recursos locales.
- UI: foco + keyboard + Esc + alto contraste + Senior/Normal.
- E2E: únicamente el flujo crítico real de security, serial y con fixture offline; marcar NOT_EXECUTED hasta correrlo.
- Seguridad: frontera de perfil y autorización, sin acceso a otros perfiles por defecto.
- Benchmark si aplica: consumo de CPU/RAM y no bloquear main thread; no afirmar ahorro antes de medir.

## DoD
Contrato implementado, build y tests con evidencia, revisión de seguridad/UX, regresión atendida, commit atómico y rollback.
