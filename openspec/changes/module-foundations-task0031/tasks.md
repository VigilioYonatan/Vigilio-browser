# Tasks — module-foundations-task0031

Status: PROPOSED

- [ ] TASK-001: Declarar campos miembro y métodos de crash recovery en `apps/vigilio-browser/src/ui/BrowserWindow.h`
  - *Criterio de verificación*: El archivo declara `crashRecoveryService_`, `crashRecoveryBanner_`, `heartbeatTimer_`, métodos privados `setupCrashRecovery`, `setupCrashRecoveryBanner`, `handleCrashRecoveryRestore`, `handleCrashRecoveryDiscard`, `handleCrashRecoveryDismiss`, y getters de inspección para pruebas `crashRecoveryBanner()` y `crashRecoveryService()`.
  - *Comando*: `test -f apps/vigilio-browser/src/ui/BrowserWindow.h && grep -E "crashRecoveryService_|crashRecoveryBanner_|setupCrashRecoveryBanner" apps/vigilio-browser/src/ui/BrowserWindow.h`

- [ ] TASK-002: Implementar inicialización de `CrashRecoveryService` y aislamiento de incógnito en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: El constructor de `BrowserWindow` evalúa `isIncognito_`: si es true, la recuperación tras caídas se omite totalmente; si es false, se inicializa `CrashRecoveryService` y se evalúa el reporte; si se detecta `AbnormalTermination` o `RepeatedCrashes`, se despliega el banner sin bloquear el hilo de UI.
  - *Comando*: `grep -E "setupCrashRecovery|initialize.*CrashRecoveryService|isIncognito_" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-003: Construir el widget nativo de banner accesible `vigilio-crash-recovery-banner` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `setupCrashRecoveryBanner` crea un `QFrame` con `setObjectName("vigilio-crash-recovery-banner")`, layout horizontal, icono Lucide SVG, etiqueta con copy público aprobado en español («El navegador no se cerró correctamente...»), botón «Restaurar pestañas», botón «Empezar de cero» y botón «Cerrar aviso».
  - *Comando*: `grep -E "vigilio-crash-recovery-banner|vigilio-crash-recovery-restore|vigilio-crash-recovery-discard" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-004: Implementar manejador de restauración de pestañas con carga diferida (lazy loading) en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `handleCrashRecoveryRestore` llama a `applyRecovery()`, agrega la primera pestaña como visor activo y las restantes como marcadores diferidos con `deferredUrl`, oculta el banner y muestra confirmación en la barra de estado.
  - *Comando*: `grep -E "handleCrashRecoveryRestore|applyRecovery|deferredUrl" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-005: Implementar manejadores de descarte y cancelación (`handleCrashRecoveryDiscard` / `handleCrashRecoveryDismiss`) y atajo `Escape` en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: `handleCrashRecoveryDiscard` invoca `discardRecovery()`, oculta el banner y enfoca la barra de direcciones; `handleCrashRecoveryDismiss` y el atajo `Key_Escape` invocan `cancelRecovery()` y ocultan el banner preservando los datos.
  - *Comando*: `grep -E "handleCrashRecoveryDiscard|handleCrashRecoveryDismiss|discardRecovery|cancelRecovery" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-006: Implementar temporizador periódico de latido (Heartbeat) y coordinación de apagado en `apps/vigilio-browser/src/ui/BrowserWindow.cpp`
  - *Criterio de verificación*: Un `QTimer` ejecuta cada 30 segundos `crashRecoveryService_->recordHeartbeat(now)`; en `closeEvent`, si `report.cleanExit == true`, se invoca `crashRecoveryService_->finalizeShutdown(true, CrashReason::CleanShutdown)`.
  - *Comando*: `grep -E "recordHeartbeat|finalizeShutdown|heartbeatTimer_" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-007: Implementar escalado dinámico en la «Interfaz más fácil de usar» (Modo Senior) en `BrowserWindow::setSeniorMode(bool)`
  - *Criterio de verificación*: En `setSeniorMode(true)`, el banner escala su altura mínima a $\ge 56$ px y sus botones a $\ge 48$ px; al desactivarse, revierten a sus dimensiones estándar (42 px y 32 px).
  - *Comando*: `grep -E "crashRecoveryBanner_.*setMinimumHeight" apps/vigilio-browser/src/ui/BrowserWindow.cpp`

- [ ] TASK-008: Ampliar la suite de pruebas unitarias y de UI en `apps/vigilio-browser/tests/ui/WindowTests.cpp`
  - *Criterio de verificación*: Nuevas aserciones en `WindowTests.cpp` verificando: banner creado ante terminación anómala, banner ausente en salida limpia, aislamiento total en incógnito, clics en restaurar y descartar, textos de crash loop, navegación por teclado y escalado en Modo Senior.
  - *Comando*: `test -f apps/vigilio-browser/tests/ui/WindowTests.cpp && grep -E "test_crash_recovery_banner|vigilio-crash-recovery-banner" apps/vigilio-browser/tests/ui/WindowTests.cpp`

- [ ] TASK-009: Ejecutar el pipeline de compilación y verificación continua local (`apps/vigilio-browser/scripts/check.sh`)
  - *Criterio de verificación*: Compilación Ninja limpia bajo C++20 con `-Werror`, cero advertencias, formato Clang-Format impecable y 100% de tests de CTest pasando exitosamente (exit code 0).
  - *Comando*: `bash apps/vigilio-browser/scripts/check.sh`

- [ ] TASK-010: Validar referencias canónicas y lockfile con Prompt Builder CLI
  - *Criterio de verificación*: `validate-references` certifica `valid: true` sobre todas las referencias canónicas, y `verify-lock` certifica la integridad del archivo lock correspondiente a 0091.
  - *Comando*: `node /config/Desktop/auto/prompt-builder/dist/cli.js validate-references --project-root . && node /config/Desktop/auto/prompt-builder/dist/cli.js verify-lock --lock-file generated/prompts/full-project/0091-module.foundations.task0031.propose.lock.json --project-root .`

- [ ] TASK-011: Sincronizar documentación canónica en `docs/rules/01-modules/foundations/micro/0031-crash-recovery-policy/task.md`
  - *Criterio de verificación*: El documento de la microtarea formaliza el cumplimiento de la fase UI y se mantiene coherente con los contratos visuales Qt de crash recovery.
  - *Comando*: `test -f docs/rules/01-modules/foundations/micro/0031-crash-recovery-policy/task.md && grep -E "module.foundations.task0031" docs/rules/01-modules/foundations/micro/0031-crash-recovery-policy/task.md`
