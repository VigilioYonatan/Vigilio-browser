#include "core/ResourceInclusionService.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace vigilio {

namespace {

constexpr std::string_view kEmbeddedLightThemeJson =
    R"({
  "id": "light",
  "color": {
    "background": "#F7F9FC",
    "surface": "#FFFFFF",
    "text": "#172033",
    "accent": "#2563EB",
    "border": "#DCE3ED"
  },
  "radii": {
    "control": 8,
    "panel": 12
  },
  "spacingBase": 4
}
)";

std::filesystem::path findPhysicalAsset(std::string_view assetPath) {
    if (const char* envDir = std::getenv("VIGILIO_ASSETS_DIR")) {
        std::filesystem::path candidate(envDir);
        std::string_view rel = assetPath;
        if (rel.starts_with("assets/")) {
            rel.remove_prefix(7);
        }
        std::filesystem::path p = candidate / rel;
        if (std::filesystem::is_regular_file(p)) {
            return p;
        }
    }

#ifdef VIGILIO_ASSETS_DIR
    {
        std::filesystem::path candidate(VIGILIO_ASSETS_DIR);
        std::string_view rel = assetPath;
        if (rel.starts_with("assets/")) {
            rel.remove_prefix(7);
        }
        std::filesystem::path p = candidate / rel;
        if (std::filesystem::is_regular_file(p)) {
            return p;
        }
    }
#endif

    const std::array<std::string, 4> prefixes = {"", "apps/vigilio-browser/", "../", "../../"};
    for (const auto& prefix : prefixes) {
        std::filesystem::path p(prefix + std::string(assetPath));
        if (std::filesystem::is_regular_file(p)) {
            return p;
        }
    }

    return {};
}

} // namespace

ResourceStartupReport
ResourceInclusionService::validateStartupResources(std::string_view qrcContent) noexcept {
    ResourceStartupReport report{};

    if (qrcContent.empty()) {
        const auto critical = ResourceQrcCatalog::criticalResources();
        report.criticalVerified = critical.size();
        report.totalVerified = ResourceQrcCatalog::totalResourceCount();
        report.status = ResourceStartupStatus::Ready;
        report.diagnosticMessage = "All critical startup resources successfully verified.";
        return report;
    }

    const auto consistency = ResourceQrcCatalog::validateQrcConsistency(qrcContent);

    if (consistency == ResourceInclusionStatus::MissingCriticalResource) {
        report.status = ResourceStartupStatus::MissingCritical;
        const auto critical = ResourceQrcCatalog::criticalResources();
        for (const auto& crit : critical) {
            if (qrcContent.find(crit.alias) == std::string_view::npos) {
                report.missingCritical.push_back(std::string(crit.alias));
            }
        }
        report.criticalVerified = critical.size() - report.missingCritical.size();
        report.totalVerified = ResourceQrcCatalog::totalResourceCount();
        report.diagnosticMessage = "Critical startup resources are missing from the resource bundle.";
        report.remediationAction =
            "Ensure all critical resources are defined in resources.qrc and packaged correctly.";
        return report;
    }

    if (consistency == ResourceInclusionStatus::PrefixMismatch ||
        consistency == ResourceInclusionStatus::DuplicateAlias ||
        consistency == ResourceInclusionStatus::InvalidAliasFormat ||
        consistency == ResourceInclusionStatus::ExcludedPathIncluded) {
        report.status = ResourceStartupStatus::CatalogCorruption;
        report.diagnosticMessage = std::string("Resource catalog or QRC definition is corrupted: ") +
                                   std::string(ResourceQrcCatalog::statusName(consistency));
        report.remediationAction = "Fix the QRC file structure, prefix or syntax in resources.qrc.";
        return report;
    }

    if (consistency == ResourceInclusionStatus::CategoryMismatch ||
        consistency == ResourceInclusionStatus::ResourceNotFound) {
        report.status = ResourceStartupStatus::Degraded;
        report.warnings.push_back(std::string(ResourceQrcCatalog::statusName(consistency)));
        report.diagnosticMessage = "Resource catalog has non-critical discrepancies.";
        report.remediationAction = "Review resource aliases and category mappings in resources.qrc.";
        return report;
    }

    report.status = ResourceStartupStatus::Ready;
    report.criticalVerified = ResourceQrcCatalog::criticalResources().size();
    report.totalVerified = ResourceQrcCatalog::totalResourceCount();
    report.diagnosticMessage = "All critical startup resources successfully verified.";
    return report;
}

