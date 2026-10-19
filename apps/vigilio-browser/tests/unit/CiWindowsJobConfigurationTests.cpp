#include "core/CiWindowsJobConfiguration.h"
#include "core/CiWindowsJobTypes.h"
#include "core/CiWindowsJobValidator.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

// TEST-001: test_ci_windows_canonical_debug_plan
void test_ci_windows_canonical_debug_plan() {
    const auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    CHECK(plan.presetName == "ci-windows-debug");
    CHECK(plan.buildDir == "build-ci-win-debug");
    CHECK(plan.totalTimeoutSeconds == 600);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 6);
    CHECK(plan.steps[0].step == CiWindowsPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiWindowsPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiWindowsPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiWindowsPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiWindowsPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiWindowsPipelineStep::TestSuiteExecution);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiWindowsJobConfiguration::verifyInvariants(plan));
}

// TEST-002: test_ci_windows_canonical_release_plan
void test_ci_windows_canonical_release_plan() {
    const auto plan = CiWindowsJobConfiguration::canonicalReleasePlan();
    CHECK(plan.presetName == "ci-windows-release");
    CHECK(plan.buildDir == "build-ci-win-release");
    CHECK(plan.totalTimeoutSeconds == 900);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 7);
    CHECK(plan.steps[0].step == CiWindowsPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiWindowsPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiWindowsPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiWindowsPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiWindowsPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiWindowsPipelineStep::TestSuiteExecution);
    CHECK(plan.steps[6].step == CiWindowsPipelineStep::ArtifactPackaging);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiWindowsJobConfiguration::verifyInvariants(plan));
}

// TEST-003: test_ci_windows_plan_validation_valid
void test_ci_windows_plan_validation_valid() {
    const auto debugPlan = CiWindowsJobConfiguration::canonicalDebugPlan();
    CHECK(CiWindowsJobValidator::validatePlan(debugPlan) == CiWindowsJobError::None);

    const auto releasePlan = CiWindowsJobConfiguration::canonicalReleasePlan();
    CHECK(CiWindowsJobValidator::validatePlan(releasePlan) == CiWindowsJobError::None);
}

// TEST-004: test_ci_windows_plan_validation_empty_steps
void test_ci_windows_plan_validation_empty_steps() {
    auto plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.presetName = "";
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 0;
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 4000;
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 0;
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::MemoryLimitExceeded);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 16ULL * 1024 * 1024 * 1024;
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::MemoryLimitExceeded);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = "../unsafe";
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::UnsafePipelinePath);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].name = "";
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].command = "";
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);

    plan = CiWindowsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].options.timeoutMs = 0;
    CHECK(CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::InvalidWorkflowSchema);
}

