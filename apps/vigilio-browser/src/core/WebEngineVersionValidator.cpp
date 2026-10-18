#include "core/WebEngineVersionValidator.h"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <filesystem>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace vigilio {

std::string ChromiumVersion::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build) + "." +
           std::to_string(patch);
}

std::optional<ChromiumVersion> ChromiumVersion::parse(std::string_view versionStr) {
    while (!versionStr.empty() && std::isspace(static_cast<unsigned char>(versionStr.front()))) {
        versionStr.remove_prefix(1);
    }
    while (!versionStr.empty() && std::isspace(static_cast<unsigned char>(versionStr.back()))) {
        versionStr.remove_suffix(1);
    }
    if (versionStr.empty()) {
        return std::nullopt;
    }

    if (versionStr.front() == 'v' || versionStr.front() == 'V') {
        versionStr.remove_prefix(1);
    }

    if (versionStr.empty() || !std::isdigit(static_cast<unsigned char>(versionStr.front()))) {
        return std::nullopt;
    }

    uint32_t maj = 0;
    auto res = std::from_chars(versionStr.data(), versionStr.data() + versionStr.size(), maj);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }
    if (res.ptr == versionStr.data() + versionStr.size() || *res.ptr != '.') {
        return std::nullopt;
    }

    const char* p = res.ptr + 1;
    const char* end = versionStr.data() + versionStr.size();
    if (p >= end || !std::isdigit(static_cast<unsigned char>(*p))) {
        return std::nullopt;
    }

    uint32_t min = 0;
    res = std::from_chars(p, end, min);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }
    if (res.ptr >= end || *res.ptr != '.') {
        return std::nullopt;
    }

    p = res.ptr + 1;
    if (p >= end || !std::isdigit(static_cast<unsigned char>(*p))) {
        return std::nullopt;
    }

    uint32_t bld = 0;
    res = std::from_chars(p, end, bld);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }
    if (res.ptr >= end || *res.ptr != '.') {
        return std::nullopt;
    }

    p = res.ptr + 1;
    if (p >= end || !std::isdigit(static_cast<unsigned char>(*p))) {
        return std::nullopt;
    }

    uint32_t ptch = 0;
    res = std::from_chars(p, end, ptch);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    while (res.ptr < end && std::isspace(static_cast<unsigned char>(*res.ptr))) {
        res.ptr++;
    }
    if (res.ptr != end) {
        return std::nullopt;
    }

    return ChromiumVersion{maj, min, bld, ptch};
}

std::optional<ChromiumVersion>
WebEngineVersionValidator::parseChromiumVersion(std::string_view versionStr) {
    return ChromiumVersion::parse(versionStr);
}

ChromiumVersion WebEngineVersionValidator::minimumChromiumVersionForQt(const SemanticVersion& qtVer) {
    if (qtVer.major > 6 || (qtVer.major == 6 && qtVer.minor >= 10)) {
        return {128, 0, 0, 0};
    }
    if (qtVer.major == 6 && qtVer.minor == 9) {
        return {122, 0, 0, 0};
    }
    if (qtVer.major == 6 && qtVer.minor == 8) {
        return {118, 0, 0, 0};
    }
    return {112, 0, 0, 0};
}

std::vector<WebEngineFeature>
WebEngineVersionValidator::supportedFeaturesForMilestone(uint32_t milestone) {
    if (milestone >= 128) {
        return {
            WebEngineFeature::ManifestV3Extensions,
            WebEngineFeature::PdfViewerInternal,
            WebEngineFeature::WebRTC,
            WebEngineFeature::DevToolsFrontend,
            WebEngineFeature::ProcessSandboxing,
            WebEngineFeature::HardwareAcceleration,
            WebEngineFeature::LocalDevStorage,
            WebEngineFeature::Http3Quic,
        };
    }
    if (milestone >= 122) {
        return {
            WebEngineFeature::PdfViewerInternal,    WebEngineFeature::WebRTC,
            WebEngineFeature::DevToolsFrontend,     WebEngineFeature::ProcessSandboxing,
            WebEngineFeature::HardwareAcceleration, WebEngineFeature::LocalDevStorage,
        };
    }
    if (milestone > 0) {
        return {
            WebEngineFeature::WebRTC,
            WebEngineFeature::DevToolsFrontend,
            WebEngineFeature::HardwareAcceleration,
            WebEngineFeature::LocalDevStorage,
        };
    }
    return {};
}

std::vector<WebEngineFeature>
WebEngineVersionValidator::requiredFeaturesForProfile(WebEngineCompatibilityProfile profile) {
    switch (profile) {
    case WebEngineCompatibilityProfile::FullDesktopBrowser:
        return {
            WebEngineFeature::ManifestV3Extensions,
            WebEngineFeature::PdfViewerInternal,
            WebEngineFeature::ProcessSandboxing,
            WebEngineFeature::LocalDevStorage,
        };
    case WebEngineCompatibilityProfile::RestrictedSecureProfile:
        return {
            WebEngineFeature::ProcessSandboxing,
            WebEngineFeature::LocalDevStorage,
        };
    case WebEngineCompatibilityProfile::HeadlessInspection:
        return {
            WebEngineFeature::LocalDevStorage,
        };
    case WebEngineCompatibilityProfile::MinimalCore:
        return {};
    }
    return {};
}

