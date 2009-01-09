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
    
    if ( url.numberOfArgs() == 4 ) {
        
        QString type = url.arg( 0 );
        QString collection = url.arg( 1 );
        QString groupMode = url.arg( 2 );
        QString filter = url.arg( 3 );


        if ( type == "service" )
            type = "Internet";
        else if ( type == "collection" )
            type = "Collection";

        The::mainWindow()->showBrowser( type );

        if ( type ==  "Internet" ) {
        
            ServiceBase * service = ServiceBrowser::instance()->services().value( collection );
            service->setFilter( filter );

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

            ServiceBrowser::instance()->showService( collection );

            return true;

        }

    }
    
    return false;

    
}

QString NavigationUrlRunner::command() const
{
    return "navigate";
}


