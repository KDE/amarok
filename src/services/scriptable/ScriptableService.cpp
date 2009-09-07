/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ScriptableService.h"

#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "ScriptManager.h"
#include "ServiceBrowser.h"
#include "ScriptableServiceInfoParser.h"
#include "Amarok.h"
#include "Debug.h"
#include "SearchWidget.h"

#include <KStandardDirs>

using namespace Meta;

ScriptableService::ScriptableService( const QString & name )
    : ServiceBase( name, 0 )
    , m_polished( false )
    , m_name( name )
    , m_trackIdCounter( 0 )
    , m_albumIdCounter( 0 )
    , m_artistIdCounter( 0 )
    , m_genreIdCounter( 0 )
{
    DEBUG_BLOCK
    debug() << "creating ScriptableService " << name;
    m_collection = 0;
    m_bottomPanel->hide();

}

ScriptableService::~ ScriptableService()
{
    delete m_collection;
}

void ScriptableService::init( int levels, const QString & rootHtml, bool showSearchBar )
{
    DEBUG_BLOCK
    m_levels = levels;
    m_rootHtml = rootHtml;
    m_hasSearchBar = showSearchBar;
    m_searchWidget->showAdvancedButton( false );
    setInfoParser( new ScriptableServiceInfoParser( m_name ) );
    m_collection = new ScriptableServiceCollection( m_name );
    m_collection->setLevels( levels );

    if ( !showSearchBar )
        m_searchWidget->hide();
}

ServiceCollection * ScriptableService::collection()
{
    return m_collection;
}


