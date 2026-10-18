#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef VIGILIO_CXX_COMPILER
#define VIGILIO_CXX_COMPILER "c++"
#endif

#ifndef VIGILIO_FIXTURES_DIR
#define VIGILIO_FIXTURES_DIR ""
#endif

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error("CHECK failed at line " + std::to_string(__LINE__) +               \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

// Compile-time checks for C++20 and environment hygiene
static_assert(__cplusplus >= 202002L, "C++20 standard is required");
static_assert(sizeof(void*) >= 4, "32-bit or 64-bit architecture is required");
static_assert(alignof(std::max_align_t) >= 4, "Alignment hygiene check");
static_assert(sizeof(std::int32_t) == 4, "std::int32_t must be exactly 4 bytes");
static_assert(sizeof(std::int64_t) == 8, "std::int64_t must be exactly 8 bytes");

struct TempFileGuard {
    std::filesystem::path path;
    explicit TempFileGuard(std::filesystem::path p) : path(std::move(p)) {
        cleanup();
    }
    ~TempFileGuard() {
        cleanup();
    }
    void cleanup() noexcept {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
};

struct PolymorphicBase {
    virtual ~PolymorphicBase() = default;
    [[nodiscard]] virtual int evaluate() const = 0;
};

struct PolymorphicDerived final : PolymorphicBase {
    [[nodiscard]] int evaluate() const override {
        return 42;
    }
};

struct CommandOutput {
    int exitCode{0};
    std::string stderrOutput;
};

std::string getCompilerExecutable() {
    const char* envCxx = std::getenv("CXX");
    if (envCxx != nullptr && *envCxx != '\0') {
        return std::string(envCxx);
    }
    return std::string(VIGILIO_CXX_COMPILER);
}

CommandOutput runCompiler(const std::string& compiler, const std::vector<std::string>& flags,
                          const std::filesystem::path& sourceFile) {
    static std::uint64_t fileSequence = 0;
    ++fileSequence;

    const auto tempDir = std::filesystem::temp_directory_path();
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    const auto processId = static_cast<unsigned long long>(getpid());
#else
    const auto processId = 1000ULL;
#endif

    const auto errPath = tempDir / ("vigilio_compiler_test_err_" + std::to_string(processId) + "_" +
                                    std::to_string(fileSequence) + ".log");
    TempFileGuard errGuard(errPath);

    std::ostringstream commandStream;
    commandStream << compiler << " -std=c++20";
    for (const auto& flag : flags) {
        commandStream << " " << flag;
    }
    commandStream << " -fsyntax-only \"" << sourceFile.string() << "\" 2> \"" << errPath.string()
                  << "\"";

    const std::string command = commandStream.str();
    const int rawStatus = std::system(command.c_str());

    int exitCode = rawStatus;
#if defined(WIFEXITED) && defined(WEXITSTATUS)
    if (WIFEXITED(rawStatus)) {
        exitCode = WEXITSTATUS(rawStatus);
    }
#endif

    std::string stderrStr;
    {
        std::ifstream errFile(errPath);
        if (errFile) {
            std::ostringstream ss;
            ss << errFile.rdbuf();
            stderrStr = ss.str();
        }
    }

    return {exitCode, stderrStr};
}

std::filesystem::path resolveFixturePath(const std::string& filename, const std::string& fallbackContent,
                                         std::vector<std::unique_ptr<TempFileGuard>>& guards) {
    if (std::string(VIGILIO_FIXTURES_DIR).length() > 0) {
        const auto candidate = std::filesystem::path(VIGILIO_FIXTURES_DIR) / filename;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    static std::uint64_t fallbackSequence = 0;
    ++fallbackSequence;
    const auto tempDir = std::filesystem::temp_directory_path();
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    const auto processId = static_cast<unsigned long long>(getpid());
#else
    const auto processId = 1000ULL;
#endif

    const auto fallbackPath = tempDir / ("vigilio_synth_fixture_" + std::to_string(processId) + "_" +
                                         std::to_string(fallbackSequence) + ".cpp");
    guards.push_back(std::make_unique<TempFileGuard>(fallbackPath));

    {
        std::ofstream ofs(fallbackPath, std::ios::trunc);
        if (!ofs) {
            throw std::runtime_error("Failed to write synthetic fixture: " + fallbackPath.string());
        }
        ofs << fallbackContent;
    }

    return fallbackPath;
}

void testHygienicCpp20Semantics() {
    // 1. Uniform initialization without narrowing
    const int cleanInteger{123};
    CHECK(cleanInteger == 123);

    // 2. Explicit static_cast without C-style casts
    const double preciseValue = 3.14159;
    const auto castedValue = static_cast<int>(preciseValue);
    CHECK(castedValue == 3);

    // 3. Polymorphism and virtual destructor verification
    const PolymorphicBase* polyInstance = new PolymorphicDerived();
    CHECK(polyInstance->evaluate() == 42);
    delete polyInstance;

    // 4. Safe signed and unsigned comparison hygiene with static_cast
    const unsigned int unsignedVal = 10u;
    const int signedVal = 10;
    CHECK(static_cast<int>(unsignedVal) == signedVal);
}

void testCleanCodePositiveCompilation(const std::string& compiler,
                                      std::vector<std::unique_ptr<TempFileGuard>>& guards) {
    const std::string cleanFallback = "#include <cstdint>\n"
                                      "int main() {\n"
                                      "    const std::int32_t cleanValue{42};\n"
                                      "    return (cleanValue == 42) ? 0 : 1;\n"
                                      "}\n";

    const auto fixturePath = resolveFixturePath("fixture_clean.cpp", cleanFallback, guards);
    const std::vector<std::string> strictFlags = {"-Wall",
                                                  "-Wextra",
                                                  "-Wpedantic",
                                                  "-Wconversion",
                                                  "-Wsign-conversion",
                                                  "-Wshadow",
                                                  "-Wnon-virtual-dtor",
                                                  "-Wold-style-cast",
                                                  "-Werror"};

    const auto result = runCompiler(compiler, strictFlags, fixturePath);
    CHECK(result.exitCode == 0);
}

void testNegativeWarningRejection(const std::string& compiler,
                                  std::vector<std::unique_ptr<TempFileGuard>>& guards) {
    // 1. Narrowing conversion: double to int (-Wconversion)
    {
        const std::string fallback = "int main() {\n"
                                     "    double d = 3.14;\n"
                                     "    int i = d;\n"
                                     "    return i;\n"
                                     "}\n";
        const auto path = resolveFixturePath("fixture_narrowing.cpp", fallback, guards);
        const auto result = runCompiler(compiler, {"-Werror", "-Wconversion"}, path);
        CHECK(result.exitCode != 0);
        CHECK(result.stderrOutput.find("conversion") != std::string::npos);
    }

    // 2. Sign conversion: signed to unsigned int (-Wsign-conversion)
    {
        const std::string fallback = "int main() {\n"
                                     "    int s = -1;\n"
                                     "    unsigned int u = s;\n"
                                     "    return (u == 0) ? 0 : 1;\n"
                                     "}\n";
        const auto path = resolveFixturePath("fixture_sign.cpp", fallback, guards);
        const auto result = runCompiler(compiler, {"-Werror", "-Wsign-conversion"}, path);
        CHECK(result.exitCode != 0);
        CHECK(result.stderrOutput.find("sign-conversion") != std::string::npos);
    }

    // 3. Variable shadowing (-Wshadow)
    {
        const std::string fallback = "int calculate(int x) {\n"
                                     "    int inner = 0;\n"
                                     "    {\n"
                                     "        int x = 2;\n"
                                     "        inner += x;\n"
                                     "    }\n"
                                     "    return inner + x;\n"
                                     "}\n"
                                     "int main() {\n"
                                     "    return calculate(1);\n"
                                     "}\n";
        const auto path = resolveFixturePath("fixture_shadow.cpp", fallback, guards);
        const auto result = runCompiler(compiler, {"-Werror", "-Wshadow"}, path);
        CHECK(result.exitCode != 0);
        CHECK(result.stderrOutput.find("shadow") != std::string::npos);
    }

    // 4. Non-virtual destructor in polymorphic class (-Wnon-virtual-dtor)
    {
        const std::string fallback = "struct BasePolymorphic {\n"
                                     "    virtual void execute();\n"
                                     "    ~BasePolymorphic();\n"
                                     "};\n";
        const auto path = resolveFixturePath("fixture_non_virtual_dtor.cpp", fallback, guards);
        const auto result = runCompiler(compiler, {"-Werror", "-Wnon-virtual-dtor"}, path);
        CHECK(result.exitCode != 0);
        CHECK(result.stderrOutput.find("non-virtual-dtor") != std::string::npos);
    }

    // 5. Old-style C cast (-Wold-style-cast)
    {
        const std::string fallback = "int main() {\n"
                                     "    double d = 4.2;\n"
                                     "    int i = (int)d;\n"
                                     "    return i;\n"
                                     "}\n";
        const auto path = resolveFixturePath("fixture_c_cast.cpp", fallback, guards);
        const auto result = runCompiler(compiler, {"-Werror", "-Wold-style-cast"}, path);
        CHECK(result.exitCode != 0);
        CHECK(result.stderrOutput.find("old-style-cast") != std::string::npos);
    }
}

void testPermissiveModeRecovery(const std::string& compiler,
                                std::vector<std::unique_ptr<TempFileGuard>>& guards) {
    // In permissive mode (without -Werror), compiler emits warning diagnostic to stderr but exits with
    // code 0
    const std::string fallback = "int main() {\n"
                                 "    double d = 4.2;\n"
                                 "    int i = (int)d;\n"
                                 "    return i;\n"
                                 "}\n";
    const auto path = resolveFixturePath("fixture_c_cast.cpp", fallback, guards);
    const auto result = runCompiler(compiler, {"-Wold-style-cast"}, path);
    CHECK(result.exitCode == 0);
    CHECK(result.stderrOutput.find("warning") != std::string::npos);
    CHECK(result.stderrOutput.find("old-style-cast") != std::string::npos);
}

} // namespace

int main() {
    try {
        std::vector<std::unique_ptr<TempFileGuard>> temporaryGuards;
        const std::string compiler = getCompilerExecutable();

        // Level A: In-process C++20 standard hygiene and semantics
        testHygienicCpp20Semantics();

        // Level B1: Positive clean compilation with all strict warning flags + -Werror
        testCleanCodePositiveCompilation(compiler, temporaryGuards);

        // Level B2: Negative rejection of controlled warning violations
        testNegativeWarningRejection(compiler, temporaryGuards);

        // Level C: Permissive mode recovery without -Werror
        testPermissiveModeRecovery(compiler, temporaryGuards);

        std::cout << "Compiler warnings validation: PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Compiler warnings validation: FAIL - " << ex.what() << '\n';
        return 1;
    }
}
