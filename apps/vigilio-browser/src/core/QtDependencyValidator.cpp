#include "core/QtDependencyValidator.h"

#include <cctype>
#include <charconv>
#include <cstdlib>

namespace vigilio {

std::string SemanticVersion::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::optional<SemanticVersion> SemanticVersion::parse(std::string_view versionStr) {
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

    uint32_t pat = 0;
    if (res.ptr < end && *res.ptr == '.') {
        p = res.ptr + 1;
        if (p < end && std::isdigit(static_cast<unsigned char>(*p))) {
            res = std::from_chars(p, end, pat);
            if (res.ec != std::errc{}) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
    }

    if (res.ptr < end && *res.ptr != '-' && *res.ptr != '+') {
        return std::nullopt;
    }

    return SemanticVersion{maj, min, pat};
}

std::optional<SemanticVersion> QtDependencyValidator::parseVersion(std::string_view versionStr) {
    return SemanticVersion::parse(versionStr);
}

std::vector<QtModuleId> QtDependencyValidator::requiredModules(QtRequirementProfile profile) {
    switch (profile) {
    case QtRequirementProfile::HeadlessCore:
        return {};
    case QtRequirementProfile::MinimalGui:
        return {QtModuleId::Core, QtModuleId::Widgets};
    case QtRequirementProfile::FullBrowser:
        return {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets};
    case QtRequirementProfile::DeveloperTest:
        return {QtModuleId::Core, QtModuleId::Widgets, QtModuleId::WebEngineWidgets, QtModuleId::Test};
    }
    return {};
}

std::string_view QtDependencyValidator::moduleName(QtModuleId id) noexcept {
    switch (id) {
    case QtModuleId::Core:
        return "Core";
    case QtModuleId::Widgets:
        return "Widgets";
    case QtModuleId::WebEngineWidgets:
        return "WebEngineWidgets";
    case QtModuleId::Test:
        return "Test";
    case QtModuleId::Svg:
        return "Svg";
    }
    return "Unknown";
}

std::string QtDependencyValidator::formatOutcome(QtValidationOutcome outcome) {
    switch (outcome) {
    case QtValidationOutcome::Satisfied:
        return "Satisfied";
    case QtValidationOutcome::DegradedHeadless:
        return "DegradedHeadless";
    case QtValidationOutcome::IncompatibleVersion:
        return "IncompatibleVersion";
    case QtValidationOutcome::MissingComponent:
        return "MissingComponent";
    case QtValidationOutcome::EnvironmentError:
        return "EnvironmentError";
    }
    return "Unknown";
}

std::string QtDependencyValidator::formatReport(const QtValidationReport& report) {
    std::string out = "Qt Validation: " + formatOutcome(report.outcome);
    if (report.foundVersion.has_value()) {
        out += " (version: " + report.foundVersion->toString() + ")";
    }
    if (!report.missingModules.empty()) {
        out += "\nMissing modules:";
        for (auto m : report.missingModules) {
            out += " ";
            out += moduleName(m);
        }
    }
    if (!report.diagnosticMessage.empty()) {
        out += "\nDiagnostics: " + report.diagnosticMessage;
    }
    return out;
}

QtValidationReport QtDependencyValidator::validateProfile(QtRequirementProfile profile,
                                                          const std::vector<QtModuleId>& available,
                                                          const SemanticVersion& detectedVer,
                                                          const SemanticVersion& minVer) {
    QtValidationReport report;
    report.foundVersion = detectedVer;
    report.modulesFound = available;

    const auto required = requiredModules(profile);
    if (required.empty()) {
        report.outcome = QtValidationOutcome::Satisfied;
        report.diagnosticMessage = "Headless profile requires no Qt modules.";
        return report;
    }

    for (const auto req : required) {
        bool found = false;
        for (const auto avail : available) {
            if (req == avail) {
                found = true;
                break;
            }
        }
        if (!found) {
            report.missingModules.push_back(req);
        }
    }

    if (!report.missingModules.empty()) {
        report.outcome = QtValidationOutcome::MissingComponent;
        report.diagnosticMessage = "Missing required Qt module(s):";
        for (const auto m : report.missingModules) {
            report.diagnosticMessage += " ";
            report.diagnosticMessage += moduleName(m);
        }
        return report;
    }

    if (!detectedVer.isCompatibleWith(minVer)) {
        report.outcome = QtValidationOutcome::IncompatibleVersion;
        report.diagnosticMessage = "Qt version " + detectedVer.toString() +
                                   " is incompatible with required minimum " + minVer.toString();
        return report;
    }

    report.outcome = QtValidationOutcome::Satisfied;
    report.diagnosticMessage =
        "All required Qt dependencies satisfied (" + detectedVer.toString() + ").";
    return report;
}

QtValidationReport QtDependencyValidator::validateRuntimeEnvironment(std::string_view runtimeVer,
                                                                     std::string_view compileVer,
                                                                     const char* displayEnv,
                                                                     const char* waylandEnv) {
    QtValidationReport report;

    const auto rVer = SemanticVersion::parse(runtimeVer);
    const auto cVer = SemanticVersion::parse(compileVer);

    if (!rVer.has_value() || !cVer.has_value()) {
        report.outcome = QtValidationOutcome::EnvironmentError;
        report.diagnosticMessage = "Failed to parse Qt version: runtime='" + std::string(runtimeVer) +
                                   "', compile='" + std::string(compileVer) + "'";
        return report;
    }

    report.foundVersion = *rVer;

    // ABI consistency check: major version mismatch
    if (rVer->major != cVer->major) {
        report.outcome = QtValidationOutcome::EnvironmentError;
        report.diagnosticMessage =
            "Qt runtime ABI version mismatch: compile major=" + std::to_string(cVer->major) + " (" +
            std::string(compileVer) + "), runtime major=" + std::to_string(rVer->major) + " (" +
            std::string(runtimeVer) + ")";
        return report;
    }

    // Minimum required version: Qt >= 6.10.0
    const SemanticVersion minVersion{6, 10, 0};
    if (!rVer->isCompatibleWith(minVersion)) {
        report.outcome = QtValidationOutcome::IncompatibleVersion;
        report.diagnosticMessage = "Qt runtime version incompatible: found " + rVer->toString() +
                                   ", required minimum is " + minVersion.toString();
        return report;
    }

#if defined(__linux__)
    const char* qpaPlatform = std::getenv("QT_QPA_PLATFORM");
    const bool isOffscreen = (qpaPlatform != nullptr && (std::string_view(qpaPlatform) == "offscreen" ||
                                                         std::string_view(qpaPlatform) == "minimal"));
    const bool hasDisplay = (displayEnv != nullptr && displayEnv[0] != '\0');
    const bool hasWayland = (waylandEnv != nullptr && waylandEnv[0] != '\0');
    if (!hasDisplay && !hasWayland && !isOffscreen) {
        report.outcome = QtValidationOutcome::EnvironmentError;
        report.diagnosticMessage =
            "No graphical display server detected (neither DISPLAY nor WAYLAND_DISPLAY is set). "
            "Desktop GUI requires an active display server or offscreen platform.";
        return report;
    }
#else
    (void)displayEnv;
    (void)waylandEnv;
#endif

    report.outcome = QtValidationOutcome::Satisfied;
    report.diagnosticMessage =
        "Qt runtime environment validation satisfied (Qt " + rVer->toString() + ").";
    return report;
}

} // namespace vigilio
