#include "core/CiLinuxJobConfiguration.h"
#include "core/CiLinuxJobService.h"
#include "core/CiLinuxJobTypes.h"
#include "core/CiLinuxJobValidator.h"

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

// TEST-CI-LINUX-SRV-001: test_ci_linux_service_initialization
void test_ci_linux_service_initialization() {
    CiLinuxJobService defaultService;
    const auto defPlan = defaultService.plan();
    CHECK(defPlan.presetName == "ci-linux-debug");
    CHECK(defPlan.buildDir == "build-ci-debug");
    CHECK(defPlan.steps.size() == 6);

    const auto defOpts = defaultService.options();
    CHECK(defOpts.mode == CiLinuxExecutionMode::DryRun);
    CHECK(defOpts.failFast);
    CHECK(defOpts.maxRetries == 3);
    CHECK(defOpts.retryBackoffMs == 50);
    CHECK(defOpts.captureOutput);

    // Constructor con plan explícito
    auto customPlan = CiLinuxJobConfiguration::canonicalReleasePlan();
    CiLinuxJobService releaseService(customPlan);
    CHECK(releaseService.plan().presetName == "ci-linux-release");
    CHECK(releaseService.plan().steps.size() == 7);

    // Modificación de plan y opciones
    customPlan.totalTimeoutSeconds = 800;
    defaultService.setPlan(customPlan);
    CHECK(defaultService.plan().totalTimeoutSeconds == 800);

    CiLinuxServiceOptions newOpts;
    newOpts.mode = CiLinuxExecutionMode::Simulated;
    newOpts.failFast = false;
    newOpts.maxRetries = 5;
    newOpts.retryBackoffMs = 100;
    newOpts.captureOutput = false;
    defaultService.setOptions(newOpts);
    CHECK(defaultService.options() == newOpts);
}

// TEST-CI-LINUX-SRV-002: test_ci_linux_service_dry_run_pipeline
void test_ci_linux_service_dry_run_pipeline() {
    CiLinuxJobService service;
    auto report = service.executePlan();

    CHECK(report.status == CiLinuxJobStatus::Passed);
    CHECK(report.isPassed());
    CHECK(report.isClean);
    CHECK(report.planName == "ci-linux-debug");
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 6);
    CHECK(report.failedStepsCount() == 0);
    CHECK(!report.remediationHint.empty());

    for (const auto& stepRes : report.stepResults) {
        CHECK(stepRes.status == CiLinuxStepStatus::Success);
        CHECK(stepRes.isSuccess());
        CHECK(stepRes.exitCode == 0);
        CHECK(!stepRes.outputSummary.empty());
    }
}

// TEST-CI-LINUX-SRV-003: test_ci_linux_service_fail_fast
void test_ci_linux_service_fail_fast() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    // Simular que la etapa de formato falla
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación de formato fallida";

    CiLinuxJobService service(plan);
    auto report = service.executePlan();

    CHECK(report.status == CiLinuxJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.isClean);
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 1); // EnvironmentCheck pasó
    CHECK(report.failedStepsCount() == 1);     // FormatVerification falló

    CHECK(report.stepResults[0].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[1].status == CiLinuxStepStatus::Failure);
    CHECK(report.stepResults[1].exitCode == 1);

    // Etapas 2 a 5 deben estar omitidas por fail-fast
    for (std::size_t i = 2; i < 6; ++i) {
        CHECK(report.stepResults[i].status == CiLinuxStepStatus::Omitted);
    }
}