bool WebEngineVersionValidator::isHelperExecutable(const std::string& helperPath) {
    if (helperPath.empty()) {
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::exists(helperPath, ec) || !std::filesystem::is_regular_file(helperPath, ec)) {
        return false;
    }
#if defined(_WIN32)
    return true;
#else
    return access(helperPath.c_str(), X_OK) == 0;
#endif
}

WebEngineCompatibilityReport WebEngineVersionValidator::validateCompatibility(
    WebEngineCompatibilityProfile profile, const std::optional<SemanticVersion>& detectedQtVer,
    const std::optional<ChromiumVersion>& detectedChromiumVer, const std::string& helperPath,
    bool sandboxingSupported) {
    WebEngineCompatibilityReport report;
    report.detectedQtVersion = detectedQtVer;
    report.detectedChromiumVersion = detectedChromiumVer;
    report.helperPath = helperPath;

    // 1. MinimalCore profile degrades safely to DegradedHeadless without WebEngine requirement
    if (profile == WebEngineCompatibilityProfile::MinimalCore) {
        report.status = WebEngineCompatibilityStatus::DegradedHeadless;
        report.diagnosticMessage =
            "Minimal core profile active; WebEngine features are not required or initialized.";
        report.remediationAction = "None required. Headless and core services run without WebEngine.";
        return report;
    }

    // 2. Qt version validation (Qt >= 6.10.0 required)
    const SemanticVersion requiredQtMin{6, 10, 0};
    if (!detectedQtVer.has_value() || !detectedQtVer->isCompatibleWith(requiredQtMin)) {
        report.status = WebEngineCompatibilityStatus::IncompatibleQtVersion;
        report.diagnosticMessage = "Qt version " +
                                   (detectedQtVer.has_value() ? detectedQtVer->toString() : "none") +
                                   " is incompatible with required Qt 6.10+ baseline for WebEngine.";
        report.remediationAction = "Upgrade Qt installation to version 6.10.0 or higher.";
        return report;
    }

    // 3. Chromium version validation (Chromium milestone >= 128 required)
    if (!detectedChromiumVer.has_value() || detectedChromiumVer->milestone() < 128) {
        report.status = WebEngineCompatibilityStatus::IncompatibleChromiumVersion;
        report.diagnosticMessage =
            "Chromium version " +
            (detectedChromiumVer.has_value() ? detectedChromiumVer->toString() : "none") +
            " is below required baseline (milestone >= 128).";
        report.remediationAction =
            "WebEngine requires an embedded Chromium milestone >= 128 (Qt 6.10+).";
        return report;
    }

    // Features resolution
    const auto supported = supportedFeaturesForMilestone(detectedChromiumVer->milestone());
    report.supportedFeatures = supported;

    const auto required = requiredFeaturesForProfile(profile);
    for (const auto req : required) {
        bool found = false;
        for (const auto sup : supported) {
            if (req == sup) {
                found = true;
                break;
            }
        }
        if (!found) {
            report.missingFeatures.push_back(req);
        }
    }

    if (!report.missingFeatures.empty()) {
        report.status = WebEngineCompatibilityStatus::FeatureUnsupported;
        report.diagnosticMessage = "Required WebEngine feature(s) unsupported for milestone " +
                                   std::to_string(detectedChromiumVer->milestone()) + ":";
        for (const auto f : report.missingFeatures) {
            report.diagnosticMessage += " ";
            report.diagnosticMessage += featureName(f);
        }
        report.remediationAction =
            "Use a newer WebEngine/Chromium build supporting all required features.";
        return report;
    }

    // 4. Helper process check for GUI profiles
    const bool requiresHelper = (profile == WebEngineCompatibilityProfile::FullDesktopBrowser ||
                                 profile == WebEngineCompatibilityProfile::RestrictedSecureProfile);
    if (requiresHelper && !isHelperExecutable(helperPath)) {
        report.status = WebEngineCompatibilityStatus::ProcessHelperMissing;
        report.diagnosticMessage =
            "QtWebEngineProcess helper executable is missing or not executable at: '" + helperPath + "'";
        report.remediationAction = "Verify Qt WebEngine installation and ensure QtWebEngineProcess "
                                   "exists with execute permissions.";
        return report;
    }

    // 5. Sandboxing check
    if (!sandboxingSupported) {
        report.status = WebEngineCompatibilityStatus::EnvironmentMisconfigured;
        report.diagnosticMessage = "Process sandboxing is unsupported or restricted by host environment "
                                   "(unprivileged user namespaces disabled).";
        report.remediationAction =
            "Enable unprivileged user namespaces in the host kernel or adjust sandbox permissions.";
        return report;
    }

    report.status = WebEngineCompatibilityStatus::Compatible;
    report.diagnosticMessage = "WebEngine compatibility requirements satisfied for profile " +
                               std::string(profileName(profile)) + " (Chromium " +
                               detectedChromiumVer->toString() + ").";
    report.remediationAction = "None required.";
    return report;
}

