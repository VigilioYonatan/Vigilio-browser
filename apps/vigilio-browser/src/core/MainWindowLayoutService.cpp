#include "core/MainWindowLayoutService.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

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
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '"') {
            return parseString(out.strVal) && ((out.type = JsonValueType::String), true);
        }
        if (c == 't' || c == 'f') {
            return parseBool(out);
        }
        if (c == 'n') {
            return parseNull(out);
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(out);
        }
        return false;
    }

    bool parseObject(SimpleJsonValue& out) {
        out.type = JsonValueType::Object;
        out.objVal.clear();
        ++m_pos; // omitir '{'
        skipWhitespace();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            return true;
        }

        while (m_pos < m_text.size()) {
            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
                return false;
            }
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') {
                return false;
            }
            ++m_pos; // omitir ':'
            SimpleJsonValue val;
            if (!parseValue(val)) {
                return false;
            }
            out.objVal.push_back({std::move(key), std::make_shared<SimpleJsonValue>(std::move(val))});
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                ++m_pos;
                skipWhitespace();
                if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                    return false;
                }
            } else if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                ++m_pos;
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool parseString(std::string& out) {
        out.clear();
        if (m_pos >= m_text.size() || m_text[m_pos] != '"') {
            return false;
        }
        ++m_pos; // omitir comilla de apertura '"'
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos++];
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (m_pos >= m_text.size()) {
                    return false;
                }
                const char esc = m_text[m_pos++];
                switch (esc) {
                case '"':
                    out += '"';
                    break;
                case '\\':
                    out += '\\';
                    break;
                case '/':
                    out += '/';
                    break;
                case 'b':
                    out += '\b';
                    break;
                case 'f':
                    out += '\f';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                default:
                    out += esc;
                    break;
                }
            } else {
                out += c;
            }
        }
        return false;
    }

    bool parseBool(SimpleJsonValue& out) {
        if (m_pos + 4 <= m_text.size() && m_text.substr(m_pos, 4) == "true") {
            out.type = JsonValueType::Boolean;
            out.boolVal = true;
            m_pos += 4;
            return true;
        }
        if (m_pos + 5 <= m_text.size() && m_text.substr(m_pos, 5) == "false") {
            out.type = JsonValueType::Boolean;
            out.boolVal = false;
            m_pos += 5;
            return true;
        }
        return false;
    }

    bool parseNull(SimpleJsonValue& out) {
        if (m_pos + 4 <= m_text.size() && m_text.substr(m_pos, 4) == "null") {
            out.type = JsonValueType::Null;
            m_pos += 4;
            return true;
        }
        return false;
    }

    bool parseNumber(SimpleJsonValue& out) {
        const std::size_t start = m_pos;
        if (m_pos < m_text.size() && m_text[m_pos] == '-') {
            ++m_pos;
        }
        while (m_pos < m_text.size() &&
               ((m_text[m_pos] >= '0' && m_text[m_pos] <= '9') || m_text[m_pos] == '.' ||
                m_text[m_pos] == 'e' || m_text[m_pos] == 'E' || m_text[m_pos] == '+' ||
                m_text[m_pos] == '-')) {
            ++m_pos;
        }
        const std::string numStr(m_text.substr(start, m_pos - start));
        try {
            out.type = JsonValueType::Number;
            out.numVal = std::stod(numStr);
            return true;
        } catch (...) {
            return false;
        }
    }
};

