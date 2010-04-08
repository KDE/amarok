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

#include "JamendoXmlParser.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"

#include <QFile>

#include <KFilterDev>
#include <KLocale>

using namespace Meta;

static const QString COVERURL_BASE = "http://api.jamendo.com/get2/image/album/redirect/?id=%1&imagesize=100";
static const QString TORRENTURL_BASE = "http://api.jamendo.com/get2/bittorrent/file/plain/?album_id=%1&type=archive&class=%2";

JamendoXmlParser::JamendoXmlParser( const QString &filename )
    : ThreadWeaver::Job()
    , n_numberOfTransactions ( 0 )
    , n_maxNumberOfTransactions ( 5000 )
    , m_aborted( false )
{
    DEBUG_BLOCK

    // From: http://www.linuxselfhelp.com/HOWTO/MP3-HOWTO-13.html#ss13.3
    m_id3GenreHash.insert(  0, "Blues"             );
    m_id3GenreHash.insert(  1, "Classic Rock"      );
    m_id3GenreHash.insert(  2, "Country"           );
    m_id3GenreHash.insert(  3, "Dance"             );
    m_id3GenreHash.insert(  4, "Disco"             );
    m_id3GenreHash.insert(  5, "Funk"              );
    m_id3GenreHash.insert(  6, "Grunge"            );
    m_id3GenreHash.insert(  7, "Hip-Hop"           );
    m_id3GenreHash.insert(  8, "Jazz"              );
    m_id3GenreHash.insert(  9, "Metal"             );
    m_id3GenreHash.insert( 10, "New Age"           );
    m_id3GenreHash.insert( 11, "Oldies"            );
    m_id3GenreHash.insert( 12, "Other"             );
    m_id3GenreHash.insert( 13, "Pop"               );
    m_id3GenreHash.insert( 14, "R&B"               );
    m_id3GenreHash.insert( 15, "Rap"               );
    m_id3GenreHash.insert( 16, "Reggae"            );
    m_id3GenreHash.insert( 17, "Rock"              );
    m_id3GenreHash.insert( 18, "Techno"            );
    m_id3GenreHash.insert( 19, "Industrial"        );
    m_id3GenreHash.insert( 20, "Alternative"       );
    m_id3GenreHash.insert( 21, "Ska"               );
    m_id3GenreHash.insert( 22, "Death Metal"       );
    m_id3GenreHash.insert( 23, "Pranks"            );
    m_id3GenreHash.insert( 24, "Soundtrack"        );
    m_id3GenreHash.insert( 25, "Euro-Techno"       );
    m_id3GenreHash.insert( 26, "Ambient"           );
    m_id3GenreHash.insert( 27, "Trip-Hop"          );
    m_id3GenreHash.insert( 28, "Vocal"             );
    m_id3GenreHash.insert( 29, "Jazz+Funk"         );
    m_id3GenreHash.insert( 30, "Fusion"            );
    m_id3GenreHash.insert( 31, "Trance"            );
    m_id3GenreHash.insert( 32, "Classical"         );
    m_id3GenreHash.insert( 33, "Instrumental"      );
    m_id3GenreHash.insert( 34, "Acid"              );
    m_id3GenreHash.insert( 35, "House"             );
    m_id3GenreHash.insert( 36, "Game"              );
    m_id3GenreHash.insert( 37, "Sound Clip"        );
    m_id3GenreHash.insert( 38, "Gospel"            );
    m_id3GenreHash.insert( 39, "Noise"             );
    m_id3GenreHash.insert( 40, "AlternRock"        );
    m_id3GenreHash.insert( 41, "Bass"              );
    m_id3GenreHash.insert( 42, "Soul"              );
    m_id3GenreHash.insert( 43, "Punk"              );
    m_id3GenreHash.insert( 44, "Space"             );
    m_id3GenreHash.insert( 45, "Meditative"        );
    m_id3GenreHash.insert( 46, "Instrumental Pop"  );
    m_id3GenreHash.insert( 47, "Instrumental Rock" );
    m_id3GenreHash.insert( 48, "Ethnic"            );
    m_id3GenreHash.insert( 49, "Gothic"            );
    m_id3GenreHash.insert( 50, "Darkwave"          );
    m_id3GenreHash.insert( 51, "Techno-Industrial" );
    m_id3GenreHash.insert( 52, "Electronic"        );
    m_id3GenreHash.insert( 53, "Pop-Folk"          );
    m_id3GenreHash.insert( 54, "Eurodance"         );
    m_id3GenreHash.insert( 55, "Dream"             );
    m_id3GenreHash.insert( 56, "Southern Rock"     );
    m_id3GenreHash.insert( 57, "Comedy"            );
    m_id3GenreHash.insert( 58, "Cult"              );
    m_id3GenreHash.insert( 59, "Gangsta"           );
    m_id3GenreHash.insert( 60, "Top 40"            );
    m_id3GenreHash.insert( 61, "Christian Rap"     );
    m_id3GenreHash.insert( 62, "Pop/Funk"          );
    m_id3GenreHash.insert( 63, "Jungle"            );
    m_id3GenreHash.insert( 64, "Native American"   );
    m_id3GenreHash.insert( 65, "Cabaret"           );
    m_id3GenreHash.insert( 66, "New Wave"          );
    m_id3GenreHash.insert( 67, "Psychedelic"       );
    m_id3GenreHash.insert( 68, "Rave"              );
    m_id3GenreHash.insert( 69, "Showtunes"         );
    m_id3GenreHash.insert( 70, "Trailer"           );
    m_id3GenreHash.insert( 71, "Lo-Fi"             );
    m_id3GenreHash.insert( 72, "Tribal"            );
    m_id3GenreHash.insert( 73, "Acid Punk"         );
    m_id3GenreHash.insert( 74, "Acid Jazz"         );
    m_id3GenreHash.insert( 75, "Polka"             );
    m_id3GenreHash.insert( 76, "Retro"             );
    m_id3GenreHash.insert( 77, "Musical"           );
    m_id3GenreHash.insert( 78, "Rock & Roll"       );
    m_id3GenreHash.insert( 79, "Hard Rock"         );

    m_sFileName = filename;
    albumTags.clear();
    m_dbHandler = new JamendoDatabaseHandler();
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}

