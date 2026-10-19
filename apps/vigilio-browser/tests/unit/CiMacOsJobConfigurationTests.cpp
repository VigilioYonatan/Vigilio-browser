#include "core/CiMacOsJobConfiguration.h"
#include "core/CiMacOsJobTypes.h"
#include "core/CiMacOsJobValidator.h"

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

// TEST-001: test_ci_macos_canonical_debug_plan
void test_ci_macos_canonical_debug_plan() {
    const auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    CHECK(plan.presetName == "ci-macos-debug");
    CHECK(plan.buildDir == "build-ci-macos-debug");
    CHECK(plan.totalTimeoutSeconds == 600);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 6);
    CHECK(plan.steps[0].step == CiMacOsPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiMacOsPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiMacOsPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiMacOsPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiMacOsPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiMacOsPipelineStep::TestSuiteExecution);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiMacOsJobConfiguration::verifyInvariants(plan));
}

// TEST-002: test_ci_macos_canonical_release_plan
void test_ci_macos_canonical_release_plan() {
    const auto plan = CiMacOsJobConfiguration::canonicalReleasePlan();
    CHECK(plan.presetName == "ci-macos-release");
    CHECK(plan.buildDir == "build-ci-macos-release");
    CHECK(plan.totalTimeoutSeconds == 900);
    CHECK(plan.maxMemoryBytes == 4294967296ULL);
    CHECK(plan.failFast);
    CHECK(plan.steps.size() == 7);
    CHECK(plan.steps[0].step == CiMacOsPipelineStep::EnvironmentCheck);
    CHECK(plan.steps[1].step == CiMacOsPipelineStep::FormatVerification);
    CHECK(plan.steps[2].step == CiMacOsPipelineStep::StaticAnalysis);
    CHECK(plan.steps[3].step == CiMacOsPipelineStep::ConfigurePreset);
    CHECK(plan.steps[4].step == CiMacOsPipelineStep::BuildCompilation);
    CHECK(plan.steps[5].step == CiMacOsPipelineStep::TestSuiteExecution);
    CHECK(plan.steps[6].step == CiMacOsPipelineStep::BundleVerification);
    for (const auto& step : plan.steps) {
        CHECK(step.isMandatory);
        CHECK(!step.name.empty());
        CHECK(!step.command.empty());
        CHECK(step.options.timeoutMs > 0);
    }
    CHECK(CiMacOsJobConfiguration::verifyInvariants(plan));
}

// TEST-003: test_ci_macos_plan_validation_valid
void test_ci_macos_plan_validation_valid() {
    const auto debugPlan = CiMacOsJobConfiguration::canonicalDebugPlan();
    CHECK(CiMacOsJobValidator::validatePlan(debugPlan) == CiMacOsJobError::None);

    const auto releasePlan = CiMacOsJobConfiguration::canonicalReleasePlan();
    CHECK(CiMacOsJobValidator::validatePlan(releasePlan) == CiMacOsJobError::None);
}

// TEST-004: test_ci_macos_plan_validation_empty_steps
void test_ci_macos_plan_validation_empty_steps() {
    auto plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps.clear();
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.presetName = "";
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 0;
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.totalTimeoutSeconds = 4000;
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 0;
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::MemoryLimitExceeded);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.maxMemoryBytes = 16ULL * 1024 * 1024 * 1024;
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::MemoryLimitExceeded);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.buildDir = "../unsafe";
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::UnsafePipelinePath);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].name = "";
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].command = "";
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);

    plan = CiMacOsJobConfiguration::canonicalDebugPlan();
    plan.steps[0].options.timeoutMs = 0;
    CHECK(CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::InvalidWorkflowSchema);
}

