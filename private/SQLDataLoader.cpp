#include "SQLDataLoader.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QMetaProperty>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>
#include <QUuid>

#include "IzSQLUtilities/SQLDataContainer.h"
#include "IzSQLUtilities/SQLErrorInterpreterA2.h"
#include "IzSQLUtilities/SQLdbc.h"
#include "SQLData.h"

IzSQLUtilities::SQLDataLoader::SQLDataLoader(QObject* parent)
	: QObject(parent)
{
}

IzSQLUtilities::SQLDataLoader::SQLDataLoader(IzSQLUtilities::DatabaseType databaseType, const QVariantMap& databaseParameters, QObject* parent)
	: QObject(parent)
	, m_databaseType(databaseType)
	, m_databaseParameters(databaseParameters)
{
}

void IzSQLUtilities::SQLDataLoader::loadData(const QString& query, bool asynchronousLoad, bool partialRefresh, bool reportProgress, bool emitOnAbort, const QPair<QString, QMap<int, QVariant>>& elementsToRefresh)
{
	if (m_isLoadingData) {
		qCritical() << "Loader is currently loading data.";
		return;
	}
	m_query                   = query;
	m_partialRefresh          = partialRefresh;
	m_asynchronousLoad        = asynchronousLoad;
	m_emitOnAbort             = emitOnAbort;
	m_countRaportingRequested = true;
	m_abortRequested          = false;
	m_UUID                    = QUuid::createUuid().toString();
	//	m_countRaportingRequested = reportProgress;
	if (m_countRaportingRequested) {
		if (QCoreApplication::instance()->property("SQLDataLoader_raportingFrequency").canConvert<int>()) {
			m_countReportingFrequency = QCoreApplication::instance()->property("SQLDataLoader_raportingFrequency").toInt();
			qInfo() << "Reporting frequency is set to:" << m_countReportingFrequency;
		} else {
			qInfo() << "Reporting frequency left at default value:" << m_countReportingFrequency;
		}
	}
	if (m_asynchronousLoad) {
		m_loadFuture = std::async(std::launch::async, std::bind(&IzSQLUtilities::SQLDataLoader::doWork, this, elementsToRefresh));
	} else {
		doWork();
	}
}