// TEST-005: test_ci_windows_step_sequence_ordering
void test_ci_windows_step_sequence_ordering() {
    // Secuencia canónica válida de 6 etapas
    std::vector<CiWindowsPipelineStep> validSeq = {
        CiWindowsPipelineStep::EnvironmentCheck, CiWindowsPipelineStep::FormatVerification,
        CiWindowsPipelineStep::StaticAnalysis,   CiWindowsPipelineStep::ConfigurePreset,
        CiWindowsPipelineStep::BuildCompilation, CiWindowsPipelineStep::TestSuiteExecution};
    CHECK(CiWindowsJobValidator::validateStepSequence(validSeq) == CiWindowsJobError::None);

    // Secuencia válida con SanitizerVerification y ArtifactPackaging
    std::vector<CiWindowsPipelineStep> fullSeq = {
        CiWindowsPipelineStep::EnvironmentCheck,      CiWindowsPipelineStep::FormatVerification,
        CiWindowsPipelineStep::StaticAnalysis,        CiWindowsPipelineStep::ConfigurePreset,
        CiWindowsPipelineStep::BuildCompilation,      CiWindowsPipelineStep::TestSuiteExecution,
        CiWindowsPipelineStep::SanitizerVerification, CiWindowsPipelineStep::ArtifactPackaging};
    CHECK(CiWindowsJobValidator::validateStepSequence(fullSeq) == CiWindowsJobError::None);

    // Secuencia vacía
    CHECK(CiWindowsJobValidator::validateStepSequence({}) == CiWindowsJobError::InvalidWorkflowSchema);

    // Compilación antes de configuración
    std::vector<CiWindowsPipelineStep> invalidBuildBeforeConfig = {
        CiWindowsPipelineStep::BuildCompilation, CiWindowsPipelineStep::ConfigurePreset};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidBuildBeforeConfig) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Pruebas antes de compilación
    std::vector<CiWindowsPipelineStep> invalidTestsBeforeBuild = {
        CiWindowsPipelineStep::ConfigurePreset, CiWindowsPipelineStep::TestSuiteExecution,
        CiWindowsPipelineStep::BuildCompilation};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidTestsBeforeBuild) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Formato después de compilación
    std::vector<CiWindowsPipelineStep> invalidFormatAfterBuild = {
        CiWindowsPipelineStep::ConfigurePreset, CiWindowsPipelineStep::BuildCompilation,
        CiWindowsPipelineStep::FormatVerification};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidFormatAfterBuild) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Análisis estático después de compilación
    std::vector<CiWindowsPipelineStep> invalidStaticAfterBuild = {
        CiWindowsPipelineStep::ConfigurePreset, CiWindowsPipelineStep::BuildCompilation,
        CiWindowsPipelineStep::StaticAnalysis};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidStaticAfterBuild) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Verificación de entorno después de compilación
    std::vector<CiWindowsPipelineStep> invalidEnvAfterBuild = {CiWindowsPipelineStep::ConfigurePreset,
                                                               CiWindowsPipelineStep::BuildCompilation,
                                                               CiWindowsPipelineStep::EnvironmentCheck};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidEnvAfterBuild) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Sanitizador antes de compilación
    std::vector<CiWindowsPipelineStep> invalidSanitizerBeforeBuild = {
        CiWindowsPipelineStep::SanitizerVerification, CiWindowsPipelineStep::ConfigurePreset,
        CiWindowsPipelineStep::BuildCompilation};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidSanitizerBeforeBuild) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Empaquetado antes de pruebas (cuando hay pruebas)
    std::vector<CiWindowsPipelineStep> invalidPackagingBeforeTests = {
        CiWindowsPipelineStep::ConfigurePreset, CiWindowsPipelineStep::BuildCompilation,
        CiWindowsPipelineStep::ArtifactPackaging, CiWindowsPipelineStep::TestSuiteExecution};
    CHECK(CiWindowsJobValidator::validateStepSequence(invalidPackagingBeforeTests) ==
          CiWindowsJobError::DependencyOrderingViolation);

    // Etapa duplicada
    std::vector<CiWindowsPipelineStep> duplicateSeq = {CiWindowsPipelineStep::ConfigurePreset,
                                                       CiWindowsPipelineStep::ConfigurePreset,
                                                       CiWindowsPipelineStep::BuildCompilation};
    CHECK(CiWindowsJobValidator::validateStepSequence(duplicateSeq) ==
          CiWindowsJobError::DependencyOrderingViolation);
}

