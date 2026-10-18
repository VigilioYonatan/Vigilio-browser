#include "core/CMakePresetsTypes.h"
#include "core/CMakePresetsValidator.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error(std::string("CHECK failed at line ") + std::to_string(__LINE__) +  \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

// JSON canónico representativo con los perfiles canónicos
const std::string CANONICAL_VALID_JSON = R"({
  "version": 6,
  "configurePresets": [
    {
      "name": "core-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-core-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "OFF",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "core-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-core-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_BUILD_GUI": "OFF",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "desktop-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "ON",
        "VIGILIO_ENABLE_E2E": "OFF"
      }
    },
    {
      "name": "desktop-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "OFF",
        "VIGILIO_ENABLE_E2E": "OFF"
      }
    },
    {
      "name": "desktop-e2e",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-desktop-e2e",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_BUILD_GUI": "ON",
        "BUILD_TESTING": "ON",
        "VIGILIO_ENABLE_E2E": "ON"
      }
    },
    {
      "name": "ci-linux-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ci-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIGILIO_WARNINGS_AS_ERRORS": "ON",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "ci-linux-release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ci-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIGILIO_WARNINGS_AS_ERRORS": "ON"
      }
    },
    {
      "name": "sanitizer-asan",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-asan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "sanitizer-ubsan",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-ubsan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTING": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "core-debug", "configurePreset": "core-debug" },
    { "name": "core-release", "configurePreset": "core-release" },
    { "name": "desktop-debug", "configurePreset": "desktop-debug" },
    { "name": "desktop-release", "configurePreset": "desktop-release" },
    { "name": "desktop-e2e", "configurePreset": "desktop-e2e" },
    { "name": "ci-linux-debug", "configurePreset": "ci-linux-debug" },
    { "name": "ci-linux-release", "configurePreset": "ci-linux-release" },
    { "name": "sanitizer-asan", "configurePreset": "sanitizer-asan" },
    { "name": "sanitizer-ubsan", "configurePreset": "sanitizer-ubsan" }
  ],
  "testPresets": [
    { "name": "core-debug", "configurePreset": "core-debug", "output": { "outputOnFailure": true } },
    { "name": "core-release", "configurePreset": "core-release", "output": { "outputOnFailure": true } },
    { "name": "desktop-debug", "configurePreset": "desktop-debug", "output": { "outputOnFailure": true } },
    { "name": "desktop-e2e", "configurePreset": "desktop-e2e", "output": { "outputOnFailure": true } },
    { "name": "ci-linux-debug", "configurePreset": "ci-linux-debug", "output": { "outputOnFailure": true } },
    { "name": "sanitizer-asan", "configurePreset": "sanitizer-asan", "output": { "outputOnFailure": true } },
    { "name": "sanitizer-ubsan", "configurePreset": "sanitizer-ubsan", "output": { "outputOnFailure": true } }
  ],
  "packagePresets": [
    { "name": "desktop-linux-deb" }
  ],
  "workflowPresets": [
    { "name": "ci-pipeline" }
  ]
})";

// TEST-PRESET-001: JSON canónico con presets válidos
void test_presets_canonical_valid() {
    const auto report = CMakePresetsValidator::validatePresetsJson(CANONICAL_VALID_JSON);

    CHECK(report.isValid());
    CHECK(report.outcome == CMakePresetValidationOutcome::Valid);
    CHECK(report.schemaVersion == 6);
    CHECK(report.configurePresets.size() == 9);
    CHECK(report.buildPresets.size() == 9);
    CHECK(report.testPresets.size() == 7);
    CHECK(report.packagePresets.size() == 1);
    CHECK(report.workflowPresets.size() == 1);
    CHECK(!report.message.empty());
}

