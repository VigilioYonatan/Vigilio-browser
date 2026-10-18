#include "core/ResourceQrcCatalog.h"

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition);                                      \
    } while (false)

namespace {

void test_catalog_total_count() {
    CHECK(ResourceQrcCatalog::totalResourceCount() == 35);
    CHECK(ResourceQrcCatalog::canonicalPrefix() == ":/vigilio");

    const auto all = ResourceQrcCatalog::allResources();
    CHECK(all.size() == 35);
}

void test_resource_lookup_positive() {
    // Branding
    CHECK(ResourceQrcCatalog::contains("branding/vigilio-logo.svg"));
    const auto logo = ResourceQrcCatalog::find("branding/vigilio-logo.svg");
    CHECK(logo.has_value());
    CHECK(logo->alias == "branding/vigilio-logo.svg");
    CHECK(logo->qrcPath == ":/vigilio/branding/vigilio-logo.svg");
    CHECK(logo->physicalAssetPath == "assets/branding/vigilio-logo.svg");
    CHECK(logo->category == ResourceCategory::Branding);
    CHECK(logo->mimeType == "image/svg+xml");
    CHECK(logo->isCritical == true);

    // Design tokens
    CHECK(ResourceQrcCatalog::contains("design-tokens/tokens.json"));
    const auto tokens = ResourceQrcCatalog::find("design-tokens/tokens.json");
    CHECK(tokens.has_value());
    CHECK(tokens->category == ResourceCategory::DesignTokens);
    CHECK(tokens->mimeType == "application/json");
    CHECK(tokens->isCritical == true);

    // Critical Navigation icon
    CHECK(ResourceQrcCatalog::contains("icons/navigation/search.svg"));
    const auto search = ResourceQrcCatalog::find("icons/navigation/search.svg");
    CHECK(search.has_value());
    CHECK(search->category == ResourceCategory::NavigationIcon);
    CHECK(search->isCritical == true);

    // Non-critical theme
    CHECK(ResourceQrcCatalog::contains("themes/sunset.json"));
    const auto sunset = ResourceQrcCatalog::find("themes/sunset.json");
    CHECK(sunset.has_value());
    CHECK(sunset->category == ResourceCategory::Theme);
    CHECK(sunset->isCritical == false);

    // Wallpaper
    CHECK(ResourceQrcCatalog::contains("wallpapers/vigilio-landscape-01.jpg"));
    const auto wall1 = ResourceQrcCatalog::find("wallpapers/vigilio-landscape-01.jpg");
    CHECK(wall1.has_value());
    CHECK(wall1->category == ResourceCategory::Wallpaper);
    CHECK(wall1->mimeType == "image/jpeg");
    CHECK(wall1->isCritical == false);

    // Tool icon (senior mode)
    CHECK(ResourceQrcCatalog::contains("icons/tools/senior.svg"));
    const auto senior = ResourceQrcCatalog::find("icons/tools/senior.svg");
    CHECK(senior.has_value());
    CHECK(senior->category == ResourceCategory::ToolIcon);
    CHECK(senior->isCritical == false);
}

void test_resource_lookup_negative() {
    CHECK(!ResourceQrcCatalog::contains("icons/unknown.svg"));
    CHECK(!ResourceQrcCatalog::find("icons/unknown.svg").has_value());

    CHECK(!ResourceQrcCatalog::contains("themes/nonexistent.json"));
    CHECK(!ResourceQrcCatalog::find("themes/nonexistent.json").has_value());

    CHECK(!ResourceQrcCatalog::contains("malicious/payload.exe"));
    CHECK(!ResourceQrcCatalog::find("malicious/payload.exe").has_value());

    CHECK(!ResourceQrcCatalog::contains(""));
    CHECK(!ResourceQrcCatalog::find("").has_value());

    CHECK(!ResourceQrcCatalog::contains("../branding/vigilio-logo.svg"));
    CHECK(!ResourceQrcCatalog::find("../branding/vigilio-logo.svg").has_value());
}

void test_qrc_path_resolution() {
    CHECK(ResourceQrcCatalog::resolveQrcPath("themes/midnight.json") ==
          ":/vigilio/themes/midnight.json");
    CHECK(ResourceQrcCatalog::resolveQrcPath("icons/tools/shield.svg") ==
          ":/vigilio/icons/tools/shield.svg");
    CHECK(ResourceQrcCatalog::resolveQrcPath("/branding/vigilio-logo.svg") ==
          ":/vigilio/branding/vigilio-logo.svg");
    CHECK(ResourceQrcCatalog::resolveQrcPath("branding/vigilio-logo.svg") ==
          ":/vigilio/branding/vigilio-logo.svg");
}

void test_alias_syntax_validation() {
    // Valid aliases
    CHECK(ResourceQrcCatalog::isValidAlias("branding/vigilio-logo.svg"));
    CHECK(ResourceQrcCatalog::isValidAlias("themes/light.json"));
    CHECK(ResourceQrcCatalog::isValidAlias("wallpapers/vigilio-landscape-01.jpg"));
    CHECK(ResourceQrcCatalog::isValidAlias("icons/tools/mcp.svg"));

    // Directory traversal rejection
    CHECK(!ResourceQrcCatalog::isValidAlias("../etc/passwd"));
    CHECK(!ResourceQrcCatalog::isValidAlias("icons/../../secret"));
    CHECK(!ResourceQrcCatalog::isValidAlias(".."));
    CHECK(!ResourceQrcCatalog::isValidAlias("themes/../themes/light.json"));

    // Slash anomalies
    CHECK(!ResourceQrcCatalog::isValidAlias("/branding/logo.svg"));
    CHECK(!ResourceQrcCatalog::isValidAlias("branding/logo.svg/"));
    CHECK(!ResourceQrcCatalog::isValidAlias("themes//light.json"));
    CHECK(!ResourceQrcCatalog::isValidAlias("/"));

    // Empty
    CHECK(!ResourceQrcCatalog::isValidAlias(""));

    // Unsupported extension or characters
    CHECK(!ResourceQrcCatalog::isValidAlias("icons/navigation/search.exe"));
    CHECK(!ResourceQrcCatalog::isValidAlias("icons/navigation/search"));
    CHECK(!ResourceQrcCatalog::isValidAlias("icons/navigation/SEARCH.SVG")); // uppercase rejected
    CHECK(!ResourceQrcCatalog::isValidAlias("branding/logo@2x.svg"));        // special char '@'
    CHECK(!ResourceQrcCatalog::isValidAlias("icons/navigation/search?.svg"));
}

void test_exclusion_rules() {
    // Reference / mockups
    CHECK(ResourceQrcCatalog::isExcludedPath("reference/mockup.png"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/reference/mockups/01.png"));
    CHECK(ResourceQrcCatalog::isExcludedPath("reference/spec.md"));

    // Installer packaging
    CHECK(ResourceQrcCatalog::isExcludedPath("installer/setup.iss"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/installer/setup.iss"));

    // Fonts without audited license
    CHECK(ResourceQrcCatalog::isExcludedPath("fonts/Inter-Regular.ttf"));
    CHECK(ResourceQrcCatalog::isExcludedPath("assets/fonts/Inter-Regular.ttf"));

    // OS root traversal / absolute paths
    CHECK(ResourceQrcCatalog::isExcludedPath("/usr/local/bin"));
    CHECK(ResourceQrcCatalog::isExcludedPath("/tmp/evil"));
    CHECK(ResourceQrcCatalog::isExcludedPath("C:\\Windows\\System32"));

    // Permitted paths
    CHECK(!ResourceQrcCatalog::isExcludedPath("assets/branding/vigilio-logo.svg"));
    CHECK(!ResourceQrcCatalog::isExcludedPath("themes/light.json"));
    CHECK(!ResourceQrcCatalog::isExcludedPath("wallpapers/vigilio-landscape-01.jpg"));
    CHECK(!ResourceQrcCatalog::isExcludedPath(""));
}

void test_category_filtering() {
    const auto themes = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Theme);
    CHECK(themes.size() == 6);
    for (const auto& item : themes) {
        CHECK(item.category == ResourceCategory::Theme);
        CHECK(item.alias.starts_with("themes/"));
    }

    const auto wallpapers = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Wallpaper);
    CHECK(wallpapers.size() == 6);
    for (const auto& item : wallpapers) {
        CHECK(item.category == ResourceCategory::Wallpaper);
        CHECK(item.alias.starts_with("wallpapers/"));
    }

    const auto branding = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::Branding);
    CHECK(branding.size() == 1);
    CHECK(branding[0].alias == "branding/vigilio-logo.svg");

    const auto tokens = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::DesignTokens);
    CHECK(tokens.size() == 1);
    CHECK(tokens[0].alias == "design-tokens/tokens.json");

    const auto navIcons = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::NavigationIcon);
    CHECK(navIcons.size() == 10);
    for (const auto& item : navIcons) {
        CHECK(item.category == ResourceCategory::NavigationIcon);
        CHECK(item.alias.starts_with("icons/navigation/"));
    }

