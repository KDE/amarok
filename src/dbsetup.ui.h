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
    configStack->raiseWidget( 0 );
#ifdef USE_MYSQL
    databaseEngine->insertItem( "MySQL", -1 );
    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql))
    {
        databaseEngine->setCurrentItem("MySQL");
        configStack->raiseWidget( 1 );
    }
#endif

#ifdef USE_POSTGRESQL
    databaseEngine->insertItem( "Postgresql", -1 );
    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::postgresql))
    {
        databaseEngine->setCurrentItem("Postgresql");
        configStack->raiseWidget( 2 );
    }
#endif
}

void DbSetup::databaseEngine_activated( int item )
{
    if( item == 0 )
        configStack->raiseWidget( 0 );

    // If built with MySQL support, the PostgreSQL config widget is #2
    // Without MySQL it's #1
#ifdef USE_MYSQL
    else if( item == 1 )
        configStack->raiseWidget( 1 );
    else if( item == 2 )
        configStack->raiseWidget( 2 );
#elif defined(USE_POSTGRESQL)
    else if( item == 1 )
        configStack->raiseWidget( 2 );
#endif
}
