#include "core/ResourceInclusionService.h"

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

void test_startup_validation_success() {
    // 1. Without XML argument (canonical catalog validation)
    const auto report = ResourceInclusionService::validateStartupResources();
    CHECK(report.status == ResourceStartupStatus::Ready);
    CHECK(report.criticalVerified == 8);
    CHECK(report.totalVerified == 35);
    CHECK(report.missingCritical.empty());
    CHECK(report.warnings.empty());
    CHECK(!report.diagnosticMessage.empty());

    // 2. With real QRC file content
    std::string path = "apps/vigilio-browser/resources.qrc";
#ifdef VIGILIO_RESOURCES_QRC_PATH
    path = VIGILIO_RESOURCES_QRC_PATH;
#endif
    std::ifstream file(path);
    if (!file.is_open()) {
        file.open("../resources.qrc");
    }
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        const auto fileReport = ResourceInclusionService::validateStartupResources(buffer.str());
        CHECK(fileReport.status == ResourceStartupStatus::Ready);
        CHECK(fileReport.criticalVerified == 8);
        CHECK(fileReport.missingCritical.empty());
    }
}

void test_startup_validation_missing_critical() {
    // Synthetic QRC omitting branding/vigilio-logo.svg
    const std::string xmlMissingLogo = R"(<RCC>
    <qresource prefix="/vigilio">
        <file alias="design-tokens/tokens.json">assets/design-tokens/tokens.json</file>
        <file alias="themes/light.json">assets/themes/light.json</file>
        <file alias="themes/dark.json">assets/themes/dark.json</file>
        <file alias="icons/navigation/search.svg">assets/icons/navigation/search.svg</file>
        <file alias="icons/navigation/back.svg">assets/icons/navigation/back.svg</file>
        <file alias="icons/navigation/forward.svg">assets/icons/navigation/forward.svg</file>
        <file alias="icons/navigation/reload.svg">assets/icons/navigation/reload.svg</file>
    </qresource>
</RCC>)";

    const auto report = ResourceInclusionService::validateStartupResources(xmlMissingLogo);
    CHECK(report.status == ResourceStartupStatus::MissingCritical);
    CHECK(report.criticalVerified == 7);
    CHECK(report.missingCritical.size() == 1);
    CHECK(report.missingCritical[0] == "branding/vigilio-logo.svg");
    CHECK(!report.diagnosticMessage.empty());
    CHECK(!report.remediationAction.empty());
}

void test_startup_validation_catalog_corruption() {
    // 1. Wrong prefix
    const std::string badPrefix =
        R"(<RCC><qresource prefix="/invalid"><file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file></qresource></RCC>)";
    const auto rep1 = ResourceInclusionService::validateStartupResources(badPrefix);
    CHECK(rep1.status == ResourceStartupStatus::CatalogCorruption);
    CHECK(!rep1.diagnosticMessage.empty());

    // 2. Duplicate alias
    const std::string duplicateAlias =
        R"(<RCC><qresource prefix="/vigilio">
        <file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file>
        <file alias="branding/vigilio-logo.svg">assets/branding/vigilio-logo.svg</file>
        </qresource></RCC>)";
    const auto rep2 = ResourceInclusionService::validateStartupResources(duplicateAlias);
    CHECK(rep2.status == ResourceStartupStatus::CatalogCorruption);

    // 3. Excluded path
    const std::string excluded =
        R"(<RCC><qresource prefix="/vigilio"><file alias="reference/mockup.png">assets/reference/mockup.png</file></qresource></RCC>)";
    const auto rep3 = ResourceInclusionService::validateStartupResources(excluded);
    CHECK(rep3.status == ResourceStartupStatus::CatalogCorruption);
}

void test_load_text_resource_success() {
    // Light theme
    const auto res = ResourceInclusionService::loadTextResource("themes/light.json");
    CHECK(res.isSuccess());
    CHECK(res.error == ResourceReadError::None);
    CHECK(!res.hasFallback);
    CHECK(res.data.has_value());
    CHECK(res.data->find("light") != std::string::npos);
    CHECK(res.data->find("background") != std::string::npos);

    // Design tokens
    const auto tokensRes = ResourceInclusionService::loadTextResource("design-tokens/tokens.json");
    CHECK(tokensRes.isSuccess());
    CHECK(tokensRes.error == ResourceReadError::None);
    CHECK(!tokensRes.hasFallback);
    CHECK(tokensRes.data.has_value());
}