    const auto toolIcons = ResourceQrcCatalog::resourcesByCategory(ResourceCategory::ToolIcon);
    CHECK(toolIcons.size() == 11);
    for (const auto& item : toolIcons) {
        CHECK(item.category == ResourceCategory::ToolIcon);
        CHECK(item.alias.starts_with("icons/tools/"));
    }

    CHECK(themes.size() + wallpapers.size() + branding.size() + tokens.size() + navIcons.size() +
              toolIcons.size() ==
          35);
}

void test_critical_resources() {
    const auto critical = ResourceQrcCatalog::criticalResources();
    CHECK(critical.size() == 8);

    auto hasAlias = [&](std::string_view a) {
        for (const auto& r : critical) {
            if (r.alias == a)
                return true;
        }
        return false;
    };

    CHECK(hasAlias("branding/vigilio-logo.svg"));
    CHECK(hasAlias("design-tokens/tokens.json"));
    CHECK(hasAlias("themes/light.json"));
    CHECK(hasAlias("themes/dark.json"));
    CHECK(hasAlias("icons/navigation/search.svg"));
    CHECK(hasAlias("icons/navigation/back.svg"));
    CHECK(hasAlias("icons/navigation/forward.svg"));
    CHECK(hasAlias("icons/navigation/reload.svg"));
}

