#pragma once

#include "core/QtDependencyValidator.h"
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Estructura inmutable que modela la versión cuatripartita de Chromium
/// (major.minor.build.patch).
struct ChromiumVersion {
    uint32_t major{0}; // Milestone (e.g. 128)
    uint32_t minor{0}; // Sub-milestone (usually 0)
    uint32_t build{0}; // Build number (e.g. 6613)
    uint32_t patch{0}; // Patch level (e.g. 119)

    auto operator<=>(const ChromiumVersion&) const = default;

    [[nodiscard]] constexpr uint32_t milestone() const noexcept {
        return major;
    }

    [[nodiscard]] bool isCompatibleWith(const ChromiumVersion& requiredMin) const noexcept {
        return *this >= requiredMin;
    }

    [[nodiscard]] std::string toString() const;
    static std::optional<ChromiumVersion> parse(std::string_view versionStr);
};

/// @brief Capacidades funcionales del motor Chromium embebido en Qt WebEngine.
enum class WebEngineFeature {
    ManifestV3Extensions,
    PdfViewerInternal,
    WebRTC,
    DevToolsFrontend,
    ProcessSandboxing,
    HardwareAcceleration,
    LocalDevStorage,
    Http3Quic
};

/// @brief Perfiles declarativos de compatibilidad de WebEngine.
enum class WebEngineCompatibilityProfile {
    FullDesktopBrowser,
    HeadlessInspection,
    RestrictedSecureProfile,
    MinimalCore
};

/// @brief Estados deterministas del resultado de validación de WebEngine.
enum class WebEngineCompatibilityStatus {
    Compatible,
    DegradedHeadless,
    IncompatibleQtVersion,
    IncompatibleChromiumVersion,
    ProcessHelperMissing,
    FeatureUnsupported,
    EnvironmentMisconfigured
};

/// @brief Estructura inmutable de reporte que consolida el resultado de la validación.
struct WebEngineCompatibilityReport {
    WebEngineCompatibilityStatus status{WebEngineCompatibilityStatus::Compatible};
    std::optional<SemanticVersion> detectedQtVersion{};
    std::optional<ChromiumVersion> detectedChromiumVersion{};
    std::vector<WebEngineFeature> supportedFeatures{};
    std::vector<WebEngineFeature> missingFeatures{};
    std::string helperPath{};
    std::string diagnosticMessage{};
    std::string remediationAction{};
};

/// @brief Servicio de dominio desacoplado para validación de compatibilidad con WebEngine.
class WebEngineVersionValidator {
  public:
    static std::optional<ChromiumVersion> parseChromiumVersion(std::string_view versionStr);

    static constexpr ChromiumVersion baselineChromiumVersion() noexcept {
        return {128, 0, 0, 0};
    }

    static ChromiumVersion minimumChromiumVersionForQt(const SemanticVersion& qtVer);

    static std::vector<WebEngineFeature> supportedFeaturesForMilestone(uint32_t milestone);

    static std::vector<WebEngineFeature>
    requiredFeaturesForProfile(WebEngineCompatibilityProfile profile);

    static bool isHelperExecutable(const std::string& helperPath);

    static WebEngineCompatibilityReport
    validateCompatibility(WebEngineCompatibilityProfile profile,
                          const std::optional<SemanticVersion>& detectedQtVer,
                          const std::optional<ChromiumVersion>& detectedChromiumVer,
                          const std::string& helperPath, bool sandboxingSupported = true);

    static WebEngineCompatibilityReport
    validateStartup(const std::optional<SemanticVersion>& detectedQtVer, std::string_view chromiumVerStr,
                    const std::string& helperPath, bool isHeadlessMode);

    static std::string formatStatus(WebEngineCompatibilityStatus status);

    static std::string formatReport(const WebEngineCompatibilityReport& report);

    static std::string_view featureName(WebEngineFeature feature) noexcept;

    static std::string_view profileName(WebEngineCompatibilityProfile profile) noexcept;
};

} // namespace vigilio
