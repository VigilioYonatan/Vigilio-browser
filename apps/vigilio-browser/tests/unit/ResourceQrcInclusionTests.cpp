#include "core/ResourceInclusionService.h"
#include "core/ResourceQrcCatalog.h"
#include "core/ResourceTypes.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error("CHECK failed at line " + std::to_string(__LINE__) +               \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

std::filesystem::path resolvePhysicalAsset(std::string_view assetPath) {
    if (const char* envDir = std::getenv("VIGILIO_ASSETS_DIR")) {
        std::filesystem::path candidate(envDir);
        std::string_view rel = assetPath;
        if (rel.starts_with("assets/")) {
            rel.remove_prefix(7);
        }
        std::filesystem::path p = candidate / rel;
        if (std::filesystem::exists(p)) {
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
        if (std::filesystem::exists(p)) {
            return p;
        }
    }
#endif

    std::string_view rel = assetPath;
    if (rel.starts_with("assets/")) {
        rel.remove_prefix(7);
    }
    const std::array<std::string, 5> prefixes = {"apps/vigilio-browser/assets/", "assets/", "../assets/",
                                                 "../../assets/", "../../apps/vigilio-browser/assets/"};
    for (const auto& prefix : prefixes) {
        std::filesystem::path p(prefix + std::string(rel));
        if (std::filesystem::exists(p)) {
            return p;
        }
    }
    return {};
}

void test_physical_assets_integrity() {
    const auto all = ResourceQrcCatalog::allResources();
    CHECK(all.size() == 35);

    for (const auto& res : all) {
        const auto p = resolvePhysicalAsset(res.physicalAssetPath);
        CHECK(!p.empty());
        CHECK(std::filesystem::exists(p));
        CHECK(!std::filesystem::is_directory(p));
        CHECK(std::filesystem::file_size(p) > 0);

        std::ifstream file(p, std::ios::binary);
        CHECK(file.is_open());
        CHECK(file.good());
    }
}

void test_asset_content_structure() {
    // 1. Tokens
    {
        const auto p = resolvePhysicalAsset("assets/design-tokens/tokens.json");
        CHECK(!p.empty());
        std::ifstream file(p);
        CHECK(file.is_open());
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string content = buffer.str();
        CHECK(!content.empty());
        CHECK(content.find('{') != std::string::npos);
        CHECK(content.find('}') != std::string::npos);
        CHECK(content.find("fontFamily") != std::string::npos);
        CHECK(content.find("spacing") != std::string::npos);
        CHECK(content.find("fontSize") != std::string::npos);
        CHECK(content.find("touchTargetSenior") != std::string::npos);
    }

    // 2. Themes (6 themes)
    const auto themes = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Theme);
    CHECK(themes.size() == 6);
    for (const auto& theme : themes) {
        const auto p = resolvePhysicalAsset(theme.physicalAssetPath);
        CHECK(!p.empty());
        std::ifstream file(p);
        CHECK(file.is_open());
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string content = buffer.str();
        CHECK(!content.empty());
        CHECK(content.find('{') != std::string::npos);
        CHECK(content.find('}') != std::string::npos);
        CHECK(content.find("id") != std::string::npos);
        CHECK(content.find("color") != std::string::npos);
        CHECK(content.find("background") != std::string::npos);
        CHECK(content.find("surface") != std::string::npos);
    }

    // 3. SVG vector quality and security (22 files)
    const auto branding = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Branding);
    const auto navIcons = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::NavigationIcon);
    const auto toolIcons = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::ToolIcon);
    CHECK(branding.size() == 1);
    CHECK(navIcons.size() == 10);
    CHECK(toolIcons.size() == 11);

    std::vector<ResourceDescriptor> svgResources;
    svgResources.insert(svgResources.end(), branding.begin(), branding.end());
    svgResources.insert(svgResources.end(), navIcons.begin(), navIcons.end());
    svgResources.insert(svgResources.end(), toolIcons.begin(), toolIcons.end());
    CHECK(svgResources.size() == 22);

    for (const auto& svgRes : svgResources) {
        const auto p = resolvePhysicalAsset(svgRes.physicalAssetPath);
        CHECK(!p.empty());
        std::ifstream file(p);
        CHECK(file.is_open());
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string content = buffer.str();
        CHECK(!content.empty());
        CHECK(content.find("<svg") != std::string::npos);
        CHECK(content.find("</svg>") != std::string::npos);
        CHECK(content.find("viewBox=") != std::string::npos);

        if (svgRes.category == ResourceCategory::NavigationIcon ||
            svgRes.category == ResourceCategory::ToolIcon) {
            CHECK(content.find("viewBox=\"0 0 24 24\"") != std::string::npos);
        } else if (svgRes.category == ResourceCategory::Branding) {
            CHECK(content.find("viewBox=\"0 0 128 128\"") != std::string::npos);
        }

        CHECK(content.find("<script") == std::string::npos);
        CHECK(content.find("<SCRIPT") == std::string::npos);
        CHECK(content.find("xlink:href=\"http") == std::string::npos);
    }

    // 4. Wallpapers JPEG magic numbers (6 files)
    const auto wallpapers = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Wallpaper);
    CHECK(wallpapers.size() == 6);
    for (const auto& wall : wallpapers) {
        const auto p = resolvePhysicalAsset(wall.physicalAssetPath);
        CHECK(!p.empty());
        std::ifstream file(p, std::ios::binary);
        CHECK(file.is_open());
        uint8_t header[3] = {0, 0, 0};
        file.read(reinterpret_cast<char*>(header), 3);
        CHECK(file.gcount() == 3);
        CHECK(header[0] == 0xFF);
        CHECK(header[1] == 0xD8);
        CHECK(header[2] == 0xFF);
    }
}

