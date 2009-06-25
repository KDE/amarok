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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
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

    QString target;
    
    for( int i = 0; i < url.numberOfArgs(); i++ )
    {
        target += url.arg( i );
        target += "/";
    }
    target.chop( 1 );
 
    QString leftover = The::mainWindow()->browserWidget()->list()->navigate( target );

    debug() << "leftover: " << leftover;

    return true;
}

QString NavigationUrlRunner::command() const
{
    return "navigate";
}