void test_qrc_xml_consistency_real_file() {
    std::string path = "apps/vigilio-browser/resources.qrc";
#ifdef VIGILIO_RESOURCES_QRC_PATH
    path = VIGILIO_RESOURCES_QRC_PATH;
#endif

    std::ifstream file(path);
    if (!file.is_open()) {
        // Fallback relative path check
        file.open("../resources.qrc");
    }
    CHECK(file.is_open());

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    const auto status = ResourceQrcCatalog::validateQrcConsistency(content);
    CHECK(status == ResourceInclusionStatus::Valid);
}

void test_qrc_xml_consistency_synthetic() {
    // 1. Prefix mismatch
    const std::string badPrefix =
        R"(<RCC><qresource prefix="/other"><file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(badPrefix) ==
          ResourceInclusionStatus::PrefixMismatch);

    // 2. Missing prefix
    const std::string noPrefix =
        R"(<RCC><qresource><file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(noPrefix) ==
          ResourceInclusionStatus::PrefixMismatch);

    // 3. Excluded path in alias
    const std::string excludedAlias =
        R"(<RCC><qresource prefix="/vigilio"><file alias="reference/mockup.png">assets/reference/mockup.png</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(excludedAlias) ==
          ResourceInclusionStatus::ExcludedPathIncluded);

    // 4. Excluded path in file target
    const std::string excludedTarget =
        R"(<RCC><qresource prefix="/vigilio"><file alias="branding/vigilio-logo.svg">assets/reference/mockup.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(excludedTarget) ==
          ResourceInclusionStatus::ExcludedPathIncluded);

    // 5. Duplicate alias
    const std::string duplicateAlias =
        R"(<RCC><qresource prefix="/vigilio">
        <file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file>
        <file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file>
        </qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(duplicateAlias) ==
          ResourceInclusionStatus::DuplicateAlias);

    // 6. Invalid alias format (directory traversal)
    const std::string traversalAlias =
        R"(<RCC><qresource prefix="/vigilio"><file alias="../evil/file.svg">assets/evil/file.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(traversalAlias) ==
          ResourceInclusionStatus::InvalidAliasFormat);

    // 7. Resource not found in catalog
    const std::string unknownResource =
        R"(<RCC><qresource prefix="/vigilio"><file alias="icons/tools/unknown.svg">assets/icons/tools/unknown.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(unknownResource) ==
          ResourceInclusionStatus::ResourceNotFound);

    // 8. Missing critical resource (only 1 critical resource present, others absent)
    const std::string missingCritical =
        R"(<RCC><qresource prefix="/vigilio"><file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file></qresource></RCC>)";
    CHECK(ResourceQrcCatalog::validateQrcConsistency(missingCritical) ==
          ResourceInclusionStatus::MissingCriticalResource);
}