void test_mime_type_consistency() {
    const auto all = ResourceQrcCatalog::allResources();
    CHECK(all.size() == 35);

    for (const auto& res : all) {
        if (res.alias.ends_with(".svg")) {
            CHECK(res.mimeType == "image/svg+xml");
            CHECK(res.category == ResourceCategory::Branding ||
                  res.category == ResourceCategory::NavigationIcon ||
                  res.category == ResourceCategory::ToolIcon);
        } else if (res.alias.ends_with(".json")) {
            CHECK(res.mimeType == "application/json");
            CHECK(res.category == ResourceCategory::Theme ||
                  res.category == ResourceCategory::DesignTokens);
        } else if (res.alias.ends_with(".jpg") || res.alias.ends_with(".jpeg")) {
            CHECK(res.mimeType == "image/jpeg");
            CHECK(res.category == ResourceCategory::Wallpaper);
        } else {
            throw std::runtime_error("Unexpected extension in catalog resource: " +
                                     std::string(res.alias));
        }
    }
}

void test_negative_alias_syntax_fuzzing() {
    const std::vector<std::string_view> invalidAliases = {
        // Path traversal
        "../etc/passwd", "../../secret.json", "themes/../themes/light.json", "..", "../", "..\\windows",
        "themes/..",

        // Slashes and separators
        "/branding/logo.svg", "branding/logo.svg/", "themes//light.json", "/", "//", "///",
        "\\windows\\system32",

        // Empty and spaces
        "", " ", "   ", "branding/ logo.svg", "themes/light.json ", " themes/light.json",
        "themes/ light.json",

        // Uppercase
        "BRANDING/VIGILIO-LOGO.SVG", "themes/Light.json", "THEMES/LIGHT.JSON",
        "ICONS/NAVIGATION/SEARCH.SVG", "icons/navigation/Search.svg",

        // Forbidden characters
        "logo@2x.svg", "search?.svg", "icon#1.svg", "theme$var.json", "icon!alert.svg",
        "search*star.svg", "semi;colon.svg", "quote'test.svg",

        // Unauthorized extensions
        "script.exe", "run.sh", "module.bin", "exploit.py", "theme.yaml", "config.xml", "readme.txt",
        "library.so"};

    for (const auto& alias : invalidAliases) {
        CHECK(!ResourceQrcCatalog::isValidAlias(alias));
    }
}

