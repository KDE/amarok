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
    databaseEngine->insertItem( "MySQL", -1 );
    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql))
    {
        databaseEngine->setCurrentItem("MySQL");
    }
    mysqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql));
#else 
    mysqlConfig->hide();
#endif

#ifdef USE_POSTGRESQL
    databaseEngine->insertItem( "Postgresql", -1 );
    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql))
    {
        databaseEngine->setCurrentItem("Postgresql");
    }
    postgresqlConfig->setEnabled(AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql));
#else
    postgresqlConfig->hide();
#endif
   
    connect(databaseEngine, SIGNAL(activated( int )), SLOT(databaseEngineChanged()) );
}

void DbSetup::databaseEngineChanged()
{
    mysqlConfig->setEnabled(databaseEngine->currentText() == "MySQL");
    postgresqlConfig->setEnabled(databaseEngine->currentText() == "Postgresql");
}
