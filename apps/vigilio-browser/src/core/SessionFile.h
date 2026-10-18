#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace vigilio {
// Simple bounded URL-only session snapshot. Never stores cookies or credentials.
bool saveSession(const std::filesystem::path& path, const std::vector<std::string>& urls);
std::vector<std::string> loadSession(const std::filesystem::path& path);
} // namespace vigilio