std::string serializePlanToJson(const MainWindowLayoutPlan& plan) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": 1,\n";
    ss << "  \"windowWidth\": " << plan.window.width << ",\n";
    ss << "  \"windowHeight\": " << plan.window.height << ",\n";
    ss << "  \"windowMinWidth\": " << plan.window.minWidth << ",\n";
    ss << "  \"windowMinHeight\": " << plan.window.minHeight << ",\n";
    ss << "  \"density\": \""
       << (plan.density == LayoutDensity::Senior
               ? "Senior"
               : (plan.density == LayoutDensity::Compact ? "Compact" : "Default"))
       << "\",\n";
    ss << "  \"chromeMode\": \""
       << (plan.chromeMode == WindowChromeMode::Integrated
               ? "Integrated"
               : (plan.chromeMode == WindowChromeMode::Fullscreen ? "Fullscreen" : "Standard"))
       << "\",\n";
    ss << "  \"sidebarState\": \""
       << (plan.sidebar.state == SidebarState::Expanded
               ? "Expanded"
               : (plan.sidebar.state == SidebarState::Hidden ? "Hidden" : "Collapsed"))
       << "\",\n";
    ss << "  \"sidebarDockPosition\": \""
       << (plan.sidebar.position == SidebarDockPosition::Right ? "Right" : "Left") << "\",\n";
    ss << "  \"sidebarWidth\": " << plan.sidebar.width << ",\n";
    ss << "  \"sidebarCollapsedWidth\": " << plan.sidebar.collapsedWidth << ",\n";
    ss << "  \"sidebarDefaultExpandedWidth\": " << plan.sidebar.defaultExpandedWidth << ",\n";
    ss << "  \"sidebarMinExpandedWidth\": " << plan.sidebar.minExpandedWidth << ",\n";
    ss << "  \"sidebarMaxExpandedWidth\": " << plan.sidebar.maxExpandedWidth << ",\n";
    ss << "  \"showTitleBar\": " << (plan.showTitleBar ? "true" : "false") << ",\n";
    ss << "  \"showTabStrip\": " << (plan.showTabStrip ? "true" : "false") << ",\n";
    ss << "  \"showToolBar\": " << (plan.showToolBar ? "true" : "false") << ",\n";
    ss << "  \"showBanner\": " << (plan.showBanner ? "true" : "false") << ",\n";
    ss << "  \"showSidebar\": " << (plan.showSidebar ? "true" : "false") << ",\n";
    ss << "  \"showStatusBar\": " << (plan.showStatusBar ? "true" : "false") << "\n";
    ss << "}\n";
    return ss.str();
}

