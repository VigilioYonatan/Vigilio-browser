#include "core/QtDependencyValidator.h"

#include <atomic>
#include <cstdint>
#include <iostream>
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

void test_version_parsing_positive() {
    // Standard 3-part version
    const auto v6100 = SemanticVersion::parse("6.10.0");
    CHECK(v6100.has_value());
    CHECK(v6100->major == 6 && v6100->minor == 10 && v6100->patch == 0);
    CHECK(v6100->toString() == "6.10.0");

    // Incremented patch
    const auto v6104 = SemanticVersion::parse("6.10.4");
    CHECK(v6104.has_value());
    CHECK(v6104->major == 6 && v6104->minor == 10 && v6104->patch == 4);
    CHECK(v6104->toString() == "6.10.4");

    // 2-part version normalized with patch 0
    const auto vTwoPart = SemanticVersion::parse("6.10");
    CHECK(vTwoPart.has_value());
    CHECK(vTwoPart->major == 6 && vTwoPart->minor == 10 && vTwoPart->patch == 0);
    CHECK(vTwoPart->toString() == "6.10.0");

    // Literal 'v' prefix
    const auto vPrefixed = SemanticVersion::parse("v6.12.3");
    CHECK(vPrefixed.has_value());
    CHECK(vPrefixed->major == 6 && vPrefixed->minor == 12 && vPrefixed->patch == 3);

    const auto vUpperPrefixed = SemanticVersion::parse("V6.10.1");
    CHECK(vUpperPrefixed.has_value());
    CHECK(vUpperPrefixed->major == 6 && vUpperPrefixed->minor == 10 && vUpperPrefixed->patch == 1);

    // Peripheral whitespace trimming
    const auto vPadded = SemanticVersion::parse("  6.10.2  ");
    CHECK(vPadded.has_value());
    CHECK(vPadded->major == 6 && vPadded->minor == 10 && vPadded->patch == 2);
    CHECK(vPadded->toString() == "6.10.2");

    // Pre-release suffix
    const auto vBeta = SemanticVersion::parse("6.10.1-beta");
    CHECK(vBeta.has_value());
    CHECK(vBeta->major == 6 && vBeta->minor == 10 && vBeta->patch == 1);

    // Build metadata suffix
    const auto vBuild = SemanticVersion::parse("6.10.0+build2026");
    CHECK(vBuild.has_value());
    CHECK(vBuild->major == 6 && vBuild->minor == 10 && vBuild->patch == 0);

    const auto vBuildDotted = SemanticVersion::parse("6.10.0+build.2026");
    CHECK(vBuildDotted.has_value());
    CHECK(vBuildDotted->major == 6 && vBuildDotted->minor == 10 && vBuildDotted->patch == 0);

    // QtDependencyValidator::parseVersion parity
    const auto vViaClass = QtDependencyValidator::parseVersion("6.10.0");
    CHECK(vViaClass.has_value());
    CHECK(*vViaClass == *v6100);
}

void test_version_parsing_negative() {
    // Empty and whitespace only
    CHECK(!SemanticVersion::parse("").has_value());
    CHECK(!SemanticVersion::parse("   ").has_value());
    CHECK(!SemanticVersion::parse("\t\r\n").has_value());

    // Non-numeric strings
    CHECK(!SemanticVersion::parse("abc").has_value());
    CHECK(!SemanticVersion::parse("qt6").has_value());
    CHECK(!SemanticVersion::parse("invalid.version").has_value());

    // Malformed punctuation and incomplete dots
    CHECK(!SemanticVersion::parse("6.").has_value());
    CHECK(!SemanticVersion::parse(".6.10").has_value());
    CHECK(!SemanticVersion::parse("6..10").has_value());
    CHECK(!SemanticVersion::parse("..6.10").has_value());
    CHECK(!SemanticVersion::parse("6.10.").has_value());
    CHECK(!SemanticVersion::parse("6.10..0").has_value());

    // Invalid intermediate characters
    CHECK(!SemanticVersion::parse("6.abc").has_value());
    CHECK(!SemanticVersion::parse("6.abc.0").has_value());
    CHECK(!SemanticVersion::parse("6.10a").has_value());

    // Integer overflow protection
    CHECK(!SemanticVersion::parse("99999999999999999999.0.0").has_value());
    CHECK(!SemanticVersion::parse("6.99999999999999999999.0").has_value());
    CHECK(!SemanticVersion::parse("6.10.99999999999999999999").has_value());
}