// TEST-CI-LINUX-SRV-004: test_ci_linux_service_continue_on_error
void test_ci_linux_service_continue_on_error() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    // Etapa 1 configurada con continueOnError y no obligatoria
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación no obligatoria fallida";
    plan.steps[1].isMandatory = false;
    plan.steps[1].options.continueOnError = true;

    CiLinuxJobService service(plan);
    auto report = service.executePlan();

    // El pipeline continuó ejecutando el resto de etapas
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[1].status == CiLinuxStepStatus::Failure);
    CHECK(report.stepResults[2].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[3].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[4].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[5].status == CiLinuxStepStatus::Success);

    CHECK(report.failedStepsCount() == 1);
    CHECK(report.successfulStepsCount() == 5);
    CHECK(report.isPassed());
    CHECK(report.status == CiLinuxJobStatus::Passed);
}

// TEST-CI-LINUX-SRV-005: test_ci_linux_service_cancellation
void test_ci_linux_service_cancellation() {
    // Caso 1: Cancelación antes de iniciar
    {
        CiLinuxJobService service;
        CancellationToken cancelToken;
        cancelToken.requestCancellation();

        auto report = service.executePlan(&cancelToken);
        CHECK(report.status == CiLinuxJobStatus::Cancelled);
        CHECK(!report.isPassed());
        CHECK(report.stepResults.empty());
    }

    // Caso 2: Cancelación cooperativa multihilo durante la ejecución
    {
        CiLinuxJobService service;
        CiLinuxServiceOptions opts;
        opts.mode = CiLinuxExecutionMode::Simulated;
        service.setOptions(opts);

        CancellationToken cancelToken;
        std::atomic<bool> workerStarted{false};
        CiLinuxJobReport report;

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

        CHECK(report.status == CiLinuxJobStatus::Cancelled);
        CHECK(!report.isPassed());
        // Al menos una etapa debe haber sido omitida
        bool hasOmitted = false;
        for (const auto& res : report.stepResults) {
            if (res.status == CiLinuxStepStatus::Omitted) {
                hasOmitted = true;
                break;
            }
        }
        CHECK(hasOmitted);
    }
}

// TEST-CI-LINUX-SRV-006: test_ci_linux_service_clean_build_directory_and_traversal
void test_ci_linux_service_clean_build_directory_and_traversal() {
    const std::filesystem::path safeRelativeDir = "build-test-clean-dir";
    std::filesystem::remove_all(safeRelativeDir);

    // Crear directorio seguro con archivos
    writeFile(safeRelativeDir / "sub" / "artifact.o", "dummy content");
    CHECK(std::filesystem::exists(safeRelativeDir / "sub" / "artifact.o"));

    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.buildDir = safeRelativeDir;

    CiLinuxJobService service(plan);
    std::string errorMsg;
    CHECK(service.cleanBuildDirectory(&errorMsg));
    CHECK(!std::filesystem::exists(safeRelativeDir));

    // Limpieza cuando el directorio ya no existe
    CHECK(service.cleanBuildDirectory(&errorMsg));

    // Intento con ruta insegura con Directory Traversal (..)
    plan.buildDir = "../unsafe_traversal_dir";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Intento con ruta absoluta del sistema
    plan.buildDir = "/tmp/unsafe_absolute_dir";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());
}

// TEST-CI-LINUX-SRV-007: test_ci_linux_service_execute_step
void test_ci_linux_service_execute_step() {
    CiLinuxJobService service;

    // Ejecución de etapa individual
    auto stepRes = service.executeStep(CiLinuxPipelineStep::ConfigurePreset);
    CHECK(stepRes.step == CiLinuxPipelineStep::ConfigurePreset);
    CHECK(stepRes.status == CiLinuxStepStatus::Success);
    CHECK(stepRes.exitCode == 0);
    CHECK(stepRes.isSuccess());

    // Ejecución con cancelación
    CancellationToken cancelToken;
    cancelToken.requestCancellation();
    auto cancelledRes = service.executeStep(CiLinuxPipelineStep::BuildCompilation, &cancelToken);
    CHECK(cancelledRes.step == CiLinuxPipelineStep::BuildCompilation);
    CHECK(cancelledRes.status == CiLinuxStepStatus::Omitted);
    CHECK(!cancelledRes.isSuccess());
}

