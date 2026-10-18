#include "core/CMakePresetsService.h"
#include "core/CMakePresetsTypes.h"
#include "core/CMakePresetsValidator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error(std::string("CHECK failed at line ") + std::to_string(__LINE__) +  \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

const std::string CANONICAL_VALID_JSON = R"({
  "version": 6,
  "configurePresets": [
    {
      "name": "core-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-core-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "OFF",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "core-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-core-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_BUILD_GUI": "OFF",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "desktop-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "ON",
        "VIGILIO_ENABLE_E2E": "OFF"
      }
    },
    {
      "name": "desktop-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "OFF",
        "VIGILIO_ENABLE_E2E": "OFF"
      }
    },
    {
      "name": "desktop-e2e",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-e2e",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "ON",
        "VIGILIO_ENABLE_E2E": "ON"
      }
    },
    {
      "name": "ci-linux-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ci-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_WARNINGS_AS_ERRORS": "ON",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "ci-linux-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ci-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_WARNINGS_AS_ERRORS": "ON"
      }
    },
    {
      "name": "sanitizer-asan",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-asan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "sanitizer-ubsan",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ubsan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTING": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "core-debug", "configurePreset": "core-debug" },
    { "name": "core-release", "configurePreset": "core-release" },
    { "name": "desktop-debug", "configurePreset": "desktop-debug" },
    { "name": "desktop-release", "configurePreset": "desktop-release" },
    { "name": "desktop-e2e", "configurePreset": "desktop-e2e" },
    { "name": "ci-linux-debug", "configurePreset": "ci-linux-debug" },
    { "name": "ci-linux-release", "configurePreset": "ci-linux-release" },
    { "name": "sanitizer-asan", "configurePreset": "sanitizer-asan" },
    { "name": "sanitizer-ubsan", "configurePreset": "sanitizer-ubsan" }
  ],
  "testPresets": [
    { "name": "core-debug", "configurePreset": "core-debug", "output": { "outputOnFailure": true } },
    { "name": "core-release", "configurePreset": "core-release", "output": { "outputOnFailure": true } },
    { "name": "desktop-debug", "configurePreset": "desktop-debug", "output": { "outputOnFailure": true } },
    { "name": "desktop-e2e", "configurePreset": "desktop-e2e", "output": { "outputOnFailure": true } },
    { "name": "ci-linux-debug", "configurePreset": "ci-linux-debug", "output": { "outputOnFailure": true } },
    { "name": "sanitizer-asan", "configurePreset": "sanitizer-asan", "output": { "outputOnFailure": true } },
    { "name": "sanitizer-ubsan", "configurePreset": "sanitizer-ubsan", "output": { "outputOnFailure": true } }
  ],
  "packagePresets": [
    { "name": "desktop-linux-deb" }
  ],
  "workflowPresets": [
    { "name": "ci-pipeline" }
  ]
})";

const std::string UNSAFE_PRESET_JSON = R"({
  "version": 6,
  "configurePresets": [
    { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
    { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" },
    { "name": "unsafe-preset", "generator": "Ninja", "binaryDir": "${sourceDir}/../../etc" },
    { "name": "safe-clean-preset", "generator": "Ninja", "binaryDir": "${sourceDir}/_build-test-clean" }
  ]
})";

// TEST-PRESET-SVC-001 y TEST-PRESET-SVC-002: Carga canónica en memoria y desde archivo real
void test_service_load_canonical() {
    CMakePresetsService svc;
    CHECK(!svc.isLoaded());

    // TEST-PRESET-SVC-001: Carga desde JSON en memoria
    const bool loadedMem = svc.loadFromJson(CANONICAL_VALID_JSON);
    CHECK(loadedMem);
    CHECK(svc.isLoaded());
    CHECK(svc.validationReport().isValid());

    const auto summary = svc.summary();
    CHECK(summary.schemaVersion == 6);
    CHECK(summary.totalConfigurePresets == 9);
    CHECK(summary.totalBuildPresets == 9);
    CHECK(summary.totalTestPresets == 7);
    CHECK(summary.totalPackagePresets == 1);
    CHECK(summary.totalWorkflowPresets == 1);
    CHECK(summary.configurePresetNames.size() == 9);
    CHECK(summary.buildPresetNames.size() == 9);
    CHECK(summary.testPresetNames.size() == 7);

    // TEST-PRESET-SVC-002: Carga desde archivo real CMakePresets.json
#ifdef VIGILIO_CMAKE_PRESETS_PATH
    CMakePresetsService fileSvc;
    const bool loadedFile = fileSvc.loadFromFile(VIGILIO_CMAKE_PRESETS_PATH);
    CHECK(loadedFile);
    CHECK(fileSvc.isLoaded());
    CHECK(fileSvc.validationReport().isValid());
    CHECK(fileSvc.summary().totalConfigurePresets >= 9);
#endif
}

