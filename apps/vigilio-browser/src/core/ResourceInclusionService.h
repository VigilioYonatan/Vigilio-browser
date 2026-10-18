#pragma once

#include "core/ResourceQrcCatalog.h"
#include "core/ResourceTypes.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Clasificación exhaustiva de errores en operaciones de lectura y provisión de recursos.
enum class ResourceReadError : uint8_t {
    None,             // Operación exitosa
    NotFound,         // Recurso no encontrado en el catálogo ni en el bundle
    AccessDenied,     // Acceso denegado (ruta prohibida o intento de Directory Traversal)
    InvalidFormat,    // Formato sintáctico o extensión no válida para el tipo de recurso
    CorruptedContent, // Contenido dañado, truncado o inconsistente
    Cancelled,        // Operación cancelada explícitamente vía CancellationToken
    IOError           // Fallo general de entrada/salida
};

/// @brief Estructura inmutable con semántica de valor para resultados de lectura de recursos.
template <typename T> struct ResourceReadResult {
    ResourceReadError error{ResourceReadError::None};
    std::optional<T> data{std::nullopt};
    std::string errorMessage{};
    bool hasFallback{false};

    [[nodiscard]] constexpr bool isSuccess() const noexcept {
        return error == ResourceReadError::None && data.has_value();
    }
};

/// @brief Estados de diagnóstico del sistema de recursos durante el arranque.
enum class ResourceStartupStatus : uint8_t {
    Ready,            // Todos los recursos críticos están íntegros y accesibles
    Degraded,         // Recursos críticos disponibles, pero se detectaron advertencias menores
    MissingCritical,  // Uno o más recursos críticos indispensables no están disponibles
    CatalogCorruption // Inconsistencia estructural grave en el archivo de recursos o prefijo
};

/// @brief Reporte consolidado de la pre-validación de recursos en arranque.
struct ResourceStartupReport {
    ResourceStartupStatus status{ResourceStartupStatus::Ready};
    size_t totalVerified{0};
    size_t criticalVerified{0};
    std::vector<std::string> missingCritical{};
    std::vector<std::string> warnings{};
    std::string diagnosticMessage{};
    std::string remediationAction{};
};

/// @brief Token de cancelación cooperativo y ligero para operaciones en segundo plano.
class CancellationToken {
  public:
    explicit CancellationToken(bool initial = false) noexcept : cancelled_(initial) {
    }

    void requestCancellation() noexcept {
        cancelled_.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

  private:
    std::atomic<bool> cancelled_{false};
};

/// @brief Servicio propietario desacoplado para provisión, validación y recuperación de recursos.
class ResourceInclusionService {
  public:
    // Identificadores y constantes canónicas de fallback
    static constexpr std::string_view kDefaultThemeAlias = "themes/light.json";
    static constexpr std::string_view kFallbackIconAlias = "icons/tools/tools.svg";

    [[nodiscard]] static constexpr std::string_view defaultThemeAlias() noexcept {
        return kDefaultThemeAlias;
    }

    [[nodiscard]] static constexpr std::string_view fallbackIconAlias() noexcept {
        return kFallbackIconAlias;
    }

    // Pre-validación en tiempo de arranque (Startup Bootstrap)
    [[nodiscard]] static ResourceStartupReport
    validateStartupResources(std::string_view qrcContent = {}) noexcept;

    // Carga síncrona de recursos textuales con soporte de fallback
    [[nodiscard]] static ResourceReadResult<std::string>
    loadTextResource(std::string_view alias, bool allowFallback = true) noexcept;

    // Carga de recursos binarios con soporte de cancelación cooperativa
    [[nodiscard]] static ResourceReadResult<std::vector<uint8_t>>
    loadBinaryResource(std::string_view alias, const CancellationToken* cancelToken = nullptr) noexcept;

    // Resolución segura de iconos con fallback garantizado
    [[nodiscard]] static std::string resolveIconWithFallback(std::string_view alias) noexcept;

    // Resolución segura de temas con fallback garantizado
    [[nodiscard]] static std::string resolveThemeWithFallback(std::string_view themeAlias) noexcept;

    // Formateadores estáticos para observabilidad y logging
    [[nodiscard]] static std::string_view errorName(ResourceReadError error) noexcept;
    [[nodiscard]] static std::string_view startupStatusName(ResourceStartupStatus status) noexcept;
};

} // namespace vigilio
