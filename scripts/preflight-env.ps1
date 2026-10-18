param(
  [string]$ProjectRoot = ".",
  [string]$Config = ""
)

$ErrorActionPreference = "Continue"
Set-Location -LiteralPath (Resolve-Path -LiteralPath $ProjectRoot)
New-Item -ItemType Directory -Path ".prompt-runner" -Force | Out-Null
$report = Join-Path (Get-Location) ".prompt-runner\preflight-latest.log"
$script:failed = $false
$script:warnings = 0
$lines = [System.Collections.Generic.List[string]]::new()

function Emit([string]$Text) { $lines.Add($Text); Write-Output $Text }
function Ok([string]$Text) { Emit "OK: $Text" }
function Bad([string]$Text) { Emit "FAIL: $Text"; $script:failed = $true }
function Warn([string]$Text) { Emit "WARN: $Text"; $script:warnings++ }
function Section([string]$Text) { Emit ""; Emit "=== $Text ===" }

$settings = @{
  PREFLIGHT_REQUIRE_POSTGRES = "auto"
  PREFLIGHT_POSTGRES_URL_ENV = "DATABASE_URL"
  PREFLIGHT_REQUIRE_REDIS = "auto"
  PREFLIGHT_REDIS_URL_ENV = "REDIS_URL"
  PREFLIGHT_REQUIRE_DOCKER = "auto"
  PREFLIGHT_REQUIRE_UI = "auto"
  PREFLIGHT_REQUIRE_TARGET_PROFILES = "auto"
  PREFLIGHT_ASSETS_DIR = if ($env:PREFLIGHT_ASSETS_DIR) { $env:PREFLIGHT_ASSETS_DIR } else { "assets" }
  PREFLIGHT_REQUIRED_FILES = ""
  PREFLIGHT_REQUIRED_DIRECTORIES = ""
  PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES = ""
  PREFLIGHT_REQUIRED_TCP = ""
  PREFLIGHT_REQUIRED_HTTP = ""
  PREFLIGHT_REQUIRED_PROCESSES = ""
  PREFLIGHT_PROMPT_BUILDER_VERSION = "0.6.3"
  PREFLIGHT_PROMPT_BUILDER_CLI = if ($env:VIGILIO_PROMPT_BUILDER_CLI) { $env:VIGILIO_PROMPT_BUILDER_CLI } else { "" }
}

if (-not $Config) { $Config = "scripts/preflight-services.env" }
if (Test-Path -LiteralPath $Config) {
  foreach ($line in Get-Content -LiteralPath $Config) {
    $trimmed = $line.Trim()
    if (-not $trimmed -or $trimmed.StartsWith("#")) { continue }
    $parts = $trimmed.Split("=", 2)
    if ($parts.Count -ne 2 -or -not $settings.ContainsKey($parts[0])) {
      Bad "unsupported key in ${Config}: $($parts[0])"
      continue
    }
    $settings[$parts[0]] = $parts[1]
  }
}

function EnvValue([string]$Name) {
  if (-not $Name) { return "" }
  return [Environment]::GetEnvironmentVariable($Name)
}

function IsRequired([string]$Policy, [string]$Signal = "") {
  switch ($Policy.ToLowerInvariant()) {
    { $_ -in @("true", "1", "yes", "required") } { return $true }
    { $_ -in @("false", "0", "no", "disabled") } { return $false }
    "auto" { return -not [string]::IsNullOrWhiteSpace($Signal) }
    default { Bad "invalid readiness policy: $Policy"; return $false }
  }
}

function Specs([string]$Raw) {
  if (-not $Raw) { return @() }
  return @($Raw.Split(";", [StringSplitOptions]::RemoveEmptyEntries))
}

function TestTcp([string]$HostName, [int]$Port) {
  try {
    $client = [Net.Sockets.TcpClient]::new()
    $task = $client.ConnectAsync($HostName, $Port)
    $ready = $task.Wait(3000) -and $client.Connected
    $client.Dispose()
    return $ready
  } catch { return $false }
}