void test_load_text_fallback_enabled() {
    // Non-existent theme with fallback
    const auto res = ResourceInclusionService::loadTextResource("themes/nonexistent.json", true);
    CHECK(res.isSuccess());
    CHECK(res.error == ResourceReadError::None);
    CHECK(res.hasFallback == true);
    CHECK(res.data.has_value());
    CHECK(res.data->find("light") != std::string::npos);

    // Unknown alias with fallback
    const auto res2 = ResourceInclusionService::loadTextResource("themes/custom_v2.json", true);
    CHECK(res2.isSuccess());
    CHECK(res2.hasFallback == true);
    CHECK(res2.data.has_value());
}

void test_load_text_fallback_disabled() {
    // Non-existent theme without fallback
    const auto res = ResourceInclusionService::loadTextResource("themes/nonexistent.json", false);
    CHECK(!res.isSuccess());
    CHECK(res.error == ResourceReadError::NotFound);
    CHECK(!res.data.has_value());
    CHECK(!res.hasFallback);
}

void test_access_denied_excluded_paths() {
    // reference/
    const auto r1 = ResourceInclusionService::loadTextResource("reference/mockup.png");
    CHECK(r1.error == ResourceReadError::AccessDenied);
    CHECK(!r1.isSuccess());

    // installer/
    const auto r2 = ResourceInclusionService::loadTextResource("installer/setup.iss");
    CHECK(r2.error == ResourceReadError::AccessDenied);

    // fonts/
    const auto r3 = ResourceInclusionService::loadTextResource("fonts/Inter.ttf");
    CHECK(r3.error == ResourceReadError::AccessDenied);

    // Directory traversal
    const auto r4 = ResourceInclusionService::loadTextResource("../secret.json");
    // Invalid format or access denied
    CHECK(r4.error == ResourceReadError::InvalidFormat || r4.error == ResourceReadError::AccessDenied);

    // Binary with excluded path
    const auto b1 = ResourceInclusionService::loadBinaryResource("reference/mockup.png");
    CHECK(b1.error == ResourceReadError::AccessDenied);
}

void test_cancellation() {
    // 1. CancellationToken basic usage
    CancellationToken token;
    CHECK(!token.isCancelled());
    token.requestCancellation();
    CHECK(token.isCancelled());

    // Initialized as cancelled
    CancellationToken preCancelled(true);
    CHECK(preCancelled.isCancelled());

    // 2. Binary load with cancelled token
    const auto res = ResourceInclusionService::loadBinaryResource("wallpapers/vigilio-landscape-01.jpg",
                                                                  &preCancelled);
    CHECK(res.error == ResourceReadError::Cancelled);
    CHECK(!res.isSuccess());
    CHECK(!res.data.has_value());

    // 3. Binary load without cancellation
    CancellationToken validToken(false);
    const auto validRes =
        ResourceInclusionService::loadBinaryResource("wallpapers/vigilio-landscape-01.jpg", &validToken);
    CHECK(validRes.isSuccess());
    CHECK(validRes.error == ResourceReadError::None);
    CHECK(validRes.data.has_value());
    CHECK(!validRes.data->empty());

    // 4. Binary load with null token
    const auto nullTokenRes =
        ResourceInclusionService::loadBinaryResource("branding/vigilio-logo.svg", nullptr);
    CHECK(nullTokenRes.isSuccess());
    CHECK(nullTokenRes.error == ResourceReadError::None);
    CHECK(nullTokenRes.data.has_value());
    CHECK(!nullTokenRes.data->empty());
}

