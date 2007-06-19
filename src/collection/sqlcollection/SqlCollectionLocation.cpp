/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "SqlCollectionLocation.h"

#include "sqlmeta.h"

#include <QFile>

#include <KLocale.h>
#include <KSharedPtr.h>

SqlCollectionLocation::SqlCollectionLocation( SqlCollection *collection )
    : CollectionLocation()
    , m_collection( collection )
{
    //nothing to do
}

SqlCollectionLocation::~SqlCollectionLocation()
{
    //nothing to do
}

QString
SqlCollectionLocation::prettyLocation() const
{
    return i18n( "Local Collection" );
}

bool
SqlCollectionLocation::isWriteable() const
{
    return true;
}

bool
SqlCollectionLocation::remove( Meta::Track track )
{
    KSharedPtr<SqlTrack> sqlTrack = KSharedPtr<SqlTrack>::dynamicCast( track );
    if( sqlTrack && sqlTrack->inCollection() && sqlTrack->collection()->collectionId() == m_collection->collectionId() )
    {
        bool removed = QFile::remove( sqlTrack->playableUrl().path() );
        if( removed )
        {
            QString query = QString( "DELETE FROM tags WHERE deviceid = %1 AND url = '%2';" )
                                .arg( QString::number( sqlTrack->deviceid() ), m_collection->escape( sqlTrack->rpath() ) );
            m_collection->query( query );
        }
        return removed;
    }
    else
    {
        return false;
    }
}

void
SqlCollectionLocation::copyUrlsToLocation( const KUrl::List &sources )
{
    //TODO
    slotCopyOperationFinished();
}