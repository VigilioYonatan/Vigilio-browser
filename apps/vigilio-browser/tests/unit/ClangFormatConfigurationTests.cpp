#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatTypes.h"
#include "core/ClangFormatValidator.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition " at line " +                          \
                                     std::to_string(__LINE__));                                         \
    } while (false)

namespace {

// TEST-FORMAT-001: test_clang_format_canonical_options
void test_clang_format_canonical_options() {
    const auto canonical = ClangFormatConfiguration::canonicalConfig();
    CHECK(canonical.stylePreset == ClangFormatStylePreset::LLVM);
    CHECK(canonical.indentWidth == 4);
    CHECK(canonical.columnLimit == 105);
    CHECK(canonical.pointerAlignment == PointerAlignmentPolicy::Left);
    CHECK(canonical.allowShortFunctions == ShortFunctionPolicy::None);
    CHECK(canonical.cppStandard == "c++20");
    CHECK(ClangFormatConfiguration::verifyInvariants(canonical));
}

// TEST-FORMAT-002: test_clang_format_validator_valid_options
void test_clang_format_validator_valid_options() {
    const auto canonical = ClangFormatConfiguration::canonicalConfig();
    CHECK(ClangFormatValidator::validateOptions(canonical) == ClangFormatError::None);

    ClangFormatOptions custom = canonical;
    custom.indentWidth = 2;
    CHECK(ClangFormatValidator::validateOptions(custom) == ClangFormatError::None);
    custom.indentWidth = 8;
    CHECK(ClangFormatValidator::validateOptions(custom) == ClangFormatError::None);
    custom.columnLimit = 80;
    CHECK(ClangFormatValidator::validateOptions(custom) == ClangFormatError::None);
    custom.columnLimit = 140;
    CHECK(ClangFormatValidator::validateOptions(custom) == ClangFormatError::None);
}

// TEST-FORMAT-003: test_clang_format_validator_invalid_indent
void test_clang_format_validator_invalid_indent() {
    auto opts = ClangFormatConfiguration::canonicalConfig();
    opts.indentWidth = 0;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidIndentWidth);

    opts.indentWidth = 16;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidIndentWidth);

    opts.indentWidth = 3;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidIndentWidth);

    opts.indentWidth = -2;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidIndentWidth);
}

// TEST-FORMAT-004: test_clang_format_validator_invalid_column_limit
void test_clang_format_validator_invalid_column_limit() {
    auto opts = ClangFormatConfiguration::canonicalConfig();
    opts.columnLimit = 30;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidColumnLimit);

    opts.columnLimit = 250;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidColumnLimit);

    opts.columnLimit = 79;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidColumnLimit);

    opts.columnLimit = 141;
    CHECK(ClangFormatValidator::validateOptions(opts) == ClangFormatError::InvalidColumnLimit);
}

// TEST-FORMAT-005: test_clang_format_parse_canonical_yaml
void test_clang_format_parse_canonical_yaml() {
    const std::string yaml = "BasedOnStyle: LLVM\n"
                             "IndentWidth: 4\n"
                             "ColumnLimit: 105\n"
                             "PointerAlignment: Left\n"
                             "AllowShortFunctionsOnASingleLine: None\n";

    ClangFormatOptions parsed;
    std::string errMsg;
    CHECK(ClangFormatValidator::parseConfig(yaml, parsed, &errMsg));
    CHECK(errMsg.empty());
    CHECK(parsed == ClangFormatConfiguration::canonicalConfig());

#ifdef VIGILIO_CLANG_FORMAT_PATH
    if (std::filesystem::exists(VIGILIO_CLANG_FORMAT_PATH)) {
        const auto report = ClangFormatValidator::validateConfigurationFile(VIGILIO_CLANG_FORMAT_PATH);
        CHECK(report.isValid);
        CHECK(report.error == ClangFormatError::None);
    }
#endif
}

// TEST-FORMAT-006: test_clang_format_serialize_roundtrip
void test_clang_format_serialize_roundtrip() {
    const auto original = ClangFormatConfiguration::canonicalConfig();
    const std::string serialized = ClangFormatValidator::serializeConfig(original);
    CHECK(!serialized.empty());

    ClangFormatOptions reloaded;
    std::string errMsg;
    CHECK(ClangFormatValidator::parseConfig(serialized, reloaded, &errMsg));
    CHECK(reloaded == original);

    ClangFormatOptions modified = original;
    modified.indentWidth = 2;
    modified.columnLimit = 120;
    const std::string serializedMod = ClangFormatValidator::serializeConfig(modified);
    ClangFormatOptions reloadedMod;
    CHECK(ClangFormatValidator::parseConfig(serializedMod, reloadedMod, &errMsg));
    CHECK(reloadedMod == modified);
}

