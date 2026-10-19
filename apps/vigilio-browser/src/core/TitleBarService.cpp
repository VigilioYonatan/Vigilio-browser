#include "core/TitleBarService.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace vigilio {

namespace {

enum class JsonValueType : std::uint8_t { Null, Boolean, Number, String, Object };

struct SimpleJsonValue;

struct SimpleJsonObjectMember {
    std::string key;
    std::shared_ptr<SimpleJsonValue> value;
};

struct SimpleJsonValue {
    JsonValueType type{JsonValueType::Null};
    bool boolVal{false};
    double numVal{0.0};
    std::string strVal;
    std::vector<SimpleJsonObjectMember> objVal;

    [[nodiscard]] const SimpleJsonValue* find(std::string_view key) const {
        if (type != JsonValueType::Object) {
            return nullptr;
        }
        for (const auto& member : objVal) {
            if (member.key == key) {
                return member.value.get();
            }
        }
        return nullptr;
    }
};

class SimpleJsonParser {
  public:
    explicit SimpleJsonParser(std::string_view text) : m_text(text), m_pos(0) {
    }

    bool parse(SimpleJsonValue& out) {
        skipWhitespace();
        if (m_pos >= m_text.size()) {
            return false;
        }
        if (!parseValue(out)) {
            return false;
        }
        skipWhitespace();
        return m_pos == m_text.size();
    }

  private:
    std::string_view m_text;
    std::size_t m_pos{0};

    void skipWhitespace() {
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++m_pos;
            } else {
                break;
            }
        }
    }

    bool parseValue(SimpleJsonValue& out) {
        skipWhitespace();
        if (m_pos >= m_text.size()) {
            return false;
        }

        const char c = m_text[m_pos];
        if (c == 'n') {
            return parseNull(out);
        }
        if (c == 't' || c == 'f') {
            return parseBool(out);
        }
        if (c == '"') {
            return parseString(out);
        }
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(out);
        }
        return false;
    }

    bool parseNull(SimpleJsonValue& out) {
        if (m_text.substr(m_pos, 4) == "null") {
            m_pos += 4;
            out.type = JsonValueType::Null;
            return true;
        }
        return false;
    }

    bool parseBool(SimpleJsonValue& out) {
        if (m_text.substr(m_pos, 4) == "true") {
            m_pos += 4;
            out.type = JsonValueType::Boolean;
            out.boolVal = true;
            return true;
        }
        if (m_text.substr(m_pos, 5) == "false") {
            m_pos += 5;
            out.type = JsonValueType::Boolean;
            out.boolVal = false;
            return true;
        }
        return false;
    }

    bool parseString(SimpleJsonValue& out) {
        if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
            return false;
        }
        ++m_pos;

        std::string s;
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos++];
            if (c == '"') {
                out.type = JsonValueType::String;
                out.strVal = std::move(s);
                return true;
            }
            if (c == '\\') {
                if (m_pos >= m_text.size()) {
                    return false;
                }
                const char esc = m_text[m_pos++];
                switch (esc) {
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                case '/':
                    s.push_back('/');
                    break;
                case 'b':
                    s.push_back('\b');
                    break;
                case 'f':
                    s.push_back('\f');
                    break;
                case 'n':
                    s.push_back('\n');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                default:
                    s.push_back(esc);
                    break;
                }
            } else {
                s.push_back(c);
            }
        }
        return false;
    }

    bool parseNumber(SimpleJsonValue& out) {
        const std::size_t start = m_pos;
        if (m_pos < m_text.size() && m_text[m_pos] == '-') {
            ++m_pos;
        }
        while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') {
            ++m_pos;
        }
        if (m_pos < m_text.size() && m_text[m_pos] == '.') {
            ++m_pos;
            while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') {
                ++m_pos;
            }
        }

        const auto numStr = std::string(m_text.substr(start, m_pos - start));
        try {
            out.type = JsonValueType::Number;
            out.numVal = std::stod(numStr);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool parseObject(SimpleJsonValue& out) {
        if (m_pos >= m_text.size() || m_text[m_pos] != '{') {
            return false;
        }
        ++m_pos;
        out.type = JsonValueType::Object;
        out.objVal.clear();

        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            skipWhitespace();
            SimpleJsonValue keyVal;
            if (!parseString(keyVal)) {
                return false;
            }

            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
                return false;
            }
            ++m_pos;

            auto child = std::make_shared<SimpleJsonValue>();
            if (!parseValue(*child)) {
                return false;
            }

            out.objVal.push_back(SimpleJsonObjectMember{std::move(keyVal.strVal), std::move(child)});

            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                ++m_pos;
                return true;
            }
            return false;
        }
        return false;
    }
};

