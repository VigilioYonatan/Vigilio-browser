#include "core/WebEngineVersionValidator.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error("CHECK failed at line " + std::to_string(__LINE__) +               \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

struct TempHelperFileGuard {
    std::filesystem::path path;
    explicit TempHelperFileGuard(std::filesystem::path p) : path(std::move(p)) {
        cleanup();
    }
    ~TempHelperFileGuard() {
        cleanup();
    }
    void cleanup() noexcept {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempHelperFileGuard(const TempHelperFileGuard&) = delete;
    TempHelperFileGuard& operator=(const TempHelperFileGuard&) = delete;
};

void test_chromium_version_parsing_positive() {
    // 1. Standard 4-part version
    const auto v128 = ChromiumVersion::parse("128.0.6613.119");
    CHECK(v128.has_value());
    CHECK(v128->major == 128);
    CHECK(v128->minor == 0);
    CHECK(v128->build == 6613);
    CHECK(v128->patch == 119);
    CHECK(v128->milestone() == 128);
    CHECK(v128->toString() == "128.0.6613.119");

    // 2. Base version zeroes
    const auto vBase = ChromiumVersion::parse("128.0.0.0");
    CHECK(vBase.has_value());
    CHECK(vBase->major == 128 && vBase->minor == 0 && vBase->build == 0 && vBase->patch == 0);
    CHECK(vBase->milestone() == 128);
    CHECK(vBase->toString() == "128.0.0.0");

    const auto vBase132 = ChromiumVersion::parse("132.0.0.0");
    CHECK(vBase132.has_value());
    CHECK(vBase132->major == 132 && vBase132->minor == 0 && vBase132->build == 0 &&
          vBase132->patch == 0);
    CHECK(vBase132->milestone() == 132);
    CHECK(vBase132->toString() == "132.0.0.0");

    // 3. Lowercase 'v' prefix
    const auto vLower = ChromiumVersion::parse("v130.0.6723.58");
    CHECK(vLower.has_value());
    CHECK(vLower->major == 130 && vLower->minor == 0 && vLower->build == 6723 && vLower->patch == 58);
    CHECK(vLower->milestone() == 130);
    CHECK(vLower->toString() == "130.0.6723.58");

    // 4. Uppercase 'V' prefix
    const auto vUpper = ChromiumVersion::parse("V132.0.6834.15");
    CHECK(vUpper.has_value());
    CHECK(vUpper->major == 132 && vUpper->minor == 0 && vUpper->build == 6834 && vUpper->patch == 15);
    CHECK(vUpper->milestone() == 132);
    CHECK(vUpper->toString() == "132.0.6834.15");

    // 5. Surrounding whitespace trimming (spaces, tabs, carriage returns, newlines)
    const auto vPadded = ChromiumVersion::parse("   129.0.6668.70   \t");
    CHECK(vPadded.has_value());
    CHECK(vPadded->major == 129 && vPadded->minor == 0 && vPadded->build == 6668 &&
          vPadded->patch == 70);
    CHECK(vPadded->toString() == "129.0.6668.70");

    const auto vMultilinePadded = ChromiumVersion::parse(" \r\n\t 128.0.6613.119 \t\r\n ");
    CHECK(vMultilinePadded.has_value());
    CHECK(vMultilinePadded->major == 128 && vMultilinePadded->minor == 0 &&
          vMultilinePadded->build == 6613 && vMultilinePadded->patch == 119);
    CHECK(vMultilinePadded->toString() == "128.0.6613.119");

    // 6. Via static WebEngineVersionValidator method
    const auto vDelegated = WebEngineVersionValidator::parseChromiumVersion("128.0.6613.120");
    CHECK(vDelegated.has_value());
    CHECK(vDelegated->major == 128 && vDelegated->minor == 0 && vDelegated->build == 6613 &&
          vDelegated->patch == 120);
    CHECK(vDelegated->milestone() == 128);
    CHECK(vDelegated->toString() == "128.0.6613.120");
}

void test_chromium_version_parsing_negative() {
    // Empty, whitespace-only and control characters
    CHECK(!ChromiumVersion::parse("").has_value());
    CHECK(!ChromiumVersion::parse("   ").has_value());
    CHECK(!ChromiumVersion::parse("\t\n").has_value());
    CHECK(!ChromiumVersion::parse("\r\n\t  \n").has_value());

    // Lone prefix or prefix followed by punctuation
    CHECK(!ChromiumVersion::parse("v").has_value());
    CHECK(!ChromiumVersion::parse("V").has_value());
    CHECK(!ChromiumVersion::parse("v.").has_value());
    CHECK(!ChromiumVersion::parse("V.").has_value());
    CHECK(!ChromiumVersion::parse("v.128.0.0.1").has_value());

    // Incomplete parts (less than 4 components)
    CHECK(!ChromiumVersion::parse("128").has_value());
    CHECK(!ChromiumVersion::parse("128.0").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613").has_value());
    CHECK(!ChromiumVersion::parse("v128.0.6613").has_value());

    // Excess parts (more than 4 components)
    CHECK(!ChromiumVersion::parse("128.0.6613.119.1").has_value());
    CHECK(!ChromiumVersion::parse("1.2.3.4.5").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119.0.0").has_value());

    // Trailing invalid content, pre-release tags or unexpected suffixes
    CHECK(!ChromiumVersion::parse("128.0.6613.119extra").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119-beta").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119+build1").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119a").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119 rc1").has_value());

    // Non-digit characters and misplaced letters
    CHECK(!ChromiumVersion::parse("abc").has_value());
    CHECK(!ChromiumVersion::parse("a128.0.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.x.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.y.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.z").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119z").has_value());

    // Punctuation anomalies: leading, double, trailing or consecutive dots
    CHECK(!ChromiumVersion::parse(".128.0.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128..6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0..119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.119.").has_value());
    CHECK(!ChromiumVersion::parse(".").has_value());
    CHECK(!ChromiumVersion::parse("..").has_value());
    CHECK(!ChromiumVersion::parse("...").has_value());
    CHECK(!ChromiumVersion::parse("....").has_value());

    // Embedded whitespace between parts
    CHECK(!ChromiumVersion::parse("128 .0.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128. 0.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0. 6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613. 119").has_value());

    // Negative numbers
    CHECK(!ChromiumVersion::parse("-128.0.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.-1.6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.-6613.119").has_value());
    CHECK(!ChromiumVersion::parse("128.0.6613.-119").has_value());
    CHECK(!ChromiumVersion::parse("+128.0.6613.119").has_value());

    // Integer overflow protection (uint32_t overflow)
    CHECK(!ChromiumVersion::parse("99999999999999999999.0.0.0").has_value());
    CHECK(!ChromiumVersion::parse("128.99999999999999999999.0.0").has_value());
    CHECK(!ChromiumVersion::parse("128.0.99999999999999999999.0").has_value());
    CHECK(!ChromiumVersion::parse("128.0.0.99999999999999999999").has_value());
    CHECK(!ChromiumVersion::parse("4294967296.0.0.0").has_value());           // 2^32
    CHECK(!ChromiumVersion::parse("18446744073709551616.0.0.0").has_value()); // 2^64
    CHECK(!ChromiumVersion::parse("128.4294967296.0.0").has_value());
    CHECK(!ChromiumVersion::parse("128.0.4294967296.0").has_value());
    CHECK(!ChromiumVersion::parse("128.0.0.4294967296").has_value());
}

void test_chromium_version_comparison_and_compatibility() {
    const ChromiumVersion v127_full{127, 0, 6533, 134};
    const ChromiumVersion v128_base{128, 0, 0, 0};
    const ChromiumVersion v128_minor{128, 1, 0, 0};
    const ChromiumVersion v128_build1{128, 0, 6613, 0};
    const ChromiumVersion v128_build2{128, 0, 6614, 0};
    const ChromiumVersion v128_patch1{128, 0, 6613, 119};
    const ChromiumVersion v128_patch2{128, 0, 6613, 120};
    const ChromiumVersion v129_base{129, 0, 0, 0};

    // 1. Reflexivity: a == a, (a <=> a) == 0, !(a < a), !(a > a)
    CHECK(v128_base == v128_base);
    CHECK((v128_base <=> v128_base) == 0);
    CHECK(!(v128_base < v128_base));
    CHECK(!(v128_base > v128_base));
    CHECK(v128_base <= v128_base);
    CHECK(v128_base >= v128_base);

    // 2. Antisymmetry: a < b implies b > a, and !(b < a)
    CHECK(v128_base < v128_patch1);
    CHECK(v128_patch1 > v128_base);
    CHECK(!(v128_patch1 < v128_base));
    CHECK((v128_base <=> v128_patch1) < 0);
    CHECK((v128_patch1 <=> v128_base) > 0);

    // 3. Transitivity: a < b and b < c implies a < c
    CHECK(v127_full < v128_base);
    CHECK(v128_base < v128_patch1);
    CHECK(v127_full < v128_patch1);

    CHECK(v128_patch1 < v128_patch2);
    CHECK(v128_patch2 < v129_base);
    CHECK(v128_patch1 < v129_base);

    // 4. Strict field-by-field ordering
    // Major field
    CHECK(v127_full < v128_base);
    CHECK(v128_base < v129_base);

    // Minor field
    CHECK(v128_base < v128_minor);
    CHECK(v128_minor > v128_base);

    // Build field
    CHECK(v128_build1 < v128_build2);
    CHECK(v128_build2 > v128_build1);

    // Patch field
    CHECK(v128_patch1 < v128_patch2);
    CHECK(v128_patch2 > v128_patch1);
    CHECK(v128_patch1 != v128_patch2);
    CHECK(v128_patch1 == (ChromiumVersion{128, 0, 6613, 119}));

    // 5. Semantic compatibility predicate: isCompatibleWith
    CHECK(v128_base.isCompatibleWith(v128_base));
    CHECK(v128_patch1.isCompatibleWith(v128_base));
    CHECK(v128_patch2.isCompatibleWith(v128_base));
    CHECK(v129_base.isCompatibleWith(v128_base));
    CHECK(!v127_full.isCompatibleWith(v128_base));

    // Compatibility against custom baseline threshold
    const ChromiumVersion vThreshold{128, 0, 6613, 119};
    CHECK(v128_patch1.isCompatibleWith(vThreshold));
    CHECK(v128_patch2.isCompatibleWith(vThreshold));
    CHECK(v129_base.isCompatibleWith(vThreshold));
    CHECK(!v128_base.isCompatibleWith(vThreshold));

    // 6. Invariant check: baseline Chromium version must be 128.0.0.0
    CHECK(WebEngineVersionValidator::baselineChromiumVersion() == (ChromiumVersion{128, 0, 0, 0}));
}

void test_minimum_chromium_version_for_qt() {
    // Qt >= 6.10 maps to Chromium 128.0.0.0 (Decision Q-002)
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 10, 0}) ==
          (ChromiumVersion{128, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 10, 2}) ==
          (ChromiumVersion{128, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 11, 0}) ==
          (ChromiumVersion{128, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({7, 0, 0}) ==
          (ChromiumVersion{128, 0, 0, 0}));

    // Qt 6.9 maps to Chromium 122.0.0.0
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 9, 0}) ==
          (ChromiumVersion{122, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 9, 3}) ==
          (ChromiumVersion{122, 0, 0, 0}));

    // Qt 6.8 maps to Chromium 118.0.0.0
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 8, 0}) ==
          (ChromiumVersion{118, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 8, 2}) ==
          (ChromiumVersion{118, 0, 0, 0}));

    // Qt 6.7 and legacy map to Chromium 112.0.0.0
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 7, 0}) ==
          (ChromiumVersion{112, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({6, 7, 1}) ==
          (ChromiumVersion{112, 0, 0, 0}));
    CHECK(WebEngineVersionValidator::minimumChromiumVersionForQt({5, 15, 0}) ==
          (ChromiumVersion{112, 0, 0, 0}));
}

