#include "core/CiMacOsJobConfiguration.h"
#include "core/CiMacOsJobValidator.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace vigilio {

CiMacOsJobPlan CiMacOsJobConfiguration::canonicalDebugPlan() {
    CiMacOsJobPlan plan;
    plan.presetName = "ci-macos-debug";
    plan.buildDir = "build-ci-macos-debug";
    plan.totalTimeoutSeconds = 600;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::EnvironmentCheck,
                              .name = "Verificación del entorno de compilación en macOS",
                              .command = "cmake --version && ninja --version && clang --version",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 15000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::FormatVerification,
                              .name = "Comprobación de formato de código",
                              .command = "clang-format --dry-run --Werror",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 30000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::StaticAnalysis,
                              .name = "Análisis estático de calidad de código",
                              .command = "clang-tidy -p build-ci-macos-debug",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::ConfigurePreset,
                              .name = "Configuración de compilación del sistema",
                              .command = "cmake --preset ci-macos-debug",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 60000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::BuildCompilation,
                              .name = "Compilación del proyecto",
                              .command = "cmake --build --preset ci-macos-debug",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 300000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::TestSuiteExecution,
                              .name = "Ejecución de pruebas automatizadas",
                              .command = "ctest --preset ci-macos-debug",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}}};

    return plan;
}

CiMacOsJobPlan CiMacOsJobConfiguration::canonicalReleasePlan() {
    CiMacOsJobPlan plan;
    plan.presetName = "ci-macos-release";
    plan.buildDir = "build-ci-macos-release";
    plan.totalTimeoutSeconds = 900;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::EnvironmentCheck,
                              .name = "Verificación del entorno de compilación en macOS",
                              .command = "cmake --version && ninja --version && clang --version",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 15000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::FormatVerification,
                              .name = "Comprobación de formato de código",
                              .command = "clang-format --dry-run --Werror",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 30000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::StaticAnalysis,
                              .name = "Análisis estático de calidad de código",
                              .command = "clang-tidy -p build-ci-macos-release",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::ConfigurePreset,
                              .name = "Configuración de compilación del sistema",
                              .command = "cmake --preset ci-macos-release",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 60000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::BuildCompilation,
                              .name = "Compilación del proyecto",
                              .command = "cmake --build --preset ci-macos-release",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 300000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{.step = CiMacOsPipelineStep::TestSuiteExecution,
                              .name = "Ejecución de pruebas automatizadas",
                              .command = "ctest --preset ci-macos-release",
                              .isMandatory = true,
                              .options = CiMacOsStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiMacOsStepDefinition{
            .step = CiMacOsPipelineStep::BundleVerification,
            .name = "Verificación del paquete de la aplicación",
            .command = "test -d build-ci-macos-release/VigilioBrowser.app/Contents/MacOS && test -f "
                       "build-ci-macos-release/VigilioBrowser.app/Contents/Info.plist",
            .isMandatory = true,
            .options = CiMacOsStepOptions{.timeoutMs = 60000,
                                          .continueOnError = false,
                                          .requiredEnvVars = {},
                                          .workingDirectory = "."}}};

    return plan;
}

bool CiMacOsJobConfiguration::verifyInvariants(const CiMacOsJobPlan& plan) {
    return CiMacOsJobValidator::validatePlan(plan) == CiMacOsJobError::None;
}

bool CiMacOsJobConfiguration::isSupportedMacOsVersion(std::string_view version) {
    if (version.empty()) {
        return false;
    }

    std::string lower;
    lower.reserve(version.size());
    for (char c : version) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // Exclusiones explícitas de sistemas no macOS
    if (lower.find("linux") != std::string::npos || lower.find("windows") != std::string::npos ||
        lower.find("win32") != std::string::npos || lower.find("win64") != std::string::npos ||
        lower.find("win 10") != std::string::npos || lower.find("win 11") != std::string::npos ||
        lower.find("win10") != std::string::npos || lower.find("win11") != std::string::npos ||
        lower.find("win7") != std::string::npos || lower.find("winxp") != std::string::npos ||
        lower.find("solaris") != std::string::npos || lower.find("bsd") != std::string::npos ||
        lower.find("android") != std::string::npos || lower.find("unix") != std::string::npos) {
        return false;
    }

    // Exclusiones explícitas de versiones obsoletas de macOS / OS X / Mac OS X
    if (lower.find("macos 10") != std::string::npos || lower.find("mac os x") != std::string::npos ||
        lower.find("os x") != std::string::npos || lower.find("osx") != std::string::npos ||
        lower.find("10.15") != std::string::npos || lower.find("10.14") != std::string::npos ||
        lower.find("10.13") != std::string::npos || lower.find("catalina") != std::string::npos ||
        lower.find("mojave") != std::string::npos || lower.find("high sierra") != std::string::npos ||
        lower.find("sierra") != std::string::npos || lower.find("el capitan") != std::string::npos ||
        lower.find("yosemite") != std::string::npos || lower.find("macos 11") != std::string::npos ||
        lower.find("big sur") != std::string::npos || lower.find("darwin 20") != std::string::npos ||
        lower.find("macos 12") != std::string::npos || lower.find("monterey") != std::string::npos ||
        lower.find("darwin 21") != std::string::npos) {
        return false;
    }

    // Versiones modernas de macOS soportadas oficialmente (macOS 13+, Darwin 22+)
    if (lower.find("macos 13") != std::string::npos || lower.find("ventura") != std::string::npos ||
        lower.find("darwin 22") != std::string::npos || lower.find("macos 14") != std::string::npos ||
        lower.find("sonoma") != std::string::npos || lower.find("darwin 23") != std::string::npos ||
        lower.find("macos 15") != std::string::npos || lower.find("sequoia") != std::string::npos ||
        lower.find("darwin 24") != std::string::npos) {
        return true;
    }

    // Comprobación de números de versión mayores en macOS / Darwin
    if (lower.find("macos") != std::string::npos || lower.find("mac os") != std::string::npos) {
        if (lower.find("13") != std::string::npos || lower.find("14") != std::string::npos ||
            lower.find("15") != std::string::npos || lower.find("16") != std::string::npos) {
            return true;
        }
    }

    if (lower.find("darwin") != std::string::npos) {
        if (lower.find("22") != std::string::npos || lower.find("23") != std::string::npos ||
            lower.find("24") != std::string::npos || lower.find("25") != std::string::npos) {
            return true;
        }
    }

    return false;
}

} // namespace vigilio
