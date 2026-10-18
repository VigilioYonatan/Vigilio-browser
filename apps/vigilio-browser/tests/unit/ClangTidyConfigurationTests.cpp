#include "core/ClangTidyConfiguration.h"
#include "core/ClangTidyTypes.h"
#include "core/ClangTidyValidator.h"

#include <algorithm>
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

// TEST-TIDY-001: test_clang_tidy_canonical_options
void test_clang_tidy_canonical_options() {
    const auto canonical = ClangTidyConfiguration::canonicalConfig();
    CHECK(canonical.checks == "-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,"
                              "readability-braces-around-statements");
    CHECK(canonical.warningsAsErrors.empty());
    CHECK(canonical.headerFilterRegex == "src/.*");
    CHECK(canonical.formatStyle == "none");
    CHECK(ClangTidyConfiguration::verifyInvariants(canonical));
}

// TEST-TIDY-002: test_clang_tidy_validator_valid_options
void test_clang_tidy_validator_valid_options() {
    const auto canonical = ClangTidyConfiguration::canonicalConfig();
    CHECK(ClangTidyValidator::validateOptions(canonical) == ClangTidyError::None);

    ClangTidyOptions custom = canonical;
    custom.checks = "bugprone-*";
    CHECK(ClangTidyValidator::validateOptions(custom) == ClangTidyError::None);

    custom.checks = "-*,performance-*";
    CHECK(ClangTidyValidator::validateOptions(custom) == ClangTidyError::None);

    custom.headerFilterRegex = "include/.*";
    CHECK(ClangTidyValidator::validateOptions(custom) == ClangTidyError::None);
}

// TEST-TIDY-003: test_clang_tidy_validator_empty_checks
void test_clang_tidy_validator_empty_checks() {
    auto opts = ClangTidyConfiguration::canonicalConfig();

    opts.checks = "";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::EmptyChecks);

    opts.checks = "   \t  \n";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::EmptyChecks);

    opts.checks = "-*";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::EmptyChecks);

    opts.checks = "-*, -clang-analyzer-*";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::EmptyChecks);
}

// TEST-TIDY-004: test_clang_tidy_validator_invalid_header_filter
void test_clang_tidy_validator_invalid_header_filter() {
    auto opts = ClangTidyConfiguration::canonicalConfig();

    opts.headerFilterRegex = "[a-z(";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::InvalidHeaderFilterRegex);

    opts.headerFilterRegex = "(?+invalid";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::InvalidHeaderFilterRegex);

    opts.headerFilterRegex = "*invalid";
    CHECK(ClangTidyValidator::validateOptions(opts) == ClangTidyError::InvalidHeaderFilterRegex);
}

// TEST-TIDY-005: test_clang_tidy_parse_canonical_yaml
void test_clang_tidy_parse_canonical_yaml() {
    const std::string yaml = "Checks: "
                             "'-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-nullptr,"
                             "readability-braces-around-statements'\n"
                             "WarningsAsErrors: ''\n"
                             "HeaderFilterRegex: 'src/.*'\n";

    ClangTidyOptions parsed;
    std::string errMsg;
    CHECK(ClangTidyValidator::parseConfig(yaml, parsed, &errMsg));
    CHECK(errMsg.empty());
    CHECK(parsed == ClangTidyConfiguration::canonicalConfig());

#ifdef VIGILIO_CLANG_TIDY_PATH
    if (std::filesystem::exists(VIGILIO_CLANG_TIDY_PATH)) {
        const auto report = ClangTidyValidator::validateConfigurationFile(VIGILIO_CLANG_TIDY_PATH);
        CHECK(report.isValid);
        CHECK(report.error == ClangTidyError::None);
        CHECK(!report.enabledChecks.empty());
    }
#endif
}

// TEST-TIDY-006: test_clang_tidy_serialize_roundtrip
void test_clang_tidy_serialize_roundtrip() {
    const auto original = ClangTidyConfiguration::canonicalConfig();
    const std::string serialized = ClangTidyValidator::serializeConfig(original);
    CHECK(!serialized.empty());

    ClangTidyOptions reloaded;
    std::string errMsg;
    CHECK(ClangTidyValidator::parseConfig(serialized, reloaded, &errMsg));
    CHECK(reloaded == original);

    ClangTidyOptions custom = original;
    custom.checks = "-*,cert-*";
    custom.warningsAsErrors = "cert-*";
    custom.headerFilterRegex = "src/core/.*";
    custom.formatStyle = "file";
    const std::string customSerialized = ClangTidyValidator::serializeConfig(custom);
    ClangTidyOptions customReloaded;
    CHECK(ClangTidyValidator::parseConfig(customSerialized, customReloaded, &errMsg));
    CHECK(customReloaded == custom);
}