// TEST-PRESET-SVC-003, TEST-PRESET-SVC-004, TEST-PRESET-SVC-005: Búsqueda y resolución de jerarquías
void test_service_find_and_resolve() {
    CMakePresetsService svc;
    CHECK(svc.loadFromJson(CANONICAL_VALID_JSON));

    // TEST-PRESET-SVC-003: Búsqueda de preset existente desktop-debug
    const auto preset = svc.findPreset("desktop-debug", CMakePresetKind::Configure);
    CHECK(preset.has_value());
    CHECK(preset->name == "desktop-debug");
    CHECK(preset->generator == "Ninja");
    CHECK(preset->binaryDir == "${sourceDir}/build-desktop-debug");
    CHECK(preset->cacheVariables.at("CMAKE_BUILD_TYPE") == "Debug");
    CHECK(preset->cacheVariables.at("VIGILIO_BUILD_GUI") == "ON");

    // TEST-PRESET-SVC-004: Búsqueda de preset inexistente
    const auto missing = svc.findPreset("fake-preset", CMakePresetKind::Configure);
    CHECK(!missing.has_value());

    // TEST-PRESET-SVC-005: Resolución de configurePreset asociado
    const std::string cfgForBuild =
        svc.resolveAssociatedConfigurePreset("desktop-debug", CMakePresetKind::Build);
    CHECK(cfgForBuild == "desktop-debug");

    const std::string cfgForTest =
        svc.resolveAssociatedConfigurePreset("sanitizer-asan", CMakePresetKind::Test);
    CHECK(cfgForTest == "sanitizer-asan");

    const std::string cfgForDirect =
        svc.resolveAssociatedConfigurePreset("core-debug", CMakePresetKind::Configure);
    CHECK(cfgForDirect == "core-debug");

    const std::string cfgMissing =
        svc.resolveAssociatedConfigurePreset("unknown-build", CMakePresetKind::Build);
    CHECK(cfgMissing.empty());
}

// TEST-PRESET-SVC-006, TEST-PRESET-SVC-007, TEST-PRESET-SVC-008: Síntesis determinista de CLI
void test_service_command_line_synthesis() {
    CMakePresetsService svc;
    CHECK(svc.loadFromJson(CANONICAL_VALID_JSON, "/workspace/my-browser"));

    // TEST-PRESET-SVC-006: Síntesis de comando configure
    const std::string cfgCmd = svc.buildConfigureCommandLine("core-debug");
    CHECK(cfgCmd == "cmake --preset core-debug");

    CMakePresetCommandOptions cfgOpts;
    cfgOpts.additionalArgs = {"-DCMAKE_VERBOSE_MAKEFILE=ON", "-B", "${sourceDir}/custom-build"};
    const std::string cfgCmdCustom = svc.buildConfigureCommandLine("core-debug", cfgOpts);
    CHECK(cfgCmdCustom ==
          "cmake --preset core-debug -DCMAKE_VERBOSE_MAKEFILE=ON -B /workspace/my-browser/custom-build");

    // TEST-PRESET-SVC-007: Síntesis de comando build
    const std::string bldCmd = svc.buildBuildCommandLine("core-release");
    CHECK(bldCmd == "cmake --build --preset core-release");

    CMakePresetCommandOptions bldOpts;
    bldOpts.additionalArgs = {"--parallel", "4", "--target", "vigilio_core"};
    const std::string bldCmdCustom = svc.buildBuildCommandLine("core-release", bldOpts);
    CHECK(bldCmdCustom == "cmake --build --preset core-release --parallel 4 --target vigilio_core");

    // TEST-PRESET-SVC-008: Síntesis de comando test
    const std::string tstCmd = svc.buildTestCommandLine("sanitizer-asan");
    CHECK(tstCmd == "ctest --preset sanitizer-asan");

    CMakePresetCommandOptions tstOpts;
    tstOpts.additionalArgs = {"-R", "unit", "--verbose"};
    const std::string tstCmdCustom = svc.buildTestCommandLine("sanitizer-asan", tstOpts);
    CHECK(tstCmdCustom == "ctest --preset sanitizer-asan -R unit --verbose");
}

