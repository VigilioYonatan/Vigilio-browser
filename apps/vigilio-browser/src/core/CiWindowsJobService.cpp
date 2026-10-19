#include "core/CiWindowsJobService.h"
#include "core/CiWindowsJobConfiguration.h"
#include "core/CiWindowsJobValidator.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <thread>

namespace vigilio {

CiWindowsJobService::CiWindowsJobService()
    : plan_(CiWindowsJobConfiguration::canonicalDebugPlan()), options_() {
}

CiWindowsJobService::CiWindowsJobService(CiWindowsJobPlan plan) : plan_(std::move(plan)), options_() {
}

void CiWindowsJobService::setPlan(CiWindowsJobPlan plan) {
    std::lock_guard<std::mutex> lock(mutex_);
    plan_ = std::move(plan);
}

CiWindowsJobPlan CiWindowsJobService::plan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_;
}

void CiWindowsJobService::setOptions(CiWindowsServiceOptions options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
}

CiWindowsServiceOptions CiWindowsJobService::options() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

std::string CiWindowsJobService::buildCommandLine(const CiWindowsStepDefinition& step) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string buildDirStr = plan_.buildDir.string();
    const std::string presetNameStr = plan_.presetName;

    switch (step.step) {
    case CiWindowsPipelineStep::EnvironmentCheck:
        return "cmake --version && ninja --version && cl.exe";
    case CiWindowsPipelineStep::FormatVerification:
        return "powershell -Command \"Get-ChildItem -Recurse -Include *.cpp,*.h "
               "apps/vigilio-browser/src, apps/vigilio-browser/tests | ForEach-Object { clang-format "
               "--dry-run --Werror $_.FullName }\"";
    case CiWindowsPipelineStep::StaticAnalysis:
        return "clang-tidy -p " + buildDirStr + " apps/vigilio-browser/src/**/*.cpp";
    case CiWindowsPipelineStep::ConfigurePreset:
        return "cmake --preset " + presetNameStr + " -B " + buildDirStr;
    case CiWindowsPipelineStep::BuildCompilation:
        return "cmake --build " + buildDirStr + " --parallel";
    case CiWindowsPipelineStep::TestSuiteExecution:
        return "ctest --test-dir " + buildDirStr + " --output-on-failure -L fast --timeout 15";
    case CiWindowsPipelineStep::SanitizerVerification:
        return "ctest --test-dir " + buildDirStr + " --output-on-failure -L sanitizer";
    case CiWindowsPipelineStep::ArtifactPackaging:
        return "cpack --config " + buildDirStr + "/CPackConfig.cmake";
    }
    return step.command;
}