bool deserializePlanFromJson(std::string_view text, MainWindowLayoutPlan& outPlan) {
    SimpleJsonValue root;
    SimpleJsonParser parser(text);
    if (!parser.parse(root) || root.type != JsonValueType::Object) {
        return false;
    }

    const auto* wVal = root.find("windowWidth");
    const auto* hVal = root.find("windowHeight");
    const auto* minWVal = root.find("windowMinWidth");
    const auto* minHVal = root.find("windowMinHeight");
    const auto* densityVal = root.find("density");
    const auto* chromeVal = root.find("chromeMode");
    const auto* sbStateVal = root.find("sidebarState");
    const auto* sbDockVal = root.find("sidebarDockPosition");
    const auto* sbWidthVal = root.find("sidebarWidth");
    const auto* sbCollapsedWVal = root.find("sidebarCollapsedWidth");
    const auto* sbDefaultWVal = root.find("sidebarDefaultExpandedWidth");
    const auto* sbMinWVal = root.find("sidebarMinExpandedWidth");
    const auto* sbMaxWVal = root.find("sidebarMaxExpandedWidth");
    const auto* titleBarVal = root.find("showTitleBar");
    const auto* tabStripVal = root.find("showTabStrip");
    const auto* toolBarVal = root.find("showToolBar");
    const auto* bannerVal = root.find("showBanner");
    const auto* sidebarVal = root.find("showSidebar");
    const auto* statusBarVal = root.find("showStatusBar");

    if (!wVal || !hVal || !minWVal || !minHVal || !densityVal || !sbStateVal || !sbDockVal ||
        !sbWidthVal || !sbCollapsedWVal || !sbDefaultWVal || !sbMinWVal || !sbMaxWVal || !titleBarVal ||
        !tabStripVal || !toolBarVal || !bannerVal || !sidebarVal || !statusBarVal) {
        return false;
    }

    if (wVal->type != JsonValueType::Number || hVal->type != JsonValueType::Number ||
        minWVal->type != JsonValueType::Number || minHVal->type != JsonValueType::Number ||
        densityVal->type != JsonValueType::String || sbStateVal->type != JsonValueType::String ||
        sbDockVal->type != JsonValueType::String || sbWidthVal->type != JsonValueType::Number ||
        sbCollapsedWVal->type != JsonValueType::Number || sbDefaultWVal->type != JsonValueType::Number ||
        sbMinWVal->type != JsonValueType::Number || sbMaxWVal->type != JsonValueType::Number ||
        titleBarVal->type != JsonValueType::Boolean || tabStripVal->type != JsonValueType::Boolean ||
        toolBarVal->type != JsonValueType::Boolean || bannerVal->type != JsonValueType::Boolean ||
        sidebarVal->type != JsonValueType::Boolean || statusBarVal->type != JsonValueType::Boolean) {
        return false;
    }

    LayoutDensity density = LayoutDensity::Default;
    if (densityVal->strVal == "Senior") {
        density = LayoutDensity::Senior;
    } else if (densityVal->strVal == "Compact") {
        density = LayoutDensity::Compact;
    } else if (densityVal->strVal != "Default") {
        return false;
    }

    WindowChromeMode chrome = WindowChromeMode::Standard;
    if (chromeVal && chromeVal->type == JsonValueType::String) {
        if (chromeVal->strVal == "Integrated") {
            chrome = WindowChromeMode::Integrated;
        } else if (chromeVal->strVal == "Fullscreen") {
            chrome = WindowChromeMode::Fullscreen;
        }
    }

    SidebarState sbState = SidebarState::Collapsed;
    if (sbStateVal->strVal == "Expanded") {
        sbState = SidebarState::Expanded;
    } else if (sbStateVal->strVal == "Hidden") {
        sbState = SidebarState::Hidden;
    } else if (sbStateVal->strVal != "Collapsed") {
        return false;
    }

    SidebarDockPosition sbDock = SidebarDockPosition::Left;
    if (sbDockVal->strVal == "Right") {
        sbDock = SidebarDockPosition::Right;
    } else if (sbDockVal->strVal != "Left") {
        return false;
    }

    MainWindowLayoutPlan plan =
        (density == LayoutDensity::Senior)    ? MainWindowLayoutConfiguration::canonicalSeniorLayout()
        : (density == LayoutDensity::Compact) ? MainWindowLayoutConfiguration::canonicalCompactLayout()
                                              : MainWindowLayoutConfiguration::canonicalDefaultLayout();

    plan.window.width = static_cast<int>(wVal->numVal);
    plan.window.height = static_cast<int>(hVal->numVal);
    plan.window.minWidth = static_cast<int>(minWVal->numVal);
    plan.window.minHeight = static_cast<int>(minHVal->numVal);
    plan.density = density;
    plan.chromeMode = chrome;

    plan.sidebar.state = sbState;
    plan.sidebar.position = sbDock;
    plan.sidebar.width = static_cast<int>(sbWidthVal->numVal);
    plan.sidebar.collapsedWidth = static_cast<int>(sbCollapsedWVal->numVal);
    plan.sidebar.defaultExpandedWidth = static_cast<int>(sbDefaultWVal->numVal);
    plan.sidebar.minExpandedWidth = static_cast<int>(sbMinWVal->numVal);
    plan.sidebar.maxExpandedWidth = static_cast<int>(sbMaxWVal->numVal);

    plan.showTitleBar = titleBarVal->boolVal;
    plan.showTabStrip = tabStripVal->boolVal;
    plan.showToolBar = toolBarVal->boolVal;
    plan.showBanner = bannerVal->boolVal;
    plan.showSidebar = sidebarVal->boolVal;
    plan.showStatusBar = statusBarVal->boolVal;

    for (auto& zb : plan.zones) {
        switch (zb.zone) {
        case LayoutZone::TitleBar:
            zb.visible = plan.showTitleBar;
            break;
        case LayoutZone::TabStrip:
            zb.visible = plan.showTabStrip;
            break;
        case LayoutZone::ToolBar:
            zb.visible = plan.showToolBar;
            break;
        case LayoutZone::NotificationBanner:
            zb.visible = plan.showBanner;
            break;
        case LayoutZone::Sidebar:
            zb.visible = plan.showSidebar;
            break;
        case LayoutZone::StatusBar:
            zb.visible = plan.showStatusBar;
            break;
        case LayoutZone::Content:
            zb.visible = true;
            break;
        }
    }

    outPlan = std::move(plan);
    return true;
}

} // namespace

