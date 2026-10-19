#include "core/CiLinuxJobValidator.h"

#include <cctype>

namespace vigilio {

CiLinuxJobError CiLinuxJobValidator::validatePlan(const CiLinuxJobPlan& plan) {
    if (plan.steps.empty() || plan.presetName.empty()) {
        return CiLinuxJobError::InvalidWorkflowSchema;
    }
    if (plan.totalTimeoutSeconds == 0 || plan.totalTimeoutSeconds > 3600) {
        return CiLinuxJobError::InvalidWorkflowSchema;
    }
    if (plan.maxMemoryBytes == 0 || plan.maxMemoryBytes > 8589934592ULL) {
        return CiLinuxJobError::MemoryLimitExceeded;
    }
    if (!isSafeBuildPath(plan.buildDir)) {
        return CiLinuxJobError::UnsafePipelinePath;
    }
    for (const auto& stepDef : plan.steps) {
        if (stepDef.name.empty() || stepDef.command.empty()) {
            return CiLinuxJobError::InvalidWorkflowSchema;
        }
        if (stepDef.options.timeoutMs == 0) {
            return CiLinuxJobError::InvalidWorkflowSchema;
        }
    }

    std::vector<CiLinuxPipelineStep> stepSeq;
    stepSeq.reserve(plan.steps.size());
    for (const auto& stepDef : plan.steps) {
        stepSeq.push_back(stepDef.step);
    }
    return validateStepSequence(stepSeq);
}

CiLinuxJobError
CiLinuxJobValidator::validateStepSequence(const std::vector<CiLinuxPipelineStep>& steps) {
    if (steps.empty()) {
        return CiLinuxJobError::InvalidWorkflowSchema;
    }

    int envCheckIdx = -1;
    int formatIdx = -1;
    int staticIdx = -1;
    int configIdx = -1;
    int buildIdx = -1;
    int testIdx = -1;
    int sanitizerIdx = -1;
    int packageIdx = -1;

    for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
        const auto step = steps[static_cast<std::size_t>(i)];
        switch (step) {
        case CiLinuxPipelineStep::EnvironmentCheck:
            if (envCheckIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            envCheckIdx = i;
            break;
        case CiLinuxPipelineStep::FormatVerification:
            if (formatIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            formatIdx = i;
            break;
        case CiLinuxPipelineStep::StaticAnalysis:
            if (staticIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            staticIdx = i;
            break;
        case CiLinuxPipelineStep::ConfigurePreset:
            if (configIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            configIdx = i;
            break;
        case CiLinuxPipelineStep::BuildCompilation:
            if (buildIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            buildIdx = i;
            break;
        case CiLinuxPipelineStep::TestSuiteExecution:
            if (testIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            testIdx = i;
            break;
        case CiLinuxPipelineStep::SanitizerVerification:
            if (sanitizerIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            sanitizerIdx = i;
            break;
        case CiLinuxPipelineStep::ArtifactPackaging:
            if (packageIdx != -1) {
                return CiLinuxJobError::DependencyOrderingViolation;
            }
            packageIdx = i;
            break;
        }
    }

    // Comprobaciones estáticas y de entorno previas a compilación
    if (envCheckIdx != -1 && buildIdx != -1 && envCheckIdx > buildIdx) {
        return CiLinuxJobError::DependencyOrderingViolation;
    }
    if (formatIdx != -1 && buildIdx != -1 && formatIdx > buildIdx) {
        return CiLinuxJobError::DependencyOrderingViolation;
    }
    if (staticIdx != -1 && buildIdx != -1 && staticIdx > buildIdx) {
        return CiLinuxJobError::DependencyOrderingViolation;
    }

    // Compilación requiere configuración previa
    if (buildIdx != -1) {
        if (configIdx == -1 || configIdx > buildIdx) {
            return CiLinuxJobError::DependencyOrderingViolation;
        }
    }

    // Pruebas requieren compilación previa
    if (testIdx != -1) {
        if (buildIdx == -1 || buildIdx > testIdx) {
            return CiLinuxJobError::DependencyOrderingViolation;
        }
    }

    // Sanitizadores requieren compilación previa
    if (sanitizerIdx != -1) {
        if (buildIdx == -1 || buildIdx > sanitizerIdx) {
            return CiLinuxJobError::DependencyOrderingViolation;
        }
    }

    // Empaquetado requiere compilación y pruebas (si existen pruebas)
    if (packageIdx != -1) {
        if (buildIdx == -1 || buildIdx > packageIdx) {
            return CiLinuxJobError::DependencyOrderingViolation;
        }
        if (testIdx != -1 && packageIdx < testIdx) {
            return CiLinuxJobError::DependencyOrderingViolation;
        }
    }

    return CiLinuxJobError::None;
}

CiLinuxJobError CiLinuxJobValidator::validateEnvironment(const CiLinuxEnvironment& env) {
    if (env.architecture != CiLinuxArchitecture::X86_64 &&
        env.architecture != CiLinuxArchitecture::AArch64) {
        return CiLinuxJobError::ToolchainMissing;
    }

    if (env.compiler == CiLinuxCompiler::GCC) {
        if (env.compilerMajorVersion < 14) {
            return CiLinuxJobError::ToolchainMissing;
        }
    } else if (env.compiler == CiLinuxCompiler::Clang) {
        if (env.compilerMajorVersion < 18) {
            return CiLinuxJobError::ToolchainMissing;
        }
    }

    if (env.cmakeMajorVersion < 3 || (env.cmakeMajorVersion == 3 && env.cmakeMinorVersion < 24)) {
        return CiLinuxJobError::ToolchainMissing;
    }

    if (env.totalMemoryBytes < 2147483648ULL) {
        return CiLinuxJobError::ToolchainMissing;
    }

    if (env.availableCores < 1) {
        return CiLinuxJobError::ToolchainMissing;
    }

    return CiLinuxJobError::None;
}

bool CiLinuxJobValidator::isSafeBuildPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    const std::string s = path.generic_string();
    if (s.empty()) {
        return false;
    }

    // Rechazo de Directory Traversal y secuencias inseguras
    if (s.find("..") != std::string::npos || s.find('\\') != std::string::npos ||
        s.find('\0') != std::string::npos) {
        return false;
    }

    // Rechazo de rutas absolutas POSIX y Windows
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory() || s.front() == '/') {
        return false;
    }
    if (s.size() >= 2 && std::isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':') {
        return false;
    }

    std::string_view target = s;
    constexpr std::string_view sourceDirPrefix = "${sourceDir}/";
    if (target.rfind(sourceDirPrefix, 0) == 0) {
        target = target.substr(sourceDirPrefix.size());
    }

    std::filesystem::path cleanPath(target);
    auto it = cleanPath.begin();
    if (it == cleanPath.end()) {
        return false;
    }
    const std::string first = it->generic_string();
    if (first.rfind("build-ci-", 0) == 0 || first.rfind("_build", 0) == 0 ||
        first.rfind("build-", 0) == 0) {
        return true;
    }
    return false;
}

std::string CiLinuxJobValidator::jobStatusToString(CiLinuxJobStatus status) {
    switch (status) {
    case CiLinuxJobStatus::Pending:
        return "Pendiente de ejecución";
    case CiLinuxJobStatus::Running:
        return "En ejecución";
    case CiLinuxJobStatus::Passed:
        return "Aprobado con éxito";
    case CiLinuxJobStatus::Failed:
        return "Falló con incidencias";
    case CiLinuxJobStatus::Cancelled:
        return "Cancelado por el usuario";
    case CiLinuxJobStatus::TimedOut:
        return "Tiempo límite excedido";
    }
    return "Estado desconocido";
}

std::string CiLinuxJobValidator::jobErrorToString(CiLinuxJobError error) {
    switch (error) {
    case CiLinuxJobError::None:
        return "Sin incidencias";
    case CiLinuxJobError::ToolchainMissing:
        return "Herramientas de compilación requeridas no disponibles";
    case CiLinuxJobError::ConfigurationFailed:
        return "Error en la configuración de compilación";
    case CiLinuxJobError::CompilationFailed:
        return "Error en la compilación del código";
    case CiLinuxJobError::TestExecutionFailed:
        return "Una o más pruebas automáticas fallaron";
    case CiLinuxJobError::FormatDiscrepancy:
        return "Discrepancias en el formato de código fuente";
    case CiLinuxJobError::StaticAnalysisIssue:
        return "Incidencias detectadas en el análisis estático de código";
    case CiLinuxJobError::UnsafePipelinePath:
        return "Ruta de trabajo no autorizada o insegura";
    case CiLinuxJobError::TimeoutExceeded:
        return "Tiempo límite de ejecución excedido";
    case CiLinuxJobError::InvalidWorkflowSchema:
        return "Esquema de etapas de trabajo no válido";
    case CiLinuxJobError::DependencyOrderingViolation:
        return "Orden incorrecto en las etapas del trabajo";
    case CiLinuxJobError::MemoryLimitExceeded:
        return "Límite de memoria asignada excedido";
    }
    return "Error no clasificado";
}

std::string CiLinuxJobValidator::stepToString(CiLinuxPipelineStep step) {
    switch (step) {
    case CiLinuxPipelineStep::EnvironmentCheck:
        return "Verificación del entorno de compilación";
    case CiLinuxPipelineStep::FormatVerification:
        return "Comprobación de formato de código";
    case CiLinuxPipelineStep::StaticAnalysis:
        return "Análisis estático de calidad de código";
    case CiLinuxPipelineStep::ConfigurePreset:
        return "Configuración de compilación del sistema";
    case CiLinuxPipelineStep::BuildCompilation:
        return "Compilación del proyecto";
    case CiLinuxPipelineStep::TestSuiteExecution:
        return "Ejecución de pruebas automatizadas";
    case CiLinuxPipelineStep::SanitizerVerification:
        return "Verificación de sanitizadores de memoria";
    case CiLinuxPipelineStep::ArtifactPackaging:
        return "Validación de empaquetado del navegador";
    }
    return "Etapa no reconocida";
}

std::string CiLinuxJobValidator::stepStatusToString(CiLinuxStepStatus status) {
    switch (status) {
    case CiLinuxStepStatus::NotStarted:
        return "No iniciada";
    case CiLinuxStepStatus::InProgress:
        return "En curso";
    case CiLinuxStepStatus::Success:
        return "Completada con éxito";
    case CiLinuxStepStatus::Failure:
        return "Fallida";
    case CiLinuxStepStatus::Omitted:
        return "Omitida";
    }
    return "Estado de etapa desconocido";
}

std::string CiLinuxJobValidator::architectureToString(CiLinuxArchitecture arch) {
    switch (arch) {
    case CiLinuxArchitecture::X86_64:
        return "x86_64";
    case CiLinuxArchitecture::AArch64:
        return "aarch64";
    }
    return "desconocida";
}

std::string CiLinuxJobValidator::compilerToString(CiLinuxCompiler compiler) {
    switch (compiler) {
    case CiLinuxCompiler::GCC:
        return "GCC";
    case CiLinuxCompiler::Clang:
        return "Clang";
    }
    return "Desconocido";
}

std::string CiLinuxJobValidator::generateRemediationHint(CiLinuxJobError error,
                                                         std::string_view stepName) {
    std::string prefix;
    if (!stepName.empty()) {
        prefix = "En la etapa '" + std::string(stepName) + "': ";
    }
    switch (error) {
    case CiLinuxJobError::None:
        return "No se requiere ninguna acción de remediación.";
    case CiLinuxJobError::ToolchainMissing:
        return prefix + "Instale o actualice las herramientas de compilación requeridas (CMake 3.24 o "
                        "superior y compilador C++20 compatible como GCC 14 o Clang 18).";
    case CiLinuxJobError::ConfigurationFailed:
        return prefix + "Revise los archivos de configuración y asegúrese de que las dependencias y "
                        "opciones del sistema sean correctas.";
    case CiLinuxJobError::CompilationFailed:
        return prefix + "Corrija los errores sintácticos o advertencias de compilación reportados en el "
                        "código fuente.";
    case CiLinuxJobError::TestExecutionFailed:
        return prefix + "Examine las aserciones fallidas en las pruebas automatizadas y verifique las "
                        "precondiciones del entorno.";
    case CiLinuxJobError::FormatDiscrepancy:
        return prefix + "Ejecute la verificación y corrección de formato de código para alinear los "
                        "archivos al estándar establecido.";
    case CiLinuxJobError::StaticAnalysisIssue:
        return prefix + "Revise y resuelva las incidencias de calidad, rendimiento o seguridad "
                        "señaladas por el análisis estático.";
    case CiLinuxJobError::UnsafePipelinePath:
        return prefix + "Especifique un directorio de compilación relativo y seguro dentro del proyecto "
                        "(como build-ci-debug o _build), sin referencias externas ni rutas del sistema.";
    case CiLinuxJobError::TimeoutExceeded:
        return prefix + "Optimice la duración de las tareas o incremente el tiempo límite asignado a la "
                        "etapa afectada si es necesario.";
    case CiLinuxJobError::InvalidWorkflowSchema:
        return prefix + "Defina una lista válida de etapas con parámetros consistentes, tiempos límites "
                        "positivos y memoria dentro del presupuesto.";
    case CiLinuxJobError::DependencyOrderingViolation:
        return prefix + "Ordene las etapas respetando sus dependencias lógicas: configure antes de "
                        "compilar y compile antes de ejecutar pruebas.";
    case CiLinuxJobError::MemoryLimitExceeded:
        return prefix + "Ajuste el presupuesto de memoria del trabajo para que no supere el límite "
                        "máximo seguro permitido (4 GB).";
    }
    return prefix + "Consulte los registros locales para obtener detalles adicionales de la incidencia.";
}

} // namespace vigilio