// TEST-006: test_ci_windows_environment_validation
void test_ci_windows_environment_validation() {
    // Entorno conforme MSVC 19.38
    CiWindowsEnvironment envMsvc;
    envMsvc.compiler = CiWindowsCompiler::MSVC;
    envMsvc.compilerMajorVersion = 19;
    envMsvc.compilerMinorVersion = 38;
    envMsvc.windowsSdkVersion = "10.0.22621.0";
    envMsvc.cmakeMajorVersion = 3;
    envMsvc.cmakeMinorVersion = 28;
    envMsvc.ninjaMajorVersion = 1;
    envMsvc.ninjaMinorVersion = 12;
    envMsvc.architecture = CiWindowsArchitecture::X86_64;
    envMsvc.windowsVersion = "Windows 11 23H2";
    envMsvc.totalMemoryBytes = 4294967296ULL;
    envMsvc.availableCores = 4;
    CHECK(CiWindowsJobValidator::validateEnvironment(envMsvc) == CiWindowsJobError::None);

    // Entorno conforme Clang-cl 18 en ARM64
    CiWindowsEnvironment envClang;
    envClang.compiler = CiWindowsCompiler::ClangCl;
    envClang.compilerMajorVersion = 18;
    envClang.compilerMinorVersion = 1;
    envClang.windowsSdkVersion = "10.0.22621.0";
    envClang.cmakeMajorVersion = 3;
    envClang.cmakeMinorVersion = 28;
    envClang.ninjaMajorVersion = 1;
    envClang.ninjaMinorVersion = 12;
    envClang.architecture = CiWindowsArchitecture::ARM64;
    envClang.windowsVersion = "Windows 11";
    envClang.totalMemoryBytes = 4294967296ULL;
    envClang.availableCores = 4;
    CHECK(CiWindowsJobValidator::validateEnvironment(envClang) == CiWindowsJobError::None);

    // Entorno conforme MinGW 13
    CiWindowsEnvironment envMinGw = envMsvc;
    envMinGw.compiler = CiWindowsCompiler::MinGW;
    envMinGw.compilerMajorVersion = 13;
    CHECK(CiWindowsJobValidator::validateEnvironment(envMinGw) == CiWindowsJobError::None);

    // MSVC obsoleto (< 19.38)
    auto badMsvc = envMsvc;
    badMsvc.compilerMinorVersion = 37;
    CHECK(CiWindowsJobValidator::validateEnvironment(badMsvc) == CiWindowsJobError::ToolchainMissing);

    // Clang-cl obsoleto (< 18)
    auto badClang = envClang;
    badClang.compilerMajorVersion = 17;
    CHECK(CiWindowsJobValidator::validateEnvironment(badClang) == CiWindowsJobError::ToolchainMissing);

    // MinGW obsoleto (< 13)
    auto badMinGw = envMinGw;
    badMinGw.compilerMajorVersion = 12;
    CHECK(CiWindowsJobValidator::validateEnvironment(badMinGw) == CiWindowsJobError::ToolchainMissing);

    // SDK ausente o vacío
    auto noSdk = envMsvc;
    noSdk.windowsSdkVersion = "";
    CHECK(CiWindowsJobValidator::validateEnvironment(noSdk) == CiWindowsJobError::WindowsSdkMissing);

    // SDK obsoleto (< 10.0.19041)
    auto oldSdk = envMsvc;
    oldSdk.windowsSdkVersion = "10.0.10240.0";
    CHECK(CiWindowsJobValidator::validateEnvironment(oldSdk) == CiWindowsJobError::WindowsSdkMissing);

    // SDK malformado
    auto malformedSdk = envMsvc;
    malformedSdk.windowsSdkVersion = "8.1";
    CHECK(CiWindowsJobValidator::validateEnvironment(malformedSdk) ==
          CiWindowsJobError::WindowsSdkMissing);

    // Versión de Windows incompatible
    auto badWin = envMsvc;
    badWin.windowsVersion = "Windows 7 Service Pack 1";
    CHECK(CiWindowsJobValidator::validateEnvironment(badWin) == CiWindowsJobError::ToolchainMissing);

    // CMake obsoleto (< 3.28)
    auto badCmake = envMsvc;
    badCmake.cmakeMajorVersion = 3;
    badCmake.cmakeMinorVersion = 24;
    CHECK(CiWindowsJobValidator::validateEnvironment(badCmake) == CiWindowsJobError::ToolchainMissing);

    // Ninja obsoleto (< 1.12)
    auto badNinja = envMsvc;
    badNinja.ninjaMajorVersion = 1;
    badNinja.ninjaMinorVersion = 10;
    CHECK(CiWindowsJobValidator::validateEnvironment(badNinja) == CiWindowsJobError::ToolchainMissing);

    // Memoria insuficiente (< 2 GB)
    auto lowMemory = envMsvc;
    lowMemory.totalMemoryBytes = 1073741824ULL;
    CHECK(CiWindowsJobValidator::validateEnvironment(lowMemory) == CiWindowsJobError::ToolchainMissing);

    // Cero núcleos
    auto zeroCores = envMsvc;
    zeroCores.availableCores = 0;
    CHECK(CiWindowsJobValidator::validateEnvironment(zeroCores) == CiWindowsJobError::ToolchainMissing);
}

