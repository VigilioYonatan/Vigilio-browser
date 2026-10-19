#include "core/TabStripDensityService.h"

#include <algorithm>
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

enum class JsonValueType : std::uint8_t { Null, Boolean, Number, String, Object, Array };

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
    std::vector<std::shared_ptr<SimpleJsonValue>> arrVal;

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
        if (c == '[') {
            return parseArray(out);
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

    bool parseArray(SimpleJsonValue& out) {
        if (m_pos >= m_text.size() || m_text[m_pos] != '[') {
            return false;
        }
        ++m_pos;
        out.type = JsonValueType::Array;
        out.arrVal.clear();

        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            auto child = std::make_shared<SimpleJsonValue>();
            if (!parseValue(*child)) {
                return false;
            }
            out.arrVal.push_back(std::move(child));

            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_pos < m_text.size() && m_text[m_pos] == ']') {
                ++m_pos;
                return true;
            }
            return false;
        }
        return false;
    }
};

std::string escapeJsonString(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\b') {
            out += "\\b";
        } else if (c == '\f') {
            out += "\\f";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out += c;
        }
    }
    return out;
}

std::string sanitizeUrl(std::string_view rawUrl) {
    std::string url(rawUrl);
    const auto atPos = url.find('@');
    const auto schemePos = url.find("://");
    if (atPos != std::string::npos && schemePos != std::string::npos && atPos > schemePos + 3) {
        const auto slashPos = url.find('/', schemePos + 3);
        if (atPos < slashPos || slashPos == std::string::npos) {
            url.erase(schemePos + 3, atPos - (schemePos + 3) + 1);
        }
    }
    const auto qPos = url.find('?');
    if (qPos != std::string::npos) {
        if (url.find("token=", qPos) != std::string::npos ||
            url.find("access_token=", qPos) != std::string::npos ||
            url.find("password=", qPos) != std::string::npos ||
            url.find("secret=", qPos) != std::string::npos) {
            url.erase(qPos);
        }
    }
    return url;
}

