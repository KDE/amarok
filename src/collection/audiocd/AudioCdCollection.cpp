/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "AudioCdCollection.h"

#include "AudioCdCollectionLocation.h"
#include "AudioCdMeta.h"
#include "AudioCdQueryMaker.h"
#include "covermanager/CoverFetcher.h"
#include "Debug.h"
#include "MediaDeviceMonitor.h"
#include "MemoryQueryMaker.h"

#include <kio/job.h>


AMAROK_EXPORT_PLUGIN( AudioCdCollectionFactory )

using namespace Meta;

AudioCdCollectionFactory::AudioCdCollectionFactory()
    : Amarok::CollectionFactory()
    , m_collection( 0 )
    , m_currentUid( QString() )
{
}

void AudioCdCollectionFactory::init()
{
    DEBUG_BLOCK
    connect( MediaDeviceMonitor::instance(), SIGNAL( audioCdDetected( const QString & ) ), this, SLOT( audioCdAdded( const QString & ) ) );
    connect( MediaDeviceMonitor::instance(), SIGNAL( deviceRemoved( const QString & ) ), this, SLOT( deviceRemoved( const QString & ) ) );
}

void AudioCdCollectionFactory::audioCdAdded( const QString & uid )
{
    if ( m_collection )
    {
        delete m_collection;
        m_collection = 0;
    }

    m_currentUid = uid;
    m_collection = new AudioCdCollection();
    emit newCollection( m_collection );
}

void AudioCdCollectionFactory::deviceRemoved( const QString & uid )
{
    DEBUG_BLOCK
    debug() << "uid: " << uid;
    debug() << "m_currentUid: " << m_currentUid;
    if ( m_currentUid == uid )
    {
        m_collection->cdRemoved(); //deleted by col. manager
        m_collection = 0;
        m_currentUid = QString();
    }
}


////////////////////////////

AudioCdCollection::AudioCdCollection()
   : Collection()
   , MemoryCollection()
   , m_encodingFormat( OGG )
{
    DEBUG_BLOCK
    readCd();
}


AudioCdCollection::~AudioCdCollection()
{
}

void AudioCdCollection::readCd()
{
    DEBUG_BLOCK

    //get the CDDB info file if possible.
    m_cdInfoJob =  KIO::storedGet(  KUrl( "audiocd:/Information/CDDB Information.txt" ), KIO::NoReload, KIO::HideProgressInfo );
    connect( m_cdInfoJob, SIGNAL( result( KJob * ) )
            , this, SLOT( infoFetchComplete( KJob *) ) );
    
}