std::string serializeTitleBarStateToJson(const TitleBarConfig& config, int windowWidth) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": 1,\n";
    ss << "  \"window_width\": " << windowWidth << ",\n";
    ss << "  \"mode\": \"";
    switch (config.mode) {
    case TitleBarMode::NativeSystem:
        ss << "native_system";
        break;
    case TitleBarMode::CustomIntegrated:
        ss << "custom_integrated";
        break;
    case TitleBarMode::Suppressed:
        ss << "suppressed";
        break;
    }
    ss << "\",\n";
    ss << "  \"placement\": \""
       << (config.placement == TitleBarControlsPlacement::Right ? "right" : "left") << "\",\n";
    ss << "  \"title\": \"";
    for (char c : config.title) {
        if (c == '"') {
            ss << "\\\"";
        } else if (c == '\\') {
            ss << "\\\\";
        } else {
            ss << c;
        }
    }
    ss << "\",\n";
    ss << "  \"show_icon\": " << (config.showIcon ? "true" : "false") << ",\n";
    ss << "  \"show_senior_toggle\": " << (config.showSeniorToggle ? "true" : "false") << ",\n";
    ss << "  \"density\": \"";
    switch (config.density) {
    case LayoutDensity::Senior:
        ss << "senior";
        break;
    case LayoutDensity::Compact:
        ss << "compact";
        break;
    case LayoutDensity::Default:
        ss << "default";
        break;
    }
    ss << "\"\n";
    ss << "}\n";
    return ss.str();
}

bool deserializeTitleBarStateFromJson(std::string_view text, TitleBarConfig& outConfig, int& outWidth) {
    SimpleJsonValue root;
    SimpleJsonParser parser(text);
    if (!parser.parse(root) || root.type != JsonValueType::Object) {
        return false;
    }

    const auto* modeVal = root.find("mode");
    const auto* placementVal = root.find("placement");
    const auto* titleVal = root.find("title");
    const auto* showIconVal = root.find("show_icon");
    const auto* showSeniorVal = root.find("show_senior_toggle");
    const auto* densityVal = root.find("density");

    if (!modeVal || !placementVal || !titleVal || !showIconVal || !showSeniorVal || !densityVal) {
        return false;
    }

    if (modeVal->type != JsonValueType::String || placementVal->type != JsonValueType::String ||
        titleVal->type != JsonValueType::String || showIconVal->type != JsonValueType::Boolean ||
        showSeniorVal->type != JsonValueType::Boolean || densityVal->type != JsonValueType::String) {
        return false;
    }

    if (modeVal->strVal == "custom_integrated" || modeVal->strVal == "CustomIntegrated") {
        outConfig.mode = TitleBarMode::CustomIntegrated;
    } else if (modeVal->strVal == "native_system" || modeVal->strVal == "NativeSystem") {
        outConfig.mode = TitleBarMode::NativeSystem;
    } else if (modeVal->strVal == "suppressed" || modeVal->strVal == "Suppressed") {
        outConfig.mode = TitleBarMode::Suppressed;
    } else {
        return false;
    }

    if (placementVal->strVal == "right" || placementVal->strVal == "Right") {
        outConfig.placement = TitleBarControlsPlacement::Right;
    } else if (placementVal->strVal == "left" || placementVal->strVal == "Left") {
        outConfig.placement = TitleBarControlsPlacement::Left;
    } else {
        return false;
    }

    outConfig.title = titleVal->strVal;
    outConfig.showIcon = showIconVal->boolVal;
    outConfig.showSeniorToggle = showSeniorVal->boolVal;

    if (densityVal->strVal == "senior" || densityVal->strVal == "Senior") {
        outConfig.density = LayoutDensity::Senior;
        outConfig.state.seniorModeActive = true;
    } else if (densityVal->strVal == "compact" || densityVal->strVal == "Compact") {
        outConfig.density = LayoutDensity::Compact;
        outConfig.state.seniorModeActive = false;
    } else if (densityVal->strVal == "default" || densityVal->strVal == "Default") {
        outConfig.density = LayoutDensity::Default;
        outConfig.state.seniorModeActive = false;
    } else {
        return false;
    }

    const auto* widthVal = root.find("window_width");
    if (!widthVal) {
        widthVal = root.find("windowWidth");
    }
    if (widthVal && widthVal->type == JsonValueType::Number) {
        outWidth = static_cast<int>(widthVal->numVal);
    } else {
        outWidth = 1200;
    }

    return true;
}

} // namespace