MainWindowLayoutService::MainWindowLayoutService()
    : plan_(MainWindowLayoutConfiguration::canonicalDefaultLayout()), options_({}) {
}

MainWindowLayoutService::MainWindowLayoutService(MainWindowLayoutPlan initialPlan,
                                                 MainWindowLayoutServiceOptions options)
    : plan_(std::move(initialPlan)), options_(std::move(options)) {
}

MainWindowLayoutService::MainWindowLayoutService(MainWindowLayoutService&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    plan_ = std::move(other.plan_);
    options_ = std::move(other.options_);
    callback_ = std::move(other.callback_);
}

MainWindowLayoutService& MainWindowLayoutService::operator=(MainWindowLayoutService&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(mutex_, other.mutex_);
        plan_ = std::move(other.plan_);
        options_ = std::move(other.options_);
        callback_ = std::move(other.callback_);
    }
    return *this;
}

MainWindowLayoutPlan MainWindowLayoutService::plan() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plan_;
}

MainWindowLayoutServiceOptions MainWindowLayoutService::options() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

void MainWindowLayoutService::setOptions(const MainWindowLayoutServiceOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    options_ = options;
}

MainWindowLayoutReport MainWindowLayoutService::setDensity(LayoutDensity density) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    plan_.density = density;

    switch (density) {
    case LayoutDensity::Senior:
        plan_.window.minWidth = MainWindowLayoutConfiguration::kMinWindowWidthSenior;
        plan_.window.minHeight = MainWindowLayoutConfiguration::kMinWindowHeightSenior;
        break;
    case LayoutDensity::Compact:
        plan_.window.minWidth = MainWindowLayoutConfiguration::kMinWindowWidthCompact;
        plan_.window.minHeight = MainWindowLayoutConfiguration::kMinWindowHeightCompact;
        break;
    case LayoutDensity::Default:
        plan_.window.minWidth = MainWindowLayoutConfiguration::kMinWindowWidthDefault;
        plan_.window.minHeight = MainWindowLayoutConfiguration::kMinWindowHeightDefault;
        break;
    }

    if (plan_.window.width < plan_.window.minWidth) {
        plan_.window.width = plan_.window.minWidth;
    }
    if (plan_.window.height < plan_.window.minHeight) {
        plan_.window.height = plan_.window.minHeight;
    }

    if (plan_.zones.empty()) {
        plan_.zones = (density == LayoutDensity::Senior)
                          ? MainWindowLayoutConfiguration::canonicalSeniorLayout().zones
                      : (density == LayoutDensity::Compact)
                          ? MainWindowLayoutConfiguration::canonicalCompactLayout().zones
                          : MainWindowLayoutConfiguration::canonicalDefaultLayout().zones;
    } else {
        for (auto& zb : plan_.zones) {
            switch (zb.zone) {
            case LayoutZone::TabStrip: {
                const int h = (density == LayoutDensity::Senior)
                                  ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightSenior
                              : (density == LayoutDensity::Compact)
                                  ? MainWindowLayoutConfiguration::kCanonicalTabStripHeightCompact
                                  : MainWindowLayoutConfiguration::kCanonicalTabStripHeightDefault;
                zb.preferredHeight = h;
                zb.minHeight = h;
                zb.maxHeight = h;
                break;
            }
            case LayoutZone::ToolBar: {
                const int h = (density == LayoutDensity::Senior)
                                  ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightSenior
                              : (density == LayoutDensity::Compact)
                                  ? MainWindowLayoutConfiguration::kCanonicalToolBarHeightCompact
                                  : MainWindowLayoutConfiguration::kCanonicalToolBarHeightDefault;
                zb.preferredHeight = h;
                zb.minHeight = h;
                zb.maxHeight = h;
                break;
            }
            case LayoutZone::NotificationBanner: {
                const int h = (density == LayoutDensity::Senior)
                                  ? MainWindowLayoutConfiguration::kCanonicalBannerHeightSenior
                              : (density == LayoutDensity::Compact)
                                  ? MainWindowLayoutConfiguration::kCanonicalBannerHeightCompact
                                  : MainWindowLayoutConfiguration::kCanonicalBannerHeightDefault;
                zb.preferredHeight = h;
                zb.minHeight = h;
                zb.maxHeight = h;
                break;
            }
            case LayoutZone::StatusBar: {
                const int h = (density == LayoutDensity::Senior)
                                  ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightSenior
                              : (density == LayoutDensity::Compact)
                                  ? MainWindowLayoutConfiguration::kCanonicalStatusBarHeightCompact
                                  : MainWindowLayoutConfiguration::kCanonicalStatusBarHeightDefault;
                zb.preferredHeight = h;
                zb.minHeight = h;
                zb.maxHeight = h;
                break;
            }
            default:
                break;
            }
        }
    }

    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::resizeWindow(int width, int height) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    plan_.window.width = width;
    plan_.window.height = height;

    auto report = MainWindowLayoutValidator::calculateLayoutAreas(width, height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::setSidebarState(SidebarState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    plan_.sidebar.state = state;
    if (state == SidebarState::Collapsed) {
        plan_.sidebar.width = plan_.sidebar.collapsedWidth;
    } else if (state == SidebarState::Expanded) {
        if (plan_.sidebar.width < plan_.sidebar.minExpandedWidth ||
            plan_.sidebar.width > plan_.sidebar.maxExpandedWidth) {
            plan_.sidebar.width = plan_.sidebar.defaultExpandedWidth;
        }
    }

    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::toggleSidebar() {
    SidebarState nextState = SidebarState::Expanded;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (plan_.sidebar.state == SidebarState::Expanded) {
            nextState = SidebarState::Collapsed;
        } else {
            nextState = SidebarState::Expanded;
        }
    }
    return setSidebarState(nextState);
}

MainWindowLayoutReport MainWindowLayoutService::setSidebarWidth(int width) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    if (width == plan_.sidebar.collapsedWidth) {
        plan_.sidebar.state = SidebarState::Collapsed;
        plan_.sidebar.width = width;
    } else if (width >= plan_.sidebar.minExpandedWidth && width <= plan_.sidebar.maxExpandedWidth) {
        plan_.sidebar.state = SidebarState::Expanded;
        plan_.sidebar.width = width;
    } else {
        MainWindowLayoutReport errReport{};
        errReport.valid = false;
        errReport.error = MainWindowLayoutError::InvalidSidebarWidth;
        errReport.message =
            MainWindowLayoutValidator::layoutErrorToString(MainWindowLayoutError::InvalidSidebarWidth);
        errReport.remediationHint = MainWindowLayoutValidator::generateRemediationHint(
            MainWindowLayoutError::InvalidSidebarWidth,
            "El ancho debe estar en el rango [200, 360] px o ser exactamente 48 px si está colapsada.");
        return errReport;
    }

    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::setSidebarDockPosition(SidebarDockPosition position) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    plan_.sidebar.position = position;

    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::setZoneVisible(LayoutZone zone, bool visible) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto backup = plan_;

    switch (zone) {
    case LayoutZone::TitleBar:
        plan_.showTitleBar = visible;
        break;
    case LayoutZone::TabStrip:
        plan_.showTabStrip = visible;
        break;
    case LayoutZone::ToolBar:
        plan_.showToolBar = visible;
        break;
    case LayoutZone::NotificationBanner:
        plan_.showBanner = visible;
        break;
    case LayoutZone::Sidebar:
        plan_.showSidebar = visible;
        break;
    case LayoutZone::StatusBar:
        plan_.showStatusBar = visible;
        break;
    case LayoutZone::Content:
        break;
    }

    for (auto& zb : plan_.zones) {
        if (zb.zone == zone) {
            zb.visible = visible;
        }
    }

    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    if (!report.valid) {
        plan_ = backup;
        return report;
    }

    notifyChangedLocked(report);
    return report;
}