void test_excluded_paths_defense() {
    // 1. Reference / mockups
    CHECK(ResourceQrcCatalog::isExcludedPath("reference/mockup.png"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/reference/spec.md"));

    // 2. Installer packaging
    CHECK(ResourceQrcCatalog::isExcludedPath("installer/setup.iss"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/installer/setup.exe"));

    // 3. Fonts
    CHECK(ResourceQrcCatalog::isExcludedPath("fonts/Inter-Regular.ttf"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/fonts/Inter.ttf"));

    // 4. System absolute paths
    CHECK(ResourceQrcCatalog::isExcludedPath("/usr/bin/bash"));
    CHECK(ResourceQrcCatalog::isExcludedPath("/tmp/evil.txt"));
    CHECK(ResourceQrcCatalog::isExcludedPath("C:\\boot.ini"));
    CHECK(ResourceQrcCatalog::isExcludedPath("c:\\windows\\system32"));

    // 5. Permitted paths
    CHECK(!ResourceQrcCatalog::isExcludedPath("branding/vigilio-logo.svg"));
    CHECK(!ResourceQrcCatalog::isExcludedPath("themes/light.json"));
    CHECK(!ResourceQrcCatalog::isExcludedPath("wallpapers/vigilio-landscape-01.jpg"));
    CHECK(!ResourceQrcCatalog::isExcludedPath(""));

    // 6. AccessDenied defense in loadTextResource and loadBinaryResource
    const auto resText = ResourceInclusionService::loadTextResource("reference/mockup.png");
    CHECK(!resText.isSuccess());
    CHECK(resText.error == ResourceReadError::AccessDenied);

    const auto resBin = ResourceInclusionService::loadBinaryResource("assets/installer/setup.exe");
    CHECK(!resBin.isSuccess());
    CHECK(resBin.error == ResourceReadError::AccessDenied);

    const auto resSys = ResourceInclusionService::loadTextResource("/usr/bin/bash");
    CHECK(!resSys.isSuccess());
    CHECK(resSys.error == ResourceReadError::AccessDenied);
}

void test_synthetic_qrc_manifest_corruption() {
    // 1. Wrong prefix
    const std::string badPrefix =
        "<RCC><qresource prefix=\"/other\"><file "
        "alias=\"branding/vigilio-logo.svg\">assets/branding/vigilio-logo.svg</file></qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(badPrefix) ==
          ResourceInclusionStatus::PrefixMismatch);
    const auto rep1 = ResourceInclusionService::validateStartupResources(badPrefix);
    CHECK(rep1.status == ResourceStartupStatus::CatalogCorruption);
    CHECK(!rep1.diagnosticMessage.empty());
    CHECK(!rep1.remediationAction.empty());

    // 2. Missing prefix
    const std::string missingPrefix =
        "<RCC><qresource><file "
        "alias=\"branding/vigilio-logo.svg\">assets/branding/vigilio-logo.svg</file></qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(missingPrefix) ==
          ResourceInclusionStatus::PrefixMismatch);
    const auto rep2 = ResourceInclusionService::validateStartupResources(missingPrefix);
    CHECK(rep2.status == ResourceStartupStatus::CatalogCorruption);

    // 3. Duplicate alias
    const std::string duplicateAlias =
        "<RCC><qresource prefix=\"/vigilio\">\n"
        "<file alias=\"branding/vigilio-logo.svg\">assets/branding/vigilio-logo.svg</file>\n"
        "<file alias=\"branding/vigilio-logo.svg\">assets/branding/vigilio-logo.svg</file>\n"
        "</qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(duplicateAlias) ==
          ResourceInclusionStatus::DuplicateAlias);
    const auto rep3 = ResourceInclusionService::validateStartupResources(duplicateAlias);
    CHECK(rep3.status == ResourceStartupStatus::CatalogCorruption);

    // 4. Resource not in catalog
    const std::string notInCatalog =
        "<RCC><qresource prefix=\"/vigilio\"><file "
        "alias=\"themes/nonexistent.json\">assets/themes/nonexistent.json</file></qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(notInCatalog) ==
          ResourceInclusionStatus::ResourceNotFound);
    const auto rep4 = ResourceInclusionService::validateStartupResources(notInCatalog);
    CHECK(rep4.status == ResourceStartupStatus::Degraded);

    // 5. Invalid alias format (path traversal)
    const std::string traversal =
        "<RCC><qresource prefix=\"/vigilio\"><file alias=\"../secret.json\">assets/secret.json</file></"
        "qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(traversal) ==
          ResourceInclusionStatus::InvalidAliasFormat);
    const auto rep5 = ResourceInclusionService::validateStartupResources(traversal);
    CHECK(rep5.status == ResourceStartupStatus::CatalogCorruption);

    // 6. Excluded path included
    const std::string excluded =
        "<RCC><qresource prefix=\"/vigilio\"><file "
        "alias=\"reference/mockup.png\">assets/reference/mockup.png</file></qresource></RCC>";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(excluded) ==
          ResourceInclusionStatus::ExcludedPathIncluded);
    const auto rep6 = ResourceInclusionService::validateStartupResources(excluded);
    CHECK(rep6.status == ResourceStartupStatus::CatalogCorruption);
}