// TEST-PRESET-002: Esquema de versión menor a 3
void test_presets_invalid_schema_version() {
    const std::string jsonV2 = R"({
      "version": 2,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto report = CMakePresetsValidator::validatePresetsJson(jsonV2);
    CHECK(!report.isValid());
    CHECK(report.outcome == CMakePresetValidationOutcome::InvalidSchemaVersion);
    CHECK(report.schemaVersion == 2);

    const std::string jsonNoVersion = R"({
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" }
      ]
    })";
    const auto reportNoVersion = CMakePresetsValidator::validatePresetsJson(jsonNoVersion);
    CHECK(!reportNoVersion.isValid());
    CHECK(reportNoVersion.outcome == CMakePresetValidationOutcome::InvalidSchemaVersion);
}

// TEST-PRESET-003: Preset obligatorio faltante (falta core-debug o desktop-debug)
void test_presets_missing_required_preset() {
    const std::string jsonMissingCore = R"({
      "version": 6,
      "configurePresets": [
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportCore = CMakePresetsValidator::validatePresetsJson(jsonMissingCore);
    CHECK(!reportCore.isValid());
    CHECK(reportCore.outcome == CMakePresetValidationOutcome::MissingRequiredPreset);

    const std::string jsonMissingDesktop = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" }
      ]
    })";

    const auto reportDesktop = CMakePresetsValidator::validatePresetsJson(jsonMissingDesktop);
    CHECK(!reportDesktop.isValid());
    CHECK(reportDesktop.outcome == CMakePresetValidationOutcome::MissingRequiredPreset);
}

// TEST-PRESET-004 & TEST-PRESET-005: Referencia huérfana en buildPresets o testPresets
void test_presets_orphan_configure_reference() {
    // Build preset con configurePreset huérfano
    const std::string jsonOrphanBuild = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "buildPresets": [
        { "name": "core-debug", "configurePreset": "nonexistent-preset" }
      ]
    })";

    const auto reportBuild = CMakePresetsValidator::validatePresetsJson(jsonOrphanBuild);
    CHECK(!reportBuild.isValid());
    CHECK(reportBuild.outcome == CMakePresetValidationOutcome::OrphanConfigurePresetReference);

    // Test preset con configurePreset huérfano
    const std::string jsonOrphanTest = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "testPresets": [
        { "name": "core-debug", "configurePreset": "ghost-preset" }
      ]
    })";

    const auto reportTest = CMakePresetsValidator::validatePresetsJson(jsonOrphanTest);
    CHECK(!reportTest.isValid());
    CHECK(reportTest.outcome == CMakePresetValidationOutcome::OrphanConfigurePresetReference);
}

// TEST-PRESET-006: Nombre duplicado de presets
void test_presets_duplicate_name() {
    const std::string jsonDuplicateConfigure = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug-2" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportCfg = CMakePresetsValidator::validatePresetsJson(jsonDuplicateConfigure);
    CHECK(!reportCfg.isValid());
    CHECK(reportCfg.outcome == CMakePresetValidationOutcome::DuplicatePresetName);

    const std::string jsonDuplicateBuild = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "buildPresets": [
        { "name": "core-debug", "configurePreset": "core-debug" },
        { "name": "core-debug", "configurePreset": "core-debug" }
      ]
    })";

    const auto reportBld = CMakePresetsValidator::validatePresetsJson(jsonDuplicateBuild);
    CHECK(!reportBld.isValid());
    CHECK(reportBld.outcome == CMakePresetValidationOutcome::DuplicatePresetName);

    const std::string jsonDuplicateTest = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "testPresets": [
        { "name": "core-debug", "configurePreset": "core-debug" },
        { "name": "core-debug", "configurePreset": "core-debug" }
      ]
    })";

    const auto reportTst = CMakePresetsValidator::validatePresetsJson(jsonDuplicateTest);
    CHECK(!reportTst.isValid());
    CHECK(reportTst.outcome == CMakePresetValidationOutcome::DuplicatePresetName);
}

