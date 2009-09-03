/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef MP3TUNESSERVICEQUERYMAKER_H
#define MP3TUNESSERVICEQUERYMAKER_H

#include "DynamicServiceQueryMaker.h"

//#include "Mp3TunesMeta.h"
#include "Mp3tunesLocker.h"
#include "Mp3tunesServiceCollection.h"
#include "Mp3tunesWorkers.h"

#include <kio/jobclasses.h>

namespace ThreadWeaver
{
    class Job;
}


/**
A query maker for fetching external data

	@author
*/
class Mp3tunesServiceQueryMaker : public DynamicServiceQueryMaker
{
Q_OBJECT
public:
    Mp3tunesServiceQueryMaker( Mp3tunesServiceCollection * collection, const QString &sessionId );
    Mp3tunesServiceQueryMaker( Mp3tunesLocker * locker, const QString &sessionId, Mp3tunesServiceCollection * collection );
    ~Mp3tunesServiceQueryMaker();

    virtual QueryMaker* reset();
    virtual void run();

   // virtual void runQuery();
    virtual void abortQuery();

    virtual QueryMaker* setQueryType( QueryType type );

    using DynamicServiceQueryMaker::addMatch;
    virtual QueryMaker* addMatch ( const Meta::ArtistPtr &artist );
    virtual QueryMaker* addMatch ( const Meta::AlbumPtr &album );

    virtual QueryMaker* setReturnResultAsDataPtrs ( bool resultAsDataPtrs );

    virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );

    virtual int validFilterMask();

    //Methods "borrowed" from MemoryQueryMaker
    void handleResult();
    void handleResult( const Meta::TrackList &tracks );
    void handleResult( const Meta::ArtistList &artists );
    void handleResult( const Meta::AlbumList &albums );

    void fetchArtists();
    void fetchAlbums();
    void fetchTracks();

protected:
    template<class PointerType, class ListType>
    void emitProperResult( const ListType& list );

    Mp3tunesServiceCollection * m_collection;
    Mp3tunesLocker * m_locker;
    KIO::StoredTransferJob * m_storedTransferJob;

    class Private;
    Private * const d;

    QString m_sessionId;
    QString m_parentAlbumId;
    QString m_parentArtistId;

    QString m_artistFilter;
    QString m_albumFilter;
    QString m_trackFilter;
    int m_filterType;

public slots:

    void artistDownloadComplete( QList<Mp3tunesLockerArtist> artists );
    void albumDownloadComplete( QList<Mp3tunesLockerAlbum> albums );
    void trackDownloadComplete( QList<Mp3tunesLockerTrack> tracks );


};

#endif
