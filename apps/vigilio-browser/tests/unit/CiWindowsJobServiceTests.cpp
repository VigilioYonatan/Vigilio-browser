#include "core/CiWindowsJobConfiguration.h"
#include "core/CiWindowsJobService.h"
#include "core/CiWindowsJobTypes.h"
#include "core/CiWindowsJobValidator.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

void writeFile(const std::filesystem::path& filePath, std::string_view content) {
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream ofs(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        throw std::runtime_error("No fue posible crear archivo: " + filePath.string());
    }
    ofs << content;
}

// TEST-CI-WIN-SRV-001: test_ci_windows_service_initialization
void test_ci_windows_service_initialization() {
    CiWindowsJobService defaultService;
    const auto defPlan = defaultService.plan();
    CHECK(defPlan.presetName == "ci-windows-debug");
    CHECK(defPlan.buildDir == "build-ci-win-debug");
    CHECK(defPlan.steps.size() == 6);

    const auto defOpts = defaultService.options();
    CHECK(defOpts.mode == CiWindowsExecutionMode::DryRun);
    CHECK(defOpts.failFast);
    CHECK(defOpts.maxRetries == 3);
    CHECK(defOpts.retryBackoffMs == 50);
    CHECK(defOpts.captureOutput);

    // Constructor con plan explícito
    auto customPlan = CiWindowsJobConfiguration::canonicalReleasePlan();
    CiWindowsJobService releaseService(customPlan);
    CHECK(releaseService.plan().presetName == "ci-windows-release");
    CHECK(releaseService.plan().steps.size() == 7);

    // Modificación de plan y opciones
    customPlan.totalTimeoutSeconds = 800;
    defaultService.setPlan(customPlan);
    CHECK(defaultService.plan().totalTimeoutSeconds == 800);

    CiWindowsServiceOptions newOpts;
    newOpts.mode = CiWindowsExecutionMode::Simulated;
    newOpts.failFast = false;
    newOpts.maxRetries = 5;
    newOpts.retryBackoffMs = 100;
    newOpts.captureOutput = false;
    defaultService.setOptions(newOpts);
    CHECK(defaultService.options() == newOpts);
}

// TEST-CI-WIN-SRV-002: test_ci_windows_service_concurrency_and_synchronization
void test_ci_windows_service_concurrency_and_synchronization() {
    CiWindowsJobService service;
    std::atomic<bool> startFlag{false};
    constexpr int totalThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(totalThreads);

    for (int i = 0; i < totalThreads; ++i) {
        threads.emplace_back([&, i]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (i % 2 == 0) {
                auto p = service.plan();
                p.totalTimeoutSeconds = 600 + static_cast<uint32_t>(i);
                service.setPlan(p);
            } else {
                auto opt = service.options();
                opt.maxRetries = static_cast<uint32_t>(i);
                service.setOptions(opt);
            }
        });
    }

    startFlag.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }

    CHECK(service.plan().presetName == "ci-windows-debug");
}

// TEST-CI-WIN-SRV-003: test_ci_windows_service_dry_run_pipeline
void test_ci_windows_service_dry_run_pipeline() {
    CiWindowsJobService service;
    auto report = service.executePlan();

    CHECK(report.status == CiWindowsJobStatus::Passed);
    CHECK(report.isPassed());
    CHECK(report.isClean);
    CHECK(report.planName == "ci-windows-debug");
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 6);
    CHECK(report.failedStepsCount() == 0);
    CHECK(!report.remediationHint.empty());

    for (const auto& stepRes : report.stepResults) {
        CHECK(stepRes.status == CiWindowsStepStatus::Success);
        CHECK(stepRes.isSuccess());
        CHECK(stepRes.exitCode == 0);
        CHECK(!stepRes.outputSummary.empty());
    }
}

// TEST-CI-WIN-SRV-004: test_ci_windows_service_fail_fast
void test_ci_windows_service_fail_fast() {
    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    // Simular que la etapa de formato falla
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación de formato fallida";

    CiWindowsJobService service(plan);
    auto report = service.executePlan();

    CHECK(report.status == CiWindowsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.isClean);
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 1); // EnvironmentCheck pasó
    CHECK(report.failedStepsCount() == 1);     // FormatVerification falló

    CHECK(report.stepResults[0].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiWindowsStepStatus::Failure);
    CHECK(report.stepResults[1].exitCode == 1);

    // Etapas 2 a 5 deben estar omitidas por fail-fast
    for (std::size_t i = 2; i < 6; ++i) {
        CHECK(report.stepResults[i].status == CiWindowsStepStatus::Omitted);
    }
}

