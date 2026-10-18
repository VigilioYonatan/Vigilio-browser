#include "core/ClangTidyConfiguration.h"
#include "core/ClangTidyValidator.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>

namespace vigilio {

ClangTidyOptions ClangTidyConfiguration::canonicalConfig() {
    ClangTidyOptions options;
    options.checks = "-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,readability-"
                     "braces-around-statements";
    options.warningsAsErrors = "";
    options.headerFilterRegex = "src/.*";
    options.formatStyle = "none";
    return options;
}

bool ClangTidyConfiguration::isSupportedSourceExtension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc" || ext == ".cxx";
}

bool ClangTidyConfiguration::isExcludedDirectory(const std::filesystem::path& path) {
    static const std::set<std::string> excludedDirs = {"_build",         "build",  "generated",   ".git",
                                                       ".prompt-runner", "assets", "node_modules"};

    const std::string pathStr = path.string();
    if (excludedDirs.contains(pathStr)) {
        return true;
    }

    for (const auto& part : path) {
        if (excludedDirs.contains(part.string())) {
            return true;
        }
    }
    return false;
}

bool ClangTidyConfiguration::verifyInvariants(const ClangTidyOptions& options) {
    return ClangTidyValidator::validateOptions(options) == ClangTidyError::None;
}

} // namespace vigilio