void test_feature_matrix_and_profiles() {
    auto hasFeature = [](const std::vector<WebEngineFeature>& list, WebEngineFeature feat) {
        for (const auto& f : list) {
            if (f == feat) {
                return true;
            }
        }
        return false;
    };

    // Features for milestone >= 128: all 8 canonical features supported
    for (uint32_t m : {128u, 130u, 132u, 150u}) {
        const auto features = WebEngineVersionValidator::supportedFeaturesForMilestone(m);
        CHECK(features.size() == 8);
        CHECK(hasFeature(features, WebEngineFeature::ManifestV3Extensions));
        CHECK(hasFeature(features, WebEngineFeature::PdfViewerInternal));
        CHECK(hasFeature(features, WebEngineFeature::WebRTC));
        CHECK(hasFeature(features, WebEngineFeature::DevToolsFrontend));
        CHECK(hasFeature(features, WebEngineFeature::ProcessSandboxing));
        CHECK(hasFeature(features, WebEngineFeature::HardwareAcceleration));
        CHECK(hasFeature(features, WebEngineFeature::LocalDevStorage));
        CHECK(hasFeature(features, WebEngineFeature::Http3Quic));
    }

    // Features for milestone 122..127: 6 features supported (lacks MV3 and Http3Quic)
    for (uint32_t m : {122u, 125u, 127u}) {
        const auto features = WebEngineVersionValidator::supportedFeaturesForMilestone(m);
        CHECK(features.size() == 6);
        CHECK(!hasFeature(features, WebEngineFeature::ManifestV3Extensions));
        CHECK(!hasFeature(features, WebEngineFeature::Http3Quic));
        CHECK(hasFeature(features, WebEngineFeature::PdfViewerInternal));
        CHECK(hasFeature(features, WebEngineFeature::WebRTC));
        CHECK(hasFeature(features, WebEngineFeature::DevToolsFrontend));
        CHECK(hasFeature(features, WebEngineFeature::ProcessSandboxing));
        CHECK(hasFeature(features, WebEngineFeature::HardwareAcceleration));
        CHECK(hasFeature(features, WebEngineFeature::LocalDevStorage));
    }

    // Features for milestone 118..121: 4 features supported (lacks sandboxing, pdf, mv3, quic)
    for (uint32_t m : {118u, 119u, 121u}) {
        const auto features = WebEngineVersionValidator::supportedFeaturesForMilestone(m);
        CHECK(features.size() == 4);
        CHECK(!hasFeature(features, WebEngineFeature::ManifestV3Extensions));
        CHECK(!hasFeature(features, WebEngineFeature::PdfViewerInternal));
        CHECK(!hasFeature(features, WebEngineFeature::ProcessSandboxing));
        CHECK(!hasFeature(features, WebEngineFeature::Http3Quic));
        CHECK(hasFeature(features, WebEngineFeature::WebRTC));
        CHECK(hasFeature(features, WebEngineFeature::DevToolsFrontend));
        CHECK(hasFeature(features, WebEngineFeature::HardwareAcceleration));
        CHECK(hasFeature(features, WebEngineFeature::LocalDevStorage));
    }

    // Features for milestone 0 / unknown: empty
    const auto features0 = WebEngineVersionValidator::supportedFeaturesForMilestone(0);
    CHECK(features0.empty());

    // Profile requirements verification
    // 1. FullDesktopBrowser: requires MV3, PDF viewer, Sandboxing, LocalDevStorage
    const auto reqDesktop = WebEngineVersionValidator::requiredFeaturesForProfile(
        WebEngineCompatibilityProfile::FullDesktopBrowser);
    CHECK(reqDesktop.size() == 4);
    CHECK(hasFeature(reqDesktop, WebEngineFeature::ManifestV3Extensions));
    CHECK(hasFeature(reqDesktop, WebEngineFeature::PdfViewerInternal));
    CHECK(hasFeature(reqDesktop, WebEngineFeature::ProcessSandboxing));
    CHECK(hasFeature(reqDesktop, WebEngineFeature::LocalDevStorage));

    // 2. RestrictedSecureProfile: requires Sandboxing, LocalDevStorage
    const auto reqSecure = WebEngineVersionValidator::requiredFeaturesForProfile(
        WebEngineCompatibilityProfile::RestrictedSecureProfile);
    CHECK(reqSecure.size() == 2);
    CHECK(!hasFeature(reqSecure, WebEngineFeature::ManifestV3Extensions));
    CHECK(hasFeature(reqSecure, WebEngineFeature::ProcessSandboxing));
    CHECK(hasFeature(reqSecure, WebEngineFeature::LocalDevStorage));

    // 3. HeadlessInspection: requires LocalDevStorage
    const auto reqHeadless = WebEngineVersionValidator::requiredFeaturesForProfile(
        WebEngineCompatibilityProfile::HeadlessInspection);
    CHECK(reqHeadless.size() == 1);
    CHECK(hasFeature(reqHeadless, WebEngineFeature::LocalDevStorage));

    // 4. MinimalCore: requires zero features
    const auto reqMinimal = WebEngineVersionValidator::requiredFeaturesForProfile(
        WebEngineCompatibilityProfile::MinimalCore);
    CHECK(reqMinimal.empty());
}

