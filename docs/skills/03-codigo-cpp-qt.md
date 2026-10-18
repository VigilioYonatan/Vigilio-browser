# Buenas prácticas de código C++20 + Qt

- Mantener el núcleo de negocio independiente de la ventana principal cuando sea posible.
- No bloquear el hilo de UI.
- Usar RAII, const-correctness y tipos estándar de C++20.
- Limitar macros y herencia innecesaria.
- En Qt, dar ownership claro a widgets y objetos.
- Separar `src/core`, `src/storage`, `src/ui` y módulos futuros por responsabilidad.
- Validar entradas de usuario y rutas de archivo.
- No mezclar persistencia SQLite con rendering de UI.