// TEST-CI-WIN-SRV-005: test_ci_windows_service_continue_on_error
void test_ci_windows_service_continue_on_error() {
    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    // Etapa 1 configurada con continueOnError y no obligatoria
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación no obligatoria fallida";
    plan.steps[1].isMandatory = false;
    plan.steps[1].options.continueOnError = true;

    CiWindowsJobService service(plan);
    auto report = service.executePlan();

    // El pipeline continuó ejecutando el resto de etapas
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiWindowsStepStatus::Failure);
    CHECK(report.stepResults[2].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[3].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[4].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[5].status == CiWindowsStepStatus::Success);

    CHECK(report.failedStepsCount() == 1);
    CHECK(report.successfulStepsCount() == 5);
    CHECK(report.isPassed());
    CHECK(report.status == CiWindowsJobStatus::Passed);
}

// TEST-CI-WIN-SRV-006: test_ci_windows_service_cancellation
void test_ci_windows_service_cancellation() {
    // Caso 1: Cancelación antes de iniciar
    {
        CiWindowsJobService service;
        CancellationToken cancelToken;
        cancelToken.requestCancellation();

        auto report = service.executePlan(&cancelToken);
        CHECK(report.status == CiWindowsJobStatus::Cancelled);
        CHECK(!report.isPassed());
        CHECK(report.stepResults.empty());
    }

    // Caso 2: Cancelación cooperativa multihilo durante la ejecución
    {
        CiWindowsJobService service;
        CiWindowsServiceOptions opts;
        opts.mode = CiWindowsExecutionMode::Simulated;
        service.setOptions(opts);

        CancellationToken cancelToken;
        std::atomic<bool> workerStarted{false};
        CiWindowsJobReport report;

        std::thread worker([&]() {
            workerStarted.store(true);
            report = service.executePlan(&cancelToken);
        });

        while (!workerStarted.load()) {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        cancelToken.requestCancellation();

        worker.join();

        CHECK(report.status == CiWindowsJobStatus::Cancelled);
        CHECK(!report.isPassed());
        // Al menos una etapa debe haber sido omitida
        bool hasOmitted = false;
        for (const auto& res : report.stepResults) {
            if (res.status == CiWindowsStepStatus::Omitted) {
                hasOmitted = true;
                break;
            }
        }
        CHECK(hasOmitted);
    }
}

// TEST-CI-WIN-SRV-007: test_ci_windows_service_clean_build_directory
void test_ci_windows_service_clean_build_directory() {
    const std::filesystem::path safeRelativeDir = "build-ci-win-test-clean-dir";
    std::filesystem::remove_all(safeRelativeDir);

    // Crear directorio seguro con archivos
    writeFile(safeRelativeDir / "sub" / "artifact.obj", "dummy binary content");
    CHECK(std::filesystem::exists(safeRelativeDir / "sub" / "artifact.obj"));

    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = safeRelativeDir;

    CiWindowsJobService service(plan);
    std::string errorMsg;
    CHECK(service.cleanBuildDirectory(&errorMsg));
    CHECK(!std::filesystem::exists(safeRelativeDir));

    // Limpieza cuando el directorio ya no existe devuelve true
    CHECK(service.cleanBuildDirectory(&errorMsg));
}

// TEST-CI-WIN-SRV-008: test_ci_windows_service_clean_build_directory_and_traversal
void test_ci_windows_service_clean_build_directory_and_traversal() {
    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    CiWindowsJobService service(plan);
    std::string errorMsg;

    // Intento con ruta insegura con Directory Traversal (..)
    plan.buildDir = "../unsafe_traversal_dir";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con barra invertida Windows traversal (..\)
    plan.buildDir = "..\\unsafe_win_traversal";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con ruta absoluta del sistema (POSIX)
    plan.buildDir = "/tmp/unsafe_absolute_dir";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con unidad de disco Windows
    plan.buildDir = "C:\\Windows\\System32";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con prefijo UNC
    plan.buildDir = "\\\\server\\share\\build";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con prefijo NT
    plan.buildDir = "\\\\?\\C:\\build";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con dispositivo DOS reservado
    plan.buildDir = "CON";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    plan.buildDir = "NUL";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());
}