// TEST-PRESET-007 & TEST-PRESET-008: Rutas binarias no seguras
void test_presets_unsafe_binary_dir() {
    // TEST-PRESET-007: Directory traversal
    const std::string jsonTraversal = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/../../etc" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportTraversal = CMakePresetsValidator::validatePresetsJson(jsonTraversal);
    CHECK(!reportTraversal.isValid());
    CHECK(reportTraversal.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);

    // TEST-PRESET-008: Ruta absoluta POSIX fija
    const std::string jsonAbsPosix = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "/var/tmp/build" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportAbsPosix = CMakePresetsValidator::validatePresetsJson(jsonAbsPosix);
    CHECK(!reportAbsPosix.isValid());
    CHECK(reportAbsPosix.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);

    // Ruta absoluta Windows fija
    const std::string jsonAbsWin = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "C:\\build" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportAbsWin = CMakePresetsValidator::validatePresetsJson(jsonAbsWin);
    CHECK(!reportAbsWin.isValid());
    CHECK(reportAbsWin.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);

    // Ruta vacía
    const std::string jsonEmptyDir = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportEmpty = CMakePresetsValidator::validatePresetsJson(jsonEmptyDir);
    CHECK(!reportEmpty.isValid());
    CHECK(reportEmpty.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);
}

// TEST-PRESET-009: Variables de caché contradictorias
void test_presets_conflicting_variables() {
    const std::string jsonConflict = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-core-debug",
          "cacheVariables": {
            "VIGILIO_BUILD_GUI": "OFF",
            "VIGILIO_ENABLE_E2E": "ON"
          }
        },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto report = CMakePresetsValidator::validatePresetsJson(jsonConflict);
    CHECK(!report.isValid());
    CHECK(report.outcome == CMakePresetValidationOutcome::ConflictingVariables);

    // Versión con booleanos literales
    const std::string jsonConflictBool = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-core-debug",
          "cacheVariables": {
            "VIGILIO_BUILD_GUI": false,
            "VIGILIO_ENABLE_E2E": true
          }
        },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";

    const auto reportBool = CMakePresetsValidator::validatePresetsJson(jsonConflictBool);
    CHECK(!reportBool.isValid());
    CHECK(reportBool.outcome == CMakePresetValidationOutcome::ConflictingVariables);
}

// TEST-PRESET-010: JSON malformado o vacío
void test_presets_malformed_json() {
    // Vacío
    const auto reportEmpty = CMakePresetsValidator::validatePresetsJson("");
    CHECK(!reportEmpty.isValid());
    CHECK(reportEmpty.outcome == CMakePresetValidationOutcome::MalformedJson);

    // Sintaxis trunca
    const auto reportTruncated = CMakePresetsValidator::validatePresetsJson("{ \"version\": 6, ");
    CHECK(!reportTruncated.isValid());
    CHECK(reportTruncated.outcome == CMakePresetValidationOutcome::MalformedJson);

    // No es objeto (array en raíz)
    const auto reportArrayRoot = CMakePresetsValidator::validatePresetsJson("[ 1, 2, 3 ]");
    CHECK(!reportArrayRoot.isValid());
    CHECK(reportArrayRoot.outcome == CMakePresetValidationOutcome::MalformedJson);

    // Llaves no emparejadas
    const auto reportBroken =
        CMakePresetsValidator::validatePresetsJson("{ \"version\": 6 } extra garbage");
    CHECK(!reportBroken.isValid());
    CHECK(reportBroken.outcome == CMakePresetValidationOutcome::MalformedJson);
}