// TEST-005: test_ci_macos_step_sequence_ordering
void test_ci_macos_step_sequence_ordering() {
    // Secuencia canónica válida de 6 etapas
    std::vector<CiMacOsPipelineStep> validSeq = {
        CiMacOsPipelineStep::EnvironmentCheck, CiMacOsPipelineStep::FormatVerification,
        CiMacOsPipelineStep::StaticAnalysis,   CiMacOsPipelineStep::ConfigurePreset,
        CiMacOsPipelineStep::BuildCompilation, CiMacOsPipelineStep::TestSuiteExecution};
    CHECK(CiMacOsJobValidator::validateStepSequence(validSeq) == CiMacOsJobError::None);

    // Secuencia completa con SanitizerVerification y BundleVerification
    std::vector<CiMacOsPipelineStep> fullSeq = {
        CiMacOsPipelineStep::EnvironmentCheck,      CiMacOsPipelineStep::FormatVerification,
        CiMacOsPipelineStep::StaticAnalysis,        CiMacOsPipelineStep::ConfigurePreset,
        CiMacOsPipelineStep::BuildCompilation,      CiMacOsPipelineStep::TestSuiteExecution,
        CiMacOsPipelineStep::SanitizerVerification, CiMacOsPipelineStep::BundleVerification};
    CHECK(CiMacOsJobValidator::validateStepSequence(fullSeq) == CiMacOsJobError::None);

    // Secuencia vacía
    CHECK(CiMacOsJobValidator::validateStepSequence({}) == CiMacOsJobError::InvalidWorkflowSchema);

    // Compilación antes de configuración
    std::vector<CiMacOsPipelineStep> invalidBuildBeforeConfig = {CiMacOsPipelineStep::BuildCompilation,
                                                                 CiMacOsPipelineStep::ConfigurePreset};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidBuildBeforeConfig) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Pruebas antes de compilación
    std::vector<CiMacOsPipelineStep> invalidTestsBeforeBuild = {CiMacOsPipelineStep::ConfigurePreset,
                                                                CiMacOsPipelineStep::TestSuiteExecution,
                                                                CiMacOsPipelineStep::BuildCompilation};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidTestsBeforeBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Formato después de compilación
    std::vector<CiMacOsPipelineStep> invalidFormatAfterBuild = {CiMacOsPipelineStep::ConfigurePreset,
                                                                CiMacOsPipelineStep::BuildCompilation,
                                                                CiMacOsPipelineStep::FormatVerification};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidFormatAfterBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Análisis estático después de compilación
    std::vector<CiMacOsPipelineStep> invalidStaticAfterBuild = {CiMacOsPipelineStep::ConfigurePreset,
                                                                CiMacOsPipelineStep::BuildCompilation,
                                                                CiMacOsPipelineStep::StaticAnalysis};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidStaticAfterBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Verificación de entorno después de compilación
    std::vector<CiMacOsPipelineStep> invalidEnvAfterBuild = {CiMacOsPipelineStep::ConfigurePreset,
                                                             CiMacOsPipelineStep::BuildCompilation,
                                                             CiMacOsPipelineStep::EnvironmentCheck};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidEnvAfterBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Sanitizador antes de compilación
    std::vector<CiMacOsPipelineStep> invalidSanitizerBeforeBuild = {
        CiMacOsPipelineStep::SanitizerVerification, CiMacOsPipelineStep::ConfigurePreset,
        CiMacOsPipelineStep::BuildCompilation};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidSanitizerBeforeBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // BundleVerification antes de pruebas
    std::vector<CiMacOsPipelineStep> invalidBundleBeforeTests = {
        CiMacOsPipelineStep::ConfigurePreset, CiMacOsPipelineStep::BuildCompilation,
        CiMacOsPipelineStep::BundleVerification, CiMacOsPipelineStep::TestSuiteExecution};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidBundleBeforeTests) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // BundleVerification sin compilación
    std::vector<CiMacOsPipelineStep> invalidBundleNoBuild = {CiMacOsPipelineStep::ConfigurePreset,
                                                             CiMacOsPipelineStep::BundleVerification};
    CHECK(CiMacOsJobValidator::validateStepSequence(invalidBundleNoBuild) ==
          CiMacOsJobError::DependencyOrderingViolation);

    // Etapa duplicada
    std::vector<CiMacOsPipelineStep> duplicateSeq = {CiMacOsPipelineStep::ConfigurePreset,
                                                     CiMacOsPipelineStep::ConfigurePreset,
                                                     CiMacOsPipelineStep::BuildCompilation};
    CHECK(CiMacOsJobValidator::validateStepSequence(duplicateSeq) ==
          CiMacOsJobError::DependencyOrderingViolation);
}