JamendoXmlParser::~JamendoXmlParser()
{
    DEBUG_BLOCK
    m_reader.clear();
    delete m_dbHandler;
}

void
JamendoXmlParser::run( )
{
    if( m_aborted )
        return;
    
    readConfigFile( m_sFileName );
}

void
JamendoXmlParser::completeJob()
{
    if( m_aborted )
        return;
    
    Amarok::Components::logger()->longMessage(
          i18ncp( "First part of: Jamendo.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "Jamendo.com database update complete. Added 1 track on ", "Jamendo.com database update complete. Added %1 tracks on ", m_nNumberOfTracks)
        + i18ncp( "Middle part of: Jamendo.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "1 album from ", "%1 albums from ", m_nNumberOfAlbums)
        + i18ncp( "Last part of: Jamendo.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "1 artist.", "%1 artists.", m_nNumberOfArtists )
        , Amarok::Logger::Information );

    debug() << "JamendoXmlParser: total number of artists: " << m_nNumberOfArtists;
    debug() << "JamendoXmlParser: total number of albums: " << m_nNumberOfAlbums;
    debug() << "JamendoXmlParser: total number of tracks: " << m_nNumberOfTracks;
    emit doneParsing();
    deleteLater();
}

void
JamendoXmlParser::readConfigFile( const QString &filename )
{
    if( m_aborted )
        return;
 
    m_nNumberOfTracks = 0;
    m_nNumberOfAlbums = 0;
    m_nNumberOfArtists = 0;

    if( !QFile::exists( filename ) )
    {
        debug() << "jamendo xml file does not exist";
        return;
    }

    QIODevice *file = KFilterDev::deviceForFile( filename, "application/x-gzip", true );

    if( !file || !file->open( QIODevice::ReadOnly ) )
    {
        debug() << "JamendoXmlParser::readConfigFile error reading file";
        return;
    }

    m_reader.setDevice( file );

    m_dbHandler->destroyDatabase();
    m_dbHandler->createDatabase();

    m_dbHandler->begin(); //start transaction (MAJOR speedup!!)
    while( !m_reader.atEnd() )
    {
        m_reader.readNext();
        if( m_reader.isStartElement() )
        {
            QStringRef localname = m_reader.name();
            if( localname == "artist" )
            {
                readArtist();
            }
        }
    }


    m_dbHandler->commit(); //complete transaction
    //as genres are jsut user tags, remove any that are not applied to at least 10 albums to weed out the worst crap
    //perhaps make this a config option
    m_dbHandler->trimGenres( 10 );

    file->close();
    delete file;
    QFile::remove( filename );
}

void
JamendoXmlParser::readArtist()
{
    if( m_aborted )
        return;
    
    Q_ASSERT( m_reader.isStartElement() && m_reader.name() == "artist" );

//     debug() << "Found artist: ";
    m_nNumberOfArtists++;

    QString name;
    QString description;
    QString imageUrl;
    QString jamendoUrl;

    while( !m_reader.atEnd() )
    {
        m_reader.readNext();

        if( m_reader.isEndElement() && m_reader.name() == "artist" )
            break;
        if( m_reader.isStartElement() )
        {
            QStringRef localname = m_reader.name();
            if( localname == "id" )
                m_currentArtistId = m_reader.readElementText().toInt();
            else if ( localname == "name" )
                name = m_reader.readElementText();
            else if( localname == "url" )
                jamendoUrl = m_reader.readElementText();
            else if( localname == "image" )
                imageUrl = m_reader.readElementText();
            else if( localname == "album" )
                readAlbum();
        }
    }

    JamendoArtist currentArtist( name );
    currentArtist.setDescription( description );

    currentArtist.setId( m_currentArtistId );
    currentArtist.setPhotoURL( imageUrl );
    currentArtist.setJamendoURL( jamendoUrl );

    m_dbHandler->insertArtist( &currentArtist );
    countTransaction();

//     debug() << "    Name:       " << currentArtist.name();
//     debug() << "    Id:         " << currentArtist.id();
//     debug() << "    Photo:      " << currentArtist.photoURL();
//     debug() << "    J_url:      " << currentArtist.jamendoURL();
//     debug() << "    H_url:      " << currentArtist.homeURL();
//     debug() << "    Decription: " << currentArtist.description();

}

void
JamendoXmlParser::readAlbum()
{
    if( m_aborted )
        return;
    
    Q_ASSERT( m_reader.isStartElement() && m_reader.name() == "album" );

    //debug() << "Found album: ";
    

    QString name;
    QString genre;
    QString description;
    QStringList tags;
    QString coverUrl;
    QString mp3TorrentUrl;
    QString oggTorrentUrl;
    QString releaseDate;

   while( !m_reader.atEnd() )
    {
        m_reader.readNext();

        if( m_reader.isEndElement() && m_reader.name() == "album" )
            break;
        if( m_reader.isStartElement() )
        {
            QStringRef localname = m_reader.name();

            if( localname == "id" )
                m_currentAlbumId = m_reader.readElementText().toInt();
            else if ( localname == "name" )
                name = m_reader.readElementText();
            else if( localname == "id3genre" )
                genre = m_id3GenreHash.value( m_reader.readElementText().toInt() );
            else if( localname == "releasedate" )
                releaseDate = m_reader.readElementText();
            else if( localname == "track" )
                readTrack();
//             else if ( currentChildElement.tagName() == "description" )
//                  description = currentChildElement.text();
            //we use tags instad of genres for creating genres in the database, as the
            //Jamendo.com genres are messy at best
//             else if ( currentChildElement.tagName() == "tags" )
//                 tags = currentChildElement.text().split(' ', QString::SkipEmptyParts);
//             n = n.nextSibling();
        }
    }

    //We really do not like albums with no genres, makes the service freeze, so simply ignore this.
    if( !genre.isEmpty() && genre != "Unknown" )
    {
        m_nNumberOfAlbums++;
        JamendoAlbum currentAlbum( name );
        currentAlbum.setGenre( genre );
        currentAlbum.setDescription( description );
        currentAlbum.setId( m_currentAlbumId );
        currentAlbum.setArtistId( m_currentArtistId );
        currentAlbum.setLaunchYear( releaseDate.left( 4 ).toInt() );
        currentAlbum.setCoverUrl( COVERURL_BASE.arg( m_currentAlbumId ) );
        currentAlbum.setMp3TorrentUrl( TORRENTURL_BASE.arg( QString::number( m_currentAlbumId ), "mp32" ) );
        currentAlbum.setOggTorrentUrl( TORRENTURL_BASE.arg( QString::number( m_currentAlbumId ), "ogg3" ) );
        m_albumArtistMap.insert( currentAlbum.id(), currentAlbum.artistId() );

        int newId = m_dbHandler->insertAlbum( &currentAlbum );
        countTransaction();

        //debug() << "inserting genre with album_id = " << newId << " and name = " << genreName;
        ServiceGenre currentGenre( genre );
        currentGenre.setAlbumId( newId );
        m_dbHandler->insertGenre( &currentGenre );
        countTransaction();
    }
}

void
JamendoXmlParser::readTrack()
{
    if( m_aborted )
        return;
    
    Q_ASSERT( m_reader.isStartElement() && m_reader.name() == "track" );
    //debug() << "Found track: ";
    m_nNumberOfTracks++;

    QString name;
    QString id;
    qint64     length = 0LL;
    QString trackNumber;
    QString genre;

    while( !m_reader.atEnd() )
    {
        m_reader.readNext();

        if( m_reader.isEndElement() && m_reader.name() == "track" )
            break;
        if( m_reader.isStartElement() )
        {
            QStringRef localname = m_reader.name();
            if( localname == "name" )
                name = m_reader.readElementText();
            else if( localname == "id" )
                id = m_reader.readElementText();
            else if( localname == "duration" )
                length = m_reader.readElementText().toFloat() * 1000;
            else if ( localname == "numalbum" )
                trackNumber = m_reader.readElementText();
            else if ( localname == "id3genre" )
                genre = m_id3GenreHash.value( m_reader.readElementText().toInt() );
        }
    }

    //FIXME: temp workaround for bug 221922. Remove if Jamendo fixes their redirects
    //static const QString previewUrl = "http://api.jamendo.com/get2/stream/track/redirect/?id=%1&streamencoding=ogg2";
    static const QString previewUrl = "http://api.jamendo.com/get2/stream/track/redirect/?id=%1";

    JamendoTrack currentTrack( name );
    currentTrack.setId( id.toInt() );
    currentTrack.setUidUrl( previewUrl.arg( id ) );
    currentTrack.setAlbumId( m_currentAlbumId );
    currentTrack.setArtistId( m_currentArtistId );
    currentTrack.setLength( length );
    currentTrack.setTrackNumber( trackNumber.toInt() );
    currentTrack.setGenre( genre );

    if( m_albumArtistMap.contains( currentTrack.albumId() ) )
        currentTrack.setArtistId( m_albumArtistMap.value( currentTrack.albumId() ) );

   // debug() << "inserting track with artist id: " << currentTrack.artistId();

    m_dbHandler->insertTrack( &currentTrack );
    countTransaction();
}

void
JamendoXmlParser::countTransaction()
{
    n_numberOfTransactions++;
    if ( n_numberOfTransactions >= n_maxNumberOfTransactions )
    {
        m_dbHandler->commit();
        m_dbHandler->begin();
        n_numberOfTransactions = 0;
    }
}

void
JamendoXmlParser::requestAbort()
{
    m_aborted = true;
}

#include "JamendoXmlParser.moc"

