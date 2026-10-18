#include "core/TabModel.h"
#include <algorithm>
#include <utility>
namespace vigilio {
unsigned long long TabModel::open(std::string url) {
    auto id = nextId_++;
    tabs_.push_back({id, std::move(url), false, false});
    return id;
}
bool TabModel::close(unsigned long long id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(), [id](const Tab& t) { return t.id == id; });
    if (it == tabs_.end())
        return false;
    tabs_.erase(it);
    return true;
}
bool TabModel::setPinned(unsigned long long id, bool value) {
    for (auto& t : tabs_) {
        if (t.id == id) {
            t.pinned = value;
            return true;
        }
    }
    return false;
}
bool TabModel::setMuted(unsigned long long id, bool value) {
    for (auto& t : tabs_) {
        if (t.id == id) {
            t.muted = value;
            return true;
        }
    }
    return false;
}
std::optional<unsigned long long> TabModel::duplicate(unsigned long long id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(), [id](const Tab& t) { return t.id == id; });
    if (it == tabs_.end())
        return std::nullopt;
    return open(it->url);
}
bool TabModel::closeOthers(unsigned long long except) {
    auto it =
        std::find_if(tabs_.begin(), tabs_.end(), [except](const Tab& t) { return t.id == except; });
    if (it == tabs_.end())
        return false;
    tabs_.erase(std::remove_if(tabs_.begin(), tabs_.end(),
                               [except](const Tab& t) { return t.id != except && !t.pinned; }),
                tabs_.end());
    return true;
}
} // namespace vigilio