void test_critical_resources_isolation_and_remediation() {
    const auto critical = ResourceQrcCatalog::criticalResources();
    CHECK(critical.size() == 8);

    const auto all = ResourceQrcCatalog::allResources();
    CHECK(all.size() == 35);

    for (const auto& crit : critical) {
        std::string xml = "<RCC><qresource prefix=\"/vigilio\">\n";
        for (const auto& res : all) {
            if (res.alias == crit.alias) {
                continue;
            }
            xml += "<file alias=\"" + std::string(res.alias) + "\">" +
                   std::string(res.physicalAssetPath) + "</file>\n";
        }
        xml += "</qresource></RCC>";

        const auto report = ResourceInclusionService::validateStartupResources(xml);
        CHECK(report.status == ResourceStartupStatus::MissingCritical);
        CHECK(report.criticalVerified == 7);
        CHECK(report.missingCritical.size() == 1);
        CHECK(report.missingCritical[0] == crit.alias);
        CHECK(!report.diagnosticMessage.empty());
        CHECK(!report.remediationAction.empty());
    }
}

void test_fallback_recovery_mechanisms() {
    // 1. Icon resolution
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/navigation/missing.svg") ==
          ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/tools/unknown.svg") ==
          ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("invalid_alias") ==
          ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("") == ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("themes/light.json") ==
          ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("../branding/vigilio-logo.svg") ==
          ":/vigilio/icons/tools/tools.svg");

    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/navigation/search.svg") ==
          ":/vigilio/icons/navigation/search.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/tools/senior.svg") ==
          ":/vigilio/icons/tools/senior.svg");

    // 2. Theme resolution
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/missing.json") ==
          "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("nonexistent") == "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("") == "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("../theme.json") == "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("icons/tools/tools.svg") ==
          "themes/light.json");

    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/dark.json") == "themes/dark.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/midnight.json") ==
          "themes/midnight.json");

    // 3. Text loading with allowFallback = true
    const auto fallbackLoad = ResourceInclusionService::loadTextResource("themes/missing.json", true);
    CHECK(fallbackLoad.isSuccess());
    CHECK(fallbackLoad.error == ResourceReadError::None);
    CHECK(fallbackLoad.hasFallback == true);
    CHECK(fallbackLoad.data.has_value());
    CHECK(!fallbackLoad.data->empty());
    CHECK(fallbackLoad.data->find("light") != std::string::npos ||
          fallbackLoad.data->find("color") != std::string::npos);

    // 4. Text loading with allowFallback = false
    const auto strictLoad = ResourceInclusionService::loadTextResource("themes/missing.json", false);
    CHECK(!strictLoad.isSuccess());
    CHECK(strictLoad.error == ResourceReadError::NotFound);
    CHECK(strictLoad.hasFallback == false);
    CHECK(!strictLoad.data.has_value());
}

void test_cancellation_cooperative() {
    // 1. Pre-cancelled token
    CancellationToken preCancelled(true);
    CHECK(preCancelled.isCancelled());
    const auto res1 = ResourceInclusionService::loadBinaryResource("wallpapers/vigilio-landscape-01.jpg",
                                                                   &preCancelled);
    CHECK(!res1.isSuccess());
    CHECK(res1.error == ResourceReadError::Cancelled);
    CHECK(!res1.data.has_value());

    // 2. Cancellation requested dynamically
    CancellationToken token;
    CHECK(!token.isCancelled());
    token.requestCancellation();
    CHECK(token.isCancelled());
    const auto res2 =
        ResourceInclusionService::loadBinaryResource("wallpapers/vigilio-landscape-02.jpg", &token);
    CHECK(!res2.isSuccess());
    CHECK(res2.error == ResourceReadError::Cancelled);
    CHECK(!res2.data.has_value());

    // 3. Normal read with uncancelled token
    CancellationToken liveToken(false);
    const auto res3 =
        ResourceInclusionService::loadBinaryResource("wallpapers/vigilio-landscape-01.jpg", &liveToken);
    CHECK(res3.isSuccess());
    CHECK(res3.error == ResourceReadError::None);
    CHECK(res3.data.has_value());
    CHECK(res3.data->size() > 1000);
}