void test_helper_executable_check() {
    // Empty path
    CHECK(!WebEngineVersionValidator::isHelperExecutable(""));

    // Non-existent path
    CHECK(!WebEngineVersionValidator::isHelperExecutable("/non/existent/path/QtWebEngineProcess"));

    // Directory path
    CHECK(!WebEngineVersionValidator::isHelperExecutable("/tmp"));

    // Synthetic regular files with RAII cleanup
    const std::filesystem::path tempExec =
        std::filesystem::temp_directory_path() / "test_fake_qtwebengineprocess";
    TempHelperFileGuard guardExec(tempExec);

    {
        std::ofstream ofs(tempExec);
        ofs << "#!/bin/sh\nexit 0\n";
    }

#if !defined(_WIN32)
    // Non-executable file (0644)
    ::chmod(tempExec.c_str(), 0644);
    CHECK(!WebEngineVersionValidator::isHelperExecutable(tempExec.string()));

    // Executable file (0755)
    ::chmod(tempExec.c_str(), 0755);
    CHECK(WebEngineVersionValidator::isHelperExecutable(tempExec.string()));
#else
    CHECK(WebEngineVersionValidator::isHelperExecutable(tempExec.string()));
#endif

    // Symlink tests
    const std::filesystem::path validSymlink =
        std::filesystem::temp_directory_path() / "test_symlink_valid_helper";
    TempHelperFileGuard guardValidSymlink(validSymlink);

    std::error_code ec;
    std::filesystem::create_symlink(tempExec, validSymlink, ec);
    if (!ec) {
#if !defined(_WIN32)
        ::chmod(tempExec.c_str(), 0755);
        CHECK(WebEngineVersionValidator::isHelperExecutable(validSymlink.string()));

        ::chmod(tempExec.c_str(), 0644);
        CHECK(!WebEngineVersionValidator::isHelperExecutable(validSymlink.string()));
        ::chmod(tempExec.c_str(), 0755);
#else
        CHECK(WebEngineVersionValidator::isHelperExecutable(validSymlink.string()));
#endif
    }

    // Broken symlink (target does not exist)
    const std::filesystem::path brokenSymlink =
        std::filesystem::temp_directory_path() / "test_symlink_broken_helper";
    TempHelperFileGuard guardBrokenSymlink(brokenSymlink);

    std::filesystem::create_symlink("/tmp/non_existent_target_process_binary", brokenSymlink, ec);
    if (!ec) {
        CHECK(!WebEngineVersionValidator::isHelperExecutable(brokenSymlink.string()));
    }

    // Symlink pointing to a directory
    const std::filesystem::path dirSymlink =
        std::filesystem::temp_directory_path() / "test_symlink_dir_helper";
    TempHelperFileGuard guardDirSymlink(dirSymlink);

    std::filesystem::create_directory_symlink(std::filesystem::temp_directory_path(), dirSymlink, ec);
    if (!ec) {
        CHECK(!WebEngineVersionValidator::isHelperExecutable(dirSymlink.string()));
    }
}

