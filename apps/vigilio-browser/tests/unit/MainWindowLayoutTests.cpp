#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutTypes.h"
#include "core/MainWindowLayoutValidator.h"

#include <cctype>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

// TEST-001: testCanonicalDefaultLayoutInvariants
void testCanonicalDefaultLayoutInvariants() {
    const auto plan = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    CHECK(plan.density == LayoutDensity::Default);
    CHECK(plan.window.width == 1200);
    CHECK(plan.window.height == 780);
    CHECK(plan.window.minWidth == 640);
    CHECK(plan.window.minHeight == 480);
    CHECK(plan.sidebar.collapsedWidth == 48);
    CHECK(plan.sidebar.width == 48);
    CHECK(plan.sidebar.state == SidebarState::Collapsed);
    CHECK(plan.sidebar.position == SidebarDockPosition::Left);
    CHECK(MainWindowLayoutConfiguration::verifyInvariants(plan));
    CHECK(MainWindowLayoutValidator::validatePlan(plan) == MainWindowLayoutError::None);
    const auto report = MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, plan);
    CHECK(report.valid);
    CHECK(report.error == MainWindowLayoutError::None);
    CHECK(report.contentArea.width > 0);
    CHECK(report.contentArea.height > 0);
}

// TEST-002: testCanonicalSeniorLayoutInvariants
void testCanonicalSeniorLayoutInvariants() {
    const auto plan = MainWindowLayoutConfiguration::canonicalSeniorLayout();
    CHECK(plan.density == LayoutDensity::Senior);
    CHECK(plan.window.minWidth >= 720);
    CHECK(plan.window.minHeight >= 520);
    CHECK(MainWindowLayoutConfiguration::kSeniorMinTapTargetSize >= 48);
    CHECK(MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior >= 56);
    CHECK(MainWindowLayoutConfiguration::verifyInvariants(plan));
    CHECK(MainWindowLayoutValidator::validatePlan(plan) == MainWindowLayoutError::None);

    for (const auto& zb : plan.zones) {
        if (zb.zone == LayoutZone::ToolBar && zb.visible) {
            CHECK(zb.preferredHeight >= 56);
            CHECK(zb.minHeight >= 56);
        }
        if (zb.zone == LayoutZone::TabStrip && zb.visible) {
            CHECK(zb.preferredHeight >= 48);
            CHECK(zb.minHeight >= 48);
        }
    }

    WindowDimensions subMinimumDims{.width = 719, .height = 519, .minWidth = 720, .minHeight = 520};
    CHECK(MainWindowLayoutValidator::validateDimensions(subMinimumDims, LayoutDensity::Senior) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    const auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan.window.width, plan.window.height, plan);
    CHECK(report.valid);
    CHECK(report.error == MainWindowLayoutError::None);
}

// TEST-003: testCanonicalCompactLayoutInvariants
void testCanonicalCompactLayoutInvariants() {
    const auto plan = MainWindowLayoutConfiguration::canonicalCompactLayout();
    CHECK(plan.density == LayoutDensity::Compact);
    CHECK(plan.window.minWidth == 500);
    CHECK(plan.window.minHeight == 380);
    CHECK(MainWindowLayoutConfiguration::verifyInvariants(plan));
    CHECK(MainWindowLayoutValidator::validatePlan(plan) == MainWindowLayoutError::None);
    const auto report = MainWindowLayoutValidator::calculateLayoutAreas(1024, 640, plan);
    CHECK(report.valid);
    CHECK(report.error == MainWindowLayoutError::None);
}

