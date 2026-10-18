#include "core/ResourceQrcCatalog.h"

#include <algorithm>
#include <array>

namespace vigilio {

namespace {

constexpr std::array<ResourceDescriptor, 35> kApprovedResources = {
    // 6 Themes
    ResourceDescriptor{"themes/light.json", ":/vigilio/themes/light.json", "assets/themes/light.json",
                       ResourceCategory::Theme, "application/json", true},
    ResourceDescriptor{"themes/dark.json", ":/vigilio/themes/dark.json", "assets/themes/dark.json",
                       ResourceCategory::Theme, "application/json", true},
    ResourceDescriptor{"themes/midnight.json", ":/vigilio/themes/midnight.json",
                       "assets/themes/midnight.json", ResourceCategory::Theme, "application/json",
                       false},
    ResourceDescriptor{"themes/forest.json", ":/vigilio/themes/forest.json", "assets/themes/forest.json",
                       ResourceCategory::Theme, "application/json", false},
    ResourceDescriptor{"themes/aurora.json", ":/vigilio/themes/aurora.json", "assets/themes/aurora.json",
                       ResourceCategory::Theme, "application/json", false},
    ResourceDescriptor{"themes/sunset.json", ":/vigilio/themes/sunset.json", "assets/themes/sunset.json",
                       ResourceCategory::Theme, "application/json", false},

    // 6 Wallpapers
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-01.jpg", ":/vigilio/wallpapers/vigilio-landscape-01.jpg",
        "assets/wallpapers/vigilio-landscape-01.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-02.jpg", ":/vigilio/wallpapers/vigilio-landscape-02.jpg",
        "assets/wallpapers/vigilio-landscape-02.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-03.jpg", ":/vigilio/wallpapers/vigilio-landscape-03.jpg",
        "assets/wallpapers/vigilio-landscape-03.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-04.jpg", ":/vigilio/wallpapers/vigilio-landscape-04.jpg",
        "assets/wallpapers/vigilio-landscape-04.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-05.jpg", ":/vigilio/wallpapers/vigilio-landscape-05.jpg",
        "assets/wallpapers/vigilio-landscape-05.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},
    ResourceDescriptor{
        "wallpapers/vigilio-landscape-06.jpg", ":/vigilio/wallpapers/vigilio-landscape-06.jpg",
        "assets/wallpapers/vigilio-landscape-06.jpg", ResourceCategory::Wallpaper, "image/jpeg", false},

    // 1 Branding
    ResourceDescriptor{"branding/vigilio-logo.svg", ":/vigilio/branding/vigilio-logo.svg",
                       "assets/branding/vigilio-logo.svg", ResourceCategory::Branding, "image/svg+xml",
                       true},

    // 1 DesignTokens
    ResourceDescriptor{"design-tokens/tokens.json", ":/vigilio/design-tokens/tokens.json",
                       "assets/design-tokens/tokens.json", ResourceCategory::DesignTokens,
                       "application/json", true},

    // 10 Navigation Icons
    ResourceDescriptor{"icons/navigation/search.svg", ":/vigilio/icons/navigation/search.svg",
                       "assets/icons/navigation/search.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", true},
    ResourceDescriptor{"icons/navigation/back.svg", ":/vigilio/icons/navigation/back.svg",
                       "assets/icons/navigation/back.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", true},
    ResourceDescriptor{"icons/navigation/forward.svg", ":/vigilio/icons/navigation/forward.svg",
                       "assets/icons/navigation/forward.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", true},
    ResourceDescriptor{"icons/navigation/reload.svg", ":/vigilio/icons/navigation/reload.svg",
                       "assets/icons/navigation/reload.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", true},
    ResourceDescriptor{"icons/navigation/home.svg", ":/vigilio/icons/navigation/home.svg",
                       "assets/icons/navigation/home.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},
    ResourceDescriptor{"icons/navigation/bookmarks.svg", ":/vigilio/icons/navigation/bookmarks.svg",
                       "assets/icons/navigation/bookmarks.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},
    ResourceDescriptor{"icons/navigation/history.svg", ":/vigilio/icons/navigation/history.svg",
                       "assets/icons/navigation/history.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},
    ResourceDescriptor{"icons/navigation/download.svg", ":/vigilio/icons/navigation/download.svg",
                       "assets/icons/navigation/download.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},
    ResourceDescriptor{"icons/navigation/settings.svg", ":/vigilio/icons/navigation/settings.svg",
                       "assets/icons/navigation/settings.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},
    ResourceDescriptor{"icons/navigation/tabs.svg", ":/vigilio/icons/navigation/tabs.svg",
                       "assets/icons/navigation/tabs.svg", ResourceCategory::NavigationIcon,
                       "image/svg+xml", false},

    // 11 Tool Icons
    ResourceDescriptor{"icons/tools/note.svg", ":/vigilio/icons/tools/note.svg",
                       "assets/icons/tools/note.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/mcp.svg", ":/vigilio/icons/tools/mcp.svg",
                       "assets/icons/tools/mcp.svg", ResourceCategory::ToolIcon, "image/svg+xml", false},
    ResourceDescriptor{"icons/tools/reader.svg", ":/vigilio/icons/tools/reader.svg",
                       "assets/icons/tools/reader.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/camera.svg", ":/vigilio/icons/tools/camera.svg",
                       "assets/icons/tools/camera.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/senior.svg", ":/vigilio/icons/tools/senior.svg",
                       "assets/icons/tools/senior.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/tools.svg", ":/vigilio/icons/tools/tools.svg",
                       "assets/icons/tools/tools.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/memory.svg", ":/vigilio/icons/tools/memory.svg",
                       "assets/icons/tools/memory.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/extensions.svg", ":/vigilio/icons/tools/extensions.svg",
                       "assets/icons/tools/extensions.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/split.svg", ":/vigilio/icons/tools/split.svg",
                       "assets/icons/tools/split.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
    ResourceDescriptor{"icons/tools/qr.svg", ":/vigilio/icons/tools/qr.svg", "assets/icons/tools/qr.svg",
                       ResourceCategory::ToolIcon, "image/svg+xml", false},
    ResourceDescriptor{"icons/tools/shield.svg", ":/vigilio/icons/tools/shield.svg",
                       "assets/icons/tools/shield.svg", ResourceCategory::ToolIcon, "image/svg+xml",
                       false},
};

static_assert(kApprovedResources.size() == ResourceQrcCatalog::kExpectedResourceCount,
              "kApprovedResources size must match kExpectedResourceCount (35)");

} // namespace

bool ResourceQrcCatalog::contains(std::string_view alias) noexcept {
    return find(alias).has_value();
}

std::optional<ResourceDescriptor> ResourceQrcCatalog::find(std::string_view alias) noexcept {
    for (const auto& resource : kApprovedResources) {
        if (resource.alias == alias) {
            return resource;
        }
    }
    return std::nullopt;
}

std::string ResourceQrcCatalog::resolveQrcPath(std::string_view alias) {
    if (alias.starts_with('/')) {
        return std::string(kCanonicalPrefix) + std::string(alias);
    }
    return std::string(kCanonicalPrefix) + "/" + std::string(alias);
}

bool ResourceQrcCatalog::isValidAlias(std::string_view alias) noexcept {
    if (alias.empty()) {
        return false;
    }
    if (alias.front() == '/' || alias.back() == '/') {
        return false;
    }
    if (alias.find("//") != std::string_view::npos) {
        return false;
    }
    if (alias.find("..") != std::string_view::npos) {
        return false;
    }
    for (char c : alias) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '/' ||
              c == '-')) {
            return false;
        }
    }
    if (!alias.ends_with(".svg") && !alias.ends_with(".json") && !alias.ends_with(".jpg") &&
        !alias.ends_with(".jpeg")) {
        return false;
    }
    return true;
}

bool ResourceQrcCatalog::isExcludedPath(std::string_view path) noexcept {
    if (path.empty()) {
        return false;
    }
    if (path.find("reference/") != std::string_view::npos ||
        path.find("installer/") != std::string_view::npos ||
        path.find("fonts/") != std::string_view::npos) {
        return true;
    }
    if (path.starts_with("/usr/") || path.starts_with("/tmp/") || path.starts_with("C:\\") ||
        path.starts_with("c:\\")) {
        return true;
    }
    return false;
}

std::vector<ResourceDescriptor> ResourceQrcCatalog::allResources() {
    return {kApprovedResources.begin(), kApprovedResources.end()};
}

std::vector<ResourceDescriptor> ResourceQrcCatalog::resourcesByCategory(ResourceCategory category) {
    std::vector<ResourceDescriptor> result;
    result.reserve(kApprovedResources.size());
    for (const auto& resource : kApprovedResources) {
        if (resource.category == category) {
            result.push_back(resource);
        }
    }
    return result;
}

std::vector<ResourceDescriptor> ResourceQrcCatalog::criticalResources() {
    std::vector<ResourceDescriptor> result;
    result.reserve(8);
    for (const auto& resource : kApprovedResources) {
        if (resource.isCritical) {
            result.push_back(resource);
        }
    }
    return result;
}

ResourceInclusionStatus ResourceQrcCatalog::validateQrcConsistency(std::string_view qrcXmlContent) {
    auto qresPos = qrcXmlContent.find("<qresource");
    if (qresPos == std::string_view::npos) {
        return ResourceInclusionStatus::PrefixMismatch;
    }
    auto tagEnd = qrcXmlContent.find('>', qresPos);
    if (tagEnd == std::string_view::npos) {
        return ResourceInclusionStatus::PrefixMismatch;
    }
    std::string_view qresTag = qrcXmlContent.substr(qresPos, tagEnd - qresPos);
    auto prefixPos = qresTag.find("prefix=");
    if (prefixPos == std::string_view::npos) {
        return ResourceInclusionStatus::PrefixMismatch;
    }
    size_t valStart = prefixPos + 7;
    if (valStart >= qresTag.size()) {
        return ResourceInclusionStatus::PrefixMismatch;
    }
    char quote = qresTag[valStart];
    std::string_view prefixVal;
    if (quote == '"' || quote == '\'') {
        size_t valEnd = qresTag.find(quote, valStart + 1);
        if (valEnd == std::string_view::npos) {
            return ResourceInclusionStatus::PrefixMismatch;
        }
        prefixVal = qresTag.substr(valStart + 1, valEnd - valStart - 1);
    } else {
        size_t valEnd = qresTag.find_first_of(" \t\r\n/>", valStart);
        prefixVal = qresTag.substr(valStart, valEnd - valStart);
    }
    if (prefixVal != "/vigilio") {
        return ResourceInclusionStatus::PrefixMismatch;
    }

    size_t currentPos = tagEnd + 1;
    std::vector<std::string> seenAliases;

    while (true) {
        auto filePos = qrcXmlContent.find("<file", currentPos);
        if (filePos == std::string_view::npos) {
            break;
        }
        auto fileTagEnd = qrcXmlContent.find('>', filePos);
        if (fileTagEnd == std::string_view::npos) {
            break;
        }
        auto endFilePos = qrcXmlContent.find("</file>", fileTagEnd);
        if (endFilePos == std::string_view::npos) {
            break;
        }

        std::string_view fileTag = qrcXmlContent.substr(filePos, fileTagEnd - filePos);
        std::string_view fileBody = qrcXmlContent.substr(fileTagEnd + 1, endFilePos - fileTagEnd - 1);
        currentPos = endFilePos + 7;

        while (!fileBody.empty() && (fileBody.front() == ' ' || fileBody.front() == '\t' ||
                                     fileBody.front() == '\r' || fileBody.front() == '\n')) {
            fileBody.remove_prefix(1);
        }
        while (!fileBody.empty() && (fileBody.back() == ' ' || fileBody.back() == '\t' ||
                                     fileBody.back() == '\r' || fileBody.back() == '\n')) {
            fileBody.remove_suffix(1);
        }

        std::string_view alias = fileBody;
        auto aliasPos = fileTag.find("alias=");
        if (aliasPos != std::string_view::npos) {
            size_t aValStart = aliasPos + 6;
            if (aValStart < fileTag.size()) {
                char aQuote = fileTag[aValStart];
                if (aQuote == '"' || aQuote == '\'') {
                    size_t aValEnd = fileTag.find(aQuote, aValStart + 1);
                    if (aValEnd != std::string_view::npos) {
                        alias = fileTag.substr(aValStart + 1, aValEnd - aValStart - 1);
                    }
                } else {
                    size_t aValEnd = fileTag.find_first_of(" \t\r\n/>", aValStart);
                    alias = fileTag.substr(aValStart, aValEnd - aValStart);
                }
            }
        }

        if (isExcludedPath(alias) || isExcludedPath(fileBody)) {
            return ResourceInclusionStatus::ExcludedPathIncluded;
        }

        if (!isValidAlias(alias)) {
            return ResourceInclusionStatus::InvalidAliasFormat;
        }

        for (const auto& seen : seenAliases) {
            if (seen == alias) {
                return ResourceInclusionStatus::DuplicateAlias;
            }
        }
        seenAliases.emplace_back(alias);

        auto descriptor = find(alias);
        if (!descriptor.has_value()) {
            return ResourceInclusionStatus::ResourceNotFound;
        }

        bool categoryMatches = false;
        switch (descriptor->category) {
        case ResourceCategory::Theme:
            categoryMatches = alias.starts_with("themes/") && alias.ends_with(".json");
            break;
        case ResourceCategory::Wallpaper:
            categoryMatches = alias.starts_with("wallpapers/") &&
                              (alias.ends_with(".jpg") || alias.ends_with(".jpeg"));
            break;
        case ResourceCategory::Branding:
            categoryMatches = alias.starts_with("branding/") && alias.ends_with(".svg");
            break;
        case ResourceCategory::DesignTokens:
            categoryMatches = alias.starts_with("design-tokens/") && alias.ends_with(".json");
            break;
        case ResourceCategory::NavigationIcon:
            categoryMatches = alias.starts_with("icons/navigation/") && alias.ends_with(".svg");
            break;
        case ResourceCategory::ToolIcon:
            categoryMatches = alias.starts_with("icons/tools/") && alias.ends_with(".svg");
            break;
        }
        if (!categoryMatches) {
            return ResourceInclusionStatus::CategoryMismatch;
        }
    }

    for (const auto& crit : criticalResources()) {
        bool foundCritical = false;
        for (const auto& seen : seenAliases) {
            if (seen == crit.alias) {
                foundCritical = true;
                break;
            }
        }
        if (!foundCritical) {
            return ResourceInclusionStatus::MissingCriticalResource;
        }
    }

    return ResourceInclusionStatus::Valid;
}

std::string_view ResourceQrcCatalog::categoryName(ResourceCategory category) noexcept {
    switch (category) {
    case ResourceCategory::Theme:
        return "Theme";
    case ResourceCategory::Wallpaper:
        return "Wallpaper";
    case ResourceCategory::Branding:
        return "Branding";
    case ResourceCategory::DesignTokens:
        return "DesignTokens";
    case ResourceCategory::NavigationIcon:
        return "NavigationIcon";
    case ResourceCategory::ToolIcon:
        return "ToolIcon";
    }
    return "Unknown";
}

std::string_view ResourceQrcCatalog::statusName(ResourceInclusionStatus status) noexcept {
    switch (status) {
    case ResourceInclusionStatus::Valid:
        return "Valid";
    case ResourceInclusionStatus::InvalidAliasFormat:
        return "InvalidAliasFormat";
    case ResourceInclusionStatus::ResourceNotFound:
        return "ResourceNotFound";
    case ResourceInclusionStatus::CategoryMismatch:
        return "CategoryMismatch";
    case ResourceInclusionStatus::PrefixMismatch:
        return "PrefixMismatch";
    case ResourceInclusionStatus::DuplicateAlias:
        return "DuplicateAlias";
    case ResourceInclusionStatus::MissingCriticalResource:
        return "MissingCriticalResource";
    case ResourceInclusionStatus::ExcludedPathIncluded:
        return "ExcludedPathIncluded";
    }
    return "Unknown";
}

} // namespace vigilio
