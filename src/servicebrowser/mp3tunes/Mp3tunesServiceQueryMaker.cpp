/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
 *                                                                         *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "Mp3tunesServiceQueryMaker.h"

#include "amarok.h"
#include "debug.h"
#include "servicemetabase.h"
#include "collection/support/MemoryMatcher.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

 #include <QDomDocument>

using namespace Meta;

struct Mp3tunesServiceQueryMaker::Private {
    enum QueryType { NONE, TRACK, ARTIST, ALBUM, COMPOSER, YEAR, GENRE, CUSTOM };
    QueryType type;
    int maxsize;
    bool returnDataPtrs;
};


Mp3tunesServiceQueryMaker::Mp3tunesServiceQueryMaker( Mp3tunesServiceCollection * collection, const QString &sessionId  )
 : DynamicServiceQueryMaker()
 , m_storedTransferJob( 0 )
 , d( new Private )

{
    DEBUG_BLOCK
    m_collection = collection;
    m_sessionId = sessionId;
    reset();
}


Mp3tunesServiceQueryMaker::~Mp3tunesServiceQueryMaker()
{
    delete d;
}

QueryMaker * Mp3tunesServiceQueryMaker::reset()
{
    d->type = Private::NONE;
    d->maxsize = -1;
    d->returnDataPtrs = false;

    return this;
}

QueryMaker*
Mp3tunesServiceQueryMaker::returnResultAsDataPtrs( bool resultAsDataPtrs )
{
    d->returnDataPtrs = resultAsDataPtrs;
    return this;
}

void Mp3tunesServiceQueryMaker::run()
{
    DEBUG_BLOCK
    if ( d->type == Private::NONE )
        //TODO error handling
        return;
    if (  d->type == Private::ARTIST )       
        fetchArtists();
}

void Mp3tunesServiceQueryMaker::runQuery()
{

    DEBUG_BLOCK

    if ( m_storedTransferJob != 0 )
        return;

    m_collection->acquireReadLock();
    //naive implementation, fix this
    //note: we are not handling filtering yet
  
    //this is where the fun stuff happens
    if (  d->type == Private::ARTIST )       
        fetchArtists();

    m_collection->releaseLock();
}


void Mp3tunesServiceQueryMaker::abortQuery()
{
}

QueryMaker * Mp3tunesServiceQueryMaker::startArtistQuery()
{
    DEBUG_BLOCK
    d->type = Private::ARTIST;
    return this;
}




// What's worse, a bunch of almost identical repeated code, or a not so obvious macro? :-)
// The macro below will emit the proper result signal. If m_resultAsDataPtrs is true,
// it'll emit the signal that takes a list of DataPtrs. Otherwise, it'll call the
// signal that takes the list of the specific class.
// (copied from sqlquerybuilder.cpp with a few minor tweaks)

#define emitProperResult( PointerType, list ) { \
            if ( d->returnDataPtrs ) { \
                DataList data; \
                foreach( PointerType p, list ) { \
                    data << DataPtr::staticCast( p ); \
                } \
                emit newResultReady( m_collection->collectionId(), data ); \
            } \
            else { \
                emit newResultReady( m_collection->collectionId(), list ); \
            } \
        }


void Mp3tunesServiceQueryMaker::handleResult() {
    DEBUG_BLOCK

}




void Mp3tunesServiceQueryMaker::fetchArtists()
{
    DEBUG_BLOCK
    if ( m_collection->artistMap().values().count() != 0 ) {
        //handleResult();
        debug() << "no need to fetch artists again! ";
    }
    else {

        QString urlString = "http://ws.mp3tunes.com/api/v0/lockerData?sid=<SESSION_ID>&partner_token=<PARTNER_TOKEN>&output=xml&type=artist";

        urlString.replace( "<SESSION_ID>", m_sessionId);
        urlString.replace( "<PARTNER_TOKEN>", "7359149936");


        m_storedTransferJob =  KIO::storedGet(  KUrl( urlString ), false, false );
        connect( m_storedTransferJob, SIGNAL( result( KJob * ) )
            , this, SLOT( artistDownloadComplete( KJob *) ) );
    }
}

void Mp3tunesServiceQueryMaker::artistDownloadComplete(KJob * job)
{

    DEBUG_BLOCK

    debug() << "recieved artists: " <<  m_storedTransferJob->data();

}



#include "Mp3tunesServiceQueryMaker.moc"






