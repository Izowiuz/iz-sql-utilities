#include "IzSQLUtilities/SQLConnector.h"

Q_GLOBAL_STATIC(IzSQLUtilities::ConnectionPool, IzDbConnectionPoolInstance);

IzSQLUtilities::ConnectionPool* IzSQLUtilities::ConnectionPool::instance()
{
	return IzDbConnectionPoolInstance;
}
