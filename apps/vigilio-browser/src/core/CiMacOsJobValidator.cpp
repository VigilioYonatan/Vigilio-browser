#include "core/CiMacOsJobValidator.h"
#include "core/CiMacOsJobConfiguration.h"

#include <cctype>
#include <vector>

namespace vigilio {

CiMacOsJobError CiMacOsJobValidator::validatePlan(const CiMacOsJobPlan& plan) {
    if (plan.steps.empty() || plan.presetName.empty()) {
        return CiMacOsJobError::InvalidWorkflowSchema;
    }
    if (plan.totalTimeoutSeconds == 0 || plan.totalTimeoutSeconds > 3600) {
        return CiMacOsJobError::InvalidWorkflowSchema;
    }
    if (plan.maxMemoryBytes == 0 || plan.maxMemoryBytes > 8589934592ULL) {
        return CiMacOsJobError::MemoryLimitExceeded;
    }
    if (!isSafeBuildPath(plan.buildDir)) {
        return CiMacOsJobError::UnsafePipelinePath;
    }
    for (const auto& stepDef : plan.steps) {
        if (stepDef.name.empty() || stepDef.command.empty()) {
            return CiMacOsJobError::InvalidWorkflowSchema;
        }
        if (stepDef.options.timeoutMs == 0) {
            return CiMacOsJobError::InvalidWorkflowSchema;
        }
    }

    std::vector<CiMacOsPipelineStep> stepSeq;
    stepSeq.reserve(plan.steps.size());
    for (const auto& stepDef : plan.steps) {
        stepSeq.push_back(stepDef.step);
    }
    return validateStepSequence(stepSeq);
}

CiMacOsJobError
CiMacOsJobValidator::validateStepSequence(const std::vector<CiMacOsPipelineStep>& steps) {
    if (steps.empty()) {
        return CiMacOsJobError::InvalidWorkflowSchema;
    }

    int envCheckIdx = -1;
    int formatIdx = -1;
    int staticIdx = -1;
    int configIdx = -1;
    int buildIdx = -1;
    int testIdx = -1;
    int sanitizerIdx = -1;
    int bundleIdx = -1;

    for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
        const auto step = steps[static_cast<std::size_t>(i)];
        switch (step) {
        case CiMacOsPipelineStep::EnvironmentCheck:
            if (envCheckIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            envCheckIdx = i;
            break;
        case CiMacOsPipelineStep::FormatVerification:
            if (formatIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            formatIdx = i;
            break;
        case CiMacOsPipelineStep::StaticAnalysis:
            if (staticIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            staticIdx = i;
            break;
        case CiMacOsPipelineStep::ConfigurePreset:
            if (configIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            configIdx = i;
            break;
        case CiMacOsPipelineStep::BuildCompilation:
            if (buildIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            buildIdx = i;
            break;
        case CiMacOsPipelineStep::TestSuiteExecution:
            if (testIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            testIdx = i;
            break;
        case CiMacOsPipelineStep::SanitizerVerification:
            if (sanitizerIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            sanitizerIdx = i;
            break;
        case CiMacOsPipelineStep::BundleVerification:
            if (bundleIdx != -1) {
                return CiMacOsJobError::DependencyOrderingViolation;
            }
            bundleIdx = i;
            break;
        }
    }

    // Comprobaciones estáticas y de entorno previas a compilación
    if (envCheckIdx != -1 && buildIdx != -1 && envCheckIdx > buildIdx) {
        return CiMacOsJobError::DependencyOrderingViolation;
    }
    if (formatIdx != -1 && buildIdx != -1 && formatIdx > buildIdx) {
        return CiMacOsJobError::DependencyOrderingViolation;
    }
    if (staticIdx != -1 && buildIdx != -1 && staticIdx > buildIdx) {
        return CiMacOsJobError::DependencyOrderingViolation;
    }

    // Compilación requiere configuración previa
    if (buildIdx != -1) {
        if (configIdx == -1 || configIdx > buildIdx) {
            return CiMacOsJobError::DependencyOrderingViolation;
        }
    }

    // Pruebas requieren compilación previa
    if (testIdx != -1) {
        if (buildIdx == -1 || buildIdx > testIdx) {
            return CiMacOsJobError::DependencyOrderingViolation;
        }
    }

    // Sanitizadores requieren compilación previa
    if (sanitizerIdx != -1) {
        if (buildIdx == -1 || buildIdx > sanitizerIdx) {
            return CiMacOsJobError::DependencyOrderingViolation;
        }
    }

    // Verificación de bundle requiere compilación y pruebas (si existen pruebas)
    if (bundleIdx != -1) {
        if (buildIdx == -1 || buildIdx > bundleIdx) {
            return CiMacOsJobError::DependencyOrderingViolation;
        }
        if (testIdx != -1 && bundleIdx < testIdx) {
            return CiMacOsJobError::DependencyOrderingViolation;
        }
    }

    return CiMacOsJobError::None;
}

CiMacOsJobError CiMacOsJobValidator::validateEnvironment(const CiMacOsEnvironment& env) {
    if (env.architecture != CiMacOsArchitecture::ARM64 &&
        env.architecture != CiMacOsArchitecture::X86_64 &&
        env.architecture != CiMacOsArchitecture::Universal) {
        return CiMacOsJobError::ToolchainMissing;
    }

    if (!CiMacOsJobConfiguration::isSupportedMacOsVersion(env.osVersion)) {
        return CiMacOsJobError::ToolchainMissing;
    }

    if (env.compiler == CiMacOsCompiler::AppleClang) {
        if (env.compilerMajorVersion < 15) {
            return CiMacOsJobError::ToolchainMissing;
        }
    } else if (env.compiler == CiMacOsCompiler::HomebrewClang) {
        if (env.compilerMajorVersion < 18) {
            return CiMacOsJobError::ToolchainMissing;
        }
    } else {
        return CiMacOsJobError::ToolchainMissing;
    }

    // Comprobación de herramientas de línea de comandos de Xcode
    if (!env.xcodeToolsInstalled) {
        return CiMacOsJobError::XcodeToolsMissing;
    }

    // Validación de macOS SDK
    const std::string& sdk = env.macOsSdkVersion.empty() ? env.sdkVersion : env.macOsSdkVersion;
    if (sdk.empty()) {
        return CiMacOsJobError::MacOsSdkMissing;
    }
    std::size_t firstDigit = sdk.find_first_of("0123456789");
    if (firstDigit == std::string::npos) {
        return CiMacOsJobError::MacOsSdkMissing;
    }
    std::size_t endDigit = sdk.find_first_not_of("0123456789", firstDigit);
    std::string majorStr = (endDigit == std::string::npos)
                               ? sdk.substr(firstDigit)
                               : sdk.substr(firstDigit, endDigit - firstDigit);
    try {
        unsigned long sdkMajor = std::stoul(majorStr);
        if (sdkMajor < 13) {
            return CiMacOsJobError::MacOsSdkMissing;
        }
    } catch (...) {
        return CiMacOsJobError::MacOsSdkMissing;
    }

    // Validación de CMake
    if (env.cmakeMajorVersion < 3 || (env.cmakeMajorVersion == 3 && env.cmakeMinorVersion < 28)) {
        return CiMacOsJobError::ToolchainMissing;
    }

    // Validación de Ninja
    if (env.ninjaMajorVersion < 1 || (env.ninjaMajorVersion == 1 && env.ninjaMinorVersion < 12)) {
        return CiMacOsJobError::ToolchainMissing;
    }

    // Validación de memoria (mínimo 2 GB)
    if (env.totalMemoryBytes < 2147483648ULL) {
        return CiMacOsJobError::ToolchainMissing;
    }

    // Validación de núcleos (mínimo 1 núcleo)
    if (env.availableCores < 1) {
        return CiMacOsJobError::ToolchainMissing;
    }

    return CiMacOsJobError::None;
}

bool CiMacOsJobValidator::isSafeBuildPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    const std::string s = path.string();
    if (s.empty()) {
        return false;
    }

    // Rechazo de Directory Traversal y caracteres nulos o no imprimibles
    if (s.find("..") != std::string::npos || s.find('\0') != std::string::npos) {
        return false;
    }
    for (char ch : s) {
        auto c = static_cast<unsigned char>(ch);
        if (c < 32 || c == 127) {
            return false;
        }
    }

    // Rechazo de rutas absolutas POSIX, directorio personal y rutas Windows/UNC
    if (path.is_absolute() || s.front() == '/' || s.front() == '\\' || s.front() == '~') {
        return false;
    }
    if (s.size() >= 2 && std::isalpha(static_cast<unsigned char>(s[0])) && s[1] == ':') {
        return false;
    }
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

    // Manejo de prefijo ${sourceDir}/
    constexpr std::string_view sourceDirPrefix = "${sourceDir}/";
    std::string_view target = normalized;
    if (target.rfind(sourceDirPrefix, 0) == 0) {
        target = target.substr(sourceDirPrefix.size());
    }

    // División en segmentos
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

    // Verificación de segmentos individuales y artefactos conflictivos de macOS
    for (const auto& segment : segments) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            return false;
        }
        // Rechazo de metadatos de macOS (.DS_Store) y bifurcaciones AppleDouble (._*)
        if (segment == ".DS_Store" || segment.rfind("._", 0) == 0) {
            return false;
        }
    }

    // Confinamiento: primer segmento debe pertenecer a directorios autorizados
    const std::string& first = segments.front();
    if (first.rfind("build-ci-macos-", 0) == 0 || first.rfind("build-ci-", 0) == 0 ||
        first.rfind("_build", 0) == 0 || first.rfind("build-", 0) == 0) {
        return true;
    }

    return false;
}

std::string CiMacOsJobValidator::jobStatusToString(CiMacOsJobStatus status) {
    switch (status) {
    case CiMacOsJobStatus::Pending:
        return "Pendiente de ejecución";
    case CiMacOsJobStatus::Running:
        return "En ejecución";
    case CiMacOsJobStatus::Passed:
        return "Completado correctamente";
    case CiMacOsJobStatus::Failed:
        return "Finalizado con incidencias";
    case CiMacOsJobStatus::Cancelled:
        return "Cancelado por el usuario";
    case CiMacOsJobStatus::TimedOut:
        return "Tiempo límite excedido";
    }
    return "Estado desconocido";
}

std::string CiMacOsJobValidator::jobErrorToString(CiMacOsJobError error) {
    switch (error) {
    case CiMacOsJobError::None:
        return "Sin incidencias";
    case CiMacOsJobError::ToolchainMissing:
        return "Herramientas de compilación no encontradas o incompatibles en el equipo";
    case CiMacOsJobError::MacOsSdkMissing:
        return "Paquete de desarrollo de macOS (SDK) no encontrado en el sistema";
    case CiMacOsJobError::XcodeToolsMissing:
        return "Herramientas de línea de comandos de Xcode no instaladas en el sistema";
    case CiMacOsJobError::ConfigurationFailed:
        return "Error en la configuración de compilación";
    case CiMacOsJobError::CompilationFailed:
        return "Error en la compilación del código";
    case CiMacOsJobError::TestExecutionFailed:
        return "Una o más pruebas automáticas fallaron";
    case CiMacOsJobError::FormatDiscrepancy:
        return "Discrepancias en el formato de código fuente";
    case CiMacOsJobError::StaticAnalysisIssue:
        return "Incidencias detectadas en el análisis estático de código";
    case CiMacOsJobError::UnsafePipelinePath:
        return "Ruta de compilación no segura o fuera del directorio del proyecto";
    case CiMacOsJobError::TimeoutExceeded:
        return "Tiempo límite de ejecución excedido";
    case CiMacOsJobError::InvalidWorkflowSchema:
        return "Esquema de etapas de trabajo no válido";
    case CiMacOsJobError::DependencyOrderingViolation:
        return "Orden incorrecto en las etapas de compilación y verificación";
    case CiMacOsJobError::MemoryLimitExceeded:
        return "Límite de memoria asignada excedido para la tarea";
    case CiMacOsJobError::BundleVerificationFailed:
        return "Verificación de la estructura del paquete de la aplicación fallida";
    }
    return "Error no clasificado";
}

std::string CiMacOsJobValidator::stepToString(CiMacOsPipelineStep step) {
    switch (step) {
    case CiMacOsPipelineStep::EnvironmentCheck:
        return "Verificación del entorno de compilación en macOS";
    case CiMacOsPipelineStep::FormatVerification:
        return "Comprobación de formato de código";
    case CiMacOsPipelineStep::StaticAnalysis:
        return "Análisis estático de calidad de código";
    case CiMacOsPipelineStep::ConfigurePreset:
        return "Configuración de compilación del sistema";
    case CiMacOsPipelineStep::BuildCompilation:
        return "Compilación del proyecto";
    case CiMacOsPipelineStep::TestSuiteExecution:
        return "Ejecución de pruebas automatizadas";
    case CiMacOsPipelineStep::SanitizerVerification:
        return "Verificación de sanitizadores de memoria";
    case CiMacOsPipelineStep::BundleVerification:
        return "Verificación del paquete de la aplicación";
    }
    return "Etapa no reconocida";
}

std::string CiMacOsJobValidator::stepStatusToString(CiMacOsStepStatus status) {
    switch (status) {
    case CiMacOsStepStatus::NotStarted:
        return "No iniciado";
    case CiMacOsStepStatus::InProgress:
        return "En progreso";
    case CiMacOsStepStatus::Success:
        return "Completado con éxito";
    case CiMacOsStepStatus::Failure:
        return "Error en la etapa";
    case CiMacOsStepStatus::Omitted:
        return "Omitido";
    }
    return "Estado de etapa desconocido";
}

std::string CiMacOsJobValidator::architectureToString(CiMacOsArchitecture arch) {
    switch (arch) {
    case CiMacOsArchitecture::ARM64:
        return "arm64";
    case CiMacOsArchitecture::X86_64:
        return "x86_64";
    case CiMacOsArchitecture::Universal:
        return "universal";
    }
    return "desconocida";
}

std::string CiMacOsJobValidator::compilerToString(CiMacOsCompiler compiler) {
    switch (compiler) {
    case CiMacOsCompiler::AppleClang:
        return "AppleClang";
    case CiMacOsCompiler::HomebrewClang:
        return "HomebrewClang";
    }
    return "Desconocido";
}

std::string CiMacOsJobValidator::generateRemediationHint(CiMacOsJobError error,
                                                         std::string_view stepName) {
    std::string prefix;
    if (!stepName.empty()) {
        prefix = "En la etapa '" + std::string(stepName) + "': ";
    }
    switch (error) {
    case CiMacOsJobError::None:
        return "No se requiere ninguna acción de remediación.";
    case CiMacOsJobError::ToolchainMissing:
        return prefix +
               "Instale o actualice las herramientas de compilación requeridas para macOS (Apple "
               "Clang 15 o Homebrew Clang 18, Ninja 1.12 y CMake 3.28 o superior).";
    case CiMacOsJobError::MacOsSdkMissing:
        return prefix + "Instale el paquete de desarrollo de macOS (macOS SDK versión 13.0 "
                        "o superior) mediante Xcode o las herramientas de línea de comandos.";
    case CiMacOsJobError::XcodeToolsMissing:
        return prefix + "Instale las herramientas de línea de comandos de Xcode ejecutando "
                        "'xcode-select --install' en la terminal.";
    case CiMacOsJobError::ConfigurationFailed:
        return prefix + "Revise los archivos de configuración y asegúrese de que las dependencias y "
                        "opciones del sistema sean correctas.";
    case CiMacOsJobError::CompilationFailed:
        return prefix + "Corrija los errores sintácticos o advertencias de compilación reportados en el "
                        "código fuente.";
    case CiMacOsJobError::TestExecutionFailed:
        return prefix + "Examine las aserciones fallidas en las pruebas automatizadas y verifique las "
                        "precondiciones del entorno.";
    case CiMacOsJobError::FormatDiscrepancy:
        return prefix + "Ejecute la verificación y corrección de formato de código para alinear los "
                        "archivos al estándar establecido.";
    case CiMacOsJobError::StaticAnalysisIssue:
        return prefix + "Revise y resuelva las incidencias de calidad, rendimiento o seguridad "
                        "señaladas por el análisis estático.";
    case CiMacOsJobError::UnsafePipelinePath:
        return prefix +
               "Especifique un directorio de compilación relativo y seguro dentro del proyecto (como "
               "build-ci-macos-debug o _build), sin referencias externas ni rutas del sistema.";
    case CiMacOsJobError::TimeoutExceeded:
        return prefix + "Optimice la duración de las tareas o incremente el tiempo límite asignado a la "
                        "etapa afectada si es necesario.";
    case CiMacOsJobError::InvalidWorkflowSchema:
        return prefix + "Defina una lista válida de etapas con parámetros consistentes, tiempos límites "
                        "positivos y memoria dentro del presupuesto.";
    case CiMacOsJobError::DependencyOrderingViolation:
        return prefix + "Ordene las etapas respetando sus dependencias lógicas: configure antes de "
                        "compilar y compile antes de ejecutar pruebas.";
    case CiMacOsJobError::MemoryLimitExceeded:
        return prefix + "Ajuste el presupuesto de memoria del trabajo para que no supere el límite "
                        "máximo seguro permitido (4 GB).";
    case CiMacOsJobError::BundleVerificationFailed:
        return prefix + "Verifique que la estructura del paquete de la aplicación contenga los "
                        "directorios y archivos requeridos (Contents/MacOS y Contents/Info.plist).";
    }
    return prefix + "Consulte los registros locales para obtener detalles adicionales de la incidencia.";
}

} // namespace vigilio
