/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "databasehandlerbase.h"

//#include "collectionmanager.h"
#include "collectiondb.h"

/*DatabaseHandlerBase* 
DatabaseHandlerBase::instance()
{
    if ( m_pInstance == 0 )
    {
        m_pInstance = new DatabaseHandlerBase();
    }
    return m_pInstance;
}*/

//DatabaseHandlerBase *DatabaseHandlerBase::m_pInstance = 0;

void 
DatabaseHandlerBase::begin( )
{

    //CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "BEGIN;";
    CollectionDB::instance()->query(  queryString );
}

void 
DatabaseHandlerBase::commit( )
{
    //CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "COMMIT;";
    CollectionDB::instance()->query( queryString );
}