// TEST-PRESET-SVC-009: Ejecución en modo dryRun
void test_service_dry_run() {
    CMakePresetsService svc;
    CHECK(svc.loadFromJson(CANONICAL_VALID_JSON, "/tmp/vigilio-dry-run"));

    CMakePresetCommandOptions opts;
    opts.dryRun = true;

    // Configure dry-run
    const auto cfgRes = svc.executeConfigure("core-debug", opts);
    CHECK(cfgRes.isSuccess());
    CHECK(cfgRes.status == CMakePresetExecutionStatus::Success);
    CHECK(cfgRes.exitCode == 0);
    CHECK(cfgRes.commandLine == "cmake --preset core-debug");
    CHECK(cfgRes.workingDirectory == "/tmp/vigilio-dry-run");
    CHECK(cfgRes.standardOutput.find("[dry-run]") != std::string::npos);

    // Build dry-run
    const auto bldRes = svc.executeBuild("core-release", opts);
    CHECK(bldRes.isSuccess());
    CHECK(bldRes.status == CMakePresetExecutionStatus::Success);
    CHECK(bldRes.exitCode == 0);
    CHECK(bldRes.commandLine == "cmake --build --preset core-release");

    // Test dry-run
    const auto tstRes = svc.executeTest("sanitizer-asan", opts);
    CHECK(tstRes.isSuccess());
    CHECK(tstRes.status == CMakePresetExecutionStatus::Success);
    CHECK(tstRes.exitCode == 0);
    CHECK(tstRes.commandLine == "ctest --preset sanitizer-asan");
}

// TEST-PRESET-SVC-010: Cancelación cooperativa multihilo vía CancellationToken
void test_service_cancellation() {
    CMakePresetsService svc;
    CHECK(svc.loadFromJson(CANONICAL_VALID_JSON));

    CancellationToken token;
    CHECK(!token.isCancelled());

    token.requestCancellation();
    CHECK(token.isCancelled());

    const auto res = svc.executeConfigure("core-debug", {}, &token);
    CHECK(!res.isSuccess());
    CHECK(res.status == CMakePresetExecutionStatus::Cancelled);
    CHECK(res.exitCode == -1);
    CHECK(!res.remediationHint.empty());
}

// TEST-PRESET-SVC-011 y TEST-PRESET-SVC-012: Saneamiento y limpieza segura de binaryDir
void test_service_clean_binary_dir() {
    CMakePresetsService svc;
    const bool loaded = svc.loadFromJson(UNSAFE_PRESET_JSON, "/tmp/vigilio-clean-test");
    CHECK(!loaded);

    // TEST-PRESET-SVC-011: Rechazo estricto de ruta insegura (../../etc)
    std::string errMsg;
    const bool unsafeClean = svc.cleanBinaryDir("unsafe-preset", &errMsg);
    CHECK(!unsafeClean);
    CHECK(!errMsg.empty());
    CHECK(errMsg.find("no segura") != std::string::npos);

    // Preset inexistente
    const bool missingClean = svc.cleanBinaryDir("nonexistent-preset", &errMsg);
    CHECK(!missingClean);

    // TEST-PRESET-SVC-012: Limpieza segura de directorio temporal válido
    const std::filesystem::path testDir = "/tmp/vigilio-clean-test/_build-test-clean";
    std::error_code ec;
    std::filesystem::create_directories(testDir / "CMakeFiles", ec);
    std::ofstream cacheOut(testDir / "CMakeCache.txt");
    cacheOut << "# CMake cache file stub\nCMAKE_BUILD_TYPE:STRING=Debug\n";
    cacheOut.close();

    CHECK(std::filesystem::exists(testDir / "CMakeCache.txt"));
    CHECK(std::filesystem::exists(testDir / "CMakeFiles"));

    const bool safeClean = svc.cleanBinaryDir("safe-clean-preset", &errMsg);
    CHECK(safeClean);
    CHECK(!std::filesystem::exists(testDir / "CMakeCache.txt"));
    CHECK(!std::filesystem::exists(testDir / "CMakeFiles"));

    std::filesystem::remove_all("/tmp/vigilio-clean-test", ec);
}