void test_version_comparison_and_compatibility() {
    const SemanticVersion v5152{5, 15, 2};
    const SemanticVersion v693{6, 9, 3};
    const SemanticVersion v6100{6, 10, 0};
    const SemanticVersion v6101{6, 10, 1};
    const SemanticVersion v6102{6, 10, 2};
    const SemanticVersion v6110{6, 11, 0};
    const SemanticVersion v700{7, 0, 0};

    // Reflexivity
    CHECK(v6100 == v6100);
    CHECK(v6100 <= v6100);
    CHECK(v6100 >= v6100);
    CHECK(!(v6100 < v6100));
    CHECK(!(v6100 > v6100));

    // Total ordering
    CHECK(v693 < v6100);
    CHECK(v6100 < v6101);
    CHECK(v6101 < v6102);
    CHECK(v6102 < v6110);
    CHECK(v6110 < v700);

    CHECK(v700 > v6110);
    CHECK(v6110 > v6102);
    CHECK(v6102 > v6101);
    CHECK(v6101 > v6100);
    CHECK(v6100 > v693);

    // Qt6 semantic compatibility predicate (Qt >= 6.10.0 within major 6)
    CHECK(v6100.isCompatibleWith(v6100));
    CHECK(v6101.isCompatibleWith(v6100));
    CHECK(v6102.isCompatibleWith(v6100));
    CHECK(v6110.isCompatibleWith(v6100));

    // Incompatible: older version within same major
    CHECK(!v693.isCompatibleWith(v6100));

    // Incompatible: different major versions
    CHECK(!v5152.isCompatibleWith(v6100));
    CHECK(!v700.isCompatibleWith(v6100));
}

void test_profile_evaluation_positive() {
    const SemanticVersion v6100{6, 10, 0};
    const SemanticVersion v6102{6, 10, 2};

    // HeadlessCore: requires no Qt modules
    const auto headlessReq = QtDependencyValidator::requiredModules(QtRequirementProfile::HeadlessCore);
    CHECK(headlessReq.empty());
    const auto repHeadless =
        QtDependencyValidator::validateProfile(QtRequirementProfile::HeadlessCore, {}, {0, 0, 0});
    CHECK(repHeadless.outcome == QtValidationOutcome::Satisfied);
    CHECK(repHeadless.missingModules.empty());

    // MinimalGui: requires Core + Widgets
    const auto minGuiReq = QtDependencyValidator::requiredModules(QtRequirementProfile::MinimalGui);
    CHECK(minGuiReq.size() == 2);
    CHECK(minGuiReq[0] == QtModuleId::Core && minGuiReq[1] == QtModuleId::Widgets);

    const auto repMinGui = QtDependencyValidator::validateProfile(
        QtRequirementProfile::MinimalGui, {QtModuleId::Core, QtModuleId::Widgets}, v6100);
    CHECK(repMinGui.outcome == QtValidationOutcome::Satisfied);
    CHECK(repMinGui.missingModules.empty());

    // Order-insensitive module declaration
    const auto repMinGuiReversed = QtDependencyValidator::validateProfile(
        QtRequirementProfile::MinimalGui, {QtModuleId::Widgets, QtModuleId::Core}, v6100);
    CHECK(repMinGuiReversed.outcome == QtValidationOutcome::Satisfied);

    // Extra modules present
    const auto repMinGuiExtra = QtDependencyValidator::validateProfile(
        QtRequirementProfile::MinimalGui,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::Svg, QtModuleId::Test}, v6100);
    CHECK(repMinGuiExtra.outcome == QtValidationOutcome::Satisfied);

    // FullBrowser: requires Core + Widgets + WebEngineWidgets
    const auto fullReq = QtDependencyValidator::requiredModules(QtRequirementProfile::FullBrowser);
    CHECK(fullReq.size() == 3);

    const auto repFull = QtDependencyValidator::validateProfile(
        QtRequirementProfile::FullBrowser,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v6102);
    CHECK(repFull.outcome == QtValidationOutcome::Satisfied);
    CHECK(repFull.missingModules.empty());

    // DeveloperTest: requires Core + Widgets + WebEngineWidgets + Test
    const auto devTestReq = QtDependencyValidator::requiredModules(QtRequirementProfile::DeveloperTest);
    CHECK(devTestReq.size() == 4);

    const auto repDevTest = QtDependencyValidator::validateProfile(
        QtRequirementProfile::DeveloperTest,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets, QtModuleId::Test}, v6100);
    CHECK(repDevTest.outcome == QtValidationOutcome::Satisfied);
    CHECK(repDevTest.missingModules.empty());
}

