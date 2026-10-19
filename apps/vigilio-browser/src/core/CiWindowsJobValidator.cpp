#include "core/CiWindowsJobValidator.h"
#include "core/CiWindowsJobConfiguration.h"

#include <cctype>
#include <vector>

namespace vigilio {

CiWindowsJobError CiWindowsJobValidator::validatePlan(const CiWindowsJobPlan& plan) {
    if (plan.steps.empty() || plan.presetName.empty()) {
        return CiWindowsJobError::InvalidWorkflowSchema;
    }
    if (plan.totalTimeoutSeconds == 0 || plan.totalTimeoutSeconds > 3600) {
        return CiWindowsJobError::InvalidWorkflowSchema;
    }
    if (plan.maxMemoryBytes == 0 || plan.maxMemoryBytes > 8589934592ULL) {
        return CiWindowsJobError::MemoryLimitExceeded;
    }
    if (!isSafeBuildPath(plan.buildDir)) {
        return CiWindowsJobError::UnsafePipelinePath;
    }
    for (const auto& stepDef : plan.steps) {
        if (stepDef.name.empty() || stepDef.command.empty()) {
            return CiWindowsJobError::InvalidWorkflowSchema;
        }
        if (stepDef.options.timeoutMs == 0) {
            return CiWindowsJobError::InvalidWorkflowSchema;
        }
    }

    std::vector<CiWindowsPipelineStep> stepSeq;
    stepSeq.reserve(plan.steps.size());
    for (const auto& stepDef : plan.steps) {
        stepSeq.push_back(stepDef.step);
    }
    return validateStepSequence(stepSeq);
}

CiWindowsJobError
CiWindowsJobValidator::validateStepSequence(const std::vector<CiWindowsPipelineStep>& steps) {
    if (steps.empty()) {
        return CiWindowsJobError::InvalidWorkflowSchema;
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
        case CiWindowsPipelineStep::EnvironmentCheck:
            if (envCheckIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            envCheckIdx = i;
            break;
        case CiWindowsPipelineStep::FormatVerification:
            if (formatIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            formatIdx = i;
            break;
        case CiWindowsPipelineStep::StaticAnalysis:
            if (staticIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            staticIdx = i;
            break;
        case CiWindowsPipelineStep::ConfigurePreset:
            if (configIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            configIdx = i;
            break;
        case CiWindowsPipelineStep::BuildCompilation:
            if (buildIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            buildIdx = i;
            break;
        case CiWindowsPipelineStep::TestSuiteExecution:
            if (testIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            testIdx = i;
            break;
        case CiWindowsPipelineStep::SanitizerVerification:
            if (sanitizerIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            sanitizerIdx = i;
            break;
        case CiWindowsPipelineStep::ArtifactPackaging:
            if (packageIdx != -1) {
                return CiWindowsJobError::DependencyOrderingViolation;
            }
            packageIdx = i;
            break;
        }
    }

    // Comprobaciones estáticas y de entorno previas a compilación
    if (envCheckIdx != -1 && buildIdx != -1 && envCheckIdx > buildIdx) {
        return CiWindowsJobError::DependencyOrderingViolation;
    }
    if (formatIdx != -1 && buildIdx != -1 && formatIdx > buildIdx) {
        return CiWindowsJobError::DependencyOrderingViolation;
    }
    if (staticIdx != -1 && buildIdx != -1 && staticIdx > buildIdx) {
        return CiWindowsJobError::DependencyOrderingViolation;
    }

    // Compilación requiere configuración previa
    if (buildIdx != -1) {
        if (configIdx == -1 || configIdx > buildIdx) {
            return CiWindowsJobError::DependencyOrderingViolation;
        }
    }

    // Pruebas requieren compilación previa
    if (testIdx != -1) {
        if (buildIdx == -1 || buildIdx > testIdx) {
            return CiWindowsJobError::DependencyOrderingViolation;
        }
    }

    // Sanitizadores requieren compilación previa
    if (sanitizerIdx != -1) {
        if (buildIdx == -1 || buildIdx > sanitizerIdx) {
            return CiWindowsJobError::DependencyOrderingViolation;
        }
    }

    // Empaquetado requiere compilación y pruebas (si existen pruebas)
    if (packageIdx != -1) {
        if (buildIdx == -1 || buildIdx > packageIdx) {
            return CiWindowsJobError::DependencyOrderingViolation;
        }
        if (testIdx != -1 && packageIdx < testIdx) {
            return CiWindowsJobError::DependencyOrderingViolation;
        }
    }

    return CiWindowsJobError::None;
}

CiWindowsJobError CiWindowsJobValidator::validateEnvironment(const CiWindowsEnvironment& env) {
    if (env.architecture != CiWindowsArchitecture::X86_64 &&
        env.architecture != CiWindowsArchitecture::ARM64) {
        return CiWindowsJobError::ToolchainMissing;
    }

    if (!CiWindowsJobConfiguration::isSupportedWindowsVersion(env.windowsVersion)) {
        return CiWindowsJobError::ToolchainMissing;
    }

    if (env.compiler == CiWindowsCompiler::MSVC) {
        if (env.compilerMajorVersion < 19 ||
            (env.compilerMajorVersion == 19 && env.compilerMinorVersion < 38)) {
            return CiWindowsJobError::ToolchainMissing;
        }
    } else if (env.compiler == CiWindowsCompiler::ClangCl) {
        if (env.compilerMajorVersion < 18) {
            return CiWindowsJobError::ToolchainMissing;
        }
    } else if (env.compiler == CiWindowsCompiler::MinGW) {
        if (env.compilerMajorVersion < 13) {
            return CiWindowsJobError::ToolchainMissing;
        }
    }

    // Validación de Windows SDK
    if (env.windowsSdkVersion.empty()) {
        return CiWindowsJobError::WindowsSdkMissing;
    }
    if (env.windowsSdkVersion.rfind("10.0.", 0) != 0) {
        return CiWindowsJobError::WindowsSdkMissing;
    }
    std::size_t nextDot = env.windowsSdkVersion.find('.', 5);
    std::string buildStr = (nextDot == std::string::npos) ? env.windowsSdkVersion.substr(5)
                                                          : env.windowsSdkVersion.substr(5, nextDot - 5);
    try {
        unsigned long buildNum = std::stoul(buildStr);
        if (buildNum < 19041) {
            return CiWindowsJobError::WindowsSdkMissing;
        }
    } catch (...) {
        return CiWindowsJobError::WindowsSdkMissing;
    }

    if (env.cmakeMajorVersion < 3 || (env.cmakeMajorVersion == 3 && env.cmakeMinorVersion < 28)) {
        return CiWindowsJobError::ToolchainMissing;
    }

    if (env.ninjaMajorVersion < 1 || (env.ninjaMajorVersion == 1 && env.ninjaMinorVersion < 12)) {
        return CiWindowsJobError::ToolchainMissing;
    }

    if (env.totalMemoryBytes < 2147483648ULL) {
        return CiWindowsJobError::ToolchainMissing;
    }

    if (env.availableCores < 1) {
        return CiWindowsJobError::ToolchainMissing;
    }

    return CiWindowsJobError::None;
}

bool CiWindowsJobValidator::isSafeBuildPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    const std::string s = path.string();
    if (s.empty()) {
        return false;
    }

    // Rechazo de Directory Traversal y caracteres nulos
    if (s.find("..") != std::string::npos || s.find('\0') != std::string::npos) {
        return false;
    }

    // Rechazo de rutas absolutas POSIX y Windows
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory() || s.front() == '/' ||
        s.front() == '\\') {
        return false;
    }
    if (s.size() >= 2 && std::isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':') {
        return false;
    }

    // Rechazo de prefijos UNC y espacios de nombres NT
    if (s.rfind("//", 0) == 0 || s.rfind("\\\\", 0) == 0) {
        return false;
    }

    // Normalización de separadores
    std::string normalized = s;
    for (char& c : normalized) {
        if (c == '\\') {
            c = '/';
        }
    }

    constexpr std::string_view sourceDirPrefix = "${sourceDir}/";
    std::string_view target = normalized;
    if (target.rfind(sourceDirPrefix, 0) == 0) {
        target = target.substr(sourceDirPrefix.size());
    }

    // Análisis de segmentos y nombres reservados DOS
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start < target.size()) {
        std::size_t end = target.find('/', start);
        if (end == std::string::npos) {
            segments.emplace_back(target.substr(start));
            break;
        }
        segments.emplace_back(target.substr(start, end - start));
        start = end + 1;
    }

    if (segments.empty()) {
        return false;
    }

    static const std::vector<std::string> reservedDos = {
        "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
        "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    for (const auto& segment : segments) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        std::size_t dotPos = segment.find('.');
        std::string base = (dotPos == std::string::npos) ? segment : segment.substr(0, dotPos);
        for (char& c : base) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        for (const auto& res : reservedDos) {
            if (base == res) {
                return false;
            }
        }
    }

    const std::string& first = segments.front();
    if (first.rfind("build-ci-win-", 0) == 0 || first.rfind("build-ci-", 0) == 0 ||
        first.rfind("_build", 0) == 0 || first.rfind("build-", 0) == 0) {
        return true;
    }

    return false;
}

std::string CiWindowsJobValidator::jobStatusToString(CiWindowsJobStatus status) {
    switch (status) {
    case CiWindowsJobStatus::Pending:
        return "Pendiente de ejecución";
    case CiWindowsJobStatus::Running:
        return "En ejecución";
    case CiWindowsJobStatus::Passed:
        return "Completado correctamente";
    case CiWindowsJobStatus::Failed:
        return "Finalizado con incidencias";
    case CiWindowsJobStatus::Cancelled:
        return "Cancelado por el usuario";
    case CiWindowsJobStatus::TimedOut:
        return "Tiempo límite excedido";
    }
    return "Estado desconocido";
}

std::string CiWindowsJobValidator::jobErrorToString(CiWindowsJobError error) {
    switch (error) {
    case CiWindowsJobError::None:
        return "Sin incidencias";
    case CiWindowsJobError::ToolchainMissing:
        return "Herramientas de compilación no encontradas o incompatibles en el equipo";
    case CiWindowsJobError::WindowsSdkMissing:
        return "Paquete de desarrollo de Windows (SDK) no encontrado en el sistema";
    case CiWindowsJobError::ConfigurationFailed:
        return "Error en la configuración de compilación";
    case CiWindowsJobError::CompilationFailed:
        return "Error en la compilación del código";
    case CiWindowsJobError::TestExecutionFailed:
        return "Una o más pruebas automáticas fallaron";
    case CiWindowsJobError::FormatDiscrepancy:
        return "Discrepancias en el formato de código fuente";
    case CiWindowsJobError::StaticAnalysisIssue:
        return "Incidencias detectadas en el análisis estático de código";
    case CiWindowsJobError::UnsafePipelinePath:
        return "Ruta de compilación no segura o fuera del directorio del proyecto";
    case CiWindowsJobError::TimeoutExceeded:
        return "Tiempo límite de ejecución excedido";
    case CiWindowsJobError::InvalidWorkflowSchema:
        return "Esquema de etapas de trabajo no válido";
    case CiWindowsJobError::DependencyOrderingViolation:
        return "Orden incorrecto en las etapas de compilación y verificación";
    case CiWindowsJobError::MemoryLimitExceeded:
        return "Límite de memoria asignada excedido para la tarea";
    }
    return "Error no clasificado";
}

std::string CiWindowsJobValidator::stepToString(CiWindowsPipelineStep step) {
    switch (step) {
    case CiWindowsPipelineStep::EnvironmentCheck:
        return "Verificación del entorno de compilación en Windows";
    case CiWindowsPipelineStep::FormatVerification:
        return "Comprobación de formato de código";
    case CiWindowsPipelineStep::StaticAnalysis:
        return "Análisis estático de calidad de código";
    case CiWindowsPipelineStep::ConfigurePreset:
        return "Configuración de compilación del sistema";
    case CiWindowsPipelineStep::BuildCompilation:
        return "Compilación del proyecto";
    case CiWindowsPipelineStep::TestSuiteExecution:
        return "Ejecución de pruebas automatizadas";
    case CiWindowsPipelineStep::SanitizerVerification:
        return "Verificación de sanitizadores de memoria";
    case CiWindowsPipelineStep::ArtifactPackaging:
        return "Validación de empaquetado del instalador";
    }
    return "Etapa no reconocida";
}

std::string CiWindowsJobValidator::stepStatusToString(CiWindowsStepStatus status) {
    switch (status) {
    case CiWindowsStepStatus::NotStarted:
        return "No iniciado";
    case CiWindowsStepStatus::InProgress:
        return "En progreso";
    case CiWindowsStepStatus::Success:
        return "Completado con éxito";
    case CiWindowsStepStatus::Failure:
        return "Error en la etapa";
    case CiWindowsStepStatus::Omitted:
        return "Omitido";
    }
    return "Estado de etapa desconocido";
}

std::string CiWindowsJobValidator::architectureToString(CiWindowsArchitecture arch) {
    switch (arch) {
    case CiWindowsArchitecture::X86_64:
        return "x86_64";
    case CiWindowsArchitecture::ARM64:
        return "arm64";
    }
    return "desconocida";
}

std::string CiWindowsJobValidator::compilerToString(CiWindowsCompiler compiler) {
    switch (compiler) {
    case CiWindowsCompiler::MSVC:
        return "MSVC";
    case CiWindowsCompiler::ClangCl:
        return "Clang-cl";
    case CiWindowsCompiler::MinGW:
        return "MinGW";
    }
    return "Desconocido";
}

std::string CiWindowsJobValidator::generateRemediationHint(CiWindowsJobError error,
                                                           std::string_view stepName) {
    std::string prefix;
    if (!stepName.empty()) {
        prefix = "En la etapa '" + std::string(stepName) + "': ";
    }
    switch (error) {
    case CiWindowsJobError::None:
        return "No se requiere ninguna acción de remediación.";
    case CiWindowsJobError::ToolchainMissing:
        return prefix +
               "Instale o actualice las herramientas de compilación requeridas para Windows (Visual "
               "Studio 2022 v143 con MSVC 19.38 o Clang-cl 18, Ninja 1.12 y CMake 3.28 o superior).";
    case CiWindowsJobError::WindowsSdkMissing:
        return prefix + "Instale el paquete de desarrollo de Windows (Windows SDK versión 10.0.22621.0 "
                        "o superior) mediante el instalador de Visual Studio.";
    case CiWindowsJobError::ConfigurationFailed:
        return prefix + "Revise los archivos de configuración y asegúrese de que las dependencias y "
                        "opciones del sistema sean correctas.";
    case CiWindowsJobError::CompilationFailed:
        return prefix + "Corrija los errores sintácticos o advertencias de compilación reportados en el "
                        "código fuente.";
    case CiWindowsJobError::TestExecutionFailed:
        return prefix + "Examine las aserciones fallidas en las pruebas automatizadas y verifique las "
                        "precondiciones del entorno.";
    case CiWindowsJobError::FormatDiscrepancy:
        return prefix + "Ejecute la verificación y corrección de formato de código para alinear los "
                        "archivos al estándar establecido.";
    case CiWindowsJobError::StaticAnalysisIssue:
        return prefix + "Revise y resuelva las incidencias de calidad, rendimiento o seguridad "
                        "señaladas por el análisis estático.";
    case CiWindowsJobError::UnsafePipelinePath:
        return prefix +
               "Especifique un directorio de compilación relativo y seguro dentro del proyecto (como "
               "build-ci-win-debug o _build), sin referencias externas ni rutas del sistema.";
    case CiWindowsJobError::TimeoutExceeded:
        return prefix + "Optimice la duración de las tareas o incremente el tiempo límite asignado a la "
                        "etapa afectada si es necesario.";
    case CiWindowsJobError::InvalidWorkflowSchema:
        return prefix + "Defina una lista válida de etapas con parámetros consistentes, tiempos límites "
                        "positivos y memoria dentro del presupuesto.";
    case CiWindowsJobError::DependencyOrderingViolation:
        return prefix + "Ordene las etapas respetando sus dependencias lógicas: configure antes de "
                        "compilar y compile antes de ejecutar pruebas.";
    case CiWindowsJobError::MemoryLimitExceeded:
        return prefix + "Ajuste el presupuesto de memoria del trabajo para que no supere el límite "
                        "máximo seguro permitido (4 GB).";
    }
    return prefix + "Consulte los registros locales para obtener detalles adicionales de la incidencia.";
}

} // namespace vigilio
