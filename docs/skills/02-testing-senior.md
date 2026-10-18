# Testing Senior

## Pirámide de pruebas
1. Unit tests para lógica y contratos.
2. Integración para SQLite, perfiles, archivos y servicios internos.
3. E2E solo para recorridos críticos.

## Reglas
- Cada bug reproducible debe convertirse en una prueba.
- Las pruebas E2E deben ejecutarse **en serie** si consumen mucha RAM.
- Usar fixtures deterministas y datos locales.
- Evitar sleeps fijos; esperar condiciones observables.
- Registrar claramente `PASS`, `FAIL`, `NOT_EXECUTED` y motivo.

## Cobertura mínima deseada
- Núcleo propio: alta cobertura de caminos críticos.
- UI: foco en estados, teclado, accesibilidad y flujos de error.
- Seguridad: pruebas negativas y de permisos.