void test_profile_evaluation_negative() {
    const SemanticVersion v6100{6, 10, 0};
    const SemanticVersion v693{6, 9, 3};
    const SemanticVersion v5152{5, 15, 2};
    const SemanticVersion v700{7, 0, 0};

    // MinimalGui missing Widgets
    const auto repMinMissing = QtDependencyValidator::validateProfile(QtRequirementProfile::MinimalGui,
                                                                      {QtModuleId::Core}, v6100);
    CHECK(repMinMissing.outcome == QtValidationOutcome::MissingComponent);
    CHECK(repMinMissing.missingModules.size() == 1);
    CHECK(repMinMissing.missingModules[0] == QtModuleId::Widgets);

    // MinimalGui missing both Core and Widgets
    const auto repMinEmpty =
        QtDependencyValidator::validateProfile(QtRequirementProfile::MinimalGui, {}, v6100);
    CHECK(repMinEmpty.outcome == QtValidationOutcome::MissingComponent);
    CHECK(repMinEmpty.missingModules.size() == 2);

    // FullBrowser missing WebEngineWidgets
    const auto repFullMissing = QtDependencyValidator::validateProfile(
        QtRequirementProfile::FullBrowser, {QtModuleId::Core, QtModuleId::Widgets}, v6100);
    CHECK(repFullMissing.outcome == QtValidationOutcome::MissingComponent);
    CHECK(repFullMissing.missingModules.size() == 1);
    CHECK(repFullMissing.missingModules[0] == QtModuleId::WebEngineWidgets);

    // DeveloperTest missing Test module
    const auto repDevMissing = QtDependencyValidator::validateProfile(
        QtRequirementProfile::DeveloperTest,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v6100);
    CHECK(repDevMissing.outcome == QtValidationOutcome::MissingComponent);
    CHECK(repDevMissing.missingModules.size() == 1);
    CHECK(repDevMissing.missingModules[0] == QtModuleId::Test);

    // Incompatible version: older minor/patch (6.9.3 < 6.10.0)
    const auto repIncompatibleOlder = QtDependencyValidator::validateProfile(
        QtRequirementProfile::FullBrowser,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v693);
    CHECK(repIncompatibleOlder.outcome == QtValidationOutcome::IncompatibleVersion);

    // Incompatible version: major mismatch (5.15.2 and 7.0.0)
    const auto repIncompatibleQt5 = QtDependencyValidator::validateProfile(
        QtRequirementProfile::FullBrowser,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v5152);
    CHECK(repIncompatibleQt5.outcome == QtValidationOutcome::IncompatibleVersion);

    const auto repIncompatibleQt7 = QtDependencyValidator::validateProfile(
        QtRequirementProfile::FullBrowser,
        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v700);
    CHECK(repIncompatibleQt7.outcome == QtValidationOutcome::IncompatibleVersion);
}

void test_runtime_environment_validation() {
    // Normal cases: X11 display active
    const auto runX11 =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "6.10.0", ":0", nullptr);
    CHECK(runX11.outcome == QtValidationOutcome::Satisfied);
    CHECK(runX11.foundVersion.has_value());
    CHECK(runX11.foundVersion->toString() == "6.10.0");

    // Normal cases: Wayland display active
    const auto runWayland =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.2", "6.10.0", nullptr, "wayland-0");
    CHECK(runWayland.outcome == QtValidationOutcome::Satisfied);

    // Normal cases: Dual display active
    const auto runDual =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.1", "6.10.1", ":1", "wayland-1");
    CHECK(runDual.outcome == QtValidationOutcome::Satisfied);

    // Malformed runtime version string
    const auto runMalformedRuntime =
        QtDependencyValidator::validateRuntimeEnvironment("not-a-version", "6.10.0", ":0", nullptr);
    CHECK(runMalformedRuntime.outcome == QtValidationOutcome::EnvironmentError);

    // Empty runtime version string
    const auto runEmptyRuntime =
        QtDependencyValidator::validateRuntimeEnvironment("", "6.10.0", ":0", nullptr);
    CHECK(runEmptyRuntime.outcome == QtValidationOutcome::EnvironmentError);

    // Malformed compile version string
    const auto runMalformedCompile =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "invalid-compile", ":0", nullptr);
    CHECK(runMalformedCompile.outcome == QtValidationOutcome::EnvironmentError);

    // ABI major version mismatch (Qt 7 runtime vs Qt 6 compile)
    const auto runAbiMismatchHigh =
        QtDependencyValidator::validateRuntimeEnvironment("7.0.0", "6.10.0", ":0", nullptr);
    CHECK(runAbiMismatchHigh.outcome == QtValidationOutcome::EnvironmentError);

    // ABI major version mismatch (Qt 5 runtime vs Qt 6 compile)
    const auto runAbiMismatchLow =
        QtDependencyValidator::validateRuntimeEnvironment("5.15.2", "6.10.0", ":0", nullptr);
    CHECK(runAbiMismatchLow.outcome == QtValidationOutcome::EnvironmentError);

    // Incompatible runtime version (< 6.10.0)
    const auto runIncompatibleVer =
        QtDependencyValidator::validateRuntimeEnvironment("6.9.3", "6.9.3", ":0", nullptr);
    CHECK(runIncompatibleVer.outcome == QtValidationOutcome::IncompatibleVersion);

    // Missing display server in Linux (both nullptr)
    const auto runNoDisplayNull =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "6.10.0", nullptr, nullptr);
    CHECK(runNoDisplayNull.outcome == QtValidationOutcome::EnvironmentError);

    // Missing display server in Linux (both empty strings)
    const auto runNoDisplayEmpty =
        QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "6.10.0", "", "");
    CHECK(runNoDisplayEmpty.outcome == QtValidationOutcome::EnvironmentError);
}

