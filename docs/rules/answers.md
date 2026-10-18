# Human Decision Register
Project mode: greenfield

## Q-001 — Modelo de identidad
- Estado: ANSWERED
- Por qué requiere decisión humana: privacidad y arquitectura.
- Opciones: A Sin cuentas Vigilio; B servicio cloud.
- Recomendación: A.
- Decisión: Sin login, cuentas, nube, sincronización ni telemetría remota de Vigilio; login en sitios ajenos permitido.
- Provenance: decisión explícita de usuario en conversación.
- Bloquea:
  - none
- Owners a sincronizar: project-brief, architecture, privacy, installer, mcp.
- Fecha: 2026-10-16

## Q-002 — Tecnología base
- Estado: ANSWERED
- Por qué requiere decisión humana: arquitectura y runtime.
- Opciones: A C++20/Qt6.10+/WebEngine/SQLite/CMake; B Rust/GPUI.
- Recomendación: A.
- Decisión: A; Rust/GPUI aplica a Vigilio Code, no Browser.
- Provenance: decisiones previas del usuario.
- Bloquea:
  - none
- Owners a sincronizar: architecture, app, docs.
- Fecha: 2026-10-16

## Q-003 — Modalidad de código
- Estado: ANSWERED
- Por qué requiere decisión humana: distribución y licencias.
- Opciones: A abierto; B propietario.
- Recomendación: A.
- Decisión: código abierto; licencia exacta pendiente de aprobación, propuesta MIT.
- Provenance: confirmación textual del usuario.
- Bloquea:
  - none
- Owners a sincronizar: LICENSE, delivery.
- Fecha: 2026-10-16

## Q-004 — Plataformas
- Estado: ANSWERED
- Por qué requiere decisión humana: empaquetado y soporte.
- Opciones: A Win10/11 x64, Linux x64, macOS x64/arm64; B subgrupo.
- Recomendación: A sujeto a Qt y pruebas.
- Decisión: A como target de soporte; restricciones de Qt se verificarán.
- Provenance: selección de primera opción del usuario.
- Bloquea:
  - none
- Owners a sincronizar: architecture, delivery, test-matrix.
- Fecha: 2026-10-16

## Q-005 — Actualizaciones
- Estado: ANSWERED
- Por qué requiere decisión humana: privacidad y red.
- Opciones: A comprobación manual; B automática opcional.
- Recomendación: A.
- Decisión: A; no comprobación periódica oculta.
- Provenance: selección de primera opción del usuario.
- Bloquea:
  - none
- Owners a sincronizar: delivery, installer.
- Fecha: 2026-10-16

## Q-006 — Ubicación de assets
- Estado: ANSWERED
- Por qué requiere decisión humana: empaquetado de assets.
- Opciones: A apps/vigilio-browser/assets; B raíz del monorepo.
- Recomendación: A.
- Decisión: todos los recursos consumidos por la aplicación dentro de apps/vigilio-browser/assets, bajo rutas relativas y empaquetado explícito.
- Provenance: decisión explícita del usuario.
- Bloquea:
  - none
- Owners a sincronizar: architecture, asset-inventory.
- Fecha: 2026-10-16

## Q-007 — Licencia jurídica concreta
- Estado: OPEN
- Por qué requiere decisión humana: publicar un repositorio como software open source requiere revisar licencia, atribuciones y titularidad del código.
- Opciones: A MIT; B Apache-2.0; C MPL-2.0; D otra aprobada.
- Recomendación: MIT para un proyecto permisivo de escritorio, sujeta a decisión del titular.
- Decisión: PENDING — no afirmar licencia escogida.
- Provenance: usuario aprobó código abierto, no eligió licencia.
- Bloquea:
  - release.public-release
- Owners a sincronizar: README, LICENSE, packaging, notices.
- Fecha: PENDING

## Q-008 — Firma y notarización de builds
- Estado: ANSWERED
- Por qué requiere decisión humana: las versiones finales Windows/macOS requieren identidades y firma correctas para distribución confiable.
- Opciones: A certificados ya existen; B aprovisionar antes de release.
- Recomendación: B.
- Decisión: PENDING, no pedir secretos.
- Provenance: requisitos packaging.
- Bloquea:
  - none
- Owners a sincronizar: delivery, packaging.
- Fecha: PENDING