// TEST-004: testDimensionsValidationStandard
void testDimensionsValidationStandard() {
    WindowDimensions validDims{.width = 640, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(validDims, LayoutDensity::Default) ==
          MainWindowLayoutError::None);

    WindowDimensions largeDims{.width = 1920, .height = 1080, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(largeDims, LayoutDensity::Default) ==
          MainWindowLayoutError::None);

    WindowDimensions subWidth{.width = 639, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(subWidth, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions subHeight{.width = 640, .height = 479, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(subHeight, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions negativeDims{.width = -10, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(negativeDims, LayoutDensity::Default) ==
          MainWindowLayoutError::NegativeMarginOrPadding);
}

// TEST-005: testDimensionsValidationSenior
void testDimensionsValidationSenior() {
    WindowDimensions validDims{.width = 720, .height = 520, .minWidth = 720, .minHeight = 520};
    CHECK(MainWindowLayoutValidator::validateDimensions(validDims, LayoutDensity::Senior) ==
          MainWindowLayoutError::None);

    WindowDimensions subDims{.width = 719, .height = 519, .minWidth = 720, .minHeight = 520};
    CHECK(MainWindowLayoutValidator::validateDimensions(subDims, LayoutDensity::Senior) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions standardDimsOnSenior{
        .width = 640, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(standardDimsOnSenior, LayoutDensity::Senior) ==
          MainWindowLayoutError::DimensionsBelowMinimum);
}

// TEST-006: testSidebarValidationBounds
void testSidebarValidationBounds() {
    SidebarConfig collapsedSidebar{.state = SidebarState::Collapsed, .width = 48};
    CHECK(MainWindowLayoutValidator::validateSidebar(collapsedSidebar) == MainWindowLayoutError::None);

    SidebarConfig invalidCollapsed{.state = SidebarState::Collapsed, .width = 60};
    CHECK(MainWindowLayoutValidator::validateSidebar(invalidCollapsed) ==
          MainWindowLayoutError::InvalidSidebarWidth);

    SidebarConfig expandedMin{.state = SidebarState::Expanded, .width = 200};
    CHECK(MainWindowLayoutValidator::validateSidebar(expandedMin) == MainWindowLayoutError::None);

    SidebarConfig expandedTypical{.state = SidebarState::Expanded, .width = 240};
    CHECK(MainWindowLayoutValidator::validateSidebar(expandedTypical) == MainWindowLayoutError::None);

    SidebarConfig expandedMax{.state = SidebarState::Expanded, .width = 360};
    CHECK(MainWindowLayoutValidator::validateSidebar(expandedMax) == MainWindowLayoutError::None);

    SidebarConfig expandedUnder{.state = SidebarState::Expanded, .width = 199};
    CHECK(MainWindowLayoutValidator::validateSidebar(expandedUnder) ==
          MainWindowLayoutError::InvalidSidebarWidth);

    SidebarConfig expandedOver{.state = SidebarState::Expanded, .width = 361};
    CHECK(MainWindowLayoutValidator::validateSidebar(expandedOver) ==
          MainWindowLayoutError::InvalidSidebarWidth);

    SidebarConfig negativeSidebar{.width = -1};
    CHECK(MainWindowLayoutValidator::validateSidebar(negativeSidebar) ==
          MainWindowLayoutError::NegativeMarginOrPadding);
}

// TEST-007: testLayoutCalculationDisjointAreas
void testLayoutCalculationDisjointAreas() {
    const auto plan = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    const auto report = MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, plan);
    CHECK(report.valid);
    CHECK(report.error == MainWindowLayoutError::None);
    CHECK(!report.allocatedZones.empty());

    for (const auto& zoneGeom : report.allocatedZones) {
        CHECK(zoneGeom.rect.area() > 0);
        CHECK(zoneGeom.rect.width > 0);
        CHECK(zoneGeom.rect.height > 0);
        CHECK(zoneGeom.rect.x >= 0);
        CHECK(zoneGeom.rect.y >= 0);
        CHECK(zoneGeom.rect.x + zoneGeom.rect.width <= 1200);
        CHECK(zoneGeom.rect.y + zoneGeom.rect.height <= 780);
    }

    for (size_t i = 0; i < report.allocatedZones.size(); ++i) {
        for (size_t j = i + 1; j < report.allocatedZones.size(); ++j) {
            CHECK(!report.allocatedZones[i].rect.intersects(report.allocatedZones[j].rect));
        }
    }
}

// TEST-008: testLayoutCalculationWithBanner
void testLayoutCalculationWithBanner() {
    auto planWithoutBanner = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    planWithoutBanner.showBanner = false;
    const auto reportWithout =
        MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, planWithoutBanner);
    CHECK(reportWithout.valid);

    auto planWithBanner = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    planWithBanner.showBanner = true;
    const auto reportWith = MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, planWithBanner);
    CHECK(reportWith.valid);

    bool foundBanner = false;
    int bannerHeight = 0;
    for (const auto& zg : reportWith.allocatedZones) {
        if (zg.zone == LayoutZone::NotificationBanner) {
            foundBanner = true;
            bannerHeight = zg.rect.height;
            break;
        }
    }
    CHECK(foundBanner);
    CHECK(bannerHeight == MainWindowLayoutConfiguration::kCanonicalBannerHeightDefault);
    CHECK(reportWith.contentArea.height == reportWithout.contentArea.height - bannerHeight);
    CHECK(reportWith.contentArea.y == reportWithout.contentArea.y + bannerHeight);
}

// TEST-009: testLayoutCalculationDockLeftVsRight
void testLayoutCalculationDockLeftVsRight() {
    auto planLeft = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    planLeft.sidebar.position = SidebarDockPosition::Left;
    const auto reportLeft = MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, planLeft);
    CHECK(reportLeft.valid);

    auto planRight = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    planRight.sidebar.position = SidebarDockPosition::Right;
    const auto reportRight = MainWindowLayoutValidator::calculateLayoutAreas(1200, 780, planRight);
    CHECK(reportRight.valid);

    LayoutRect leftSidebarRect{};
    LayoutRect leftContentRect{};
    for (const auto& zg : reportLeft.allocatedZones) {
        if (zg.zone == LayoutZone::Sidebar) {
            leftSidebarRect = zg.rect;
        }
        if (zg.zone == LayoutZone::Content) {
            leftContentRect = zg.rect;
        }
    }
    CHECK(leftSidebarRect.x == 0);
    CHECK(leftSidebarRect.width == 48);
    CHECK(leftContentRect.x == 48);
    CHECK(leftContentRect.width == 1200 - 48);

    LayoutRect rightSidebarRect{};
    LayoutRect rightContentRect{};
    for (const auto& zg : reportRight.allocatedZones) {
        if (zg.zone == LayoutZone::Sidebar) {
            rightSidebarRect = zg.rect;
        }
        if (zg.zone == LayoutZone::Content) {
            rightContentRect = zg.rect;
        }
    }
    CHECK(rightContentRect.x == 0);
    CHECK(rightContentRect.width == 1200 - 48);
    CHECK(rightSidebarRect.x == 1200 - 48);
    CHECK(rightSidebarRect.width == 48);
}

// TEST-010: testZeroContentAreaBudgetRejection
void testZeroContentAreaBudgetRejection() {
    auto plan = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    plan.zones = {ZoneBudget{LayoutZone::ToolBar, true, 260, 260, 300, 0, 0, 0}};
    const auto report = MainWindowLayoutValidator::calculateLayoutAreas(640, 480, plan);
    CHECK(!report.valid);
    CHECK(report.error == MainWindowLayoutError::ZeroContentAreaBudget);
    CHECK(!report.message.empty());
    CHECK(!report.remediationHint.empty());
}

// TEST-011: testUxCopySpanishCompliance
void testUxCopySpanishCompliance() {
    CHECK(MainWindowLayoutValidator::layoutDensityToString(LayoutDensity::Senior) ==
          "Interfaz más fácil de usar");
    CHECK(MainWindowLayoutValidator::layoutErrorToString(MainWindowLayoutError::None) ==
          "Disposición correcta sin anomalías");

    const auto checkStringCompliance = [](std::string_view s) {
        CHECK(!s.empty());
        std::string lower;
        lower.reserve(s.size());
        for (char c : s) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        CHECK(lower.find("senior mode") == std::string::npos);
        CHECK(lower.find("modo senior") == std::string::npos);
        CHECK(lower.find("prompt builder") == std::string::npos);
        CHECK(lower.find("dag") == std::string::npos);
        CHECK(lower.find("openspec") == std::string::npos);
    };

    const MainWindowLayoutError errors[] = {MainWindowLayoutError::None,
                                            MainWindowLayoutError::DimensionsBelowMinimum,
                                            MainWindowLayoutError::InvalidSidebarWidth,
                                            MainWindowLayoutError::ConflictingZoneAnchors,
                                            MainWindowLayoutError::ZeroContentAreaBudget,
                                            MainWindowLayoutError::NegativeMarginOrPadding,
                                            MainWindowLayoutError::InvalidDensityConfiguration,
                                            MainWindowLayoutError::LayoutPersistenceCorrupted};
    for (auto err : errors) {
        checkStringCompliance(MainWindowLayoutValidator::layoutErrorToString(err));
        checkStringCompliance(MainWindowLayoutValidator::generateRemediationHint(err));
        checkStringCompliance(
            MainWindowLayoutValidator::generateRemediationHint(err, "Información complementaria"));
    }

    const LayoutZone zones[] = {
        LayoutZone::TitleBar, LayoutZone::TabStrip, LayoutZone::ToolBar,  LayoutZone::NotificationBanner,
        LayoutZone::Sidebar,  LayoutZone::Content,  LayoutZone::StatusBar};
    for (auto z : zones) {
        checkStringCompliance(MainWindowLayoutValidator::layoutZoneToString(z));
    }

    const LayoutDensity densities[] = {LayoutDensity::Default, LayoutDensity::Senior,
                                       LayoutDensity::Compact};
    for (auto d : densities) {
        checkStringCompliance(MainWindowLayoutValidator::layoutDensityToString(d));
    }
    CHECK(MainWindowLayoutValidator::layoutDensityToString(LayoutDensity::Senior) ==
          "Interfaz más fácil de usar");

    const SidebarState states[] = {SidebarState::Hidden, SidebarState::Collapsed,
                                   SidebarState::Expanded};
    for (auto st : states) {
        checkStringCompliance(MainWindowLayoutValidator::sidebarStateToString(st));
    }

    const SidebarDockPosition docks[] = {SidebarDockPosition::Left, SidebarDockPosition::Right};
    for (auto dk : docks) {
        checkStringCompliance(MainWindowLayoutValidator::sidebarDockPositionToString(dk));
    }

    const WindowChromeMode chromes[] = {WindowChromeMode::Standard, WindowChromeMode::Integrated,
                                        WindowChromeMode::Fullscreen};
    for (auto cm : chromes) {
        checkStringCompliance(MainWindowLayoutValidator::chromeModeToString(cm));
    }
}

// TEST-012: testDynamicDensityTransition
void testDynamicDensityTransition() {
    const auto planDefault = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    const auto reportDefault = MainWindowLayoutValidator::calculateLayoutAreas(1280, 840, planDefault);
    CHECK(reportDefault.valid);

    const auto planSenior = MainWindowLayoutConfiguration::canonicalSeniorLayout();
    const auto reportSenior = MainWindowLayoutValidator::calculateLayoutAreas(1280, 840, planSenior);
    CHECK(reportSenior.valid);
    CHECK(reportSenior.contentArea.area() > 0);
    CHECK(reportSenior.contentArea.height < reportDefault.contentArea.height);

    const auto planCompact = MainWindowLayoutConfiguration::canonicalCompactLayout();
    const auto reportCompact = MainWindowLayoutValidator::calculateLayoutAreas(1280, 840, planCompact);
    CHECK(reportCompact.valid);
    CHECK(reportCompact.contentArea.height > reportDefault.contentArea.height);
}

// TEST-013: testSubMinimumDimensionsExhaustive
void testSubMinimumDimensionsExhaustive() {
    // Default density boundary checks (min: 640x480)
    WindowDimensions subWidthDef{.width = 639, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(subWidthDef, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions subHeightDef{.width = 640, .height = 479, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(subHeightDef, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    const auto planDef = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    const auto repSubWDef = MainWindowLayoutValidator::calculateLayoutAreas(639, 480, planDef);
    CHECK(!repSubWDef.valid);
    CHECK(repSubWDef.error == MainWindowLayoutError::DimensionsBelowMinimum);

    const auto repSubHDef = MainWindowLayoutValidator::calculateLayoutAreas(640, 479, planDef);
    CHECK(!repSubHDef.valid);
    CHECK(repSubHDef.error == MainWindowLayoutError::DimensionsBelowMinimum);

    // Senior density boundary checks (min: 720x520)
    WindowDimensions subWidthSen{.width = 719, .height = 520, .minWidth = 720, .minHeight = 520};
    CHECK(MainWindowLayoutValidator::validateDimensions(subWidthSen, LayoutDensity::Senior) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions subHeightSen{.width = 720, .height = 519, .minWidth = 720, .minHeight = 520};
    CHECK(MainWindowLayoutValidator::validateDimensions(subHeightSen, LayoutDensity::Senior) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    const auto planSen = MainWindowLayoutConfiguration::canonicalSeniorLayout();
    const auto repSubWSen = MainWindowLayoutValidator::calculateLayoutAreas(719, 520, planSen);
    CHECK(!repSubWSen.valid);
    CHECK(repSubWSen.error == MainWindowLayoutError::DimensionsBelowMinimum);

    const auto repSubHSen = MainWindowLayoutValidator::calculateLayoutAreas(720, 519, planSen);
    CHECK(!repSubHSen.valid);
    CHECK(repSubHSen.error == MainWindowLayoutError::DimensionsBelowMinimum);

    // Compact density boundary checks (min: 500x380)
    WindowDimensions subWidthCmp{.width = 499, .height = 380, .minWidth = 500, .minHeight = 380};
    CHECK(MainWindowLayoutValidator::validateDimensions(subWidthCmp, LayoutDensity::Compact) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions subHeightCmp{.width = 500, .height = 379, .minWidth = 500, .minHeight = 380};
    CHECK(MainWindowLayoutValidator::validateDimensions(subHeightCmp, LayoutDensity::Compact) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    const auto planCmp = MainWindowLayoutConfiguration::canonicalCompactLayout();
    const auto repSubWCmp = MainWindowLayoutValidator::calculateLayoutAreas(499, 380, planCmp);
    CHECK(!repSubWCmp.valid);
    CHECK(repSubWCmp.error == MainWindowLayoutError::DimensionsBelowMinimum);

    const auto repSubHCmp = MainWindowLayoutValidator::calculateLayoutAreas(500, 379, planCmp);
    CHECK(!repSubHCmp.valid);
    CHECK(repSubHCmp.error == MainWindowLayoutError::DimensionsBelowMinimum);
}

// TEST-014: testZeroAndNegativeDimensions
void testZeroAndNegativeDimensions() {
    // Zero dimensions return DimensionsBelowMinimum
    WindowDimensions zeroDims{.width = 0, .height = 0, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(zeroDims, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions zeroWidth{.width = 0, .height = 480, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(zeroWidth, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    WindowDimensions zeroHeight{.width = 640, .height = 0, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(zeroHeight, LayoutDensity::Default) ==
          MainWindowLayoutError::DimensionsBelowMinimum);

    const auto planDef = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    const auto repZero = MainWindowLayoutValidator::calculateLayoutAreas(0, 0, planDef);
    CHECK(!repZero.valid);
    CHECK(repZero.error == MainWindowLayoutError::DimensionsBelowMinimum);

    // Negative dimensions return NegativeMarginOrPadding
    WindowDimensions negWidth{.width = -10, .height = 500, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(negWidth, LayoutDensity::Default) ==
          MainWindowLayoutError::NegativeMarginOrPadding);

    WindowDimensions negHeight{.width = 800, .height = -5, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(negHeight, LayoutDensity::Default) ==
          MainWindowLayoutError::NegativeMarginOrPadding);

    WindowDimensions negBoth{.width = -1, .height = -1, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(negBoth, LayoutDensity::Default) ==
          MainWindowLayoutError::NegativeMarginOrPadding);

    const auto repNeg = MainWindowLayoutValidator::calculateLayoutAreas(-10, 500, planDef);
    CHECK(!repNeg.valid);
    CHECK(repNeg.error == MainWindowLayoutError::NegativeMarginOrPadding);
}

// TEST-015: testInvalidSidebarWidthBoundaries
void testInvalidSidebarWidthBoundaries() {
    // Range 0..47 px for collapsed sidebar
    const int prohibitedBelowCollapsed[] = {0, 1, 10, 24, 47};
    for (int w : prohibitedBelowCollapsed) {
        SidebarConfig sb{.state = SidebarState::Collapsed, .width = w};
        CHECK(MainWindowLayoutValidator::validateSidebar(sb) ==
              MainWindowLayoutError::InvalidSidebarWidth);
    }

    // Range 49..199 px (forbidden gap between collapsed and min expanded)
    const int prohibitedGap[] = {49, 50, 75, 100, 120, 150, 180, 199};
    for (int w : prohibitedGap) {
        SidebarConfig sbCol{.state = SidebarState::Collapsed, .width = w};
        CHECK(MainWindowLayoutValidator::validateSidebar(sbCol) ==
              MainWindowLayoutError::InvalidSidebarWidth);

        SidebarConfig sbExp{.state = SidebarState::Expanded, .width = w};
        CHECK(MainWindowLayoutValidator::validateSidebar(sbExp) ==
              MainWindowLayoutError::InvalidSidebarWidth);
    }

    // Range > 360 px (above max expanded)
    const int prohibitedAboveMax[] = {361, 380, 400, 500, 1000};
    for (int w : prohibitedAboveMax) {
        SidebarConfig sbExp{.state = SidebarState::Expanded, .width = w};
        CHECK(MainWindowLayoutValidator::validateSidebar(sbExp) ==
              MainWindowLayoutError::InvalidSidebarWidth);
    }

    // Remediation hint validation
    const auto hint =
        MainWindowLayoutValidator::generateRemediationHint(MainWindowLayoutError::InvalidSidebarWidth);
    CHECK(!hint.empty());
    CHECK(hint.find("200 y 360") != std::string::npos);
    CHECK(hint.find("48") != std::string::npos);
}

// TEST-016: testZoneBudgetValidationFailures
void testZoneBudgetValidationFailures() {
    // Negative dimensions in WindowDimensions
    WindowDimensions negDims{.width = 1000, .height = -10, .minWidth = 640, .minHeight = 480};
    CHECK(MainWindowLayoutValidator::validateDimensions(negDims, LayoutDensity::Default) ==
          MainWindowLayoutError::NegativeMarginOrPadding);

    // Negative widths in SidebarConfig
    SidebarConfig negSidebar{.width = -20};
    CHECK(MainWindowLayoutValidator::validateSidebar(negSidebar) ==
          MainWindowLayoutError::NegativeMarginOrPadding);

    // Total fixed zone heights exceeding window height -> ZeroContentAreaBudget
    auto plan = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    plan.window.height = 480;
    plan.zones = {
        ZoneBudget{LayoutZone::TitleBar, true, 40, 40, 40, 0, 0, 0},
        ZoneBudget{LayoutZone::TabStrip, true, 48, 48, 48, 0, 0, 0},
        ZoneBudget{LayoutZone::ToolBar, true, 300, 300, 300, 0, 0, 0},
        ZoneBudget{LayoutZone::NotificationBanner, true, 50, 50, 50, 0, 0, 0},
    };
    const auto report = MainWindowLayoutValidator::calculateLayoutAreas(640, 480, plan);
    CHECK(!report.valid);
    CHECK(report.error == MainWindowLayoutError::ZeroContentAreaBudget);
    CHECK(!report.message.empty());
    CHECK(!report.remediationHint.empty());

    // In validatePlan, if fixed zones exceed available window dimensions
    auto fullPlan = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    fullPlan.window.width = 640;
    fullPlan.window.height = 480;
    fullPlan.density = LayoutDensity::Default;
    fullPlan.sidebar.state = SidebarState::Expanded;
    fullPlan.sidebar.width = 300; // 640 - 300 = 340 < 400 (kMinContentWidth)
    CHECK(MainWindowLayoutValidator::validatePlan(fullPlan) ==
          MainWindowLayoutError::ZeroContentAreaBudget);
}

// TEST-017: testExtremeWindowGeometries
void testExtremeWindowGeometries() {
    const struct ExtremeResolution {
        int width;
        int height;
        const char* label;
    } resolutions[] = {
        {3840, 1080, "Ultrawide 32:9/21:9"},
        {1080, 1920, "Vertical 9:16"},
        {2560, 1440, "QHD 16:9"},
        {3840, 2160, "4K UHD"},
        {5120, 1440, "Super Ultrawide"},
    };

    const auto densities = {LayoutDensity::Default, LayoutDensity::Senior, LayoutDensity::Compact};

    for (const auto& res : resolutions) {
        for (auto dens : densities) {
            MainWindowLayoutPlan plan = (dens == LayoutDensity::Senior)
                                            ? MainWindowLayoutConfiguration::canonicalSeniorLayout()
                                        : (dens == LayoutDensity::Compact)
                                            ? MainWindowLayoutConfiguration::canonicalCompactLayout()
                                            : MainWindowLayoutConfiguration::canonicalDefaultLayout();

            const auto report =
                MainWindowLayoutValidator::calculateLayoutAreas(res.width, res.height, plan);
            CHECK(report.valid);
            CHECK(report.error == MainWindowLayoutError::None);
            CHECK(report.contentArea.width >= 400);
            CHECK(report.contentArea.height >= 250);
            CHECK(report.contentArea.area() > 0);

            // Disjointness check
            for (size_t i = 0; i < report.allocatedZones.size(); ++i) {
                const auto& zi = report.allocatedZones[i];
                CHECK(zi.rect.x >= 0);
                CHECK(zi.rect.y >= 0);
                CHECK(zi.rect.x + zi.rect.width <= res.width);
                CHECK(zi.rect.y + zi.rect.height <= res.height);

                for (size_t j = i + 1; j < report.allocatedZones.size(); ++j) {
                    const auto& zj = report.allocatedZones[j];
                    CHECK(!zi.rect.intersects(zj.rect));
                }
            }
        }
    }
}

// TEST-018: testPublicCopyAndRemediationTextAudit
void testPublicCopyAndRemediationTextAudit() {
    const auto auditString = [](std::string_view s) {
        CHECK(!s.empty());
        std::string lower;
        lower.reserve(s.size());
        for (char c : s) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        CHECK(lower.find("senior mode") == std::string::npos);
        CHECK(lower.find("modo senior") == std::string::npos);
        CHECK(lower.find("prompt builder") == std::string::npos);
        CHECK(lower.find("openspec") == std::string::npos);
        CHECK(lower.find("dag") == std::string::npos);
        CHECK(lower.find("linter") == std::string::npos);
        CHECK(lower.find("toolchain") == std::string::npos);
        CHECK(lower.find("runner") == std::string::npos);
        CHECK(lower.find("micro-prompt") == std::string::npos);
    };

    // Audit layoutZoneToString
    const LayoutZone zones[] = {
        LayoutZone::TitleBar, LayoutZone::TabStrip, LayoutZone::ToolBar,  LayoutZone::NotificationBanner,
        LayoutZone::Sidebar,  LayoutZone::Content,  LayoutZone::StatusBar};
    for (auto z : zones) {
        auditString(MainWindowLayoutValidator::layoutZoneToString(z));
    }

    // Audit layoutDensityToString
    const LayoutDensity densities[] = {LayoutDensity::Default, LayoutDensity::Senior,
                                       LayoutDensity::Compact};
    for (auto d : densities) {
        auditString(MainWindowLayoutValidator::layoutDensityToString(d));
    }
    CHECK(MainWindowLayoutValidator::layoutDensityToString(LayoutDensity::Senior) ==
          "Interfaz más fácil de usar");

    // Audit sidebarStateToString
    const SidebarState states[] = {SidebarState::Hidden, SidebarState::Collapsed,
                                   SidebarState::Expanded};
    for (auto st : states) {
        auditString(MainWindowLayoutValidator::sidebarStateToString(st));
    }

    // Audit sidebarDockPositionToString
    const SidebarDockPosition docks[] = {SidebarDockPosition::Left, SidebarDockPosition::Right};
    for (auto dk : docks) {
        auditString(MainWindowLayoutValidator::sidebarDockPositionToString(dk));
    }

    // Audit chromeModeToString
    const WindowChromeMode chromes[] = {WindowChromeMode::Standard, WindowChromeMode::Integrated,
                                        WindowChromeMode::Fullscreen};
    for (auto cm : chromes) {
        auditString(MainWindowLayoutValidator::chromeModeToString(cm));
    }

    // Audit layoutErrorToString & generateRemediationHint
    const MainWindowLayoutError errors[] = {MainWindowLayoutError::None,
                                            MainWindowLayoutError::DimensionsBelowMinimum,
                                            MainWindowLayoutError::InvalidSidebarWidth,
                                            MainWindowLayoutError::ConflictingZoneAnchors,
                                            MainWindowLayoutError::ZeroContentAreaBudget,
                                            MainWindowLayoutError::NegativeMarginOrPadding,
                                            MainWindowLayoutError::InvalidDensityConfiguration,
                                            MainWindowLayoutError::LayoutPersistenceCorrupted};
    for (auto err : errors) {
        auditString(MainWindowLayoutValidator::layoutErrorToString(err));
        auditString(MainWindowLayoutValidator::generateRemediationHint(err));
        auditString(MainWindowLayoutValidator::generateRemediationHint(err, "Detalle adicional"));
    }
}

} // namespace

int main() {
    try {
        std::cout << "[TEST] Running Main Window Layout Unit Tests...\n";

        testCanonicalDefaultLayoutInvariants();
        std::cout << "  ✓ TEST-001: testCanonicalDefaultLayoutInvariants passed.\n";

        testCanonicalSeniorLayoutInvariants();
        std::cout << "  ✓ TEST-002: testCanonicalSeniorLayoutInvariants passed.\n";

        testCanonicalCompactLayoutInvariants();
        std::cout << "  ✓ TEST-003: testCanonicalCompactLayoutInvariants passed.\n";

        testDimensionsValidationStandard();
        std::cout << "  ✓ TEST-004: testDimensionsValidationStandard passed.\n";

        testDimensionsValidationSenior();
        std::cout << "  ✓ TEST-005: testDimensionsValidationSenior passed.\n";

        testSidebarValidationBounds();
        std::cout << "  ✓ TEST-006: testSidebarValidationBounds passed.\n";

        testLayoutCalculationDisjointAreas();
        std::cout << "  ✓ TEST-007: testLayoutCalculationDisjointAreas passed.\n";

        testLayoutCalculationWithBanner();
        std::cout << "  ✓ TEST-008: testLayoutCalculationWithBanner passed.\n";

        testLayoutCalculationDockLeftVsRight();
        std::cout << "  ✓ TEST-009: testLayoutCalculationDockLeftVsRight passed.\n";

        testZeroContentAreaBudgetRejection();
        std::cout << "  ✓ TEST-010: testZeroContentAreaBudgetRejection passed.\n";

        testUxCopySpanishCompliance();
        std::cout << "  ✓ TEST-011: testUxCopySpanishCompliance passed.\n";

        testDynamicDensityTransition();
        std::cout << "  ✓ TEST-012: testDynamicDensityTransition passed.\n";

        testSubMinimumDimensionsExhaustive();
        std::cout << "  ✓ TEST-013: testSubMinimumDimensionsExhaustive passed.\n";

        testZeroAndNegativeDimensions();
        std::cout << "  ✓ TEST-014: testZeroAndNegativeDimensions passed.\n";

        testInvalidSidebarWidthBoundaries();
        std::cout << "  ✓ TEST-015: testInvalidSidebarWidthBoundaries passed.\n";

        testZoneBudgetValidationFailures();
        std::cout << "  ✓ TEST-016: testZoneBudgetValidationFailures passed.\n";

        testExtremeWindowGeometries();
        std::cout << "  ✓ TEST-017: testExtremeWindowGeometries passed.\n";

        testPublicCopyAndRemediationTextAudit();
        std::cout << "  ✓ TEST-018: testPublicCopyAndRemediationTextAudit passed.\n";

        std::cout << "[SUCCESS] All 18 Main Window Layout Unit Tests passed cleanly.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FAILURE] Exception during tests: " << ex.what() << '\n';
        return 1;
    }
}
