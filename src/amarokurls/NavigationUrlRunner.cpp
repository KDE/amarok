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

    //get to the correct category
    debug() << "Navigate to path: " << url.path();
    The::mainWindow()->browserWidget()->list()->navigate( url.path() );

    BrowserCategory * active =  The::mainWindow()->browserWidget()->list()->activeCategoryRecursive();

    QMap<QString, QString> args = url.args();

    if ( args.keys().contains( "levels" ) )
    {
        QString levelsString = args.value( "levels" );
        QList<int> levels;

        QStringList levelsStringList = levelsString.split( "-" );

        foreach( QString levelString, levelsStringList ) {
            if( levelString == "genre" )
                levels.append( CategoryId::Genre );
            else if( levelString == "artist" )
                levels.append( CategoryId::Artist );
            else if( levelString == "album" )
                levels.append( CategoryId::Album );
            else if( levelString == "composer" )
                levels.append( CategoryId::Composer );
            else if( levelString == "year" )
                levels.append( CategoryId::Year );
        }

        active->setLevels( levels );

    }

    if ( args.keys().contains( "filter" ) )
        active->setFilter( args.value( "filter" ) );

    return true;
}

QString NavigationUrlRunner::command() const
{
    return "navigate";
}

KIcon NavigationUrlRunner::icon() const
{
    return KIcon( "flag-amarok" );
}


