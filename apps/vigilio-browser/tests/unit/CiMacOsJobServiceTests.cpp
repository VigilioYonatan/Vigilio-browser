#include "core/CiMacOsJobConfiguration.h"
#include "core/CiMacOsJobService.h"
#include "core/CiMacOsJobTypes.h"
#include "core/CiMacOsJobValidator.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
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

// TEST-CI-MAC-SRV-001: test_ci_macos_service_initialization
void test_ci_macos_service_initialization() {
    CiMacOsJobService defaultService;
    const auto defPlan = defaultService.plan();
    CHECK(defPlan.presetName == "ci-macos-debug");
    CHECK(defPlan.buildDir == "build-ci-macos-debug");
    CHECK(defPlan.steps.size() == 6);

    const auto defOpts = defaultService.options();
    CHECK(defOpts.mode == CiMacOsExecutionMode::DryRun);
    CHECK(defOpts.failFast);
    CHECK(defOpts.maxRetries == 3);
    CHECK(defOpts.retryBackoffMs == 50);
    CHECK(defOpts.captureOutput);

    // Constructor con plan explícito
    auto customPlan = CiMacOsJobConfiguration::canonicalReleasePlan();
    CiMacOsJobService releaseService(customPlan);
    CHECK(releaseService.plan().presetName == "ci-macos-release");
    CHECK(releaseService.plan().steps.size() == 7);

    // Modificación de plan y opciones
    customPlan.totalTimeoutSeconds = 800;
    defaultService.setPlan(customPlan);
    CHECK(defaultService.plan().totalTimeoutSeconds == 800);

    CiMacOsServiceOptions newOpts;
    newOpts.mode = CiMacOsExecutionMode::Simulated;
    newOpts.failFast = false;
    newOpts.maxRetries = 5;
    newOpts.retryBackoffMs = 100;
    newOpts.captureOutput = false;
    defaultService.setOptions(newOpts);
    CHECK(defaultService.options() == newOpts);
}

// TEST-CI-MAC-SRV-002: test_ci_macos_service_concurrency_and_synchronization
void test_ci_macos_service_concurrency_and_synchronization() {
    CiMacOsJobService service;
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

    CHECK(service.plan().presetName == "ci-macos-debug");
}

// TEST-CI-MAC-SRV-003: test_ci_macos_service_dry_run_pipeline
void test_ci_macos_service_dry_run_pipeline() {
    CiMacOsJobService service;
    auto report = service.executePlan();

    CHECK(report.status == CiMacOsJobStatus::Passed);
    CHECK(report.isPassed());
    CHECK(report.isClean);
    CHECK(report.planName == "ci-macos-debug");
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 6);
    CHECK(report.failedStepsCount() == 0);
    CHECK(!report.remediationHint.empty());

    for (const auto& stepRes : report.stepResults) {
        CHECK(stepRes.status == CiMacOsStepStatus::Success);
        CHECK(stepRes.isSuccess());
        CHECK(stepRes.exitCode == 0);
        CHECK(!stepRes.outputSummary.empty());
    }

    // Ejecución con plan de release canónico (7 etapas, incluye BundleVerification)
    auto releasePlan = CiMacOsJobConfiguration::canonicalReleasePlan();
    service.setPlan(releasePlan);
    auto releaseReport = service.executePlan();

    CHECK(releaseReport.status == CiMacOsJobStatus::Passed);
    CHECK(releaseReport.isPassed());
    CHECK(releaseReport.isClean);
    CHECK(releaseReport.stepResults.size() == 7);
    CHECK(releaseReport.successfulStepsCount() == 7);
    CHECK(releaseReport.stepResults.back().step == CiMacOsPipelineStep::BundleVerification);
    CHECK(releaseReport.stepResults.back().isSuccess());
}

