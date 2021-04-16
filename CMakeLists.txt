# cmake minumum version
cmake_minimum_required(VERSION 3.10)

# project name and version
project(IzSQLUtilities VERSION 1.0.0 LANGUAGES CXX)

# project architecture
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(PLATFORM_TARGET "x64")
else()
    set(PLATFORM_TARGET "x86")
endif()

# Qt's modules
find_package(Qt6 COMPONENTS Core Sql Qml Concurrent REQUIRED)

# enable autogeneration of moc files
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# IzModels
find_package(IzModels CONFIG REQUIRED)

# library definition
add_library(
    IzSQLUtilities
SHARED
    ""
)

# public headers
set(
PUBLIC_HEADERS
    "include/IzSQLUtilities/AbstractSQLModel.h"
    "include/IzSQLUtilities/SQLTableModel.h"
    "include/IzSQLUtilities/SQLTableProxyModel.h"
    "include/IzSQLUtilities/SQLListModel.h"
    "include/IzSQLUtilities/SQLFunctions.h"
    "include/IzSQLUtilities/SQLErrorEvent.h"
    "include/IzSQLUtilities/IzSQLUtilities_Enums.h"
    "include/IzSQLUtilities/IzSQLUtilities_Global.h"
    "include/IzSQLUtilities/SQLConnector.h"
    "include/IzSQLUtilities/SQLRow.h"
)

target_sources(
    IzSQLUtilities
PRIVATE
    "private/AbstractSQLModel.cpp"
    "private/SQLTableModel.cpp"
    "private/SQLTableProxyModel.cpp"
    "private/SQLListModel.cpp"
    "private/SQLFunctions.cpp"
    "private/LoadedSQLData.cpp"
    "private/LoadedSQLData.h"
    "private/SQLRow.cpp"
    ${PUBLIC_HEADERS}
)

set_target_properties(
    IzSQLUtilities
PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

# include directories for target
target_include_directories(IzSQLUtilities
PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# target compile features
target_compile_features(
    IzSQLUtilities
PRIVATE
    cxx_std_17
)

# target links
target_link_libraries(
    IzSQLUtilities
PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Sql
    Qt6::Concurrent
    IzModels::IzModels
)

# target compile definitions
target_compile_definitions(
    IzSQLUtilities
PRIVATE
    IZSQLUTILITIES_LIBRARY
    QT_USE_QSTRINGBUILDER
)

# installs
include(GNUInstallDirs)
install (
    TARGETS IzSQLUtilities
    EXPORT IzSQLUtilities-export
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${CMAKE_BUILD_TYPE}/${Qt6Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_BUILD_TYPE}/${Qt6Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_BUILD_TYPE}/${Qt6Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/IzSQLUtilities
)

install(
    FILES IzSQLUtilitiesConfig.cmake
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# export target
install(
EXPORT IzSQLUtilities-export
FILE
    IzSQLUtilitiesTargets.cmake
NAMESPACE
    IzSQLUtilities::
DESTINATION cmake/IzSQLUtilities
)
