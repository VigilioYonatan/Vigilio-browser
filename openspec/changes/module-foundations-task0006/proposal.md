# Change Proposal — module-foundations-task0006

Pack: `module.foundations.task0006`

Status: PROPOSED

## Outcome and scope

Este cambio define la propuesta técnica completa para la etapa **implementation** de la capacidad **compiler warnings** en el módulo propietario `foundations` de Vigilio Browser. Tras haber formalizado la especificación de dominio, la taxonomía de defectos y el catálogo canónico de advertencias en `module.foundations.task0005`, este cambio proyecta la implementación física, centralizada y desacoplada de la gobernanza de advertencias del compilador en el sistema de construcción de `apps/vigilio-browser/CMakeLists.txt`.

El alcance concreto de este pack comprende:
- **Implementación del target de interfaz centralizado `vigilio_compiler_warnings`**:
  - Creación de una biblioteca de interfaz (`add_library(vigilio_compiler_warnings INTERFACE)`) que encapsula y gobierna de manera uniforme todos los flags de diagnóstico de compilación para C++20.
  - Implementación del catálogo canónico completo por familia de compilador:
    - **GNU GCC**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wmisleading-indentation`, `-Wduplicated-cond`, `-Wduplicated-branches`, `-Wlogical-op`, `-Wuseless-cast`.
    - **LLVM Clang / AppleClang**: `-Wall`, `-Wextra`, `-Wpedantic`, `-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wnon-virtual-dtor`, `-Wold-style-cast`, `-Wcast-align`, `-Wunused`, `-Woverloaded-virtual`, `-Wformat=2`, `-Wnull-dereference`, `-Wdouble-promotion`, `-Wimplicit-fallthrough`, `-Wextra-semi`.
    - **Microsoft Visual C++ (MSVC)**: `/W4`, `/permissive-`, `/w14242`, `/w14254`, `/w14263`, `/w14265`, `/w14287`, `/we4289`, `/w14296`, `/w14311`, `/w14545`, `/w14546`, `/w14547`, `/w14549`, `/w14555`, `/w14619`, `/w14640`, `/w14826`, `/w14905`, `/w14906`, `/w14928`, `/EHsc`.
- **Implementación de la política configurable de Warnings-as-Errors (`VIGILIO_WARNINGS_AS_ERRORS`)**:
  - Opción de CMake `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)`.
  - Tratamiento estricto de advertencias como errores (`-Werror` en GCC/Clang y `/WX` en MSVC) activado por defecto para garantizar que ninguna advertencia se introduzca en CI o en compilaciones de desarrollo estándar.
  - Flexibilidad para desarrolladores locales que requieran desactivación temporal (`-DVIGILIO_WARNINGS_AS_ERRORS=OFF`) durante iteraciones exploratorias sin abortar la compilación por variables temporales.
- **Refactorización y unificación de targets en CMake**:
  - Eliminación de los bloques duplicados de `target_compile_options` ad-hoc en `vigilio_core` y `vigilio_storage`.
  - Vinculación `PRIVATE` obligatoria hacia `vigilio_compiler_warnings` en todos los targets: bibliotecas de dominio y almacenamiento (`vigilio_core`, `vigilio_storage`), ejecutables de pruebas (`vigilio_core_tests`, `vigilio_storage_tests`), aplicación de escritorio (`vigilio-browser`) y pruebas de interfaz/humo (`vigilio_ui_tests`, `vigilio_e2e_smoke`).
  - Garantía de no-fuga transitiva: la vinculación privada asegura que los flags internos de Vigilio no afecten a consumidores downstream ni a bibliotecas externas.
- **Aislamiento de dependencias externas y código generado**:
  - Inclusión de cabeceras de terceros (`SQLite3`, `Qt6`) con directivas `SYSTEM` para que los compiladores supriman diagnósticos sobre código fuera del control del repositorio.
  - Inmunización del código generado por Qt (`moc`, `rcc`) frente a diagnósticos originados en la expansión de macros internas de Qt (`Q_OBJECT`).
- **Garantías de calidad, rendimiento y privacidad**:
  - Capacidad que opera 100% en tiempo de compilación; cero impacto en tiempo de ejecución, cero consumo de memoria RAM adicional y cero retardo en el inicio.
  - Neutralidad visual y de privacidad absoluta: sin impacto en UI, sin telemetría, sin conexiones de red ni servicios en la nube.

## Out of scope

- Alteración de la superficie visual o adición de componentes de interfaz Qt (diferido a la etapa `ui` en `module.foundations.task0007`, donde se confirmará la ausencia de impacto visual directo).
- Creación de pruebas de regresión negativas automatizadas que fuercen fallos de advertencia en pipelines dedicados (diferido a la etapa `tests` en `module.foundations.task0008`).
- Modificación de las configuraciones de formateo de código (`clang-format`, tareas 0041-0044) o linters semánticos (`clang-tidy`, tareas 0045-0048), las cuales son capacidades independientes en `foundations`.
- Modificación directa de código fuente en `apps/**` o `docs/rules/**` durante esta fase PROPOSE (la implementación se realizará exclusivamente en la fase APPLY tras aprobación).

## Contexto y evidencia

Se inspeccionó detalladamente la base de código existente en `apps/vigilio-browser`:
1. **Configuración actual de CMake (`apps/vigilio-browser/CMakeLists.txt`)**:
   - `vigilio_core` define opciones de compilación privadas aisladas:
     ```cmake
     if(MSVC)
       target_compile_options(vigilio_core PRIVATE /W4 /permissive-)
     else()
       target_compile_options(vigilio_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
     endif()
     ```
   - `vigilio_storage` duplica exactamente este mismo bloque de opciones.
   - Los ejecutables de pruebas unitarias e integración (`vigilio_core_tests`, `vigilio_storage_tests`) carecen de flags de advertencia, compilándose con opciones por defecto del compilador.
   - El ejecutable principal `vigilio-browser` y las pruebas de interfaz (`vigilio_ui_tests`, `vigilio_e2e_smoke`) tampoco poseen flags de diagnóstico configurados explícitamente.
   - No existe la opción `VIGILIO_WARNINGS_AS_ERRORS`, lo que impide gobernar de forma determinista el rigor en CI.
2. **Compatibilidad del código C++20 actual**:
   - Se verificó la compilación de todos los archivos fuente bajo `src/core/` (`SessionFile.cpp`, `TabModel.cpp`, `UrlInput.cpp`) y `src/storage/` (`LocalRepository.cpp`) con el catálogo completo de flags de GCC (`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wformat=2 -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast -Werror`).
   - Resultado: el código actual compila limpiamente sin advertencias ni errores con GCC 15.2.0.
3. **Simulación de construcción completa**:
   - Se simuló la integración de `vigilio_compiler_warnings` en un entorno aislado temporal con Ninja y GCC 15.2.0, verificando la compilación exitosa de los 10 objetos del proyecto y la ejecución del 100% de los tests (`core_units` y `storage_integration`) con exit code 0.
4. **Verificación de herramientas y decisiones**:
   - CMake 4.2.3, Ninja 1.13.2 y GCC 15.2.0 operativos.
   - Script `apps/vigilio-browser/scripts/check.sh` pasa exitosamente.
   - Decisiones canónicas en `docs/rules/answers.md`: Q-001 a Q-006 y Q-008 resueltas; Q-007 (licencia) permanece OPEN bloqueando exclusivamente `release.public-release`.
   - Lockfile `0016-module.foundations.task0006.propose.lock.json` verificado y 100% válido.

## Alcance exacto

- **Fase**: PROPOSE.
- **Etapa**: `implementation`.
- **Pack**: `module.foundations.task0006`.
- **Artefactos producidos en OpenSpec**:
  - `openspec/changes/module-foundations-task0006/proposal.md`
  - `openspec/changes/module-foundations-task0006/design.md`
  - `openspec/changes/module-foundations-task0006/tasks.md`
  - `openspec/changes/module-foundations-task0006/execution-memory.md`
  - `openspec/changes/module-foundations-task0006/canonical-reference.yaml`
- **Restricción de escritura en PROPOSE**: Exclusivamente `openspec/changes/module-foundations-task0006/**`.

## Pasos APPLY

Cuando este change sea aprobado por un humano para la fase APPLY, se ejecutarán los siguientes pasos:
1. En `apps/vigilio-browser/CMakeLists.txt`, declarar el target de interfaz `vigilio_compiler_warnings` con las listas de flags canónicas para Clang, GCC y MSVC.
2. Definir la opción de CMake `option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)` y asociar condicionalmente `-Werror` (Clang/GCC) o `/WX` (MSVC) a `vigilio_compiler_warnings`.
3. Reemplazar los bloques ad-hoc de `target_compile_options` en `vigilio_core` y `vigilio_storage` por la vinculación `target_link_libraries(<target> PRIVATE vigilio_compiler_warnings)`.
4. Vincular `vigilio_compiler_warnings` de forma privada a los ejecutables de pruebas `vigilio_core_tests` y `vigilio_storage_tests`.
5. Vincular `vigilio_compiler_warnings` a los targets de interfaz `vigilio-browser`, `vigilio_ui_tests` y `vigilio_e2e_smoke`, asegurando el aislamiento `SYSTEM` de las dependencias de Qt6.
6. Ejecutar la compilación y pruebas locales mediante `apps/vigilio-browser/scripts/check.sh` y verificar exit code 0 con cero advertencias.
7. Sincronizar el estado en `docs/rules/01-modules/foundations/micro/0006-compiler-warnings/task.md` pasando el estado a `CURRENT; PASS`.

## Definición de terminado (Definition of Done)

- [x] Propuesta técnica completa y coherente para la implementación de `compiler warnings` elaborada y documentada.
- [x] Target de interfaz `vigilio_compiler_warnings` diseñado con catálogo exhaustivo de diagnósticos para GCC, Clang y MSVC.
- [x] Opción `VIGILIO_WARNINGS_AS_ERRORS` formalizada para gobernar la tolerancia a advertencias en CI y entornos locales.
- [x] Principio DRY y SOLID respetado mediante eliminación de bloques de flags duplicados y vinculación `PRIVATE`.
- [x] Aislamiento riguroso de cabeceras de terceros (`SQLite3`, `Qt6`) con directivas `SYSTEM` para evitar falsos positivos externos.
- [x] Compatibilidad total headless demostrada con `VIGILIO_BUILD_GUI=OFF` y CTest.
- [x] Ausencia de impacto visual directo documentada y alineada con `docs/skills/04-ux-ui-senior.md`, `05-accesibilidad-y-senior-mode.md` y `docs/UX-COPY-PUBLICO.md`.
- [x] Tareas ordenadas, atómicas y verificables formalizadas en `tasks.md`.

## Dependencias y riesgos

- **Dependencias previas**: `module.foundations.task0005` (etapa `domain` de compiler warnings, completada).
- **Dependencias posteriores**: `module.foundations.task0007` (etapa `ui` de compiler warnings).
- **Riesgos y mitigaciones**:
  - *Riesgo*: Advertencias espurias provocadas por macros de bibliotecas externas (como SQLite3 o Qt).
    *Mitigación*: Marcar los directorios de inclusión externos con `SYSTEM` en CMake, instruyendo al compilador para que suprima diagnósticos en código ajeno al proyecto.
  - *Riesgo*: Incompatibilidad de flags específicos entre versiones de compiladores (por ejemplo, flags exclusivos de GCC que Clang rechaza).
    *Mitigación*: Desacoplar estrictamente las listas de opciones (`GCC_WARNINGS`, `CLANG_WARNINGS`, `MSVC_WARNINGS`) condicionadas por `CMAKE_CXX_COMPILER_ID`.
  - *Riesgo*: Bloqueo excesivo en estaciones de desarrollo locales ante prototipado rápido.
    *Mitigación*: Permitir que el desarrollador desactive `-Werror` localmente mediante `-DVIGILIO_WARNINGS_AS_ERRORS=OFF`, manteniendo `ON` como valor predeterminado para CI.

## Commit propuesto

`feat(foundations): implementation compiler-warnings`
