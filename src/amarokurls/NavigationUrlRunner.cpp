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
 
#include "NavigationUrlRunner.h"

#include "Debug.h"

#include "AmarokUrlHandler.h"

#include "MainWindow.h"
#include "PlaylistManager.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "services/ServiceBase.h"

NavigationUrlRunner::NavigationUrlRunner()
    : AmarokUrlRunnerBase()
{}


NavigationUrlRunner::~NavigationUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner( this );
}

bool
NavigationUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK;
    
    if ( url.numberOfArgs() > 0 )
    {
        QString baseTarget = url.arg( 0 );

        QString collection;
        QString groupMode;
        QString filter;

        if ( url.numberOfArgs() > 1 )
            collection = url.arg( 1 );
        if ( url.numberOfArgs() > 2 )
            groupMode = url.arg( 2 );
        if ( url.numberOfArgs() == 4 )
            filter = url.arg( 3 );

        debug() << "baseTarget: " << baseTarget;
        debug() << "collection: " << collection;
        debug() << "groupMode: " << groupMode;
        debug() << "filter: " << filter;

        QString target = baseTarget;
        
        if ( !collection.isEmpty() )
        {
            target += ( '/' + collection );
        }

        The::mainWindow()->browserWidget()->navigate( target );

        if ( The::mainWindow()->isHidden() )
            The::mainWindow()->show();
        if ( The::mainWindow()->isMinimized() )
            The::mainWindow()->showNormal();

        The::mainWindow()->raise();
        The::mainWindow()->activateWindow();

        if ( baseTarget == "internet" )
        {

            ServiceBase * service = dynamic_cast<ServiceBase *>( ServiceBrowser::instance()->categories().value( collection ) );

            if ( service == 0 ) return false;

            //ensure that everything we need is initialized ( especially if
            //amarok is launched just to handle this url ).
            service->polish();

            if ( groupMode == "artist-album" )
                service->sortByArtistAlbum();
            else if ( groupMode == "genre-artist" )
                service->sortByGenreArtist();
            else if ( groupMode == "album" )
                service->sortByAlbum();
            else if ( groupMode == "artist" )
                service->sortByArtist();
            else if ( groupMode == "genre-artist-album" )
                service->sortByGenreArtistAlbum();
            else if ( !groupMode.isEmpty() ) //allow for not specifying any sort mode ( remain the same )
                return false;

            service->setFilter( filter );
            debug() << "setting filter";

            return true;
        }
        else if ( baseTarget ==  "Collections" )
        {
            debug() << "get collection browser";
            CollectionWidget * collectionBrowser = The::mainWindow()->collectionBrowser();
            if ( collectionBrowser == 0 ) return false;

            debug() << "apply sort mode";

            if ( groupMode == "artist-album" )
                collectionBrowser->sortByArtistAlbum();
            else if ( groupMode == "genre-artist" )
                collectionBrowser->sortByGenreArtist();
            else if ( groupMode == "album" )
                collectionBrowser->sortByAlbum();
            else if ( groupMode == "artist" )
                collectionBrowser->sortByArtist();
            else if ( groupMode == "genre-artist-album" )
                collectionBrowser->sortByGenreArtistAlbum();

            debug() << "setting filter";
            collectionBrowser->setFilter( filter );

            debug() << "done";

            return true;
        }
        else if ( baseTarget ==  "playlists" )
        {
            return true;
        }
        else if ( baseTarget ==  "files" )
        {
            return true;
        }
    }
    
    return false;
}

QString NavigationUrlRunner::command() const
{
    return "navigate";
}


