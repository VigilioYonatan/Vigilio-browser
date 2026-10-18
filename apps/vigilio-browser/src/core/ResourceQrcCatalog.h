#pragma once

#include "core/ResourceTypes.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Catálogo canónico inmutable y validador de recursos QRC para Vigilio Browser.
class ResourceQrcCatalog {
  public:
    static constexpr std::string_view kCanonicalPrefix = ":/vigilio";
    static constexpr size_t kExpectedResourceCount = 35;

    // Métodos de consulta básica
    [[nodiscard]] static constexpr std::string_view canonicalPrefix() noexcept {
        return kCanonicalPrefix;
    }

    [[nodiscard]] static constexpr size_t totalResourceCount() noexcept {
        return kExpectedResourceCount;
    }

    [[nodiscard]] static bool contains(std::string_view alias) noexcept;
    [[nodiscard]] static std::optional<ResourceDescriptor> find(std::string_view alias) noexcept;
    [[nodiscard]] static std::string resolveQrcPath(std::string_view alias);

    // Validación sintáctica y de seguridad
    [[nodiscard]] static bool isValidAlias(std::string_view alias) noexcept;
    [[nodiscard]] static bool isExcludedPath(std::string_view path) noexcept;

    // Consultas estructuradas
    [[nodiscard]] static std::vector<ResourceDescriptor> allResources();
    [[nodiscard]] static std::vector<ResourceDescriptor> resourcesByCategory(ResourceCategory category);
    [[nodiscard]] static std::vector<ResourceDescriptor> criticalResources();

    // Validación estática de archivo QRC XML
    [[nodiscard]] static ResourceInclusionStatus validateQrcConsistency(std::string_view qrcXmlContent);

    // Nombres legibles para observabilidad
    [[nodiscard]] static std::string_view categoryName(ResourceCategory category) noexcept;
    [[nodiscard]] static std::string_view statusName(ResourceInclusionStatus status) noexcept;
};

} // namespace vigilio