// TEST-CI-MAC-SRV-004: test_ci_macos_service_fail_fast
void test_ci_macos_service_fail_fast() {
    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    // Simular que la etapa de formato falla
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación de formato fallida";

    CiMacOsJobService service(plan);
    auto report = service.executePlan();

    CHECK(report.status == CiMacOsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.isClean);
    CHECK(report.stepResults.size() == 6);
    CHECK(report.successfulStepsCount() == 1); // EnvironmentCheck pasó
    CHECK(report.failedStepsCount() == 1);     // FormatVerification falló

    CHECK(report.stepResults[0].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiMacOsStepStatus::Failure);
    CHECK(report.stepResults[1].exitCode == 1);

    // Etapas 2 a 5 deben estar omitidas por fail-fast
    for (std::size_t i = 2; i < 6; ++i) {
        CHECK(report.stepResults[i].status == CiMacOsStepStatus::Omitted);
    }
}

// TEST-CI-MAC-SRV-005: test_ci_macos_service_continue_on_error
void test_ci_macos_service_continue_on_error() {
    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    // Etapa 1 configurada con continueOnError y no obligatoria
    plan.steps[1].command = "false";
    plan.steps[1].name = "Comprobación no obligatoria fallida";
    plan.steps[1].isMandatory = false;
    plan.steps[1].options.continueOnError = true;

    CiMacOsJobService service(plan);
    auto report = service.executePlan();

    // El pipeline continuó ejecutando el resto de etapas
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiMacOsStepStatus::Failure);
    CHECK(report.stepResults[2].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[3].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[4].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[5].status == CiMacOsStepStatus::Success);

    CHECK(report.failedStepsCount() == 1);
    CHECK(report.successfulStepsCount() == 5);
    CHECK(report.isPassed());
    CHECK(report.status == CiMacOsJobStatus::Passed);
}

// TEST-CI-MAC-SRV-006: test_ci_macos_service_cancellation
void test_ci_macos_service_cancellation() {
    // Caso 1: Cancelación antes de iniciar
    {
        CiMacOsJobService service;
        CancellationToken cancelToken;
        cancelToken.requestCancellation();

        auto report = service.executePlan(&cancelToken);
        CHECK(report.status == CiMacOsJobStatus::Cancelled);
        CHECK(!report.isPassed());
        CHECK(report.stepResults.empty());
    }

    // Caso 2: Cancelación cooperativa multihilo durante la ejecución
    {
        CiMacOsJobService service;
        CiMacOsServiceOptions opts;
        opts.mode = CiMacOsExecutionMode::Simulated;
        service.setOptions(opts);

        CancellationToken cancelToken;
        std::atomic<bool> workerStarted{false};
        CiMacOsJobReport report;

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

        CHECK(report.status == CiMacOsJobStatus::Cancelled);
        CHECK(!report.isPassed());
        // Al menos una etapa debe haber sido omitida
        bool hasOmitted = false;
        for (const auto& res : report.stepResults) {
            if (res.status == CiMacOsStepStatus::Omitted) {
                hasOmitted = true;
                break;
            }
        }
        CHECK(hasOmitted);
    }
}