// TEST-CI-LINUX-SRV-008: test_ci_linux_service_cli_synthesis
void test_ci_linux_service_cli_synthesis() {
    CiLinuxJobService service;

    CiLinuxStepDefinition step;
    step.step = CiLinuxPipelineStep::EnvironmentCheck;
    CHECK(service.buildCommandLine(step) ==
          "cmake --version && ninja --version && ${CXX:-g++} --version");

    step.step = CiLinuxPipelineStep::FormatVerification;
    CHECK(service.buildCommandLine(step) ==
          "find apps/vigilio-browser/src apps/vigilio-browser/tests -name '*.cpp' -o -name '*.h' | "
          "while read -r f; do clang-format --dry-run --Werror \"$f\"; done");

    step.step = CiLinuxPipelineStep::StaticAnalysis;
    CHECK(service.buildCommandLine(step) == "clang-tidy -p build-ci-debug src/**/*.cpp");

    step.step = CiLinuxPipelineStep::ConfigurePreset;
    CHECK(service.buildCommandLine(step) == "cmake --preset ci-linux-debug -B build-ci-debug");

    step.step = CiLinuxPipelineStep::BuildCompilation;
    CHECK(service.buildCommandLine(step) == "cmake --build build-ci-debug --parallel");

    step.step = CiLinuxPipelineStep::TestSuiteExecution;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-debug --output-on-failure -L fast --timeout 15");

    step.step = CiLinuxPipelineStep::SanitizerVerification;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-debug --output-on-failure -L sanitizer");

    step.step = CiLinuxPipelineStep::ArtifactPackaging;
    CHECK(service.buildCommandLine(step) == "cpack --config build-ci-debug/CPackConfig.cmake");
}

// TEST-CI-LINUX-SRV-009: test_ci_linux_service_resource_and_memory_limits
void test_ci_linux_service_resource_and_memory_limits() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    // Exceder memoria máxima permitida (> 4 GB)
    plan.maxMemoryBytes = 5ULL * 1024 * 1024 * 1024; // 5 GB

    CiLinuxJobService service(plan);
    auto report = service.executePlan();
    CHECK(report.status == CiLinuxJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());

    // Plan con etapas vacías
    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    service.setPlan(plan);
    report = service.executePlan();
    CHECK(report.status == CiLinuxJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());
}

// TEST-CI-LINUX-SRV-010: test_ci_linux_service_timeout_handling
void test_ci_linux_service_timeout_handling() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    // Simular paso con timeout
    plan.steps[1].command = "timeout";
    plan.steps[1].name = "Etapa con tiempo límite excedido";

    CiLinuxJobService service(plan);
    auto report = service.executePlan();

    CHECK(report.status == CiLinuxJobStatus::TimedOut);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiLinuxStepStatus::Success);
    CHECK(report.stepResults[1].status == CiLinuxStepStatus::Omitted);
    CHECK(report.stepResults[2].status == CiLinuxStepStatus::Omitted);
}

// TEST-CI-LINUX-SRV-011: test_ci_linux_service_retry_backoff_on_contention
void test_ci_linux_service_retry_backoff_on_contention() {
    const std::filesystem::path retryDir = "build-test-retry-dir";
    std::filesystem::remove_all(retryDir);

    writeFile(retryDir / "sub" / "file.txt", "bloqueo transitorio");
    std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::none);

    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.buildDir = retryDir;

    CiLinuxJobService service(plan);
    CiLinuxServiceOptions opts;
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

    // Limpiar restos en caso de que persistiera
    std::error_code ec;
    std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::all, ec);
    std::filesystem::remove_all(retryDir, ec);

    CHECK(cleaned);
    CHECK(!std::filesystem::exists(retryDir));
}