// TEST-FORMAT-007: test_clang_format_missing_mandatory_keys
void test_clang_format_missing_mandatory_keys() {
    ClangFormatOptions opts;
    std::string errMsg;

    const std::string noBasedOnStyle = "IndentWidth: 4\n"
                                       "ColumnLimit: 105\n";
    CHECK(!ClangFormatValidator::parseConfig(noBasedOnStyle, opts, &errMsg));
    CHECK(!errMsg.empty());

    const std::string noColumnLimit = "BasedOnStyle: LLVM\n"
                                      "IndentWidth: 4\n";
    CHECK(!ClangFormatValidator::parseConfig(noColumnLimit, opts, &errMsg));

    const std::string noIndentWidth = "BasedOnStyle: LLVM\n"
                                      "ColumnLimit: 105\n";
    CHECK(!ClangFormatValidator::parseConfig(noIndentWidth, opts, &errMsg));

    CHECK(!ClangFormatValidator::parseConfig("", opts, &errMsg));
}

// TEST-FORMAT-008: test_clang_format_unsafe_path_rejection
void test_clang_format_unsafe_path_rejection() {
    CHECK(!ClangFormatValidator::isSafeConfigPath("../../etc/.clang-format"));
    CHECK(!ClangFormatValidator::isSafeConfigPath("/etc/shadow"));
    CHECK(!ClangFormatValidator::isSafeConfigPath("/var/log/syslog"));
    CHECK(!ClangFormatValidator::isSafeConfigPath("../.clang-format"));
    CHECK(!ClangFormatValidator::isSafeConfigPath(""));

    std::string nullPath = "test";
    nullPath.push_back('\0');
    nullPath.append(".clang-format");
    CHECK(!ClangFormatValidator::isSafeConfigPath(nullPath));

    const auto report = ClangFormatValidator::validateConfigurationFile("../../etc/.clang-format");
    CHECK(!report.isValid);
    CHECK(report.error == ClangFormatError::UnsafeFilePath);

    const auto shadowReport = ClangFormatValidator::validateConfigurationFile("/etc/shadow");
    CHECK(!shadowReport.isValid);
    CHECK(shadowReport.error == ClangFormatError::UnsafeFilePath);
}

// TEST-FORMAT-009: test_clang_format_file_extensions_and_exclusions
void test_clang_format_file_extensions_and_exclusions() {
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("main.cpp"));
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("header.h"));
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("template.hpp"));
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("source.cc"));
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("impl.cxx"));
    CHECK(ClangFormatConfiguration::isSupportedSourceExtension("Upper.CPP"));

    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension("config.json"));
    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension("README.md"));
    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension("notes.txt"));
    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension("resources.qrc"));
    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension("Makefile"));
    CHECK(!ClangFormatConfiguration::isSupportedSourceExtension(""));

    CHECK(ClangFormatConfiguration::isExcludedDirectory("_build"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory("build"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory("generated"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory(".git"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory(".prompt-runner"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory("node_modules"));
    CHECK(ClangFormatConfiguration::isExcludedDirectory("_build/obj/foo.o"));

    CHECK(!ClangFormatConfiguration::isExcludedDirectory("src/core"));
    CHECK(!ClangFormatConfiguration::isExcludedDirectory("apps/vigilio-browser/src"));
    CHECK(!ClangFormatConfiguration::isExcludedDirectory("tests/unit"));
}

// TEST-FORMAT-010: test_clang_format_public_copy_compliance
void test_clang_format_public_copy_compliance() {
    const std::vector<std::string> forbiddenTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                     "DAG",         "OpenSpec",    "toolchain"};

    const std::vector<ClangFormatValidationState> allStates = {
        ClangFormatValidationState::Unconfigured, ClangFormatValidationState::Configured,
        ClangFormatValidationState::Valid, ClangFormatValidationState::Invalid,
        ClangFormatValidationState::DiscrepancyDetected};

    for (const auto state : allStates) {
        const std::string text = ClangFormatValidator::formatStateToString(state);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }

    const std::vector<ClangFormatError> allErrors = {ClangFormatError::None,
                                                     ClangFormatError::FileNotFound,
                                                     ClangFormatError::ParseError,
                                                     ClangFormatError::UnsupportedVersion,
                                                     ClangFormatError::InvalidIndentWidth,
                                                     ClangFormatError::InvalidColumnLimit,
                                                     ClangFormatError::UnsafeFilePath,
                                                     ClangFormatError::CorruptedRules,
                                                     ClangFormatError::FormattingDiscrepancy};

    for (const auto err : allErrors) {
        const std::string text = ClangFormatValidator::formatErrorToString(err);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }
}

} // namespace

int main() {
    try {
        test_clang_format_canonical_options();
        test_clang_format_validator_valid_options();
        test_clang_format_validator_invalid_indent();
        test_clang_format_validator_invalid_column_limit();
        test_clang_format_parse_canonical_yaml();
        test_clang_format_serialize_roundtrip();
        test_clang_format_missing_mandatory_keys();
        test_clang_format_unsafe_path_rejection();
        test_clang_format_file_extensions_and_exclusions();
        test_clang_format_public_copy_compliance();

        std::cout << "ClangFormatConfigurationTests: ALL 10 TESTS PASSED\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ClangFormatConfigurationTests FAILED: " << ex.what() << '\n';
        return 1;
    }
}