// TEST-CI-MAC-SRV-007: test_ci_macos_service_clean_build_directory
void test_ci_macos_service_clean_build_directory() {
    const std::filesystem::path safeRelativeDir = "build-ci-macos-test-clean-dir";
    std::filesystem::remove_all(safeRelativeDir);

    // Crear directorio seguro con archivos
    writeFile(safeRelativeDir / "sub" / "artifact.o", "dummy binary content");
    CHECK(std::filesystem::exists(safeRelativeDir / "sub" / "artifact.o"));

    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = safeRelativeDir;

    CiMacOsJobService service(plan);
    std::string errorMsg;
    CHECK(service.cleanBuildDirectory(&errorMsg));
    CHECK(!std::filesystem::exists(safeRelativeDir));

    // Limpieza cuando el directorio ya no existe devuelve true
    CHECK(service.cleanBuildDirectory(&errorMsg));

    // Pruebas de rutas inseguras con Directory Traversal (..)
    plan.buildDir = "../unsafe_traversal_dir";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Barra invertida (..\)
    plan.buildDir = "..\\unsafe_traversal";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    // Rutas absolutas del sistema host macOS
    const std::vector<std::string> unsafeRoots = {
        "/System",     "/Library",  "/Applications",   "/private",
        "/usr",        "/Volumes",  "/System/Library", "/Library/Frameworks",
        "/tmp/unsafe", "/var/root", "~/.config"};
    for (const auto& unsafePath : unsafeRoots) {
        plan.buildDir = unsafePath;
        service.setPlan(plan);
        errorMsg.clear();
        CHECK(!service.cleanBuildDirectory(&errorMsg));
        CHECK(!errorMsg.empty());
    }

    // Artefactos conflictivos específicos de macOS (.DS_Store y ._*)
    plan.buildDir = "build-ci-macos/.DS_Store";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());

    plan.buildDir = "build-ci-macos/._ResourceFork";
    service.setPlan(plan);
    errorMsg.clear();
    CHECK(!service.cleanBuildDirectory(&errorMsg));
    CHECK(!errorMsg.empty());
}

// TEST-CI-MAC-SRV-008: test_ci_macos_service_execute_step
void test_ci_macos_service_execute_step() {
    CiMacOsJobService service;

    // Ejecución de etapa individual
    auto stepRes = service.executeStep(CiMacOsPipelineStep::ConfigurePreset);
    CHECK(stepRes.step == CiMacOsPipelineStep::ConfigurePreset);
    CHECK(stepRes.status == CiMacOsStepStatus::Success);
    CHECK(stepRes.exitCode == 0);
    CHECK(stepRes.isSuccess());

    // Ejecución de BundleVerification
    auto bundleRes = service.executeStep(CiMacOsPipelineStep::BundleVerification);
    CHECK(bundleRes.step == CiMacOsPipelineStep::BundleVerification);
    CHECK(bundleRes.status == CiMacOsStepStatus::Success);
    CHECK(bundleRes.isSuccess());

    // Ejecución con cancelación
    CancellationToken cancelToken;
    cancelToken.requestCancellation();
    auto cancelledRes = service.executeStep(CiMacOsPipelineStep::BuildCompilation, &cancelToken);
    CHECK(cancelledRes.step == CiMacOsPipelineStep::BuildCompilation);
    CHECK(cancelledRes.status == CiMacOsStepStatus::Omitted);
    CHECK(!cancelledRes.isSuccess());

    // Ejecución con variable de entorno faltante
    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].options.requiredEnvVars = {"NON_EXISTENT_ENV_VAR_TEST_12345"};
    service.setPlan(plan);
    auto envFailRes = service.executeStep(CiMacOsPipelineStep::EnvironmentCheck);
    CHECK(envFailRes.status == CiMacOsStepStatus::Failure);
    CHECK(envFailRes.exitCode == 1);
    CHECK(!envFailRes.errorMessage.empty());
}

