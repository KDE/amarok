/*
    Copyright (C) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "SqlReadLabelCapability.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>

namespace Capabilities
{

SqlReadLabelCapability::SqlReadLabelCapability( Meta::SqlTrack *track, const QSharedPointer<SqlStorage>& storage )
    : ReadLabelCapability()
    , m_track( track )
    , m_storage( storage )
{
    //TODO: Update cached labels when new labels are added.
    fetchLabels();
}

void
SqlReadLabelCapability::fetch( const QString &uniqueURL )
{
    QStringList labels;

    if( !m_storage )
    {
        debug() << "Could not get SqlStorage, aborting" << Qt::endl;
        return;
    }

    QString query = "SELECT a.label FROM labels a";
    QStringList result;

    if ( !uniqueURL.isEmpty() )
    {
        query = query + QString( ", urls_labels b, urls c WHERE a.id=b.label AND b.url=c.id AND c.uniqueid=\"%1\"" );
        result = m_storage->query( query.arg( m_storage->escape( uniqueURL ) ) );
    }
    else
        result = m_storage->query( query );

    if( !result.isEmpty() )
    {
        for ( int x = 0; x < result.count(); x++)
        {
            if ( !labels.contains( result.value(x) ) )
                labels.append( result.value(x) );
        }
    }

    m_labels = labels;
    Q_EMIT labelsFetched( labels );
}


void
SqlReadLabelCapability::fetchLabels()
{
    fetch( m_track->uidUrl() );
}

//TODO: This shouldn't be in a track capability
void
SqlReadLabelCapability::fetchGlobalLabels()
{
    fetch( QString() );
}

QStringList
SqlReadLabelCapability::labels()
{
    return m_labels;
}

}