MainWindowLayoutReport MainWindowLayoutService::showNotificationBanner(bool show) {
    return setZoneVisible(LayoutZone::NotificationBanner, show);
}

MainWindowLayoutReport MainWindowLayoutService::showStatusBar(bool show) {
    return setZoneVisible(LayoutZone::StatusBar, show);
}

MainWindowLayoutReport MainWindowLayoutService::computeLayout(CancellationToken* cancel) const {
    if (cancel != nullptr && cancel->isCancelled()) {
        MainWindowLayoutReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = MainWindowLayoutError::ZeroContentAreaBudget;
        cancelledReport.message = "Operación de cálculo cancelada.";
        cancelledReport.remediationHint = "Operación cancelada de forma cooperativa.";
        return cancelledReport;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (cancel != nullptr && cancel->isCancelled()) {
        MainWindowLayoutReport cancelledReport{};
        cancelledReport.valid = false;
        cancelledReport.error = MainWindowLayoutError::ZeroContentAreaBudget;
        cancelledReport.message = "Operación de cálculo cancelada.";
        cancelledReport.remediationHint = "Operación cancelada de forma cooperativa.";
        return cancelledReport;
    }

    return MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height,
                                                           plan_);
}

bool MainWindowLayoutService::saveLayoutStateToFile(const std::filesystem::path& path,
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

            const std::string content = serializePlanToJson(plan_);
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

bool MainWindowLayoutService::loadLayoutStateFromFile(const std::filesystem::path& path,
                                                      CancellationToken* cancel) {
    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        plan_ = MainWindowLayoutConfiguration::canonicalDefaultLayout();
        MainWindowLayoutReport corruptReport{};
        corruptReport.valid = false;
        corruptReport.error = MainWindowLayoutError::LayoutPersistenceCorrupted;
        corruptReport.message = MainWindowLayoutValidator::layoutErrorToString(
            MainWindowLayoutError::LayoutPersistenceCorrupted);
        corruptReport.remediationHint = MainWindowLayoutValidator::generateRemediationHint(
            MainWindowLayoutError::LayoutPersistenceCorrupted,
            "El archivo de estado no existe o no es accesible.");
        notifyChangedLocked(corruptReport);
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        plan_ = MainWindowLayoutConfiguration::canonicalDefaultLayout();
        MainWindowLayoutReport corruptReport{};
        corruptReport.valid = false;
        corruptReport.error = MainWindowLayoutError::LayoutPersistenceCorrupted;
        corruptReport.message = MainWindowLayoutValidator::layoutErrorToString(
            MainWindowLayoutError::LayoutPersistenceCorrupted);
        corruptReport.remediationHint = MainWindowLayoutValidator::generateRemediationHint(
            MainWindowLayoutError::LayoutPersistenceCorrupted);
        notifyChangedLocked(corruptReport);
        return false;
    }

    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (cancel != nullptr && cancel->isCancelled()) {
        return false;
    }

    MainWindowLayoutPlan loadedPlan;
    if (!deserializePlanFromJson(text, loadedPlan)) {
        plan_ = MainWindowLayoutConfiguration::canonicalDefaultLayout();
        MainWindowLayoutReport corruptReport{};
        corruptReport.valid = false;
        corruptReport.error = MainWindowLayoutError::LayoutPersistenceCorrupted;
        corruptReport.message = MainWindowLayoutValidator::layoutErrorToString(
            MainWindowLayoutError::LayoutPersistenceCorrupted);
        corruptReport.remediationHint = MainWindowLayoutValidator::generateRemediationHint(
            MainWindowLayoutError::LayoutPersistenceCorrupted,
            "Formato JSON o valores truncados en el archivo.");
        notifyChangedLocked(corruptReport);
        return false;
    }

    if (MainWindowLayoutValidator::validatePlan(loadedPlan) != MainWindowLayoutError::None) {
        plan_ = MainWindowLayoutConfiguration::canonicalDefaultLayout();
        MainWindowLayoutReport corruptReport{};
        corruptReport.valid = false;
        corruptReport.error = MainWindowLayoutError::LayoutPersistenceCorrupted;
        corruptReport.message = MainWindowLayoutValidator::layoutErrorToString(
            MainWindowLayoutError::LayoutPersistenceCorrupted);
        corruptReport.remediationHint = MainWindowLayoutValidator::generateRemediationHint(
            MainWindowLayoutError::LayoutPersistenceCorrupted,
            "Los parámetros en disco violan las invariantes.");
        notifyChangedLocked(corruptReport);
        return false;
    }

    plan_ = loadedPlan;
    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    notifyChangedLocked(report);
    return true;
}

