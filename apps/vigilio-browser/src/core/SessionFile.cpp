#include "core/SessionFile.h"
#include "core/UrlInput.h"
#include <fstream>
#include <iomanip>
#include <system_error>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace vigilio {
namespace {
constexpr std::size_t maxTabs = 1000;
}
bool saveSession(const std::filesystem::path& path, const std::vector<std::string>& urls) {
    if (urls.size() > maxTabs)
        return false;
    for (const auto& url : urls) {
        const auto result = resolveInput(url);
        if (url.size() > 8192 || result.kind != NavigationTarget::Kind::Url || result.value != url)
            return false;
    }
    const auto temp = path.string() + ".tmp";
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file)
            return false;
        file << "VIGILIO_SESSION_V1\n";
        for (const auto& url : urls) {
            if (url.size() > 8192)
                return false;
            file << std::quoted(url) << '\n';
        }
        file.flush();
        if (!file)
            return false;
    }
    std::error_code ec;
#ifdef _WIN32
    // Same-volume replacement, preserving the old session if replacement fails.
    if (!MoveFileExW(std::filesystem::path(temp).c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp);
        return false;
    }
#else
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp);
        return false;
    }
#endif
    return true;
}
std::vector<std::string> loadSession(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::string header;
    if (!std::getline(file, header) || header != "VIGILIO_SESSION_V1")
        return {};
    std::vector<std::string> urls;
    std::string url;
    while (file >> std::quoted(url)) {
        if (url.size() > 8192 || urls.size() >= maxTabs)
            return {};
        auto target = resolveInput(url);
        if (target.kind != NavigationTarget::Kind::Url || target.value != url)
            return {};
        urls.push_back(std::move(url));
    }
    if (!file.eof())
        return {};
    return urls;
}
} // namespace vigilio