std::string serializeTabStripStateToJson(const TabStripDensityConfig& config, int availableWidth,
                                         const std::vector<TabSpec>& tabs) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": 1,\n";
    ss << "  \"available_width\": " << availableWidth << ",\n";
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
    ss << "\",\n";
    ss << "  \"show_new_tab_button\": " << (config.showNewTabButton ? "true" : "false") << ",\n";
    ss << "  \"overflow_mode\": \"";
    switch (config.overflowMode) {
    case TabStripOverflowMode::Scrollable:
        ss << "scrollable";
        break;
    case TabStripOverflowMode::DropdownMenu:
        ss << "dropdown_menu";
        break;
    case TabStripOverflowMode::None:
        ss << "none";
        break;
    }
    ss << "\",\n";
    ss << "  \"scroll_offset\": " << config.scrollOffset << ",\n";
    ss << "  \"tabs\": [\n";

    for (std::size_t i = 0; i < tabs.size(); ++i) {
        const auto& tab = tabs[i];
        ss << "    {\n";
        ss << "      \"id\": " << tab.id << ",\n";
        ss << "      \"title\": \"" << escapeJsonString(tab.title) << "\",\n";
        ss << "      \"url\": \"" << escapeJsonString(tab.url) << "\",\n";
        ss << "      \"pinned\": " << (tab.pinState == TabPinState::Pinned ? "true" : "false") << ",\n";
        ss << "      \"selected\": " << (tab.isSelected ? "true" : "false") << ",\n";
        ss << "      \"media_state\": \"";
        switch (tab.mediaState) {
        case TabMediaState::AudioPlaying:
            ss << "audio_playing";
            break;
        case TabMediaState::AudioMuted:
            ss << "audio_muted";
            break;
        case TabMediaState::None:
            ss << "none";
            break;
        }
        ss << "\",\n";
        ss << "      \"discard_state\": \""
           << (tab.discardState == TabDiscardState::Discarded ? "discarded" : "active") << "\",\n";
        ss << "      \"loading_state\": \"";
        switch (tab.loadingState) {
        case TabLoadingState::Loading:
            ss << "loading";
            break;
        case TabLoadingState::Failed:
            ss << "failed";
            break;
        case TabLoadingState::Idle:
            ss << "idle";
            break;
        }
        ss << "\"\n";
        ss << "    }" << (i + 1 < tabs.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

bool deserializeTabStripStateFromJson(std::string_view text, TabStripDensityConfig& outConfig,
                                      int& outWidth, std::vector<TabSpec>& outTabs) {
    SimpleJsonValue root;
    SimpleJsonParser parser(text);
    if (!parser.parse(root) || root.type != JsonValueType::Object) {
        return false;
    }

    const auto* densityVal = root.find("density");
    const auto* showNewTabVal = root.find("show_new_tab_button");
    const auto* tabsVal = root.find("tabs");

    if (!densityVal || densityVal->type != JsonValueType::String) {
        return false;
    }

    if (densityVal->strVal == "senior" || densityVal->strVal == "Senior") {
        outConfig.density = LayoutDensity::Senior;
    } else if (densityVal->strVal == "compact" || densityVal->strVal == "Compact") {
        outConfig.density = LayoutDensity::Compact;
    } else if (densityVal->strVal == "default" || densityVal->strVal == "Default") {
        outConfig.density = LayoutDensity::Default;
    } else {
        return false;
    }

    if (showNewTabVal && showNewTabVal->type == JsonValueType::Boolean) {
        outConfig.showNewTabButton = showNewTabVal->boolVal;
    } else {
        outConfig.showNewTabButton = true;
    }

    const auto* overflowModeVal = root.find("overflow_mode");
    if (overflowModeVal && overflowModeVal->type == JsonValueType::String) {
        if (overflowModeVal->strVal == "dropdown_menu") {
            outConfig.overflowMode = TabStripOverflowMode::DropdownMenu;
        } else if (overflowModeVal->strVal == "none") {
            outConfig.overflowMode = TabStripOverflowMode::None;
        } else {
            outConfig.overflowMode = TabStripOverflowMode::Scrollable;
        }
    } else {
        outConfig.overflowMode = TabStripOverflowMode::Scrollable;
    }

    const auto* scrollOffsetVal = root.find("scroll_offset");
    if (scrollOffsetVal && scrollOffsetVal->type == JsonValueType::Number) {
        outConfig.scrollOffset = static_cast<int>(scrollOffsetVal->numVal);
    } else {
        outConfig.scrollOffset = 0;
    }

    const auto* widthVal = root.find("available_width");
    if (!widthVal) {
        widthVal = root.find("availableWidth");
    }
    if (widthVal && widthVal->type == JsonValueType::Number) {
        outWidth = static_cast<int>(widthVal->numVal);
    } else {
        outWidth = 1000;
    }

    outTabs.clear();
    if (tabsVal && tabsVal->type == JsonValueType::Array) {
        outTabs.reserve(tabsVal->arrVal.size());
        for (const auto& itemPtr : tabsVal->arrVal) {
            if (!itemPtr || itemPtr->type != JsonValueType::Object) {
                continue;
            }
            TabSpec spec{};
            const auto* idVal = itemPtr->find("id");
            if (idVal && idVal->type == JsonValueType::Number) {
                spec.id = static_cast<std::uint64_t>(idVal->numVal);
            }
            const auto* titleVal = itemPtr->find("title");
            if (titleVal && titleVal->type == JsonValueType::String) {
                spec.title = titleVal->strVal;
            }
            const auto* urlVal = itemPtr->find("url");
            if (urlVal && urlVal->type == JsonValueType::String) {
                spec.url = urlVal->strVal;
            }
            const auto* pinnedVal = itemPtr->find("pinned");
            if (pinnedVal && pinnedVal->type == JsonValueType::Boolean && pinnedVal->boolVal) {
                spec.pinState = TabPinState::Pinned;
            } else {
                spec.pinState = TabPinState::Unpinned;
            }
            const auto* selectedVal = itemPtr->find("selected");
            if (selectedVal && selectedVal->type == JsonValueType::Boolean) {
                spec.isSelected = selectedVal->boolVal;
            }
            const auto* mediaVal = itemPtr->find("media_state");
            if (mediaVal && mediaVal->type == JsonValueType::String) {
                if (mediaVal->strVal == "audio_playing") {
                    spec.mediaState = TabMediaState::AudioPlaying;
                } else if (mediaVal->strVal == "audio_muted") {
                    spec.mediaState = TabMediaState::AudioMuted;
                } else {
                    spec.mediaState = TabMediaState::None;
                }
            }
            const auto* discardVal = itemPtr->find("discard_state");
            if (discardVal && discardVal->type == JsonValueType::String &&
                discardVal->strVal == "discarded") {
                spec.discardState = TabDiscardState::Discarded;
            } else {
                spec.discardState = TabDiscardState::Active;
            }
            const auto* loadingVal = itemPtr->find("loading_state");
            if (loadingVal && loadingVal->type == JsonValueType::String) {
                if (loadingVal->strVal == "loading") {
                    spec.loadingState = TabLoadingState::Loading;
                } else if (loadingVal->strVal == "failed") {
                    spec.loadingState = TabLoadingState::Failed;
                } else {
                    spec.loadingState = TabLoadingState::Idle;
                }
            }
            outTabs.push_back(std::move(spec));
        }
    }

    return true;
}

} // namespace

TabStripDensityService::TabStripDensityService()
    : config_{.density = LayoutDensity::Default,
              .showNewTabButton = true,
              .overflowMode = TabStripOverflowMode::Scrollable,
              .scrollOffset = 0},
      availableWidth_{1000}, tabs_{},
      plan_{TabStripDensityValidator::calculateTabStripLayout(1000, config_, tabs_).plan}, options_{} {
}

TabStripDensityService::TabStripDensityService(TabStripDensityConfig initialConfig, int availableWidth,
                                               std::vector<TabSpec> initialTabs,
                                               TabStripDensityServiceOptions options)
    : config_{initialConfig}, availableWidth_{availableWidth}, tabs_{std::move(initialTabs)},
      options_{std::move(options)} {
    int minW = TabStripDensityConfiguration::kDefaultMinStripWidth;
    if (config_.density == LayoutDensity::Senior) {
        minW = TabStripDensityConfiguration::kSeniorMinStripWidth;
    } else if (config_.density == LayoutDensity::Compact) {
        minW = TabStripDensityConfiguration::kCompactMinStripWidth;
    }
    if (availableWidth_ < minW) {
        availableWidth_ = minW;
    }

    // Asegurar invariante de precedencia de fijadas
    std::stable_partition(tabs_.begin(), tabs_.end(),
                          [](const TabSpec& t) { return t.pinState == TabPinState::Pinned; });

    auto report = TabStripDensityValidator::calculateTabStripLayout(availableWidth_, config_, tabs_);
    if (report.valid) {
        plan_ = report.plan;
    } else {
        restoreDefaultLocked();
    }
}

TabStripDensityService::TabStripDensityService(LayoutDensity density, int availableWidth,
                                               TabStripDensityServiceOptions options)
    : availableWidth_{availableWidth}, options_{std::move(options)} {
    config_.density = density;
    config_.showNewTabButton = true;
    config_.overflowMode = TabStripOverflowMode::Scrollable;
    config_.scrollOffset = 0;

    int minW = TabStripDensityConfiguration::kDefaultMinStripWidth;
    if (density == LayoutDensity::Senior) {
        minW = TabStripDensityConfiguration::kSeniorMinStripWidth;
    } else if (density == LayoutDensity::Compact) {
        minW = TabStripDensityConfiguration::kCompactMinStripWidth;
    }
    if (availableWidth_ < minW) {
        availableWidth_ = minW;
    }

    if (density == LayoutDensity::Senior) {
        plan_ = TabStripDensityConfiguration::canonicalSeniorDensity(availableWidth_);
    } else if (density == LayoutDensity::Compact) {
        plan_ = TabStripDensityConfiguration::canonicalCompactDensity(availableWidth_);
    } else {
        plan_ = TabStripDensityConfiguration::canonicalDefaultDensity(availableWidth_);
    }

    tabs_.reserve(plan_.tabs.size());
    for (const auto& item : plan_.tabs) {
        TabSpec spec{};
        spec.id = item.id;
        spec.title = item.accessibleLabel;
        spec.pinState = item.isPinned ? TabPinState::Pinned : TabPinState::Unpinned;
        spec.isSelected = item.isSelected;
        tabs_.push_back(std::move(spec));
    }
}

TabStripDensityService::TabStripDensityService(TabStripDensityPlan initialPlan,
                                               TabStripDensityServiceOptions options)
    : config_{.density = initialPlan.density,
              .showNewTabButton = (initialPlan.newTabButtonRect.width > 0),
              .overflowMode = initialPlan.overflowMode,
              .scrollOffset = 0},
      availableWidth_{initialPlan.availableWidth}, plan_{std::move(initialPlan)},
      options_{std::move(options)} {
    tabs_.reserve(plan_.tabs.size());
    for (const auto& item : plan_.tabs) {
        TabSpec spec{};
        spec.id = item.id;
        spec.title = item.accessibleLabel;
        spec.pinState = item.isPinned ? TabPinState::Pinned : TabPinState::Unpinned;
        spec.isSelected = item.isSelected;
        tabs_.push_back(std::move(spec));
    }
}

TabStripDensityService::TabStripDensityService(TabStripDensityService&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = other.config_;
    availableWidth_ = other.availableWidth_;
    tabs_ = std::move(other.tabs_);
    plan_ = std::move(other.plan_);
    options_ = std::move(other.options_);
    callback_ = std::move(other.callback_);
}

TabStripDensityService& TabStripDensityService::operator=(TabStripDensityService&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        config_ = other.config_;
        availableWidth_ = other.availableWidth_;
        tabs_ = std::move(other.tabs_);
        plan_ = std::move(other.plan_);
        options_ = std::move(other.options_);
        callback_ = std::move(other.callback_);
    }
    return *this;
}