// TEST-006: test_ci_macos_environment_validation
void test_ci_macos_environment_validation() {
    // Entorno conforme Apple Clang 15 en ARM64
    CiMacOsEnvironment envApple;
    envApple.compiler = CiMacOsCompiler::AppleClang;
    envApple.compilerMajorVersion = 15;
    envApple.compilerMinorVersion = 0;
    envApple.sdkVersion = "14.2";
    envApple.xcodeToolsInstalled = true;
    envApple.cmakeMajorVersion = 3;
    envApple.cmakeMinorVersion = 28;
    envApple.ninjaMajorVersion = 1;
    envApple.ninjaMinorVersion = 12;
    envApple.architecture = CiMacOsArchitecture::ARM64;
    envApple.osVersion = "macOS 14 Sonoma";
    envApple.totalMemoryBytes = 4294967296ULL;
    envApple.availableCores = 8;
    CHECK(CiMacOsJobValidator::validateEnvironment(envApple) == CiMacOsJobError::None);

    // Entorno conforme Homebrew Clang 18 en x86_64
    CiMacOsEnvironment envHomebrew = envApple;
    envHomebrew.compiler = CiMacOsCompiler::HomebrewClang;
    envHomebrew.compilerMajorVersion = 18;
    envHomebrew.architecture = CiMacOsArchitecture::X86_64;
    CHECK(CiMacOsJobValidator::validateEnvironment(envHomebrew) == CiMacOsJobError::None);

    // Entorno conforme Universal architecture
    CiMacOsEnvironment envUniversal = envApple;
    envUniversal.architecture = CiMacOsArchitecture::Universal;
    CHECK(CiMacOsJobValidator::validateEnvironment(envUniversal) == CiMacOsJobError::None);

    // Apple Clang obsoleto (< 15)
    auto badApple = envApple;
    badApple.compilerMajorVersion = 14;
    CHECK(CiMacOsJobValidator::validateEnvironment(badApple) == CiMacOsJobError::ToolchainMissing);

    // Homebrew Clang obsoleto (< 18)
    auto badHomebrew = envHomebrew;
    badHomebrew.compilerMajorVersion = 17;
    CHECK(CiMacOsJobValidator::validateEnvironment(badHomebrew) == CiMacOsJobError::ToolchainMissing);

    // Herramientas de Xcode ausentes
    auto noXcode = envApple;
    noXcode.xcodeToolsInstalled = false;
    CHECK(CiMacOsJobValidator::validateEnvironment(noXcode) == CiMacOsJobError::XcodeToolsMissing);

    // macOS SDK ausente o vacío
    auto noSdk = envApple;
    noSdk.sdkVersion = "";
    CHECK(CiMacOsJobValidator::validateEnvironment(noSdk) == CiMacOsJobError::MacOsSdkMissing);

    // macOS SDK obsoleto (< 13.0)
    auto oldSdk = envApple;
    oldSdk.sdkVersion = "12.3";
    CHECK(CiMacOsJobValidator::validateEnvironment(oldSdk) == CiMacOsJobError::MacOsSdkMissing);

    // Versión de macOS no soportada
    auto badOs = envApple;
    badOs.osVersion = "macOS 11 Big Sur";
    CHECK(CiMacOsJobValidator::validateEnvironment(badOs) == CiMacOsJobError::ToolchainMissing);

    // CMake obsoleto (< 3.28)
    auto badCmake = envApple;
    badCmake.cmakeMajorVersion = 3;
    badCmake.cmakeMinorVersion = 24;
    CHECK(CiMacOsJobValidator::validateEnvironment(badCmake) == CiMacOsJobError::ToolchainMissing);

    // Ninja obsoleto (< 1.12)
    auto badNinja = envApple;
    badNinja.ninjaMajorVersion = 1;
    badNinja.ninjaMinorVersion = 10;
    CHECK(CiMacOsJobValidator::validateEnvironment(badNinja) == CiMacOsJobError::ToolchainMissing);

    // Memoria insuficiente (< 2 GB)
    auto lowMemory = envApple;
    lowMemory.totalMemoryBytes = 1073741824ULL;
    CHECK(CiMacOsJobValidator::validateEnvironment(lowMemory) == CiMacOsJobError::ToolchainMissing);

    // Cero núcleos
    auto zeroCores = envApple;
    zeroCores.availableCores = 0;
    CHECK(CiMacOsJobValidator::validateEnvironment(zeroCores) == CiMacOsJobError::ToolchainMissing);
}

