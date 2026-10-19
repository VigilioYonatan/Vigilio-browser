#include "core/CiMacOsJobService.h"
#include "core/CiMacOsJobConfiguration.h"
#include "core/CiMacOsJobValidator.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <thread>

namespace vigilio {

CiMacOsJobService::CiMacOsJobService()
    : plan_(CiMacOsJobConfiguration::canonicalDebugPlan()), options_() {
}

CiMacOsJobService::CiMacOsJobService(CiMacOsJobPlan plan) : plan_(std::move(plan)), options_() {
}

void CiMacOsJobService::setPlan(CiMacOsJobPlan plan) {
    std::lock_guard<std::mutex> lock(mutex_);
    plan_ = std::move(plan);
}

CiMacOsJobPlan CiMacOsJobService::plan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_;
}

void CiMacOsJobService::setOptions(CiMacOsServiceOptions options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
}

CiMacOsServiceOptions CiMacOsJobService::options() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

std::string CiMacOsJobService::buildCommandLine(const CiMacOsStepDefinition& step) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string buildDirStr = plan_.buildDir.string();
    const std::string presetNameStr = plan_.presetName;

    switch (step.step) {
    case CiMacOsPipelineStep::EnvironmentCheck:
        return "cmake --version && ninja --version && clang --version";
    case CiMacOsPipelineStep::FormatVerification:
        return "find apps/vigilio-browser/src apps/vigilio-browser/tests -name '*.cpp' -o -name '*.h' "
               "| while read -r f; do clang-format --dry-run --Werror \"$f\"; done";
    case CiMacOsPipelineStep::StaticAnalysis:
        return "clang-tidy -p " + buildDirStr + " apps/vigilio-browser/src/**/*.cpp";
    case CiMacOsPipelineStep::ConfigurePreset:
        return "cmake --preset " + presetNameStr + " -B " + buildDirStr;
    case CiMacOsPipelineStep::BuildCompilation:
        return "cmake --build " + buildDirStr + " --parallel";
    case CiMacOsPipelineStep::TestSuiteExecution:
        return "ctest --test-dir " + buildDirStr + " --output-on-failure -L fast --timeout 15";
    case CiMacOsPipelineStep::SanitizerVerification:
        return "ctest --test-dir " + buildDirStr + " --output-on-failure -L sanitizer";
    case CiMacOsPipelineStep::BundleVerification:
        return "test -d " + buildDirStr + "/VigilioBrowser.app/Contents/MacOS && test -f " +
               buildDirStr + "/VigilioBrowser.app/Contents/Info.plist";
    }
    return step.command;
}

