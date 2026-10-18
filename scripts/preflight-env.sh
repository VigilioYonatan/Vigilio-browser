#!/usr/bin/env bash
# ============================================================
# preflight-env.sh — Senior preflight operativo. Solo genera el
# reporte y probes temporales de escritura que elimina al instante.
# Fail-closed: cualquier FAIL aborta
# antes de build-all / queue. Ver Apéndice C del master.
# Uso: ./scripts/preflight-env.sh [--project-root .] [--config scripts/preflight-services.env]
# ============================================================
set -uo pipefail

ROOT="."
CONFIG=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root) ROOT="${2:-.}"; shift 2 ;;
    --config) CONFIG="${2:-}"; shift 2 ;;
    *) printf 'FAIL: argumento preflight no soportado: %s\n' "$1" >&2; exit 2 ;;
  esac
done
cd "$ROOT" || exit 1

FAIL=0
WARN=0
ok()   { printf 'OK: %s\n' "$1"; }
bad()  { printf 'FAIL: %s\n' "$1"; FAIL=1; }
warn() { printf 'WARN: %s\n' "$1"; WARN=$((WARN + 1)); }
section() { printf '\n=== %s ===\n' "$1"; }

mkdir -p .prompt-runner
REPORT=".prompt-runner/preflight-latest.log"
exec > >(tee "$REPORT") 2>&1

CONFIG="${CONFIG:-scripts/preflight-services.env}"
PREFLIGHT_REQUIRE_POSTGRES="${PREFLIGHT_REQUIRE_POSTGRES:-auto}"
PREFLIGHT_POSTGRES_URL_ENV="${PREFLIGHT_POSTGRES_URL_ENV:-DATABASE_URL}"
PREFLIGHT_REQUIRE_REDIS="${PREFLIGHT_REQUIRE_REDIS:-auto}"
PREFLIGHT_REDIS_URL_ENV="${PREFLIGHT_REDIS_URL_ENV:-REDIS_URL}"
PREFLIGHT_REQUIRE_DOCKER="${PREFLIGHT_REQUIRE_DOCKER:-auto}"
PREFLIGHT_REQUIRE_UI="${PREFLIGHT_REQUIRE_UI:-auto}"
PREFLIGHT_REQUIRE_TARGET_PROFILES="${PREFLIGHT_REQUIRE_TARGET_PROFILES:-auto}"
PREFLIGHT_ASSETS_DIR="${PREFLIGHT_ASSETS_DIR:-assets}"
PREFLIGHT_REQUIRED_FILES="${PREFLIGHT_REQUIRED_FILES:-}"
PREFLIGHT_REQUIRED_DIRECTORIES="${PREFLIGHT_REQUIRED_DIRECTORIES:-}"
PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES="${PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES:-}"
PREFLIGHT_REQUIRED_TCP="${PREFLIGHT_REQUIRED_TCP:-}"
PREFLIGHT_REQUIRED_HTTP="${PREFLIGHT_REQUIRED_HTTP:-}"
PREFLIGHT_REQUIRED_PROCESSES="${PREFLIGHT_REQUIRED_PROCESSES:-}"
PREFLIGHT_PROMPT_BUILDER_VERSION="${PREFLIGHT_PROMPT_BUILDER_VERSION:-0.6.3}"
PREFLIGHT_PROMPT_BUILDER_CLI="${PREFLIGHT_PROMPT_BUILDER_CLI:-${VIGILIO_PROMPT_BUILDER_CLI:-}}"

