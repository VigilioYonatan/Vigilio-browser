# Auditoría — revisión docs/apps únicamente

PASS: exclusión física de `openspec/`, assets dentro de `apps/vigilio-browser`, E2E option CMake antes de uso, QRC añadido al target Qt, documentación UX, modos, instalación, herramientas y política offline.
PARTIAL: 28 módulos con anexos target-specific, 56 incrementos por commit; mantener revisión manual para eliminar boilerplate legado.
NOT_EXECUTED: build Qt6 WebEngine >=6.10, GUI, E2E reales, CI multiplataforma, instaladores nativos, Narrator/VoiceOver/Orca, benchmark.
OPEN: licencia jurídica exacta; certificados firma release. Open source como dirección aprobada, sin atribuir una licencia concreta.
No garantía de 100% de completitud o ausencia de bugs. `validate_package.py` solo valida estructura.
