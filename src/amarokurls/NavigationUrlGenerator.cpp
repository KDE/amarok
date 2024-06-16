/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "MainWindow.h"
#include "amarokconfig.h"
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "browsers/BrowserDock.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "core/support/Debug.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core-impl/collections/db/sql/SqlMeta.h"
#include "playlistmanager/PlaylistManager.h"

NavigationUrlGenerator * NavigationUrlGenerator::s_instance = nullptr;

NavigationUrlGenerator * NavigationUrlGenerator::instance()
{
    if( s_instance == nullptr )
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
    url.setCommand( QStringLiteral("navigate") );

    //get the path
    QString path = The::mainWindow()->browserDock()->list()->path();

    QStringList pathParts = path.split( QLatin1Char('/') );

    //we don't use the "Home" part in navigation urls
    if ( pathParts.at( 0 ) == QLatin1String("root list") )
        pathParts.removeFirst();
    
    url.setPath( pathParts.join( QLatin1Char('/') ) );


    QString filter = The::mainWindow()->browserDock()->list()->activeCategoryRecursive()->filter();

    if ( !filter.isEmpty() )
        url.setArg( QStringLiteral("filter"), filter );

    QList<CategoryId::CatMenuId> levels = The::mainWindow()->browserDock()->list()->activeCategoryRecursive()->levels();
    QString sortMode;

    for( CategoryId::CatMenuId level : levels ) {
        switch( level ) {
            case CategoryId::Genre:
                sortMode += QLatin1String("genre-");
                break;
            case CategoryId::Artist:
                sortMode += QLatin1String("artist-");
                break;
            case CategoryId::Album:
                sortMode += QLatin1String("album-");
                break;
            case CategoryId::AlbumArtist:
                sortMode += QLatin1String("albumartist-");
                break;
            case CategoryId::Composer:
                sortMode += QLatin1String("composer-");
                break;
            case CategoryId::Year:
                sortMode += QLatin1String("year-");
                break;
            default:
                break;
        }
    }

    //we have left a trailing '-' in there, get rid of it!
    if ( sortMode.size() > 0 )
        sortMode = sortMode.left( sortMode.size() - 1 );
    
    if ( !sortMode.isEmpty() )
        url.setArg( QStringLiteral("levels"), sortMode );


    //if in the local collection view, also store "show covers" and "show years"
    if( url.path().endsWith( QLatin1String("collections"), Qt::CaseInsensitive ) )
    {
        debug() << "bookmarking in local collection";

        if( AmarokConfig::showAlbumArt() )
            url.setArg( QStringLiteral("show_cover"), QStringLiteral("true") );
        else
            url.setArg( QStringLiteral("show_cover"), QStringLiteral("false") );

        if(  AmarokConfig::showYears() )
            url.setArg( QStringLiteral("show_years"), QStringLiteral("true") );
        else
            url.setArg( QStringLiteral("show_years"), QStringLiteral("false") );
    }

    //come up with a default name for this url..
    QString name = The::mainWindow()->browserDock()->list()->activeCategoryRecursive()->prettyName();

    //if in the file browser, also store the file path
    if( url.path().endsWith( QLatin1String("files"), Qt::CaseInsensitive ) )
    {

        //Give a proper name since it will return "/" as that is what is used in the breadcrumb.
        name = i18n( "Files" );

        FileBrowser * fileBrowser = dynamic_cast<FileBrowser *>( The::mainWindow()->browserDock()->list()->activeCategory() );
        if( fileBrowser )
        {
            url.setArg( QStringLiteral("path"), fileBrowser->currentDir() );
            name = i18n( "Files (%1)", fileBrowser->currentDir() );
        }
    }

    url.setName( name );
    
    return url;

}

AmarokUrl NavigationUrlGenerator::urlFromAlbum( Meta::AlbumPtr album )
{
    AmarokUrl url;

    QScopedPointer<Capabilities::BookmarkThisCapability> btc( album->create<Capabilities::BookmarkThisCapability>() );
    if( btc )
    {
        if( btc->isBookmarkable() ) {

            QString albumName = album->prettyName();

            url.setCommand( QStringLiteral("navigate") );

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
                url.setArg( QStringLiteral("levels"), QStringLiteral("album") );

                QString artistName;
                if ( album->albumArtist() )
                    artistName = album->albumArtist()->prettyName();

                filter = "album:\"" + albumName + "\"";
                if ( !artistName.isEmpty() )
                    filter += ( " AND artist:\"" + artistName + "\"" );
            }

            url.setArg( QStringLiteral("filter"), filter );

            if ( !btc->collectionName().isEmpty() )
                url.setName( i18n( "Album \"%1\" from %2", albumName, btc->collectionName() ) );
            else
                url.setName( i18n( "Album \"%1\"", albumName ) );

        }
    }

    //debug() << "got url: " << url.url();
    return url;
}

AmarokUrl NavigationUrlGenerator::urlFromArtist( Meta::ArtistPtr artist )
{
    DEBUG_BLOCK

    AmarokUrl url;

    QScopedPointer<Capabilities::BookmarkThisCapability> btc( artist->create<Capabilities::BookmarkThisCapability>() );
    if( btc )
    {
        if( btc->isBookmarkable() ) {

            QString artistName = artist->prettyName();

            url.setCommand( QStringLiteral("navigate") );
            
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
                url.setArg( QStringLiteral("levels"), QStringLiteral("artist-album") );
                filter = ( "artist:\"" + artistName + "\"" );
            }

            url.setArg( QStringLiteral("filter"), filter );

            if ( !btc->collectionName().isEmpty() )
                url.setName( i18n( "Artist \"%1\" from %2", artistName, btc->collectionName() ) );
            else
                url.setName( i18n( "Artist \"%1\"", artistName ) );

        }
    }

    return url;

}

QString
NavigationUrlGenerator::description()
{
    return i18n( "Bookmark Media Sources View" );
}

QIcon NavigationUrlGenerator::icon()
{
    return QIcon::fromTheme( QStringLiteral("flag-amarok") );
}

AmarokUrl
NavigationUrlGenerator::createUrl()
{
    return CreateAmarokUrl();
}

