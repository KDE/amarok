/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "SqlReadLabelCapability.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

namespace Meta
{

SqlReadLabelCapability::SqlReadLabelCapability( Meta::SqlTrack *track, SqlStorage *storage )
    : ReadLabelCapability()
    , m_track( track )
    , m_storage( storage )
{
}

QStringList SqlReadLabelCapability::labels()
{
    QStringList labels;

    if( !m_storage )
    {
        debug() << "Could not get SqlStorage, aborting" << endl;
        return labels;
    }

    const QString query = "SELECT a.label FROM labels a, urls_labels b, urls c WHERE a.id=b.label AND b.url=c.id AND c.uniqueid=\"%1\"";

    const QStringList result = m_storage->query( query.arg( m_storage->escape( m_track->uidUrl() ) ) );

    if( !result.isEmpty() )
    {
        for ( int x = 0; x < result.count(); x++)
        {
            if ( !labels.contains( result.value(x) ) )
                labels.append( result.value(x) );
        }
    }

    return labels;
}

}