function TestWritableDirectory([string]$Path) {
  if (-not (Test-Path -LiteralPath $Path -PathType Container)) { return $false }
  $probe = Join-Path $Path ".vigilio-write-probe-$PID-$([Guid]::NewGuid().ToString('N')).tmp"
  try {
    $stream = [IO.File]::Open($probe, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    $stream.Dispose()
    Remove-Item -LiteralPath $probe -Force
    return $true
  } catch {
    if (Test-Path -LiteralPath $probe) { Remove-Item -LiteralPath $probe -Force -ErrorAction SilentlyContinue }
    return $false
  }
}

Emit "VIGILIO PREFLIGHT $([DateTime]::UtcNow.ToString('o'))"
Emit "project_root=$(Get-Location) config=$Config runner_os=$($env:VIGILIO_RUNNER_OS)"

Section "Toolchain and canonical control plane"
foreach ($command in @("node", "pnpm")) {
  $resolved = Get-Command $command -ErrorAction SilentlyContinue
  if ($resolved) { Ok "$command available" } else { Bad "$command unavailable" }
}
foreach ($file in @("docs/rules/registry.yaml", "docs/rules/answers.md", "docs/rules/factory-baseline.yaml")) {
  if ((Test-Path -LiteralPath $file) -and (Get-Item -LiteralPath $file).Length -gt 0) { Ok $file } else { Bad "$file missing or empty" }
}
$targetProfilesSignal = if ((Test-Path -LiteralPath "docs/rules/target-profiles.yaml") -and (Get-Item -LiteralPath "docs/rules/target-profiles.yaml").Length -gt 0) { "present" } else { "" }
if (IsRequired $settings.PREFLIGHT_REQUIRE_TARGET_PROFILES $targetProfilesSignal) {
  if ($targetProfilesSignal) { Ok "docs/rules/target-profiles.yaml" } else { Bad "docs/rules/target-profiles.yaml missing or empty" }
} else { Warn "target profiles not required by current policy; pack eligibility remains authoritative" }
if ($settings.PREFLIGHT_PROMPT_BUILDER_CLI) {
  $pbCli = $settings.PREFLIGHT_PROMPT_BUILDER_CLI
  if (-not [IO.Path]::IsPathRooted($pbCli)) { $pbCli = Join-Path (Get-Location) $pbCli }
  if ((Test-Path -LiteralPath $pbCli) -and (Get-Command node -ErrorAction SilentlyContinue)) {
    $pb = (& node $pbCli --version 2>$null | Select-Object -Last 1)
  } else { $pb = "" }
} elseif (Get-Command pnpm -ErrorAction SilentlyContinue) {
  $pb = (& pnpm exec vigilio-prompt --version 2>$null | Select-Object -Last 1)
} else { $pb = "" }
if ($pb) {
  if ($pb -eq $settings.PREFLIGHT_PROMPT_BUILDER_VERSION) { Ok "vigilio-prompt $pb" } else { Bad "vigilio-prompt unavailable/mismatch" }
} else { Bad "vigilio-prompt unavailable/mismatch" }

Section "Container runtime"
if (IsRequired $settings.PREFLIGHT_REQUIRE_DOCKER $env:DOCKER_HOST) {
  if ((Get-Command docker -ErrorAction SilentlyContinue) -and ((& docker info 2>$null) -ne $null)) { Ok "docker daemon ready" } else { Bad "docker daemon unavailable" }
} else { Warn "docker not required by current policy" }

Section "PostgreSQL"
$postgresUrl = EnvValue $settings.PREFLIGHT_POSTGRES_URL_ENV
if (IsRequired $settings.PREFLIGHT_REQUIRE_POSTGRES $postgresUrl) {
  if (Get-Command pg_isready -ErrorAction SilentlyContinue) {
    if ($postgresUrl) { & pg_isready -d $postgresUrl *> $null } else { & pg_isready *> $null }
    if ($LASTEXITCODE -eq 0) { Ok "postgres ready" } else { Bad "postgres unavailable" }
  } elseif (TestTcp "127.0.0.1" 5432) { Ok "postgres TCP ready (pg_isready unavailable)" }
  else { Bad "postgres unavailable (pg_isready absent; TCP 127.0.0.1:5432 failed)" }
} else { Warn "postgres not required by current policy" }

Section "Redis/Valkey"
$redisUrl = EnvValue $settings.PREFLIGHT_REDIS_URL_ENV
if (IsRequired $settings.PREFLIGHT_REQUIRE_REDIS $redisUrl) {
  if (Get-Command redis-cli -ErrorAction SilentlyContinue) {
    if ($redisUrl) { $pong = & redis-cli -u $redisUrl --no-auth-warning ping 2>$null } else { $pong = & redis-cli ping 2>$null }
    if ($pong -eq "PONG") { Ok "redis ready" } else { Bad "redis unavailable" }
  } elseif (TestTcp "127.0.0.1" 6379) { Ok "redis TCP ready (redis-cli unavailable)" }
  else { Bad "redis unavailable (redis-cli absent; TCP 127.0.0.1:6379 failed)" }
} else { Warn "redis not required by current policy" }

Section "Declared files/directories/services"
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_DIRECTORIES) {
  $pair = $spec.Split("=", 2)
  if ($pair.Count -ne 2 -or -not (Test-Path -LiteralPath $pair[1] -PathType Container)) { Bad "$($pair[0]) directory missing" }
  else { Ok "$($pair[0]) directory ready" }
}
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_WRITABLE_DIRECTORIES) {
  $pair = $spec.Split("=", 2)
  if ($pair.Count -ne 2 -or -not (TestWritableDirectory $pair[1])) { Bad "$($pair[0]) directory is not writable" }
  else { Ok "$($pair[0]) directory writable" }
}
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_FILES) {
  $pair = $spec.Split("=", 2)
  if ($pair.Count -ne 2 -or -not (Test-Path -LiteralPath $pair[1] -PathType Leaf) -or (Get-Item -LiteralPath $pair[1]).Length -eq 0) { Bad "$($pair[0]) file missing or empty" }
  else { Ok "$($pair[0]) file ready" }
}
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_TCP) {
  $pair = $spec.Split("=", 2); $name = $pair[0]; $target = $pair[1]
  $hostPort = $target.Split(":", 2)
  if ($hostPort.Count -ne 2) { Bad "$name has invalid TCP target"; continue }
  if (TestTcp $hostPort[0] ([int]$hostPort[1])) { Ok "$name TCP ready" } else { Bad "$name TCP unavailable" }
}
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_HTTP) {
  $pair = $spec.Split("=", 2); $name = $pair[0]; $url = $pair[1]
  try { Invoke-WebRequest -UseBasicParsing -TimeoutSec 5 -Uri $url | Out-Null; Ok "$name HTTP ready" } catch { Bad "$name HTTP unavailable" }
}
foreach ($spec in Specs $settings.PREFLIGHT_REQUIRED_PROCESSES) {
  $pair = $spec.Split("=", 2); $name = $pair[0]; $pattern = $pair[1]
  $found = Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.Name -match $pattern -or $_.CommandLine -match $pattern } | Select-Object -First 1
  if ($found) { Ok "$name process ready" } else { Bad "$name process unavailable" }
}