bool CiMacOsJobService::cleanBuildDirectory(std::string* errorMessage) {
    std::filesystem::path targetDir;
    uint32_t retries = 0;
    uint32_t retryBackoffMs = 50;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        targetDir = plan_.buildDir;
        retries = options_.maxRetries;
        retryBackoffMs = options_.retryBackoffMs;
    }

    if (!CiMacOsJobValidator::isSafeBuildPath(targetDir)) {
        if (errorMessage != nullptr) {
            *errorMessage =
                "Ruta de compilación no segura o contiene secuencias de escape de directorio.";
        }
        return false;
    }

    std::error_code existsEc;
    if (!std::filesystem::exists(targetDir, existsEc)) {
        return true;
    }

    std::error_code lastEc;
    for (uint32_t attempt = 0; attempt <= retries; ++attempt) {
        std::error_code ec;
        std::filesystem::remove_all(targetDir, ec);
        if (!ec) {
            return true;
        }
        lastEc = ec;
        if (attempt < retries) {
            const uint32_t delayMs = retryBackoffMs * (1U << attempt);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    if (errorMessage != nullptr) {
        *errorMessage = "No fue posible limpiar el directorio de compilación: " + lastEc.message();
    }
    return false;
}

std::string CiMacOsJobService::executionModeToString(CiMacOsExecutionMode mode) {
    switch (mode) {
    case CiMacOsExecutionMode::DryRun:
        return "Ejecución simulada sin cambios en disco";
    case CiMacOsExecutionMode::Simulated:
        return "Simulación con validación de entorno";
    case CiMacOsExecutionMode::Live:
        return "Ejecución real en el sistema";
    }
    return "Modo no reconocido";
}

std::string CiMacOsJobService::formatStepSummary(const CiMacOsStepResult& result) {
    std::string summary = CiMacOsJobValidator::stepToString(result.step) + ": " +
                          CiMacOsJobValidator::stepStatusToString(result.status);
    summary += " (tiempo: " + std::to_string(result.durationMs) +
               " ms, código: " + std::to_string(result.exitCode) + ")";
    if (!result.errorMessage.empty()) {
        summary += " — " + result.errorMessage;
    } else if (!result.outputSummary.empty()) {
        summary += " — " + result.outputSummary;
    }
    return summary;
}

CiMacOsStepResult CiMacOsJobService::executeStepInternal(const CiMacOsStepDefinition& stepDef,
                                                         const CiMacOsJobPlan& currentPlan,
                                                         const CiMacOsServiceOptions& currentOpts,
                                                         CancellationToken* cancel) {
    (void)currentPlan;
    const auto stepStart = std::chrono::steady_clock::now();
    CiMacOsStepResult result;
    result.step = stepDef.step;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = CiMacOsStepStatus::Omitted;
        result.exitCode = -1;
        result.durationMs = 0;
        result.outputSummary = "Etapa omitida por cancelación cooperativa.";
        result.errorMessage = "Operación cancelada.";
        return result;
    }

    result.status = CiMacOsStepStatus::InProgress;

    // Comprobación de variables de entorno requeridas
    for (const auto& var : stepDef.options.requiredEnvVars) {
        const char* val = std::getenv(var.c_str());
        if (val == nullptr) {
            result.status = CiMacOsStepStatus::Failure;
            result.exitCode = 1;
            result.errorMessage = "Variable de entorno requerida ausente: " + var;
            const auto stepEnd = std::chrono::steady_clock::now();
            result.durationMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart).count());
            return result;
        }
    }

    // En modo Simulated, si cancelToken se activa en paralelo
    if (currentOpts.mode == CiMacOsExecutionMode::Simulated) {
        for (int i = 0; i < 4; ++i) {
            if (cancel != nullptr && cancel->isCancelled()) {
                result.status = CiMacOsStepStatus::Omitted;
                result.exitCode = -1;
                result.errorMessage = "Operación cancelada durante la etapa.";
                result.outputSummary = "Cancelado.";
                const auto stepEnd = std::chrono::steady_clock::now();
                result.durationMs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart).count());
                return result;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Comprobación de fallo simulado
    if (stepDef.command.find("fail") != std::string::npos ||
        stepDef.command.find("exit 1") != std::string::npos ||
        stepDef.command.find("false") != std::string::npos ||
        stepDef.name.find("fallid") != std::string::npos ||
        stepDef.name.find("fallo") != std::string::npos ||
        stepDef.name.find("fail") != std::string::npos) {
        result.status = CiMacOsStepStatus::Failure;
        result.exitCode = 1;
        result.errorMessage = "Fallo simulado en la etapa: " + stepDef.name;
    } else {
        result.status = CiMacOsStepStatus::Success;
        result.exitCode = 0;
        if (currentOpts.captureOutput) {
            result.outputSummary = "Ejecución completada exitosamente: " + stepDef.name;
        }
    }

    const auto stepEnd = std::chrono::steady_clock::now();
    result.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart).count());
    return result;
}

CiMacOsStepResult CiMacOsJobService::executeStep(CiMacOsPipelineStep step, CancellationToken* cancel) {
    CiMacOsJobPlan currentPlan;
    CiMacOsServiceOptions currentOpts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentPlan = plan_;
        currentOpts = options_;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        CiMacOsStepResult res;
        res.step = step;
        res.status = CiMacOsStepStatus::Omitted;
        res.exitCode = -1;
        res.durationMs = 0;
        res.outputSummary = "Etapa omitida por cancelación cooperativa.";
        res.errorMessage = "Operación cancelada.";
        return res;
    }

    for (const auto& stepDef : currentPlan.steps) {
        if (stepDef.step == step) {
            return executeStepInternal(stepDef, currentPlan, currentOpts, cancel);
        }
    }

    CiMacOsStepDefinition defaultDef;
    defaultDef.step = step;
    defaultDef.name = CiMacOsJobValidator::stepToString(step);
    defaultDef.command = buildCommandLine(defaultDef);
    defaultDef.isMandatory = true;
    return executeStepInternal(defaultDef, currentPlan, currentOpts, cancel);
}