if [[ -f "$CONFIG" ]]; then
  while IFS='=' read -r key value; do
    key="${key%%[[:space:]]*}"
    [[ -z "$key" || "$key" == \#* ]] && continue
    case "$key" in
      PREFLIGHT_REQUIRE_POSTGRES|PREFLIGHT_POSTGRES_URL_ENV|PREFLIGHT_REQUIRE_REDIS|PREFLIGHT_REDIS_URL_ENV|PREFLIGHT_REQUIRE_DOCKER|PREFLIGHT_REQUIRE_UI|PREFLIGHT_REQUIRE_TARGET_PROFILES|PREFLIGHT_ASSETS_DIR|PREFLIGHT_REQUIRED_FILES|PREFLIGHT_REQUIRED_DIRECTORIES|PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES|PREFLIGHT_REQUIRED_TCP|PREFLIGHT_REQUIRED_HTTP|PREFLIGHT_REQUIRED_PROCESSES|PREFLIGHT_PROMPT_BUILDER_VERSION|PREFLIGHT_PROMPT_BUILDER_CLI)
        printf -v "$key" '%s' "$value"; export "$key" ;;
      *) bad "clave no soportada en $CONFIG: $key" ;;
    esac
  done < "$CONFIG"
fi

policy_required() {
  local policy="${1:-auto}" signal="${2:-}"
  case "${policy,,}" in
    true|1|yes|required) return 0 ;;
    false|0|no|disabled) return 1 ;;
    auto) [[ -n "$signal" ]] ;;
    *) bad "policy inválida: $policy"; return 1 ;;
  esac
}

indirect_value() { local name="${1:-}"; [[ -n "$name" ]] && printf '%s' "${!name:-}"; }
split_specs() { IFS=';' read -r -a SPECS <<< "${1:-}"; }
process_running() {
  pgrep -f "$1" >/dev/null 2>&1 && return 0
  command -v tasklist.exe >/dev/null 2>&1 && tasklist.exe 2>/dev/null | tr -d '\r' | grep -Eiq "$1"
}
check_tcp() {
  local name="$1" target="$2" host="${2%:*}" port="${2##*:}"
  if command -v nc >/dev/null 2>&1; then
    nc -z -w 3 "$host" "$port" >/dev/null 2>&1 && ok "$name TCP responde" || bad "$name TCP no responde"
  elif command -v timeout >/dev/null 2>&1; then
    timeout 3 bash -c "</dev/tcp/$host/$port" >/dev/null 2>&1 && ok "$name TCP responde" || bad "$name TCP no responde"
  else
    bad "$name no se puede verificar: falta nc/timeout"
  fi
}
test_writable_directory() {
  local target="$1" probe
  [[ -d "$target" ]] || return 1
  probe="$target/.vigilio-write-probe-$$-${RANDOM:-0}.tmp"
  if (umask 077; : > "$probe") 2>/dev/null; then
    rm -f -- "$probe"
    return 0
  fi
  rm -f -- "$probe" 2>/dev/null || true
  return 1
}

printf 'VIGILIO PREFLIGHT %s | config=%s | runner_os=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$CONFIG" "${VIGILIO_RUNNER_OS:-unknown}"

section "0. DIRS DERIVADOS (auto-creación, Apéndice E.2)"
mkdir -p generated/prompts .prompt-runner openspec/changes && ok "dirs derivados asegurados" || bad "no se pudieron crear dirs derivados"