// TEST-CI-WIN-SRV-009: test_ci_windows_service_execute_step
void test_ci_windows_service_execute_step() {
    CiWindowsJobService service;

    // Ejecución de etapa individual
    auto stepRes = service.executeStep(CiWindowsPipelineStep::ConfigurePreset);
    CHECK(stepRes.step == CiWindowsPipelineStep::ConfigurePreset);
    CHECK(stepRes.status == CiWindowsStepStatus::Success);
    CHECK(stepRes.exitCode == 0);
    CHECK(stepRes.isSuccess());

    // Ejecución con cancelación
    CancellationToken cancelToken;
    cancelToken.requestCancellation();
    auto cancelledRes = service.executeStep(CiWindowsPipelineStep::BuildCompilation, &cancelToken);
    CHECK(cancelledRes.step == CiWindowsPipelineStep::BuildCompilation);
    CHECK(cancelledRes.status == CiWindowsStepStatus::Omitted);
    CHECK(!cancelledRes.isSuccess());
}

// TEST-CI-WIN-SRV-010: test_ci_windows_service_cli_synthesis
void test_ci_windows_service_cli_synthesis() {
    CiWindowsJobService service;

    CiWindowsStepDefinition step;
    step.step = CiWindowsPipelineStep::EnvironmentCheck;
    CHECK(service.buildCommandLine(step) == "cmake --version && ninja --version && cl.exe");

    step.step = CiWindowsPipelineStep::FormatVerification;
    CHECK(
        service.buildCommandLine(step) ==
        "powershell -Command \"Get-ChildItem -Recurse -Include *.cpp,*.h apps/vigilio-browser/src, "
        "apps/vigilio-browser/tests | ForEach-Object { clang-format --dry-run --Werror $_.FullName }\"");

    step.step = CiWindowsPipelineStep::StaticAnalysis;
    CHECK(service.buildCommandLine(step) ==
          "clang-tidy -p build-ci-win-debug apps/vigilio-browser/src/**/*.cpp");

    step.step = CiWindowsPipelineStep::ConfigurePreset;
    CHECK(service.buildCommandLine(step) == "cmake --preset ci-windows-debug -B build-ci-win-debug");

    step.step = CiWindowsPipelineStep::BuildCompilation;
    CHECK(service.buildCommandLine(step) == "cmake --build build-ci-win-debug --parallel");

    step.step = CiWindowsPipelineStep::TestSuiteExecution;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-win-debug --output-on-failure -L fast --timeout 15");

    step.step = CiWindowsPipelineStep::SanitizerVerification;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-win-debug --output-on-failure -L sanitizer");

    step.step = CiWindowsPipelineStep::ArtifactPackaging;
    CHECK(service.buildCommandLine(step) == "cpack --config build-ci-win-debug/CPackConfig.cmake");

    // Comando personalizado
    CiWindowsStepDefinition customStep;
    customStep.step = static_cast<CiWindowsPipelineStep>(99);
    customStep.command = "ninja -C build-ci-win-debug custom_target";
    CHECK(service.buildCommandLine(customStep) == "ninja -C build-ci-win-debug custom_target");
}

// TEST-CI-WIN-SRV-011: test_ci_windows_service_resource_and_memory_limits
void test_ci_windows_service_resource_and_memory_limits() {
    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    // Exceder memoria máxima permitida (> 4 GB)
    plan.maxMemoryBytes = 5ULL * 1024 * 1024 * 1024; // 5 GB

    CiWindowsJobService service(plan);
    auto report = service.executePlan();
    CHECK(report.status == CiWindowsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());

    // Plan con etapas vacías
    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    service.setPlan(plan);
    report = service.executePlan();
    CHECK(report.status == CiWindowsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());

    // Simular paso con timeout
    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps[1].command = "timeout";
    plan.steps[1].name = "Etapa con tiempo límite excedido";

    service.setPlan(plan);
    report = service.executePlan();

    CHECK(report.status == CiWindowsJobStatus::TimedOut);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiWindowsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiWindowsStepStatus::Omitted);
    CHECK(report.stepResults[2].status == CiWindowsStepStatus::Omitted);
}

