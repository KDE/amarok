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

#ifndef SCRIPTABLESERVICEQUERYMAKER_H
#define SCRIPTABLESERVICEQUERYMAKER_H

#include "../DynamicServiceQueryMaker.h"

#include "core/meta/forward_declarations.h"

#include "ScriptableServiceCollection.h"

namespace Collections {

/**
 * A query maker for fetching external data from scripted services.
 */
class ScriptableServiceQueryMaker : public DynamicServiceQueryMaker
{
    Q_OBJECT

public:
    ScriptableServiceQueryMaker( ScriptableServiceCollection * collection, const QString &name );
    ~ScriptableServiceQueryMaker() override;

    void run() override;
    void abortQuery() override;

    QueryMaker* setQueryType( QueryType type ) override;

    QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) override;

    using QueryMaker::addMatch;
    QueryMaker* addMatch( const Meta::GenrePtr &genre ) override;
    QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
    QueryMaker* addMatch( const Meta::AlbumPtr &album ) override;

    QueryMaker* setAlbumQueryMode( AlbumQueryMode mode ) override;

    // ScriptableServiceQueryMaker-specific methods

    /**
     * Set to true if ScriptableServiceQueryMaker should convert tracks which are in
     * fact playlists to Meta::MultiTrack instances to be playable. Defaults to false.
     */
    void setConvertToMultiTracks( bool convert );

protected Q_SLOTS:
    void slotScriptComplete( );

private Q_SLOTS:
    void fetchGenre();
    void fetchArtists();
    void fetchAlbums();
    void fetchTracks();

protected:
    void handleResult( const Meta::GenreList &genres );
    void handleResult( const Meta::ArtistList &artists );
    void handleResult( const Meta::AlbumList &albums );
    void handleResult( const Meta::TrackList &tracks );

    ScriptableServiceCollection * m_collection;

    struct Private;
    Private * const d;

    QString m_sessionId;
    int m_parentAlbumId;
    int m_parentArtistId;

private:
    QString m_name;
    bool m_convertToMultiTracks;
};

} //namespace Collections

#endif