void test_compatibility_validation() {
    const SemanticVersion qt610{6, 10, 0};
    const SemanticVersion qt69{6, 9, 3};
    const ChromiumVersion cr128{128, 0, 6613, 119};
    const ChromiumVersion cr122{122, 0, 6261, 94};

    const std::filesystem::path tempExec = std::filesystem::temp_directory_path() / "test_helper_valid";
    TempHelperFileGuard guard(tempExec);
    {
        std::ofstream ofs(tempExec);
        ofs << "#!/bin/sh\nexit 0\n";
    }
#if !defined(_WIN32)
    ::chmod(tempExec.c_str(), 0755);
#endif

    // 1. MinimalCore profile: degrades cleanly to DegradedHeadless regardless of environment
    const auto repMin = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::MinimalCore, std::nullopt, std::nullopt, "");
    CHECK(repMin.status == WebEngineCompatibilityStatus::DegradedHeadless);

    // 2. Incompatible Qt version (< 6.10.0 or nullopt)
    const auto repBadQt = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt69, cr128, tempExec.string(), true);
    CHECK(repBadQt.status == WebEngineCompatibilityStatus::IncompatibleQtVersion);

    const auto repNoQt = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, std::nullopt, cr128, tempExec.string(), true);
    CHECK(repNoQt.status == WebEngineCompatibilityStatus::IncompatibleQtVersion);

    // 3. Incompatible Chromium version (< 128 or nullopt)
    const auto repBadCr = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt610, cr122, tempExec.string(), true);
    CHECK(repBadCr.status == WebEngineCompatibilityStatus::IncompatibleChromiumVersion);

    const auto repNoCr = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt610, std::nullopt, tempExec.string(), true);
    CHECK(repNoCr.status == WebEngineCompatibilityStatus::IncompatibleChromiumVersion);

    // 4. Missing or non-executable helper process for FullDesktopBrowser and RestrictedSecureProfile
    const auto repNoHelperDesktop = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt610, cr128, "/invalid/helper", true);
    CHECK(repNoHelperDesktop.status == WebEngineCompatibilityStatus::ProcessHelperMissing);

    const auto repNoHelperSecure = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::RestrictedSecureProfile, qt610, cr128, "/invalid/helper", true);
    CHECK(repNoHelperSecure.status == WebEngineCompatibilityStatus::ProcessHelperMissing);

    // 5. Sandboxing unsupported in host environment
    const auto repBadSandboxDesktop = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt610, cr128, tempExec.string(), false);
    CHECK(repBadSandboxDesktop.status == WebEngineCompatibilityStatus::EnvironmentMisconfigured);

    const auto repBadSandboxSecure = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::RestrictedSecureProfile, qt610, cr128, tempExec.string(), false);
    CHECK(repBadSandboxSecure.status == WebEngineCompatibilityStatus::EnvironmentMisconfigured);

    // 6. Compatible profile FullDesktopBrowser
    const auto repGood = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::FullDesktopBrowser, qt610, cr128, tempExec.string(), true);
    CHECK(repGood.status == WebEngineCompatibilityStatus::Compatible);
    CHECK(repGood.missingFeatures.empty());
    CHECK(!repGood.supportedFeatures.empty());

    // 7. Compatible profile RestrictedSecureProfile
    const auto repGoodSecure = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::RestrictedSecureProfile, qt610, cr128, tempExec.string(), true);
    CHECK(repGoodSecure.status == WebEngineCompatibilityStatus::Compatible);

    // 8. HeadlessInspection profile: does not require helper executable
    const auto repHeadless = WebEngineVersionValidator::validateCompatibility(
        WebEngineCompatibilityProfile::HeadlessInspection, qt610, cr128, "", true);
    CHECK(repHeadless.status == WebEngineCompatibilityStatus::Compatible);
}

