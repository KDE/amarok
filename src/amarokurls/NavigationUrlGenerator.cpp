/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "NavigationUrlGenerator.h"
#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "Debug.h"
#include "MainWindow.h"
#include "ServiceBrowser.h"
#include "SourceInfoCapability.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "collection/sqlcollection/SqlMeta.h"
#include "PlaylistManager.h"

NavigationUrlGenerator * NavigationUrlGenerator::s_instance = 0;

NavigationUrlGenerator * NavigationUrlGenerator::instance()
{
    if( s_instance == 0 )
        s_instance = new NavigationUrlGenerator();

    return s_instance;
}

NavigationUrlGenerator::NavigationUrlGenerator()
{
}

NavigationUrlGenerator::~NavigationUrlGenerator()
{
    The::amarokUrlHandler()->unRegisterGenerator( this );
}

AmarokUrl NavigationUrlGenerator::CreateAmarokUrl()
{
    DEBUG_BLOCK

    AmarokUrl url;
    url.setCommand( "navigate" );

    //get the path
    QString path = The::mainWindow()->browserWidget()->list()->path();

    QStringList pathParts = path.split( '/' );

    //we don't use the "Home" part in navigation urls
    if ( pathParts.at( 0 ) == "root list" )
        pathParts.removeFirst();
    
    url.setPath( pathParts.join( "/" ) );


    QString filter = The::mainWindow()->browserWidget()->list()->activeCategoryRecursive()->filter();

    if ( !filter.isEmpty() )
        url.appendArg( "filter", filter );

    QList<int> levels = The::mainWindow()->browserWidget()->list()->activeCategoryRecursive()->levels();
    QString sortMode;

    foreach( int level, levels ) {
        switch( level ) {
            case CategoryId::Genre:
                sortMode += "genre-";
                break;
            case CategoryId::Artist:
                sortMode += "artist-";
                break;
            case CategoryId::Album:
                sortMode += "album-";
                break;
            case CategoryId::Composer:
                sortMode += "composer-";
                break;
            case CategoryId::Year:
                sortMode += "year-";
                break;
            default:
                break;
        }
    }

    //we have left a trailing '-' in there, get rid of it!
    if ( sortMode.size() > 0 )
        sortMode = sortMode.left( sortMode.size() - 1 );
    
    if ( !sortMode.isEmpty() )
        url.appendArg( "levels", sortMode );


    //come up with a default name for this url..
    QString name = The::mainWindow()->browserWidget()->list()->activeCategoryRecursive()->prettyName();
    url.setName( name );
    
    return url;

}

AmarokUrl NavigationUrlGenerator::urlFromAlbum( Meta::AlbumPtr album )
{
    AmarokUrl url;

    Meta::BookmarkThisCapability *btc = album->create<Meta::BookmarkThisCapability>();
    if( btc )
    {
        if( btc->isBookmarkable() ) {

            QString albumName = album->prettyName();

            url.setCommand( "navigate" );

            QString path = btc->browserName();
            if ( !btc->collectionName().isEmpty() )
                path += ( '/' + btc->collectionName() );
            url.setPath( path );

            QString filter;
            if ( btc->simpleFiltering() ) {
                filter = "\"" + albumName + "\"";
            }
            else
            {
                url.appendArg( "levels", "album" );

                QString artistName;
                if ( album->albumArtist() )
                    artistName = album->albumArtist()->prettyName();

                filter = "album:\"" + albumName + "\"";
                if ( !artistName.isEmpty() )
                    filter += ( " AND artist:\"" + artistName + "\"" );
            }

            url.appendArg( "filter", filter );

            if ( !btc->collectionName().isEmpty() )
                url.setName( i18n( "Album \"%1\" from %2", albumName, btc->collectionName() ) );
            else
                url.setName( i18n( "Album \"%1\"", albumName ) );

        }
        delete btc;
    }

    //debug() << "got url: " << url.url();
    return url;

}

AmarokUrl NavigationUrlGenerator::urlFromArtist( Meta::ArtistPtr artist )
{
    DEBUG_BLOCK

    AmarokUrl url;

    Meta::BookmarkThisCapability *btc = artist->create<Meta::BookmarkThisCapability>();
    if( btc )
    {
        if( btc->isBookmarkable() ) {

            QString artistName = artist->prettyName();

            url.setCommand( "navigate" );
            
            QString path = btc->browserName();
            if ( !btc->collectionName().isEmpty() )
                path += ( '/' + btc->collectionName() );
            url.setPath( path );

            //debug() << "Path: " << url.path();

            QString filter;
            if ( btc->simpleFiltering() ) {
                //for services only supporting simple filtering, do not try to set the sorting mode
                filter = "\"" + artistName + "\"";
            }
            else
            {
                url.appendArg( "levels", "artist-album" );
                filter = ( "artist:\"" + artistName + "\"" );
            }

            url.appendArg( "filter", filter );

            if ( !btc->collectionName().isEmpty() )
                url.setName( i18n( "Artist \"%1\" from %2", artistName, btc->collectionName() ) );
            else
                url.setName( i18n( "Artist \"%1\"", artistName ) );

        }
        delete btc;
    }

    return url;

}

QString
NavigationUrlGenerator::description()
{
    return i18n( "Bookmark Media Sources View" );
}

AmarokUrl
NavigationUrlGenerator::createUrl()
{
    return CreateAmarokUrl();
}