// Validación de diagnósticos y copy técnico legible
void test_service_diagnostics_formatting() {
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::Success) ==
          "Operación completada exitosamente");
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::ValidationError) ==
          "Error de validación en la especificación de configuración");
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::PresetNotFound) ==
          "Perfil de configuración no encontrado");
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::BinaryDirUnsafe) ==
          "Directorio de destino no seguro o fuera de las políticas de contención");
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::Timeout) ==
          "Tiempo límite de espera superado para la operación");
    CHECK(CMakePresetsService::executionStatusToString(CMakePresetExecutionStatus::Cancelled) ==
          "Operación cancelada a petición del usuario");

    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Idle) == "En espera");
    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Configuring) ==
          "Configurando entorno de compilación");
    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Building) ==
          "Compilando proyecto");
    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Testing) ==
          "Ejecutando pruebas de verificación");
    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Completed) ==
          "Operación completada");
    CHECK(CMakePresetsService::executionStateToString(CMakePresetExecutionState::Cancelled) ==
          "Operación cancelada");
}

// TEST-PRESET-SVC-013: Consultas de lectura concurrentes multihilo (8 hilos)
void test_service_concurrency_multi_threaded_reads() {
    CMakePresetsService svc;
    const bool loaded = svc.loadFromJson(CANONICAL_VALID_JSON, "/tmp/vigilio-concurrency-test");
    CHECK(loaded);
    CHECK(svc.isLoaded());

    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS_PER_THREAD = 50;
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    std::atomic<bool> allPassed{true};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&svc, &allPassed]() {
            for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
                // 1. findPreset
                const auto p1 = svc.findPreset("core-debug", CMakePresetKind::Configure);
                if (!p1.has_value() || p1->name != "core-debug" || p1->generator != "Ninja") {
                    allPassed.store(false);
                }
                const auto p2 = svc.findPreset("desktop-release", CMakePresetKind::Configure);
                if (!p2.has_value() || p2->name != "desktop-release") {
                    allPassed.store(false);
                }

                // 2. listPresetsByKind
                const auto listCfg = svc.listPresetsByKind(CMakePresetKind::Configure);
                if (listCfg.size() != 9) {
                    allPassed.store(false);
                }
                const auto listBuild = svc.listPresetsByKind(CMakePresetKind::Build);
                if (listBuild.size() != 9) {
                    allPassed.store(false);
                }
                const auto listTest = svc.listPresetsByKind(CMakePresetKind::Test);
                if (listTest.size() != 7) {
                    allPassed.store(false);
                }

                // 3. buildConfigureCommandLine
                CMakePresetCommandOptions opts;
                opts.additionalArgs = {"-DFOO=BAR"};
                const std::string cmd = svc.buildConfigureCommandLine("desktop-debug", opts);
                if (cmd != "cmake --preset desktop-debug -DFOO=BAR") {
                    allPassed.store(false);
                }

                // 4. summary
                const auto summary = svc.summary();
                if (summary.totalConfigurePresets != 9 || summary.totalBuildPresets != 9 ||
                    summary.totalTestPresets != 7 || !svc.validationReport().isValid()) {
                    allPassed.store(false);
                }
            }
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    CHECK(allPassed.load());
}