// TEST-TIDY-007: test_clang_tidy_missing_mandatory_keys
void test_clang_tidy_missing_mandatory_keys() {
    ClangTidyOptions opts;
    std::string errMsg;

    const std::string noChecks = "WarningsAsErrors: ''\nHeaderFilterRegex: 'src/.*'\n";
    CHECK(!ClangTidyValidator::parseConfig(noChecks, opts, &errMsg));
    CHECK(!errMsg.empty());

    const auto report = ClangTidyValidator::validateConfigContent(noChecks);
    CHECK(!report.isValid);
    CHECK(report.error == ClangTidyError::MissingMandatoryKey);

    const std::string malformed = "Checks without colon\n";
    const auto malformedReport = ClangTidyValidator::validateConfigContent(malformed);
    CHECK(!malformedReport.isValid);
    CHECK(malformedReport.error == ClangTidyError::ParseError);

    const auto emptyReport = ClangTidyValidator::validateConfigContent("");
    CHECK(!emptyReport.isValid);
    CHECK(emptyReport.error == ClangTidyError::MissingMandatoryKey ||
          emptyReport.error == ClangTidyError::ParseError);
}

// TEST-TIDY-008: test_clang_tidy_unsafe_path_rejection
void test_clang_tidy_unsafe_path_rejection() {
    CHECK(!ClangTidyValidator::isSafeConfigPath("../../etc/.clang-tidy"));
    CHECK(!ClangTidyValidator::isSafeConfigPath("..\\windows\\tidy"));
    CHECK(!ClangTidyValidator::isSafeConfigPath("/etc/shadow"));
    CHECK(!ClangTidyValidator::isSafeConfigPath("/usr/bin/clang-tidy"));
    CHECK(!ClangTidyValidator::isSafeConfigPath("../.clang-tidy"));
    CHECK(!ClangTidyValidator::isSafeConfigPath(""));

    std::string nullPath = "test";
    nullPath.push_back('\0');
    nullPath.append(".clang-tidy");
    CHECK(!ClangTidyValidator::isSafeConfigPath(nullPath));

    const auto report = ClangTidyValidator::validateConfigurationFile("../../etc/.clang-tidy");
    CHECK(!report.isValid);
    CHECK(report.error == ClangTidyError::UnsafeFilePath);

    const auto shadowReport = ClangTidyValidator::validateConfigurationFile("/etc/shadow");
    CHECK(!shadowReport.isValid);
    CHECK(shadowReport.error == ClangTidyError::UnsafeFilePath);
}