void test_startup_validation() {
    const SemanticVersion qt610{6, 10, 0};
    const SemanticVersion qt69{6, 9, 3};

    // 1. Headless mode produces DegradedHeadless
    const auto repHeadless =
        WebEngineVersionValidator::validateStartup(qt610, "128.0.6613.119", "", true);
    CHECK(repHeadless.status == WebEngineCompatibilityStatus::DegradedHeadless);

    // 2. Desktop mode with explicit helper
    const std::filesystem::path tempExec =
        std::filesystem::temp_directory_path() / "test_startup_helper";
    TempHelperFileGuard guard(tempExec);
    {
        std::ofstream ofs(tempExec);
        ofs << "#!/bin/sh\nexit 0\n";
    }
#if !defined(_WIN32)
    ::chmod(tempExec.c_str(), 0755);
#endif

    const auto repDesktop =
        WebEngineVersionValidator::validateStartup(qt610, "128.0.6613.119", tempExec.string(), false);
    CHECK(repDesktop.status == WebEngineCompatibilityStatus::Compatible);

    // 3. Desktop mode with missing helper
    const auto repMissingHelper = WebEngineVersionValidator::validateStartup(
        qt610, "128.0.6613.119", "/invalid/nonexistent/helper", false);
    CHECK(repMissingHelper.status == WebEngineCompatibilityStatus::ProcessHelperMissing);

    // 4. Desktop mode with incompatible Qt
    const auto repBadQt =
        WebEngineVersionValidator::validateStartup(qt69, "128.0.6613.119", tempExec.string(), false);
    CHECK(repBadQt.status == WebEngineCompatibilityStatus::IncompatibleQtVersion);

    // 5. Desktop mode with incompatible Chromium (< 128)
    const auto repBadCr =
        WebEngineVersionValidator::validateStartup(qt610, "122.0.6261.94", tempExec.string(), false);
    CHECK(repBadCr.status == WebEngineCompatibilityStatus::IncompatibleChromiumVersion);

    // 6. Desktop mode with empty chromium string -> auto-resolves from Qt 6.10 to 128.0.0.0
    const auto repAutoCr =
        WebEngineVersionValidator::validateStartup(qt610, "", tempExec.string(), false);
    CHECK(repAutoCr.status == WebEngineCompatibilityStatus::Compatible);
    CHECK(repAutoCr.detectedChromiumVersion == (ChromiumVersion{128, 0, 0, 0}));
}