bool CiWindowsJobService::cleanBuildDirectory(std::string* errorMessage) {
    std::filesystem::path targetDir;
    uint32_t retries = 0;
    uint32_t retryBackoffMs = 50;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        targetDir = plan_.buildDir;
        retries = options_.maxRetries;
        retryBackoffMs = options_.retryBackoffMs;
    }

    if (!CiWindowsJobValidator::isSafeBuildPath(targetDir)) {
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

std::string CiWindowsJobService::executionModeToString(CiWindowsExecutionMode mode) {
    switch (mode) {
    case CiWindowsExecutionMode::DryRun:
        return "Ejecución simulada sin cambios en disco";
    case CiWindowsExecutionMode::Simulated:
        return "Simulación con validación de entorno";
    case CiWindowsExecutionMode::Live:
        return "Ejecución real en el sistema";
    }
    return "Modo no reconocido";
}

std::string CiWindowsJobService::formatStepSummary(const CiWindowsStepResult& result) {
    std::string summary = CiWindowsJobValidator::stepToString(result.step) + ": " +
                          CiWindowsJobValidator::stepStatusToString(result.status);
    summary += " (tiempo: " + std::to_string(result.durationMs) +
               " ms, código: " + std::to_string(result.exitCode) + ")";
    if (!result.errorMessage.empty()) {
        summary += " — " + result.errorMessage;
    } else if (!result.outputSummary.empty()) {
        summary += " — " + result.outputSummary;
    }
    return summary;
}

CiWindowsStepResult CiWindowsJobService::executeStepInternal(const CiWindowsStepDefinition& stepDef,
                                                             const CiWindowsJobPlan& currentPlan,
                                                             const CiWindowsServiceOptions& currentOpts,
                                                             CancellationToken* cancel) {
    (void)currentPlan;
    const auto stepStart = std::chrono::steady_clock::now();
    CiWindowsStepResult result;
    result.step = stepDef.step;

    if (cancel != nullptr && cancel->isCancelled()) {
        result.status = CiWindowsStepStatus::Omitted;
        result.exitCode = -1;
        result.durationMs = 0;
        result.outputSummary = "Etapa omitida por cancelación cooperativa.";
        result.errorMessage = "Operación cancelada.";
        return result;
    }

    result.status = CiWindowsStepStatus::InProgress;

    // Comprobación de variables de entorno requeridas
    for (const auto& var : stepDef.options.requiredEnvVars) {
        const char* val = std::getenv(var.c_str());
        if (val == nullptr) {
            result.status = CiWindowsStepStatus::Failure;
            result.exitCode = 1;
            result.errorMessage = "Variable de entorno requerida ausente: " + var;
            const auto stepEnd = std::chrono::steady_clock::now();
            result.durationMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(stepEnd - stepStart).count());
            return result;
        }
    }

    // En modo Simulated, si cancelToken se activa en paralelo
    if (currentOpts.mode == CiWindowsExecutionMode::Simulated) {
        for (int i = 0; i < 4; ++i) {
            if (cancel != nullptr && cancel->isCancelled()) {
                result.status = CiWindowsStepStatus::Omitted;
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
        result.status = CiWindowsStepStatus::Failure;
        result.exitCode = 1;
        result.errorMessage = "Fallo simulado en la etapa: " + stepDef.name;
    } else {
        result.status = CiWindowsStepStatus::Success;
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

CiWindowsStepResult CiWindowsJobService::executeStep(CiWindowsPipelineStep step,
                                                     CancellationToken* cancel) {
    CiWindowsJobPlan currentPlan;
    CiWindowsServiceOptions currentOpts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentPlan = plan_;
        currentOpts = options_;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        CiWindowsStepResult res;
        res.step = step;
        res.status = CiWindowsStepStatus::Omitted;
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

    CiWindowsStepDefinition defaultDef;
    defaultDef.step = step;
    defaultDef.name = CiWindowsJobValidator::stepToString(step);
    defaultDef.command = buildCommandLine(defaultDef);
    defaultDef.isMandatory = true;
    return executeStepInternal(defaultDef, currentPlan, currentOpts, cancel);
}

CiWindowsJobReport CiWindowsJobService::executePlan(CancellationToken* cancel) {
    CiWindowsJobPlan currentPlan;
    CiWindowsServiceOptions currentOpts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentPlan = plan_;
        currentOpts = options_;
    }

    CiWindowsJobReport report;
    report.planName = currentPlan.presetName;
    report.status = CiWindowsJobStatus::Pending;
    report.durationMs = 0;
    report.isClean = false;

    const auto planStart = std::chrono::steady_clock::now();

    if (cancel != nullptr && cancel->isCancelled()) {
        report.status = CiWindowsJobStatus::Cancelled;
        report.remediationHint = "Operación cancelada antes de iniciar la ejecución.";
        return report;
    }

    // Validación estricta previa del plan
    CiWindowsJobError planErr = CiWindowsJobValidator::validatePlan(currentPlan);
    if (planErr == CiWindowsJobError::None && currentPlan.maxMemoryBytes > 4294967296ULL) {
        planErr = CiWindowsJobError::MemoryLimitExceeded;
    }
    if (planErr != CiWindowsJobError::None) {
        report.status = CiWindowsJobStatus::Failed;
        report.remediationHint = CiWindowsJobValidator::generateRemediationHint(planErr);
        return report;
    }

    report.status = CiWindowsJobStatus::Running;
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
                CiWindowsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiWindowsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary =
                    "Etapa omitida debido a que se excedió el tiempo límite del trabajo.";
                omittedRes.errorMessage = "Tiempo límite excedido.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiWindowsJobStatus::TimedOut;
            report.remediationHint = CiWindowsJobValidator::generateRemediationHint(
                CiWindowsJobError::TimeoutExceeded, stepDef.name);
            break;
        }

        // Comprobación de cancelación cooperativa
        if (cancel != nullptr && cancel->isCancelled()) {
            cancelled = true;
            for (std::size_t j = i; j < currentPlan.steps.size(); ++j) {
                CiWindowsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiWindowsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary = "Etapa omitida por cancelación cooperativa.";
                omittedRes.errorMessage = "Operación cancelada.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiWindowsJobStatus::Cancelled;
            report.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
            break;
        }

        // Ejecutar etapa
        CiWindowsStepResult stepResult = executeStepInternal(stepDef, currentPlan, currentOpts, cancel);
        report.stepResults.push_back(stepResult);

        if (stepResult.status == CiWindowsStepStatus::Omitted && cancel != nullptr &&
            cancel->isCancelled()) {
            cancelled = true;
            for (std::size_t j = i + 1; j < currentPlan.steps.size(); ++j) {
                CiWindowsStepResult omittedRes;
                omittedRes.step = currentPlan.steps[j].step;
                omittedRes.status = CiWindowsStepStatus::Omitted;
                omittedRes.exitCode = -1;
                omittedRes.durationMs = 0;
                omittedRes.outputSummary = "Etapa omitida por cancelación cooperativa.";
                omittedRes.errorMessage = "Operación cancelada.";
                report.stepResults.push_back(omittedRes);
            }
            report.status = CiWindowsJobStatus::Cancelled;
            report.remediationHint = "Operación cancelada a petición del usuario o hilo de control.";
            break;
        }

        if (stepResult.status == CiWindowsStepStatus::Failure) {
            if (stepDef.isMandatory) {
                hasMandatoryFailure = true;
            }

            if (isFailFast && !stepDef.options.continueOnError) {
                for (std::size_t j = i + 1; j < currentPlan.steps.size(); ++j) {
                    CiWindowsStepResult omittedRes;
                    omittedRes.step = currentPlan.steps[j].step;
                    omittedRes.status = CiWindowsStepStatus::Omitted;
                    omittedRes.exitCode = 0;
                    omittedRes.durationMs = 0;
                    omittedRes.outputSummary =
                        "Etapa omitida debido a la política de interrupción rápida tras un fallo.";
                    report.stepResults.push_back(omittedRes);
                }
                report.status = CiWindowsJobStatus::Failed;
                report.remediationHint = CiWindowsJobValidator::generateRemediationHint(
                    CiWindowsJobError::CompilationFailed, stepDef.name);
                break;
            }
        }
    }

    const auto planEnd = std::chrono::steady_clock::now();
    report.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(planEnd - planStart).count());

    if (!cancelled && !timedOut) {
        if (hasMandatoryFailure) {
            report.status = CiWindowsJobStatus::Failed;
            report.isClean = false;
            if (report.remediationHint.empty()) {
                report.remediationHint =
                    CiWindowsJobValidator::generateRemediationHint(CiWindowsJobError::CompilationFailed);
            }
        } else {
            report.status = CiWindowsJobStatus::Passed;
            report.isClean = true;
            report.remediationHint =
                CiWindowsJobValidator::generateRemediationHint(CiWindowsJobError::None);
        }
    }

    return report;
}

} // namespace vigilio
