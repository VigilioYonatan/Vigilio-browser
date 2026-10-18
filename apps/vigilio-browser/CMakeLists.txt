cmake_minimum_required(VERSION 3.24)
project(VigilioBrowser VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
find_package(SQLite3 REQUIRED)

# ==============================================================================
# Capacidad: compiler warnings (etapa implementation)
# Target de interfaz centralizado para diagnósticos de compilación C++20
# ==============================================================================
add_library(vigilio_compiler_warnings INTERFACE)

set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wformat=2
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wextra-semi
)

set(GCC_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wformat=2
    -Wnull-dereference
    -Wdouble-promotion
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

set(MSVC_WARNINGS
    /W4
    /permissive-
    /w14242
    /w14254
    /w14263
    /w14265
    /w14287
    /we4289
    /w14296
    /w14311
    /w14545
    /w14546
    /w14547
    /w14549
    /w14555
    /w14619
    /w14640
    /w14826
    /w14905
    /w14906
    /w14928
    /EHsc
)

option(VIGILIO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(MSVC)
  set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS /WX)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS -Werror)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(PROJECT_WARNINGS ${GCC_WARNINGS})
  if(VIGILIO_WARNINGS_AS_ERRORS)
    list(APPEND PROJECT_WARNINGS -Werror)
  endif()
else()
  message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} not explicitly supported for strict warnings; falling back to basic warnings.")
  set(PROJECT_WARNINGS -Wall -Wextra)
endif()

target_compile_options(vigilio_compiler_warnings INTERFACE ${PROJECT_WARNINGS})

add_library(vigilio_core STATIC
  src/core/UrlInput.cpp
  src/core/TabModel.cpp
  src/core/SessionFile.cpp
  src/core/QtDependencyValidator.cpp
  src/core/WebEngineVersionValidator.cpp
  src/core/ResourceQrcCatalog.cpp
  src/core/ResourceInclusionService.cpp
  src/core/StartupBootstrapCoordinator.cpp
  src/core/ApplicationLifetimeCoordinator.cpp
  src/core/CrashRecoveryCoordinator.cpp
  src/core/CMakePresetsValidator.cpp
  src/core/CMakePresetsService.cpp
  src/core/TestFixtureValidator.cpp
  src/core/TestFixtureFoundation.cpp
  src/core/TestFixtureService.cpp
  src/core/ClangFormatValidator.cpp
  src/core/ClangFormatConfiguration.cpp
  src/core/ClangFormatService.cpp)
