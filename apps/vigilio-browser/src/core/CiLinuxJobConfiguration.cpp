#include "core/CiLinuxJobConfiguration.h"
#include "core/CiLinuxJobValidator.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace vigilio {

CiLinuxJobPlan CiLinuxJobConfiguration::canonicalDebugPlan() {
    CiLinuxJobPlan plan;
    plan.presetName = "ci-linux-debug";
    plan.buildDir = "build-ci-debug";
    plan.totalTimeoutSeconds = 600;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::EnvironmentCheck,
                              .name = "Verificación del entorno de compilación",
                              .command = "cmake --version && ninja --version && $CXX --version",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 15000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::FormatVerification,
                              .name = "Comprobación de formato de código",
                              .command = "clang-format --dry-run --Werror",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 30000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::StaticAnalysis,
                              .name = "Análisis estático de calidad de código",
                              .command = "clang-tidy -p build-ci-debug",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::ConfigurePreset,
                              .name = "Configuración de compilación del sistema",
                              .command = "cmake --preset ci-linux-debug",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 60000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::BuildCompilation,
                              .name = "Compilación del proyecto",
                              .command = "cmake --build --preset ci-linux-debug",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 300000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::TestSuiteExecution,
                              .name = "Ejecución de pruebas automatizadas",
                              .command = "ctest --preset ci-linux-debug",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}}};

    return plan;
}

CiLinuxJobPlan CiLinuxJobConfiguration::canonicalReleasePlan() {
    CiLinuxJobPlan plan;
    plan.presetName = "ci-linux-release";
    plan.buildDir = "build-ci-release";
    plan.totalTimeoutSeconds = 900;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::EnvironmentCheck,
                              .name = "Verificación del entorno de compilación",
                              .command = "cmake --version && ninja --version && $CXX --version",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 15000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::FormatVerification,
                              .name = "Comprobación de formato de código",
                              .command = "clang-format --dry-run --Werror",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 30000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::StaticAnalysis,
                              .name = "Análisis estático de calidad de código",
                              .command = "clang-tidy -p build-ci-release",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::ConfigurePreset,
                              .name = "Configuración de compilación del sistema",
                              .command = "cmake --preset ci-linux-release",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 60000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::BuildCompilation,
                              .name = "Compilación del proyecto",
                              .command = "cmake --build --preset ci-linux-release",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 300000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::TestSuiteExecution,
                              .name = "Ejecución de pruebas automatizadas",
                              .command = "ctest --preset ci-linux-release",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 120000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}},
        CiLinuxStepDefinition{.step = CiLinuxPipelineStep::ArtifactPackaging,
                              .name = "Validación de empaquetado del navegador",
                              .command = "cmake --build --preset ci-linux-release --target package",
                              .isMandatory = true,
                              .options = CiLinuxStepOptions{.timeoutMs = 180000,
                                                            .continueOnError = false,
                                                            .requiredEnvVars = {},
                                                            .workingDirectory = "."}}};

    return plan;
}

bool CiLinuxJobConfiguration::verifyInvariants(const CiLinuxJobPlan& plan) {
    return CiLinuxJobValidator::validatePlan(plan) == CiLinuxJobError::None;
}

bool CiLinuxJobConfiguration::isSupportedLinuxDistro(std::string_view distro) {
    if (distro.empty()) {
        return false;
    }

    std::string lower;
    lower.reserve(distro.size());
    for (char c : distro) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // Exclusiones explícitas de sistemas no Linux
    if (lower.find("windows") != std::string::npos || lower.find("solaris") != std::string::npos ||
        lower.find("macos") != std::string::npos || lower.find("darwin") != std::string::npos ||
        lower.find("bsd") != std::string::npos) {
        return false;
    }

    // Distribuciones Linux soportadas oficialmente
    if (lower.find("ubuntu") != std::string::npos || lower.find("debian") != std::string::npos ||
        lower.find("fedora") != std::string::npos || lower.find("arch") != std::string::npos) {
        return true;
    }

    return false;
}

} // namespace vigilio
