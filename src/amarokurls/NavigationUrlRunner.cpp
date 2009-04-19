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
#include "services/ServiceBase.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "PlaylistManager.h"

NavigationUrlRunner::NavigationUrlRunner()
    : AmarokUrlRunnerBase()
{
}


NavigationUrlRunner::~NavigationUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner( this );
}

bool
NavigationUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK;
    
    if ( url.numberOfArgs() > 0 ) {
        
        QString type = url.arg( 0 );

        QString collection;
        QString groupMode;
        QString filter;

        if ( url.numberOfArgs() > 1 )
            collection = url.arg( 1 );
        if ( url.numberOfArgs() > 2 )
            groupMode = url.arg( 2 );
        if ( url.numberOfArgs() == 4 )
            filter = url.arg( 3 );

        debug() << "type: " << type;
        debug() << "collection: " << collection;
        debug() << "groupMode: " << groupMode;
        debug() << "filter: " << filter;

        if ( type == "service" || type == "Internet" )
            type = "Internet";
        else if ( type == "collection" )
            type = "CollectionBrowser";
        else if ( type == "playlists" )
            type = "PlaylistBrowser";
        else if ( type == "files" )
            type = "FileBrowser::Widget";
        else
            return false;

        The::mainWindow()->showBrowser( type );

        if ( type ==  "Internet" ) {

            if ( collection.isEmpty() ) {
                ServiceBrowser::instance()->home();
                return true;
            }
        
            ServiceBase * service = ServiceBrowser::instance()->services().value( collection );

            if ( service == 0 ) return false;

            service->polish(); //ensure that everything we need is initialized ( especially if
                               //amarok is launched just to handle this url ).
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

            debug() << "showing service";
            ServiceBrowser::instance()->showService( collection );

            //ensure that the Amarok window is activated and on top

            if ( The::mainWindow()->isHidden() )
                The::mainWindow()->show();
            if ( The::mainWindow()->isMinimized() )
                The::mainWindow()->showNormal();

            The::mainWindow()->raise();
            The::mainWindow()->activateWindow();


            return true;

        } else  if ( type ==  "CollectionBrowser" ) {

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

            if ( The::mainWindow()->isHidden() )
                The::mainWindow()->show();
            if ( The::mainWindow()->isMinimized() )
                The::mainWindow()->showNormal();

            The::mainWindow()->raise();
            The::mainWindow()->activateWindow();


            return true;

        } else  if ( type ==  "PlaylistBrowser" ) {

            debug() << "Show playlist category: " << collection;
            QList<int> categories = The::playlistManager()->availableCategories();

            int cat = -1;

            foreach( int currentCat, categories ) {
                if ( The::playlistManager()->typeName( currentCat ) == collection ) {
                    cat = currentCat;
                    break;
                }
            }
            debug() << "got cat: " << cat;
            if ( cat != -1 )
                The::mainWindow()->playlistBrowser()->showCategory( cat );
            
            return true;
        }
        else  if ( type ==  "FileBrowser::Widget" ) {
            return true;
        }

    }
    
    return false;

}

QString NavigationUrlRunner::command() const
{
    return "navigate";
}