// TEST-CI-MAC-SRV-009: test_ci_macos_service_cli_synthesis
void test_ci_macos_service_cli_synthesis() {
    CiMacOsJobService service;

    CiMacOsStepDefinition step;
    step.step = CiMacOsPipelineStep::EnvironmentCheck;
    CHECK(service.buildCommandLine(step) == "cmake --version && ninja --version && clang --version");

    step.step = CiMacOsPipelineStep::FormatVerification;
    CHECK(service.buildCommandLine(step) ==
          "find apps/vigilio-browser/src apps/vigilio-browser/tests -name '*.cpp' -o -name '*.h' "
          "| while read -r f; do clang-format --dry-run --Werror \"$f\"; done");

    step.step = CiMacOsPipelineStep::StaticAnalysis;
    CHECK(service.buildCommandLine(step) ==
          "clang-tidy -p build-ci-macos-debug apps/vigilio-browser/src/**/*.cpp");

    step.step = CiMacOsPipelineStep::ConfigurePreset;
    CHECK(service.buildCommandLine(step) == "cmake --preset ci-macos-debug -B build-ci-macos-debug");

    step.step = CiMacOsPipelineStep::BuildCompilation;
    CHECK(service.buildCommandLine(step) == "cmake --build build-ci-macos-debug --parallel");

    step.step = CiMacOsPipelineStep::TestSuiteExecution;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-macos-debug --output-on-failure -L fast --timeout 15");

    step.step = CiMacOsPipelineStep::SanitizerVerification;
    CHECK(service.buildCommandLine(step) ==
          "ctest --test-dir build-ci-macos-debug --output-on-failure -L sanitizer");

    step.step = CiMacOsPipelineStep::BundleVerification;
    CHECK(service.buildCommandLine(step) ==
          "test -d build-ci-macos-debug/VigilioBrowser.app/Contents/MacOS && test -f "
          "build-ci-macos-debug/VigilioBrowser.app/Contents/Info.plist");

    // Comando personalizado
    CiMacOsStepDefinition customStep;
    constexpr uint8_t unknownStepVal = 99;
    std::memcpy(&customStep.step, &unknownStepVal, sizeof(unknownStepVal));
    customStep.command = "ninja -C build-ci-macos-debug custom_target";
    CHECK(service.buildCommandLine(customStep) == "ninja -C build-ci-macos-debug custom_target");
}

// TEST-CI-MAC-SRV-010: test_ci_macos_service_resource_and_memory_limits
void test_ci_macos_service_resource_and_memory_limits() {
    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    // Exceder memoria máxima permitida (> 4 GB)
    plan.maxMemoryBytes = 5ULL * 1024 * 1024 * 1024; // 5 GB

    CiMacOsJobService service(plan);
    auto report = service.executePlan();
    CHECK(report.status == CiMacOsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());

    // Plan con etapas vacías
    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    service.setPlan(plan);
    report = service.executePlan();
    CHECK(report.status == CiMacOsJobStatus::Failed);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());

    // Simular paso con timeout
    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps[1].command = "timeout";
    plan.steps[1].name = "Etapa con tiempo límite excedido";

    service.setPlan(plan);
    report = service.executePlan();

    CHECK(report.status == CiMacOsJobStatus::TimedOut);
    CHECK(!report.isPassed());
    CHECK(!report.remediationHint.empty());
    CHECK(report.stepResults.size() == 6);
    CHECK(report.stepResults[0].status == CiMacOsStepStatus::Success);
    CHECK(report.stepResults[1].status == CiMacOsStepStatus::Omitted);
    CHECK(report.stepResults[2].status == CiMacOsStepStatus::Omitted);
}

