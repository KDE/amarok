/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "NavigationUrlGenerator.h"
#include "AmarokUrl.h"
#include "Debug.h"
#include "MainWindow.h"
#include "ServiceBrowser.h"
#include "SourceInfoCapability.h"
#include "browsers/collectionbrowser/CollectionTreeItemModelBase.h"
#include "browsers/collectionbrowser/CollectionWidget.h"

NavigationUrlGenerator::NavigationUrlGenerator()
{
}


NavigationUrlGenerator::~NavigationUrlGenerator()
{
}

AmarokUrl NavigationUrlGenerator::CreateAmarokUrl()
{
    DEBUG_BLOCK
    //first, which browser is active?

    QString browser = The::mainWindow()->activeBrowserName();


    AmarokUrl url;
    url.setCommand( "navigate" );

    if ( browser == "Internet" ) {

        browser = "service";

        QString serviceName = ServiceBrowser::instance()->activeServiceName();
        debug() << "serviceName: " << serviceName;
        
        QString filter = ServiceBrowser::instance()->activeServiceFilter();
        debug() << "filter: " <<  filter;
        
        QList<int> levels = ServiceBrowser::instance()->activeServiceLevels();
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

        debug() << "sortMode: " <<  sortMode;


        url.appendArg( browser );

        if ( !serviceName.isEmpty() )
            url.appendArg( serviceName );

        if ( !sortMode.isEmpty() )
            url.appendArg( sortMode );

        if ( !filter.isEmpty() )
            url.appendArg( filter );
        
        return url;
    

    } else if ( browser == "CollectionBrowser" ) {
        
        browser = "collection";

        QString collection; //empty for now... we keep this space reserved if we later want to be able to specify a specific colletion.

        QString filter = The::mainWindow()->collectionBrowser()->filter();
        debug() << "filter: " <<  filter;
        
        QList<int> levels = The::mainWindow()->collectionBrowser()->levels();
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

        debug() << "sortMode: " <<  sortMode;

        url.appendArg( browser );

        if ( !sortMode.isEmpty() || !filter.isEmpty() )
            url.appendArg( collection );
        
        if ( !sortMode.isEmpty() )
            url.appendArg( sortMode );

        if ( !filter.isEmpty() )
            url.appendArg( filter );
        
        return url;

    }
    else if ( browser == "PlaylistBrowser" )
    {
        browser = "playlists";
        url.appendArg( browser );
         //TODO: add handling of playlist browser categories here!
        return url;

    }
    else if ( browser == "FileBrowser::Widget" )
    {
        browser = "files";
        url.appendArg( browser );
        //TODO: add handling of playlist browser categories here!
        return url;

    }
}

AmarokUrl NavigationUrlGenerator::urlFromAlbum( Meta::AlbumPtr album )
{
    AmarokUrl url;

    //we need to figure out the type of this thing...

    //case 1, from a service:
    if ( album->hasCapabilityInterface( Meta::Capability::SourceInfo ) ) {
        Meta::SourceInfoCapability * sic = qobject_cast<Meta::SourceInfoCapability *>( album->asCapabilityInterface( Meta::Capability::SourceInfo ) );
        QString serviceName = sic->sourceName();
        QString albumName = album->prettyName();
        QString artistName;
        if ( album->albumArtist() )
            artistName = album->albumArtist()->prettyName();

        debug() << "Got and album from service: " << serviceName;
        
        url.setCommand( "navigate" );
        url.appendArg( "service" );
        url.appendArg( serviceName );
        url.appendArg( "album" );

        QString filter = "album:\"" + albumName + "\"";
        if ( !artistName.isEmpty() )
            filter += ( " AND artist:\"" + artistName + "\"" );
        
        url.appendArg( filter );

        debug() << "got url: " << url.url();

        url.setName( i18n( "Album \"%1\" from %2", albumName, serviceName ) );

        debug() << "url name: " << url.name();

    }

    return url;
}

AmarokUrl NavigationUrlGenerator::urlFromArtist( Meta::ArtistPtr artist )
{
    AmarokUrl url;

 //case 1, from a service:
    if ( artist->hasCapabilityInterface( Meta::Capability::SourceInfo ) ) {
        Meta::SourceInfoCapability * sic = qobject_cast<Meta::SourceInfoCapability *>( artist->asCapabilityInterface( Meta::Capability::SourceInfo ) );
        QString serviceName = sic->sourceName();
        QString artistName = artist->prettyName();
        
        debug() << "Got and artist from service: " << serviceName;
        
        url.setCommand( "navigate" );
        url.appendArg( "service" );
        url.appendArg( serviceName );
        url.appendArg( "artist-album" );

        QString filter = "artist:\"" + artistName + "\"";
        
        url.appendArg( filter );

        debug() << "got url: " << url.url();

        url.setName( i18n( "Artist \"%1\" from %2", artistName, serviceName ) );

        debug() << "url name: " << url.name();

    }

    return url;
}