// TEST-CI-LINUX-SRV-012: test_ci_linux_service_ux_copy_public_compliance
void test_ci_linux_service_ux_copy_public_compliance() {
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
    const std::vector<CiLinuxExecutionMode> modes = {
        CiLinuxExecutionMode::DryRun, CiLinuxExecutionMode::Simulated, CiLinuxExecutionMode::Live};
    for (auto m : modes) {
        checkNoProhibitedTerms(CiLinuxJobService::executionModeToString(m));
        checkNoProhibitedTerms(executionModeToString(m));
    }

    // Resúmenes de etapas
    CiLinuxStepResult sampleResult;
    sampleResult.step = CiLinuxPipelineStep::BuildCompilation;
    sampleResult.status = CiLinuxStepStatus::Success;
    sampleResult.durationMs = 120;
    sampleResult.exitCode = 0;
    sampleResult.outputSummary = "Compilación completada sin errores.";
    checkNoProhibitedTerms(CiLinuxJobService::formatStepSummary(sampleResult));
    checkNoProhibitedTerms(formatStepSummary(sampleResult));

    sampleResult.status = CiLinuxStepStatus::Failure;
    sampleResult.exitCode = 1;
    sampleResult.errorMessage = "Se encontraron errores en el código fuente.";
    checkNoProhibitedTerms(CiLinuxJobService::formatStepSummary(sampleResult));
}