// TEST-CI-MAC-SRV-011: test_ci_macos_service_retry_backoff_and_timeout
void test_ci_macos_service_retry_backoff_and_timeout() {
    const std::filesystem::path retryDir = "build-ci-macos-test-retry-dir";
    std::filesystem::remove_all(retryDir);

    writeFile(retryDir / "sub" / "file.txt", "bloqueo transitorio");
    std::filesystem::permissions(retryDir / "sub", std::filesystem::perms::none);

    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = retryDir;

    CiMacOsJobService service(plan);
    CiMacOsServiceOptions opts;
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

// TEST-CI-MAC-SRV-012: test_ci_macos_service_ux_copy_public_compliance
void test_ci_macos_service_ux_copy_public_compliance() {
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
    const std::vector<CiMacOsExecutionMode> modes = {
        CiMacOsExecutionMode::DryRun, CiMacOsExecutionMode::Simulated, CiMacOsExecutionMode::Live};
    for (auto m : modes) {
        checkNoProhibitedTerms(CiMacOsJobService::executionModeToString(m));
        checkNoProhibitedTerms(executionModeToString(m));
    }

    // Resúmenes de etapas
    CiMacOsStepResult sampleResult;
    sampleResult.step = CiMacOsPipelineStep::BuildCompilation;
    sampleResult.status = CiMacOsStepStatus::Success;
    sampleResult.durationMs = 120;
    sampleResult.exitCode = 0;
    sampleResult.outputSummary = "Compilación completada sin errores.";
    checkNoProhibitedTerms(CiMacOsJobService::formatStepSummary(sampleResult));
    checkNoProhibitedTerms(formatStepSummary(sampleResult));

    sampleResult.status = CiMacOsStepStatus::Failure;
    sampleResult.exitCode = 1;
    sampleResult.errorMessage = "Se encontraron errores en el código fuente.";
    checkNoProhibitedTerms(CiMacOsJobService::formatStepSummary(sampleResult));

    // Pistas de remediación de reporte
    CiMacOsJobService service;
    auto report = service.executePlan();
    checkNoProhibitedTerms(report.remediationHint);
}

// Test adicional de concurrencia masiva multihilo
void test_ci_macos_service_massive_multithreading_concurrency() {
    CiMacOsJobService service;
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
                CHECK(rep.isPassed() || rep.status == CiMacOsJobStatus::Passed);
            }
            completedWorkers.fetch_add(1, std::memory_order_release);
        });
    }

    // 4 hilos ejecutando executeStep de etapas heterogéneas
    const std::vector<CiMacOsPipelineStep> testSteps = {
        CiMacOsPipelineStep::EnvironmentCheck, CiMacOsPipelineStep::ConfigurePreset,
        CiMacOsPipelineStep::BuildCompilation, CiMacOsPipelineStep::TestSuiteExecution};
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
                CiMacOsServiceOptions opts;
                opts.mode =
                    (it % 2 == 0) ? CiMacOsExecutionMode::DryRun : CiMacOsExecutionMode::Simulated;
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

} // namespace

int main() {
    try {
        std::cout << "Ejecutando pruebas operativas de CiMacOsJobService...\n";
        test_ci_macos_service_initialization();
        std::cout << "  [PASS] test_ci_macos_service_initialization\n";
        test_ci_macos_service_concurrency_and_synchronization();
        std::cout << "  [PASS] test_ci_macos_service_concurrency_and_synchronization\n";
        test_ci_macos_service_dry_run_pipeline();
        std::cout << "  [PASS] test_ci_macos_service_dry_run_pipeline\n";
        test_ci_macos_service_fail_fast();
        std::cout << "  [PASS] test_ci_macos_service_fail_fast\n";
        test_ci_macos_service_continue_on_error();
        std::cout << "  [PASS] test_ci_macos_service_continue_on_error\n";
        test_ci_macos_service_cancellation();
        std::cout << "  [PASS] test_ci_macos_service_cancellation\n";
        test_ci_macos_service_clean_build_directory();
        std::cout << "  [PASS] test_ci_macos_service_clean_build_directory\n";
        test_ci_macos_service_execute_step();
        std::cout << "  [PASS] test_ci_macos_service_execute_step\n";
        test_ci_macos_service_cli_synthesis();
        std::cout << "  [PASS] test_ci_macos_service_cli_synthesis\n";
        test_ci_macos_service_resource_and_memory_limits();
        std::cout << "  [PASS] test_ci_macos_service_resource_and_memory_limits\n";
        test_ci_macos_service_retry_backoff_and_timeout();
        std::cout << "  [PASS] test_ci_macos_service_retry_backoff_and_timeout\n";
        test_ci_macos_service_ux_copy_public_compliance();
        std::cout << "  [PASS] test_ci_macos_service_ux_copy_public_compliance\n";
        test_ci_macos_service_massive_multithreading_concurrency();
        std::cout << "  [PASS] test_ci_macos_service_massive_multithreading_concurrency\n";

        std::cout << "Todas las pruebas de CiMacOsJobService pasaron exitosamente.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo en prueba de CiMacOsJobService: " << e.what() << '\n';
        return 1;
    }
}
