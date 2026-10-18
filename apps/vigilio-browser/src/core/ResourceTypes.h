#pragma once

#include <compare>
#include <cstdint>
#include <string_view>

namespace vigilio {

/// @brief Clasificación taxonómica de los recursos embebidos en el sistema QRC.
enum class ResourceCategory : uint8_t {
    Theme,          // Esquemas JSON de paleta y diseño (light, dark, midnight, forest, aurora, sunset)
    Wallpaper,      // Fondos de paisaje JPG para nueva pestaña (01 a 06)
    Branding,       // Identidad y logotipo oficial SVG
    DesignTokens,   // Tokens JSON de medidas, espaciado, radios y tipografía
    NavigationIcon, // Iconos SVG de barra de direcciones y navegación
    ToolIcon        // Iconos SVG de panel de herramientas, privacidad y utilidades
};

/// @brief Taxonomía exhaustiva de resultados de validación y causas de rechazo de recursos QRC.
enum class ResourceInclusionStatus : uint8_t {
    Valid,                   // Conforme y verificado
    InvalidAliasFormat,      // Alias malformado (traversal, caracteres ilegales, barras erróneas)
    ResourceNotFound,        // Alias ausente en el catálogo canónico
    CategoryMismatch,        // Discrepancia entre la ruta del alias y la categoría declarada
    PrefixMismatch,          // Archivo QRC utiliza prefijo distinto a /vigilio
    DuplicateAlias,          // Alias duplicado en el archivo de recursos
    MissingCriticalResource, // Recurso crítico para arranque ausente en el bundle
    ExcludedPathIncluded     // Inclusión indebida de rutas prohibidas (reference/, installer/, fonts/)
};

/// @brief Entidad de valor inmutable que encapsula los metadatos canónicos de un recurso Qt.
struct ResourceDescriptor {
    std::string_view alias{};             // e.g. "branding/vigilio-logo.svg"
    std::string_view qrcPath{};           // e.g. ":/vigilio/branding/vigilio-logo.svg"
    std::string_view physicalAssetPath{}; // e.g. "assets/branding/vigilio-logo.svg"
    ResourceCategory category{ResourceCategory::Branding};
    std::string_view mimeType{}; // "image/svg+xml"
    bool isCritical{false};      // true si es indispensable para inicializar UI básica

    constexpr bool operator==(const ResourceDescriptor& other) const noexcept = default;
    constexpr auto operator<=>(const ResourceDescriptor& other) const noexcept {
        return alias <=> other.alias;
    }
};

} // namespace vigilio