void test_resolve_icon_with_fallback() {
    // Valid navigation icon
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/navigation/search.svg") ==
          ":/vigilio/icons/navigation/search.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/navigation/back.svg") ==
          ":/vigilio/icons/navigation/back.svg");

    // Valid tool icon
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/tools/senior.svg") ==
          ":/vigilio/icons/tools/senior.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/tools/tools.svg") ==
          ":/vigilio/icons/tools/tools.svg");

    // Unknown navigation icon -> fallback to tools.svg
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/navigation/unknown.svg") ==
          ":/vigilio/icons/tools/tools.svg");

    // Unknown tool icon -> fallback to tools.svg
    CHECK(ResourceInclusionService::resolveIconWithFallback("icons/tools/unknown.svg") ==
          ":/vigilio/icons/tools/tools.svg");

    // Invalid format / directory traversal -> fallback to tools.svg
    CHECK(ResourceInclusionService::resolveIconWithFallback("../secret.svg") ==
          ":/vigilio/icons/tools/tools.svg");
    CHECK(ResourceInclusionService::resolveIconWithFallback("") == ":/vigilio/icons/tools/tools.svg");

    // Non-icon category (Theme) -> fallback to tools.svg
    CHECK(ResourceInclusionService::resolveIconWithFallback("themes/light.json") ==
          ":/vigilio/icons/tools/tools.svg");
}

void test_resolve_theme_with_fallback() {
    // Valid themes
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/light.json") ==
          "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/dark.json") == "themes/dark.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/midnight.json") ==
          "themes/midnight.json");

    // Unknown theme -> fallback to light.json
    CHECK(ResourceInclusionService::resolveThemeWithFallback("themes/unknown.json") ==
          "themes/light.json");

    // Invalid format -> fallback to light.json
    CHECK(ResourceInclusionService::resolveThemeWithFallback("../theme.json") == "themes/light.json");
    CHECK(ResourceInclusionService::resolveThemeWithFallback("") == "themes/light.json");

    // Constants
    CHECK(ResourceInclusionService::defaultThemeAlias() == "themes/light.json");
    CHECK(ResourceInclusionService::fallbackIconAlias() == "icons/tools/tools.svg");
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
                const auto rep = ResourceInclusionService::validateStartupResources();
                const auto icon =
                    ResourceInclusionService::resolveIconWithFallback("icons/tools/senior.svg");
                const auto fallbackIcon =
                    ResourceInclusionService::resolveIconWithFallback("icons/invalid.svg");
                const auto theme =
                    ResourceInclusionService::resolveThemeWithFallback("themes/dark.json");
                const auto fallbackTheme = ResourceInclusionService::resolveThemeWithFallback("invalid");

                if (rep.status == ResourceStartupStatus::Ready &&
                    icon == ":/vigilio/icons/tools/senior.svg" &&
                    fallbackIcon == ":/vigilio/icons/tools/tools.svg" && theme == "themes/dark.json" &&
                    fallbackTheme == "themes/light.json" &&
                    ResourceInclusionService::errorName(ResourceReadError::None) == "None" &&
                    ResourceInclusionService::startupStatusName(ResourceStartupStatus::Ready) ==
                        "Ready") {
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

void test_name_formatters() {
    CHECK(ResourceInclusionService::errorName(ResourceReadError::None) == "None");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::NotFound) == "NotFound");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::AccessDenied) == "AccessDenied");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::InvalidFormat) == "InvalidFormat");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::CorruptedContent) ==
          "CorruptedContent");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::Cancelled) == "Cancelled");
    CHECK(ResourceInclusionService::errorName(ResourceReadError::IOError) == "IOError");
    CHECK(ResourceInclusionService::errorName(static_cast<ResourceReadError>(99)) == "Unknown");

    CHECK(ResourceInclusionService::startupStatusName(ResourceStartupStatus::Ready) == "Ready");
    CHECK(ResourceInclusionService::startupStatusName(ResourceStartupStatus::Degraded) == "Degraded");
    CHECK(ResourceInclusionService::startupStatusName(ResourceStartupStatus::MissingCritical) ==
          "MissingCritical");
    CHECK(ResourceInclusionService::startupStatusName(ResourceStartupStatus::CatalogCorruption) ==
          "CatalogCorruption");
    CHECK(ResourceInclusionService::startupStatusName(static_cast<ResourceStartupStatus>(99)) ==
          "Unknown");
}

} // namespace

int main() {
    try {
        test_startup_validation_success();
        test_startup_validation_missing_critical();
        test_startup_validation_catalog_corruption();
        test_load_text_resource_success();
        test_load_text_fallback_enabled();
        test_load_text_fallback_disabled();
        test_access_denied_excluded_paths();
        test_cancellation();
        test_resolve_icon_with_fallback();
        test_resolve_theme_with_fallback();
        test_concurrency();
        test_name_formatters();

        std::cout << "Resource Inclusion Service tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