// TEST-PRESET-011: Validación del archivo real del proyecto
void test_presets_project_file_validation() {
#if defined(VIGILIO_CMAKE_PRESETS_PATH)
    const std::filesystem::path presetsPath(VIGILIO_CMAKE_PRESETS_PATH);
#elif defined(VIGILIO_PRESETS_JSON_PATH)
    const std::filesystem::path presetsPath(VIGILIO_PRESETS_JSON_PATH);
#else
    const std::filesystem::path presetsPath("CMakePresets.json");
#endif
    if (std::filesystem::exists(presetsPath)) {
        const auto report = CMakePresetsValidator::validatePresetsFile(presetsPath);
        CHECK(report.isValid());
        CHECK(report.outcome == CMakePresetValidationOutcome::Valid);
        CHECK(report.schemaVersion >= 3);
        CHECK(!report.configurePresets.empty());

        // Debe contener core-debug y desktop-debug
        bool hasCoreDebug = false;
        bool hasDesktopDebug = false;
        for (const auto& name : report.configurePresets) {
            if (name == "core-debug")
                hasCoreDebug = true;
            if (name == "desktop-debug")
                hasDesktopDebug = true;
        }
        CHECK(hasCoreDebug);
        CHECK(hasDesktopDebug);
    } else {
        std::cout << "Aviso: Archivo de presets no encontrado en " << presetsPath
                  << ", omitiendo test de lectura directa\n";
    }

    // Comprobar manejo de archivo inexistente
    const auto missingReport =
        CMakePresetsValidator::validatePresetsFile("/nonexistent/path/to/presets.json");
    CHECK(!missingReport.isValid());
    CHECK(missingReport.outcome == CMakePresetValidationOutcome::MalformedJson);
}

// TEST-PRESET-012: Prueba de rendimiento y consumo acotado
void test_presets_performance_benchmark() {
    const auto start = std::chrono::steady_clock::now();
    constexpr int ITERATIONS = 100;

    for (int i = 0; i < ITERATIONS; ++i) {
        const auto report = CMakePresetsValidator::validatePresetsJson(CANONICAL_VALID_JSON);
        CHECK(report.isValid());
    }

    const auto end = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Performance: " << ITERATIONS << " validations in " << elapsedMs << " ms\n";
    // Exigencia del diseño: 50 validaciones < 50 ms (para 100 iteraciones exigimos < 150 ms)
    CHECK(elapsedMs < 150);
}

// Pruebas complementarias de métodos auxiliares y tipos
void test_presets_helper_methods_and_types() {
    // isSafeBinaryDir
    CHECK(CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/build-core-debug"));
    CHECK(CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/_build-core"));
    CHECK(CMakePresetsValidator::isSafeBinaryDir("build-debug"));
    CHECK(CMakePresetsValidator::isSafeBinaryDir("_build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir(""));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("/tmp/build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("\\tmp\\build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("D:\\build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/../build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("some/other/path"));

    // canonical lists
    const auto req = CMakePresetsValidator::canonicalRequiredPresets();
    CHECK(req.size() == 2);
    CHECK(req[0] == "core-debug");
    CHECK(req[1] == "desktop-debug");

    const auto names = CMakePresetsValidator::canonicalPresetNames();
    CHECK(names == req);

    const auto all = CMakePresetsValidator::canonicalAllPresets();
    CHECK(all.size() == 9);

    // outcomeToString
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::Valid).empty());
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::InvalidSchemaVersion)
               .empty());
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::MissingRequiredPreset)
               .empty());
    CHECK(!CMakePresetsValidator::outcomeToString(
               CMakePresetValidationOutcome::OrphanConfigurePresetReference)
               .empty());
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::DuplicatePresetName)
               .empty());
    CHECK(
        !CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::UnsafeBinaryDir).empty());
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::ConflictingVariables)
               .empty());
    CHECK(!CMakePresetsValidator::outcomeToString(CMakePresetValidationOutcome::MalformedJson).empty());

    // Tipos de dominio
    CMakePresetDefinition def;
    def.kind = CMakePresetKind::Configure;
    def.name = "test-preset";
    def.generator = "Ninja";
    def.binaryDir = "${sourceDir}/build-test";
    CHECK(def.outputOnFailure);

    CMakePresetsValidationReport r;
    CHECK(r.isValid());
    r.outcome = CMakePresetValidationOutcome::MalformedJson;
    CHECK(!r.isValid());
}

