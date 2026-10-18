#include "core/QtDependencyValidator.h"
#include "core/SessionFile.h"
#include "core/TabModel.h"
#include "core/UrlInput.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition);                                      \
    } while (false)

namespace {
struct TempFileGuard {
    std::filesystem::path path;
    explicit TempFileGuard(std::filesystem::path p) : path(std::move(p)) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    ~TempFileGuard() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
};
} // namespace

int main() {
    try {
        // --- 1. Positive URL and search resolution cases ---
        auto query = resolveInput("  buenas tardes  ");
        CHECK(query.kind == NavigationTarget::Kind::Search);
        CHECK(query.value == "https://www.google.com/search?q=buenas+tardes");

        CHECK(resolveInput("https://example.com").kind == NavigationTarget::Kind::Url);
        CHECK(resolveInput("http://example.com/test?k=v#anchor").kind == NavigationTarget::Kind::Url);
        CHECK(resolveInput("example.com").value == "https://example.com");
        CHECK(resolveInput("example.com/path").value == "https://example.com/path");
        CHECK(resolveInput("localhost:8080").value == "http://localhost:8080");
        CHECK(resolveInput("localhost:3000/api").value == "http://localhost:3000/api");
        CHECK(resolveInput("localhost").value == "https://localhost");

        CHECK(resolveInput("ñandú").kind == NavigationTarget::Kind::Search);
        CHECK(resolveInput("two + two").value.find("two+%2B+two") != std::string::npos);

        // --- 2. Negative and security rejection cases ---
        CHECK(resolveInput("").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput(" ").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput(" \t\r\n ").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("https://").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("http://").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("ftp://").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("javascript:alert(1)").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("  JAVASCRIPT:void(0)  ").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("data:text/html,<b>xss</b>").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("file:///etc/passwd").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("file:///C:/Windows/win.ini").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("https://user:secret@example.com").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("user:pass@example.com").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("https://example.com\\malicious").kind == NavigationTarget::Kind::Rejected);
        CHECK(resolveInput("example.com\\malicious").kind == NavigationTarget::Kind::Rejected);

        // Extreme length rejection (> 8192 characters)
        const std::string oversized(8193, 'x');
        CHECK(resolveInput(oversized).kind == NavigationTarget::Kind::Rejected);

        // --- 3. TabModel: Positive and negative operations ---
        TabModel tabs;
        CHECK(tabs.size() == 0);
        CHECK(tabs.tabs().empty());

        const auto a = tabs.open("https://one.test");
        const auto b = tabs.open("https://two.test");
        const auto c = tabs.open("https://three.test");
        CHECK(tabs.size() == 3);
        CHECK(a != b && b != c && a != c);

        CHECK(tabs.setPinned(a, true));
        CHECK(tabs.setMuted(b, true));

        // Duplicate existing tab
        const auto dupB = tabs.duplicate(b);
        CHECK(dupB.has_value());
        CHECK(tabs.size() == 4);

        // Negative operations on non-existent tab ID
        CHECK(!tabs.close(9999));
        CHECK(!tabs.setPinned(9999, true));
        CHECK(!tabs.setMuted(9999, true));
        CHECK(!tabs.duplicate(9999).has_value());
        CHECK(!tabs.closeOthers(9999));

        // Close others: pinned 'a' must survive; 'b' is the target; unpinned 'c' and 'dupB' close
        CHECK(tabs.closeOthers(b));
        CHECK(tabs.size() == 2);

        // Close remaining
        CHECK(tabs.close(a));
        CHECK(tabs.close(b));
        CHECK(tabs.size() == 0);

        // --- 4. SessionFile: Positive, negative, corruption and recovery cases ---
        const auto tempSession =
            std::filesystem::temp_directory_path() / "vigilio-test-core-session.txt";
        TempFileGuard sessionGuard(tempSession);

        // Save and restore valid session
        CHECK(saveSession(tempSession, {"https://example.com", "https://hello.test"}));
        auto restored = loadSession(tempSession);
        CHECK(restored.size() == 2);
        CHECK(restored[0] == "https://example.com");
        CHECK(restored[1] == "https://hello.test");

        // Overwrite existing session atomically
        CHECK(saveSession(tempSession, {"https://new.test"}));
        CHECK(loadSession(tempSession).size() == 1);

        // Rejection: saving invalid or dangerous URLs must fail
        CHECK(!saveSession(tempSession, {"javascript:alert(1)"}));
        CHECK(!saveSession(tempSession, {""}));
        CHECK(!saveSession(tempSession, {"file:///etc/passwd"}));

        // Rejection: saving exceeding maxTabs (1000 tabs)
        std::vector<std::string> tooManyTabs(1001, "https://example.com");
        CHECK(!saveSession(tempSession, tooManyTabs));

        // Recovery: loading non-existent session returns empty vector
        const auto nonExistent =
            std::filesystem::temp_directory_path() / "vigilio-non-existent-session-987.txt";
        CHECK(loadSession(nonExistent).empty());

        // Recovery: corrupted header handling
        {
            std::ofstream corruptFile(tempSession, std::ios::binary | std::ios::trunc);
            corruptFile << "NOT_VIGILIO_HEADER\n\"https://example.com\"\n";
        }
        CHECK(loadSession(tempSession).empty());

        // Recovery: corrupted/unquoted contents handling
        {
            std::ofstream corruptFile(tempSession, std::ios::binary | std::ios::trunc);
            corruptFile << "VIGILIO_SESSION_V1\nmalformed unquoted line\n";
        }
        CHECK(loadSession(tempSession).empty());

        // Recovery: session file containing rejected scheme
        {
            std::ofstream corruptFile(tempSession, std::ios::binary | std::ios::trunc);
            corruptFile << "VIGILIO_SESSION_V1\n\"javascript:alert(1)\"\n";
        }
        CHECK(loadSession(tempSession).empty());

        // --- 5. QtDependencyValidator: Version parsing, comparison, profiles and runtime ---
        // CRIT-001: SemanticVersion::parse positive and negative
        const auto v6100 = SemanticVersion::parse("6.10.0");
        CHECK(v6100.has_value());
        CHECK(v6100->major == 6 && v6100->minor == 10 && v6100->patch == 0);
        CHECK(v6100->toString() == "6.10.0");

        const auto vBeta = SemanticVersion::parse("6.10.1-beta");
        CHECK(vBeta.has_value());
        CHECK(vBeta->major == 6 && vBeta->minor == 10 && vBeta->patch == 1);

        const auto vPrefixed = SemanticVersion::parse("v6.12.3");
        CHECK(vPrefixed.has_value());
        CHECK(vPrefixed->major == 6 && vPrefixed->minor == 12 && vPrefixed->patch == 3);

        const auto vTwoPart = SemanticVersion::parse("6.10");
        CHECK(vTwoPart.has_value());
        CHECK(vTwoPart->major == 6 && vTwoPart->minor == 10 && vTwoPart->patch == 0);

        CHECK(!SemanticVersion::parse("").has_value());
        CHECK(!SemanticVersion::parse("   ").has_value());
        CHECK(!SemanticVersion::parse("abc").has_value());
        CHECK(!SemanticVersion::parse("6.").has_value());
        CHECK(!SemanticVersion::parse("6.abc").has_value());
        CHECK(!SemanticVersion::parse("invalid.version").has_value());

        // CRIT-002: SemanticVersion comparison and compatibility
        const SemanticVersion v693{6, 9, 3};
        const SemanticVersion v5152{5, 15, 2};
        const SemanticVersion v6102{6, 10, 2};
        const SemanticVersion v700{7, 0, 0};

        CHECK(*v6100 == *v6100);
        CHECK(v6102 > *v6100);
        CHECK(v693 < *v6100);
        CHECK(v700 > *v6100);

        CHECK(v6100->isCompatibleWith(*v6100));
        CHECK(v6102.isCompatibleWith(*v6100));
        CHECK(!v693.isCompatibleWith(*v6100));
        CHECK(!v5152.isCompatibleWith(*v6100));
        CHECK(!v700.isCompatibleWith(*v6100));

        // CRIT-003: Profile evaluation
        // HeadlessCore requires no modules and passes
        const auto headlessReport =
            QtDependencyValidator::validateProfile(QtRequirementProfile::HeadlessCore, {}, {0, 0, 0});
        CHECK(headlessReport.outcome == QtValidationOutcome::Satisfied);

        // MinimalGui requires Core + Widgets
        const auto minGuiReport = QtDependencyValidator::validateProfile(
            QtRequirementProfile::MinimalGui, {QtModuleId::Core, QtModuleId::Widgets}, *v6100);
        CHECK(minGuiReport.outcome == QtValidationOutcome::Satisfied);

        const auto minGuiMissing = QtDependencyValidator::validateProfile(
            QtRequirementProfile::MinimalGui, {QtModuleId::Core}, *v6100);
        CHECK(minGuiMissing.outcome == QtValidationOutcome::MissingComponent);
        CHECK(minGuiMissing.missingModules.size() == 1);
        CHECK(minGuiMissing.missingModules[0] == QtModuleId::Widgets);

        // FullBrowser requires Core + Widgets + WebEngineWidgets
        const auto fullReport = QtDependencyValidator::validateProfile(
            QtRequirementProfile::FullBrowser,
            {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v6102);
        CHECK(fullReport.outcome == QtValidationOutcome::Satisfied);

        const auto fullMissing = QtDependencyValidator::validateProfile(
            QtRequirementProfile::FullBrowser, {QtModuleId::Core, QtModuleId::Widgets}, *v6100);
        CHECK(fullMissing.outcome == QtValidationOutcome::MissingComponent);
        CHECK(fullMissing.missingModules.size() == 1);
        CHECK(fullMissing.missingModules[0] == QtModuleId::WebEngineWidgets);

        const auto fullIncompatible = QtDependencyValidator::validateProfile(
            QtRequirementProfile::FullBrowser,
            {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, v693);
        CHECK(fullIncompatible.outcome == QtValidationOutcome::IncompatibleVersion);

        // DeveloperTest requires Core + Widgets + WebEngineWidgets + Test
        const auto devTestReport = QtDependencyValidator::validateProfile(
            QtRequirementProfile::DeveloperTest,
            {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets, QtModuleId::Test},
            *v6100);
        CHECK(devTestReport.outcome == QtValidationOutcome::Satisfied);

        const auto devTestMissing = QtDependencyValidator::validateProfile(
            QtRequirementProfile::DeveloperTest,
            {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets}, *v6100);
        CHECK(devTestMissing.outcome == QtValidationOutcome::MissingComponent);
        CHECK(devTestMissing.missingModules.size() == 1);
        CHECK(devTestMissing.missingModules[0] == QtModuleId::Test);

        // CRIT-004: Runtime environment validation
        // Normal cases with display
        const auto runX11 =
            QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "6.10.0", ":0", nullptr);
        CHECK(runX11.outcome == QtValidationOutcome::Satisfied);

        const auto runWayland =
            QtDependencyValidator::validateRuntimeEnvironment("6.10.2", "6.10.0", nullptr, "wayland-0");
        CHECK(runWayland.outcome == QtValidationOutcome::Satisfied);

        // ABI major mismatch
        const auto runAbiMismatch =
            QtDependencyValidator::validateRuntimeEnvironment("7.0.0", "6.10.0", ":0", nullptr);
        CHECK(runAbiMismatch.outcome == QtValidationOutcome::EnvironmentError);

        // Incompatible version (< 6.10.0)
        const auto runIncompatible =
            QtDependencyValidator::validateRuntimeEnvironment("6.9.1", "6.9.1", ":0", nullptr);
        CHECK(runIncompatible.outcome == QtValidationOutcome::IncompatibleVersion);

        // Missing display in Linux
        const auto runNoDisplay =
            QtDependencyValidator::validateRuntimeEnvironment("6.10.0", "6.10.0", nullptr, nullptr);
        CHECK(runNoDisplay.outcome == QtValidationOutcome::EnvironmentError);

        // Malformed version strings
        const auto runMalformed =
            QtDependencyValidator::validateRuntimeEnvironment("not-a-version", "6.10.0", ":0", nullptr);
        CHECK(runMalformed.outcome == QtValidationOutcome::EnvironmentError);

        // Diagnostic formatting helper checks
        CHECK(!QtDependencyValidator::formatOutcome(QtValidationOutcome::Satisfied).empty());
        CHECK(!QtDependencyValidator::formatOutcome(QtValidationOutcome::MissingComponent).empty());
        CHECK(!QtDependencyValidator::formatReport(fullMissing).empty());
        CHECK(QtDependencyValidator::moduleName(QtModuleId::Core) == "Core");

        std::cout << "Core unit tests: PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