// TEST-PRESET-SVC-014: Estrés de cancelación cooperativa e idempotencia atómica
void test_service_cancellation_stress_and_idempotence() {
    CancellationToken token;
    CHECK(!token.isCancelled());

    // 1. Invocaciones concurrentes masivas a requestCancellation() desde múltiples hilos
    constexpr int NUM_CANCEL_THREADS = 8;
    std::vector<std::thread> cancelThreads;
    cancelThreads.reserve(NUM_CANCEL_THREADS);

    for (int i = 0; i < NUM_CANCEL_THREADS; ++i) {
        cancelThreads.emplace_back([&token]() {
            for (int k = 0; k < 100; ++k) {
                token.requestCancellation();
                if (!token.isCancelled()) {
                    throw std::runtime_error(
                        "CancellationToken not cancelled after requestCancellation");
                }
            }
        });
    }

    for (auto& th : cancelThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    CHECK(token.isCancelled());

    // 2. Idempotencia: llamadas sucesivas no deben alterar el estado cancelado
    token.requestCancellation();
    token.requestCancellation();
    CHECK(token.isCancelled());

    // 3. Ejecución contra servicio con token ya cancelado
    CMakePresetsService svc;
    const bool loaded = svc.loadFromJson(CANONICAL_VALID_JSON, "/tmp/vigilio-cancel-test");
    CHECK(loaded);

    CMakePresetCommandOptions opts;
    opts.dryRun = true;

    const auto resCfg = svc.executeConfigure("core-debug", opts, &token);
    CHECK(!resCfg.isSuccess());
    CHECK(resCfg.status == CMakePresetExecutionStatus::Cancelled);
    CHECK(resCfg.exitCode == -1);
    CHECK(!resCfg.remediationHint.empty());

    const auto resBuild = svc.executeBuild("core-debug", opts, &token);
    CHECK(!resBuild.isSuccess());
    CHECK(resBuild.status == CMakePresetExecutionStatus::Cancelled);
    CHECK(resBuild.exitCode == -1);

    const auto resTest = svc.executeTest("core-debug", opts, &token);
    CHECK(!resTest.isSuccess());
    CHECK(resTest.status == CMakePresetExecutionStatus::Cancelled);
    CHECK(resTest.exitCode == -1);
}

// TEST-PRESET-SVC-015: Inyección de fallos en sistema de archivos y recuperación operativa
void test_service_filesystem_io_errors_and_recovery() {
    const std::filesystem::path rootDir = "/tmp/vigilio-io-recovery-test";
    std::error_code ec;
    std::filesystem::remove_all(rootDir, ec);
    std::filesystem::create_directories(rootDir, ec);

    const std::string readonlyPresetJson = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/_build-readonly",
          "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
        },
        {
          "name": "desktop-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/_build-desktop",
          "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
        }
      ]
    })";

    CMakePresetsService svc;
    const bool loaded = svc.loadFromJson(readonlyPresetJson, rootDir);
    CHECK(loaded);

    const std::filesystem::path buildDir = rootDir / "_build-readonly";
    const std::filesystem::path cmakeFilesDir = buildDir / "CMakeFiles";
    std::filesystem::create_directories(cmakeFilesDir, ec);
    std::ofstream cacheOut(buildDir / "CMakeCache.txt");
    cacheOut << "# synthetic cache\n";
    cacheOut.close();

    std::ofstream dummyFile(cmakeFilesDir / "dummy.txt");
    dummyFile << "locked file\n";
    dummyFile.close();

    // Modificar permisos del directorio a solo lectura
    std::filesystem::permissions(buildDir,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::replace, ec);

    std::string errMsg;
    const bool cleanResult = svc.cleanBinaryDir("core-debug", &errMsg);

    // Restaurar permisos para permitir limpieza posterior
    std::filesystem::permissions(buildDir, std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::replace, ec);

    if (!cleanResult) {
        CHECK(!errMsg.empty());
    }

    // 2. Comprobar recuperación y reporte ante preset inexistente
    std::string missingErrMsg;
    const bool missingResult = svc.cleanBinaryDir("nonexistent-profile", &missingErrMsg);
    CHECK(!missingResult);
    CHECK(!missingErrMsg.empty());

    // 3. Limpieza final de directorio sintético
    std::filesystem::remove_all(rootDir, ec);
}