// TEST-PRESET-013: Sintaxis JSON corrompida extrema y bytes nulos
void test_presets_corrupted_json_syntax_variations() {
    // 1. Byte nulo binario embebido
    std::string nullByteJson = "{\n  \"version\": 6,\n  \"configurePresets\": [{\n    \"name\": \"core";
    nullByteJson.push_back('\0');
    nullByteJson += "-debug\",\n    \"generator\": \"Ninja\"\n  }]\n}";
    const auto rNull = CMakePresetsValidator::validatePresetsJson(nullByteJson);
    CHECK(!rNull.isValid());
    CHECK(rNull.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 2. Llaves desbalanceadas (falta cerrar)
    const std::string unbalancedBraces = "{\n  \"version\": 6,\n  \"configurePresets\": [{\n";
    const auto rBraces = CMakePresetsValidator::validatePresetsJson(unbalancedBraces);
    CHECK(!rBraces.isValid());
    CHECK(rBraces.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 3. Corchetes desbalanceados
    const std::string unbalancedBrackets = "{\n  \"version\": 6,\n  \"configurePresets\": [\n";
    const auto rBrackets = CMakePresetsValidator::validatePresetsJson(unbalancedBrackets);
    CHECK(!rBrackets.isValid());
    CHECK(rBrackets.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 4. Comillas no cerradas
    const std::string unclosedQuote =
        "{\n  \"version\": 6,\n  \"configurePresets\": [ { \"name: \"core-debug\" } ] }";
    const auto rQuote = CMakePresetsValidator::validatePresetsJson(unclosedQuote);
    CHECK(!rQuote.isValid());
    CHECK(rQuote.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 5. Array en el nivel raíz en lugar de objeto
    const std::string arrayAtRoot = "[ { \"version\": 6 } ]";
    const auto rArray = CMakePresetsValidator::validatePresetsJson(arrayAtRoot);
    CHECK(!rArray.isValid());
    CHECK(rArray.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 6. Primitivo o literal en la raíz
    const std::string primitiveAtRoot = "\"not an object\"";
    const auto rPrim = CMakePresetsValidator::validatePresetsJson(primitiveAtRoot);
    CHECK(!rPrim.isValid());
    CHECK(rPrim.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 7. Coma final no permitida (trailing comma)
    const std::string trailingComma = "{\n  \"version\": 6,\n}";
    const auto rComma = CMakePresetsValidator::validatePresetsJson(trailingComma);
    CHECK(!rComma.isValid());
    CHECK(rComma.outcome == CMakePresetValidationOutcome::MalformedJson);
}

// TEST-PRESET-014: Inversión de tipos estructurales en JSON
void test_presets_type_inversion_and_malformed_structures() {
    // 1. Versión de esquema como cadena ("6" en vez de 6)
    const std::string versionAsString = R"({
      "version": "6",
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ]
    })";
    const auto rVerStr = CMakePresetsValidator::validatePresetsJson(versionAsString);
    CHECK(!rVerStr.isValid());
    CHECK(rVerStr.outcome == CMakePresetValidationOutcome::InvalidSchemaVersion);

    // 2. configurePresets declarado como objeto asociativo en vez de array
    const std::string configureAsObject = R"({
      "version": 6,
      "configurePresets": {
        "core-debug": { "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" }
      }
    })";
    const auto rCfgObj = CMakePresetsValidator::validatePresetsJson(configureAsObject);
    CHECK(!rCfgObj.isValid());
    CHECK(rCfgObj.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 3. configurePresets con elemento que no es objeto (string o número)
    const std::string configureElementNotObject = R"({
      "version": 6,
      "configurePresets": [
        "core-debug",
        "desktop-debug"
      ]
    })";
    const auto rElemNotObj = CMakePresetsValidator::validatePresetsJson(configureElementNotObject);
    CHECK(!rElemNotObj.isValid());
    CHECK(rElemNotObj.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 4. cacheVariables declarado como array en lugar de objeto
    const std::string cacheVarsAsArray = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-core-debug",
          "cacheVariables": [ "CMAKE_BUILD_TYPE=Debug" ]
        },
        {
          "name": "desktop-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-desktop-debug"
        }
      ]
    })";
    const auto rCacheArr = CMakePresetsValidator::validatePresetsJson(cacheVarsAsArray);
    CHECK(!rCacheArr.isValid());
    CHECK(rCacheArr.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 5. buildPresets como objeto asociativo en vez de lista
    const std::string buildAsObject = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "buildPresets": {
        "core-debug": { "configurePreset": "core-debug" }
      }
    })";
    const auto rBuildObj = CMakePresetsValidator::validatePresetsJson(buildAsObject);
    CHECK(!rBuildObj.isValid());
    CHECK(rBuildObj.outcome == CMakePresetValidationOutcome::MalformedJson);

    // 6. testPresets como objeto asociativo en vez de lista
    const std::string testAsObject = R"({
      "version": 6,
      "configurePresets": [
        { "name": "core-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
        { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
      ],
      "testPresets": {
        "core-debug": { "configurePreset": "core-debug" }
      }
    })";
    const auto rTestObj = CMakePresetsValidator::validatePresetsJson(testAsObject);
    CHECK(!rTestObj.isValid());
    CHECK(rTestObj.outcome == CMakePresetValidationOutcome::MalformedJson);
}