bool MainWindowLayoutService::saveCurrentState(CancellationToken* cancel) {
    std::filesystem::path path;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (options_.isIncognitoSession || options_.stateFilePath.empty()) {
            return false;
        }
        path = options_.stateFilePath;
    }
    return saveLayoutStateToFile(path, cancel);
}

bool MainWindowLayoutService::restoreDefaultLayout() {
    std::lock_guard<std::mutex> lock(mutex_);
    plan_ = MainWindowLayoutConfiguration::canonicalDefaultLayout();
    auto report =
        MainWindowLayoutValidator::calculateLayoutAreas(plan_.window.width, plan_.window.height, plan_);
    notifyChangedLocked(report);
    return true;
}

void MainWindowLayoutService::setIncognitoMode(bool incognito) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    options_.isIncognitoSession = incognito;
}

bool MainWindowLayoutService::isIncognitoMode() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_.isIncognitoSession;
}

void MainWindowLayoutService::setLayoutChangedCallback(OnLayoutChangedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void MainWindowLayoutService::clearLayoutChangedCallback() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = nullptr;
}

void MainWindowLayoutService::notifyChangedLocked(const MainWindowLayoutReport& report) {
    if (callback_) {
        callback_(plan_, report);
    }
}

std::string MainWindowLayoutService::formatLayoutSummary(const MainWindowLayoutPlan& plan,
                                                         const MainWindowLayoutReport& report) {
    std::string summary;
    summary.reserve(512);
    summary += "Disposición de ventana: ";
    summary += MainWindowLayoutValidator::layoutDensityToString(plan.density);
    summary += " | Dimensiones: ";
    summary += std::to_string(plan.window.width);
    summary += "x";
    summary += std::to_string(plan.window.height);
    summary += " (mínimo: ";
    summary += std::to_string(plan.window.minWidth);
    summary += "x";
    summary += std::to_string(plan.window.minHeight);
    summary += ") | Barra lateral: ";
    summary += MainWindowLayoutValidator::sidebarStateToString(plan.sidebar.state);
    summary += " (";
    summary += std::to_string(plan.sidebar.width);
    summary += " px, ";
    summary += MainWindowLayoutValidator::sidebarDockPositionToString(plan.sidebar.position);
    summary += ") | Estado: ";
    summary += report.valid ? "Válida" : "Inválida";
    if (!report.valid && !report.message.empty()) {
        summary += " - ";
        summary += report.message;
    }
    if (report.valid && (report.contentArea.width > 0 || report.contentArea.height > 0)) {
        summary += " | Área de contenido: ";
        summary += std::to_string(report.contentArea.width);
        summary += "x";
        summary += std::to_string(report.contentArea.height);
    }
    return summary;
}

} // namespace vigilio