// TEST-PRESET-SVC-016: Bounded buffers (64 KB) y política Zero-Zombie
void test_service_bounded_buffers_and_zero_zombie() {
    const std::filesystem::path testDir = "/tmp/vigilio-buffer-test";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    CMakePresetsService svc;
    const bool loaded = svc.loadFromJson(CANONICAL_VALID_JSON, testDir);
    CHECK(loaded);

    // 1. Verificación del límite por defecto de 64 KB (65536 bytes)
    CMakePresetCommandOptions defaultOpts;
    CHECK(defaultOpts.maxOutputBytes == 65536);

    // 2. Simulación de ejecución con truncado de buffer configurable
    CMakePresetCommandOptions boundedOpts;
    boundedOpts.maxOutputBytes = 256;
    boundedOpts.additionalArgs = {"--help"};

    const auto resBounded = svc.executeConfigure("core-debug", boundedOpts);
    CHECK(!resBounded.standardOutput.empty());
    CHECK(resBounded.standardOutput.find("salida truncada para preservar límites de memoria") !=
          std::string::npos);

    // 3. Verificación de respeto de timeout acotado
    CMakePresetCommandOptions timeoutOpts;
    timeoutOpts.timeoutMs = 50;                                  // 50 ms timeout
    timeoutOpts.additionalArgs = {"--help", "&&", "sleep", "1"}; // dura 1 s si no se interrumpe

    const auto resTimeout = svc.executeConfigure("core-debug", timeoutOpts);
    CHECK(!resTimeout.isSuccess());
    CHECK(resTimeout.status == CMakePresetExecutionStatus::Timeout);
    CHECK(resTimeout.exitCode == -1);
    CHECK(resTimeout.elapsedMilliseconds < 1000);

    std::filesystem::remove_all(testDir, ec);
}

// TEST-PRESET-SVC-017: Invocaciones en estado no inicializado y presets inexistentes
void test_service_uninitialized_and_invalid_state_transitions() {
    CMakePresetsService uninitSvc;
    CHECK(!uninitSvc.isLoaded());

    const auto resCfg = uninitSvc.executeConfigure("core-debug");
    CHECK(!resCfg.isSuccess());
    CHECK(resCfg.status == CMakePresetExecutionStatus::ValidationError);

    const auto resBuild = uninitSvc.executeBuild("core-debug");
    CHECK(!resBuild.isSuccess());
    CHECK(resBuild.status == CMakePresetExecutionStatus::ValidationError);

    const auto resTest = uninitSvc.executeTest("core-debug");
    CHECK(!resTest.isSuccess());
    CHECK(resTest.status == CMakePresetExecutionStatus::ValidationError);

    CHECK(!uninitSvc.findPreset("core-debug", CMakePresetKind::Configure).has_value());
    CHECK(uninitSvc.listPresetsByKind(CMakePresetKind::Configure).empty());

    CMakePresetsService loadedSvc;
    CHECK(loadedSvc.loadFromJson(CANONICAL_VALID_JSON));
    CHECK(loadedSvc.isLoaded());

    const auto resMissing = loadedSvc.executeConfigure("nonexistent-profile");
    CHECK(!resMissing.isSuccess());
    CHECK(resMissing.status == CMakePresetExecutionStatus::PresetNotFound);
}