TabStripDensityConfig TabStripDensityService::config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

TabStripDensityPlan TabStripDensityService::plan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_;
}

int TabStripDensityService::availableWidth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return availableWidth_;
}

std::vector<TabSpec> TabStripDensityService::tabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tabs_;
}

std::size_t TabStripDensityService::tabCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tabs_.size();
}

std::optional<TabSpec> TabStripDensityService::tab(std::uint64_t tabId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& t : tabs_) {
        if (t.id == tabId) {
            return t;
        }
    }
    return std::nullopt;
}

std::optional<TabSpec> TabStripDensityService::tabAtIndex(std::size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < tabs_.size()) {
        return tabs_[index];
    }
    return std::nullopt;
}

std::optional<std::uint64_t> TabStripDensityService::selectedTabId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& t : tabs_) {
        if (t.isSelected) {
            return t.id;
        }
    }
    return std::nullopt;
}

bool TabStripDensityService::isOverflowing() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_.isOverflowing;
}

TabStripDensityServiceOptions TabStripDensityService::options() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

void TabStripDensityService::setOptions(const TabStripDensityServiceOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
}

TabStripDensityReport TabStripDensityService::setDensity(LayoutDensity density) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backupConfig = config_;
    const auto backupWidth = availableWidth_;
    const auto backupPlan = plan_;

    config_.density = density;
    int minW = TabStripDensityConfiguration::kDefaultMinStripWidth;
    if (density == LayoutDensity::Senior) {
        minW = TabStripDensityConfiguration::kSeniorMinStripWidth;
    } else if (density == LayoutDensity::Compact) {
        minW = TabStripDensityConfiguration::kCompactMinStripWidth;
    }
    if (availableWidth_ < minW) {
        availableWidth_ = minW;
    }

    auto report = recalculateLayoutLocked();
    if (!report.valid) {
        config_ = backupConfig;
        availableWidth_ = backupWidth;
        plan_ = backupPlan;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

TabStripDensityReport TabStripDensityService::setOverflowMode(TabStripOverflowMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.overflowMode = mode;
    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setScrollOffset(int offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.scrollOffset = offset;
    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setShowNewTabButton(bool show) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.showNewTabButton = show;
    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::addTab(const TabSpec& tab) {
    std::lock_guard<std::mutex> lock(mutex_);
    TabSpec newTab = tab;
    if (newTab.id == 0) {
        std::uint64_t maxId = 0;
        for (const auto& t : tabs_) {
            if (t.id > maxId) {
                maxId = t.id;
            }
        }
        newTab.id = maxId + 1;
    }

    newTab.title = TabStripDensityValidator::sanitizeTitle(newTab.title, options_.isIncognitoSession);
    newTab.url = sanitizeUrl(newTab.url);

    if (newTab.isSelected) {
        for (auto& t : tabs_) {
            t.isSelected = false;
        }
    } else if (tabs_.empty()) {
        newTab.isSelected = true;
    }

    // Invariante de precedencia de fijadas
    std::size_t pinnedCount = 0;
    for (const auto& t : tabs_) {
        if (t.pinState == TabPinState::Pinned) {
            ++pinnedCount;
        }
    }

    if (newTab.pinState == TabPinState::Pinned) {
        tabs_.insert(tabs_.begin() + static_cast<std::ptrdiff_t>(pinnedCount), newTab);
    } else {
        tabs_.push_back(newTab);
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::insertTab(std::size_t index, const TabSpec& tab) {
    std::lock_guard<std::mutex> lock(mutex_);
    TabSpec newTab = tab;
    if (newTab.id == 0) {
        std::uint64_t maxId = 0;
        for (const auto& t : tabs_) {
            if (t.id > maxId) {
                maxId = t.id;
            }
        }
        newTab.id = maxId + 1;
    }

    newTab.title = TabStripDensityValidator::sanitizeTitle(newTab.title, options_.isIncognitoSession);
    newTab.url = sanitizeUrl(newTab.url);

    if (newTab.isSelected) {
        for (auto& t : tabs_) {
            t.isSelected = false;
        }
    } else if (tabs_.empty()) {
        newTab.isSelected = true;
    }

    // Contar fijadas existentes
    std::size_t pinnedCount = 0;
    for (const auto& t : tabs_) {
        if (t.pinState == TabPinState::Pinned) {
            ++pinnedCount;
        }
    }

    if (newTab.pinState == TabPinState::Pinned) {
        if (index > pinnedCount) {
            index = pinnedCount;
        }
    } else {
        if (index < pinnedCount) {
            index = pinnedCount;
        }
    }

    if (index > tabs_.size()) {
        index = tabs_.size();
    }

    tabs_.insert(tabs_.begin() + static_cast<std::ptrdiff_t>(index), newTab);

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::removeTab(std::uint64_t tabId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t foundIdx = tabs_.size();
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].id == tabId) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx == tabs_.size()) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para eliminación.";
        failReport.remediationHint = "Verifique el identificador de la pestaña solicitada.";
        return failReport;
    }

    const bool wasSelected = tabs_[foundIdx].isSelected;
    tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(foundIdx));

    // Reasignación inteligente de selección a la pestaña adyacente más próxima
    if (wasSelected && !tabs_.empty()) {
        if (foundIdx < tabs_.size()) {
            tabs_[foundIdx].isSelected = true;
        } else {
            tabs_.back().isSelected = true;
        }
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::moveTab(std::uint64_t tabId, std::size_t newIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t oldIndex = tabs_.size();
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].id == tabId) {
            oldIndex = i;
            break;
        }
    }

    if (oldIndex == tabs_.size()) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para reordenar.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    std::size_t pinnedCount = 0;
    for (const auto& t : tabs_) {
        if (t.pinState == TabPinState::Pinned) {
            ++pinnedCount;
        }
    }

    const bool isPinned = (tabs_[oldIndex].pinState == TabPinState::Pinned);
    if (isPinned) {
        if (newIndex >= pinnedCount) {
            newIndex = pinnedCount > 0 ? (pinnedCount - 1) : 0;
        }
    } else {
        if (newIndex < pinnedCount) {
            newIndex = pinnedCount;
        }
        if (newIndex >= tabs_.size()) {
            newIndex = tabs_.size() - 1;
        }
    }

    if (oldIndex != newIndex) {
        TabSpec moved = std::move(tabs_[oldIndex]);
        tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(oldIndex));
        tabs_.insert(tabs_.begin() + static_cast<std::ptrdiff_t>(newIndex), std::move(moved));
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::selectTab(std::uint64_t tabId) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : tabs_) {
        if (t.id == tabId) {
            t.isSelected = true;
            found = true;
        } else {
            t.isSelected = false;
        }
    }

    if (!found) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para selección.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setTabPinned(std::uint64_t tabId, bool pinned) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t foundIdx = tabs_.size();
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].id == tabId) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx == tabs_.size()) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para alternar fijación.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    const bool currentlyPinned = (tabs_[foundIdx].pinState == TabPinState::Pinned);
    if (currentlyPinned != pinned) {
        TabSpec tab = std::move(tabs_[foundIdx]);
        tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(foundIdx));
        tab.pinState = pinned ? TabPinState::Pinned : TabPinState::Unpinned;

        std::size_t pinnedCount = 0;
        for (const auto& t : tabs_) {
            if (t.pinState == TabPinState::Pinned) {
                ++pinnedCount;
            }
        }
        tabs_.insert(tabs_.begin() + static_cast<std::ptrdiff_t>(pinnedCount), std::move(tab));
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setTabMediaState(std::uint64_t tabId,
                                                               TabMediaState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : tabs_) {
        if (t.id == tabId) {
            t.mediaState = state;
            found = true;
            break;
        }
    }

    if (!found) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para actualizar estado de sonido.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::toggleTabAudioMute(std::uint64_t tabId) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : tabs_) {
        if (t.id == tabId) {
            if (t.mediaState == TabMediaState::AudioPlaying) {
                t.mediaState = TabMediaState::AudioMuted;
            } else if (t.mediaState == TabMediaState::AudioMuted) {
                t.mediaState = TabMediaState::AudioPlaying;
            } else {
                t.mediaState = TabMediaState::AudioMuted;
            }
            found = true;
            break;
        }
    }

    if (!found) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para alternar silencio.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setTabDiscardState(std::uint64_t tabId,
                                                                 TabDiscardState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : tabs_) {
        if (t.id == tabId) {
            t.discardState = state;
            found = true;
            break;
        }
    }

    if (!found) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para actualizar reposo.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::updateTabTitle(std::uint64_t tabId, std::string_view title,
                                                             std::string_view url) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : tabs_) {
        if (t.id == tabId) {
            t.title = TabStripDensityValidator::sanitizeTitle(title, options_.isIncognitoSession);
            if (!url.empty()) {
                t.url = sanitizeUrl(url);
            }
            found = true;
            break;
        }
    }

    if (!found) {
        TabStripDensityReport failReport{};
        failReport.valid = false;
        failReport.error = TabStripDensityError::None;
        failReport.plan = plan_;
        failReport.message = "Pestaña no encontrada para actualizar título.";
        failReport.remediationHint = "Verifique el identificador de la pestaña.";
        return failReport;
    }

    auto report = recalculateLayoutLocked();
    if (report.valid) {
        notifyChangedLocked(report);
    }
    return report;
}

