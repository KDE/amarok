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

AMAROK_EXPORT_PLUGIN( ServiceCollectionFactory );

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
    ServiceCollection *coll = new ServiceCollection( );
    emit newCollection( coll );
}


//ServiceCollection

ServiceCollection::ServiceCollection( )
    : QObject()
    , Collection()
    , MemoryCollection()
{

    //m_dbHandler = new JamendoDatabaseHandler();


    //add some dummy data for testing


    TrackMap trackMap;
    ArtistMap artistMap;
    AlbumMap albumMap;
    GenreMap genreMap;
    ComposerMap composerMap;
    YearMap yearMap;

    ServiceTrackPtr trackPtr;
    ServiceAlbumPtr albumPtr;
    ServiceArtistPtr artistPtr;


    artistPtr = ServiceArtistPtr( new ServiceArtist( "Artist 1" ) );
    artistMap.insert( "Artist 1", ArtistPtr::staticCast( artistPtr ) );

    albumPtr = ServiceAlbumPtr( new ServiceAlbum( "album 1" ) );
    albumMap.insert( "album 1", AlbumPtr::staticCast( albumPtr ) );



    trackPtr = ServiceTrackPtr( new ServiceTrack( "http://blah.sd/mp3.mp3", "track1" ) );
    albumPtr->addTrack( trackPtr );
    trackPtr->setAlbum( albumPtr );
    artistPtr->addTrack( trackPtr );
    trackPtr->setArtist( artistPtr );

    trackPtr = ServiceTrackPtr( new ServiceTrack( "http://blah.sd/mp3.mp3", "track2" ) );
    albumPtr->addTrack( trackPtr );
    trackPtr->setAlbum( albumPtr );
    artistPtr->addTrack( trackPtr );
    trackPtr->setArtist( artistPtr );

    trackPtr = ServiceTrackPtr( new ServiceTrack( "http://blah.sd/mp3.mp3", "track3" ) );
    albumPtr->addTrack( trackPtr );
    trackPtr->setAlbum( albumPtr );
    artistPtr->addTrack( trackPtr );
    trackPtr->setArtist( artistPtr );



/*
        QString itemId = QString::number( var.toMap()["miid"].toList()[0].toInt() );
        QString format = var.toMap()["asfm"].toList()[0].toString();
        DaapTrackPtr track( new DaapTrack( m_host, m_port, m_databaseId, itemId, format ) );
        track->setTitle( var.toMap()["minm"].toList()[0].toString() );
        track->setLength( var.toMap()["astm"].toList()[0].toInt()/1000 );
        track->setTrackNumber( var.toMap()["astn"].toList()[0].toInt() );
        QString album = var.toMap()["asal"].toList()[0].toString();
        DaapAlbumPtr albumPtr;
        if ( albumMap.contains( album ) )
            albumPtr = DaapAlbumPtr::staticCast( albumMap.value( album ) );
        else
        {
            albumPtr = DaapAlbumPtr( new DaapAlbum( album ) );
            albumMap.insert( album, AlbumPtr::staticCast( albumPtr ) );
        }
        albumPtr->addTrack( track );
        track->setAlbum( albumPtr );

        QString artist = var.toMap()["asar"].toList()[0].toString();
        DaapArtistPtr artistPtr;
        if ( artistMap.contains( artist ) )
            artistPtr = DaapArtistPtr::staticCast( artistMap.value( artist ) );
        else
        {
            artistPtr = DaapArtistPtr( new DaapArtist( artist ) );
            artistMap.insert( artist, ArtistPtr::staticCast( artistPtr ) );
        }
        artistPtr->addTrack( track );
        track->setArtist( artistPtr );

        QString year = var.toMap()["asyr"].toList()[0].toString();
        DaapYearPtr yearPtr;
        if ( yearMap.contains( year ) )
            yearPtr = DaapYearPtr::staticCast( yearMap.value( year ) );
        else
        {
            yearPtr = DaapYearPtr( new DaapYear( year ) );
            yearMap.insert( year, YearPtr::staticCast( yearPtr ) );
        }
        yearPtr->addTrack( track );
        track->setYear( yearPtr );

        QString genre = var.toMap()["asgn"].toList()[0].toString();
        DaapGenrePtr genrePtr;
        if ( genreMap.contains( genre ) )
            genrePtr = DaapGenrePtr::staticCast( genreMap.value( genre ) );
        else
        {
            genrePtr = DaapGenrePtr( new DaapGenre( genre ) );
            genreMap.insert( genre, GenrePtr::staticCast( genrePtr ) );
        }
        genrePtr->addTrack( track );
        track->setGenre( genrePtr );
    }*/

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
    return "jamendo collection";
}

QString
ServiceCollection::prettyName() const
{
    return "jamendo collection";
}



#include "servicecollection.moc"

