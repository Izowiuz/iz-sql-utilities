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
find_package(Qt5 COMPONENTS Core Sql Qml Concurrent REQUIRED)

# if CMAKE_INSTALL_PREFIX is set to default value I override it
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	 # prefix path for windows platform
	if (${CMAKE_SYSTEM_NAME} MATCHES "^Windows$")
		set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../deploy/libraries/${PROJECT_NAME} CACHE PATH "CMAKE_INSTALL_PREFIX path" FORCE)
	endif()
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Qt's moc files
qt5_wrap_cpp(MOC_files
	"private/SQLDataLoader.h"
	"private/SQLErrorMessage.h"
	"include/IzSQLUtilities/SQLErrorInterpreterA2.h"
	"include/IzSQLUtilities/SQLProxyModel.h"
	"include/IzSQLUtilities/SQLdbc.h"
	"include/IzSQLUtilities/SQLTableModel.h"
	"include/IzSQLUtilities/SQLQueryBuilder.h"
	"include/IzSQLUtilities/SQLModel.h"
	"include/IzSQLUtilities/SQLFunctions.h"
	"include/IzSQLUtilities/SQLTableProxyModel.h"
)

# library definition
add_library(${PROJECT_NAME} SHARED
	"private/SQLQueryBuilder.cpp"
	"private/SQLOrderByElement.cpp"
	"private/SQLModel.cpp"
	"private/SQLJoinElement.cpp"
	"private/SQLFunctions.cpp"
	"private/SQLErrorMessage.cpp"
	"private/SQLErrorInterpreterA2.cpp"
	"private/SQLElement.cpp"
	"private/SQLDataLoader.cpp"
	"private/SQLDataContainer.cpp"
	"private/SQLData.cpp"
	"private/SQLProxyModel.cpp"
	"private/SQLdbc.cpp"
	"private/SQLWhereElement.cpp"
	"private/SQLTopElement.cpp"
	"private/SQLTableModel.cpp"
	"private/SQLTable.cpp"
	"private/SQLSelectElement.cpp"
	"private/SQLResponseEvent.cpp"
	"private/LoadedSQLData.cpp"
	"private/SQLTableProxyModel.cpp"
	${MOC_files}
)

# public headers
set(PUBLIC_HEADERS
	"include/IzSQLUtilities/SQLErrorInterpreterA2.h"
	"include/IzSQLUtilities/SQLDataContainer.h"
	"include/IzSQLUtilities/SQLFunctions.h"
	"include/IzSQLUtilities/SQLModel.h"
	"include/IzSQLUtilities/SQLProxyModel.h"
	"include/IzSQLUtilities/SQLQueryBuilder.h"
	"include/IzSQLUtilities/SQLTableModel.h"
	"include/IzSQLUtilities/IzSQLUtilities_Enums.h"
	"include/IzSQLUtilities/IzSQLUtilities_Global.h"
	"include/IzSQLUtilities/SQLdbc.h"
	"include/IzSQLUtilities/SQLTableProxyModel.h"
)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

# include directories for target IzLogger
target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/private
)

# target compile features
target_compile_features(${PROJECT_NAME}
	PRIVATE
		cxx_std_17
)

# target links
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Qt5::Core
	PRIVATE
		Qt5::Qml
		Qt5::Sql
		Qt5::Concurrent
)

# target compile definitions
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		IZSQLUTILITIES_LIBRARY
		QT_USE_QSTRINGBUILDER
)

# installs
include(GNUInstallDirs)
install (TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-export
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${CMAKE_BUILD_TYPE}/${Qt5Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_BUILD_TYPE}/${Qt5Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_BUILD_TYPE}/${Qt5Core_VERSION}/${CMAKE_CXX_COMPILER_ID}/${PLATFORM_TARGET}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)
install(FILES ${PROJECT_NAME}Config.cmake
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# export target
install(EXPORT ${PROJECT_NAME}-export
	FILE
		${PROJECT_NAME}Targets.cmake
	NAMESPACE
		IzSQLUtilities::
	DESTINATION cmake/${PROJECT_NAME}
)