TabStripDensityReport TabStripDensityService::setAvailableWidth(int availableWidth) {
    std::lock_guard<std::mutex> lock(mutex_);

    int minW = TabStripDensityConfiguration::kDefaultMinStripWidth;
    if (config_.density == LayoutDensity::Senior) {
        minW = TabStripDensityConfiguration::kSeniorMinStripWidth;
    } else if (config_.density == LayoutDensity::Compact) {
        minW = TabStripDensityConfiguration::kCompactMinStripWidth;
    }

    if (availableWidth < minW) {
        TabStripDensityReport report{};
        report.valid = false;
        report.error = TabStripDensityError::WidthBelowMinimum;
        report.plan = plan_;
        report.message =
            TabStripDensityValidator::tabStripErrorToString(TabStripDensityError::WidthBelowMinimum);
        report.remediationHint =
            TabStripDensityValidator::generateRemediationHint(TabStripDensityError::WidthBelowMinimum);
        return report;
    }

    const auto backupWidth = availableWidth_;
    const auto backupPlan = plan_;

    availableWidth_ = availableWidth;
    auto report = recalculateLayoutLocked();
    if (!report.valid) {
        availableWidth_ = backupWidth;
        plan_ = backupPlan;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

TabStripHitTestZone TabStripDensityService::hitTest(int x, int y) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return TabStripDensityValidator::hitTest(x, y, plan_);
}

std::optional<std::uint64_t> TabStripDensityService::tabAtPoint(int x, int y) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& tab : plan_.tabs) {
        if (tab.rect.contains(x, y)) {
            return tab.id;
        }
    }
    return std::nullopt;
}