// TEST-007: test_ci_macos_safe_build_path_traversal
void test_ci_macos_safe_build_path_traversal() {
    // Rutas canónicas autorizadas
    CHECK(CiMacOsJobValidator::isSafeBuildPath("build-ci-macos-debug"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("build-ci-macos-release"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("_build"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("_build/tests"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("build-test"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("${sourceDir}/build-ci-macos-debug"));
    CHECK(CiMacOsJobValidator::isSafeBuildPath("${sourceDir}/build-ci-macos-release"));

    // Vectores de Directory Traversal y rutas del sistema
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("../build"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("build/../../etc"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("build-ci-macos-debug/../escape"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath(""));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/System"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/Library"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/Applications"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/private/tmp"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/usr/bin"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/Volumes"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/Users"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("/etc"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("~"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("~/build"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("etc/passwd"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("home/user/build"));
}

// TEST-008: test_ci_macos_safe_build_path_macos_artifacts
void test_ci_macos_safe_build_path_macos_artifacts() {
    // Archivos de metadatos de macOS (.DS_Store)
    CHECK(!CiMacOsJobValidator::isSafeBuildPath(".DS_Store"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("build-ci-macos-debug/.DS_Store"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("_build/.DS_Store"));

    // Bifurcaciones AppleDouble (._*)
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("._icon"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("build/._icon"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("build-ci-macos-debug/._resource"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("._metadata"));
    CHECK(!CiMacOsJobValidator::isSafeBuildPath("_build/._cache"));
}

// TEST-009: test_ci_macos_supported_macos_versions
void test_ci_macos_supported_macos_versions() {
    // Versiones de macOS soportadas oficialmente (macOS 13+, Darwin 22+)
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 14 Sonoma"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 13 Ventura"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 15 Sequoia"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Darwin 23.4.0"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Darwin 22"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Darwin 24"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Sonoma"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Ventura"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("Sequoia"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macos 14"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macos 13"));
    CHECK(CiMacOsJobConfiguration::isSupportedMacOsVersion("macos 15"));

    // Versiones obsoletas o sistemas ajenos
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 12 Monterey"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 11 Big Sur"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("macOS 10.15 Catalina"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Mac OS X 10.14"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("OS X 10.11"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Darwin 21"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Darwin 20"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Ubuntu 24.04"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Linux"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion("Windows 11"));
    CHECK(!CiMacOsJobConfiguration::isSupportedMacOsVersion(""));
}

// TEST-010: test_ci_macos_step_options_and_results
void test_ci_macos_step_options_and_results() {
    CiMacOsStepOptions opts;
    opts.timeoutMs = 45000;
    opts.continueOnError = true;
    opts.requiredEnvVars = {"CI", "VIGILIO_MACOS_CI"};
    opts.workingDirectory = "src";
    CHECK(opts.timeoutMs == 45000);
    CHECK(opts.continueOnError);
    CHECK(opts.requiredEnvVars.size() == 2);
    CHECK(opts.workingDirectory == "src");

    CiMacOsStepResult res1;
    res1.step = CiMacOsPipelineStep::ConfigurePreset;
    res1.status = CiMacOsStepStatus::Success;
    res1.exitCode = 0;
    res1.durationMs = 1200;
    res1.outputSummary = "Configuración correcta";
    CHECK(res1.isSuccess());

    CiMacOsStepResult res2;
    res2.step = CiMacOsPipelineStep::BuildCompilation;
    res2.status = CiMacOsStepStatus::Failure;
    res2.exitCode = 1;
    res2.durationMs = 5000;
    res2.errorMessage = "Error de sintaxis";
    CHECK(!res2.isSuccess());

    CiMacOsJobReport report;
    report.status = CiMacOsJobStatus::Failed;
    report.planName = "ci-macos-debug";
    report.durationMs = 6200;
    report.stepResults = {res1, res2};
    report.isClean = false;
    report.remediationHint = "Corrija los errores de compilación";

    CHECK(report.successfulStepsCount() == 1);
    CHECK(report.failedStepsCount() == 1);
    CHECK(!report.isPassed());

    report.status = CiMacOsJobStatus::Passed;
    report.stepResults[1].status = CiMacOsStepStatus::Success;
    report.isClean = true;
    CHECK(report.successfulStepsCount() == 2);
    CHECK(report.failedStepsCount() == 0);
    CHECK(report.isPassed());

    // CancellationToken
    CancellationToken token;
    CHECK(!token.isCancelled());
    token.requestCancellation();
    CHECK(token.isCancelled());

    // CiMacOsServiceOptions
    CiMacOsServiceOptions sOpts;
    CHECK(sOpts.mode == CiMacOsExecutionMode::DryRun);
    CHECK(sOpts.failFast);
    CHECK(sOpts.maxRetries == 3);
}

// TEST-011: test_ci_macos_ux_copy_public_compliance
void test_ci_macos_ux_copy_public_compliance() {
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
    const std::vector<CiMacOsJobStatus> statuses = {
        CiMacOsJobStatus::Pending, CiMacOsJobStatus::Running,   CiMacOsJobStatus::Passed,
        CiMacOsJobStatus::Failed,  CiMacOsJobStatus::Cancelled, CiMacOsJobStatus::TimedOut};
    for (auto s : statuses) {
        checkNoProhibitedTerms(jobStatusToString(s));
    }

    // Errores de trabajo y pistas de remediación
    const std::vector<CiMacOsJobError> errors = {CiMacOsJobError::None,
                                                 CiMacOsJobError::ToolchainMissing,
                                                 CiMacOsJobError::MacOsSdkMissing,
                                                 CiMacOsJobError::XcodeToolsMissing,
                                                 CiMacOsJobError::ConfigurationFailed,
                                                 CiMacOsJobError::CompilationFailed,
                                                 CiMacOsJobError::TestExecutionFailed,
                                                 CiMacOsJobError::FormatDiscrepancy,
                                                 CiMacOsJobError::StaticAnalysisIssue,
                                                 CiMacOsJobError::UnsafePipelinePath,
                                                 CiMacOsJobError::TimeoutExceeded,
                                                 CiMacOsJobError::InvalidWorkflowSchema,
                                                 CiMacOsJobError::DependencyOrderingViolation,
                                                 CiMacOsJobError::MemoryLimitExceeded,
                                                 CiMacOsJobError::BundleVerificationFailed};
    for (auto e : errors) {
        checkNoProhibitedTerms(jobErrorToString(e));
        checkNoProhibitedTerms(generateRemediationHint(e));
        checkNoProhibitedTerms(generateRemediationHint(e, "Compilación del proyecto"));
    }

    // Etapas del pipeline
    const std::vector<CiMacOsPipelineStep> steps = {
        CiMacOsPipelineStep::EnvironmentCheck,      CiMacOsPipelineStep::FormatVerification,
        CiMacOsPipelineStep::StaticAnalysis,        CiMacOsPipelineStep::ConfigurePreset,
        CiMacOsPipelineStep::BuildCompilation,      CiMacOsPipelineStep::TestSuiteExecution,
        CiMacOsPipelineStep::SanitizerVerification, CiMacOsPipelineStep::BundleVerification};
    for (auto st : steps) {
        checkNoProhibitedTerms(stepToString(st));
    }

    // Estados de etapa
    const std::vector<CiMacOsStepStatus> stepStatuses = {
        CiMacOsStepStatus::NotStarted, CiMacOsStepStatus::InProgress, CiMacOsStepStatus::Success,
        CiMacOsStepStatus::Failure, CiMacOsStepStatus::Omitted};
    for (auto ss : stepStatuses) {
        checkNoProhibitedTerms(stepStatusToString(ss));
    }

    // Arquitecturas y compiladores
    checkNoProhibitedTerms(architectureToString(CiMacOsArchitecture::ARM64));
    checkNoProhibitedTerms(architectureToString(CiMacOsArchitecture::X86_64));
    checkNoProhibitedTerms(architectureToString(CiMacOsArchitecture::Universal));
    checkNoProhibitedTerms(compilerToString(CiMacOsCompiler::AppleClang));
    checkNoProhibitedTerms(compilerToString(CiMacOsCompiler::HomebrewClang));
}

} // namespace

int main() {
    try {
        std::cout << "[TEST] Running CI macOS Job Domain Unit Tests...\n";

        test_ci_macos_canonical_debug_plan();
        std::cout << "  ✓ TEST-001: test_ci_macos_canonical_debug_plan passed.\n";

        test_ci_macos_canonical_release_plan();
        std::cout << "  ✓ TEST-002: test_ci_macos_canonical_release_plan passed.\n";

        test_ci_macos_plan_validation_valid();
        std::cout << "  ✓ TEST-003: test_ci_macos_plan_validation_valid passed.\n";

        test_ci_macos_plan_validation_empty_steps();
        std::cout << "  ✓ TEST-004: test_ci_macos_plan_validation_empty_steps passed.\n";

        test_ci_macos_step_sequence_ordering();
        std::cout << "  ✓ TEST-005: test_ci_macos_step_sequence_ordering passed.\n";

        test_ci_macos_environment_validation();
        std::cout << "  ✓ TEST-006: test_ci_macos_environment_validation passed.\n";

        test_ci_macos_safe_build_path_traversal();
        std::cout << "  ✓ TEST-007: test_ci_macos_safe_build_path_traversal passed.\n";

        test_ci_macos_safe_build_path_macos_artifacts();
        std::cout << "  ✓ TEST-008: test_ci_macos_safe_build_path_macos_artifacts passed.\n";

        test_ci_macos_supported_macos_versions();
        std::cout << "  ✓ TEST-009: test_ci_macos_supported_macos_versions passed.\n";

        test_ci_macos_step_options_and_results();
        std::cout << "  ✓ TEST-010: test_ci_macos_step_options_and_results passed.\n";

        test_ci_macos_ux_copy_public_compliance();
        std::cout << "  ✓ TEST-011: test_ci_macos_ux_copy_public_compliance passed.\n";

        std::cout << "[SUCCESS] All 11 CI macOS Job Domain Unit Tests passed cleanly.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FAILURE] Exception during tests: " << ex.what() << '\n';
        return 1;
    }
}