target_include_directories(vigilio_core PUBLIC src)
target_compile_features(vigilio_core PUBLIC cxx_std_20)
target_link_libraries(vigilio_core PRIVATE SQLite::SQLite3 vigilio_compiler_warnings)
target_compile_definitions(vigilio_core PRIVATE
  VIGILIO_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

add_library(vigilio_storage STATIC
  src/storage/LocalRepository.cpp)
target_include_directories(vigilio_storage PUBLIC src)
target_compile_features(vigilio_storage PUBLIC cxx_std_20)
target_link_libraries(vigilio_storage PUBLIC vigilio_core SQLite::SQLite3 PRIVATE vigilio_compiler_warnings)

if(BUILD_TESTING)
  add_executable(vigilio_core_tests tests/unit/CoreTests.cpp)
  target_link_libraries(vigilio_core_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME core_units COMMAND vigilio_core_tests)
  set_tests_properties(core_units PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_storage_tests tests/integration/StorageTests.cpp)
  target_link_libraries(vigilio_storage_tests PRIVATE vigilio_storage vigilio_compiler_warnings)
  add_test(NAME storage_integration COMMAND vigilio_storage_tests)
  set_tests_properties(storage_integration PROPERTIES LABELS "integration;fast" TIMEOUT 15)

  add_executable(vigilio_warnings_tests tests/unit/CompilerWarningsTests.cpp)
  target_link_libraries(vigilio_warnings_tests PRIVATE vigilio_compiler_warnings)
  target_compile_definitions(vigilio_warnings_tests PRIVATE
    VIGILIO_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
    VIGILIO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/warnings"
  )
  add_test(NAME vigilio_warnings_tests COMMAND vigilio_warnings_tests)
  set_tests_properties(vigilio_warnings_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_qt_validator_tests tests/unit/QtDependencyValidatorTests.cpp)
  target_link_libraries(vigilio_qt_validator_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME qt_dependency_validation_tests COMMAND vigilio_qt_validator_tests)
  set_tests_properties(qt_dependency_validation_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_webengine_validator_tests tests/unit/WebEngineVersionValidatorTests.cpp)
  target_link_libraries(vigilio_webengine_validator_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME webengine_version_validator_tests COMMAND vigilio_webengine_validator_tests)
  set_tests_properties(webengine_version_validator_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_resource_catalog_tests tests/unit/ResourceQrcCatalogTests.cpp)
  target_link_libraries(vigilio_resource_catalog_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_resource_catalog_tests PRIVATE
    VIGILIO_RESOURCES_QRC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc"
  )
  add_test(NAME resource_qrc_catalog_tests COMMAND vigilio_resource_catalog_tests)
  set_tests_properties(resource_qrc_catalog_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_resource_inclusion_tests tests/unit/ResourceInclusionServiceTests.cpp)
  target_link_libraries(vigilio_resource_inclusion_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_resource_inclusion_tests PRIVATE
    VIGILIO_RESOURCES_QRC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc"
    VIGILIO_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
  )
  add_test(NAME resource_inclusion_service_tests COMMAND vigilio_resource_inclusion_tests)
  set_tests_properties(resource_inclusion_service_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_resource_qrc_inclusion_tests tests/unit/ResourceQrcInclusionTests.cpp)
  target_link_libraries(vigilio_resource_qrc_inclusion_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_resource_qrc_inclusion_tests PRIVATE
    VIGILIO_RESOURCES_QRC_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc"
    VIGILIO_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
  )
  add_test(NAME resource_qrc_inclusion_tests COMMAND vigilio_resource_qrc_inclusion_tests)
  set_tests_properties(resource_qrc_inclusion_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_bootstrap_tests tests/unit/StartupBootstrapTests.cpp)
  target_link_libraries(vigilio_bootstrap_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME startup_bootstrap_tests COMMAND vigilio_bootstrap_tests)
  set_tests_properties(startup_bootstrap_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_lifetime_tests tests/unit/ApplicationLifetimeTests.cpp)
  target_link_libraries(vigilio_lifetime_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME application_lifetime_tests COMMAND vigilio_lifetime_tests)
  set_tests_properties(application_lifetime_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_crash_recovery_tests tests/unit/CrashRecoveryTests.cpp)
  target_link_libraries(vigilio_crash_recovery_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME crash_recovery_tests COMMAND vigilio_crash_recovery_tests)
  set_tests_properties(crash_recovery_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_presets_tests tests/unit/CMakePresetsTests.cpp)
  target_link_libraries(vigilio_presets_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_presets_tests PRIVATE
    VIGILIO_CMAKE_PRESETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/CMakePresets.json"
  )
  add_test(NAME cmake_presets_tests COMMAND vigilio_presets_tests)
  set_tests_properties(cmake_presets_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_presets_service_tests tests/unit/CMakePresetsServiceTests.cpp)
  target_link_libraries(vigilio_presets_service_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_presets_service_tests PRIVATE
    VIGILIO_CMAKE_PRESETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/CMakePresets.json"
  )
  add_test(NAME cmake_presets_service_tests COMMAND vigilio_presets_service_tests)
  set_tests_properties(cmake_presets_service_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_fixture_tests tests/unit/TestFixtureFoundationTests.cpp)
  target_link_libraries(vigilio_fixture_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  add_test(NAME test_fixture_foundation_tests COMMAND vigilio_fixture_tests)
  set_tests_properties(test_fixture_foundation_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_fixture_service_tests tests/unit/TestFixtureServiceTests.cpp)
  target_link_libraries(vigilio_fixture_service_tests PRIVATE vigilio_core SQLite::SQLite3 vigilio_compiler_warnings)
  add_test(NAME test_fixture_service_tests COMMAND vigilio_fixture_service_tests)
  set_tests_properties(test_fixture_service_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_clang_format_tests tests/unit/ClangFormatConfigurationTests.cpp)
  target_link_libraries(vigilio_clang_format_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_clang_format_tests PRIVATE
    VIGILIO_CLANG_FORMAT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/.clang-format"
  )
  add_test(NAME clang_format_configuration_tests COMMAND vigilio_clang_format_tests)
  set_tests_properties(clang_format_configuration_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)

  add_executable(vigilio_clang_format_service_tests tests/unit/ClangFormatServiceTests.cpp)
  target_link_libraries(vigilio_clang_format_service_tests PRIVATE vigilio_core vigilio_compiler_warnings)
  target_compile_definitions(vigilio_clang_format_service_tests PRIVATE
    VIGILIO_CLANG_FORMAT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/.clang-format"
  )
  add_test(NAME clang_format_service_tests COMMAND vigilio_clang_format_service_tests)
  set_tests_properties(clang_format_service_tests PROPERTIES LABELS "unit;fast" TIMEOUT 15)
endif()

option(VIGILIO_ENABLE_E2E "Enable resource-intensive native browser E2E smoke" OFF)
option(VIGILIO_BUILD_GUI "Build Qt 6 WebEngine desktop browser" ON)
option(VIGILIO_REQUIRE_QT "Require Qt 6.10+ dependencies strictly (fail build if missing)" OFF)

if(VIGILIO_BUILD_GUI)
  set(QT_MINIMUM_VERSION "6.10.0")
  find_package(Qt6 ${QT_MINIMUM_VERSION} QUIET COMPONENTS Core Widgets WebEngineWidgets)
  if(Qt6_FOUND)
    message(STATUS "Qt6 dependency validation: Satisfied (version ${Qt6_VERSION} found)")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    add_executable(vigilio-browser src/app.cpp src/ui/BrowserWindow.cpp resources.qrc)
    target_include_directories(vigilio-browser SYSTEM PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6WebEngineWidgets_INCLUDE_DIRS})
    target_link_libraries(vigilio-browser PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets)
    set_target_properties(vigilio-browser PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE)
    install(TARGETS vigilio-browser BUNDLE DESTINATION . RUNTIME DESTINATION bin)
    if(BUILD_TESTING)
      find_package(Qt6 QUIET COMPONENTS Test)
      if(TARGET Qt6::Test)
        add_executable(vigilio_ui_tests tests/ui/WindowTests.cpp src/ui/BrowserWindow.cpp resources.qrc)
        target_include_directories(vigilio_ui_tests SYSTEM PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6WebEngineWidgets_INCLUDE_DIRS} ${Qt6Test_INCLUDE_DIRS})
        target_link_libraries(vigilio_ui_tests PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test)
        add_test(NAME window_ui_tests COMMAND vigilio_ui_tests)
        set_tests_properties(window_ui_tests PROPERTIES LABELS "ui;fast" TIMEOUT 20)
      else()
        message(STATUS "Qt6 Test unavailable: vigilio_ui_tests omitted")
      endif()
      if(VIGILIO_ENABLE_E2E)
        if(TARGET Qt6::Test)
          add_executable(vigilio_e2e_smoke tests/e2e/BrowserSmoke.cpp src/ui/BrowserWindow.cpp resources.qrc)
          target_include_directories(vigilio_e2e_smoke SYSTEM PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6WebEngineWidgets_INCLUDE_DIRS} ${Qt6Test_INCLUDE_DIRS})
          target_link_libraries(vigilio_e2e_smoke PRIVATE vigilio_core vigilio_storage vigilio_compiler_warnings Qt6::Core Qt6::Widgets Qt6::WebEngineWidgets Qt6::Test)
          add_test(NAME browser_e2e_smoke COMMAND vigilio_e2e_smoke)
          set_tests_properties(browser_e2e_smoke PROPERTIES LABELS "e2e;heavy" TIMEOUT 60 RESOURCE_LOCK browser_e2e RUN_SERIAL TRUE)
        endif()
      endif()
    endif()
  else()
    set(QT_MISSING_DETAILS "")
    find_package(Qt6Core ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6Core_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::Core")
    endif()
    find_package(Qt6Widgets ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6Widgets_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::Widgets")
    endif()
    find_package(Qt6WebEngineWidgets ${QT_MINIMUM_VERSION} QUIET)
    if(NOT Qt6WebEngineWidgets_FOUND)
      string(APPEND QT_MISSING_DETAILS " Qt6::WebEngineWidgets")
    endif()

    if(VIGILIO_REQUIRE_QT)
      message(FATAL_ERROR "Qt dependency validation FAILED (VIGILIO_REQUIRE_QT=ON): Missing or incompatible components:${QT_MISSING_DETAILS}. Required version >= ${QT_MINIMUM_VERSION}.")
    else()
      message(WARNING "Qt dependency validation DEGRADED: Missing or incompatible components:${QT_MISSING_DETAILS}. Building headless core and tests only; desktop GUI NOT built.")
    endif()
  endif()
endif()