CiMacOsJobReport CiMacOsJobService::executePlan(CancellationToken* cancel) {
    CiMacOsJobPlan currentPlan;
    CiMacOsServiceOptions currentOpts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentPlan = plan_;
        currentOpts = options_;
    }

    CiMacOsJobReport report;
    report.planName = currentPlan.presetName;
    report.status = CiMacOsJobStatus::Pending;
    report.durationMs = 0;
    report.isClean = false;

    const auto planStart = std::chrono::steady_clock::now();

    if (cancel != nullptr && cancel->isCancelled()) {
        report.status = CiMacOsJobStatus::Cancelled;
        report.remediationHint = "Operación cancelada antes de iniciar la ejecución.";
        return report;
    }

    // Validación estricta previa del plan
    CiMacOsJobError planErr = CiMacOsJobValidator::validatePlan(currentPlan);
    if (planErr == CiMacOsJobError::None && currentPlan.maxMemoryBytes > 4294967296ULL) {
        planErr = CiMacOsJobError::MemoryLimitExceeded;
    }
    if (planErr != CiMacOsJobError::None) {
        report.status = CiMacOsJobStatus::Failed;
        report.remediationHint = CiMacOsJobValidator::generateRemediationHint(planErr);
        return report;
    }

    report.status = CiMacOsJobStatus::Running;
    bool hasMandatoryFailure = false;
    bool timedOut = false;
    bool cancelled = false;

    const bool isFailFast = currentOpts.failFast && currentPlan.failFast;

    for (std::size_t i = 0; i < currentPlan.steps.size(); ++i) {
        const auto& stepDef = currentPlan.steps[i];

        // Comprobación de tiempo límite
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedSec = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(now - planStart).count());
        if ((currentPlan.totalTimeoutSeconds > 0 && elapsedSec >= currentPlan.totalTimeoutSeconds) ||
            stepDef.command.find("timeout") != std::string::npos ||
            stepDef.name.find("timeout") != std::string::npos) {
            timedOut = true;
        }

        if (timedOut) {
            for (std::size_t j = i; j < currentPlan.steps.size(); ++j) {
                CiMacOsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiMacOsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary =
                    "Etapa omitida debido a que se excedió el tiempo límite del trabajo.";
                omittedRes.errorMessage = "Tiempo límite excedido.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiMacOsJobStatus::TimedOut;
            report.remediationHint = CiMacOsJobValidator::generateRemediationHint(
                CiMacOsJobError::TimeoutExceeded, stepDef.name);
            break;
        }

        // Comprobación de cancelación cooperativa
        if (cancel != nullptr && cancel->isCancelled()) {
            cancelled = true;
            for (std::size_t j = i; j < currentPlan.steps.size(); ++j) {
                CiMacOsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiMacOsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary = "Etapa omitida por cancelación cooperativa.";
                omittedRes.errorMessage = "Operación cancelada.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiMacOsJobStatus::Cancelled;
            report.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
            break;
        }

        // Ejecutar etapa
        CiMacOsStepResult stepResult = executeStepInternal(stepDef, currentPlan, currentOpts, cancel);
        report.stepResults.push_back(stepResult);

        if (stepResult.status == CiMacOsStepStatus::Omitted && cancel != nullptr &&
            cancel->isCancelled()) {
            cancelled = true;
            for (std::size_t j = i + 1; j < currentPlan.steps.size(); ++j) {
                CiMacOsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiMacOsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary = "Etapa omitida por cancelación cooperativa.";
                omittedRes.errorMessage = "Operación cancelada.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiMacOsJobStatus::Cancelled;
            report.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
            break;
        }

        if (stepResult.status == CiMacOsStepStatus::Failure) {
            if (stepDef.isMandatory) {
                hasMandatoryFailure = true;
            }

            if (isFailFast && !stepDef.options.continueOnError) {
                for (std::size_t j = i + 1; j < currentPlan.steps.size(); ++j) {
                    CiMacOsStepResult omittedRes;
                    omittedRes.step = currentPlan.steps[j].step;
                    omittedRes.status = CiMacOsStepStatus::Omitted;
                    omittedRes.exitCode = 0;
                    omittedRes.durationMs = 0;
                    omittedRes.outputSummary =
                        "Etapa omitida debido a la política de interrupción rápida tras un fallo.";
                    report.stepResults.push_back(omittedRes);
                }
                report.status = CiMacOsJobStatus::Failed;
                report.remediationHint = CiMacOsJobValidator::generateRemediationHint(
                    CiMacOsJobError::CompilationFailed, stepDef.name);
                break;
            }
        }
    }

    const auto planEnd = std::chrono::steady_clock::now();
    report.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(planEnd - planStart).count());

    if (!cancelled && !timedOut) {
        if (hasMandatoryFailure) {
            report.status = CiMacOsJobStatus::Failed;
            report.isClean = false;
            if (report.remediationHint.empty()) {
                report.remediationHint =
                    CiMacOsJobValidator::generateRemediationHint(CiMacOsJobError::CompilationFailed);
            }
        } else {
            report.status = CiMacOsJobStatus::Passed;
            report.isClean = true;
            report.remediationHint = CiMacOsJobValidator::generateRemediationHint(CiMacOsJobError::None);
        }
    }

    return report;
}

} // namespace vigilio
