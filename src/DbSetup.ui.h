//(c) 2005 Ian Monroe see COPYING
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

void DbSetup::init()
{
    #ifdef USE_MYSQL
       kcfg_DatabaseEngine->insertItem( "MySQL", 1 );
    #endif
       connect(kcfg_DatabaseEngine, SIGNAL(activated( int )), SLOT(databaseEngineChanged()) );
}


void DbSetup::databaseEngineChanged()
{
    mysqlConfig->setEnabled(kcfg_DatabaseEngine->currentItem() != 0);
    
}