// TEST-PRESET-015 y TEST-PRESET-016: Inyección de comandos shell y path traversal avanzado
void test_presets_security_injection_and_path_hygiene() {
    // 1. Nombres de preset hostiles con inyección de comandos shell
    const std::vector<std::string> hostileNames = {
        "; rm -rf /", "&& ls", "| cat", "$(calc)", "core-debug; reboot", "`id`", "test spaces"};

    for (const auto& hostile : hostileNames) {
        std::string hostileJson =
            R"({
          "version": 6,
          "configurePresets": [
            { "name": ")" +
            hostile +
            R"(", "generator": "Ninja", "binaryDir": "${sourceDir}/build-core-debug" },
            { "name": "desktop-debug", "generator": "Ninja", "binaryDir": "${sourceDir}/build-desktop-debug" }
          ]
        })";
        const auto rHostile = CMakePresetsValidator::validatePresetsJson(hostileJson);
        CHECK(!rHostile.isValid());
        CHECK(rHostile.outcome == CMakePresetValidationOutcome::MalformedJson);
    }

    // 2. Path traversal avanzado en isSafeBinaryDir
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/build/../../secret"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/build/subdir/../../../escape"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("${sourceDir}/../build-escape"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("\\\\server\\share\\build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("//network-host/build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("/etc/shadow"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("/var/log/build"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("C:\\Windows\\System32"));
    CHECK(!CMakePresetsValidator::isSafeBinaryDir("D:\\build-unsafe"));

    // 3. Documento JSON con binaryDir que contiene path traversal avanzado
    const std::string traversalJson = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build/../../secret"
        },
        {
          "name": "desktop-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-desktop-debug"
        }
      ]
    })";
    const auto rTraversal = CMakePresetsValidator::validatePresetsJson(traversalJson);
    CHECK(!rTraversal.isValid());
    CHECK(rTraversal.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);

    // 4. Documento JSON con binaryDir UNC de red
    const std::string uncJson = R"({
      "version": 6,
      "configurePresets": [
        {
          "name": "core-debug",
          "generator": "Ninja",
          "binaryDir": "\\\\server\\share\\build"
        },
        {
          "name": "desktop-debug",
          "generator": "Ninja",
          "binaryDir": "${sourceDir}/build-desktop-debug"
        }
      ]
    })";
    const auto rUnc = CMakePresetsValidator::validatePresetsJson(uncJson);
    CHECK(!rUnc.isValid());
    CHECK(rUnc.outcome == CMakePresetValidationOutcome::UnsafeBinaryDir);
}

} // namespace

int main() {
    try {
        test_presets_canonical_valid();
        test_presets_invalid_schema_version();
        test_presets_missing_required_preset();
        test_presets_orphan_configure_reference();
        test_presets_duplicate_name();
        test_presets_unsafe_binary_dir();
        test_presets_conflicting_variables();
        test_presets_malformed_json();
        test_presets_project_file_validation();
        test_presets_performance_benchmark();
        test_presets_helper_methods_and_types();
        test_presets_corrupted_json_syntax_variations();
        test_presets_type_inversion_and_malformed_structures();
        test_presets_security_injection_and_path_hygiene();

        std::cout << "CMake presets tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