TabStripDensityReport TabStripDensityService::computeLayout(CancellationToken* cancel) const {
    if (cancel != nullptr && cancel->isCancelled()) {
        TabStripDensityReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = TabStripDensityError::None;
        cancelledReport.message = "Cálculo de tira de pestañas cancelado.";
        cancelledReport.remediationHint =
            "Reintente la operación cuando no haya solicitudes de cancelación activas.";
        return cancelledReport;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (cancel != nullptr && cancel->isCancelled()) {
        TabStripDensityReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = TabStripDensityError::None;
        cancelledReport.message = "Cálculo de tira de pestañas cancelado.";
        cancelledReport.remediationHint =
            "Reintente la operación cuando no haya solicitudes de cancelación activas.";
        return cancelledReport;
    }

    return TabStripDensityValidator::calculateTabStripLayout(availableWidth_, config_, tabs_);
}

bool TabStripDensityService::saveStateToFile(const std::filesystem::path& path,
                                             CancellationToken* cancel) {
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

            const std::string content = serializeTabStripStateToJson(config_, availableWidth_, tabs_);
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

bool TabStripDensityService::loadStateFromFile(const std::filesystem::path& path,
                                               CancellationToken* cancel) {
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

    TabStripDensityConfig loadedConfig{};
    int loadedWidth = 1000;
    std::vector<TabSpec> loadedTabs;
    if (!deserializeTabStripStateFromJson(text, loadedConfig, loadedWidth, loadedTabs)) {
        restoreDefaultLocked();
        return false;
    }

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    auto report =
        TabStripDensityValidator::calculateTabStripLayout(loadedWidth, loadedConfig, loadedTabs);
    if (!report.valid) {
        restoreDefaultLocked();
        return false;
    }

    config_ = loadedConfig;
    availableWidth_ = loadedWidth;
    tabs_ = std::move(loadedTabs);
    plan_ = report.plan;
    notifyChangedLocked(report);
    return true;
}

bool TabStripDensityService::saveCurrentState(CancellationToken* cancel) {
    if (options_.stateFilePath.empty()) {
        return false;
    }
    return saveStateToFile(options_.stateFilePath, cancel);
}

bool TabStripDensityService::restoreDefault() {
    std::lock_guard<std::mutex> lock(mutex_);
    restoreDefaultLocked();
    return true;
}

void TabStripDensityService::restoreDefaultLocked() {
    config_ = TabStripDensityConfig{};
    availableWidth_ = 1000;
    tabs_.clear();
    plan_ = TabStripDensityConfiguration::canonicalDefaultDensity(1000);
    TabStripDensityReport report{};
    report.valid = true;
    report.error = TabStripDensityError::None;
    report.plan = plan_;
    report.message = "Configuración canónica por defecto restaurada.";
    report.remediationHint =
        TabStripDensityValidator::generateRemediationHint(TabStripDensityError::None);
    notifyChangedLocked(report);
}

void TabStripDensityService::setIncognitoMode(bool incognito) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    options_.isIncognitoSession = incognito;
}

bool TabStripDensityService::isIncognitoMode() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_.isIncognitoSession;
}

