/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AmarokTrack.h"

#include <QSqlQuery>
#include <QStringList>

using namespace StatSyncing;

AmarokTrack::AmarokTrack( const qint64 urlId, const ImporterSqlProviderPtr &provider,
                          const Meta::FieldHash &metadata, const QSet<QString> &labels )
    : ImporterSqlTrack( provider, metadata, labels )
    , m_urlId( urlId )
{
}

AmarokTrack::~AmarokTrack()
{
}

void
AmarokTrack::sqlCommit( QSqlDatabase db, const QSet<qint64> &fields )
{
    db.transaction();

    QStringList updates;
    if( fields.contains( Meta::valFirstPlayed ) )
        updates << "createdate = :createdate";
    if( fields.contains( Meta::valLastPlayed ) )
        updates << "accessdate = :accessdate";
    if( fields.contains( Meta::valRating ) )
        updates << "rating = :rating";
    if( fields.contains( Meta::valPlaycount ) )
        updates << "playcount = :playcount";

    QSqlQuery query( db );

    if( !updates.isEmpty() )
    {
        query.prepare( "UPDATE statistics SET "+updates.join(", ")+" WHERE url = :url" );
        query.bindValue( ":createdate", m_statistics.value( Meta::valFirstPlayed )
                                                               .toDateTime().toTime_t() );
        query.bindValue( ":accessdate", m_statistics.value( Meta::valLastPlayed )
                                                               .toDateTime().toTime_t() );
        query.bindValue( ":rating", m_statistics.value( Meta::valRating ).toInt() );
        query.bindValue( ":playcount", m_statistics.value( Meta::valPlaycount ).toInt() );
        query.bindValue( ":url", m_urlId );

        if( !query.exec() )
        {
            db.rollback();
            return;
        }
    }

    if( fields.contains( Meta::valLabel ) )
    {
        QVariantList vlabels;
        foreach( const QString &label, m_labels )
            vlabels.append( QVariant( label ) );

        // Try to insert all labels. Since the 'label' field's unique, nothing will happen
        // if the label already exists
        query.prepare( "INSERT IGNORE INTO labels (label) VALUES (?)" );
        query.addBindValue( vlabels );
        if( !query.execBatch() )
        {
            db.rollback();
            return;
        }

        // Drop all labels for the track
        query.prepare( "DELETE QUICK FROM urls_labels WHERE url = ?" );
        query.addBindValue( m_urlId );
        if( !query.exec() )
        {
            db.rollback();
            return;
        }

        // Add labels. Note that QString::arg is used due to limitation in execBatch
        query.prepare( QString( "INSERT INTO urls_labels (url, label) VALUES (%1, "
                       "(SELECT id FROM labels WHERE label = ?))" ).arg( m_urlId ) );
        query.addBindValue( vlabels );
        if( !query.execBatch() )
        {
            db.rollback();
            return;
        }
    }

    db.commit();
}
