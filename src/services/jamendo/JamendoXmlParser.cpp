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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "JamendoXmlParser.h"

#include "Amarok.h"
#include "Debug.h"
#include "StatusBar.h"

#include <QDomDocument>
#include <QFile>

#include <KFilterDev>
#include <KLocale>

using namespace Meta;

JamendoXmlParser::JamendoXmlParser( const QString &filename )
    : ThreadWeaver::Job()
    , n_numberOfTransactions ( 0 )
    , n_maxNumberOfTransactions ( 5000 )
{
    DEBUG_BLOCK
    m_sFileName = filename;
    albumTags.clear();
    m_dbHandler = new JamendoDatabaseHandler();
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}

JamendoXmlParser::~JamendoXmlParser()
{
    DEBUG_BLOCK
    delete m_dbHandler;
}

void
JamendoXmlParser::run( )
{
    readConfigFile( m_sFileName );
}

void
JamendoXmlParser::completeJob( )
{
    The::statusBar()->longMessage( i18n( "Jamendo.com database update complete. Added %1 tracks on %2 albums from %3 artists",  m_nNumberOfTracks, m_nNumberOfAlbums, m_nNumberOfArtists ), KDE::StatusBar::Information );
    debug() << "JamendoXmlParser: total number of artists: " << m_nNumberOfArtists;
    debug() << "JamendoXmlParser: total number of albums: " << m_nNumberOfAlbums;
    debug() << "JamendoXmlParser: total number of tracks: " << m_nNumberOfTracks;
    emit doneParsing();
    deleteLater();
}

void
JamendoXmlParser::readConfigFile( const QString &filename )
{
    DEBUG_BLOCK
    m_nNumberOfTracks = 0;
    m_nNumberOfAlbums = 0;
    m_nNumberOfArtists = 0;

    QDomDocument doc( "config" );

    if ( !QFile::exists( filename ) )
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
    if ( !doc.setContent( file ) )
    {
        debug() << "JamendoXmlParser::readConfigFile error parsing file";
        file->close();
        return ;
    }
    file->close();
    delete file;

    QFile::remove( filename );

    m_dbHandler->destroyDatabase();
    m_dbHandler->createDatabase();

    //run through all the elements
    QDomElement docElem = doc.documentElement();
    m_dbHandler->begin(); //start transaction (MAJOR speedup!!)
    debug() << "begin parsing content";
    parseElement( docElem );
    debug() << "finishing transaction";
    m_dbHandler->commit(); //complete transaction
    //as genres are jsut user tags, remove any that are not applied to at least 10 albums to weed out the worst crap
    //perhaps make this a config option
    m_dbHandler->trimGenres( 10 );
}

void
JamendoXmlParser::parseElement( const  QDomElement &e )
{
    QString sElementName = e.tagName();

    if (sElementName == "artist" )
         parseArtist( e );
    else if (sElementName == "album" )
        parseAlbum( e );
    else if (sElementName == "track" )
        parseTrack( e );
    else
        parseChildren( e );
}

void
JamendoXmlParser::parseChildren( const  QDomElement &e )
{
    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() )
            parseElement( n.toElement() );

        n = n.nextSibling();
    }
}

void
JamendoXmlParser::parseArtist( const  QDomElement &e )
{
    //debug() << "Found artist: ";
    m_nNumberOfArtists++;

    QString name;
    //QString genre;
    QString description;

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() )
        {
            QDomElement currentChildElement = n.toElement();

            if ( currentChildElement.tagName() == "dispname" )
                name = currentChildElement.text();
            else if ( currentChildElement.tagName() == "genre" )
                ;
            else if ( currentChildElement.tagName() == "description" )
                 description = currentChildElement.text();

            n = n.nextSibling();
        }
    }

    JamendoArtist currentArtist( name );
    currentArtist.setDescription( description );

    currentArtist.setId( e.attribute( "id", "0" ).toInt() );
    currentArtist.setPhotoURL( e.attribute( "image", "UNDEFINED" ) );
    currentArtist.setJamendoURL( e.attribute( "link", "UNDEFINED" ) );
    currentArtist.setHomeURL( e.attribute( "homepage", "UNDEFINED" ) );

    m_dbHandler->insertArtist( &currentArtist );
    countTransaction();

    /*debug() << "    Name:       " << currentArtist.getName();
    debug() << "    Id:         " << currentArtist.getId();
    //debug() << "    Photo:      " << currentArtist.getPhotoURL();
    debug() << "    J_url:      " << currentArtist.getJamendoURL();
    debug() << "    H_url:      " << currentArtist.getHomeURL();
    debug() << "    Decription: " << currentArtist.getDescription();
*/
}

