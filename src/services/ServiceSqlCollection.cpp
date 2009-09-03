/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ServiceSqlCollection.h"

#include "CollectionManager.h"
#include "ServiceSqlQueryMaker.h"
#include "SqlStorage.h"

#include <klocale.h>



ServiceSqlCollection::ServiceSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry )
    : ServiceCollection()
    , m_metaFactory( metaFactory )
    , m_registry( registry )
    , m_collectionId( id )
    , m_prettyName( prettyName )
{
}

ServiceSqlCollection::~ServiceSqlCollection()
{
}

QString
ServiceSqlCollection::collectionId() const
{
    return m_collectionId;
}

QString
ServiceSqlCollection::prettyName() const
{
    return m_prettyName;
}

QueryMaker*
ServiceSqlCollection::queryMaker()
{
    return new ServiceSqlQueryMaker( this, m_metaFactory, m_registry );
}

QStringList
ServiceSqlCollection::query( const QString &statement )
{
    return CollectionManager::instance()->sqlStorage()->query( statement );
}

int
ServiceSqlCollection::insert( const QString &statement, const QString &table )
{
    return CollectionManager::instance()->sqlStorage()->insert( statement, table );
}


QString
ServiceSqlCollection::escape( QString text ) const
{
    return CollectionManager::instance()->sqlStorage()->escape( text );
}

Meta::TrackPtr
ServiceSqlCollection::trackForUrl(const KUrl & url)
{
    if ( !possiblyContainsTrack( url ) ) //do we even bother trying?
        return Meta::TrackPtr();

    //split out the parts we can be sure about ( strip username and such info )
    QString trackRows = m_metaFactory->getTrackSqlRows() + ',' + m_metaFactory->getAlbumSqlRows() + ',' +  m_metaFactory->getArtistSqlRows() + ',' +  m_metaFactory->getGenreSqlRows();

    QString prefix = m_metaFactory->tablePrefix();

    QString pristineUrl = url.url();

    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();

    QString from =  prefix + "_tracks";
    from += " LEFT JOIN " + prefix + "_albums ON " + prefix + "_tracks.album_id = " + prefix + "_albums.id";
    from += " LEFT JOIN " + prefix + "_artists ON " + prefix + "_albums.artist_id = " + prefix + "_artists.id";
    from += " LEFT JOIN " + prefix + "_genre ON " + prefix + "_genre.album_id = " + prefix + "_albums.id";

    QString queryString = QString( "select DISTINCT %1 FROM %2 WHERE %3_tracks.preview_url = '%4' GROUP BY %5_tracks.id;" )
            .arg( trackRows)
            .arg( from )
            .arg( prefix )
            .arg( sqlDb->escape( pristineUrl ) )
            .arg( prefix );

    //debug() << "Querying for track: " << queryString;
    QStringList result = sqlDb->query( queryString );
    //debug() << "result: " << result;

    return m_registry->getTrack( result );
}

bool
ServiceSqlCollection::possiblyContainsTrack(const KUrl & url) const
{
    return url.url().contains( m_metaFactory->tablePrefix(), Qt::CaseInsensitive );
}

#include "ServiceSqlCollection.moc"

