cmake_minimum_required(VERSION 3.12)

project(PerlinNoiseSC LANGUAGES CXX)

option(SCSYNTH "Build the scsynth plugin target" ON)
option(SUPERNOVA "Build the supernova plugin target" OFF)
option(STRICT "Enable strict compiler warnings" OFF)
option(NATIVE "Enable -march=native where supported" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/stage" CACHE PATH "Install prefix" FORCE)
endif()

if(NOT SC_PATH)
    message(FATAL_ERROR "Set SC_PATH to a SuperCollider source checkout containing tools/cmake_gen.")
endif()

get_filename_component(SC_PATH_FOR_MODULES "${SC_PATH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${SC_PATH_FOR_MODULES}/tools/cmake_gen")

include(SuperColliderServerPlugin)
sc_do_initial_compiler_config()
sc_check_sc_path("${SC_PATH}")

set(perlinnoise_cpp_files
    src/PerlinNoise.cpp
)

set(perlinnoise_sc_files
    Classes/PerlinNoise.sc
    Classes/PerlinNoise2D.sc
    Classes/PerlinNoise3D.sc
)

set(perlinnoise_schelp_files
    HelpSource/Classes/PerlinNoise.schelp
    HelpSource/Classes/PerlinNoise2D.schelp
    HelpSource/Classes/PerlinNoise3D.schelp
)

sc_add_server_plugin(
    "PerlinNoise"
    "PerlinNoise"
    "${perlinnoise_cpp_files}"
    "${perlinnoise_sc_files}"
    "${perlinnoise_schelp_files}"
)

if(TARGET PerlinNoise_scsynth)
    set_target_properties(PerlinNoise_scsynth PROPERTIES OUTPUT_NAME "PerlinNoise")
endif()

if(TARGET PerlinNoise_supernova)
    set_target_properties(PerlinNoise_supernova PROPERTIES OUTPUT_NAME "PerlinNoise_supernova")
endif()

install(FILES HelpSource/Guides/PerlinNoisePorts.schelp DESTINATION PerlinNoise/HelpSource/Guides)

include(CTest)
if(BUILD_TESTING)
    add_executable(perlinnoise_tests tests/dsp_tests.cpp)
    target_include_directories(perlinnoise_tests PRIVATE
        "${SC_PATH}/include/plugin_interface"
        "${SC_PATH}/include/common"
        "${SC_PATH}/common"
    )
    add_test(NAME perlinnoise_dsp COMMAND perlinnoise_tests)
endif()
