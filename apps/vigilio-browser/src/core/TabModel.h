#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace vigilio {
struct Tab {
    unsigned long long id;
    std::string url;
    bool pinned{false};
    bool muted{false};
};
class TabModel {
  public:
    unsigned long long open(std::string url);
    bool close(unsigned long long id);
    bool setPinned(unsigned long long id, bool value);
    bool setMuted(unsigned long long id, bool value);
    std::optional<unsigned long long> duplicate(unsigned long long id);
    bool closeOthers(unsigned long long except);
    std::vector<Tab> tabs() const {
        return tabs_;
    }
    std::size_t size() const {
        return tabs_.size();
    }

  private:
    unsigned long long nextId_{1};
    std::vector<Tab> tabs_;
};
} // namespace vigilio