section "1. Toolchain (node/pnpm/vigilio-prompt)"
command -v node >/dev/null 2>&1 && ok "node $(node --version)" || bad "node ausente"
command -v pnpm >/dev/null 2>&1 && ok "pnpm $(pnpm --version)" || bad "pnpm ausente"
if [[ -n "$PREFLIGHT_PROMPT_BUILDER_CLI" ]]; then
  PB_CLI="$PREFLIGHT_PROMPT_BUILDER_CLI"
  [[ "$PB_CLI" = /* ]] || PB_CLI="$PWD/$PB_CLI"
  PB="$(node "$PB_CLI" --version 2>/dev/null || true)"
else
  PB="$(pnpm exec vigilio-prompt --version 2>/dev/null || true)"
fi
[[ "$PB" == "$PREFLIGHT_PROMPT_BUILDER_VERSION" ]] && ok "vigilio-prompt $PB" || bad "vigilio-prompt=$PB esperado=$PREFLIGHT_PROMPT_BUILDER_VERSION"

section "2. Registry/answers/baseline"
[[ -f docs/rules/registry.yaml ]] && ok "registry.yaml existe" || bad "falta docs/rules/registry.yaml"
[[ -f docs/rules/answers.md ]] && ok "answers.md existe" || bad "falta docs/rules/answers.md"
[[ -f docs/rules/factory-baseline.yaml ]] && ok "factory-baseline.yaml existe" || bad "falta factory-baseline.yaml"
TARGET_PROFILES_SIGNAL=""
[[ -s docs/rules/target-profiles.yaml ]] && TARGET_PROFILES_SIGNAL="present"
if policy_required "$PREFLIGHT_REQUIRE_TARGET_PROFILES" "$TARGET_PROFILES_SIGNAL"; then
  [[ -s docs/rules/target-profiles.yaml ]] && ok "target-profiles.yaml existe" || bad "falta target-profiles.yaml"
else
  warn "target profiles no requeridos por policy; eligibility valida los packs que los adopten"
fi

section "3. Targets declarados"
ok "la policy PREFLIGHT_REQUIRED_DIRECTORIES/FILES valida los targets sin asumir un layout local"

section "4. Runtime: Docker, PostgreSQL y Redis/Valkey"
if policy_required "$PREFLIGHT_REQUIRE_DOCKER" "${DOCKER_HOST:-}"; then
  command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1 && ok "docker responde" || bad "docker/daemon no disponible"
else
  warn "docker no requerido por policy"
fi
POSTGRES_URL="$(indirect_value "$PREFLIGHT_POSTGRES_URL_ENV")"
if policy_required "$PREFLIGHT_REQUIRE_POSTGRES" "$POSTGRES_URL"; then
  if ! command -v pg_isready >/dev/null 2>&1; then
    check_tcp "postgres" "127.0.0.1:5432"
  elif [[ -n "$POSTGRES_URL" ]]; then
    pg_isready -d "$POSTGRES_URL" >/dev/null 2>&1 && ok "postgres responde" || bad "postgres no responde"
  else
    pg_isready >/dev/null 2>&1 && ok "postgres responde" || bad "postgres no responde"
  fi
else
  warn "postgres no requerido por policy"
fi
REDIS_URL="$(indirect_value "$PREFLIGHT_REDIS_URL_ENV")"
if policy_required "$PREFLIGHT_REQUIRE_REDIS" "$REDIS_URL"; then
  if ! command -v redis-cli >/dev/null 2>&1; then
    check_tcp "redis" "127.0.0.1:6379"
  elif [[ -n "$REDIS_URL" ]]; then
    [[ "$(redis-cli -u "$REDIS_URL" --no-auth-warning ping 2>/dev/null)" == "PONG" ]] && ok "redis responde" || bad "redis no responde"
  else
    [[ "$(redis-cli ping 2>/dev/null)" == "PONG" ]] && ok "redis responde" || bad "redis no responde"
  fi
else
  warn "redis no requerido por policy"
fi

section "4.1 Archivos/directorios/servicios declarados"
split_specs "$PREFLIGHT_REQUIRED_DIRECTORIES"
for spec in "${SPECS[@]}"; do
  [[ -z "$spec" ]] && continue
  name="${spec%%=*}"; target="${spec#*=}"
  [[ "$name" != "$target" && -d "$target" ]] && ok "$name directorio disponible" || bad "$name directorio ausente"
done
split_specs "$PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES"
for spec in "${SPECS[@]}"; do
  [[ -z "$spec" ]] && continue
  name="${spec%%=*}"; target="${spec#*=}"
  [[ "$name" != "$target" ]] && test_writable_directory "$target" && ok "$name directorio escribible" || bad "$name directorio sin permiso de escritura"
done
split_specs "$PREFLIGHT_REQUIRED_FILES"
for spec in "${SPECS[@]}"; do
  [[ -z "$spec" ]] && continue
  name="${spec%%=*}"; target="${spec#*=}"
  [[ "$name" != "$target" && -s "$target" ]] && ok "$name archivo disponible" || bad "$name archivo ausente o vacío"
done
split_specs "$PREFLIGHT_REQUIRED_TCP"
for spec in "${SPECS[@]}"; do [[ -z "$spec" ]] || check_tcp "${spec%%=*}" "${spec#*=}"; done
split_specs "$PREFLIGHT_REQUIRED_HTTP"
for spec in "${SPECS[@]}"; do
  [[ -z "$spec" ]] && continue
  name="${spec%%=*}"; url="${spec#*=}"
  command -v curl >/dev/null 2>&1 && curl --silent --fail --max-time 5 "$url" >/dev/null 2>&1 && ok "$name HTTP responde" || bad "$name HTTP no responde"
done
split_specs "$PREFLIGHT_REQUIRED_PROCESSES"
for spec in "${SPECS[@]}"; do
  [[ -z "$spec" ]] && continue
  process_running "${spec#*=}" && ok "${spec%%=*} proceso abierto" || bad "${spec%%=*} proceso no abierto"
done

section "5. AWS/servicios (solo si el slice los adopta, C-004/C-012)"
echo "Regla: ningún servicio (SQS/SNS/S3/Secrets) se asume adoptado sin consumer/contrato real en docs + change."
command -v aws >/dev/null 2>&1 && ok "aws cli presente" || warn "aws cli ausente (solo importa si el slice usa AWS)"

section "6. JS: lockfile + pins"
if [[ -f package.json ]]; then
  [[ -f pnpm-lock.yaml ]] && ok "pnpm-lock.yaml existe" || bad "falta pnpm-lock.yaml"
  [[ -d node_modules ]] && ok "node_modules instalado" || warn "ejecuta pnpm install --frozen-lockfile"
else
  ok "workspace documental sin package.json local; Prompt Builder externo ya fue validado"
fi

section "7. Prompt Builder (modo correcto)"
if [[ -n "$PREFLIGHT_PROMPT_BUILDER_CLI" ]]; then
  node "$PB_CLI" validate-references --project-root . >/dev/null 2>&1 && ok "validate-references" || bad "validate-references falló"
else
  pnpm exec vigilio-prompt validate-references --project-root . >/dev/null 2>&1 && ok "validate-references" || bad "validate-references falló"
fi
echo "Exige plan enabled-packs (77/231) o plan-file; prohibido --include-context-packs para implementar."

section "8. UI runner (solo si hay cola UI)"
echo "DISPLAY=${DISPLAY:-<vacio>} (esperado :1)"
UI_SIGNAL=""
[[ "${VIGILIO_RUNNER_OS:-}" == "windows" || -n "${DISPLAY:-}" ]] && UI_SIGNAL="configured"
if policy_required "$PREFLIGHT_REQUIRE_UI" "$UI_SIGNAL"; then
  if [[ "${VIGILIO_RUNNER_OS:-linux}" == "linux" ]]; then
    [[ -n "${DISPLAY:-}" ]] && ok "DISPLAY configurado" || bad "DISPLAY no configurado"
  fi
  [[ -d "$PREFLIGHT_ASSETS_DIR" ]] && ok "assets existen: $PREFLIGHT_ASSETS_DIR" || bad "falta directorio de assets: $PREFLIGHT_ASSETS_DIR"
  for a in chat-gpt-search-1.png chat-gpt-button-send.png chat-gpt-6-likes.png chat-gpt-new-chat-1.jpg; do
    [[ -f "$PREFLIGHT_ASSETS_DIR/$a" ]] && ok "asset $a" || bad "falta $PREFLIGHT_ASSETS_DIR/$a"
  done
else
  warn "UI no requerida por policy"
fi

echo
if [[ "$FAIL" -eq 0 ]]; then echo "PREFLIGHT-ENV: PASS — puedes hacer build-all + queue. WARN=$WARN"; else echo "PREFLIGHT-ENV: FAIL — corrige los FAIL antes de automatizar. WARN=$WARN"; fi
exit "$FAIL"