void test_concurrency() {
    constexpr int kThreads = 16;
    constexpr int kIterations = 2000;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    std::atomic<bool> startFlag{false};
    std::atomic<int> successCount{0};

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&startFlag, &successCount]() {
            while (!startFlag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (int i = 0; i < kIterations; ++i) {
                // Read queries
                if (ResourceQrcCatalog::contains("branding/vigilio-logo.svg") &&
                    ResourceQrcCatalog::find("branding/vigilio-logo.svg").has_value() &&
                    ResourceQrcCatalog::contains("design-tokens/tokens.json") &&
                    !ResourceQrcCatalog::contains("icons/nonexistent.svg") &&
                    ResourceQrcCatalog::isValidAlias("icons/tools/split.svg") &&
                    !ResourceQrcCatalog::isValidAlias("../secret.json") &&
                    ResourceQrcCatalog::isExcludedPath("assets/reference/foo.png") &&
                    !ResourceQrcCatalog::isExcludedPath("themes/midnight.json") &&
                    ResourceQrcCatalog::categoryName(ResourceCategory::Theme) == "Theme" &&
                    ResourceQrcCatalog::statusName(ResourceInclusionStatus::Valid) == "Valid") {
                    successCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    startFlag.store(true, std::memory_order_release);
    for (auto& w : workers) {
        w.join();
    }

    CHECK(successCount.load() == kThreads * kIterations);
}

void test_descriptor_operators() {
    ResourceDescriptor d1{"branding/vigilio-logo.svg",
                          ":/vigilio/branding/vigilio-logo.svg",
                          "assets/branding/vigilio-logo.svg",
                          ResourceCategory::Branding,
                          "image/svg+xml",
                          true};
    ResourceDescriptor d2{"branding/vigilio-logo.svg",
                          ":/vigilio/branding/vigilio-logo.svg",
                          "assets/branding/vigilio-logo.svg",
                          ResourceCategory::Branding,
                          "image/svg+xml",
                          true};
    ResourceDescriptor d3{"themes/dark.json",        ":/vigilio/themes/dark.json",
                          "assets/themes/dark.json", ResourceCategory::Theme,
                          "application/json",        true};

    CHECK(d1 == d2);
    CHECK(!(d1 == d3));
    CHECK((d1 <=> d2) == 0);
    CHECK((d1 <=> d3) < 0);
    CHECK((d3 <=> d1) > 0);
}

void test_name_formatters() {
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::Theme) == "Theme");
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::Wallpaper) == "Wallpaper");
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::Branding) == "Branding");
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::DesignTokens) == "DesignTokens");
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::NavigationIcon) == "NavigationIcon");
    CHECK(ResourceQrcCatalog::categoryName(ResourceCategory::ToolIcon) == "ToolIcon");
    CHECK(ResourceQrcCatalog::categoryName(static_cast<ResourceCategory>(99)) == "Unknown");

    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::Valid) == "Valid");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::InvalidAliasFormat) ==
          "InvalidAliasFormat");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::ResourceNotFound) ==
          "ResourceNotFound");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::CategoryMismatch) ==
          "CategoryMismatch");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::PrefixMismatch) == "PrefixMismatch");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::DuplicateAlias) == "DuplicateAlias");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::MissingCriticalResource) ==
          "MissingCriticalResource");
    CHECK(ResourceQrcCatalog::statusName(ResourceInclusionStatus::ExcludedPathIncluded) ==
          "ExcludedPathIncluded");
    CHECK(ResourceQrcCatalog::statusName(static_cast<ResourceInclusionStatus>(99)) == "Unknown");
}

} // namespace

int main() {
    try {
        test_catalog_total_count();
        test_resource_lookup_positive();
        test_resource_lookup_negative();
        test_qrc_path_resolution();
        test_alias_syntax_validation();
        test_exclusion_rules();
        test_category_filtering();
        test_critical_resources();
        test_qrc_xml_consistency_real_file();
        test_qrc_xml_consistency_synthetic();
        test_concurrency();
        test_descriptor_operators();
        test_name_formatters();

        std::cout << "Resource QRC Catalog tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
