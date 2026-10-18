#include "core/TestFixtureValidator.h"
#include <system_error>

namespace vigilio {

bool TestFixtureValidator::isWithinTempRoot(const std::filesystem::path& target,
                                            const std::filesystem::path& base) {
    if (target.empty() || base.empty()) {
        return false;
    }
    const std::string targetStr = target.string();
    if (targetStr.find('\0') != std::string::npos || targetStr.find("..") != std::string::npos) {
        return false;
    }
    for (char ch : targetStr) {
        if (static_cast<unsigned char>(ch) < 32 || ch == 127) {
            return false;
        }
    }

    std::error_code ec;
    const auto normTarget = std::filesystem::weakly_canonical(target, ec);
    if (ec) {
        return false;
    }
    const auto normBase = std::filesystem::weakly_canonical(base, ec);
    if (ec) {
        return false;
    }

    if (normTarget == normBase) {
        return false;
    }

    const auto rel = normTarget.lexically_relative(normBase);
    if (rel.empty() || rel == "." || rel.is_absolute()) {
        return false;
    }
    const std::string relStr = rel.string();
    if (relStr.starts_with("..") || relStr.find("..") != std::string::npos) {
        return false;
    }

    return true;
}

bool TestFixtureValidator::isSafeTestDirectory(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    const std::string pathStr = path.string();
    if (pathStr.find('\0') != std::string::npos || pathStr.find("..") != std::string::npos) {
        return false;
    }
    for (char ch : pathStr) {
        if (static_cast<unsigned char>(ch) < 32 || ch == 127) {
            return false;
        }
    }

    std::error_code ec;
    const auto tempRoot = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(), ec);
    if (ec) {
        return false;
    }
    const auto normPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return false;
    }

    if (normPath == tempRoot) {
        return false;
    }
    if (!isWithinTempRoot(normPath, tempRoot)) {
        return false;
    }

    const auto rel = normPath.lexically_relative(tempRoot);
    auto it = rel.begin();
    if (it == rel.end()) {
        return false;
    }
    const std::string topComponent = it->string();
    if (!topComponent.starts_with("vigilio_test_")) {
        return false;
    }

    return true;
}

FixtureError TestFixtureValidator::validateConfig(const TestEnvironmentConfig& config) {
    if (config.prefix.empty()) {
        return FixtureError::InvalidConfiguration;
    }
    if (config.prefix.find("..") != std::string::npos || config.prefix.find('/') != std::string::npos ||
        config.prefix.find('\\') != std::string::npos) {
        return FixtureError::UnsafePathDetected;
    }
    for (char ch : config.prefix) {
        if (static_cast<unsigned char>(ch) < 32 || ch == 127) {
            return FixtureError::UnsafePathDetected;
        }
    }

    constexpr std::size_t minQuota = 1024;
    constexpr std::size_t maxQuota = 500ULL * 1024 * 1024;
    if (config.maxTempDirSizeBytes < minQuota || config.maxTempDirSizeBytes > maxQuota) {
        return FixtureError::InvalidConfiguration;
    }

    if (config.cleanPolicy != FixtureCleanPolicy::AlwaysClean &&
        config.cleanPolicy != FixtureCleanPolicy::RetainOnFailure &&
        config.cleanPolicy != FixtureCleanPolicy::ManualCleanup) {
        return FixtureError::InvalidConfiguration;
    }

    return FixtureError::None;
}

std::string TestFixtureValidator::fixtureStateToString(TestEnvironmentState state) {
    switch (state) {
    case TestEnvironmentState::Uninitialized:
        return "Entorno de prueba no inicializado";
    case TestEnvironmentState::Configured:
        return "Configuración de prueba lista para activación";
    case TestEnvironmentState::Active:
        return "Entorno de prueba activo y aislado";
    case TestEnvironmentState::TornDown:
        return "Entorno de prueba desmontado y limpiado";
    case TestEnvironmentState::Corrupted:
        return "Error en el estado del entorno de prueba";
    }
    return "Estado de entorno desconocido";
}

std::string TestFixtureValidator::fixtureErrorToString(FixtureError error) {
    switch (error) {
    case FixtureError::None:
        return "Operación completada exitosamente";
    case FixtureError::DirectoryCreationFailed:
        return "No se pudo crear la carpeta temporal de prueba";
    case FixtureError::CleanupFailed:
        return "No se pudo limpiar completamente la carpeta temporal";
    case FixtureError::QuotaExceeded:
        return "El espacio en disco asignado para la prueba ha sido superado";
    case FixtureError::EnvironmentOverrideFailed:
        return "No se pudo configurar la variable de entorno temporal";
    case FixtureError::InvalidConfiguration:
        return "Configuración del entorno de prueba no válida";
    case FixtureError::DirectoryLocked:
        return "La carpeta temporal está bloqueada o en uso por otro proceso";
    case FixtureError::UnsafePathDetected:
        return "Ruta de destino no segura o fuera de la carpeta temporal autorizada";
    }
    return "Error de infraestructura desconocido";
}

} // namespace vigilio
