/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreeSpirit@gmail.com>               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MagnatuneXmlParser.h"

#include "Amarok.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"

#include <KFilterDev>
#include <KLocale>
#include <threadweaver/Job.h>

#include <QDomDocument>
#include <QFile>

using namespace Meta;

MagnatuneXmlParser::MagnatuneXmlParser( const QString &filename )
        : ThreadWeaver::Job()
{
    DEBUG_BLOCK
    m_currentArtist = "";
    m_sFileName = filename;
    debug() << "Creating MagnatuneXmlParser";
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}


MagnatuneXmlParser::~MagnatuneXmlParser()
{}

void
MagnatuneXmlParser::run()
{
    DEBUG_BLOCK
    m_pCurrentArtist = 0;
    m_pCurrentAlbum = 0;
    debug() << "MagnatuneXmlParser::doJob";
    readConfigFile( m_sFileName );
}


void
MagnatuneXmlParser::completeJob( )
{
    DEBUG_BLOCK
    The::statusBar() ->longMessage(
          i18ncp( "First part of: Magnatune.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "Magnatune.com database update complete. Added 1 track on ", "Magnatune.com database update complete. Added %1 tracks on ", m_nNumberOfTracks)
        + i18ncp( "Middle part of: Magnatune.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "1 album from ", "%1 albums from ", m_nNumberOfAlbums)
        + i18ncp( "Last part of: Magnatune.com database update complete. Added 3 tracks on 4 albums from 5 artists.", "1 artist.", "%1 artists.", m_nNumberOfArtists )
        , StatusBar::Information );

    emit doneParsing();
    deleteLater();
}

void
MagnatuneXmlParser::readConfigFile( const QString &filename )
{
    DEBUG_BLOCK
    m_nNumberOfTracks = 0;
    m_nNumberOfAlbums = 0;
    m_nNumberOfArtists = 0;

    QDomDocument doc( "config" );

    if ( !QFile::exists( filename ) )
    {
        debug() << "Magnatune xml file does not exist";
        return;
    }

    QIODevice *file = KFilterDev::deviceForFile( filename, "application/x-bzip2", true );
    if ( !file || !file->open( QIODevice::ReadOnly ) ) {
        debug() << "MagnatuneXmlParser::readConfigFile error reading file";
        return ;
    }
    if ( !doc.setContent( file ) )
    {
        debug() << "MagnatuneXmlParser::readConfigFile error parsing file";
        file->close();
        return ;
    }
    file->close();
    delete file;


    m_dbHandler->destroyDatabase();
    m_dbHandler->createDatabase();

    //run through all the elements
    QDomElement docElem = doc.documentElement();


    m_dbHandler->begin(); //start transaction (MAJOR speedup!!)
    parseElement( docElem );
    m_dbHandler->commit(); //complete transaction

    delete m_pCurrentArtist;
    delete m_pCurrentAlbum;

    return ;
}


void
MagnatuneXmlParser::parseElement( const QDomElement &e )
{
    QString sElementName = e.tagName();

    sElementName == "Album" ?
    parseAlbum( e ) :
    parseChildren( e );
}


void
MagnatuneXmlParser::parseChildren( const QDomElement &e )
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
MagnatuneXmlParser::parseAlbum( const QDomElement &e )
{
    //DEBUG_BLOCK
    QString sElementName;


    QString name;
    QString albumCode;
    QStringList magnatuneGenres;
    int launchYear = 0;
    QString coverUrl;
    QString description;
    QString artistName;
    QString artistDescription;
    QString artistPhotoUrl;
    QString mp3Genre;
    QString artistPageUrl;


    QDomNode n = e.firstChild();
    QDomElement childElement;

    while ( !n.isNull() )
    {
        if ( n.isElement() )
        {
            childElement = n.toElement();

            QString sElementName = childElement.tagName();


            if ( sElementName == "albumname" )
                //printf(("|--+" + childElement.text() + "\n").toAscii());
                //m_currentAlbumItem = new MagnatuneListViewAlbumItem( m_currentArtistItem);
                name = childElement.text();


            else if ( sElementName == "albumsku" )
                albumCode = childElement.text();

            else if ( sElementName == "magnatunegenres" )
                magnatuneGenres = childElement.text().split(',', QString::SkipEmptyParts);

            else if ( sElementName == "launchdate" )
            {
                QString dateString = childElement.text();
                QDate date = QDate::fromString( dateString, Qt::ISODate );
                launchYear = date.year();
            }

            else if ( sElementName == "cover_small" )
                coverUrl =  childElement.text();

            else if ( sElementName == "artist" )
                artistName = childElement.text();

            else if ( sElementName == "artistdesc" )
                artistDescription =  childElement.text();

            else if ( sElementName == "artistphoto" )
                artistPhotoUrl =  childElement.text() ;

            else if ( sElementName == "mp3genre" )
                mp3Genre = childElement.text();

            else if ( sElementName == "home" )
                artistPageUrl =  childElement.text();

            else if ( sElementName == "Track" )
                parseTrack( childElement );

            else if ( sElementName == "album_notes" )
                description = childElement.text();

        }

        n = n.nextSibling();
    }

    delete m_pCurrentAlbum;
    m_pCurrentAlbum = new MagnatuneAlbum( name );
    m_pCurrentAlbum->setAlbumCode( albumCode);
    m_pCurrentAlbum->setLaunchYear( launchYear );
    m_pCurrentAlbum->setCoverUrl( coverUrl );
    m_pCurrentAlbum->setDescription( description );


    // now we should have gathered all info about current album (and artist)...
    //Time to add stuff to the database

    //check if artist already exists, if not, create him/her/them/it


    int artistId;



    if ( artistNameIdMap.contains( artistName ) )
    {
        artistId = artistNameIdMap.value( artistName );
    } else  {
        //does not exist, lets create it...
        delete m_pCurrentArtist;
        m_pCurrentArtist = new MagnatuneArtist( artistName );
        m_pCurrentArtist->setDescription( artistDescription );
        m_pCurrentArtist->setPhotoUrl( artistPhotoUrl );
        m_pCurrentArtist->setMagnatuneUrl( artistPageUrl );

        //this is tricky in postgresql, returns id as 0 (we are within a transaction, might be the cause...)
        artistId = m_dbHandler->insertArtist( m_pCurrentArtist );

        m_nNumberOfArtists++;

        if ( artistId == 0 )
        {
            artistId = m_dbHandler->getArtistIdByExactName( m_pCurrentArtist->name() );
        }

        m_pCurrentArtist->setId( artistId );

        artistNameIdMap.insert( m_pCurrentArtist->name() , artistId );


    }

    m_pCurrentAlbum->setArtistId( artistId );
    int albumId = m_dbHandler->insertAlbum( m_pCurrentAlbum );
    if ( albumId == 0 ) // again, postgres can play tricks on us...
    {
            albumId = m_dbHandler->getAlbumIdByAlbumCode( m_pCurrentAlbum->albumCode() );
    }

    m_pCurrentAlbum->setId( albumId );

    m_nNumberOfAlbums++;

    QList<MagnatuneTrack *>::iterator it;
    for ( it = m_currentAlbumTracksList.begin(); it != m_currentAlbumTracksList.end(); ++it )
    {

        ( *it )->setAlbumId( m_pCurrentAlbum->id() );
        ( *it )->setArtistId( artistId );
        int trackId = m_dbHandler->insertTrack( ( *it ) );


        m_dbHandler->insertMoods( trackId, ( *it )->moods() );
        
        m_nNumberOfTracks++;
    }


    // handle genres

    foreach( const QString &genreName, magnatuneGenres ) {

        //debug() << "inserting genre with album_id = " << albumId << " and name = " << genreName;
        ServiceGenre currentGenre( genreName );
        currentGenre.setAlbumId( albumId );
        m_dbHandler->insertGenre( &currentGenre );

    }

    magnatuneGenres.clear();

    m_currentAlbumTracksList.clear();
}



void
MagnatuneXmlParser::parseTrack( const QDomElement &e )
{
    //DEBUG_BLOCK
    m_currentTrackMoodList.clear();

    QString trackName;
    QString trackNumber;
    QString streamingUrl;


    QString sElementName;
    QDomElement childElement;

    MagnatuneTrack * pCurrentTrack = new MagnatuneTrack( QString() );

    QDomNode n = e.firstChild();

    while ( !n.isNull() )
    {

        if ( n.isElement() )
        {

            childElement = n.toElement();

            QString sElementName = childElement.tagName();


            if ( sElementName == "trackname" )
            {
                pCurrentTrack->setTitle( childElement.text() );
            }
            else if ( sElementName == "url" )
            {
                pCurrentTrack->setUidUrl( childElement.text() );
            }
            else if ( sElementName == "oggurl" )
            {
                pCurrentTrack->setOggUrl( childElement.text() );
            }
            else if ( sElementName == "mp3lofi" )
            {
                pCurrentTrack->setLofiUrl( childElement.text() );
            }
            else if ( sElementName == "tracknum" )
            {
                pCurrentTrack->setTrackNumber( childElement.text().toInt() );
            }
            else if ( sElementName == "seconds" )
            {
                pCurrentTrack->setLength( childElement.text().toInt() );
            }
            else if ( sElementName == "moods" )
            {
                parseMoods( childElement );
            }
        }
        n = n.nextSibling();
    }

    pCurrentTrack->setMoods( m_currentTrackMoodList );
    m_currentAlbumTracksList.append( pCurrentTrack );

}

void MagnatuneXmlParser::parseMoods( const QDomElement &e )
{
    //DEBUG_BLOCK
    QDomNode n = e.firstChild();

    QDomElement childElement;

    while ( !n.isNull() )
    {

        if ( n.isElement() )
        {

            childElement = n.toElement();

            QString sElementName = childElement.tagName();

            if ( sElementName == "mood" )
            {
                m_currentTrackMoodList.append( childElement.text() );
            }
            else
            {
                //error, should not be here....
            }

        }
        n = n.nextSibling();
    }

}

void MagnatuneXmlParser::setDbHandler(MagnatuneDatabaseHandler * dbHandler)
{
    m_dbHandler = dbHandler;
}

#include "MagnatuneXmlParser.moc"

