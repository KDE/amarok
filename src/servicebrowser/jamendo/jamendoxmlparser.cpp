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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendoxmlparser.h"

#include "amarok.h"
#include "debug.h"
#include "statusbar.h"

#include <QDomDocument>
#include <QFile>

#include <KFilterDev>


JamendoXmlParser::JamendoXmlParser( const QString &filename )
        : ThreadManager::Job( "JamendoXmlParser" )
{
    DEBUG_BLOCK
    m_sFileName = filename;
    albumTags.clear();
    m_dbHandler = new JamendoDatabaseHandler();
}

JamendoXmlParser::~JamendoXmlParser()
{
    DEBUG_BLOCK
    delete m_dbHandler;
}

bool 
JamendoXmlParser::doJob( )
{
    readConfigFile( m_sFileName );
    return true;
}

void 
JamendoXmlParser::completeJob( )
{
    /*Amarok::StatusBar::instance()->longMessage(
        i18n( "Jamendo.com database update complete. Added %1 tracks on %2 albums from %3 artists" )
        .arg( m_nNumberOfTracks )
        .arg( m_nNumberOfAlbums )
        .arg( m_nNumberOfArtists ), KDE::StatusBar::Information );
*/

    debug() << "JamendoXmlParser: total number of artists: " << m_nNumberOfArtists << endl;
    debug() << "JamendoXmlParser: total number of albums: " << m_nNumberOfAlbums << endl;
    debug() << "JamendoXmlParser: total number of tracks: " << m_nNumberOfTracks << endl;
    emit( doneParsing() );
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
        debug() << "jamendo xml file does not exist" << endl;
        return;
    }

    QIODevice *file = KFilterDev::deviceForFile( filename, "application/x-gzip", true );
    if ( !file || !file->open( QIODevice::ReadOnly ) )
        return ;

    if ( !doc.setContent( file ) )
    {
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
    debug() << "begin parsing content" << endl;
    parseElement( docElem );
    debug() << "finishing transaction" << endl;
    m_dbHandler->commit(); //complete transaction

    //completeJob is called by ThreadManager
}

void 
JamendoXmlParser::parseElement( QDomElement e )
{
    QString sElementName = e.tagName();

    if (sElementName == "artist" ) {
         parseArtist( e );
    } else if (sElementName == "album" ) {
        parseAlbum( e );
    } else if (sElementName == "track" ) {
        parseTrack( e );
    } else {
        parseChildren( e );
    }
}

void 
JamendoXmlParser::parseChildren( QDomElement e )
{
    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() )
            parseElement( n.toElement() );

        n = n.nextSibling();
    }
}

void JamendoXmlParser::parseArtist( QDomElement e ) {


      //debug() << "Found artist: " << endl;
    m_nNumberOfArtists++;

    QString name;
    //QString genre;
    QString description;

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
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

    /*debug() << "    Name:       " << currentArtist.getName() << endl;
    debug() << "    Id:         " << currentArtist.getId() << endl;
    //debug() << "    Photo:      " << currentArtist.getPhotoURL() << endl;
    debug() << "    J_url:      " << currentArtist.getJamendoURL() << endl;
    debug() << "    H_url:      " << currentArtist.getHomeURL() << endl;
    debug() << "    Decription: " << currentArtist.getDescription() << endl;
*/
}

void JamendoXmlParser::parseAlbum(QDomElement e)
{
    //debug() << "Found album: " << endl;
    m_nNumberOfAlbums++;

    QString name;
    QString genre;
    QString description;
    QStringList tags;
    QString coverUrl;

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();

            if ( currentChildElement.tagName() == "dispname" )
                name = currentChildElement.text();
            else if ( currentChildElement.tagName() == "genre" )
                genre = currentChildElement.text();
            else if ( currentChildElement.tagName() == "description" )
                 description = currentChildElement.text();
            //we use tags instad of genres for creating genres in the database, as the 
            //Jamendo.com genres are messy at best
            else if ( currentChildElement.tagName() == "tags" ) {
                tags = currentChildElement.text().split(" ", QString::SkipEmptyParts);

            }
            else if ( currentChildElement.tagName() == "Covers" ) {
                coverUrl = getCoverUrl( currentChildElement, 100 );

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

    currentAlbum.setCoverURL( coverUrl );


   int newId = m_dbHandler->insertAlbum( &currentAlbum );

   foreach( QString genreName, tags ) {

        //debug() << "inserting genre with album_id = " << newId << " and name = " << genreName << endl;

        ServiceGenre currentGenre( genreName );
        currentGenre.setAlbumId( newId );
        m_dbHandler->insertGenre( &currentGenre );

    }


}

void JamendoXmlParser::parseTrack(QDomElement e)
{
    //debug() << "Found track: " << endl;
    m_nNumberOfTracks++;

    QString name;

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();

            if ( currentChildElement.tagName() == "dispname" )
                name = currentChildElement.text();
            //skip lyrics, license and url for now
            n = n.nextSibling();
        }

    }


    JamendoTrack currentTrack ( name );

    currentTrack.setId( e.attribute( "id", "0" ).toInt() );

    currentTrack.setUrl( "http://www.jamendo.com/get/track/id/track/audio/redirect/" +  QString::number( currentTrack.id() ) + "/?aue=ogg2" );

    currentTrack.setAlbumId( e.attribute( "albumID", "0" ).toInt() );
    currentTrack.setArtistId( e.attribute( "artistID", "0" ).toInt() );
    currentTrack.setLength(  e.attribute( "lengths", "0" ).toInt() );
    currentTrack.setTrackNumber(  e.attribute( "trackno", "0" ).toInt() );

    m_dbHandler->insertTrack( &currentTrack );



}

QString JamendoXmlParser::getCoverUrl(QDomElement e, int size)
{

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();

            if ( currentChildElement.tagName() == "cover" ) {
                if ( currentChildElement.attribute( "res", "0" ).toInt() == size)
                    return currentChildElement.text();
            }

            n = n.nextSibling();
        }

    }

    return QString();


}

#include "jamendoxmlparser.moc"

