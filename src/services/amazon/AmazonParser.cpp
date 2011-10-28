 /****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include "AmazonParser.h"

#include "AmazonConfig.h"

#include "core/support/Debug.h"

#include <QFile>

AmazonParser::AmazonParser( QString tempFileName, Collections::AmazonCollection* collection, AmazonMetaFactory* factory )
    : ThreadWeaver::Job()
{
    m_tempFileName = tempFileName;
    m_collection = collection;
    m_factory = factory;
}

AmazonParser::~AmazonParser()
{
}

void AmazonParser::run()
{
    DEBUG_BLOCK
    debug() << "AmazonParser::run: " << m_tempFileName;

    m_responseDocument = new QDomDocument;

    QFile responseFile( m_tempFileName );
    if( !responseFile.open( QIODevice::ReadOnly ) )
    {
        debug() << "ERROR opening temp file";
        emit( failed( this ) );
        QFile::remove( m_tempFileName );
        return;
    }

    QString errorMsg;
    int errorLine;
    int errorColumn;

    if( !m_responseDocument->setContent( &responseFile, false, &errorMsg, &errorLine, &errorColumn ) ) // parse error
    {
        debug() << m_responseDocument->toString();
        debug() << "Parse ERROR";
        debug() << "Message:" << errorMsg;
        debug() << "Line:" << errorLine;
        debug() << "Column:" << errorColumn;
        emit( failed( this ) );
        // let's keep the temp file in case of an error
        //QFile::remove( m_tempFileName );
        return;
    }

    QString trackAsin, albumAsin;
    QString artist, albumTitle, songTitle;
    QString price, imgUrl, playableUrl;
    QString artistID, albumID, trackID;
    QStringList results;

    QDomNodeList albumItemsList = m_responseDocument->documentElement().firstChildElement( QString( "albums" ) ).elementsByTagName( QString( "item" ) );
    QDomNodeList trackItemsList = m_responseDocument->documentElement().firstChildElement( QString( "tracks" ) ).elementsByTagName( QString( "item" ) );

    m_collection->acquireWriteLock(); // locks for reading AND writing
    // we have a new results page, so we can clear the old one
    m_collection->clear();

    // album parsing
    for( int i = 0; i < albumItemsList.size(); i++ )
    {
        albumAsin  = albumItemsList.at( i ).firstChildElement( QString( "asin" ) ).firstChild().nodeValue();
        artist     = albumItemsList.at( i ).firstChildElement( QString( "artist" ) ).firstChild().nodeValue();
        albumTitle = albumItemsList.at( i ).firstChildElement( QString( "album" ) ).firstChild().nodeValue();
        price      = albumItemsList.at( i ).firstChildElement( QString( "price" ) ).firstChild().nodeValue();
        imgUrl     = albumItemsList.at( i ).firstChildElement( QString( "img" ) ).firstChild().nodeValue();

        debug() << albumAsin << artist << albumTitle << price << imgUrl;

        artistID.setNum( addArtistToCollection( artist,  QString( "TODO: I am a description. Where do I show up?" ) ) );
        addAlbumToCollection( albumTitle, QString( "TODO: I am a description. Where do I show up?" ), artistID, price,  imgUrl, albumAsin );
    }

    // track parsing
    for( int i = 0; i < trackItemsList.size(); i++ )
    {
        albumAsin  = trackItemsList.at( i ).firstChildElement( QString( "albumasin" ) ).firstChild().nodeValue();
        artist     = trackItemsList.at( i ).firstChildElement( QString( "artist" ) ).firstChild().nodeValue();
        albumTitle = trackItemsList.at( i ).firstChildElement( QString( "album" ) ).firstChild().nodeValue();
        price      = trackItemsList.at( i ).firstChildElement( QString( "price" ) ).firstChild().nodeValue();
        trackAsin  = trackItemsList.at( i ).firstChildElement( QString( "asin" ) ).firstChild().nodeValue();
        songTitle  = trackItemsList.at( i ).firstChildElement( QString( "title" ) ).firstChild().nodeValue();

        // first we make sure the artist is in the collection and get its id
        artistID.setNum( addArtistToCollection( artist,  QString( "TODO: I am a description. Where do I show up?" ) ) );

        // same for the album
        albumID.setNum( addAlbumToCollection( albumTitle, QString( "TODO: I am a description. Where do I show up?" ), artistID, QString( "" ),  QString( "" ), albumAsin ) );

        // now we can be sure that artist and album of this track are in the collection and we have their IDs
        // id, name, tracknumber, length, Url, albumId, artistID
        trackID.setNum( m_collection->trackIDMap()->size() + 1 );
        playableUrl = "http://www.amazon." + AmazonConfig::instance()->country() + "/gp/dmusic/get_sample_url.html?ASIN=" + trackAsin;
        results << trackID << songTitle << "1" << "30000" << playableUrl << albumID << artistID << price << trackAsin;

        Meta::TrackPtr trackPtr = m_factory->createTrack( results );

        if( trackPtr )
        {
            dynamic_cast<Meta::AmazonTrack*>( trackPtr.data() )->setAlbumPtr( m_collection->albumById( albumID.toInt() ) );
            dynamic_cast<Meta::AmazonTrack*>( trackPtr.data() )->setArtist( m_collection->artistById( artistID.toInt() ) );
        }

        m_collection->addTrack( trackPtr );
        m_collection->trackIDMap()->insert( trackAsin, trackID.toInt() );
        results.clear();
    }

    m_collection->releaseLock();
    m_collection->emitUpdated();
    QFile::remove( m_tempFileName );
    responseFile.close();
    // ThreadWeaver::Job automatically emits the done( this ) signal
}

int AmazonParser::addArtistToCollection( QString artistName, QString description )
{
    QStringList results;
    QString artistID;

    if( !m_collection->artistIDMap()->contains( artistName ) )
    {
        artistID.setNum( m_collection->artistIDMap()->size() + 1 );
        results << artistID << artistName << description;
        m_collection->addArtist( m_factory->createArtist( results ) );
        m_collection->artistIDMap()->insert( artistName, artistID.toInt() );
    }

    // return artist ID
    return m_collection->artistIDMap()->value( artistName );
}

int AmazonParser::addAlbumToCollection( QString albumTitle, QString description, QString artistID, QString price, QString imgUrl, QString albumAsin )
{
    QStringList results;
    QString albumID;

    debug() << albumAsin;

    if( !m_collection->albumIDMap()->contains( albumAsin ) ) // we have a new album here
    {
        // id, name, description, artistID
        albumID.setNum( m_collection->albumIDMap()->size() + 1 );
        results << albumID << albumTitle << description << artistID << price << imgUrl << albumAsin;
        m_collection->addAlbum( m_factory->createAlbum( results ) );
        m_collection->albumIDMap()->insert( albumAsin, albumID.toInt() );
    }
    else // album is known, but we might need to update it
    {
        int id;
        id = m_collection->albumIDMap()->value( albumAsin );

        if( !price.isEmpty() )
        {
            dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() )->setPrice( price );
        }

        if( !imgUrl.isEmpty() )
        {
            dynamic_cast<Meta::AmazonAlbum*>( m_collection->albumById( id ).data() )->setCoverUrl( imgUrl );
        }
    }

    return m_collection->albumIDMap()->value( albumAsin );
}
