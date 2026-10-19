#include "core/CiLinuxJobConfiguration.h"
#include "core/CiLinuxJobTypes.h"
#include "core/CiLinuxJobValidator.h"

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

// TEST-001: test_ci_linux_canonical_debug_plan
void test_ci_linux_canonical_debug_plan() {
    const auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    CHECK(plan.presetName == "ci-linux-debug");
    CHECK(plan.buildDir == "build-ci-debug");
    CHECK(plan.totalTimeoutSeconds == 600);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 6);
    CHECK(plan.steps[0].step == CiLinuxPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiLinuxPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiLinuxPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiLinuxPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiLinuxPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiLinuxPipelineStep::TestSuiteExecution);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiLinuxJobConfiguration::verifyInvariants(plan));
}

// TEST-002: test_ci_linux_canonical_release_plan
void test_ci_linux_canonical_release_plan() {
    const auto plan = CiLinuxJobConfiguration::canonicalReleasePlan();
    CHECK(plan.presetName == "ci-linux-release");
    CHECK(plan.buildDir == "build-ci-release");
    CHECK(plan.totalTimeoutSeconds == 900);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 7);
    CHECK(plan.steps[0].step == CiLinuxPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiLinuxPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiLinuxPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiLinuxPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiLinuxPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiLinuxPipelineStep::TestSuiteExecution);
    CHECK(plan.steps[6].step == CiLinuxPipelineStep::ArtifactPackaging);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiLinuxJobConfiguration::verifyInvariants(plan));
}

// TEST-003: test_ci_linux_plan_validation_valid
void test_ci_linux_plan_validation_valid() {
    const auto debugPlan = CiLinuxJobConfiguration::canonicalDebugPlan();
    CHECK(CiLinuxJobValidator::validatePlan(debugPlan) == CiLinuxJobError::None);

    const auto releasePlan = CiLinuxJobConfiguration::canonicalReleasePlan();
    CHECK(CiLinuxJobValidator::validatePlan(releasePlan) == CiLinuxJobError::None);
}

// TEST-004: test_ci_linux_plan_validation_empty_steps
void test_ci_linux_plan_validation_empty_steps() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.presetName = "";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 0;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 4000;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 0;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 16ULL * 1024 * 1024 * 1024;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.buildDir = "../unsafe";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::UnsafePipelinePath);

    plan = CiLinuxJobConfiguration::canonicalDebugPlan();
    plan.steps[0].name = "";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);
}

