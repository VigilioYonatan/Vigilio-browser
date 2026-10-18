# Change Proposal — module-foundations-task0005

Pack: `module.foundations.task0005`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica integral para la etapa **domain** de la capacidad **compiler warnings** en el módulo propietario `foundations` de Vigilio Browser. Tras haber completado la gobernanza, modularización y suite de pruebas de `cmake targets` (`module.foundations.task0001` a `module.foundations.task0004`), esta microtarea formaliza el modelo de dominio, reglas, categorización de diagnósticos e invariantes de compilación estricta para garantizar una base de código C++20 con cero defectos latentes, alta portabilidad multiplataforma y cero tolerancia a advertencias en entornos de integración continua y desarrollo local.

El alcance concreto de esta etapa **domain** comprende:
- **Modelo de dominio de diagnósticos de compilación**:
  - Definición del catálogo canónico de advertencias por familia de compilador: GNU GCC (`GNU`), LLVM Clang (`Clang` / `AppleClang`) y Microsoft Visual C++ (`MSVC`).
  - Inclusión de advertencias críticas para la seguridad de tipos, memoria y estándares C++20: conversión con pérdida de datos (`-Wconversion`, `-Wsign-conversion`), sombreado de identificadores (`-Wshadow`), destructores no virtuales en clases polimórficas (`-Wnon-virtual-dtor`), conversiones de estilo C (`-Wold-style-cast`), alineación de punteros (`-Wcast-align`), código no utilizado (`-Wunused`), funciones virtuales sobrecargadas ocultas (`-Woverloaded-virtual`), formato de cadenas (`-Wformat=2`), desreferenciación nula (`-Wnull-dereference`), promoción implícita de punto flotante (`-Wdouble-promotion`) y caídas de switch no documentadas (`-Wimplicit-fallthrough`).
  - Para MSVC: activación de modo de conformidad estricto (`/permissive-`), nivel de advertencias `/W4` y activación selectiva de advertencias complementarias (`/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/we4289`, `/w14296`, `/w14311`, `/w14545`, `/w14546`, `/w14547`, `/w14549`, `/w14555`, `/w14619`, `/w14640`, `/w14826`, `/w14905`, `/w14906`, `/w14928`).
- **Política de Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS`)**:
  - Definición de la gobernanza de CMake mediante la opción `VIGILIO_WARNINGS_AS_ERRORS` (`-Werror` en GCC/Clang, `/WX` en MSVC), asegurando que ninguna advertencia pase desapercibida en CI.
  - Flexibilidad para desarrolladores locales cuando se requiera iteración rápida sin abortar la construcción ante advertencias transitorias.
- **Arquitectura de aislamiento y no-fuga transitiva**:
  - Definición del target de interfaz canónico `vigilio_compiler_warnings` en CMake, evitando la duplicación de bloques `if(MSVC) ... else() ...` en cada target.
  - Invariante de consumo: vinculación `PRIVATE` hacia `vigilio_compiler_warnings` para que las advertencias del proyecto no se propaguen a consumidores downstream ni a bibliotecas externas.
  - Aislamiento riguroso de cabeceras de terceros (`SQLite3`, `Qt6`): marcado obligatorio como `SYSTEM` en CMake (`target_include_directories(... SYSTEM ...)`) para inmunizar la compilación frente a advertencias fuera del control del proyecto.
  - Tratamiento de código generado por herramientas de Qt (`moc`, `rcc`, `uic`): exclusión o mitigación de falsos positivos conocidos (tales como expansiones internas de macros Qt) sin debilitar las comprobaciones en el código fuente propio.
- **Invariantes de calidad, rendimiento y privacidad**:
  - Garantía de que la capacidad opera exclusivamente en tiempo de compilación/análisis estático; no introduce sobrecarga en tiempo de ejecución, consumo de RAM adicional ni retardo en el inicio de la aplicación.
  - Total neutralidad visual y de privacidad: sin cambio en la interfaz gráfica de usuario, sin telemetría, sin conexiones externas ni cuentas en la nube.

## Out of scope

- Implementación del refactor del archivo `apps/vigilio-browser/CMakeLists.txt` o creación de archivos `.cmake` auxiliares (diferida a la etapa `implementation` en `module.foundations.task0006`).
- Construcción o modificación de widgets, diálogos o controles de interfaz Qt (diferida a la etapa `ui` en `module.foundations.task0007`, donde se confirmará la ausencia de impacto visual directo).
- Creación de pruebas de integración y matrices de regresión de advertencias (diferida a la etapa `tests` en `module.foundations.task0008`).
- Configuración de formateadores de código (`clang-format`, microtareas 0041-0044) o linters semánticos (`clang-tidy`, microtareas 0045-0048), los cuales constituyen capacidades independientes dentro de `foundations`.
- Modificación directa de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE.

## Contexto y evidencia

Se inspeccionó de forma exhaustiva el repositorio `apps/vigilio-browser` y su configuración actual:
1. **Configuración de construcción existente (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_core` aplica condicionales directos:
     ```cmake
     if(MSVC)
       target_compile_options(vigilio_core PRIVATE /W4 /permissive-)
     else()
       target_compile_options(vigilio_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
     endif()
     ```
   - `vigilio_storage` duplica idéntico bloque de opciones de compilación.
   - Los targets de prueba (`vigilio_core_tests`, `vigilio_storage_tests`) y de aplicación (`vigilio-browser`, `vigilio_ui_tests`, `vigilio_e2e_smoke`) no tienen opciones de advertencia asociadas de manera uniforme.
   - No existe un target de interfaz centralizado para flags de advertencia, lo que viola el principio DRY y dificulta la evolución coherente de políticas de compilación.
   - Faltan advertencias críticas de C++20 (`-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Woverloaded-virtual`, `-Wnull-dereference`, `-Wformat=2`).
