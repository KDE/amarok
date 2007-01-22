/*
 Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*/

#include "amarok.h"
#include "debug.h"
#include "magnatunedatabasehandler.h"
#include "magnatunexmlparser.h"
#include "statusbar.h"


MagnatuneXmlParser::MagnatuneXmlParser( QString filename )
        : ThreadManager::Job( "MagnatuneXmlParser" )
{
    m_currentArtist = "";
    m_sFileName = filename;
}


MagnatuneXmlParser::~MagnatuneXmlParser()
{}

bool 
MagnatuneXmlParser::doJob( )
{
    readConfigFile( m_sFileName );
    return true;
}


void 
MagnatuneXmlParser::completeJob( )
{
    Amarok::StatusBar::instance() ->longMessage(
        i18n( "Magnatune.com database update complete. Added %1 tracks on %2 albums from %3 artists" )
        .arg( m_nNumberOfTracks )
        .arg( m_nNumberOfAlbums )
        .arg( m_nNumberOfArtists ), KDE::StatusBar::Information );

    emit( doneParsing() );
}

void 
MagnatuneXmlParser::readConfigFile( QString filename )
{
    m_nNumberOfTracks = 0;
    m_nNumberOfAlbums = 0;
    m_nNumberOfArtists = 0;

    QDomDocument doc( "config" );

    QFile file( filename );
    if ( !file.open( IO_ReadOnly ) )
        return ;
    if ( !doc.setContent( &file ) )
    {
        file.close();
        return ;
    }
    file.close();


    MagnatuneDatabaseHandler::instance() ->destroyDatabase();
    MagnatuneDatabaseHandler::instance() ->createDatabase();

    //run through all the elements
    QDomElement docElem = doc.documentElement();


    MagnatuneDatabaseHandler::instance() ->begin(); //start transaction (MAJOR speedup!!)
    parseElement( docElem );
    MagnatuneDatabaseHandler::instance() ->commit(); //complete transaction

    return ;
}


void 
MagnatuneXmlParser::parseElement( QDomElement e )
{
    QString sElementName = e.tagName();

    sElementName == "Album" ?
    parseAlbum( e ) :
    parseChildren( e );
}


void 
MagnatuneXmlParser::parseChildren( QDomElement e )
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
MagnatuneXmlParser::parseAlbum( QDomElement e )
{
    m_pCurrentAlbum = new MagnatuneAlbum();
    m_pCurrentArtist = new MagnatuneArtist();

    QString sElementName;

    QDomNode n = e.firstChild();
    QDomElement childElement;

    while ( !n.isNull() )
    {
        if ( n.isElement() )
        {
            childElement = n.toElement();

            QString sElementName = childElement.tagName();


            if ( sElementName == "albumname" )
                //printf(("|--+" + childElement.text() + "\n").ascii());
                //m_currentAlbumItem = new MagnatuneListViewAlbumItem( m_currentArtistItem);
                m_pCurrentAlbum->setName( childElement.text() );

            else if ( sElementName == "albumsku" )
                m_pCurrentAlbum->setAlbumCode( childElement.text() );

            else if ( sElementName == "magnatunegenres" )
                m_pCurrentAlbum->setMagnatuneGenres( childElement.text() );

            else if ( sElementName == "launchdate" )
            {
                QString dateString = childElement.text();
                QDate date = QDate::fromString( dateString, Qt::ISODate );
                m_pCurrentAlbum->setLaunchDate( date );
            }

            else if ( sElementName == "cover_small" )
                m_pCurrentAlbum->setCoverURL( childElement.text() );

            else if ( sElementName == "artist" )
                m_pCurrentArtist->setName( childElement.text() );

            else if ( sElementName == "artistdesc" )
                m_pCurrentArtist->setDescription( childElement.text() );

            else if ( sElementName == "artistphoto" )
                m_pCurrentArtist->setPhotoURL( childElement.text() );

            else if ( sElementName == "mp3genre" )
                m_pCurrentAlbum->setMp3Genre( childElement.text() );

            else if ( sElementName == "home" )
                m_pCurrentArtist->setHomeURL( childElement.text() );

            else if ( sElementName == "Track" )
                parseTrack( childElement );

        }

        n = n.nextSibling();
    }

    // now we should have gathered all info about current album (and artist)...
    //Time to add stuff to the database

    //check if artist already exists, if not, create him/her/them/it


    int artistId = MagnatuneDatabaseHandler::instance() ->getArtistIdByExactName( m_pCurrentArtist->getName() );

    if ( artistId == -1 )
    {
        //does not exist, lets create it...

        //this is tricky in postgresql, returns id as 0 (we are within a transaction, might be the cause...)
        artistId = MagnatuneDatabaseHandler::instance()->insertArtist( m_pCurrentArtist );
        m_nNumberOfArtists++;

        if ( artistId == 0 )
        {
            artistId = MagnatuneDatabaseHandler::instance() ->getArtistIdByExactName( m_pCurrentArtist->getName() );
        }
    }


    int albumId = MagnatuneDatabaseHandler::instance()->insertAlbum( m_pCurrentAlbum, artistId );
    if ( albumId == 0 ) // again, postgres can play tricks on us...
    {
            albumId = MagnatuneDatabaseHandler::instance()->getAlbumIdByAlbumCode( m_pCurrentAlbum->getAlbumCode() );
    }

    m_nNumberOfAlbums++;

    MagnatuneTrackList::iterator it;
    for ( it = m_currentAlbumTracksList.begin(); it != m_currentAlbumTracksList.end(); ++it )
    {
        MagnatuneDatabaseHandler::instance() ->insertTrack( &( *it ), albumId, artistId );
        m_nNumberOfTracks++;
    }

    m_currentAlbumTracksList.clear();
}



void 
MagnatuneXmlParser::parseTrack( QDomElement e )
{

    QString trackName;
    QString trackNumber;
    QString streamingUrl;


    QString sElementName;
    QDomElement childElement;

    MagnatuneTrack currentTrack;


    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {

        if ( n.isElement() )
        {

            childElement = n.toElement();

            QString sElementName = childElement.tagName();


            if ( sElementName == "trackname" )
            {
                currentTrack.setName( childElement.text() );
            }
            else if ( sElementName == "url" )
            {
                currentTrack.setHifiURL( childElement.text() );
            }
            else if ( sElementName == "mp3lofi" )
            {
                currentTrack.setLofiURL( childElement.text() );
            }
            else if ( sElementName == "tracknum" )
            {
                currentTrack.setTrackNumber( childElement.text().toInt() );
            }
            else if ( sElementName == "seconds" )
            {
                currentTrack.setDuration( childElement.text().toInt() );
            }
        }
        n = n.nextSibling();
    }

    m_currentAlbumTracksList.append( currentTrack );

}

#include "magnatunexmlparser.moc"