Section "ChatGPT UI prerequisites"
$uiSignal = if ($env:VIGILIO_RUNNER_OS -eq "windows") { "windows" } else { "" }
if (IsRequired $settings.PREFLIGHT_REQUIRE_UI $uiSignal) {
  foreach ($assetName in @("chat-gpt-search-1.png", "chat-gpt-button-send.png", "chat-gpt-4-likes.png")) {
    $asset = Join-Path $settings.PREFLIGHT_ASSETS_DIR $assetName
    if ((Test-Path -LiteralPath $asset) -and (Get-Item -LiteralPath $asset).Length -gt 0) { Ok $asset } else { Bad "$asset missing" }
  }
  $readyPng = Join-Path $settings.PREFLIGHT_ASSETS_DIR "chat-gpt-6-likes.png"
  $readyJpg = Join-Path $settings.PREFLIGHT_ASSETS_DIR "chat-gpt-6-likes.jpg"
  if ((Test-Path -LiteralPath $readyPng) -or (Test-Path -LiteralPath $readyJpg)) { Ok "READY asset" } else { Bad "READY asset missing" }
} else { Warn "UI readiness not required by current policy" }

Emit ""
$status = if ($script:failed) { "FAIL" } else { "PASS" }
Emit "PREFLIGHT-ENV: $status (warnings=$($script:warnings) report=$report)"
[IO.File]::WriteAllLines($report, $lines, [Text.UTF8Encoding]::new($false))
if ($script:failed) { exit 1 }
exit 0