// TEST-CI-LINUX-SRV-013: test_ci_linux_service_massive_multithreading_concurrency (TASK-005)
void test_ci_linux_service_massive_multithreading_concurrency() {
    CiLinuxJobService service;
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
                CHECK(rep.isPassed() || rep.status == CiLinuxJobStatus::Passed);
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    // 4 hilos ejecutando executeStep de etapas heterogéneas
    const std::vector<CiLinuxPipelineStep> testSteps = {
        CiLinuxPipelineStep::EnvironmentCheck, CiLinuxPipelineStep::ConfigurePreset,
        CiLinuxPipelineStep::BuildCompilation, CiLinuxPipelineStep::TestSuiteExecution};
    for (int i = 0; i < 4; ++i) {
        const auto step = testSteps[static_cast<std::size_t>(i)];
        workers.emplace_back([&, step]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int it = 0; it < 5; ++it) {
                auto res = service.executeStep(step);
                CHECK(res.step == step);
                CHECK(res.status == CiLinuxStepStatus::Success);
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    // 4 hilos realizando lecturas y actualizaciones protegidas por mutex
    workers.emplace_back([&]() {
        while (!startFlag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int it = 0; it < 10; ++it) {
            auto p = service.plan();
            CHECK(!p.presetName.empty());
        }
        completedWorkers.fetch_add(1, std::memory_order_release);
    });

    workers.emplace_back([&]() {
        while (!startFlag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int it = 0; it < 10; ++it) {
            auto opts = service.options();
            CHECK(opts.maxRetries > 0);
        }
        completedWorkers.fetch_add(1, std::memory_order_release);
    });

    workers.emplace_back([&]() {
        while (!startFlag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        const auto debugPlan = CiLinuxJobConfiguration::canonicalDebugPlan();
        const auto releasePlan = CiLinuxJobConfiguration::canonicalReleasePlan();
        for (int it = 0; it < 5; ++it) {
            service.setPlan(releasePlan);
            service.setPlan(debugPlan);
        }
        completedWorkers.fetch_add(1, std::memory_order_release);
    });

    workers.emplace_back([&]() {
        while (!startFlag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        CiLinuxServiceOptions opts1;
        opts1.mode = CiLinuxExecutionMode::DryRun;
        opts1.maxRetries = 3;
        CiLinuxServiceOptions opts2;
        opts2.mode = CiLinuxExecutionMode::DryRun;
        opts2.maxRetries = 2;
        for (int it = 0; it < 5; ++it) {
            service.setOptions(opts1);
            service.setOptions(opts2);
        }
        completedWorkers.fetch_add(1, std::memory_order_release);
    });

    // Iniciar simultáneamente los 16 hilos
    startFlag.store(true, std::memory_order_release);

    for (auto& w : workers) {
        w.join();
    }

    CHECK(completedWorkers.load(std::memory_order_acquire) == totalThreads);
}

// TEST-CI-LINUX-SRV-014: test_ci_linux_service_cooperative_cancellation_stress (TASK-006)
void test_ci_linux_service_cooperative_cancellation_stress() {
    CiLinuxJobService service;
    auto plan = CiLinuxJobConfiguration::canonicalReleasePlan();
    service.setPlan(plan);

    CiLinuxServiceOptions opts;
    opts.mode = CiLinuxExecutionMode::Simulated;
    service.setOptions(opts);

    for (int iter = 0; iter < 4; ++iter) {
        CancellationToken cancelToken;
        std::atomic<bool> threadStarted{false};
        CiLinuxJobReport report;

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

        CHECK(report.status == CiLinuxJobStatus::Cancelled);
        CHECK(!report.isPassed());
        CHECK(cancelToken.isCancelled());

        bool hasOmitted = false;
        for (const auto& stepRes : report.stepResults) {
            if (stepRes.status == CiLinuxStepStatus::Omitted) {
                hasOmitted = true;
                break;
            }
        }
        CHECK(hasOmitted);
    }

    // Cancelación granular de executeStep
    {
        CancellationToken token;
        token.requestCancellation();
        auto stepRes = service.executeStep(CiLinuxPipelineStep::BuildCompilation, &token);
        CHECK(stepRes.status == CiLinuxStepStatus::Omitted);
        CHECK(!stepRes.isSuccess());
    }
}

// TEST-CI-LINUX-SRV-015: test_ci_linux_service_permission_failure_injection (TASK-007)
void test_ci_linux_service_permission_failure_injection() {
    const std::filesystem::path permDir = "build-test-perm-dir";
    std::error_code ec;
    std::filesystem::remove_all(permDir, ec);

    const auto subDir = permDir / "nested";
    writeFile(subDir / "protected.bin", "permiso restringido");
    CHECK(std::filesystem::exists(subDir / "protected.bin"));

    std::filesystem::permissions(subDir, std::filesystem::perms::owner_read, ec);

    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.buildDir = permDir;

    CiLinuxJobService service(plan);
    CiLinuxServiceOptions opts;
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

// TEST-CI-LINUX-SRV-016: test_ci_linux_service_retry_backoff_transient (TASK-008)
void test_ci_linux_service_retry_backoff_transient() {
    const std::filesystem::path retryDir = "build-test-backoff-transient";
    std::error_code ec;
    std::filesystem::remove_all(retryDir, ec);

    const auto subDir = retryDir / "sub";
    writeFile(subDir / "transient.txt", "contención transitoria");
    CHECK(std::filesystem::exists(subDir / "transient.txt"));

    std::filesystem::permissions(subDir, std::filesystem::perms::none, ec);

    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.buildDir = retryDir;

    CiLinuxJobService service(plan);
    CiLinuxServiceOptions opts;
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

// TEST-CI-LINUX-SRV-017: test_ci_linux_service_fail_fast_vs_continue_on_error (TASK-009)
void test_ci_linux_service_fail_fast_vs_continue_on_error() {
    // Escenario 1: failFast == true, continueOnError == false
    {
        auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
        plan.steps[1].command = "false";
        plan.steps[1].name = "Formato con fallo";
        plan.steps[1].options.continueOnError = false;
        plan.failFast = true;

        CiLinuxJobService service(plan);
        CiLinuxServiceOptions opts;
        opts.failFast = true;
        service.setOptions(opts);

        auto report = service.executePlan();
        CHECK(report.status == CiLinuxJobStatus::Failed);
        CHECK(!report.isPassed());
        CHECK(report.stepResults.size() == 6);
        CHECK(report.stepResults[0].status == CiLinuxStepStatus::Success);
        CHECK(report.stepResults[1].status == CiLinuxStepStatus::Failure);
        CHECK(report.successfulStepsCount() == 1);
        CHECK(report.failedStepsCount() == 1);

        for (std::size_t i = 2; i < 6; ++i) {
            CHECK(report.stepResults[i].status == CiLinuxStepStatus::Omitted);
        }
    }

    // Escenario 2: failFast == false, continueOnError == true en etapa no obligatoria
    {
        auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
        plan.steps[1].command = "false";
        plan.steps[1].name = "Formato no obligatorio con fallo";
        plan.steps[1].isMandatory = false;
        plan.steps[1].options.continueOnError = true;
        plan.failFast = false;

        CiLinuxJobService service(plan);
        CiLinuxServiceOptions opts;
        opts.failFast = false;
        service.setOptions(opts);

        auto report = service.executePlan();
        CHECK(report.stepResults.size() == 6);
        CHECK(report.stepResults[0].status == CiLinuxStepStatus::Success);
        CHECK(report.stepResults[1].status == CiLinuxStepStatus::Failure);
        CHECK(report.stepResults[2].status == CiLinuxStepStatus::Success);
        CHECK(report.stepResults[3].status == CiLinuxStepStatus::Success);
        CHECK(report.stepResults[4].status == CiLinuxStepStatus::Success);
        CHECK(report.stepResults[5].status == CiLinuxStepStatus::Success);

        CHECK(report.successfulStepsCount() == 5);
        CHECK(report.failedStepsCount() == 1);
        CHECK(report.isPassed());
        CHECK(report.status == CiLinuxJobStatus::Passed);
    }
}

// TEST-CI-LINUX-SRV-018: test_ci_linux_service_cli_synthesis_advanced (TASK-010)
void test_ci_linux_service_cli_synthesis_advanced() {
    CiLinuxJobPlan customPlan;
    customPlan.presetName = "ci-linux-release-optimized";
    customPlan.buildDir = "_build/nested/linux-ci";
    customPlan.totalTimeoutSeconds = 1200;
    customPlan.maxMemoryBytes = 4294967296ULL;
    customPlan.failFast = true;

    CiLinuxJobService service(customPlan);

    // ConfigurePreset
    CiLinuxStepDefinition stepConfig;
    stepConfig.step = CiLinuxPipelineStep::ConfigurePreset;
    const std::string configCli = service.buildCommandLine(stepConfig);
    CHECK(configCli.find("cmake --preset ci-linux-release-optimized") != std::string::npos);
    CHECK(configCli.find("-B _build/nested/linux-ci") != std::string::npos);

    // BuildCompilation
    CiLinuxStepDefinition stepBuild;
    stepBuild.step = CiLinuxPipelineStep::BuildCompilation;
    const std::string buildCli = service.buildCommandLine(stepBuild);
    CHECK(buildCli.find("cmake --build _build/nested/linux-ci --parallel") != std::string::npos);

    // TestSuiteExecution
    CiLinuxStepDefinition stepTest;
    stepTest.step = CiLinuxPipelineStep::TestSuiteExecution;
    const std::string testCli = service.buildCommandLine(stepTest);
    CHECK(testCli.find("ctest --test-dir _build/nested/linux-ci --output-on-failure -L fast") !=
          std::string::npos);

    // SanitizerVerification
    CiLinuxStepDefinition stepSanitizer;
    stepSanitizer.step = CiLinuxPipelineStep::SanitizerVerification;
    const std::string sanitizerCli = service.buildCommandLine(stepSanitizer);
    CHECK(
        sanitizerCli.find("ctest --test-dir _build/nested/linux-ci --output-on-failure -L sanitizer") !=
        std::string::npos);

    // ArtifactPackaging
    CiLinuxStepDefinition stepPack;
    stepPack.step = CiLinuxPipelineStep::ArtifactPackaging;
    const std::string packCli = service.buildCommandLine(stepPack);
    CHECK(packCli.find("cpack --config _build/nested/linux-ci/CPackConfig.cmake") != std::string::npos);

    // StaticAnalysis
    CiLinuxStepDefinition stepStatic;
    stepStatic.step = CiLinuxPipelineStep::StaticAnalysis;
    const std::string staticCli = service.buildCommandLine(stepStatic);
    CHECK(staticCli.find("clang-tidy -p _build/nested/linux-ci src/**/*.cpp") != std::string::npos);

    // Paso con comando personalizado fuera de las etapas estándar
    CiLinuxStepDefinition customStep;
    customStep.step = static_cast<CiLinuxPipelineStep>(99);
    customStep.command = "ninja -C _build/nested/linux-ci custom_target";
    CHECK(service.buildCommandLine(customStep) == "ninja -C _build/nested/linux-ci custom_target");
}

} // namespace

int main() {
    try {
        std::cout << "Ejecutando pruebas operativas de CiLinuxJobService...\n";
        test_ci_linux_service_initialization();
        std::cout << "  [PASS] test_ci_linux_service_initialization\n";
        test_ci_linux_service_dry_run_pipeline();
        std::cout << "  [PASS] test_ci_linux_service_dry_run_pipeline\n";
        test_ci_linux_service_fail_fast();
        std::cout << "  [PASS] test_ci_linux_service_fail_fast\n";
        test_ci_linux_service_continue_on_error();
        std::cout << "  [PASS] test_ci_linux_service_continue_on_error\n";
        test_ci_linux_service_cancellation();
        std::cout << "  [PASS] test_ci_linux_service_cancellation\n";
        test_ci_linux_service_clean_build_directory_and_traversal();
        std::cout << "  [PASS] test_ci_linux_service_clean_build_directory_and_traversal\n";
        test_ci_linux_service_execute_step();
        std::cout << "  [PASS] test_ci_linux_service_execute_step\n";
        test_ci_linux_service_cli_synthesis();
        std::cout << "  [PASS] test_ci_linux_service_cli_synthesis\n";
        test_ci_linux_service_resource_and_memory_limits();
        std::cout << "  [PASS] test_ci_linux_service_resource_and_memory_limits\n";
        test_ci_linux_service_timeout_handling();
        std::cout << "  [PASS] test_ci_linux_service_timeout_handling\n";
        test_ci_linux_service_retry_backoff_on_contention();
        std::cout << "  [PASS] test_ci_linux_service_retry_backoff_on_contention\n";
        test_ci_linux_service_ux_copy_public_compliance();
        std::cout << "  [PASS] test_ci_linux_service_ux_copy_public_compliance\n";
        test_ci_linux_service_massive_multithreading_concurrency();
        std::cout << "  [PASS] test_ci_linux_service_massive_multithreading_concurrency\n";
        test_ci_linux_service_cooperative_cancellation_stress();
        std::cout << "  [PASS] test_ci_linux_service_cooperative_cancellation_stress\n";
        test_ci_linux_service_permission_failure_injection();
        std::cout << "  [PASS] test_ci_linux_service_permission_failure_injection\n";
        test_ci_linux_service_retry_backoff_transient();
        std::cout << "  [PASS] test_ci_linux_service_retry_backoff_transient\n";
        test_ci_linux_service_fail_fast_vs_continue_on_error();
        std::cout << "  [PASS] test_ci_linux_service_fail_fast_vs_continue_on_error\n";
        test_ci_linux_service_cli_synthesis_advanced();
        std::cout << "  [PASS] test_ci_linux_service_cli_synthesis_advanced\n";

        std::cout << "Todas las pruebas de CiLinuxJobService pasaron exitosamente.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo en prueba de CiLinuxJobService: " << e.what() << '\n';
        return 1;
    }
}