ResourceReadResult<std::string> ResourceInclusionService::loadTextResource(std::string_view alias,
                                                                           bool allowFallback) noexcept {
    if (ResourceQrcCatalog::isExcludedPath(alias)) {
        return {ResourceReadError::AccessDenied, std::nullopt,
                "Access denied to excluded path: " + std::string(alias), false};
    }

    if (!ResourceQrcCatalog::isValidAlias(alias)) {
        return {ResourceReadError::InvalidFormat, std::nullopt,
                "Invalid alias format: " + std::string(alias), false};
    }

    const auto descriptor = ResourceQrcCatalog::find(alias);
    if (!descriptor.has_value()) {
        if (allowFallback) {
            std::string fallbackContent;
            const auto defaultDesc = ResourceQrcCatalog::find(kDefaultThemeAlias);
            if (defaultDesc.has_value()) {
                const auto p = findPhysicalAsset(defaultDesc->physicalAssetPath);
                if (!p.empty()) {
                    std::ifstream file(p, std::ios::binary);
                    if (file) {
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        fallbackContent = buffer.str();
                    }
                }
            }
            if (fallbackContent.empty()) {
                fallbackContent = std::string(kEmbeddedLightThemeJson);
            }
            return {ResourceReadError::None, fallbackContent, "Fallback to default theme", true};
        }
        return {ResourceReadError::NotFound, std::nullopt,
                "Resource not found in catalog: " + std::string(alias), false};
    }

    const auto assetFile = findPhysicalAsset(descriptor->physicalAssetPath);
    if (!assetFile.empty()) {
        std::ifstream file(assetFile, std::ios::binary);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return {ResourceReadError::None, buffer.str(), "", false};
        }
        return {ResourceReadError::IOError, std::nullopt,
                "Failed to read physical asset file: " + assetFile.string(), false};
    }

    if (alias == kDefaultThemeAlias) {
        return {ResourceReadError::None, std::string(kEmbeddedLightThemeJson), "", false};
    }

    return {ResourceReadError::NotFound, std::nullopt,
            "Physical asset not found on disk: " + std::string(descriptor->physicalAssetPath), false};
}