// TEST-CI-WIN-SRV-012: test_ci_windows_service_ux_copy_public_compliance
void test_ci_windows_service_ux_copy_public_compliance() {
    const std::vector<std::string> prohibitedTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                      "DAG",         "OpenSpec",    "toolchain",
                                                      "runner",      "linter"};

    auto checkNoProhibitedTerms = [&](const std::string& text) {
        CHECK(!text.empty());
        for (const auto& term : prohibitedTerms) {
            std::string lowerText;
            lowerText.reserve(text.size());
            for (char c : text) {
                lowerText.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }

            std::string lowerTerm;
            lowerTerm.reserve(term.size());
            for (char c : term) {
                lowerTerm.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }

            CHECK(lowerText.find(lowerTerm) == std::string::npos);
        }
    };

    // Modos de ejecución
    const std::vector<CiWindowsExecutionMode> modes = {
        CiWindowsExecutionMode::DryRun, CiWindowsExecutionMode::Simulated, CiWindowsExecutionMode::Live};
    for (auto m : modes) {
        checkNoProhibitedTerms(CiWindowsJobService::executionModeToString(m));
        checkNoProhibitedTerms(executionModeToString(m));
    }

    // Resúmenes de etapas
    CiWindowsStepResult sampleResult;
    sampleResult.step = CiWindowsPipelineStep::BuildCompilation;
    sampleResult.status = CiWindowsStepStatus::Success;
    sampleResult.durationMs = 120;
    sampleResult.exitCode = 0;
    sampleResult.outputSummary = "Compilación completada sin errores.";
    checkNoProhibitedTerms(CiWindowsJobService::formatStepSummary(sampleResult));
    checkNoProhibitedTerms(formatStepSummary(sampleResult));

    sampleResult.status = CiWindowsStepStatus::Failure;
    sampleResult.exitCode = 1;
    sampleResult.errorMessage = "Se encontraron errores en el código fuente.";
    checkNoProhibitedTerms(CiWindowsJobService::formatStepSummary(sampleResult));

    // Pistas de remediación de reporte
    CiWindowsJobService service;
    auto report = service.executePlan();
    checkNoProhibitedTerms(report.remediationHint);
}

// Test de concurrencia masiva multihilo
void test_ci_windows_service_massive_multithreading_concurrency() {
    CiWindowsJobService service;
    std::atomic<bool> startFlag{false};
    std::atomic<int> completedWorkers{0};
    constexpr int totalThreads = 16;
    std::vector<std::thread> workers;
    workers.reserve(totalThreads);

    // 8 hilos ejecutando executePlan en modo DryRun
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int it = 0; it < 5; ++it) {
                auto rep = service.executePlan();
                CHECK(rep.isPassed() || rep.status == CiWindowsJobStatus::Passed);
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    // 4 hilos ejecutando executeStep de etapas heterogéneas
    const std::vector<CiWindowsPipelineStep> testSteps = {
        CiWindowsPipelineStep::EnvironmentCheck, CiWindowsPipelineStep::ConfigurePreset,
        CiWindowsPipelineStep::BuildCompilation, CiWindowsPipelineStep::TestSuiteExecution};
    for (int i = 0; i < 4; ++i) {
        const auto step = testSteps[static_cast<std::size_t>(i)];
        workers.emplace_back([&, step]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int it = 0; it < 10; ++it) {
                auto res = service.executeStep(step);
                CHECK(res.isSuccess());
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    // 4 hilos modificando y leyendo opciones concurrentemente
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int it = 0; it < 10; ++it) {
                CiWindowsServiceOptions opts;
                opts.mode =
                    (it % 2 == 0) ? CiWindowsExecutionMode::DryRun : CiWindowsExecutionMode::Simulated;
                opts.maxRetries = static_cast<uint32_t>(i + 1);
                service.setOptions(opts);
                auto currentOpts = service.options();
                CHECK(currentOpts.maxRetries >= 1);
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    startFlag.store(true, std::memory_order_release);
    for (auto& w : workers) {
        w.join();
    }

    CHECK(completedWorkers.load(std::memory_order_acquire) == totalThreads);
}

// Test de reintentos con contención
void test_ci_windows_service_retry_backoff_on_contention() {
    const std::filesystem::path retryDir = "build-ci-win-test-retry-dir";
    std::filesystem::remove_all(retryDir);

    writeFile(retryDir / "sub" / "file.txt", "bloqueo transitorio");
    std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::none);

    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = retryDir;

    CiWindowsJobService service(plan);
    CiWindowsServiceOptions opts;
    opts.retryBackoffMs = 25;
    opts.maxRetries = 3;
    service.setOptions(opts);

    std::thread unblocker([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::error_code ec;
        std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::all, ec);
    });

    std::string errorMsg;
    bool cleaned = service.cleanBuildDirectory(&errorMsg);
    unblocker.join();

    std::error_code ec;
    std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::all, ec);
    std::filesystem::remove_all(retryDir, ec);

    CHECK(cleaned);
    CHECK(!std::filesystem::exists(retryDir));
}

