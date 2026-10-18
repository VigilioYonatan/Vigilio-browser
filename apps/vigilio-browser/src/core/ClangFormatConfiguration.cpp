#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatValidator.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>

namespace vigilio {

ClangFormatOptions ClangFormatConfiguration::canonicalConfig() {
    ClangFormatOptions options;
    options.stylePreset = ClangFormatStylePreset::LLVM;
    options.indentWidth = 4;
    options.columnLimit = 105;
    options.pointerAlignment = PointerAlignmentPolicy::Left;
    options.allowShortFunctions = ShortFunctionPolicy::None;
    options.cppStandard = "c++20";
    return options;
}

bool ClangFormatConfiguration::isSupportedSourceExtension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cc" || ext == ".cxx";
}

bool ClangFormatConfiguration::isExcludedDirectory(const std::filesystem::path& path) {
    static const std::set<std::string> excludedDirs = {"_build", "build",          "generated",
                                                       ".git",   ".prompt-runner", "node_modules"};

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

bool ClangFormatConfiguration::verifyInvariants(const ClangFormatOptions& options) {
    return ClangFormatValidator::validateOptions(options) == ClangFormatError::None;
}

} // namespace vigilio