// TEST-005: test_ci_linux_step_sequence_ordering
void test_ci_linux_step_sequence_ordering() {
    // Secuencia canónica válida
    std::vector<CiLinuxPipelineStep> validSeq = {
        CiLinuxPipelineStep::EnvironmentCheck, CiLinuxPipelineStep::FormatVerification,
        CiLinuxPipelineStep::StaticAnalysis,   CiLinuxPipelineStep::ConfigurePreset,
        CiLinuxPipelineStep::BuildCompilation, CiLinuxPipelineStep::TestSuiteExecution};
    CHECK(CiLinuxJobValidator::validateStepSequence(validSeq) == CiLinuxJobError::None);

    // Secuencia válida con empaquetado
    validSeq.push_back(CiLinuxPipelineStep::ArtifactPackaging);
    CHECK(CiLinuxJobValidator::validateStepSequence(validSeq) == CiLinuxJobError::None);

    // Secuencia vacía
    CHECK(CiLinuxJobValidator::validateStepSequence({}) == CiLinuxJobError::InvalidWorkflowSchema);

    // Compilación antes de configuración
    std::vector<CiLinuxPipelineStep> invalidBuildBeforeConfig = {CiLinuxPipelineStep::BuildCompilation,
                                                                 CiLinuxPipelineStep::ConfigurePreset};
    CHECK(CiLinuxJobValidator::validateStepSequence(invalidBuildBeforeConfig) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Pruebas antes de compilación
    std::vector<CiLinuxPipelineStep> invalidTestsBeforeBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                                CiLinuxPipelineStep::TestSuiteExecution,
                                                                CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(invalidTestsBeforeBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Formato después de compilación
    std::vector<CiLinuxPipelineStep> invalidFormatAfterBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                                CiLinuxPipelineStep::BuildCompilation,
                                                                CiLinuxPipelineStep::FormatVerification};
    CHECK(CiLinuxJobValidator::validateStepSequence(invalidFormatAfterBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Sanitizador antes de compilación
    std::vector<CiLinuxPipelineStep> invalidSanitizerBeforeBuild = {
        CiLinuxPipelineStep::SanitizerVerification, CiLinuxPipelineStep::ConfigurePreset,
        CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(invalidSanitizerBeforeBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Empaquetado antes de pruebas (cuando hay pruebas)
    std::vector<CiLinuxPipelineStep> invalidPackagingBeforeTests = {
        CiLinuxPipelineStep::ConfigurePreset, CiLinuxPipelineStep::BuildCompilation,
        CiLinuxPipelineStep::ArtifactPackaging, CiLinuxPipelineStep::TestSuiteExecution};
    CHECK(CiLinuxJobValidator::validateStepSequence(invalidPackagingBeforeTests) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Etapa duplicada
    std::vector<CiLinuxPipelineStep> duplicateSeq = {CiLinuxPipelineStep::ConfigurePreset,
                                                     CiLinuxPipelineStep::ConfigurePreset,
                                                     CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(duplicateSeq) ==
          CiLinuxJobError::DependencyOrderingViolation);
}

// TEST-006: test_ci_linux_environment_validation
void test_ci_linux_environment_validation() {
    // Entorno conforme GCC 14
    CiLinuxEnvironment envGcc;
    envGcc.compiler = CiLinuxCompiler::GCC;
    envGcc.compilerMajorVersion = 14;
    envGcc.compilerMinorVersion = 0;
    envGcc.cmakeMajorVersion = 3;
    envGcc.cmakeMinorVersion = 28;
    envGcc.architecture = CiLinuxArchitecture::X86_64;
    envGcc.totalMemoryBytes = 4294967296ULL;
    envGcc.availableCores = 4;
    CHECK(CiLinuxJobValidator::validateEnvironment(envGcc) == CiLinuxJobError::None);

    // Entorno conforme Clang 18 en AArch64
    CiLinuxEnvironment envClang;
    envClang.compiler = CiLinuxCompiler::Clang;
    envClang.compilerMajorVersion = 18;
    envClang.compilerMinorVersion = 1;
    envClang.cmakeMajorVersion = 3;
    envClang.cmakeMinorVersion = 24;
    envClang.architecture = CiLinuxArchitecture::AArch64;
    envClang.totalMemoryBytes = 2147483648ULL;
    envClang.availableCores = 2;
    CHECK(CiLinuxJobValidator::validateEnvironment(envClang) == CiLinuxJobError::None);

    // GCC obsoleto (< 14)
    auto badGcc = envGcc;
    badGcc.compilerMajorVersion = 13;
    CHECK(CiLinuxJobValidator::validateEnvironment(badGcc) == CiLinuxJobError::ToolchainMissing);

    // Clang obsoleto (< 18)
    auto badClang = envClang;
    badClang.compilerMajorVersion = 17;
    CHECK(CiLinuxJobValidator::validateEnvironment(badClang) == CiLinuxJobError::ToolchainMissing);

    // CMake obsoleto (< 3.24)
    auto badCmake = envGcc;
    badCmake.cmakeMajorVersion = 3;
    badCmake.cmakeMinorVersion = 20;
    CHECK(CiLinuxJobValidator::validateEnvironment(badCmake) == CiLinuxJobError::ToolchainMissing);

    // Memoria insuficiente (< 2 GB)
    auto lowMemory = envGcc;
    lowMemory.totalMemoryBytes = 1073741824ULL;
    CHECK(CiLinuxJobValidator::validateEnvironment(lowMemory) == CiLinuxJobError::ToolchainMissing);

    // Cero núcleos
    auto zeroCores = envGcc;
    zeroCores.availableCores = 0;
    CHECK(CiLinuxJobValidator::validateEnvironment(zeroCores) == CiLinuxJobError::ToolchainMissing);
}

// TEST-007: test_ci_linux_safe_build_path_traversal
void test_ci_linux_safe_build_path_traversal() {
    // Rutas canónicas autorizadas
    CHECK(CiLinuxJobValidator::isSafeBuildPath("build-ci-debug"));
    CHECK(CiLinuxJobValidator::isSafeBuildPath("build-ci-release"));
    CHECK(CiLinuxJobValidator::isSafeBuildPath("_build"));
    CHECK(CiLinuxJobValidator::isSafeBuildPath("_build/tests"));
    CHECK(CiLinuxJobValidator::isSafeBuildPath("build-test"));
    CHECK(CiLinuxJobValidator::isSafeBuildPath("${sourceDir}/build-ci-debug"));

    // Vectores de escape y Directory Traversal
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("../build"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("foo/../../bar"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("build-ci-debug/../escape"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("..\\escaped"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath(""));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("/etc"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("/usr/bin"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("/"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("/var/tmp"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("C:/build"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("D:\\build"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("etc/passwd"));
    CHECK(!CiLinuxJobValidator::isSafeBuildPath("home/user/build"));
}

// TEST-008: test_ci_linux_supported_distros
void test_ci_linux_supported_distros() {
    // Soportadas
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Ubuntu"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Ubuntu 24.04 LTS"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("ubuntu 22.04"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Debian"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Debian 12"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Fedora"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Fedora 40"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("Arch Linux"));
    CHECK(CiLinuxJobConfiguration::isSupportedLinuxDistro("arch"));

    // No soportadas
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro("Solaris"));
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro("Windows"));
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro("macOS"));
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro("FreeBSD"));
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro("OpenBSD"));
    CHECK(!CiLinuxJobConfiguration::isSupportedLinuxDistro(""));
}

// TEST-009: test_ci_linux_step_options_and_results
void test_ci_linux_step_options_and_results() {
    CiLinuxStepOptions opts;
    opts.timeoutMs = 45000;
    opts.continueOnError = true;
    opts.requiredEnvVars = {"CI", "VIGILIO_CI_ENV"};
    opts.workingDirectory = "src";
    CHECK(opts.timeoutMs == 45000);
    CHECK(opts.continueOnError);
    CHECK(opts.requiredEnvVars.size() == 2);
    CHECK(opts.workingDirectory == "src");

    CiLinuxStepResult res1;
    res1.step = CiLinuxPipelineStep::ConfigurePreset;
    res1.status = CiLinuxStepStatus::Success;
    res1.exitCode = 0;
    res1.durationMs = 1200;
    res1.outputSummary = "Configuración correcta";
    CHECK(res1.isSuccess());

    CiLinuxStepResult res2;
    res2.step = CiLinuxPipelineStep::BuildCompilation;
    res2.status = CiLinuxStepStatus::Failure;
    res2.exitCode = 1;
    res2.durationMs = 5000;
    res2.errorMessage = "Error de sintaxis";
    CHECK(!res2.isSuccess());

    CiLinuxJobReport report;
    report.status = CiLinuxJobStatus::Failed;
    report.planName = "ci-linux-debug";
    report.durationMs = 6200;
    report.stepResults = {res1, res2};
    report.isClean = false;
    report.remediationHint = "Corrija los errores de compilación";

    CHECK(report.successfulStepsCount() == 1);
    CHECK(report.failedStepsCount() == 1);
    CHECK(!report.isPassed());

    report.status = CiLinuxJobStatus::Passed;
    report.stepResults[1].status = CiLinuxStepStatus::Success;
    report.isClean = true;
    CHECK(report.successfulStepsCount() == 2);
    CHECK(report.failedStepsCount() == 0);
    CHECK(report.isPassed());

    // CancellationToken
    CancellationToken token;
    CHECK(!token.isCancelled());
    token.requestCancellation();
    CHECK(token.isCancelled());
}

// TEST-010: test_ci_linux_ux_copy_public_compliance
void test_ci_linux_ux_copy_public_compliance() {
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
    const std::vector<CiLinuxJobStatus> statuses = {
        CiLinuxJobStatus::Pending, CiLinuxJobStatus::Running,   CiLinuxJobStatus::Passed,
        CiLinuxJobStatus::Failed,  CiLinuxJobStatus::Cancelled, CiLinuxJobStatus::TimedOut};
    for (auto s : statuses) {
        checkNoProhibitedTerms(jobStatusToString(s));
    }

    // Errores de trabajo
    const std::vector<CiLinuxJobError> errors = {CiLinuxJobError::None,
                                                 CiLinuxJobError::ToolchainMissing,
                                                 CiLinuxJobError::ConfigurationFailed,
                                                 CiLinuxJobError::CompilationFailed,
                                                 CiLinuxJobError::TestExecutionFailed,
                                                 CiLinuxJobError::FormatDiscrepancy,
                                                 CiLinuxJobError::StaticAnalysisIssue,
                                                 CiLinuxJobError::UnsafePipelinePath,
                                                 CiLinuxJobError::TimeoutExceeded,
                                                 CiLinuxJobError::InvalidWorkflowSchema,
                                                 CiLinuxJobError::DependencyOrderingViolation,
                                                 CiLinuxJobError::MemoryLimitExceeded};
    for (auto e : errors) {
        checkNoProhibitedTerms(jobErrorToString(e));
        checkNoProhibitedTerms(generateRemediationHint(e));
        checkNoProhibitedTerms(generateRemediationHint(e, "Compilación del proyecto"));
    }

    // Etapas del pipeline
    const std::vector<CiLinuxPipelineStep> steps = {
        CiLinuxPipelineStep::EnvironmentCheck,      CiLinuxPipelineStep::FormatVerification,
        CiLinuxPipelineStep::StaticAnalysis,        CiLinuxPipelineStep::ConfigurePreset,
        CiLinuxPipelineStep::BuildCompilation,      CiLinuxPipelineStep::TestSuiteExecution,
        CiLinuxPipelineStep::SanitizerVerification, CiLinuxPipelineStep::ArtifactPackaging};
    for (auto st : steps) {
        checkNoProhibitedTerms(stepToString(st));
    }

    // Estados de etapa
    const std::vector<CiLinuxStepStatus> stepStatuses = {
        CiLinuxStepStatus::NotStarted, CiLinuxStepStatus::InProgress, CiLinuxStepStatus::Success,
        CiLinuxStepStatus::Failure, CiLinuxStepStatus::Omitted};
    for (auto ss : stepStatuses) {
        checkNoProhibitedTerms(stepStatusToString(ss));
    }
}

// TEST-011: test_ci_linux_remediation_hints
void test_ci_linux_remediation_hints() {
    const std::vector<CiLinuxJobError> errors = {CiLinuxJobError::None,
                                                 CiLinuxJobError::ToolchainMissing,
                                                 CiLinuxJobError::ConfigurationFailed,
                                                 CiLinuxJobError::CompilationFailed,
                                                 CiLinuxJobError::TestExecutionFailed,
                                                 CiLinuxJobError::FormatDiscrepancy,
                                                 CiLinuxJobError::StaticAnalysisIssue,
                                                 CiLinuxJobError::UnsafePipelinePath,
                                                 CiLinuxJobError::TimeoutExceeded,
                                                 CiLinuxJobError::InvalidWorkflowSchema,
                                                 CiLinuxJobError::DependencyOrderingViolation,
                                                 CiLinuxJobError::MemoryLimitExceeded};

    for (auto err : errors) {
        const auto hint = generateRemediationHint(err);
        CHECK(!hint.empty());
        const auto hintWithStep = generateRemediationHint(err, "Verificación");
        CHECK(!hintWithStep.empty());
        if (err != CiLinuxJobError::None) {
            CHECK(hintWithStep.find("En la etapa 'Verificación':") != std::string::npos);
        }
    }
}

// TEST-012: test_ci_linux_plan_validation_boundaries (TASK-001)
void test_ci_linux_plan_validation_boundaries() {
    auto plan = CiLinuxJobConfiguration::canonicalDebugPlan();

    // Límites de tiempo total (totalTimeoutSeconds)
    plan.totalTimeoutSeconds = 0;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan.totalTimeoutSeconds = 3601;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    plan.totalTimeoutSeconds = 7200;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);

    // Límite superior válido: 3600 segundos
    plan.totalTimeoutSeconds = 3600;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::None);

    // Límite inferior válido: 1 segundo
    plan.totalTimeoutSeconds = 1;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::None);

    plan.totalTimeoutSeconds = 600;

    // Límites de cuota de memoria RAM (maxMemoryBytes)
    plan.maxMemoryBytes = 0;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    plan.maxMemoryBytes = 8589934593ULL;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    plan.maxMemoryBytes = 16ULL * 1024 * 1024 * 1024;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    plan.maxMemoryBytes = 64ULL * 1024 * 1024 * 1024;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::MemoryLimitExceeded);

    // Límite superior permitido: exactamente 8 GB
    plan.maxMemoryBytes = 8589934592ULL;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::None);

    plan.maxMemoryBytes = 4294967296ULL;

    // Preset name vacío
    plan.presetName = "";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);
    plan.presetName = "ci-linux-debug";

    // Pasos con nombres vacíos o comandos vacíos
    auto stepCopy = plan.steps;
    plan.steps[0].name = "";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);
    plan.steps = stepCopy;

    plan.steps[1].command = "";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);
    plan.steps = stepCopy;

    // Paso con timeoutMs == 0
    plan.steps[2].options.timeoutMs = 0;
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::InvalidWorkflowSchema);
    plan.steps = stepCopy;

    // Ruta de compilación insegura
    plan.buildDir = "../escape";
    CHECK(CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::UnsafePipelinePath);
}