void test_high_concurrency_stress_and_thread_safety() {
    constexpr unsigned int kThreadCount = 16;
    constexpr size_t kIterationsPerThread = 2000;

    std::atomic<bool> startFlag{false};
    std::atomic<size_t> totalOperations{0};
    std::atomic<size_t> errorCount{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);

    for (unsigned int t = 0; t < kThreadCount; ++t) {
        workers.emplace_back([&]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (size_t i = 0; i < kIterationsPerThread; ++i) {
                try {
                    // 1. Catalog lookup
                    const auto logo = ResourceQrcCatalog::find("branding/vigilio-logo.svg");
                    if (!logo.has_value() || logo->category != ResourceCategory::Branding) {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 2. Contains check
                    if (!ResourceQrcCatalog::contains("design-tokens/tokens.json") ||
                        ResourceQrcCatalog::contains("invalid/path.json")) {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 3. Path resolution
                    const auto qrc = ResourceQrcCatalog::resolveQrcPath("themes/midnight.json");
                    if (qrc != ":/vigilio/themes/midnight.json") {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 4. Icon fallback
                    const auto icon =
                        ResourceInclusionService::resolveIconWithFallback("icons/unknown.svg");
                    if (icon != ":/vigilio/icons/tools/tools.svg") {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 5. Theme fallback
                    const auto theme =
                        ResourceInclusionService::resolveThemeWithFallback("themes/unknown.json");
                    if (theme != "themes/light.json") {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 6. Startup validation (in-memory)
                    const auto report = ResourceInclusionService::validateStartupResources();
                    if (report.status != ResourceStartupStatus::Ready || report.criticalVerified != 8) {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    // 7. Validation predicates
                    if (!ResourceQrcCatalog::isValidAlias("icons/tools/senior.svg") ||
                        ResourceQrcCatalog::isValidAlias("../bad/alias.svg") ||
                        !ResourceQrcCatalog::isExcludedPath("reference/mockup.png")) {
                        errorCount.fetch_add(1, std::memory_order_relaxed);
                    }

                    totalOperations.fetch_add(1, std::memory_order_relaxed);
                } catch (...) {
                    errorCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    const auto startTime = std::chrono::steady_clock::now();
    startFlag.store(true, std::memory_order_release);

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    const auto elapsed = std::chrono::steady_clock::now() - startTime;
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    CHECK(errorCount.load() == 0);
    CHECK(totalOperations.load() == kThreadCount * kIterationsPerThread);

    std::cout << "    [Stress] " << totalOperations.load() << " concurrent operations completed in "
              << elapsedMs << " ms (error count: " << errorCount.load() << ")" << std::endl;
}

} // namespace

int main() {
    try {
        std::cout << "[ResourceQrcInclusionTests] Starting test suite..." << std::endl;

        test_physical_assets_integrity();
        std::cout << "  [PASS] test_physical_assets_integrity" << std::endl;

        test_asset_content_structure();
        std::cout << "  [PASS] test_asset_content_structure" << std::endl;

        test_mime_type_consistency();
        std::cout << "  [PASS] test_mime_type_consistency" << std::endl;

        test_negative_alias_syntax_fuzzing();
        std::cout << "  [PASS] test_negative_alias_syntax_fuzzing" << std::endl;

        test_excluded_paths_defense();
        std::cout << "  [PASS] test_excluded_paths_defense" << std::endl;

        test_synthetic_qrc_manifest_corruption();
        std::cout << "  [PASS] test_synthetic_qrc_manifest_corruption" << std::endl;

        test_critical_resources_isolation_and_remediation();
        std::cout << "  [PASS] test_critical_resources_isolation_and_remediation" << std::endl;

        test_fallback_recovery_mechanisms();
        std::cout << "  [PASS] test_fallback_recovery_mechanisms" << std::endl;

        test_cancellation_cooperative();
        std::cout << "  [PASS] test_cancellation_cooperative" << std::endl;

        test_high_concurrency_stress_and_thread_safety();
        std::cout << "  [PASS] test_high_concurrency_stress_and_thread_safety" << std::endl;

        std::cout << "[ResourceQrcInclusionTests] All 10 tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[ResourceQrcInclusionTests] FAILURE: " << ex.what() << std::endl;
        return 1;
    }
}
