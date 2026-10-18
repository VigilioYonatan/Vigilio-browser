#pragma once
#include <string>

namespace vigilio {
struct NavigationTarget {
    enum class Kind { Url, Search, Rejected } kind;
    std::string value;
};
NavigationTarget resolveInput(const std::string& input);
} // namespace vigilio
