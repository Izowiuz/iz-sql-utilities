#ifndef IZSQLUTILITIES_GLOBAL_H
#define IZSQLUTILITIES_GLOBAL_H

/*
	IzSQLUtilities

	A set of tools for SQL database oriented Qt programs

	Some global setttings can be stored as dynamic properties inside QCoreApplication class.
*/

#include <QtCore/qglobal.h>

#if defined(IZSQLUTILITIES_LIBRARY)
#define IZSQLUTILITIESSHARED_EXPORT Q_DECL_EXPORT
#else
#define IZSQLUTILITIESSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif   // IZSQLUTILITIES_GLOBAL_H
