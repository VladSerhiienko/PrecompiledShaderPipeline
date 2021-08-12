cmake_minimum_required(VERSION 3.4.1)

include(ExternalProject)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
project (PrecompiledShaderPipeline CXX)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")

#
#
# platform decisions
#
#

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -DNV_EXTENSIONS=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DNV_EXTENSIONS=1")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("/std:c++latest" COMPILER_SUPPORTS_CXXLATEST)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if (COMPILER_SUPPORTS_CXXLATEST)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
elseif(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

get_filename_component(BUILD_FOLDER_SUFFIX ${CMAKE_BINARY_DIR} NAME)
message(STATUS "BUILD_FOLDER_SUFFIX = ${BUILD_FOLDER_SUFFIX}")

if (WIN32)
    set(default_cmake_args -G "${CMAKE_GENERATOR}")
elseif(APPLE)
    set(default_cmake_args -G "Xcode") # -B${BUILD_FOLDER_SUFFIX} -H.)
else()
    set(default_cmake_args -B${BUILD_FOLDER_SUFFIX} -H.)
endif()

#
#
# cxxopts
#
#

ExternalProject_Add(
    cxxopts
    GIT_REPOSITORY "git@github.com:jarro2783/cxxopts.git"
    GIT_TAG "1_0"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/cxxopts"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON)

ExternalProject_Get_Property(cxxopts SOURCE_DIR)
set(cxxopts_source_dir ${SOURCE_DIR})
message(STATUS "cxxopts_source_dir = ${cxxopts_source_dir}")

#
#
# spdlog
#
#

ExternalProject_Add(
    spdlog
    GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/spdlog"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(spdlog SOURCE_DIR)
set(spdlog_source_dir ${SOURCE_DIR})
message(STATUS "spdlog_source_dir = ${spdlog_source_dir}")

#
#
# eastl
# via submodules
#


#
#
# googletest
#
#

ExternalProject_Add(
    googletest
    GIT_REPOSITORY "git@github.com:google/googletest.git"
    GIT_TAG "release-1.10.0"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/googletest"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS ${default_cmake_args} -Dgtest_force_shared_crt:BOOL=ON
    TEST_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(googletest SOURCE_DIR)
ExternalProject_Get_Property(googletest BINARY_DIR)
set(googletest_source_dir ${SOURCE_DIR})
set(googletest_binary_dir ${BINARY_DIR})
message(STATUS "googletest_source_dir = ${googletest_source_dir}")
message(STATUS "googletest_binary_dir = ${googletest_binary_dir}")

#
#
# flatbuffers
#
#

ExternalProject_Add(
    flatbuffers
    GIT_REPOSITORY "git@github.com:google/flatbuffers.git"
    GIT_TAG "v1.11.0"
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/flatbuffers"
    CMAKE_ARGS ${default_cmake_args}
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(flatbuffers SOURCE_DIR)
ExternalProject_Get_Property(flatbuffers BINARY_DIR)
set(flatbuffers_source_dir ${SOURCE_DIR})
set(flatbuffers_binary_dir ${BINARY_DIR})
message(STATUS "flatbuffers_source_dir = ${flatbuffers_source_dir}")
message(STATUS "flatbuffers_binary_dir = ${flatbuffers_binary_dir}")

#
#
# json
#
#

ExternalProject_Add(
    json
    GIT_REPOSITORY "git@github.com:nlohmann/json.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/json"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(json SOURCE_DIR)
set(json_source_dir ${SOURCE_DIR})
message(STATUS "json_source_dir = ${json_source_dir}")

#
#
# taskflow
#
#

ExternalProject_Add(
    taskflow
    GIT_REPOSITORY "git@github.com:cpp-taskflow/cpp-taskflow.git"
    GIT_TAG "master"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(taskflow SOURCE_DIR)
set(taskflow_source_dir ${SOURCE_DIR})
message(STATUS "taskflow_source_dir = ${taskflow_source_dir}")

#
#
# shaderc
#
#

ExternalProject_Add(
    shaderc
    GIT_REPOSITORY "git@github.com:google/shaderc.git"
    GIT_TAG "v2019.1"
    UPDATE_COMMAND ""
    PATCH_COMMAND python "${CMAKE_SOURCE_DIR}/dependencies/shaderc/utils/git-sync-deps"
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/shaderc"
    CMAKE_ARGS ${default_cmake_args} -DSHADERC_SKIP_TESTS:BOOL=TRUE -DSHADERC_SKIP_INSTALL:BOOL=TRUE -DSHADERC_ENABLE_SHARED_CRT:BOOL=TRUE -DSHADERC_ENABLE_SPVC:BOOL=TRUE
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(shaderc SOURCE_DIR)
ExternalProject_Get_Property(shaderc BINARY_DIR)
set(shaderc_source_dir ${SOURCE_DIR})
set(shaderc_binary_dir ${BINARY_DIR})
message(STATUS "shaderc_source_dir = ${shaderc_source_dir}")
message(STATUS "shaderc_binary_dir = ${shaderc_binary_dir}")

set(Shaderc_IncludeDir ${shaderc_source_dir}/libshaderc/include)
set(Shaderc_DebugCombinedLibrary ${shaderc_binary_dir}/libshaderc/Debug/libshaderc_combined.a)
set(Shaderc_ReleaseCombinedLibrary ${shaderc_binary_dir}/libshaderc/Release/libshaderc_combined.a)

message(STATUS "Shaderc_IncludeDir = ${Shaderc_IncludeDir}")
message(STATUS "Shaderc_DebugCombinedLibrary = ${Shaderc_DebugCombinedLibrary}")
message(STATUS "Shaderc_ReleaseCombinedLibrary = ${Shaderc_ReleaseCombinedLibrary}")

if (UNIX AND NOT APPLE)
add_definitions("-rdynamic")
endif()

if (UNIX AND APPLE)
add_definitions("-Wno-invalid-offsetof -Wno-deprecated-declarations -fno-omit-frame-pointer")
endif()

# add_definitions("-DAPEMODE_NO_GLOBAL_NEW_DELETE_OP_OVERRIDES")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    # using Clang
    # add_definitions("-stdlib=libc++")
    add_definitions("-std=c++17")
    add_definitions("-g")
    add_definitions( "-DHAVE_PTHREAD" )
    set( CONFIGURATION_SUFFIX "" )

    if (UNIX AND NOT APPLE)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_XLIB_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_XLIB_KHR" )
    elseif (WIN32)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_WIN32_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_WIN32_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")

    # using AppleClang
    add_definitions( "-g" )
    add_definitions("-std=c++17")
    add_definitions( "-DHAVE_PTHREAD" )
    add_definitions( "-D_XOPEN_SOURCE=600" )
    add_definitions( "-D_DARWIN_C_SOURCE=600" )
    set( CONFIGURATION_SUFFIX "/$CONFIGURATION" )

    if (UNIX AND APPLE)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_MACOS_MVK" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_MACOS_MVK" )
    elseif( DEFINED IOS_PLATFORM )
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_IOS_MVK" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_IOS_MVK" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    # using GCC
    add_definitions( "-g" )
    add_definitions( "-DHAVE_PTHREAD" )
    set( CONFIGURATION_SUFFIX "" )

    if (UNIX)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_XLIB_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_XLIB_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

# elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    # using Microsoft Visual C++
    set( CONFIGURATION_SUFFIX "/$(Configuration)" )
    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS" )

    if (WIN32)
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVK_USE_PLATFORM_WIN32_KHR" )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DVK_USE_PLATFORM_WIN32_KHR" )
    else()
        message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
    endif()

else()
    message( FATAL_ERROR "Unsupported configuration for ${CMAKE_CXX_COMPILER_ID} compiler." )
endif()

#
#
# Shaderc
#
#

add_library(
    ShaderCompilerLibrary
    STATIC
    ${CMAKE_SOURCE_DIR}/src/shaderc/ShaderCompiler.h
    ${CMAKE_SOURCE_DIR}/src/shaderc/ShaderCompiler.cpp
    )

target_include_directories(
    ShaderCompilerLibrary
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shaderc
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow
    ${CMAKE_SOURCE_DIR}/dependencies/shaderc/libshaderc/include
    ${CMAKE_SOURCE_DIR}/dependencies/shaderc/third_party/spirv-cross
    ${CMAKE_SOURCE_DIR}/dependencies/shaderc/third_party/spirv-cross/include
    )

add_dependencies(
    ShaderCompilerLibrary
    shaderc
    cxxopts
    spdlog
    taskflow
    )

add_library(
    PrecompiledShaderPipelineLibrary
    STATIC
    ${CMAKE_SOURCE_DIR}/src/shaderc/ShaderCompilerApp.cpp
    # ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.cpp
    # ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared/AssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/AppState.h
    # ${CMAKE_SOURCE_DIR}/src/apemode/platform/IAssetManager.h
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.cpp
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/CityHash.h
    )

target_include_directories(
    PrecompiledShaderPipelineLibrary
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include/mutable_generated
    ${CMAKE_SOURCE_DIR}/src/shaderc
    ${CMAKE_SOURCE_DIR}/dependencies/cxxopts/include
    ${CMAKE_SOURCE_DIR}/dependencies/spdlog/include
    ${CMAKE_SOURCE_DIR}/dependencies/cpp-taskflow
    ${CMAKE_SOURCE_DIR}/dependencies/json/include
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    )

add_dependencies(
    PrecompiledShaderPipelineLibrary
    ShaderCompilerLibrary
    shaderc
    json
    flatbuffers
    cxxopts
    spdlog
    taskflow
)

add_executable(
    PrecompiledShaderPipeline
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    )

target_include_directories(
    PrecompiledShaderPipeline
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    )

add_dependencies(
    PrecompiledShaderPipeline
    PrecompiledShaderPipelineLibrary
    ShaderCompilerLibrary
    shaderc
    json
    flatbuffers
    cxxopts
    spdlog
    taskflow
)

add_executable(
    PrecompiledShaderPipelineTests
    ${CMAKE_SOURCE_DIR}/include/utils/PrecompiledShaderPipelineUtils.h
    ${CMAKE_SOURCE_DIR}/tests/src/test_main.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_embedded.cpp
    )

target_include_directories(
    PrecompiledShaderPipelineTests
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests/src
    ${CMAKE_SOURCE_DIR}/include/utils
    ${CMAKE_SOURCE_DIR}/include/mutable_generated
    ${CMAKE_SOURCE_DIR}/tests/assets/shaders
    ${CMAKE_SOURCE_DIR}/src/apemode
    ${CMAKE_SOURCE_DIR}/src/apemode/platform
    ${CMAKE_SOURCE_DIR}/src/apemode/platform/shared
    ${CMAKE_SOURCE_DIR}/dependencies/flatbuffers/include
    ${CMAKE_SOURCE_DIR}/dependencies/googletest/googlemock/include
    ${CMAKE_SOURCE_DIR}/dependencies/googletest/googletest/include
    )

add_dependencies(
    PrecompiledShaderPipelineTests
    PrecompiledShaderPipelineLibrary
    ShaderCompilerLibrary
    shaderc
    json
    flatbuffers
    googletest
    cxxopts
    spdlog
    taskflow
)

if (APPLE)
    set(PrecompiledShaderPipelineLibraries

        debug ${CMAKE_BINARY_DIR}/Debug/libShaderCompilerLibrary.a
        debug ${CMAKE_BINARY_DIR}/Debug/libPrecompiledShaderPipelineLibrary.a
        debug ${flatbuffers_binary_dir}/Debug/libflatbuffers.a
        debug ${shaderc_binary_dir}/libshaderc/Debug/libshaderc_combined.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-util.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-glsl.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-reflect.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-c.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-cpp.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-core.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-msl.a
        debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/libspirv-cross-hlsl.a

        optimized ${CMAKE_BINARY_DIR}/Release/libShaderCompilerLibrary.a
        optimized ${CMAKE_BINARY_DIR}/Release/libPrecompiledShaderPipelineLibrary.a
        optimized ${flatbuffers_binary_dir}/Release/libflatbuffers.a
        optimized ${shaderc_binary_dir}/libshaderc/Release/libshaderc_combined.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-util.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-glsl.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-reflect.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-c.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-cpp.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-core.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-msl.a
        optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/libspirv-cross-hlsl.a
        
        dl
        stdc++
        objc
        iconv
        )

    set(PrecompiledShaderPipelineTestsLibraries

        debug ${googletest_binary_dir}/lib/Debug/libgmockd.a
        debug ${googletest_binary_dir}/lib/Debug/libgtestd.a
        debug ${googletest_binary_dir}/lib/Debug/libgtest_maind.a

        optimized ${googletest_binary_dir}/lib/Release/libgmock.a
        optimized ${googletest_binary_dir}/lib/Release/libgtest.a
        optimized ${googletest_binary_dir}/lib/Release/libgtest_main.a

        dl
        stdc++
        objc
        iconv
        )
    
    target_link_libraries(
        PrecompiledShaderPipeline
        ${PrecompiledShaderPipelineLibraries}
        )

    target_link_libraries(
        PrecompiledShaderPipelineTests
        ${PrecompiledShaderPipelineLibraries}
        ${PrecompiledShaderPipelineTestsLibraries}
        )

elseif (WIN32)
set(PrecompiledShaderPipelineLibraries

    debug ${CMAKE_BINARY_DIR}/Debug/ShaderCompilerLibrary.lib
    debug ${CMAKE_BINARY_DIR}/Debug/PrecompiledShaderPipelineLibrary.lib
    debug ${flatbuffers_binary_dir}/Debug/flatbuffers.lib
    debug ${shaderc_binary_dir}/libshaderc/Debug/shaderc_combined.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-utild.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-glsld.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-reflectd.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-cd.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-cppd.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-cored.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-msld.lib
    debug ${shaderc_binary_dir}/third_party/spirv-cross/Debug/spirv-cross-hlsld.lib

    optimized ${CMAKE_BINARY_DIR}/Release/ShaderCompilerLibrary.lib
    optimized ${CMAKE_BINARY_DIR}/Release/PrecompiledShaderPipelineLibrary.lib
    optimized ${flatbuffers_binary_dir}/Release/flatbuffers.lib
    optimized ${shaderc_binary_dir}/libshaderc/Release/shaderc_combined.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-util.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-glsl.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-reflect.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-c.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-cpp.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-core.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-msl.lib
    optimized ${shaderc_binary_dir}/third_party/spirv-cross/Release/spirv-cross-hlsl.lib
    )

set(PrecompiledShaderPipelineTestsLibraries

    debug ${googletest_binary_dir}/lib/Debug/gmockd.lib
    debug ${googletest_binary_dir}/lib/Debug/gtestd.lib
    debug ${googletest_binary_dir}/lib/Debug/gtest_maind.lib

    optimized ${googletest_binary_dir}/lib/Release/gmock.lib
    optimized ${googletest_binary_dir}/lib/Release/gtest.lib
    optimized ${googletest_binary_dir}/lib/Release/gtest_main.lib
    )

target_link_libraries(
    PrecompiledShaderPipeline
    ${PrecompiledShaderPipelineLibraries}
    )

target_link_libraries(
    PrecompiledShaderPipelineTests
    ${PrecompiledShaderPipelineLibraries}
    ${PrecompiledShaderPipelineTestsLibraries}
    )

elseif (UNIX)
    target_link_libraries(
        PrecompiledShaderPipeline
        ${shaderc_binary_dir}/libshaderc${CONFIGURATION_SUFFIX}/libshaderc_combined.a
        ${flatbuffers_binary_dir}${CONFIGURATION_SUFFIX}/libflatbuffers.a
        pthread
        stdc++fs
        dl
        )
endif ()

set_target_properties(
    PrecompiledShaderPipeline
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_target_properties(
    PrecompiledShaderPipelineTests
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    "$(OutDir)"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")