TitleBarService::TitleBarService()
    : config_{}, plan_{TitleBarConfiguration::canonicalDefaultTitleBar(1200)}, windowWidth_{1200},
      options_{} {
}

TitleBarService::TitleBarService(TitleBarConfig initialConfig, int windowWidth,
                                 TitleBarServiceOptions options)
    : config_{std::move(initialConfig)}, windowWidth_{windowWidth}, options_{std::move(options)} {
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (report.valid) {
        plan_ = report.plan;
    } else {
        plan_ =
            TitleBarConfiguration::canonicalDefaultTitleBar(windowWidth_ >= 400 ? windowWidth_ : 1200);
    }
}

TitleBarService::TitleBarService(LayoutDensity density, int windowWidth, TitleBarServiceOptions options)
    : windowWidth_{windowWidth}, options_{std::move(options)} {
    config_.density = density;
    if (density == LayoutDensity::Senior) {
        config_.showSeniorToggle = true;
        config_.state.seniorModeActive = true;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthSenior) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthSenior;
        }
        plan_ = TitleBarConfiguration::canonicalSeniorTitleBar(windowWidth_);
    } else if (density == LayoutDensity::Compact) {
        config_.state.seniorModeActive = false;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthCompact) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthCompact;
        }
        plan_ = TitleBarConfiguration::canonicalCompactTitleBar(windowWidth_);
    } else {
        config_.state.seniorModeActive = false;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthDefault) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthDefault;
        }
        plan_ = TitleBarConfiguration::canonicalDefaultTitleBar(windowWidth_);
    }
}

TitleBarService::TitleBarService(TitleBarLayoutPlan initialPlan, TitleBarServiceOptions options)
    : plan_{std::move(initialPlan)}, windowWidth_{plan_.windowWidth}, options_{std::move(options)} {
    config_.density = plan_.density;
    config_.mode = plan_.mode;
    config_.placement = plan_.placement;
    config_.state = plan_.state;
    config_.showIcon = (plan_.iconRect.width > 0);
    for (const auto& ctrl : plan_.controls) {
        if (ctrl.type == TitleBarButtonType::SeniorModeToggle) {
            config_.showSeniorToggle = true;
        }
    }
}

TitleBarService::TitleBarService(TitleBarService&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = std::move(other.config_);
    plan_ = std::move(other.plan_);
    windowWidth_ = other.windowWidth_;
    options_ = std::move(other.options_);
    callback_ = std::move(other.callback_);
}

TitleBarService& TitleBarService::operator=(TitleBarService&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        config_ = std::move(other.config_);
        plan_ = std::move(other.plan_);
        windowWidth_ = other.windowWidth_;
        options_ = std::move(other.options_);
        callback_ = std::move(other.callback_);
    }
    return *this;
}

TitleBarConfig TitleBarService::config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

TitleBarLayoutPlan TitleBarService::plan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_;
}

int TitleBarService::windowWidth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return windowWidth_;
}

TitleBarServiceOptions TitleBarService::options() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

void TitleBarService::setOptions(const TitleBarServiceOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
}

