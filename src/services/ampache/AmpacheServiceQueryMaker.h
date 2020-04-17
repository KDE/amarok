/****************************************************************************************
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

#ifndef AMPACHESERVICEQUERYMAKER_H
#define AMPACHESERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

#include "AmpacheServiceCollection.h"
#include "AmpacheService.h"
#include "NetworkAccessManagerProxy.h"

#include <QUrl>

namespace Collections {

/** QueryMaker for Ampache
    Since Ampache only supports a very limited set of searches
    this query maker is very restricted.
    E.g. it does not support searches with AND and OR.
    TODO: support tags
    TODO: think about only using the MemoryQueryMaker
*/
class AmpacheServiceQueryMaker : public DynamicServiceQueryMaker
{
    Q_OBJECT

public:
    AmpacheServiceQueryMaker( AmpacheServiceCollection * collection, const QUrl &server, const QString &sessionId );
    ~AmpacheServiceQueryMaker() override;

    void run() override;
    void abortQuery() override;

    QueryMaker* setQueryType( QueryType type ) override;

    using DynamicServiceQueryMaker::addMatch;
    QueryMaker* addMatch( const Meta::TrackPtr &track ) override;
    QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
    QueryMaker* addMatch( const Meta::AlbumPtr &album ) override;

    QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) override;
    QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;

    int validFilterMask() override;
    QueryMaker* limitMaxResultSize( int size ) override;

    void fetchArtists();
    void fetchAlbums();
    void fetchTracks();

protected:
    struct Private;
    Private * const d;

public Q_SLOTS:
    void artistDownloadComplete( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
    void albumDownloadComplete( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
    void trackDownloadComplete( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );

private:
    // Disable copy constructor and assignment
    AmpacheServiceQueryMaker( const AmpacheServiceQueryMaker& );
    AmpacheServiceQueryMaker& operator= ( const AmpacheServiceQueryMaker& );

    /** Gets the url for the ampache requests.
        Already adds query items for the dateFilter and the limit.
    */
    QUrl getRequestUrl( const QString &action = QString() ) const;

    /*
    template<class PointerType, class ListType>
    void emitProperResult(const ListType& list);
    */
};

} //namespace Collections

#endif
