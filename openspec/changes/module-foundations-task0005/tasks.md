# Tasks — module-foundations-task0005

Status: PROPOSED

- [ ] TASK-001: Formalizar la especificación de dominio y catálogo canónico de advertencias del compilador para GCC, Clang, AppleClang y MSVC
  - *Criterio de verificación*: La especificación de dominio detalla sin ambigüedad la matriz de advertencias obligatorias (seguridad de tipos, polimorfismo C++20, higiene de identificadores y prevención de UB), la distinción entre compiladores y el catálogo formal de flags.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`

- [ ] TASK-002: Diseñar y formalizar la arquitectura del target de interfaz CMake (`vigilio_compiler_warnings`) y las reglas de aislamiento
  - *Criterio de verificación*: El diseño técnico formaliza la encapsulación de flags en un target `INTERFACE` en CMake, vinculación obligatoria `PRIVATE` para impedir la fuga transitiva hacia consumidores externos y la inclusión de cabeceras de terceros (`SQLite3`, `Qt6`) mediante la directiva `SYSTEM`.
  - *Comando*: `test -f openspec/changes/module-foundations-task0005/design.md`

- [ ] TASK-003: Definir la gobernanza de la opción `VIGILIO_WARNINGS_AS_ERRORS` y la compatibilidad con código generado por Qt (`moc`, `rcc`)
  - *Criterio de verificación*: Se especifica formalmente la opción configurable `VIGILIO_WARNINGS_AS_ERRORS` (`-Werror` / `/WX`) con activación predeterminada en CI y presets estrictos, junto con la estrategia para mitigar advertencias por expansiones internas de macros de Qt (`Q_OBJECT`).
  - *Comando*: `grep -q "VIGILIO_WARNINGS_AS_ERRORS" openspec/changes/module-foundations-task0005/design.md`

- [ ] TASK-004: Validar la ausencia de regresiones y comprobar que la base de código actual compila limpiamente sin advertencias latentes
  - *Criterio de verificación*: Ejecución limpia del script de comprobación local en modo headless con Ninja, superando el 100% de las pruebas de CTest (`core_units` y `storage_integration`) y auditoría de formato con ClangFormat con exit code 0.
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-005: Sincronizar la documentación canónica de la microtarea y verificar el árbol de referencias
  - *Criterio de verificación*: El archivo canónico `docs/rules/01-modules/foundations/micro/0005-compiler-warnings/task.md` actualiza su estado de ejecución tras la aprobación y pasa satisfactoriamente la validación de Prompt Builder.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`
