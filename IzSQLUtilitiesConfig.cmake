get_filename_component(IzSQLUtilities_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(Qt6 REQUIRED COMPONENTS Core Sql Qml Concurrent)
find_dependency(IzModels REQUIRED)

if(NOT TARGET IzSQLUtilities::IzSQLUtilities)
    include("${IzSQLUtilities_CMAKE_DIR}/cmake/IzSQLUtilities/IzSQLUtilitiesTargets.cmake")
endif()