void AudioCdCollection::infoFetchComplete( KJob * job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        error() << job->error();
        m_cdInfoJob->deleteLater();
    }
    else
    {

        QString cddbInfo = m_cdInfoJob->data();
        debug() << "got cddb info: " << cddbInfo;

        int startIndex;
        int endIndex;

        QString artist;
        QString album;
        QString year;
        QString genre;

        startIndex = cddbInfo.indexOf( "DTITLE=", 0 );
        if ( startIndex != -1 )
        {
            startIndex += 7;
            endIndex = cddbInfo.indexOf( "\n", startIndex );
            QString compoundTitle = cddbInfo.mid( startIndex, endIndex - startIndex );

            debug() << "compoundTitle: " << compoundTitle;

            QStringList compoundTitleList = compoundTitle.split( " / " );

            artist = compoundTitleList.at( 0 );
            album = compoundTitleList.at( 1 );
        }

        AudioCdArtistPtr artistPtr = AudioCdArtistPtr( new  AudioCdArtist( artist ) );
        addArtist( ArtistPtr::staticCast( artistPtr ) );
        AudioCdAlbumPtr albumPtr = AudioCdAlbumPtr( new  AudioCdAlbum( album ) );
        albumPtr->setAlbumArtist( artistPtr );
        addAlbum( AlbumPtr::staticCast( albumPtr ) );


        startIndex = cddbInfo.indexOf( "DYEAR=", 0 );
        if ( startIndex != -1 )
        {
            startIndex += 6;
            endIndex = cddbInfo.indexOf( "\n", startIndex );
            year = cddbInfo.mid( startIndex, endIndex - startIndex );
        }

        AudioCdYearPtr yearPtr = AudioCdYearPtr( new AudioCdYear( year ) );
        addYear( YearPtr::staticCast( yearPtr ) );


        startIndex = cddbInfo.indexOf( "DGENRE=", 0 );
        if ( startIndex != -1 )
        {
            startIndex += 7;
            endIndex = cddbInfo.indexOf( "\n", startIndex );
            genre = cddbInfo.mid( startIndex, endIndex - startIndex );
        }

        AudioCdGenrePtr genrePtr = AudioCdGenrePtr( new  AudioCdGenre( genre ) );
        addGenre( GenrePtr::staticCast( genrePtr ) );

        startIndex = cddbInfo.indexOf( "DISCID=", 0 );
        if ( startIndex != -1 )
        {
            startIndex += 7;
            endIndex = cddbInfo.indexOf( "\n", startIndex );
            m_discCddbId = cddbInfo.mid( startIndex, endIndex - startIndex );
        }

        //get the list of tracknames
        startIndex = cddbInfo.indexOf( "TTITLE0=", 0 );
        if ( startIndex != -1 )
        {
            endIndex = cddbInfo.indexOf( "\nEXTD=", startIndex );
            QString tracksBlock = cddbInfo.mid( startIndex, endIndex - startIndex );
            debug() << "Tracks block: " << tracksBlock;
            QStringList tracksBlockList = tracksBlock.split( "\n" );

            int numberOfTracks = tracksBlockList.count();

            for ( int i = 0; i < numberOfTracks; i++ )
            {
                QString prefix = "TTITLE" + QString::number( i ) + "=";
                debug() << "prefix: " << prefix;
                QString trackName = tracksBlockList.at( i );
                trackName = trackName.replace( prefix, "" );
                debug() << "Track name: " << trackName;

                QString padding = i < 10 ? "0" : QString();

                QString baseFileName = artist + " - " + padding  + QString::number( i + 1 ) + " - " + trackName;

                QString baseUrl = "audiocd:/" + baseFileName + ".wav";

                debug() << "Track url: " << baseUrl;

                AudioCdTrackPtr trackPtr = AudioCdTrackPtr( new AudioCdTrack( this, trackName, baseUrl ) );

                trackPtr->setTrackNumber( i + 1 );
                trackPtr->setFileNameBase( baseFileName );
                
                addTrack( TrackPtr::staticCast( trackPtr ) );

                artistPtr->addTrack( trackPtr );
                trackPtr->setArtist( artistPtr );

                albumPtr->addTrack( trackPtr );
                trackPtr->setAlbum( albumPtr );

                genrePtr->addTrack( trackPtr );
                trackPtr->setGenre( genrePtr );

                yearPtr->addTrack( trackPtr );
                trackPtr->setYear( yearPtr );
                
            }
        }

        //lets see if we can find a cover for the album:
        The::coverFetcher()->queueAlbum( AlbumPtr::staticCast( albumPtr ) );

    }

    emit ( updated() );
}

QueryMaker * AudioCdCollection::queryMaker()
{
    return new AudioCdQueryMaker( this, collectionId() );
}

QString AudioCdCollection::collectionId() const
{
    return "AudioCd";
}

QString AudioCdCollection::prettyName() const
{
    return "Audio Cd";
}

KIcon AudioCdCollection::icon() const
{
    return KIcon( "media-optical-audio");
}

void AudioCdCollection::cdRemoved()
{
    emit remove();
}

QString AudioCdCollection::encodingFormat() const
{
    switch( m_encodingFormat ) {
        case WAV:
            return "vaw";
        case FLAC:
            return "flac";
        case OGG:
            return "ogg";
        case MP3:
            return "mp3";
    }
}

QString AudioCdCollection::copyableBasePath() const
{
    switch( m_encodingFormat ) {
        case WAV:
            return "audiocd:/";
        case FLAC:
            return "audiocd:/FLAC/";
        case OGG:
            return "audiocd:/Ogg Vorbis/";
        case MP3:
            return "audiocd:/MP3/";
    }
}

void AudioCdCollection::setEncodingFormat( int format ) const
{
    m_encodingFormat = format;
}

CollectionLocation * AudioCdCollection::location() const
{
    return new AudioCdCollectionLocation( this );
}



#include "AudioCdCollection.moc"