// TEST-PRESET-SVC-018: Auditoría rigurosa de copy público (docs/UX-COPY-PUBLICO.md)
void test_service_public_copy_compliance() {
    const std::vector<std::string> forbiddenTerms = {"Senior Mode",  "Modo Senior", "Prompt Builder",
                                                     "OpenSpec",     "DAG",         "toolchain",
                                                     "micro-prompt", "Skill"};

    auto assertNoForbidden = [&forbiddenTerms](const std::string& str, const std::string& ctx) {
        CHECK(!str.empty());
        for (const auto& term : forbiddenTerms) {
            if (str.find(term) != std::string::npos) {
                throw std::runtime_error("Forbidden copy term '" + term + "' found in " + ctx + ": " +
                                         str);
            }
        }
    };

    const CMakePresetValidationOutcome outcomes[] = {
        CMakePresetValidationOutcome::Valid,
        CMakePresetValidationOutcome::InvalidSchemaVersion,
        CMakePresetValidationOutcome::MissingRequiredPreset,
        CMakePresetValidationOutcome::OrphanConfigurePresetReference,
        CMakePresetValidationOutcome::DuplicatePresetName,
        CMakePresetValidationOutcome::UnsafeBinaryDir,
        CMakePresetValidationOutcome::ConflictingVariables,
        CMakePresetValidationOutcome::MalformedJson};
    for (const auto outcome : outcomes) {
        assertNoForbidden(CMakePresetsValidator::outcomeToString(outcome),
                          "CMakePresetsValidator::outcomeToString");
    }

    const CMakePresetExecutionStatus statuses[] = {
        CMakePresetExecutionStatus::Success,        CMakePresetExecutionStatus::ValidationError,
        CMakePresetExecutionStatus::PresetNotFound, CMakePresetExecutionStatus::BinaryDirUnsafe,
        CMakePresetExecutionStatus::ToolNotFound,   CMakePresetExecutionStatus::ProcessFailed,
        CMakePresetExecutionStatus::Timeout,        CMakePresetExecutionStatus::Cancelled,
        CMakePresetExecutionStatus::IOError};
    for (const auto status : statuses) {
        assertNoForbidden(CMakePresetsService::executionStatusToString(status),
                          "CMakePresetsService::executionStatusToString");
    }

    const CMakePresetExecutionState states[] = {
        CMakePresetExecutionState::Idle,        CMakePresetExecutionState::Validating,
        CMakePresetExecutionState::Configuring, CMakePresetExecutionState::Building,
        CMakePresetExecutionState::Testing,     CMakePresetExecutionState::Completed,
        CMakePresetExecutionState::Failed,      CMakePresetExecutionState::Cancelled};
    for (const auto state : states) {
        assertNoForbidden(CMakePresetsService::executionStateToString(state),
                          "CMakePresetsService::executionStateToString");
    }
}

} // namespace

int main() {
    try {
        std::cout << "[test] Running test_service_load_canonical..." << std::endl;
        test_service_load_canonical();

        std::cout << "[test] Running test_service_find_and_resolve..." << std::endl;
        test_service_find_and_resolve();

        std::cout << "[test] Running test_service_command_line_synthesis..." << std::endl;
        test_service_command_line_synthesis();

        std::cout << "[test] Running test_service_dry_run..." << std::endl;
        test_service_dry_run();

        std::cout << "[test] Running test_service_cancellation..." << std::endl;
        test_service_cancellation();

        std::cout << "[test] Running test_service_clean_binary_dir..." << std::endl;
        test_service_clean_binary_dir();

        std::cout << "[test] Running test_service_diagnostics_formatting..." << std::endl;
        test_service_diagnostics_formatting();

        std::cout << "[test] Running test_service_concurrency_multi_threaded_reads..." << std::endl;
        test_service_concurrency_multi_threaded_reads();

        std::cout << "[test] Running test_service_cancellation_stress_and_idempotence..." << std::endl;
        test_service_cancellation_stress_and_idempotence();

        std::cout << "[test] Running test_service_filesystem_io_errors_and_recovery..." << std::endl;
        test_service_filesystem_io_errors_and_recovery();

        std::cout << "[test] Running test_service_bounded_buffers_and_zero_zombie..." << std::endl;
        test_service_bounded_buffers_and_zero_zombie();

        std::cout << "[test] Running test_service_uninitialized_and_invalid_state_transitions..."
                  << std::endl;
        test_service_uninitialized_and_invalid_state_transitions();

        std::cout << "[test] Running test_service_public_copy_compliance..." << std::endl;
        test_service_public_copy_compliance();

        std::cout << "[test] ALL CMakePresetsService unit tests PASSED successfully." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[test] FAILED with exception: " << ex.what() << std::endl;
        return 1;
    }
}