// TEST-CI-WIN-SRV-014: test_ci_windows_service_cooperative_cancellation_stress (TASK-006)
void test_ci_windows_service_cooperative_cancellation_stress() {
    CiWindowsJobService service;
    auto plan = CiWindowsJobConfiguration::canonicalReleasePlan();
    service.setPlan(plan);

    CiWindowsServiceOptions opts;
    opts.mode = CiWindowsExecutionMode::Simulated;
    service.setOptions(opts);

    for (int iter = 0; iter < 4; ++iter) {
        CancellationToken cancelToken;
        std::atomic<bool> threadStarted{false};
        CiWindowsJobReport report;

        std::thread worker([&]() {
            threadStarted.store(true, std::memory_order_release);
            report = service.executePlan(&cancelToken);
        });

        while (!threadStarted.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(iter * 3 + 2));
        cancelToken.requestCancellation();

        worker.join();

        CHECK(report.status == CiWindowsJobStatus::Cancelled);
        CHECK(!report.isPassed());
        CHECK(cancelToken.isCancelled());

        bool hasOmitted = false;
        for (const auto& stepRes : report.stepResults) {
            if (stepRes.status == CiWindowsStepStatus::Omitted) {
                hasOmitted = true;
                break;
            }
        }
        CHECK(hasOmitted);
    }
}

// TEST-CI-WIN-SRV-015: test_ci_windows_service_permission_failure_injection (TASK-007)
void test_ci_windows_service_permission_failure_injection() {
    const std::filesystem::path permDir = "build-ci-win-test-perm-dir";
    std::error_code ec;
    std::filesystem::remove_all(permDir, ec);

    const auto subDir = permDir / "nested";
    writeFile(subDir / "protected.bin", "permiso restringido");
    CHECK(std::filesystem::exists(subDir / "protected.bin"));

    std::filesystem::permissions(subDir, std::filesystem::perms::owner_read, ec);

    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = permDir;

    CiWindowsJobService service(plan);
    CiWindowsServiceOptions opts;
    opts.maxRetries = 1;
    opts.retryBackoffMs = 5;
    service.setOptions(opts);

    std::string errorMsg;
    bool cleaned = service.cleanBuildDirectory(&errorMsg);

    std::filesystem::permissions(subDir, std::filesystem::perms::all, ec);
    std::filesystem::remove_all(permDir, ec);

    if (!cleaned) {
        CHECK(!errorMsg.empty());
        CHECK(errorMsg.find("No fue posible limpiar") != std::string::npos);
    }

    plan.buildDir = "../traversal_injection";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());
    CHECK(errorMsg.find("no segura") != std::string::npos);
}

// TEST-CI-WIN-SRV-016: test_ci_windows_service_retry_backoff_transient (TASK-008)
void test_ci_windows_service_retry_backoff_transient() {
    const std::filesystem::path retryDir = "build-ci-win-test-backoff-transient";
    std::error_code ec;
    std::filesystem::remove_all(retryDir, ec);

    const auto subDir = retryDir / "sub";
    writeFile(subDir / "transient.txt", "contención transitoria");
    CHECK(std::filesystem::exists(subDir / "transient.txt"));

    std::filesystem::permissions(subDir, std::filesystem::perms::none, ec);

    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = retryDir;

    CiWindowsJobService service(plan);
    CiWindowsServiceOptions opts;
    opts.maxRetries = 4;
    opts.retryBackoffMs = 15;
    service.setOptions(opts);

    CHECK(service.options().maxRetries == 4);
    CHECK(service.options().retryBackoffMs == 15);

    std::thread unblocker([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::error_code uec;
        std::filesystem::permissions(subDir, std::filesystem::perms::all, uec);
    });

    std::string errorMsg;
    bool cleaned = service.cleanBuildDirectory(&errorMsg);
    unblocker.join();

    std::filesystem::permissions(subDir, std::filesystem::perms::all, ec);
    std::filesystem::remove_all(retryDir, ec);

    CHECK(cleaned);
    CHECK(!std::filesystem::exists(retryDir));
}