// TEST-013: test_ci_linux_step_sequence_topological_violations (TASK-002)
void test_ci_linux_step_sequence_topological_violations() {
    // Secuencia vacía -> InvalidWorkflowSchema
    CHECK(CiLinuxJobValidator::validateStepSequence({}) == CiLinuxJobError::InvalidWorkflowSchema);

    // TestSuiteExecution antes de BuildCompilation
    std::vector<CiLinuxPipelineStep> testBeforeBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                        CiLinuxPipelineStep::TestSuiteExecution,
                                                        CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(testBeforeBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // ArtifactPackaging antes de TestSuiteExecution (cuando hay tests)
    std::vector<CiLinuxPipelineStep> packageBeforeTest = {
        CiLinuxPipelineStep::ConfigurePreset, CiLinuxPipelineStep::BuildCompilation,
        CiLinuxPipelineStep::ArtifactPackaging, CiLinuxPipelineStep::TestSuiteExecution};
    CHECK(CiLinuxJobValidator::validateStepSequence(packageBeforeTest) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // ArtifactPackaging sin BuildCompilation
    std::vector<CiLinuxPipelineStep> packageNoBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                       CiLinuxPipelineStep::ArtifactPackaging};
    CHECK(CiLinuxJobValidator::validateStepSequence(packageNoBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // SanitizerVerification antes de BuildCompilation
    std::vector<CiLinuxPipelineStep> sanitizerBeforeBuild = {CiLinuxPipelineStep::SanitizerVerification,
                                                             CiLinuxPipelineStep::ConfigurePreset,
                                                             CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(sanitizerBeforeBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // BuildCompilation sin ConfigurePreset
    std::vector<CiLinuxPipelineStep> buildNoConfig = {CiLinuxPipelineStep::BuildCompilation,
                                                      CiLinuxPipelineStep::TestSuiteExecution};
    CHECK(CiLinuxJobValidator::validateStepSequence(buildNoConfig) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // EnvironmentCheck después de BuildCompilation
    std::vector<CiLinuxPipelineStep> envAfterBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                      CiLinuxPipelineStep::BuildCompilation,
                                                      CiLinuxPipelineStep::EnvironmentCheck};
    CHECK(CiLinuxJobValidator::validateStepSequence(envAfterBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // FormatVerification después de BuildCompilation
    std::vector<CiLinuxPipelineStep> formatAfterBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                         CiLinuxPipelineStep::BuildCompilation,
                                                         CiLinuxPipelineStep::FormatVerification};
    CHECK(CiLinuxJobValidator::validateStepSequence(formatAfterBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // StaticAnalysis después de BuildCompilation
    std::vector<CiLinuxPipelineStep> staticAfterBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                         CiLinuxPipelineStep::BuildCompilation,
                                                         CiLinuxPipelineStep::StaticAnalysis};
    CHECK(CiLinuxJobValidator::validateStepSequence(staticAfterBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    // Etapas duplicadas
    std::vector<CiLinuxPipelineStep> dupEnv = {
        CiLinuxPipelineStep::EnvironmentCheck, CiLinuxPipelineStep::EnvironmentCheck,
        CiLinuxPipelineStep::ConfigurePreset, CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(dupEnv) ==
          CiLinuxJobError::DependencyOrderingViolation);

    std::vector<CiLinuxPipelineStep> dupBuild = {CiLinuxPipelineStep::ConfigurePreset,
                                                 CiLinuxPipelineStep::BuildCompilation,
                                                 CiLinuxPipelineStep::BuildCompilation};
    CHECK(CiLinuxJobValidator::validateStepSequence(dupBuild) ==
          CiLinuxJobError::DependencyOrderingViolation);

    std::vector<CiLinuxPipelineStep> dupTest = {
        CiLinuxPipelineStep::ConfigurePreset, CiLinuxPipelineStep::BuildCompilation,
        CiLinuxPipelineStep::TestSuiteExecution, CiLinuxPipelineStep::TestSuiteExecution};
    CHECK(CiLinuxJobValidator::validateStepSequence(dupTest) ==
          CiLinuxJobError::DependencyOrderingViolation);

    std::vector<CiLinuxPipelineStep> dupPackage = {
        CiLinuxPipelineStep::ConfigurePreset, CiLinuxPipelineStep::BuildCompilation,
        CiLinuxPipelineStep::TestSuiteExecution, CiLinuxPipelineStep::ArtifactPackaging,
        CiLinuxPipelineStep::ArtifactPackaging};
    CHECK(CiLinuxJobValidator::validateStepSequence(dupPackage) ==
          CiLinuxJobError::DependencyOrderingViolation);
}

// TEST-014: test_ci_linux_advanced_path_traversal (TASK-003)
void test_ci_linux_advanced_path_traversal() {
    const std::vector<std::string> maliciousVectors = {"./../..",
                                                       "build/../../etc",
                                                       "..\\windows\\path",
                                                       "build/..\\escape",
                                                       "..",
                                                       "../",
                                                       "../../",
                                                       "/etc/shadow",
                                                       "/var/run",
                                                       "/proc/sys",
                                                       "/",
                                                       "/tmp",
                                                       "/var/tmp",
                                                       "",
                                                       "C:/build",
                                                       "D:\\build",
                                                       "\\\\server\\share",
                                                       "${sourceDir}/../escape",
                                                       "build-ci-debug/../../root",
                                                       "build-ci-debug/subdir/../../../escape",
                                                       "etc/passwd",
                                                       "var/log",
                                                       "usr/local",
                                                       "home/user/build",
                                                       "src/build",
                                                       "custom/build"};

    for (const auto& vec : maliciousVectors) {
        CHECK(!CiLinuxJobValidator::isSafeBuildPath(vec));
    }

    // Ruta con carácter nulo embebido
    const std::string nullVector("build-ci-debug\0malicious", 23);
    CHECK(!CiLinuxJobValidator::isSafeBuildPath(nullVector));

    // Rutas legítimas que deben ser aceptadas
    const std::vector<std::string> legitimatePaths = {"build-ci-debug",
                                                      "build-ci-release",
                                                      "_build",
                                                      "_build/tests",
                                                      "_build/sub/deep",
                                                      "build-test",
                                                      "build-custom-preset",
                                                      "${sourceDir}/build-ci-debug",
                                                      "${sourceDir}/_build",
                                                      "${sourceDir}/build-test/artifacts"};

    for (const auto& path : legitimatePaths) {
        CHECK(CiLinuxJobValidator::isSafeBuildPath(path));
    }
}

// TEST-015: test_ci_linux_environment_boundaries (TASK-004)
void test_ci_linux_environment_boundaries() {
    CiLinuxEnvironment env;
    env.compiler = CiLinuxCompiler::GCC;
    env.compilerMajorVersion = 14;
    env.compilerMinorVersion = 0;
    env.cmakeMajorVersion = 3;
    env.cmakeMinorVersion = 28;
    env.architecture = CiLinuxArchitecture::X86_64;
    env.totalMemoryBytes = 4294967296ULL; // 4 GB
    env.availableCores = 4;

    // Entorno nominal válido
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    // Límites de GCC: 14 es el mínimo permitido para C++20
    env.compiler = CiLinuxCompiler::GCC;
    env.compilerMajorVersion = 14;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.compilerMajorVersion = 15;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.compilerMajorVersion = 13;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.compilerMajorVersion = 11;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    // Límites de Clang: 18 es el mínimo permitido para C++20
    env.compiler = CiLinuxCompiler::Clang;
    env.compilerMajorVersion = 18;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.compilerMajorVersion = 19;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.compilerMajorVersion = 17;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.compilerMajorVersion = 16;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    // Restaurar compilador válido
    env.compiler = CiLinuxCompiler::GCC;
    env.compilerMajorVersion = 14;

    // Límites de CMake: 3.24 es el mínimo
    env.cmakeMajorVersion = 3;
    env.cmakeMinorVersion = 24;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.cmakeMinorVersion = 23;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.cmakeMajorVersion = 2;
    env.cmakeMinorVersion = 8;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.cmakeMajorVersion = 3;
    env.cmakeMinorVersion = 28;

    // Límites de memoria RAM: mínimo 2 GB (2147483648ULL)
    env.totalMemoryBytes = 2147483648ULL;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.totalMemoryBytes = 2147483647ULL; // 2 GB - 1 byte
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.totalMemoryBytes = 1073741824ULL; // 1 GB
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.totalMemoryBytes = 4294967296ULL;

    // Límites de núcleos: mínimo 1 núcleo
    env.availableCores = 1;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.availableCores = 0;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);

    env.availableCores = 4;

    // Arquitecturas soportadas (x86_64, aarch64) y no reconocidas
    env.architecture = CiLinuxArchitecture::X86_64;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.architecture = CiLinuxArchitecture::AArch64;
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::None);

    env.architecture = static_cast<CiLinuxArchitecture>(99);
    CHECK(CiLinuxJobValidator::validateEnvironment(env) == CiLinuxJobError::ToolchainMissing);
}

} // namespace

int main() {
    try {
        std::cout << "Ejecutando pruebas de dominio para CI Linux job...\n";
        test_ci_linux_canonical_debug_plan();
        std::cout << "  [PASS] test_ci_linux_canonical_debug_plan\n";
        test_ci_linux_canonical_release_plan();
        std::cout << "  [PASS] test_ci_linux_canonical_release_plan\n";
        test_ci_linux_plan_validation_valid();
        std::cout << "  [PASS] test_ci_linux_plan_validation_valid\n";
        test_ci_linux_plan_validation_empty_steps();
        std::cout << "  [PASS] test_ci_linux_plan_validation_empty_steps\n";
        test_ci_linux_step_sequence_ordering();
        std::cout << "  [PASS] test_ci_linux_step_sequence_ordering\n";
        test_ci_linux_environment_validation();
        std::cout << "  [PASS] test_ci_linux_environment_validation\n";
        test_ci_linux_safe_build_path_traversal();
        std::cout << "  [PASS] test_ci_linux_safe_build_path_traversal\n";
        test_ci_linux_supported_distros();
        std::cout << "  [PASS] test_ci_linux_supported_distros\n";
        test_ci_linux_step_options_and_results();
        std::cout << "  [PASS] test_ci_linux_step_options_and_results\n";
        test_ci_linux_ux_copy_public_compliance();
        std::cout << "  [PASS] test_ci_linux_ux_copy_public_compliance\n";
        test_ci_linux_remediation_hints();
        std::cout << "  [PASS] test_ci_linux_remediation_hints\n";
        test_ci_linux_plan_validation_boundaries();
        std::cout << "  [PASS] test_ci_linux_plan_validation_boundaries\n";
        test_ci_linux_step_sequence_topological_violations();
        std::cout << "  [PASS] test_ci_linux_step_sequence_topological_violations\n";
        test_ci_linux_advanced_path_traversal();
        std::cout << "  [PASS] test_ci_linux_advanced_path_traversal\n";
        test_ci_linux_environment_boundaries();
        std::cout << "  [PASS] test_ci_linux_environment_boundaries\n";

        std::cout << "Todas las pruebas de dominio de CI Linux job pasaron exitosamente.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fallo en prueba: " << e.what() << '\n';
        return 1;
    }
}