bool IzSQLUtilities::SQLDataLoader::doWork(const QPair<QString, QMap<int, QVariant>>& elementsToRefresh)
{
	m_isLoadingData = true;
	SQLdbc db(QStringLiteral("model-refresh"), true, m_databaseType, m_databaseParameters);
	QSharedPointer<IzSQLUtilities::SQLData> sqlData = QSharedPointer<IzSQLUtilities::SQLData>(new IzSQLUtilities::SQLData());
	if (m_abortRequested) {
		qInfo() << "Processing abort request...";
		if (!m_restartOperation) {
			sqlData->setLoadStatus(ModelLoadStatus::ABORTED);
			emit workCompleted(sqlData);
		}
		return true;
	}
	if (db.initializeConnection()) {
		qInfo() << "Loader:" << m_UUID << "is starting work in" << (m_asynchronousLoad ? "asynchronous mode." : "synchronous mode.");
		QSqlQuery loadData(db.getConnection());
		loadData.setForwardOnly(true);
		loadData.prepare(m_query);
		if (loadData.exec()) {
			int rowCount     = 0;
			int columnsCount = loadData.record().count();
			m_isLoadingData  = true;
			QHash<QString, int> columnIndexMap;
			columnIndexMap.reserve(columnsCount);
			QHash<int, QString> indexColumnMap;
			indexColumnMap.reserve(columnsCount);
			QStringList columns;
			columns.reserve(columnsCount);
			for (int i = 0; i < columnsCount; i++) {
				columns.append(loadData.record().fieldName(i));
				columnIndexMap.insert(loadData.record().fieldName(i), i);
				indexColumnMap.insert(i, loadData.record().fieldName(i));
			}
			sqlData->setColumnIndexMap(columnIndexMap);
			sqlData->setIndexColumnMap(indexColumnMap);
			sqlData->setSQLColumnNames(columns);
			sqlData->setPartialRefresh(m_partialRefresh);
			if (m_countRaportingRequested) {
				emit loadedCount(0);
			}
			int identityIndex                    = columnIndexMap.value(elementsToRefresh.first);
			QMap<int, QVariant> elementsToRemove = elementsToRefresh.second;
			QMap<int, QVariant> elementsRefreshed;
			while (loadData.next() && !m_abortRequested) {
				QSharedPointer<SQLDataContainer> row = QSharedPointer<SQLDataContainer>(new SQLDataContainer(static_cast<unsigned int>(columnsCount)));
				row->setIsInitializing(true);
				for (int i = 0; i < columnsCount; ++i) {
					row->addField(loadData.value(i));
				}
				if (m_partialRefresh) {
					elementsToRemove.remove(elementsToRemove.key(row->fieldValue(identityIndex)));
					elementsRefreshed.insert(elementsToRefresh.second.key(row->fieldValue(identityIndex)), row->fieldValue(identityIndex));
				}
				row->setIsInitializing(false);
				sqlData->addRow(row);
				if (m_countRaportingRequested && (rowCount % m_countReportingFrequency == 0) && rowCount > 0) {
					emit loadedCount(rowCount);
				}
				rowCount++;
			}
			if (m_partialRefresh) {
				// TODO: tu detach'uje się QList'a; trzeba na to rzucic okiem
				sqlData->setRemovedElements(elementsToRemove.keys().toVector());
				sqlData->setRefreshedElements(QPair<QString, QMap<int, QVariant>>{ elementsToRefresh.first, elementsRefreshed });
			}
			if (m_countRaportingRequested) {
				emit loadedCount(rowCount);
			}
			/* disable loading state */
			m_isLoadingData = false;
			if (m_abortRequested) {
				qInfo() << "Processing abort request...";
				if (!m_restartOperation) {
					if (!m_emitOnAbort) {
						sqlData->clearData();
					}
					sqlData->setLoadStatus(ModelLoadStatus::ABORTED);
					emit workCompleted(sqlData);
				}
				return true;
			}
			if (rowCount == 0) {
				qInfo() << "Empty result set.";
				sqlData->setLoadStatus(ModelLoadStatus::QUERY_EMPTY);
				emit workCompleted(sqlData);
				return true;
			}
			qInfo() << "Loaded data. Emiting...";
			sqlData->setLoadStatus(ModelLoadStatus::LOADED);
			emit workCompleted(sqlData);
			return true;
		}
		sqlData->setSqlError(loadData.lastError());
		sqlData->setLoadStatus(ModelLoadStatus::QUERY_ERROR);
		sendSQLError(loadData.lastError());
		emit workCompleted(sqlData);
	} else {
		sqlData->setSqlError(db.lastError());
		sqlData->setLoadStatus(ModelLoadStatus::SQL_ERROR);
		sendSQLError(db.lastError());
		emit workCompleted(sqlData);
	}
	return false;
}

void IzSQLUtilities::SQLDataLoader::sendSQLError(const QSqlError& error)
{
	SQLErrorInterpreterA2::instance()->sqlResponse(SQLResponseSeverity::SQL_RESPONSE_ERROR, error);
}

QVariantMap IzSQLUtilities::SQLDataLoader::databaseParameters() const
{
	return m_databaseParameters;
}

void IzSQLUtilities::SQLDataLoader::setDatabaseParameters(const QVariantMap& databaseParameters)
{
	m_databaseParameters = databaseParameters;
}

IzSQLUtilities::DatabaseType IzSQLUtilities::SQLDataLoader::databaseType() const
{
	return m_databaseType;
}

void IzSQLUtilities::SQLDataLoader::setDatabaseType(IzSQLUtilities::DatabaseType databaseType)
{
	m_databaseType = databaseType;
}

bool IzSQLUtilities::SQLDataLoader::isLoadingData() const
{
	return m_isLoadingData;
}

void IzSQLUtilities::SQLDataLoader::abortOperation()
{
	m_abortRequested = true;
	qInfo() << "Abort signal intercepted.";
}

void IzSQLUtilities::SQLDataLoader::restartOperation(const QString& newQuery)
{
	if (m_isLoadingData) {
		m_abortRequested = true;
		qInfo() << "Restart operation signal intercepted.";
	}
	m_query            = newQuery;
	m_restartOperation = true;
}