// TEST-CI-WIN-SRV-017: test_ci_windows_service_fail_fast_vs_continue_on_error (TASK-009)
void test_ci_windows_service_fail_fast_vs_continue_on_error() {
    // Escenario 1: failFast == true, continueOnError == false
    {
        auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
        plan.steps[1].command = "false";
        plan.steps[1].name = "Formato con fallo";
        plan.steps[1].options.continueOnError = false;
        plan.failFast = true;

        CiWindowsJobService service(plan);
        CiWindowsServiceOptions opts;
        opts.failFast = true;
        service.setOptions(opts);

        auto report = service.executePlan();
        CHECK(report.status == CiWindowsJobStatus::Failed);
        CHECK(!report.isPassed());
        CHECK(report.stepResults.size() == 6);
        CHECK(report.stepResults[0].status == CiWindowsStepStatus::Success);
        CHECK(report.stepResults[1].status == CiWindowsStepStatus::Failure);
        CHECK(report.successfulStepsCount() == 1);
        CHECK(report.failedStepsCount() == 1);

        for (std::size_t i = 2; i < 6; ++i) {
            CHECK(report.stepResults[i].status == CiWindowsStepStatus::Omitted);
        }
    }

    // Escenario 2: failFast == false, continueOnError == true en etapa no obligatoria
    {
        auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
        plan.steps[1].command = "false";
        plan.steps[1].name = "Formato no obligatorio con fallo";
        plan.steps[1].isMandatory = false;
        plan.steps[1].options.continueOnError = true;
        plan.failFast = false;

        CiWindowsJobService service(plan);
        CiWindowsServiceOptions opts;
        opts.failFast = false;
        service.setOptions(opts);

        auto report = service.executePlan();
        CHECK(report.stepResults.size() == 6);
        CHECK(report.stepResults[0].status == CiWindowsStepStatus::Success);
        CHECK(report.stepResults[1].status == CiWindowsStepStatus::Failure);
        CHECK(report.stepResults[2].status == CiWindowsStepStatus::Success);
        CHECK(report.stepResults[3].status == CiWindowsStepStatus::Success);
        CHECK(report.stepResults[4].status == CiWindowsStepStatus::Success);
        CHECK(report.stepResults[5].status == CiWindowsStepStatus::Success);

        CHECK(report.successfulStepsCount() == 5);
        CHECK(report.failedStepsCount() == 1);
        CHECK(report.isPassed());
        CHECK(report.status == CiWindowsJobStatus::Passed);
    }
}

// TEST-CI-WIN-SRV-018: test_ci_windows_service_cli_synthesis_advanced (TASK-010)
void test_ci_windows_service_cli_synthesis_advanced() {
    CiWindowsJobPlan customPlan;
    customPlan.presetName = "ci-windows-release-optimized";
    customPlan.buildDir = "_build/nested/win-ci";
    customPlan.totalTimeoutSeconds = 1200;
    customPlan.maxMemoryBytes = 4294967296ULL;
    customPlan.failFast = true;

    CiWindowsJobService service(customPlan);

    // ConfigurePreset
    CiWindowsStepDefinition stepConfig;
    stepConfig.step = CiWindowsPipelineStep::ConfigurePreset;
    const std::string configCli = service.buildCommandLine(stepConfig);
    CHECK(configCli.find("cmake --preset ci-windows-release-optimized") != std::string::npos);
    CHECK(configCli.find("-B _build/nested/win-ci") != std::string::npos);

    // BuildCompilation
    CiWindowsStepDefinition stepBuild;
    stepBuild.step = CiWindowsPipelineStep::BuildCompilation;
    const std::string buildCli = service.buildCommandLine(stepBuild);
    CHECK(buildCli.find("cmake --build _build/nested/win-ci --parallel") != std::string::npos);

    // TestSuiteExecution
    CiWindowsStepDefinition stepTest;
    stepTest.step = CiWindowsPipelineStep::TestSuiteExecution;
    const std::string testCli = service.buildCommandLine(stepTest);
    CHECK(testCli.find("ctest --test-dir _build/nested/win-ci --output-on-failure -L fast") !=
          std::string::npos);

    // SanitizerVerification
    CiWindowsStepDefinition stepSanitizer;
    stepSanitizer.step = CiWindowsPipelineStep::SanitizerVerification;
    const std::string sanitizerCli = service.buildCommandLine(stepSanitizer);
    CHECK(sanitizerCli.find("ctest --test-dir _build/nested/win-ci --output-on-failure -L sanitizer") !=
          std::string::npos);

    // ArtifactPackaging
    CiWindowsStepDefinition stepPack;
    stepPack.step = CiWindowsPipelineStep::ArtifactPackaging;
    const std::string packCli = service.buildCommandLine(stepPack);
    CHECK(packCli.find("cpack --config _build/nested/win-ci/CPackConfig.cmake") != std::string::npos);

    // StaticAnalysis
    CiWindowsStepDefinition stepStatic;
    stepStatic.step = CiWindowsPipelineStep::StaticAnalysis;
    const std::string staticCli = service.buildCommandLine(stepStatic);
    CHECK(staticCli.find("clang-tidy -p _build/nested/win-ci apps/vigilio-browser/src/**/*.cpp") !=
          std::string::npos);

    // Paso con comando personalizado (e.g. con compilador en Program Files o sanitizers)
    CiWindowsStepDefinition customStep;
    customStep.step = static_cast<CiWindowsPipelineStep>(99);
    customStep.command = "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\bin\\cl.exe\" "
                         "/permissive- -fsanitize=address";
    CHECK(service.buildCommandLine(customStep) ==
          "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\bin\\cl.exe\" /permissive- "
          "-fsanitize=address");
}

} // namespace