void test_formatting_and_introspection() {
    // Feature names
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::ManifestV3Extensions) ==
          "ManifestV3Extensions");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::PdfViewerInternal) ==
          "PdfViewerInternal");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::WebRTC) == "WebRTC");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::DevToolsFrontend) ==
          "DevToolsFrontend");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::ProcessSandboxing) ==
          "ProcessSandboxing");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::HardwareAcceleration) ==
          "HardwareAcceleration");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::LocalDevStorage) ==
          "LocalDevStorage");
    CHECK(WebEngineVersionValidator::featureName(WebEngineFeature::Http3Quic) == "Http3Quic");

    // Profile names
    CHECK(WebEngineVersionValidator::profileName(WebEngineCompatibilityProfile::FullDesktopBrowser) ==
          "FullDesktopBrowser");
    CHECK(WebEngineVersionValidator::profileName(WebEngineCompatibilityProfile::HeadlessInspection) ==
          "HeadlessInspection");
    CHECK(WebEngineVersionValidator::profileName(
              WebEngineCompatibilityProfile::RestrictedSecureProfile) == "RestrictedSecureProfile");
    CHECK(WebEngineVersionValidator::profileName(WebEngineCompatibilityProfile::MinimalCore) ==
          "MinimalCore");

    // Status formatting
    CHECK(WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus::Compatible) ==
          "Compatible");
    CHECK(WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus::DegradedHeadless) ==
          "DegradedHeadless");
    CHECK(WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus::IncompatibleQtVersion) ==
          "IncompatibleQtVersion");
    CHECK(WebEngineVersionValidator::formatStatus(
              WebEngineCompatibilityStatus::IncompatibleChromiumVersion) ==
          "IncompatibleChromiumVersion");
    CHECK(WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus::ProcessHelperMissing) ==
          "ProcessHelperMissing");
    CHECK(WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus::FeatureUnsupported) ==
          "FeatureUnsupported");
    CHECK(WebEngineVersionValidator::formatStatus(
              WebEngineCompatibilityStatus::EnvironmentMisconfigured) == "EnvironmentMisconfigured");

    // Report formatting
    WebEngineCompatibilityReport rep;
    rep.status = WebEngineCompatibilityStatus::FeatureUnsupported;
    rep.detectedQtVersion = SemanticVersion{6, 10, 0};
    rep.detectedChromiumVersion = ChromiumVersion{122, 0, 6261, 94};
    rep.helperPath = "/usr/lib/qt6/libexec/QtWebEngineProcess";
    rep.missingFeatures = {WebEngineFeature::ManifestV3Extensions};
    rep.diagnosticMessage = "ManifestV3Extensions required";
    rep.remediationAction = "Upgrade Chromium milestone";

    const auto formatted = WebEngineVersionValidator::formatReport(rep);
    CHECK(formatted.find("FeatureUnsupported") != std::string::npos);
    CHECK(formatted.find("6.10.0") != std::string::npos);
    CHECK(formatted.find("122.0.6261.94") != std::string::npos);
    CHECK(formatted.find("QtWebEngineProcess") != std::string::npos);
    CHECK(formatted.find("ManifestV3Extensions") != std::string::npos);
    CHECK(formatted.find("ManifestV3Extensions required") != std::string::npos);
    CHECK(formatted.find("Upgrade Chromium milestone") != std::string::npos);
}

