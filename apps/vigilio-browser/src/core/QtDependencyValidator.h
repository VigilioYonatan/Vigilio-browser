#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

enum class QtModuleId { Core, Widgets, WebEngineWidgets, Test, Svg };

struct SemanticVersion {
    uint32_t major{0};
    uint32_t minor{0};
    uint32_t patch{0};

    auto operator<=>(const SemanticVersion&) const = default;

    [[nodiscard]] bool isCompatibleWith(const SemanticVersion& requiredMin) const noexcept {
        if (major != requiredMin.major) {
            return false;
        }
        return *this >= requiredMin;
    }

    [[nodiscard]] std::string toString() const;
    static std::optional<SemanticVersion> parse(std::string_view versionStr);
};

enum class QtRequirementProfile { HeadlessCore, MinimalGui, FullBrowser, DeveloperTest };

enum class QtValidationOutcome {
    Satisfied,
    DegradedHeadless,
    IncompatibleVersion,
    MissingComponent,
    EnvironmentError
};

struct QtValidationReport {
    QtValidationOutcome outcome{QtValidationOutcome::Satisfied};
    std::optional<SemanticVersion> foundVersion{};
    std::vector<QtModuleId> modulesFound{};
    std::vector<QtModuleId> missingModules{};
    std::string diagnosticMessage{};
};

class QtDependencyValidator {
  public:
    static std::optional<SemanticVersion> parseVersion(std::string_view versionStr);

    static std::vector<QtModuleId> requiredModules(QtRequirementProfile profile);

    static QtValidationReport validateProfile(QtRequirementProfile profile,
                                              const std::vector<QtModuleId>& available,
                                              const SemanticVersion& detectedVer,
                                              const SemanticVersion& minVer = {6, 10, 0});

    static QtValidationReport validateRuntimeEnvironment(std::string_view runtimeVer,
                                                         std::string_view compileVer,
                                                         const char* displayEnv, const char* waylandEnv);

    static std::string formatOutcome(QtValidationOutcome outcome);

    static std::string formatReport(const QtValidationReport& report);

    static std::string_view moduleName(QtModuleId id) noexcept;
};

} // namespace vigilio