2. **Entorno de compilación y herramientas de desarrollo**:
   - CMake 4.2.3, Ninja 1.13.2, GCC 15.2.0 y Qt 6.10.2 instalados y operativos.
   - Baseline verificado: script `apps/vigilio-browser/scripts/check.sh` compila limpiamente con Ninja en modo headless y pasa el 100% de las pruebas (`core_units` y `storage_integration`, exit code 0).
3. **Verificación de gobernanza y Prompt Builder**:
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root .`: 5 referencias canónicas validadas, estado `valid: true`, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0013-module.foundations.task0005.propose.lock.json`: validación al 100% aprobada sobre todos los documentos canónicos y el baseline de fábrica, exit code 0.
   - `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-decisions --project-root .`: decisiones Q-001 a Q-006 y Q-008 resueltas/no bloqueantes; Q-007 permanece OPEN afectando únicamente el hito final de publicación `release.public-release`.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `domain`.
- **Pack**: `module.foundations.task0005`.
- **Capacidad**: `compiler warnings`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0005/proposal.md`
  - `openspec/changes/module-foundations-task0005/design.md`
  - `openspec/changes/module-foundations-task0005/tasks.md`
  - `openspec/changes/module-foundations-task0005/execution-memory.md`
  - `openspec/changes/module-foundations-task0005/canonical-reference.yaml`
- **Restricción de escritura**: Exclusivamente `openspec/changes/module-foundations-task0005/**`. Prohibida la modificación de código bajo `apps/**` o `docs/rules/**` en este paso.

## Pasos APPLY

Cuando este change sea aprobado para la fase APPLY, se ejecutarán en orden determinista los siguientes pasos:
1. **Formalización de especificación de dominio**:
   - Consolidar las especificaciones y contratos de flags de advertencia por compilador en la documentación canónica de la microtarea `docs/rules/01-modules/foundations/micro/0005-compiler-warnings/task.md`.
2. **Definición del modelo de datos e invariantes**:
   - Registrar la taxonomía de perfiles de advertencia (`Dev`, `Strict/CI`, `Release`) y la gobernanza de la opción `VIGILIO_WARNINGS_AS_ERRORS`.
3. **Validación de compatibilidad con el código actual**:
   - Ejecutar una inspección previa sobre los fuentes existentes en `apps/vigilio-browser/src` para constatar que cumplen con el catálogo propuesto sin generar advertencias ocultas.
4. **Verificación integral de gobernanza y calidad**:
   - Revalidar referencias y decisiones con Prompt Builder (`validate-references`, `validate-decisions`).
   - Ejecutar `apps/vigilio-browser/scripts/check.sh` asegurando que el baseline se mantiene 100% verde.

## Definición de terminado (Definition of Done)

- [x] Modelo de dominio, taxonomía de flags e invariantes de `compiler warnings` formalizados exhaustivamente en la propuesta.
- [x] Catálogo de advertencias canónicas definido para GNU GCC, LLVM Clang, AppleClang y MSVC, cubriendo seguridad de tipos y memoria C++20.
- [x] Política de `VIGILIO_WARNINGS_AS_ERRORS` y mecanismo de aislamiento de dependencias de terceros (`SYSTEM`) y código generado Qt (`moc`, `rcc`) formalizados.
- [x] Ausencia total de cambio visual directo documentada y justificada conforme a `docs/skills/04-ux-ui-senior.md`, `docs/skills/05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Requisitos de rendimiento (cero sobrecarga en runtime), privacidad (local-first estricto) y límites de RAM verificados según las skills del proyecto.
- [x] Matriz de pruebas trazable completa con clasificación explícita (`APPLY`, `LATER`, `NOT_APPLICABLE`).
- [x] Tareas de implementación atómicas y verificables redactadas con identificadores estables en `tasks.md`.
- [x] Memoria de ejecución completa con registro de habilidades consultadas, evidencias de comandos y criterios de handoff.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0004` (etapa `tests` de cmake targets, completada con éxito).
- **Dependencias posteriores**: `module.foundations.task0006` (etapa `implementation` de compiler warnings).
- **Riesgos técnicos y mitigaciones**:
  - *Riesgo*: Falsos positivos en macros y código generado por Qt (`moc_*.cpp`, `qrc_*.cpp`) al activar advertencias estrictas como `-Wold-style-cast` o `-Wuseless-cast`.
    *Mitigación*: En la arquitectura de CMake, aislar los archivos generados o excluir flags que entren en conflicto directo con las convenciones de generación de código de Qt, manteniendo las comprobaciones estrictas sobre el código fuente del proyecto.
  - *Riesgo*: Advertencias provenientes de cabeceras del sistema o dependencias externas (`sqlite3.h`, cabeceras Qt) al compilar con `-Werror` / `/WX`.
    *Mitigación*: Uso mandatorio de la directiva `SYSTEM` en las rutas de inclusión de bibliotecas externas en CMake (`target_include_directories(... SYSTEM ...)`), instruyendo a los compiladores a suprimir advertencias en esas cabeceras.
  - *Riesgo*: Divergencias de comportamiento entre versiones de compiladores (GCC vs Clang vs MSVC).
    *Mitigación*: Pruebas de configuración condicionales por `CMAKE_CXX_COMPILER_ID` y validación de compatibilidad en matrices de CI.

## Commit propuesto

`docs(foundations): domain compiler-warnings`
