#//(c) 2005 Ian Monroe see COPYING
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include "config.h"
#include "amarokconfig.h"
#include "collectiondb.h"

void DbSetup::init()
{
#ifdef USE_MYSQL
    kcfg_DatabaseEngine->insertItem( "MySQL", -1 );
#else 
    mysqlConfig->hide();
#endif
    int mysqlItem = kcfg_DatabaseEngine->count()-1;

#ifdef USE_POSTGRESQL
    kcfg_DatabaseEngine->insertItem( "Postgresql", -1 );
#else
    postgresqlConfig->hide();
#endif
    int postgresqlItem = kcfg_DatabaseEngine->count()-1;

    connect(kcfg_DatabaseEngine, SIGNAL(activated( int )), SLOT(databaseEngineChanged()) );
    mysqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql));
    postgresqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql));

    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql))
    {
        kcfg_DatabaseEngine->setCurrentItem(mysqlItem);
    }
    else if (AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql))
    {
        kcfg_DatabaseEngine->setCurrentItem(postgresqlItem);
    }
}

void DbSetup::databaseEngineChanged()
{
    if (kcfg_DatabaseEngine->currentText() == "SQLite") 
    {
        AmarokConfig::setDatabaseEngine(QString::number(DbConnection::sqlite));
    }
    else if (kcfg_DatabaseEngine->currentText() == "MySQL")
    {
        AmarokConfig::setDatabaseEngine(QString::number(DbConnection::mysql));
    }
    else if (kcfg_DatabaseEngine->currentText() == "Postgresql")
    {
        AmarokConfig::setDatabaseEngine(QString::number(DbConnection::postgresql));
    }

    mysqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql));
    postgresqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql));
}
