# Matriz de prueba — Historial
Evidencia inicial: **NOT_EXECUTED** para este módulo completo salvo fuente histórica verificable. No proclamar PASS por existencia de mocks.
| Tipo | Caso obligatorio | Gate |
|---|---|---|
| Unit | argumentos vacíos, malformed, duplicate, state machine y lifecycle | PR |
| Integration | adapter real, persistencia, error injection, cancellation, rollback | PR de módulo |
| Component Qt | hover/focus/keyboard/screenreader, error/status y mode Senior | UI PR |
| Contract | API/IPC o repositorio cuando aplique, versiones y tipos | PR |
| Security | forbidden origin/profile, secrets, injection, path traversal | PR/release |
| E2E crítico | No private history persistence a través de Qt WebEngine real si cruza capas | release; smoke selectivo |
| Performance | comparar CPU/RAM/latencia con baseline, procesos totales | nightly/release |
| Recovery | kill/restart, disco lleno, timeout ambiguo, restore/deduplicate | release |

E2E pesados serializados, servidor fixture local, sin red pública, profile temp aislado, tests con esperas por estado, no sleeps, teardown de subprocesos. Registrar resultados PASS/FAIL/BLOCKED/NOT_EXECUTED por caso con comando/hardware/fecha/log hash.