void
JamendoXmlParser::parseAlbum( const  QDomElement &e)
{
    //debug() << "Found album: ";
    m_nNumberOfAlbums++;

    QString name;
    QString genre;
    QString description;
    QStringList tags;
    QString coverUrl;
    QString mp3TorrentUrl;
    QString oggTorrentUrl;

    QDomNode n = e.firstChild();

    while( !n.isNull() )
    {
        if( n.isElement() )
        {
            QDomElement currentChildElement = n.toElement();

            if ( currentChildElement.tagName() == "dispname" )
                name = currentChildElement.text();
            else if ( currentChildElement.tagName() == "genre" )
                genre = currentChildElement.text();
            else if ( currentChildElement.tagName() == "description" )
                 description = currentChildElement.text();
            //we use tags instad of genres for creating genres in the database, as the
            //Jamendo.com genres are messy at best
            else if ( currentChildElement.tagName() == "tags" )
                tags = currentChildElement.text().split(' ', QString::SkipEmptyParts);
            else if ( currentChildElement.tagName() == "Covers" )
                coverUrl = getCoverUrl( currentChildElement, 100 );
            else if ( currentChildElement.tagName() == "P2PLinks" )
            {
                QDomNode m = currentChildElement.firstChild();
                while ( !m.isNull() )
                {
                    if ( m.isElement() )
                    {
                        QDomElement p2pElement = m.toElement();

                        if ( p2pElement.tagName() == "p2plink" )
                        {
                            if ( p2pElement.attribute( "network", "" ) == "bittorrent" )
                            {  //ignore edonkey stuff
                                if ( p2pElement.attribute( "audioEncoding", "" ) == "ogg3" )
                                {
                                    oggTorrentUrl = p2pElement.text();
                                }
                                else if ( p2pElement.attribute( "audioEncoding", "" ) == "mp32" )
                                {
                                    mp3TorrentUrl = p2pElement.text();
                                }
                            }
                        }
                    }
                    m = m.nextSibling();
                }
            }
            n = n.nextSibling();
        }
    }

    JamendoAlbum currentAlbum( name );
    currentAlbum.setGenre( genre );
    currentAlbum.setDescription( description );
    currentAlbum.setId( e.attribute( "id", "0" ).toInt() );
    currentAlbum.setArtistId( e.attribute( "artistID", "0" ).toInt() );
    currentAlbum.setLaunchYear( 1000 );
    currentAlbum.setCoverUrl( coverUrl );
    currentAlbum.setMp3TorrentUrl( mp3TorrentUrl );
    currentAlbum.setOggTorrentUrl( oggTorrentUrl );
    m_albumArtistMap.insert( currentAlbum.id(), currentAlbum.artistId() );

    int newId = m_dbHandler->insertAlbum( &currentAlbum );
    countTransaction();

    foreach( const QString &genreName, tags )
    {
        //debug() << "inserting genre with album_id = " << newId << " and name = " << genreName;
        ServiceGenre currentGenre( genreName );
        currentGenre.setAlbumId( newId );
        m_dbHandler->insertGenre( &currentGenre );
        countTransaction();
    }
}

void
JamendoXmlParser::parseTrack( const  QDomElement &e)
{
    //debug() << "Found track: ";
    m_nNumberOfTracks++;

    QString name;

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() )
        {
            QDomElement currentChildElement = n.toElement();
            if ( currentChildElement.tagName() == "dispname" )
                name = currentChildElement.text();
            //skip lyrics, license and url for now
            n = n.nextSibling();
        }
    }
    int id = e.attribute( "id", "0" ).toInt();
    QString uidUrl = "http://www.jamendo.com/get/track/id/track/audio/redirect/" +  QString::number( id ) + "/?aue=ogg2";
    JamendoTrack currentTrack ( name, uidUrl );
    currentTrack.setId( id );
    currentTrack.setUidUrl( uidUrl );
    currentTrack.setAlbumId( e.attribute( "albumID", "0" ).toInt() );
    //currentTrack.setArtistId( e.attribute( "artistID", "0" ).toInt() );
    currentTrack.setLength(  e.attribute( "lengths", "0" ).toInt() );
    currentTrack.setTrackNumber(  e.attribute( "trackno", "0" ).toInt() );

    if( m_albumArtistMap.contains( currentTrack.albumId() ) )
        currentTrack.setArtistId( m_albumArtistMap.value( currentTrack.albumId() ) );

   // debug() << "inserting track with artist id: " << currentTrack.artistId();

    m_dbHandler->insertTrack( &currentTrack );
    countTransaction();
}

QString JamendoXmlParser::getCoverUrl( const QDomElement &e, int size)
{
    QDomNode n = e.firstChild();
    while ( !n.isNull() )
    {
        if ( n.isElement() )
        {
            QDomElement currentChildElement = n.toElement();
            if ( currentChildElement.tagName() == "cover" )
            {
                if ( currentChildElement.attribute( "res", "0" ).toInt() == size)
                    return currentChildElement.text();
            }
            n = n.nextSibling();
        }
    }
    return QString();
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

#include "JamendoXmlParser.moc"

