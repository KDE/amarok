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

#include "debug.h"
#include "sqlcollection.h"
#include "sqlmeta.h"

#include <QFile>

#include <KLocale>
#include <KSharedPtr>

using namespace Meta;

SqlCollectionLocation::SqlCollectionLocation( SqlCollection const *collection )
    : CollectionLocation()
    , m_collection( const_cast<SqlCollection*>( collection ) )
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
SqlCollectionLocation::remove( Meta::TrackPtr track )
{
    KSharedPtr<SqlTrack> sqlTrack = KSharedPtr<SqlTrack>::dynamicCast( track );
    if( sqlTrack && sqlTrack->inCollection() && sqlTrack->collection()->collectionId() == m_collection->collectionId() )
    {
        bool removed = QFile::remove( sqlTrack->playableUrl().path() );
        if( removed )
        {
            
            QString query = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" )
                                .arg( QString::number( sqlTrack->deviceid() ), m_collection->escape( sqlTrack->rpath() ) );
            QStringList res = m_collection->query( query );
            if( res.isEmpty() )
            {
                warning() << "Tried to remove a track from SqlCollection which is not in the collection";
            }
            else
            {
                int id = res[0].toInt();
                QString query = QString( "DELETE FROM tracks where id = %1;" ).arg( id );
                m_collection->query( query );
            }
        }
        return removed;
    }
    else
    {
        return false;
    }
}

void
SqlCollectionLocation::copyUrlsToCollection( const KUrl::List &sources )
{
    Q_UNUSED( sources );
    //TODO
    slotCopyOperationFinished();
}

#include "SqlCollectionLocation.moc"
