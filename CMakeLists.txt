cmake_minimum_required(VERSION 3.20)

project(binja_covex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(FetchContent)

option(BINJA_COVEX_USE_SYSTEM_QT "Use system Qt instead of qt-artifacts" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(BinjaQt)

if(NOT BINJA_COVEX_USE_SYSTEM_QT)
    binja_qt_configure()
endif()

set(BINJA_API_VERSION "" CACHE STRING "Binary Ninja API git revision (commit hash)")
if(NOT BINJA_API_VERSION)
    message(FATAL_ERROR "BINJA_API_VERSION is required. Set it to the commit hash in api_REVISION.txt from your Binary Ninja install.")
endif()

set(BN_API_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(binaryninjaapi
    GIT_REPOSITORY https://github.com/Vector35/binaryninja-api.git
    GIT_TAG        ${BINJA_API_VERSION}
    GIT_SUBMODULES_RECURSE ON
)

FetchContent_MakeAvailable(binaryninjaapi)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

add_library(covex_ui SHARED
    src/covex/ui/plugin/ui_plugin.cpp
    src/covex/ui/widgets/sidebar_widget.cpp
    src/covex/ui/theme/icon_loader.cpp
    src/covex/ui/settings/covex_settings.cpp
    src/covex/ui/controllers/workspace_controller.cpp
    src/covex/ui/painting/coverage_painter.cpp
    src/covex/ui/models/trace_table_model.cpp
    src/covex/ui/models/block_table_model.cpp
    src/covex/core/coverage_discovery.cpp
    src/covex/core/coverage_mapper.cpp
    src/covex/core/block_filter.cpp
    src/covex/core/module_matcher.cpp
    src/covex/coverage/coverage_expression.cpp
    src/covex/coverage/coverage_dataset.cpp
    src/covex/coverage/coverage_operations.cpp
    src/covex/coverage/coverage_store.cpp
    src/covex/coverage/coverage_parser.cpp
    src/covex/coverage/drcov_reader.cpp
    src/covex/coverage/addr_trace_reader.cpp
    resources/covex_icons.qrc
)

set_target_properties(covex_ui PROPERTIES
    AUTORCC ON
    AUTOMOC ON
    PREFIX ""
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(covex_ui
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/lib/third_party
)

target_link_libraries(covex_ui
    PRIVATE
        binaryninjaapi
        binaryninjaui
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
)

find_program(CLANG_FORMAT NAMES clang-format clang-format-17 clang-format-16)
if(CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
    )

    list(FILTER FORMAT_SOURCES EXCLUDE REGEX "/lib/third_party/")

    add_custom_target(binja_covex-format
        COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "formatting source files"
        VERBATIM
    )
endif()
