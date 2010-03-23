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

#include "core/meta/Meta.h"

#include "ScriptableServiceCollection.h"

using namespace Meta;

/**
A query maker for fetching external data

	@author
*/
class ScriptableServiceQueryMaker : public DynamicServiceQueryMaker
{
Q_OBJECT
public:
    ScriptableServiceQueryMaker( ScriptableServiceCollection * collection, QString name );
    ~ScriptableServiceQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();
    virtual void abortQuery();

    virtual QueryMaker* setQueryType( QueryType type );

    virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );

    using QueryMaker::addMatch;
    virtual QueryMaker* addMatch ( const Meta::GenrePtr &genre );
    virtual QueryMaker* addMatch ( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch ( const Meta::AlbumPtr &album );
    virtual QueryMaker* addMatch ( const Meta::DataPtr &  data );

    virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

    virtual QueryMaker* setReturnResultAsDataPtrs ( bool resultAsDataPtrs );

    void handleResult();


protected slots:

    void slotScriptComplete( );

private slots:

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
    AmarokProcIO * m_script;

    struct Private;
    Private * const d;

    QString m_sessionId;
    int m_parentAlbumId;
    int m_parentArtistId;

/*public slots:

    void artistInsertionsComplete();
    void albumInsertionsComplete();
    void trackInsertionsComplete();
*/

private:
    template<class PointerType, class ListType>
    void emitProperResult( const ListType& list );

    QString m_name;
 
};

#endif
