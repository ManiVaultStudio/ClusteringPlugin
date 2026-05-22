cmake_minimum_required(VERSION 3.22)

option(MV_UNITY_BUILD "Combine target source files into batches for faster compilation" OFF)

if(WIN32 AND NOT DEFINED VCPKG_LIBRARY_LINKAGE)
    set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")
endif()

# -----------------------------------------------------------------------------
# Clustering Plugin
# -----------------------------------------------------------------------------
set(ClusteringPlugin "ClusteringPlugin")

PROJECT(${ClusteringPlugin}
        DESCRIPTION "Several cluster algorithms for ManiVault"
        LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /W3 /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)

find_package(ManiVault COMPONENTS Core PointData ClusterData CONFIG QUIET)

find_package(Armadillo CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(ensmallen CONFIG REQUIRED)
find_path(MLPACK_INCLUDE_DIRS "mlpack.hpp" REQUIRED)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    find_package(OpenMP)
else()
    foreach(_tgt armadillo ensmallen::ensmallen)
        if (TARGET ${_tgt})
            get_target_property(_libs ${_tgt} INTERFACE_LINK_LIBRARIES)
            if (_libs)
                message(STATUS "${_tgt}: _libs (before removal): ${_libs}")
                list(REMOVE_ITEM _libs "OpenMP::OpenMP_CXX" "OpenMP::OpenMP_C")
                message(STATUS "${_tgt}: _libs (after removal): ${_libs}")
                set_target_properties(${_tgt} PROPERTIES INTERFACE_LINK_LIBRARIES "${_libs}")
            endif()
        endif()
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(SourcesClusteringPlugin
    src/ClusteringPlugin.h
    src/ClusteringPlugin.cpp
    src/SettingsAction.h
    src/SettingsAction.cpp
)

set(PluginInfoClusteringPlugin
    PluginInfo.json
)

source_group(Plugin FILES ${SourcesClusteringPlugin})
source_group(Aux FILES ${PluginInfoClusteringPlugin})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${ClusteringPlugin} SHARED ${SourcesClusteringPlugin} ${PluginInfoClusteringPlugin})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${ClusteringPlugin} PRIVATE "${ManiVault_INCLUDE_DIR}")

target_include_directories(${ClusteringPlugin} PRIVATE "${MLPACK_INCLUDE_DIRS}")
    
# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${ClusteringPlugin} PRIVATE cxx_std_20)

set_target_properties(${ClusteringPlugin} PROPERTIES 
    AUTOMOC ON
    UNITY_BUILD ${MV_UNITY_BUILD}
)

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${ClusteringPlugin} PRIVATE Qt6::Widgets)
target_link_libraries(${ClusteringPlugin} PRIVATE Qt6::WebEngineWidgets)

target_link_libraries(${ClusteringPlugin} PRIVATE ManiVault::Core)
target_link_libraries(${ClusteringPlugin} PRIVATE ManiVault::PointData)
target_link_libraries(${ClusteringPlugin} PRIVATE ManiVault::ClusterData)

target_link_libraries(${ClusteringPlugin} PRIVATE 
    armadillo
    cereal::cereal
    ensmallen::ensmallen
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "${ClusteringPlugin}: DO NOT link against openmp")
    target_compile_options(${ClusteringPlugin} PRIVATE /openmp-)
elseif(OpenMP_CXX_FOUND)
    message(STATUS "${ClusteringPlugin}: Linking against openmp")
    target_link_libraries(${ClusteringPlugin} PRIVATE OpenMP::OpenMP_CXX)
endif()

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
install(TARGETS ${ClusteringPlugin}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
)

add_custom_command(TARGET ${ClusteringPlugin} POST_BUILD
	COMMAND "${CMAKE_COMMAND}"
	--install ${CMAKE_CURRENT_BINARY_DIR}
	--config $<CONFIGURATION>
	--prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

mv_handle_plugin_config(${ClusteringPlugin})

# -----------------------------------------------------------------------------
# Misc
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${ClusteringPlugin} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${ClusteringPlugin} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
