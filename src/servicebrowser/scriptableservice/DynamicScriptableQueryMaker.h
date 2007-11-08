/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SHOUTCASTSERVICEQUERYMAKER_H
#define SHOUTCASTSERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

#include "meta.h"

#include "DynamicScriptableServiceCollection.h"

using namespace Meta;

/**
A query maker for fetching external data

	@author
*/
class DynamicScriptableQueryMaker : public DynamicServiceQueryMaker
{
Q_OBJECT
public:
    DynamicScriptableQueryMaker( DynamicScriptableServiceCollection * collection, QString script );
    ~DynamicScriptableQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker* startGenreQuery();
    virtual QueryMaker* startArtistQuery();
    virtual QueryMaker* startAlbumQuery();
    virtual QueryMaker* startTrackQuery();

    virtual QueryMaker* addMatch ( const Meta::GenrePtr &genre );
    virtual QueryMaker* addMatch ( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch ( const Meta::AlbumPtr &album );


    virtual QueryMaker* returnResultAsDataPtrs ( bool resultAsDataPtrs );

    void handleResult();


protected slots:

    void slotScriptComplete( );


protected:

    void fetchGenres();
    void fetchArtists();
    void fetchAlbums();
    void fetchTracks();

    void handleResult( const Meta::GenreList &genres );
    void handleResult( const Meta::ArtistList &artists );
    void handleResult( const Meta::AlbumList &albums );
    void handleResult( const Meta::TrackList &tracks );


    DynamicScriptableServiceCollection * m_collection;
    QString m_script;

    class Private;
    Private * const d;

    QString m_sessionId;
    QString m_parentAlbumId;
    QString m_parentArtistId;

/*public slots:

    void artistInsertionsComplete();
    void albumInsertionsComplete();
    void trackInsertionsComplete();
*/

};

#endif