void TabStripDensityService::setTabStripChangedCallback(OnTabStripChangedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void TabStripDensityService::clearTabStripChangedCallback() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = nullptr;
}

void TabStripDensityService::notifyChangedLocked(const TabStripDensityReport& report) {
    if (callback_) {
        callback_(plan_, report);
    }
}

TabStripDensityReport TabStripDensityService::recalculateLayoutLocked() {
    auto report = TabStripDensityValidator::calculateTabStripLayout(availableWidth_, config_, tabs_);
    if (report.valid) {
        plan_ = report.plan;
    }
    return report;
}

std::string TabStripDensityService::formatTabStripSummary(const TabStripDensityPlan& plan,
                                                          const TabStripDensityReport& report) {
    std::string summary;
    summary.reserve(512);
    summary += "Tira de pestañas | Densidad: ";
    if (plan.density == LayoutDensity::Senior) {
        summary += "Interfaz más fácil de usar";
    } else if (plan.density == LayoutDensity::Compact) {
        summary += "Compacta";
    } else {
        summary += "Estándar";
    }
    summary += " | Dimensiones: ";
    summary += std::to_string(plan.availableWidth);
    summary += "x";
    summary += std::to_string(plan.height);
    summary += " px | Pestañas: ";
    summary += std::to_string(plan.tabs.size());
    summary += " (";
    summary += std::to_string(plan.pinnedTabsCount);
    summary += " fijadas, ";
    summary += std::to_string(plan.regularTabsCount);
    summary += " regulares)";
    summary += " | Ancho efectivo: ";
    summary += std::to_string(plan.effectiveTabWidth);
    summary += " px | Desbordamiento: ";
    summary += plan.isOverflowing ? "Sí (desplazable)" : "No";
    summary += " | Estado: ";
    summary += report.valid ? "Válida" : "Inválida";
    if (!report.valid && !report.message.empty()) {
        summary += " - ";
        summary += report.message;
    }
    return summary;
}

} // namespace vigilio
