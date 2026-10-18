#include "core/UrlInput.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace vigilio {
namespace {
std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
std::string encodeQuery(const std::string& input) {
    std::ostringstream out;
    out << std::uppercase << std::hex;
    for (char raw_ch : input) {
        const auto c = static_cast<unsigned char>(raw_ch);
        if ((std::isalnum(c) && c < 128) || c == '-' || c == '_' || c == '.' || c == '~')
            out << raw_ch;
        else if (c == ' ')
            out << '+';
        else
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return out.str();
}
bool hasScheme(const std::string& value) {
    const auto colon = value.find(':');
    if (colon == std::string::npos || colon == 0)
        return false;
    if (!std::isalpha(static_cast<unsigned char>(value.front())))
        return false;
    for (std::size_t i = 1; i < colon; ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (!std::isalnum(c) && c != '+' && c != '-' && c != '.')
            return false;
    }
    return true;
}
} // namespace
NavigationTarget resolveInput(const std::string& input) {
    const std::string raw = trim(input);
    if (raw.empty() || raw.size() > 8192)
        return {NavigationTarget::Kind::Rejected, ""};
    const auto lowered = lower(raw);
    // Navigation from an untrusted text field must not execute script or local-file schemes.
    if (lowered.starts_with("localhost:") && raw.find_first_of(" \t\r\n") == std::string::npos)
        return {NavigationTarget::Kind::Url, "http://" + raw};
    if (hasScheme(raw)) {
        if (lowered.starts_with("https://") || lowered.starts_with("http://")) {
            // Whitespace inside a URL is ambiguous; use search rather than auto-correct.
            if (raw.find_first_of(" \t\r\n") != std::string::npos) {
                return {NavigationTarget::Kind::Search,
                        "https://www.google.com/search?q=" + encodeQuery(raw)};
            }
            const auto offset = lowered.starts_with("https://") ? 8U : 7U;
            const auto hostEnd = raw.find_first_of("/?#", offset);
            const auto host =
                raw.substr(offset, hostEnd == std::string::npos ? std::string::npos : hostEnd - offset);
            if (host.empty() || host.find('@') != std::string::npos ||
                host.find('\\') != std::string::npos)
                return {NavigationTarget::Kind::Rejected, ""};
            return {NavigationTarget::Kind::Url, raw};
        }
        return {NavigationTarget::Kind::Rejected, ""};
    }
    // Hostnames and localhost are treated as navigable, arbitrary text as search.
    if (raw.find_first_of(" \t\r\n") == std::string::npos &&
        (raw.find('.') != std::string::npos || lowered == "localhost" ||
         lowered.starts_with("localhost:"))) {
        if (raw.find('@') != std::string::npos || raw.find('\\') != std::string::npos)
            return {NavigationTarget::Kind::Rejected, ""};
        return {NavigationTarget::Kind::Url, "https://" + raw};
    }
    return {NavigationTarget::Kind::Search, "https://www.google.com/search?q=" + encodeQuery(raw)};
}
} // namespace vigilio