// TEST-TIDY-009: test_clang_tidy_file_extensions_and_exclusions
void test_clang_tidy_file_extensions_and_exclusions() {
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("main.cpp"));
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("header.h"));
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("template.hpp"));
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("source.cc"));
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("impl.cxx"));
    CHECK(ClangTidyConfiguration::isSupportedSourceExtension("Upper.CPP"));

    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension("config.json"));
    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension("README.md"));
    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension("notes.txt"));
    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension("resources.qrc"));
    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension("Makefile"));
    CHECK(!ClangTidyConfiguration::isSupportedSourceExtension(""));

    CHECK(ClangTidyConfiguration::isExcludedDirectory("_build"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory("build"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory("generated"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory(".git"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory(".prompt-runner"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory("assets"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory("node_modules"));
    CHECK(ClangTidyConfiguration::isExcludedDirectory("_build/obj/foo.o"));

    CHECK(!ClangTidyConfiguration::isExcludedDirectory("src/core"));
    CHECK(!ClangTidyConfiguration::isExcludedDirectory("apps/vigilio-browser/src"));
    CHECK(!ClangTidyConfiguration::isExcludedDirectory("tests/unit"));
}

// TEST-TIDY-010: test_clang_tidy_token_extraction
void test_clang_tidy_token_extraction() {
    std::vector<std::string> enabled;
    std::vector<std::string> disabled;

    ClangTidyValidator::parseCheckTokens("-*,bugprone-*,clang-analyzer-*,performance-*,modernize-use-"
                                         "nullptr,readability-braces-around-statements",
                                         enabled, disabled);

    CHECK(std::find(disabled.begin(), disabled.end(), "-*") != disabled.end());
    CHECK(std::find(enabled.begin(), enabled.end(), "bugprone-*") != enabled.end());
    CHECK(std::find(enabled.begin(), enabled.end(), "clang-analyzer-*") != enabled.end());
    CHECK(std::find(enabled.begin(), enabled.end(), "performance-*") != enabled.end());
    CHECK(std::find(enabled.begin(), enabled.end(), "modernize-use-nullptr") != enabled.end());
    CHECK(std::find(enabled.begin(), enabled.end(), "readability-braces-around-statements") !=
          enabled.end());

    std::vector<std::string> enabledPrefixed;
    std::vector<std::string> disabledPrefixed;
    ClangTidyValidator::parseCheckTokens("+cert-*, -google-*", enabledPrefixed, disabledPrefixed);
    CHECK(std::find(enabledPrefixed.begin(), enabledPrefixed.end(), "cert-*") != enabledPrefixed.end());
    CHECK(std::find(disabledPrefixed.begin(), disabledPrefixed.end(), "-google-*") !=
          disabledPrefixed.end());
}

// TEST-TIDY-011: test_clang_tidy_public_copy_compliance
void test_clang_tidy_public_copy_compliance() {
    const std::vector<std::string> forbiddenTerms = {"Senior Mode", "Modo Senior", "Prompt Builder",
                                                     "DAG",         "OpenSpec",    "toolchain"};

    const std::vector<ClangTidyValidationState> allStates = {
        ClangTidyValidationState::Unconfigured, ClangTidyValidationState::Configured,
        ClangTidyValidationState::Valid, ClangTidyValidationState::Invalid,
        ClangTidyValidationState::DiscrepancyDetected};

    for (const auto state : allStates) {
        const std::string text = ClangTidyValidator::tidyStateToString(state);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }

    const std::vector<ClangTidyError> allErrors = {ClangTidyError::None,
                                                   ClangTidyError::FileNotFound,
                                                   ClangTidyError::ParseError,
                                                   ClangTidyError::EmptyChecks,
                                                   ClangTidyError::InvalidHeaderFilterRegex,
                                                   ClangTidyError::UnsafeFilePath,
                                                   ClangTidyError::CorruptedRules,
                                                   ClangTidyError::MissingMandatoryKey};

    for (const auto err : allErrors) {
        const std::string text = ClangTidyValidator::tidyErrorToString(err);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }

    const std::vector<ClangTidyCheckCategory> allCategories = {
        ClangTidyCheckCategory::Bugprone,    ClangTidyCheckCategory::ClangAnalyzer,
        ClangTidyCheckCategory::Performance, ClangTidyCheckCategory::Modernize,
        ClangTidyCheckCategory::Readability, ClangTidyCheckCategory::CppCoreGuidelines,
        ClangTidyCheckCategory::Portability, ClangTidyCheckCategory::Custom};

    for (const auto cat : allCategories) {
        const std::string text = ClangTidyValidator::categoryToString(cat);
        CHECK(!text.empty());
        for (const auto& term : forbiddenTerms) {
            CHECK(text.find(term) == std::string::npos);
        }
    }
}

} // namespace

int main() {
    try {
        test_clang_tidy_canonical_options();
        test_clang_tidy_validator_valid_options();
        test_clang_tidy_validator_empty_checks();
        test_clang_tidy_validator_invalid_header_filter();
        test_clang_tidy_parse_canonical_yaml();
        test_clang_tidy_serialize_roundtrip();
        test_clang_tidy_missing_mandatory_keys();
        test_clang_tidy_unsafe_path_rejection();
        test_clang_tidy_file_extensions_and_exclusions();
        test_clang_tidy_token_extraction();
        test_clang_tidy_public_copy_compliance();

        std::cout << "All ClangTidy configuration tests passed successfully." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << std::endl;
        return 1;
    }
}