// TEST-007: test_ci_windows_safe_build_path_traversal
void test_ci_windows_safe_build_path_traversal() {
    // Rutas canónicas autorizadas
    CHECK(CiWindowsJobValidator::isSafeBuildPath("build-ci-win-debug"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("build-ci-win-release"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("_build"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("_build/tests"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("build-test"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("${sourceDir}/build-ci-win-debug"));
    CHECK(CiWindowsJobValidator::isSafeBuildPath("${sourceDir}\\build-ci-win-release"));

    // Vectores de Directory Traversal en Unix y Windows
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("../build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("..\\build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("foo/../../bar"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("build-ci-win-debug/../escape"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("build-ci-win-debug\\..\\escape"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("..\\escaped"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath(""));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("/etc"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("\\Windows\\System32"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("/usr/bin"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("/"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("\\"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("C:/build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("C:\\build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("D:\\build-ci-win-debug"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("\\\\server\\share\\build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("//server/share/build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("\\\\?\\C:\\build"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("etc/passwd"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("home/user/build"));
}

// TEST-008: test_ci_windows_safe_build_path_dos_devices
void test_ci_windows_safe_build_path_dos_devices() {
    // Dispositivos reservados DOS directos
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("CON"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("PRN"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("AUX"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("NUL"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("COM1"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("COM9"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("LPT1"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("LPT9"));

    // Dispositivos reservados con extensiones y minúsculas
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("con"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("NUL.txt"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("nul.txt"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("aux.dat"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("com1.log"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("lpt2.tmp"));

    // Dispositivos en subdirectorios
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("build-ci-win-debug/CON"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("build-ci-win-debug\\NUL.txt"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("_build/aux/test"));
    CHECK(!CiWindowsJobValidator::isSafeBuildPath("_build\\COM3\\output"));
}

// TEST-009: test_ci_windows_supported_windows_versions
void test_ci_windows_supported_windows_versions() {
    // Versiones soportadas
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 11"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 11 23H2"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 11 22H2"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("windows 11"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 10"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 10 22H2"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("windows 10 build 19045"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows Server 2022"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("server 2022"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Win 11"));
    CHECK(CiWindowsJobConfiguration::isSupportedWindowsVersion("Win 10"));

    // Versiones no soportadas o ajenas
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 7"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 7 Service Pack 1"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows XP"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("winxp"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 8"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 8.1"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows Vista"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Windows 2000"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Ubuntu 24.04"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Linux"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("macOS 14 Sonoma"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("Solaris"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion("FreeBSD"));
    CHECK(!CiWindowsJobConfiguration::isSupportedWindowsVersion(""));
}

// TEST-010: test_ci_windows_step_options_and_results
void test_ci_windows_step_options_and_results() {
    CiWindowsStepOptions opts;
    opts.timeoutMs = 45000;
    opts.continueOnError = true;
    opts.requiredEnvVars = {"CI", "VIGILIO_WIN_CI"};
    opts.workingDirectory = "src";
    CHECK(opts.timeoutMs == 45000);
    CHECK(opts.continueOnError);
    CHECK(opts.requiredEnvVars.size() == 2);
    CHECK(opts.workingDirectory == "src");

    CiWindowsStepResult res1;
    res1.step = CiWindowsPipelineStep::ConfigurePreset;
    res1.status = CiWindowsStepStatus::Success;
    res1.exitCode = 0;
    res1.durationMs = 1200;
    res1.outputSummary = "Configuración correcta";
    CHECK(res1.isSuccess());

    CiWindowsStepResult res2;
    res2.step = CiWindowsPipelineStep::BuildCompilation;
    res2.status = CiWindowsStepStatus::Failure;
    res2.exitCode = 1;
    res2.durationMs = 5000;
    res2.errorMessage = "Error de sintaxis";
    CHECK(!res2.isSuccess());

    CiWindowsJobReport report;
    report.status = CiWindowsJobStatus::Failed;
    report.planName = "ci-windows-debug";
    report.durationMs = 6200;
    report.stepResults = {res1, res2};
    report.isClean = false;
    report.remediationHint = "Corrija los errores de compilación";

    CHECK(report.successfulStepsCount() == 1);
    CHECK(report.failedStepsCount() == 1);
    CHECK(!report.isPassed());

    report.status = CiWindowsJobStatus::Passed;
    report.stepResults[1].status = CiWindowsStepStatus::Success;
    report.isClean = true;
    CHECK(report.successfulStepsCount() == 2);
    CHECK(report.failedStepsCount() == 0);
    CHECK(report.isPassed());

    // CancellationToken
    CancellationToken token;
    CHECK(!token.isCancelled());
    token.requestCancellation();
    CHECK(token.isCancelled());

    // CiWindowsServiceOptions
    CiWindowsServiceOptions sOpts;
    CHECK(sOpts.mode == CiWindowsExecutionMode::DryRun);
    CHECK(sOpts.failFast);
    CHECK(sOpts.maxRetries == 3);
}

// TEST-011: test_ci_windows_ux_copy_public_compliance
void test_ci_windows_ux_copy_public_compliance() {
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

    // Estados de trabajo
    const std::vector<CiWindowsJobStatus> statuses = {
        CiWindowsJobStatus::Pending, CiWindowsJobStatus::Running,   CiWindowsJobStatus::Passed,
        CiWindowsJobStatus::Failed,  CiWindowsJobStatus::Cancelled, CiWindowsJobStatus::TimedOut};
    for (auto s : statuses) {
        checkNoProhibitedTerms(jobStatusToString(s));
    }

    // Errores de trabajo
    const std::vector<CiWindowsJobError> errors = {CiWindowsJobError::None,
                                                   CiWindowsJobError::ToolchainMissing,
                                                   CiWindowsJobError::WindowsSdkMissing,
                                                   CiWindowsJobError::ConfigurationFailed,
                                                   CiWindowsJobError::CompilationFailed,
                                                   CiWindowsJobError::TestExecutionFailed,
                                                   CiWindowsJobError::FormatDiscrepancy,
                                                   CiWindowsJobError::StaticAnalysisIssue,
                                                   CiWindowsJobError::UnsafePipelinePath,
                                                   CiWindowsJobError::TimeoutExceeded,
                                                   CiWindowsJobError::InvalidWorkflowSchema,
                                                   CiWindowsJobError::DependencyOrderingViolation,
                                                   CiWindowsJobError::MemoryLimitExceeded};
    for (auto e : errors) {
        checkNoProhibitedTerms(jobErrorToString(e));
        checkNoProhibitedTerms(generateRemediationHint(e));
        checkNoProhibitedTerms(generateRemediationHint(e, "Compilación del proyecto"));
    }

    // Etapas del pipeline
    const std::vector<CiWindowsPipelineStep> steps = {
        CiWindowsPipelineStep::EnvironmentCheck,      CiWindowsPipelineStep::FormatVerification,
        CiWindowsPipelineStep::StaticAnalysis,        CiWindowsPipelineStep::ConfigurePreset,
        CiWindowsPipelineStep::BuildCompilation,      CiWindowsPipelineStep::TestSuiteExecution,
        CiWindowsPipelineStep::SanitizerVerification, CiWindowsPipelineStep::ArtifactPackaging};
    for (auto st : steps) {
        checkNoProhibitedTerms(stepToString(st));
    }

    // Estados de etapa
    const std::vector<CiWindowsStepStatus> stepStatuses = {
        CiWindowsStepStatus::NotStarted, CiWindowsStepStatus::InProgress, CiWindowsStepStatus::Success,
        CiWindowsStepStatus::Failure, CiWindowsStepStatus::Omitted};
    for (auto ss : stepStatuses) {
        checkNoProhibitedTerms(stepStatusToString(ss));
    }

    // Arquitecturas y compiladores
    checkNoProhibitedTerms(architectureToString(CiWindowsArchitecture::X86_64));
    checkNoProhibitedTerms(architectureToString(CiWindowsArchitecture::ARM64));
    checkNoProhibitedTerms(compilerToString(CiWindowsCompiler::MSVC));
    checkNoProhibitedTerms(compilerToString(CiWindowsCompiler::ClangCl));
    checkNoProhibitedTerms(compilerToString(CiWindowsCompiler::MinGW));
}

// TEST-012: test_ci_windows_remediation_hints
void test_ci_windows_remediation_hints() {
    const std::vector<CiWindowsJobError> errors = {CiWindowsJobError::None,
                                                   CiWindowsJobError::ToolchainMissing,
                                                   CiWindowsJobError::WindowsSdkMissing,
                                                   CiWindowsJobError::ConfigurationFailed,
                                                   CiWindowsJobError::CompilationFailed,
                                                   CiWindowsJobError::TestExecutionFailed,
                                                   CiWindowsJobError::FormatDiscrepancy,
                                                   CiWindowsJobError::StaticAnalysisIssue,
                                                   CiWindowsJobError::UnsafePipelinePath,
                                                   CiWindowsJobError::TimeoutExceeded,
                                                   CiWindowsJobError::InvalidWorkflowSchema,
                                                   CiWindowsJobError::DependencyOrderingViolation,
                                                   CiWindowsJobError::MemoryLimitExceeded};

    for (auto err : errors) {
        const auto hint = generateRemediationHint(err);
        CHECK(!hint.empty());
        const auto hintWithStep = generateRemediationHint(err, "Verificación");
        CHECK(!hintWithStep.empty());
        if (err != CiWindowsJobError::None) {
            CHECK(hintWithStep.find("En la etapa 'Verificación':") != std::string::npos);
        }
    }
}

} // namespace

int main() {
    try {
        std::cout << "Iniciando pruebas unitarias de dominio para CI Windows job...\n";

        test_ci_windows_canonical_debug_plan();
        std::cout << "  [PASS] test_ci_windows_canonical_debug_plan\n";
        test_ci_windows_canonical_release_plan();
        std::cout << "  [PASS] test_ci_windows_canonical_release_plan\n";
        test_ci_windows_plan_validation_valid();
        std::cout << "  [PASS] test_ci_windows_plan_validation_valid\n";
        test_ci_windows_plan_validation_empty_steps();
        std::cout << "  [PASS] test_ci_windows_plan_validation_empty_steps\n";
        test_ci_windows_step_sequence_ordering();
        std::cout << "  [PASS] test_ci_windows_step_sequence_ordering\n";
        test_ci_windows_environment_validation();
        std::cout << "  [PASS] test_ci_windows_environment_validation\n";
        test_ci_windows_safe_build_path_traversal();
        std::cout << "  [PASS] test_ci_windows_safe_build_path_traversal\n";
        test_ci_windows_safe_build_path_dos_devices();
        std::cout << "  [PASS] test_ci_windows_safe_build_path_dos_devices\n";
        test_ci_windows_supported_windows_versions();
        std::cout << "  [PASS] test_ci_windows_supported_windows_versions\n";
        test_ci_windows_step_options_and_results();
        std::cout << "  [PASS] test_ci_windows_step_options_and_results\n";
        test_ci_windows_ux_copy_public_compliance();
        std::cout << "  [PASS] test_ci_windows_ux_copy_public_compliance\n";
        test_ci_windows_remediation_hints();
        std::cout << "  [PASS] test_ci_windows_remediation_hints\n";

        std::cout << "Todas las pruebas de dominio de CI Windows job pasaron exitosamente.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo en prueba: " << e.what() << '\n';
        return 1;
    }
}