WebEngineCompatibilityReport
WebEngineVersionValidator::validateStartup(const std::optional<SemanticVersion>& detectedQtVer,
                                           std::string_view chromiumVerStr,
                                           const std::string& helperPath, bool isHeadlessMode) {
    const auto profile = isHeadlessMode ? WebEngineCompatibilityProfile::MinimalCore
                                        : WebEngineCompatibilityProfile::FullDesktopBrowser;

    std::optional<ChromiumVersion> chromiumVer;
    if (!chromiumVerStr.empty()) {
        chromiumVer = parseChromiumVersion(chromiumVerStr);
    } else if (detectedQtVer.has_value()) {
        chromiumVer = minimumChromiumVersionForQt(*detectedQtVer);
    }

    std::string resolvedHelper = helperPath;
    if (resolvedHelper.empty()) {
        const char* envPath = std::getenv("QTWEBENGINEPROCESS_PATH");
        if (envPath != nullptr && envPath[0] != '\0') {
            resolvedHelper = envPath;
        } else {
            static const char* const candidatePaths[] = {
                "/usr/lib/qt6/libexec/QtWebEngineProcess",
                "/usr/lib/x86_64-linux-gnu/qt6/libexec/QtWebEngineProcess",
                "/usr/libexec/QtWebEngineProcess",
                "/usr/lib/qt/libexec/QtWebEngineProcess",
            };
            for (const char* cand : candidatePaths) {
                if (isHelperExecutable(cand)) {
                    resolvedHelper = cand;
                    break;
                }
            }
        }
    }

    return validateCompatibility(profile, detectedQtVer, chromiumVer, resolvedHelper, true);
}

std::string_view WebEngineVersionValidator::featureName(WebEngineFeature feature) noexcept {
    switch (feature) {
    case WebEngineFeature::ManifestV3Extensions:
        return "ManifestV3Extensions";
    case WebEngineFeature::PdfViewerInternal:
        return "PdfViewerInternal";
    case WebEngineFeature::WebRTC:
        return "WebRTC";
    case WebEngineFeature::DevToolsFrontend:
        return "DevToolsFrontend";
    case WebEngineFeature::ProcessSandboxing:
        return "ProcessSandboxing";
    case WebEngineFeature::HardwareAcceleration:
        return "HardwareAcceleration";
    case WebEngineFeature::LocalDevStorage:
        return "LocalDevStorage";
    case WebEngineFeature::Http3Quic:
        return "Http3Quic";
    }
    return "Unknown";
}

std::string_view WebEngineVersionValidator::profileName(WebEngineCompatibilityProfile profile) noexcept {
    switch (profile) {
    case WebEngineCompatibilityProfile::FullDesktopBrowser:
        return "FullDesktopBrowser";
    case WebEngineCompatibilityProfile::HeadlessInspection:
        return "HeadlessInspection";
    case WebEngineCompatibilityProfile::RestrictedSecureProfile:
        return "RestrictedSecureProfile";
    case WebEngineCompatibilityProfile::MinimalCore:
        return "MinimalCore";
    }
    return "Unknown";
}

std::string WebEngineVersionValidator::formatStatus(WebEngineCompatibilityStatus status) {
    switch (status) {
    case WebEngineCompatibilityStatus::Compatible:
        return "Compatible";
    case WebEngineCompatibilityStatus::DegradedHeadless:
        return "DegradedHeadless";
    case WebEngineCompatibilityStatus::IncompatibleQtVersion:
        return "IncompatibleQtVersion";
    case WebEngineCompatibilityStatus::IncompatibleChromiumVersion:
        return "IncompatibleChromiumVersion";
    case WebEngineCompatibilityStatus::ProcessHelperMissing:
        return "ProcessHelperMissing";
    case WebEngineCompatibilityStatus::FeatureUnsupported:
        return "FeatureUnsupported";
    case WebEngineCompatibilityStatus::EnvironmentMisconfigured:
        return "EnvironmentMisconfigured";
    }
    return "Unknown";
}

std::string WebEngineVersionValidator::formatReport(const WebEngineCompatibilityReport& report) {
    std::string out = "WebEngine Compatibility: " + formatStatus(report.status);
    if (report.detectedQtVersion.has_value()) {
        out += " (Qt: " + report.detectedQtVersion->toString() + ")";
    }
    if (report.detectedChromiumVersion.has_value()) {
        out += " (Chromium: " + report.detectedChromiumVersion->toString() + ")";
    }
    if (!report.helperPath.empty()) {
        out += "\nHelper process: " + report.helperPath;
    }
    if (!report.missingFeatures.empty()) {
        out += "\nMissing features:";
        for (const auto f : report.missingFeatures) {
            out += " ";
            out += featureName(f);
        }
    }
    if (!report.diagnosticMessage.empty()) {
        out += "\nDiagnostics: " + report.diagnosticMessage;
    }
    if (!report.remediationAction.empty()) {
        out += "\nRemediation: " + report.remediationAction;
    }
    return out;
}

} // namespace vigilio