ResourceReadResult<std::vector<uint8_t>>
ResourceInclusionService::loadBinaryResource(std::string_view alias,
                                             const CancellationToken* cancelToken) noexcept {
    if (cancelToken != nullptr && cancelToken->isCancelled()) {
        return {ResourceReadError::Cancelled, std::nullopt, "Operation cancelled by token", false};
    }

    if (ResourceQrcCatalog::isExcludedPath(alias)) {
        return {ResourceReadError::AccessDenied, std::nullopt,
                "Access denied to excluded path: " + std::string(alias), false};
    }

    if (!ResourceQrcCatalog::isValidAlias(alias)) {
        return {ResourceReadError::InvalidFormat, std::nullopt,
                "Invalid alias format: " + std::string(alias), false};
    }

    const auto descriptor = ResourceQrcCatalog::find(alias);
    if (!descriptor.has_value()) {
        return {ResourceReadError::NotFound, std::nullopt,
                "Resource not found in catalog: " + std::string(alias), false};
    }

    if (cancelToken != nullptr && cancelToken->isCancelled()) {
        return {ResourceReadError::Cancelled, std::nullopt, "Operation cancelled by token", false};
    }

    const auto assetFile = findPhysicalAsset(descriptor->physicalAssetPath);
    if (assetFile.empty()) {
        return {ResourceReadError::NotFound, std::nullopt,
                "Physical asset not found on disk: " + std::string(descriptor->physicalAssetPath),
                false};
    }

    std::ifstream file(assetFile, std::ios::binary | std::ios::ate);
    if (!file) {
        return {ResourceReadError::IOError, std::nullopt,
                "Failed to open physical asset file: " + assetFile.string(), false};
    }

    const auto size = file.tellg();
    if (size < 0) {
        return {ResourceReadError::IOError, std::nullopt, "Failed to determine file size", false};
    }
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer;
    buffer.resize(static_cast<size_t>(size));

    constexpr size_t kChunkSize = 4096;
    size_t bytesRead = 0;
    while (bytesRead < buffer.size()) {
        if (cancelToken != nullptr && cancelToken->isCancelled()) {
            return {ResourceReadError::Cancelled, std::nullopt, "Operation cancelled during read",
                    false};
        }
        const size_t toRead = std::min(kChunkSize, buffer.size() - bytesRead);
        file.read(reinterpret_cast<char*>(buffer.data() + bytesRead),
                  static_cast<std::streamsize>(toRead));
        if (!file && !file.eof()) {
            return {ResourceReadError::IOError, std::nullopt, "Read error from physical file", false};
        }
        bytesRead += static_cast<size_t>(file.gcount());
        if (file.gcount() == 0) {
            break;
        }
    }

    if (cancelToken != nullptr && cancelToken->isCancelled()) {
        return {ResourceReadError::Cancelled, std::nullopt, "Operation cancelled during read", false};
    }

    buffer.resize(bytesRead);
    return {ResourceReadError::None, std::move(buffer), "", false};
}

std::string ResourceInclusionService::resolveIconWithFallback(std::string_view alias) noexcept {
    if (ResourceQrcCatalog::isValidAlias(alias)) {
        const auto descriptor = ResourceQrcCatalog::find(alias);
        if (descriptor.has_value() && (descriptor->category == ResourceCategory::NavigationIcon ||
                                       descriptor->category == ResourceCategory::ToolIcon)) {
            return ResourceQrcCatalog::resolveQrcPath(alias);
        }
    }
    return ResourceQrcCatalog::resolveQrcPath(kFallbackIconAlias);
}

std::string ResourceInclusionService::resolveThemeWithFallback(std::string_view themeAlias) noexcept {
    if (ResourceQrcCatalog::isValidAlias(themeAlias)) {
        const auto descriptor = ResourceQrcCatalog::find(themeAlias);
        if (descriptor.has_value() && descriptor->category == ResourceCategory::Theme) {
            return std::string(themeAlias);
        }
    }
    return std::string(kDefaultThemeAlias);
}

std::string_view ResourceInclusionService::errorName(ResourceReadError error) noexcept {
    switch (error) {
    case ResourceReadError::None:
        return "None";
    case ResourceReadError::NotFound:
        return "NotFound";
    case ResourceReadError::AccessDenied:
        return "AccessDenied";
    case ResourceReadError::InvalidFormat:
        return "InvalidFormat";
    case ResourceReadError::CorruptedContent:
        return "CorruptedContent";
    case ResourceReadError::Cancelled:
        return "Cancelled";
    case ResourceReadError::IOError:
        return "IOError";
    }
    return "Unknown";
}

std::string_view ResourceInclusionService::startupStatusName(ResourceStartupStatus status) noexcept {
    switch (status) {
    case ResourceStartupStatus::Ready:
        return "Ready";
    case ResourceStartupStatus::Degraded:
        return "Degraded";
    case ResourceStartupStatus::MissingCritical:
        return "MissingCritical";
    case ResourceStartupStatus::CatalogCorruption:
        return "CatalogCorruption";
    }
    return "Unknown";
}

} // namespace vigilio