TitleBarReport TitleBarService::setDensity(LayoutDensity density) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupWidth = windowWidth_;
    const auto backupPlan = plan_;

    config_.density = density;
    if (density == LayoutDensity::Senior) {
        config_.showSeniorToggle = true;
        config_.state.seniorModeActive = true;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthSenior) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthSenior;
        }
    } else if (density == LayoutDensity::Compact) {
        config_.state.seniorModeActive = false;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthCompact) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthCompact;
        }
    } else {
        config_.state.seniorModeActive = false;
        if (windowWidth_ < TitleBarConfiguration::kMinWindowWidthDefault) {
            windowWidth_ = TitleBarConfiguration::kMinWindowWidthDefault;
        }
    }

    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        windowWidth_ = backupWidth;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setMode(TitleBarMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    config_.mode = mode;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setControlsPlacement(TitleBarControlsPlacement placement) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    config_.placement = placement;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setTitle(std::string_view title) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    std::string_view trimmed = title;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t' ||
                                trimmed.front() == '\r' || trimmed.front() == '\n')) {
        trimmed.remove_prefix(1);
    }
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' ||
                                trimmed.back() == '\r' || trimmed.back() == '\n')) {
        trimmed.remove_suffix(1);
    }

    config_.title = TitleBarValidator::sanitizeTitle(trimmed, options_.isIncognitoSession);
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setShowIcon(bool show) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    config_.showIcon = show;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setShowSeniorToggle(bool show) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    config_.showSeniorToggle = show;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::setWindowState(const TitleBarState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupPlan = plan_;

    config_.state = state;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        config_ = backupConfig;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    if (state.isFullscreen) {
        for (auto& ctrl : plan_.controls) {
            ctrl.visible = false;
        }
    }
    report.plan = plan_;
    notifyChangedLocked(report);
    return report;
}

TitleBarReport TitleBarService::updateButtonState(TitleBarButtonType buttonType,
                                                  TitleBarButtonState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& ctrl : plan_.controls) {
        if (ctrl.type == buttonType) {
            ctrl.state = state;
            found = true;
        }
    }

    TitleBarReport report{};
    report.valid = found;
    report.error = found ? TitleBarError::None : TitleBarError::ConflictingControlPlacement;
    report.plan = plan_;
    report.message =
        found ? "Estado de botón actualizado." : "Botón no encontrado en los controles activos.";
    report.remediationHint = TitleBarValidator::generateRemediationHint(report.error);

    if (found) {
        notifyChangedLocked(report);
    }
    return report;
}

TitleBarReport TitleBarService::setWindowWidth(int windowWidth) {
    std::lock_guard<std::mutex> lock(mutex_);

    int minW = TitleBarConfiguration::kMinWindowWidthDefault;
    if (config_.density == LayoutDensity::Senior) {
        minW = TitleBarConfiguration::kMinWindowWidthSenior;
    } else if (config_.density == LayoutDensity::Compact) {
        minW = TitleBarConfiguration::kMinWindowWidthCompact;
    }

    if (windowWidth < minW) {
        TitleBarReport report{};
        report.valid = false;
        report.error = TitleBarError::DimensionsBelowMinimum;
        report.plan = plan_;
        report.message = TitleBarValidator::titleBarErrorToString(TitleBarError::DimensionsBelowMinimum);
        report.remediationHint =
            TitleBarValidator::generateRemediationHint(TitleBarError::DimensionsBelowMinimum);
        return report;
    }

    const auto backupWidth = windowWidth_;
    const auto backupPlan = plan_;

    windowWidth_ = windowWidth;
    auto report = TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
    if (!report.valid) {
        windowWidth_ = backupWidth;
        plan_ = backupPlan;
        return report;
    }

    plan_ = report.plan;
    notifyChangedLocked(report);
    return report;
}

TitleBarHitTestZone TitleBarService::hitTest(int x, int y) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return TitleBarValidator::hitTest(x, y, plan_);
}