void test_concurrency_and_thread_safety() {
    constexpr int kNumThreads = 8;
    constexpr int kIterations = 100;
    std::atomic<bool> allPassed{true};
    std::vector<std::jthread> workers;
    workers.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        workers.emplace_back([&allPassed, t]() {
            try {
                for (int i = 0; i < kIterations; ++i) {
                    const auto rep = QtDependencyValidator::validateProfile(
                        QtRequirementProfile::FullBrowser,
                        {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets},
                        SemanticVersion{6, 10, 0});
                    if (rep.outcome != QtValidationOutcome::Satisfied) {
                        allPassed.store(false, std::memory_order_relaxed);
                    }

                    const char* display = (t % 2 == 0) ? ":0" : nullptr;
                    const char* wayland = (t % 2 != 0) ? "wayland-0" : nullptr;
                    const auto rRep = QtDependencyValidator::validateRuntimeEnvironment(
                        "6.10.2", "6.10.0", display, wayland);
                    if (rRep.outcome != QtValidationOutcome::Satisfied) {
                        allPassed.store(false, std::memory_order_relaxed);
                    }

                    const auto v = SemanticVersion::parse("  v6.10.1-beta  ");
                    if (!v.has_value() || v->major != 6 || v->minor != 10 || v->patch != 1) {
                        allPassed.store(false, std::memory_order_relaxed);
                    }
                }
            } catch (...) {
                allPassed.store(false, std::memory_order_relaxed);
            }
        });
    }

    workers.clear();
    CHECK(allPassed.load());
}

void test_diagnostic_formatting() {
    // Module name formatting
    CHECK(QtDependencyValidator::moduleName(QtModuleId::Core) == "Core");
    CHECK(QtDependencyValidator::moduleName(QtModuleId::Widgets) == "Widgets");
    CHECK(QtDependencyValidator::moduleName(QtModuleId::WebEngineWidgets) == "WebEngineWidgets");
    CHECK(QtDependencyValidator::moduleName(QtModuleId::Test) == "Test");
    CHECK(QtDependencyValidator::moduleName(QtModuleId::Svg) == "Svg");
    CHECK(QtDependencyValidator::moduleName(static_cast<QtModuleId>(999)) == "Unknown");

    // Outcome formatting
    CHECK(QtDependencyValidator::formatOutcome(QtValidationOutcome::Satisfied) == "Satisfied");
    CHECK(QtDependencyValidator::formatOutcome(QtValidationOutcome::DegradedHeadless) ==
          "DegradedHeadless");
    CHECK(QtDependencyValidator::formatOutcome(QtValidationOutcome::IncompatibleVersion) ==
          "IncompatibleVersion");
    CHECK(QtDependencyValidator::formatOutcome(QtValidationOutcome::MissingComponent) ==
          "MissingComponent");
    CHECK(QtDependencyValidator::formatOutcome(QtValidationOutcome::EnvironmentError) ==
          "EnvironmentError");
    CHECK(QtDependencyValidator::formatOutcome(static_cast<QtValidationOutcome>(999)) == "Unknown");

    // Report formatting
    QtValidationReport sampleReport;
    sampleReport.outcome = QtValidationOutcome::MissingComponent;
    sampleReport.foundVersion = SemanticVersion{6, 10, 0};
    sampleReport.missingModules = {QtModuleId::Widgets, QtModuleId::WebEngineWidgets};
    sampleReport.diagnosticMessage = "Missing required modules";

    const auto formatted = QtDependencyValidator::formatReport(sampleReport);
    CHECK(formatted.find("MissingComponent") != std::string::npos);
    CHECK(formatted.find("6.10.0") != std::string::npos);
    CHECK(formatted.find("Widgets") != std::string::npos);
    CHECK(formatted.find("WebEngineWidgets") != std::string::npos);
    CHECK(formatted.find("Missing required modules") != std::string::npos);
}

} // namespace

int main() {
    try {
        test_version_parsing_positive();
        test_version_parsing_negative();
        test_version_comparison_and_compatibility();
        test_profile_evaluation_positive();
        test_profile_evaluation_negative();
        test_runtime_environment_validation();
        test_concurrency_and_thread_safety();
        test_diagnostic_formatting();

        std::cout << "Qt dependency validation tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
