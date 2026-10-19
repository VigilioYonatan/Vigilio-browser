#include "core/CiWindowsJobConfiguration.h"
#include "core/CiWindowsJobValidator.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace vigilio {

CiWindowsJobPlan CiWindowsJobConfiguration::canonicalDebugPlan() {
    CiWindowsJobPlan plan;
    plan.presetName = "ci-windows-debug";
    plan.buildDir = "build-ci-win-debug";
    plan.totalTimeoutSeconds = 600;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {CiWindowsStepDefinition{.step = CiWindowsPipelineStep::EnvironmentCheck,
                                          .name = "Verificación del entorno de compilación en Windows",
                                          .command = "cmake --version && ninja --version && cl",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 15000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}},
                  CiWindowsStepDefinition{.step = CiWindowsPipelineStep::FormatVerification,
                                          .name = "Comprobación de formato de código",
                                          .command = "clang-format --dry-run --Werror",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 30000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}},
                  CiWindowsStepDefinition{.step = CiWindowsPipelineStep::StaticAnalysis,
                                          .name = "Análisis estático de calidad de código",
                                          .command = "clang-tidy -p build-ci-win-debug",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 120000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}},
                  CiWindowsStepDefinition{.step = CiWindowsPipelineStep::ConfigurePreset,
                                          .name = "Configuración de compilación del sistema",
                                          .command = "cmake --preset ci-windows-debug",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 60000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}},
                  CiWindowsStepDefinition{.step = CiWindowsPipelineStep::BuildCompilation,
                                          .name = "Compilación del proyecto",
                                          .command = "cmake --build --preset ci-windows-debug",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 300000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}},
                  CiWindowsStepDefinition{.step = CiWindowsPipelineStep::TestSuiteExecution,
                                          .name = "Ejecución de pruebas automatizadas",
                                          .command = "ctest --preset ci-windows-debug",
                                          .isMandatory = true,
                                          .options = CiWindowsStepOptions{.timeoutMs = 120000,
                                                                          .continueOnError = false,
                                                                          .requiredEnvVars = {},
                                                                          .workingDirectory = "."}}};

    return plan;
}

CiWindowsJobPlan CiWindowsJobConfiguration::canonicalReleasePlan() {
    CiWindowsJobPlan plan;
    plan.presetName = "ci-windows-release";
    plan.buildDir = "build-ci-win-release";
    plan.totalTimeoutSeconds = 900;
    plan.maxMemoryBytes = 4294967296ULL;
    plan.failFast = true;

    plan.steps = {
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::EnvironmentCheck,
                                .name = "Verificación del entorno de compilación en Windows",
                                .command = "cmake --version && ninja --version && cl",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 15000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::FormatVerification,
                                .name = "Comprobación de formato de código",
                                .command = "clang-format --dry-run --Werror",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 30000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::StaticAnalysis,
                                .name = "Análisis estático de calidad de código",
                                .command = "clang-tidy -p build-ci-win-release",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 120000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::ConfigurePreset,
                                .name = "Configuración de compilación del sistema",
                                .command = "cmake --preset ci-windows-release",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 60000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::BuildCompilation,
                                .name = "Compilación del proyecto",
                                .command = "cmake --build --preset ci-windows-release",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 300000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::TestSuiteExecution,
                                .name = "Ejecución de pruebas automatizadas",
                                .command = "ctest --preset ci-windows-release",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 120000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}},
        CiWindowsStepDefinition{.step = CiWindowsPipelineStep::ArtifactPackaging,
                                .name = "Validación de empaquetado del instalador",
                                .command = "cmake --build --preset ci-windows-release --target package",
                                .isMandatory = true,
                                .options = CiWindowsStepOptions{.timeoutMs = 180000,
                                                                .continueOnError = false,
                                                                .requiredEnvVars = {},
                                                                .workingDirectory = "."}}};

    return plan;
}

bool CiWindowsJobConfiguration::verifyInvariants(const CiWindowsJobPlan& plan) {
    return CiWindowsJobValidator::validatePlan(plan) == CiWindowsJobError::None;
}

bool CiWindowsJobConfiguration::isSupportedWindowsVersion(std::string_view version) {
    if (version.empty()) {
        return false;
    }

    std::string lower;
    lower.reserve(version.size());
    for (char c : version) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // Exclusiones explícitas de sistemas no Windows o versiones obsoletas
    if (lower.find("linux") != std::string::npos || lower.find("solaris") != std::string::npos ||
        lower.find("macos") != std::string::npos || lower.find("darwin") != std::string::npos ||
        lower.find("bsd") != std::string::npos || lower.find("android") != std::string::npos ||
        lower.find("unix") != std::string::npos) {
        return false;
    }

    if (lower.find("windows 7") != std::string::npos || lower.find("win7") != std::string::npos ||
        lower.find("windows xp") != std::string::npos || lower.find("winxp") != std::string::npos ||
        lower.find("windows 8") != std::string::npos || lower.find("win8") != std::string::npos ||
        lower.find("windows vista") != std::string::npos || lower.find("vista") != std::string::npos ||
        lower.find("windows 2000") != std::string::npos || lower.find("win2000") != std::string::npos) {
        return false;
    }

    // Versiones de Windows soportadas oficialmente
    if (lower.find("windows 11") != std::string::npos || lower.find("win 11") != std::string::npos ||
        lower.find("win11") != std::string::npos || lower.find("windows 10") != std::string::npos ||
        lower.find("win 10") != std::string::npos || lower.find("win10") != std::string::npos ||
        lower.find("windows server 2022") != std::string::npos ||
        lower.find("server 2022") != std::string::npos) {
        return true;
    }

    // Builds soportados de Windows 10/11
    if (lower.find("22h2") != std::string::npos || lower.find("23h2") != std::string::npos ||
        lower.find("19045") != std::string::npos || lower.find("22621") != std::string::npos ||
        lower.find("22631") != std::string::npos) {
        if (lower.find("windows") != std::string::npos || lower.find("win") != std::string::npos) {
            return true;
        }
    }

    return false;
}

} // namespace vigilio
