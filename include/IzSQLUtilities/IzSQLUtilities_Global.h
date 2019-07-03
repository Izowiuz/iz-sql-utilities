#ifndef IZSQLUTILITIES_GLOBAL_H
#define IZSQLUTILITIES_GLOBAL_H

/*
	IzSQLUtilities

	A set of tools for SQL database oriented Qt programs

	Global setttings are stored as dynamic properties inside QCoreApplication class.

	SQL database connection:
	SQLdbc_server [string]			- server
	SQLdbc_database [string]		- name
	SQLdbc_dbUserLogin [string]		- user login
	SQLdbc_dbUserPassword [string]	- user password
	SQLdbc_appUserLogin [string]	- application user, used to identify db connection
	SQLdbc_appUserID [int]			- application user ID
	SQLdbc_appName [string]			- application name

	Klasy do wyrzucenia:
		SQLData
		SQLDataContainer
		SQLModel
		SQLProxyModel

	Klasy do modernizacji / wyrzucenia:
		SQLQueryBuilder i wszystkie klasy związane
		SQLErrorInterpreterA2 i wszystkie klasy związane
*/

#include <QtCore/qglobal.h>

#if defined(IZSQLUTILITIES_LIBRARY)
#define IZSQLUTILITIESSHARED_EXPORT Q_DECL_EXPORT
#else
#define IZSQLUTILITIESSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif   // IZSQLUTILITIES_GLOBAL_H
