get_filename_component(IzSQLUtilities_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(Qt5 REQUIRED COMPONENTS Core Sql Qml Concurrent)

if(NOT TARGET IzSQLUtilities::IzSQLUtilities)
    include("${IzSQLUtilities_CMAKE_DIR}/cmake/IzSQLUtilities/IzSQLUtilitiesTargets.cmake")
endif()
