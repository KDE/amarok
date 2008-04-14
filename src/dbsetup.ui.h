/***************************************************************************
 * copyright            : (C) 2005 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/
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
#include <config-amarok.h>  
#include "amarokconfig.h"

void DbSetup::init()
{
    configStack->raiseWidget( 0 );
#ifdef USE_MYSQL
    databaseEngine->addItem( "MySQL", -1 );
    if (AmarokConfig::databaseEngine() == QString::number(DbConnection::mysql))
    {
        databaseEngine->setCurrentItem("MySQL");
        configStack->raiseWidget( 1 );
    }
#endif

#ifdef USE_POSTGRESQL
    databaseEngine->addItem( "Postgresql", -1 );
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
