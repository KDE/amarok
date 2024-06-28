/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ServiceSqlQueryMaker.h"
#include <core/storage/SqlStorage.h>
#include <core-impl/storage/StorageManager.h>

using namespace Collections;

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
    return StorageManager::instance()->sqlStorage()->query( statement );
}

int
ServiceSqlCollection::insert( const QString &statement, const QString &table )
{
    return StorageManager::instance()->sqlStorage()->insert( statement, table );
}


QString
ServiceSqlCollection::escape(const QString &text ) const
{
    return StorageManager::instance()->sqlStorage()->escape( text );
}

Meta::TrackPtr
ServiceSqlCollection::trackForUrl(const QUrl &url)
{
    if ( !possiblyContainsTrack( url ) ) //do we even bother trying?
        return Meta::TrackPtr();

    //split out the parts we can be sure about ( strip username and such info )
    QString trackRows = m_metaFactory->getTrackSqlRows() + QLatin1Char(',') + m_metaFactory->getAlbumSqlRows() + QLatin1Char(',') +  m_metaFactory->getArtistSqlRows() + QLatin1Char(',') +  m_metaFactory->getGenreSqlRows();

    QString prefix = m_metaFactory->tablePrefix();

    QString pristineUrl = url.url();

    auto sqlDb = StorageManager::instance()->sqlStorage();

    QString from =  prefix + QStringLiteral("_tracks");
    from += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_albums ON ") + prefix + QStringLiteral("_tracks.album_id = ") + prefix + QStringLiteral("_albums.id");
    from += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_artists ON ") + prefix + QStringLiteral("_albums.artist_id = ") + prefix + QStringLiteral("_artists.id");
    from += QStringLiteral(" LEFT JOIN ") + prefix + QStringLiteral("_genre ON ") + prefix + QStringLiteral("_genre.album_id = ") + prefix + QStringLiteral("_albums.id");

    QString queryString = QStringLiteral( "select DISTINCT %1 FROM %2 WHERE %3_tracks.preview_url = '%4' GROUP BY %5_tracks.id;" )
            .arg( trackRows,
                  from,
                  prefix,
                  sqlDb->escape( pristineUrl ),
                  prefix );

    //debug() << "Querying for track: " << queryString;
    QStringList result = sqlDb->query( queryString );
    //debug() << "result: " << result;

    return m_registry->getTrack( result );
}

bool
ServiceSqlCollection::possiblyContainsTrack(const QUrl &url) const
{
    return url.url().contains( m_metaFactory->tablePrefix(), Qt::CaseInsensitive );
}


