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

#include "SqlWriteLabelCapability.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "SqlStorage.h"

namespace Meta
{

SqlWriteLabelCapability::SqlWriteLabelCapability( Meta::SqlTrack* track, SqlStorage* storage )
    : WriteLabelCapability()
    , m_track( track )
    , m_storage( storage )
{
}

void
SqlWriteLabelCapability::setLabels( const QStringList &removedLabels, const QStringList &newlabels )
{

    if( !m_storage )
    {
        debug() << "Could not get SqlStorage, aborting" << endl;
        return;
    }

    for ( int x = 0; x < newlabels.length(); x++)
    {
        //Check if all new labels are already in the Database
        const QString checkQuery = "SELECT label FROM labels WHERE label=\"%1\"";
        QStringList result = m_storage->query(  checkQuery.arg( m_storage->escape( newlabels.at( x ) ) ) );

        if ( result.isEmpty() )
        {
            const QString newQuery = "INSERT INTO labels (label) VALUE(\"%1\")";
            m_storage->query(  newQuery.arg( m_storage->escape( newlabels.at( x ) ) ) );
        }

        //Insert connection for every new label if not already there
        const QString checkNewQuery = "SELECT label from urls_labels WHERE label=(SELECT id FROM labels WHERE label=\"%1\") AND url=(SELECT id FROM urls WHERE uniqueid=\"%2\")";
        result = m_storage->query(  checkNewQuery.arg( m_storage->escape( newlabels.at( x ) ), m_storage->escape( m_track->uidUrl() ) ) );

        if ( result.isEmpty() )
        {
            const QString insertQuery = "INSERT INTO urls_labels (label,url) VALUE((SELECT id FROM labels WHERE label=\"%1\"),(SELECT id FROM urls WHERE uniqueid=\"%2\"))";
            m_storage->query(  insertQuery.arg( m_storage->escape( newlabels.at( x ) ), m_storage->escape( m_track->uidUrl() ) ) );
        }
    }

    for ( int y = 0; y < removedLabels.length(); y++)
    {
        //Delete connections for every removed label
        const QString removeQuery = "DELETE FROM urls_labels WHERE url=(SELECT id FROM urls WHERE uniqueid=\"%1\") AND label=(SELECT id FROM labels WHERE label=\"%2\")";
        m_storage->query(  removeQuery.arg( m_storage->escape( m_track->uidUrl() ), m_storage->escape( removedLabels.at( y ) ) )  );

        //Check if label isn't used anymore
        const QString checkQuery = "SELECT label FROM urls_labels where label=(SELECT id FROM labels WHERE label=\"%1\")";
        QStringList result = m_storage->query(  checkQuery.arg( m_storage->escape( removedLabels.at( y ) ) ) );

        if ( result.isEmpty() )
        {
            const QString labelRemoveQuery = "DELETE FROM labels WHERE label=\"%1\"";
            m_storage->query(  labelRemoveQuery.arg( m_storage->escape( removedLabels.at( y ) ) ) );
        }
    }
}

}