int main() {
    try {
        std::cout << "Ejecutando pruebas operativas de CiWindowsJobService...\n";
        test_ci_windows_service_initialization();
        std::cout << "  [PASS] test_ci_windows_service_initialization\n";
        test_ci_windows_service_concurrency_and_synchronization();
        std::cout << "  [PASS] test_ci_windows_service_concurrency_and_synchronization\n";
        test_ci_windows_service_dry_run_pipeline();
        std::cout << "  [PASS] test_ci_windows_service_dry_run_pipeline\n";
        test_ci_windows_service_fail_fast();
        std::cout << "  [PASS] test_ci_windows_service_fail_fast\n";
        test_ci_windows_service_continue_on_error();
        std::cout << "  [PASS] test_ci_windows_service_continue_on_error\n";
        test_ci_windows_service_cancellation();
        std::cout << "  [PASS] test_ci_windows_service_cancellation\n";
        test_ci_windows_service_clean_build_directory();
        std::cout << "  [PASS] test_ci_windows_service_clean_build_directory\n";
        test_ci_windows_service_clean_build_directory_and_traversal();
        std::cout << "  [PASS] test_ci_windows_service_clean_build_directory_and_traversal\n";
        test_ci_windows_service_execute_step();
        std::cout << "  [PASS] test_ci_windows_service_execute_step\n";
        test_ci_windows_service_cli_synthesis();
        std::cout << "  [PASS] test_ci_windows_service_cli_synthesis\n";
        test_ci_windows_service_resource_and_memory_limits();
        std::cout << "  [PASS] test_ci_windows_service_resource_and_memory_limits\n";
        test_ci_windows_service_ux_copy_public_compliance();
        std::cout << "  [PASS] test_ci_windows_service_ux_copy_public_compliance\n";
        test_ci_windows_service_massive_multithreading_concurrency();
        std::cout << "  [PASS] test_ci_windows_service_massive_multithreading_concurrency\n";
        test_ci_windows_service_retry_backoff_on_contention();
        std::cout << "  [PASS] test_ci_windows_service_retry_backoff_on_contention\n";
        test_ci_windows_service_cooperative_cancellation_stress();
        std::cout << "  [PASS] test_ci_windows_service_cooperative_cancellation_stress\n";
        test_ci_windows_service_permission_failure_injection();
        std::cout << "  [PASS] test_ci_windows_service_permission_failure_injection\n";
        test_ci_windows_service_retry_backoff_transient();
        std::cout << "  [PASS] test_ci_windows_service_retry_backoff_transient\n";
        test_ci_windows_service_fail_fast_vs_continue_on_error();
        std::cout << "  [PASS] test_ci_windows_service_fail_fast_vs_continue_on_error\n";
        test_ci_windows_service_cli_synthesis_advanced();
        std::cout << "  [PASS] test_ci_windows_service_cli_synthesis_advanced\n";

        std::cout << "Todas las pruebas de CiWindowsJobService pasaron exitosamente.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo en prueba de CiWindowsJobService: " << e.what() << '\n';
        return 1;
    }
}
