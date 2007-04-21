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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendoxmlparser.h"

#include "jamendodatabasehandler.h"

#include "amarok.h"
#include "debug.h"


JamendoXmlParser::JamendoXmlParser( const QString &filename )
        : ThreadManager::Job( "JamendoXmlParser" )
{
    m_sFileName = filename;
    debug() << "Creating JamendoXmlParser" << endl;
}


JamendoXmlParser::~JamendoXmlParser()
{}

bool 
JamendoXmlParser::doJob( )
{
    debug() << "JamendoXmlParser::doJob" << endl;
    readConfigFile( m_sFileName );
    return true;
}


void 
JamendoXmlParser::completeJob( )
{
   /* Amarok::StatusBar::instance() ->longMessage(
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
    m_nNumberOfTracks = 0;
    m_nNumberOfAlbums = 0;
    m_nNumberOfArtists = 0;

    QDomDocument doc( "config" );

    QFile file( "/tmp/dbdump.en.xml" );
    if ( !file.open( QIODevice::ReadOnly ) )
        return ;
    if ( !doc.setContent( &file ) )
    {
        file.close();
        return ;
    }
    file.close();


    JamendoDatabaseHandler::instance()->destroyDatabase();
    JamendoDatabaseHandler::instance()->createDatabase();

    //run through all the elements
    QDomElement docElem = doc.documentElement();

    JamendoDatabaseHandler::instance()->begin(); //start transaction (MAJOR speedup!!)
    parseElement( docElem );
    JamendoDatabaseHandler::instance()->commit(); //complete transaction

    completeJob( );

    return ;
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

    m_nNumberOfArtists++;
    JamendoArtist currentArtist;

    currentArtist.setId( e.attribute( "id", "0" ).toInt() );
    currentArtist.setPhotoURL( e.attribute( "image", "UNDEFINED" ) );
    currentArtist.setJamendoURL( e.attribute( "link", "UNDEFINED" ) );
    currentArtist.setHomeURL( e.attribute( "homepage", "UNDEFINED" ) );


    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();
            
            if ( currentChildElement.tagName() == "dispname" )
                currentArtist.setName( currentChildElement.text() );
            else if ( currentChildElement.tagName() == "genre" )
                ;
            else if ( currentChildElement.tagName() == "description" )
                 currentArtist.setDescription( currentChildElement.text() );

            n = n.nextSibling();
        }

    }


    JamendoDatabaseHandler::instance()->insertArtist( &currentArtist );

    /*debug() << "Found artist: " << endl;
    debug() << "    Name:       " << currentArtist.getName() << endl;
    debug() << "    Id:         " << currentArtist.getId() << endl;
    //debug() << "    Photo:      " << currentArtist.getPhotoURL() << endl;
    debug() << "    J_url:      " << currentArtist.getJamendoURL() << endl;
    debug() << "    H_url:      " << currentArtist.getHomeURL() << endl;
    debug() << "    Decription: " << currentArtist.getDescription() << endl;
*/


}

void JamendoXmlParser::parseAlbum(QDomElement e)
{

    m_nNumberOfAlbums++;
    JamendoAlbum currentAlbum;

    currentAlbum.setId( e.attribute( "id", "0" ).toInt() );
    currentAlbum.setArtistId( e.attribute( "artistID", "0" ).toInt() );

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();
            
            if ( currentChildElement.tagName() == "dispname" )
                currentAlbum.setName( currentChildElement.text() );
            else if ( currentChildElement.tagName() == "genre" )
                currentAlbum.setGenre( currentChildElement.text() );
            else if ( currentChildElement.tagName() == "description" )
                 currentAlbum.setDescription( currentChildElement.text() );

            n = n.nextSibling();
        }

    }


     JamendoDatabaseHandler::instance()->insertAlbum( &currentAlbum );

    /*debug() << "Found album: " << endl;
    debug() << "    Name:       " << currentAlbum.getName() << endl;
    debug() << "    Id:         " << currentAlbum.getId() << endl;
    debug() << "    Artist_id:  " << currentAlbum.getArtistId() << endl;
    debug() << "    Genre:      " << currentAlbum.getGenre() << endl;
    debug() << "    Decription: " << currentAlbum.getDescription() << endl;
*/

}

void JamendoXmlParser::parseTrack(QDomElement e)
{

    m_nNumberOfTracks++;
    JamendoTrack currentTrack;

    currentTrack.setId( e.attribute( "id", "0" ).toInt() );

    currentTrack.setLofiURL( "http://www.jamendo.com/get/track/id/track/audio/redirect/" +  QString::number( currentTrack.getId() ) + "/?aue=ogg2" );

    currentTrack.setAlbumId( e.attribute( "albumID", "0" ).toInt() );
    currentTrack.setDuration(  e.attribute( "lengths", "0" ).toInt() );
    currentTrack.setTrackNumber(  e.attribute( "trackno", "0" ).toInt() );

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {
        if ( n.isElement() ) {
            QDomElement currentChildElement = n.toElement();
            
            if ( currentChildElement.tagName() == "dispname" )
                currentTrack.setName( currentChildElement.text() );
            //skip lyrics, license and url for now
            n = n.nextSibling();
        }

    }

    JamendoDatabaseHandler::instance()->insertTrack( &currentTrack );

  /*  debug() << "Found track: " << endl;
    debug() << "    Name:       " << currentTrack.getName() << endl;
    debug() << "    Id:         " << currentTrack.getId() << endl;
    debug() << "    Track No:   " << currentTrack.getTrackNumber() << endl;
    debug() << "    Album_id:  " << currentTrack.getAlbumId() << endl;
    debug() << "    length (s): " << currentTrack.getDuration() << endl;
*/

}





#include "jamendoxmlparser.moc"