TitleBarReport TitleBarService::computeLayout(CancellationToken* cancel) const {
    if (cancel != nullptr && cancel->isCancelled()) {
        TitleBarReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = TitleBarError::None;
        cancelledReport.message = "Cálculo de barra de título cancelado.";
        cancelledReport.remediationHint =
            "Reintente la operación cuando no haya solicitudes de cancelación activas.";
        return cancelledReport;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (cancel != nullptr && cancel->isCancelled()) {
        TitleBarReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = TitleBarError::None;
        cancelledReport.message = "Cálculo de barra de título cancelado.";
        cancelledReport.remediationHint =
            "Reintente la operación cuando no haya solicitudes de cancelación activas.";
        return cancelledReport;
    }

    return TitleBarValidator::calculateTitleBarLayout(windowWidth_, config_);
}

bool TitleBarService::saveStateToFile(const std::filesystem::path& path, CancellationToken* cancel) {
    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (options_.isIncognitoSession) {
        return false;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    try {
        if (path.empty()) {
            return false;
        }

        const auto parent = path.parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                return false;
            }
        }

        const auto tempPath = path.string() + ".tmp";
        {
            std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
            if (!out) {
                return false;
            }

            const std::string content = serializeTitleBarStateToJson(config_, windowWidth_);
            out.write(content.data(), static_cast<std::streamsize>(content.size()));
            out.flush();
            if (!out) {
                std::error_code rmEc;
                std::filesystem::remove(tempPath, rmEc);
                return false;
            }
        }

        if (cancel != nullptr && cancel->isCancelled()) {
            std::error_code rmEc;
            std::filesystem::remove(tempPath, rmEc);
            return false;
        }

        std::error_code ec;
        std::filesystem::rename(tempPath, path, ec);
        if (ec) {
            std::error_code rmEc;
            std::filesystem::remove(tempPath, rmEc);
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool TitleBarService::loadStateFromFile(const std::filesystem::path& path, CancellationToken* cancel) {
    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (options_.isIncognitoSession) {
        return false;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        restoreDefaultLocked();
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        restoreDefaultLocked();
        return false;
    }

    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    TitleBarConfig loadedConfig{};
    int loadedWidth = 1200;
    if (!deserializeTitleBarStateFromJson(text, loadedConfig, loadedWidth)) {
        restoreDefaultLocked();
        return false;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    auto report = TitleBarValidator::calculateTitleBarLayout(loadedWidth, loadedConfig);
    if (!report.valid) {
        restoreDefaultLocked();
        return false;
    }

    config_ = loadedConfig;
    windowWidth_ = loadedWidth;
    plan_ = report.plan;
    notifyChangedLocked(report);
    return true;
}

bool TitleBarService::saveCurrentState(CancellationToken* cancel) {
    if (options_.stateFilePath.empty()) {
        return false;
    }
    return saveStateToFile(options_.stateFilePath, cancel);
}

bool TitleBarService::restoreDefault() {
    std::lock_guard<std::mutex> lock(mutex_);
    restoreDefaultLocked();
    return true;
}

void TitleBarService::restoreDefaultLocked() {
    config_ = TitleBarConfig{};
    windowWidth_ = 1200;
    plan_ = TitleBarConfiguration::canonicalDefaultTitleBar(1200);
    TitleBarReport report{};
    report.valid = true;
    report.error = TitleBarError::None;
    report.plan = plan_;
    report.message = "Configuración canónica por defecto restaurada.";
    report.remediationHint = TitleBarValidator::generateRemediationHint(TitleBarError::None);
    notifyChangedLocked(report);
}

void TitleBarService::setIncognitoMode(bool incognito) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    options_.isIncognitoSession = incognito;
}

bool TitleBarService::isIncognitoMode() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_.isIncognitoSession;
}

void TitleBarService::setTitleBarChangedCallback(OnTitleBarChangedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void TitleBarService::clearTitleBarChangedCallback() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = nullptr;
}

void TitleBarService::notifyChangedLocked(const TitleBarReport& report) {
    if (callback_) {
        callback_(plan_, report);
    }
}

std::string TitleBarService::formatTitleBarSummary(const TitleBarLayoutPlan& plan,
                                                   const TitleBarReport& report) {
    std::string summary;
    summary.reserve(512);
    summary += "Barra de título: ";
    summary += TitleBarValidator::titleBarModeToString(plan.mode);
    summary += " | Densidad: ";
    if (plan.density == LayoutDensity::Senior) {
        summary += "Interfaz más fácil de usar";
    } else if (plan.density == LayoutDensity::Compact) {
        summary += "Compacta";
    } else {
        summary += "Estándar";
    }
    summary += " | Dimensiones: ";
    summary += std::to_string(plan.windowWidth);
    summary += "x";
    summary += std::to_string(plan.height);
    summary += " px | Controles: ";
    summary += (plan.placement == TitleBarControlsPlacement::Right ? "Derecha" : "Izquierda");
    summary += " (";
    summary += std::to_string(plan.controls.size());
    summary += " botones)";
    summary += " | Área de arrastre: ";
    summary += std::to_string(plan.draggableRect.width);
    summary += " px | Estado: ";
    summary += report.valid ? "Válida" : "Inválida";
    if (!report.valid && !report.message.empty()) {
        summary += " - ";
        summary += report.message;
    }
    return summary;
}

} // namespace vigilio
