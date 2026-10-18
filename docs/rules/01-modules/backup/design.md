# Diseño de Backup/importación
## Límites y capas
El módulo **backup** expone modelos y comandos sin widgets. Controller transfiere estado desde servicios `BackupManager` hacia UI Qt; persistencia usa SQLite/QStandardPaths solo si procede y los secretos no se guardan en texto plano. Para cada cambio de estado: validar argumentos y permisos, emitir progreso, actualizar modelo en hilo apropiado, persistir de forma atómica, notificar UI y permitir recuperación. No bloquear el hilo UI, no iniciar componentes pesados cuando la función no está abierta.
## Dependencias y restricciones
settings, bookmarks, sessions selected; encrypted when sensitive. La capa de interfaz no llama directamente a SQL, WebEngine o IPC sin adapter. Se adoptan guardas cuando se cierra pestaña/ventana durante operación; referencias Qt weak (`QPointer`) cuando proceda; cancelación explícita y disconnect limpio.
## Concurrencia y seguridad
El estado del modelo debe tener propietario claro; comandos serializados o transacciones según el agregado. Proteger del contenido de web no confiable y del agente sin permisos. Nunca filtrar datos privados entre perfiles. Persistir identificadores estables versionados. Eventos no propagados a nube.
## Evolución
Fase 1 contrato + estado visual y tests; fase 2 integración real; fase 3 error/recovery/a11y/security; fase 4 performance y documentación final. Revertir mediante feature flag local y restaurar schema/archivos desde backup validado.