int ScriptableService::insertItem( int level, int parentId, const QString & name, const QString & infoHtml, const QString & callbackData, const QString & playableUrl,
                                   const QString & albumOverride, const QString & artistOverride, const QString & genreOverride,
                                   const QString & composerOverride, int yearOverride, const QString &coverUrl  )
{
    DEBUG_BLOCK

    debug() << "level: " << level;
    debug() << "parentId: " << parentId;
    debug() << "name: " << name;
    debug() << "infoHtml: " << infoHtml;
    debug() << "callbackData: " << callbackData;
    debug() << "playableUrl: " << playableUrl;

    debug() << "albumOverride: " << albumOverride;
    debug() << "artistOverride: " << artistOverride;
    debug() << "coverUrl: " << coverUrl;

    if ( ( level +1 > m_levels ) || level < 0 )
        return -1;

    switch ( level ) {

        case 0:
        {

            if ( !callbackData.isEmpty() || playableUrl.isEmpty() )
                return -1;
            
            ScriptableServiceTrack * track = new ScriptableServiceTrack( name );
            track->setAlbumId( parentId );
            track->setUidUrl( playableUrl );
            track->setServiceName( m_name );
            track->setDescription( infoHtml );

            if ( !m_customEmblem.isNull() )
                track->setServiceEmblem( m_customEmblem );
            else
                track->setServiceEmblem( QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-scripted.png" ) ) );

            if ( !m_customScalableEmblem.isEmpty() )
                track->setServiceScalableEmblem( m_customScalableEmblem );
            else
                track->setServiceEmblem( KStandardDirs::locate( "data", "amarok/images/emblem-scripted-scalable.svgz" ) );
            
            if ( !albumOverride.isEmpty() )
                track->setAlbumName( albumOverride );
            if ( !artistOverride.isEmpty() )
                track->setArtistName( artistOverride );
            if ( !genreOverride.isEmpty() )
                track->setGenreName( genreOverride );
            if ( !composerOverride.isEmpty() )
                track->setComposerName( composerOverride );
            if ( yearOverride != 0 )
                track->setYearNumber( yearOverride );
            if ( !coverUrl.isEmpty() )
                track->setCustomAlbumCoverUrl( coverUrl );
            
            return addTrack( track );
            break;
            
        } case 1:
        {
            if ( callbackData.isEmpty() || !playableUrl.isEmpty() )
                return -1;

            ScriptableServiceAlbum * album = new ScriptableServiceAlbum( name );
            album->setCallbackString( callbackData );
            album->setArtistId( parentId );
            album->setDescription( infoHtml );
            album->setServiceName( m_name );
            //debug() << "setting coverUrl: " << coverUrl;
            album->setCoverUrl( coverUrl );

            album->setServiceName( m_name );
            album->setDescription( infoHtml );

            if ( !m_customEmblem.isNull() )
                album->setServiceEmblem( m_customEmblem );
            else
                album->setServiceEmblem( QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-scripted.png" ) ) );

            if ( !m_customScalableEmblem.isEmpty() )
                album->setServiceScalableEmblem( m_customScalableEmblem );
            else
                album->setServiceEmblem( KStandardDirs::locate( "data", "amarok/images/emblem-scripted-scalable.svgz" ) );
            
            return addAlbum( album );
            
        } case 2:
        {
            if ( callbackData.isEmpty() || !playableUrl.isEmpty() )
                return -1;

            ScriptableServiceArtist * artist = new ScriptableServiceArtist( name );
            artist->setCallbackString( callbackData );
            artist->setGenreId( parentId );
            artist->setDescription( infoHtml );
            artist->setServiceName( m_name );

            artist->setServiceName( m_name );
            artist->setDescription( infoHtml );

            if ( !m_customEmblem.isNull() )
                artist->setServiceEmblem( m_customEmblem );
            else
                artist->setServiceEmblem( QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-scripted.png" ) ) );

            if ( !m_customScalableEmblem.isEmpty() )
                artist->setServiceScalableEmblem( m_customScalableEmblem );
            else
                artist->setServiceEmblem( KStandardDirs::locate( "data", "amarok/images/emblem-scripted-scalable.svgz" ) );

            
            return addArtist( artist );
            
        } case 3:
        {
            
            if ( callbackData.isEmpty() ||  !playableUrl.isEmpty() || parentId != -1 )
                return -1;

            ScriptableServiceGenre * genre = new ScriptableServiceGenre( name );
            genre->setCallbackString( callbackData );
            genre->setDescription( infoHtml );
            genre->setServiceName( m_name );

            genre->setServiceName( m_name );
            genre->setDescription( infoHtml );

            if ( !m_customEmblem.isNull() )
                genre->setServiceEmblem( m_customEmblem );
            else
                genre->setServiceEmblem( QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-scripted.png" ) ) );

            if ( !m_customScalableEmblem.isEmpty() )
                genre->setServiceScalableEmblem( m_customScalableEmblem );
            else
                genre->setServiceEmblem( KStandardDirs::locate( "data", "amarok/images/emblem-scripted-scalable.svgz" ) );

            
            return addGenre( genre );
            
        }
    }
    return -1;
}


int ScriptableService::addTrack( ScriptableServiceTrack * track )
{
    DEBUG_BLOCK

    int artistId = -1;
    int genreId = -1;

    TrackPtr trackPtr = TrackPtr( track );
    m_collection->acquireWriteLock();
    m_collection->addTrack( trackPtr );
    m_collection->releaseLock();

    m_trackIdCounter++;
    track->setId( m_trackIdCounter );

    
    int albumId = track->albumId();

    //handle albums
    if ( m_levels > 1 ) {

        if ( !m_ssAlbumIdMap.contains( albumId ) ){
            delete track;
            return -1;
        }

        ScriptableServiceAlbum * album = m_ssAlbumIdMap.value( albumId );
        track->setAlbum( album->prettyName() );
        track->setAlbumPtr( AlbumPtr( album ) );
        album->addTrack( trackPtr );

        artistId = album->artistId();

     }

     if ( m_levels > 2 ) {

         if ( !m_ssArtistIdMap.contains( artistId ) ) {
             delete track;
             return -1;
         }

         ScriptableServiceArtist * artist = m_ssArtistIdMap.value( artistId );
         track->setArtist( artist->prettyName() );
         track->setArtist( ArtistPtr( artist ) );
         artist->addTrack( trackPtr );

         genreId = artist->genreId();
     }

     if ( m_levels == 4) {
         
         if ( !m_ssGenreIdMap.contains( genreId ) ) {
             delete track;
             return -1;
         }

         ScriptableServiceGenre * genre = m_ssGenreIdMap.value( genreId );
         track->setGenre( genre->prettyName() );
         track->setGenre( GenrePtr( genre ) );
         genre->addTrack( trackPtr );
         
     }

    m_ssTrackIdMap.insert( m_trackIdCounter, track );
    m_collection->acquireWriteLock();
    m_collection->addTrack( trackPtr );
    m_collection->releaseLock();

    //m_collection->emitUpdated();

    return m_trackIdCounter;
}

int ScriptableService::addAlbum( ScriptableServiceAlbum * album )
{
    int artistId = album->artistId();
    if ( m_levels > 2 && !m_ssArtistIdMap.contains( artistId ) ) {
        delete album;
        return -1;
    }

    album->setAlbumArtist( ArtistPtr( m_ssArtistIdMap.value( artistId ) ) );

    AlbumPtr albumPtr = AlbumPtr( album );
    m_albumIdCounter++;
    album->setId( m_albumIdCounter );
    m_ssAlbumIdMap.insert( m_albumIdCounter, album );
    m_collection->acquireWriteLock();
    m_collection->addAlbum( albumPtr );
    m_collection->releaseLock();
    //m_collection->emitUpdated();
    return m_albumIdCounter;
}

int ScriptableService::addArtist( Meta::ScriptableServiceArtist * artist )
{
    int genreId = artist->genreId();
    if (  m_levels > 3 && !m_ssGenreIdMap.contains( genreId ) ) {
        delete artist;
        return -1;
    }
    
    ArtistPtr artistPtr = ArtistPtr( artist );
    m_artistIdCounter++;
    artist->setId( m_artistIdCounter );
    m_ssArtistIdMap.insert( m_artistIdCounter, artist );
    m_collection->acquireWriteLock();
    m_collection->addArtist( artistPtr );
    m_collection->releaseLock();

    return m_artistIdCounter;

}

int ScriptableService::addGenre( Meta::ScriptableServiceGenre * genre )
{
    GenrePtr genrePtr = GenrePtr( genre );
    m_genreIdCounter++;

    //debug() << "adding genre: " << genre->name() << ", with id: " << m_genreIdCounter;
    
    genre->setId( m_genreIdCounter );
    m_ssGenreIdMap.insert( m_genreIdCounter, genre );
    m_collection->acquireWriteLock();
    m_collection->addGenre( genrePtr );
    m_collection->releaseLock();

    return m_genreIdCounter;
}

void ScriptableService::donePopulating( int parentId )
{
    DEBUG_BLOCK
    m_collection->donePopulating( parentId );
}

void ScriptableService::polish()
{

    if ( !m_polished ) {
        QList<int> viewLevels;

        switch ( m_levels ) {
            case 1:
                break;
            case 2:
                viewLevels << CategoryId::Album;
                break;
            case 3:
                viewLevels << CategoryId::Artist << CategoryId::Album;
                break;
            case 4:
                viewLevels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;
                break;
            default:
                return;
        }

        m_contentView->setModel( new SingleCollectionTreeItemModel( m_collection, viewLevels ) );
        m_polished = true;

    }

    infoChanged( m_rootHtml );
}

void ScriptableService::setCustomEmblem( const QPixmap &emblem )
{
    m_customEmblem = emblem;
}

QPixmap ScriptableService::customEmblem()
{
    return m_customEmblem;
}


QString ScriptableService::customScalableEmblem()
{
    return m_customScalableEmblem;
}


void ScriptableService::setCustomScalableEmblem ( const QString& emblemPath )
{
    m_customScalableEmblem = emblemPath;
}



void ScriptableService::setCurrentInfo( const QString & info )
{
    infoChanged( info );
}






#include "ScriptableService.moc"