void test_concurrency_and_thread_safety() {
    constexpr int numThreads = 16;
    constexpr int iterationsPerThread = 1000;
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    std::atomic<bool> failure{false};

    const auto startTime = std::chrono::steady_clock::now();

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&failure]() {
            try {
                for (int i = 0; i < iterationsPerThread; ++i) {
                    // 1. Parsing & components
                    const auto v = ChromiumVersion::parse("128.0.6613.119");
                    if (!v.has_value() || v->milestone() != 128 || v->major != 128 || v->minor != 0 ||
                        v->build != 6613 || v->patch != 119) {
                        failure = true;
                    }

                    // 2. Minimum version check
                    const auto minCr =
                        WebEngineVersionValidator::minimumChromiumVersionForQt({6, 10, 0});
                    if (minCr != ChromiumVersion{128, 0, 0, 0}) {
                        failure = true;
                    }

                    // 3. Supported features
                    const auto feats = WebEngineVersionValidator::supportedFeaturesForMilestone(128);
                    if (feats.size() != 8) {
                        failure = true;
                    }

                    // 4. Required features for profile
                    const auto reqs = WebEngineVersionValidator::requiredFeaturesForProfile(
                        WebEngineCompatibilityProfile::FullDesktopBrowser);
                    if (reqs.size() != 4) {
                        failure = true;
                    }

                    // 5. Compatibility validation
                    const auto rep = WebEngineVersionValidator::validateCompatibility(
                        WebEngineCompatibilityProfile::MinimalCore, SemanticVersion{6, 10, 0},
                        ChromiumVersion{128, 0, 6613, 119}, "", true);
                    if (rep.status != WebEngineCompatibilityStatus::DegradedHeadless) {
                        failure = true;
                    }

                    // 6. Startup validation
                    const auto repStartup = WebEngineVersionValidator::validateStartup(
                        SemanticVersion{6, 10, 0}, "128.0.6613.119", "", true);
                    if (repStartup.status != WebEngineCompatibilityStatus::DegradedHeadless) {
                        failure = true;
                    }

                    // 7. Report formatting
                    const auto formatted = WebEngineVersionValidator::formatReport(repStartup);
                    if (formatted.empty()) {
                        failure = true;
                    }
                }
            } catch (...) {
                failure = true;
            }
        });
    }

    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Average latency per validation call must be well under 1 millisecond
    const double totalOps = static_cast<double>(numThreads * iterationsPerThread);
    const double msPerOp = static_cast<double>(elapsedMs) / totalOps;
    CHECK(msPerOp < 1.0);

    CHECK(!failure.load());
}

} // namespace

int main() {
    try {
        test_chromium_version_parsing_positive();
        test_chromium_version_parsing_negative();
        test_chromium_version_comparison_and_compatibility();
        test_minimum_chromium_version_for_qt();
        test_feature_matrix_and_profiles();
        test_helper_executable_check();
        test_compatibility_validation();
        test_startup_validation();
        test_formatting_and_introspection();
        test_concurrency_and_thread_safety();

        std::cout << "WebEngine version compatibility tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
