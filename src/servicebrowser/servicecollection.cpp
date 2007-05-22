/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/



#define DEBUG_PREFIX "ServiceCollection"

#include "servicecollection.h"

#include "amarokconfig.h"
#include "servicemetabase.h"
#include "debug.h"
#include "support/memoryquerymaker.h"
//#include "reader.h"

#include <QStringList>
#include <QTimer>

using namespace Meta;

//AMAROK_EXPORT_PLUGIN( ServiceCollectionFactory )

ServiceCollectionFactory::ServiceCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

ServiceCollectionFactory::~ServiceCollectionFactory()
{
    //nothing to do
}

void
ServiceCollectionFactory::init()
{
    DEBUG_BLOCK
    //ServiceCollection *coll = new ServiceCollection( 0 );
    //emit newCollection( coll );
}


//ServiceCollection

ServiceCollection::ServiceCollection(  DatabaseHandlerBase * dbHandler  )
    : Collection()
    , MemoryCollection()
    , m_dbHandler( dbHandler )
{
    if (m_dbHandler == 0) return;

    //add all data from database hander

    //TODO Pretty please with sugar on top, implement lazy loading! 


    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    ServiceTrack * currentTrack;
    ServiceAlbum * currentAlbum;
    ServiceArtist * currentArtist;


    ArtistList artists = dbHandler->getArtistsByGenre( "All" );
    foreach( ArtistPtr artistPtr, artists ) {

        currentArtist = dynamic_cast< ServiceArtist * > ( artistPtr.data() ); 
        if( !currentArtist ) continue;
       //debug() << "Got artist: " << currentArtist->prettyName() << " with id: " <<  currentArtist->id() << endl;
        AlbumList albums = dbHandler->getAlbumsByArtistId( currentArtist->id(), "All" );

       //debug() << "    artist has " << albums.count() << " albums" << endl;

        foreach( AlbumPtr albumPtr, albums ) {

            currentAlbum = dynamic_cast< ServiceAlbum * > ( albumPtr.data() ); 
            if( !currentAlbum ) continue;
            albumMap.insert( albumPtr->name(), albumPtr ); 

            TrackList tracks = dbHandler->getTracksByAlbumId( currentAlbum->id() );

            foreach( TrackPtr trackPtr, tracks ) {

                currentTrack = dynamic_cast< ServiceTrack * > ( trackPtr.data() ); 
                if( !currentTrack ) continue;
                currentTrack->setArtist( artistPtr );
                currentTrack->setAlbum( albumPtr );

                currentAlbum->addTrack( trackPtr );
                currentArtist->addTrack( trackPtr );
                trackMap.insert( trackPtr->url(),  trackPtr );

            }

            // debug() << "    album " << albumPtr->name() << endl;
            currentAlbum->setAlbumArtist( artistPtr );
        }

        artistMap.insert( artistPtr->name(), artistPtr );

    }

    acquireWriteLock();
    setTrackMap( trackMap );
    setArtistMap( artistMap );
    setAlbumMap( albumMap );
    setGenreMap( genreMap );
    setComposerMap( composerMap );
    setYearMap( yearMap );
    releaseLock();
}

ServiceCollection::~ServiceCollection()
{

}

void
ServiceCollection::startFullScan()
{
    //ignore
}

QueryMaker*
ServiceCollection::queryBuilder()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
ServiceCollection::collectionId() const
{
    return "service collection";
}

QString
ServiceCollection::prettyName() const
{
    return "service collection";
}


#include "servicecollection.moc"

