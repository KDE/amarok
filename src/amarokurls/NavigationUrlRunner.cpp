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

#include "NavigationUrlRunner.h"

#include <amarokconfig.h>
#include "core/support/Debug.h"

#include "AmarokUrlHandler.h"

#include "MainWindow.h"
#include "PlaylistManager.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
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

        QStringList levelsStringList = levelsString.split( '-' );

        foreach( const QString &levelString, levelsStringList ) {
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


    //if we are activating the local collection, check if we need to restore "show cover" and "show year"
    //if in the local collection view, also store "show covers" and "show years"
    if( url.path().endsWith( "collections", Qt::CaseInsensitive ) )
    {
        if ( args.keys().contains( "show_cover" ) )
        {
            if( args.value( "show_cover" ).compare( "true", Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowAlbumArt( true );
            else if( args.value( "show_cover" ).compare( "false", Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowAlbumArt( false );
        }

        if ( args.keys().contains( "show_years" ) )
        {
            if( args.value( "show_years" ).compare( "true", Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowYears( true );
            else if( args.value( "show_years" ).compare( "false", Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowYears( false );
        }
    }

    //also set the correct path if we are navigating to the file browser
    if( url.path().endsWith( "files", Qt::CaseInsensitive ) )
    {
        FileBrowser * fileBrowser = dynamic_cast<FileBrowser *>( The::mainWindow()->browserWidget()->list()->activeCategory() );
        if( fileBrowser )
        {
            if( args.keys().contains( "path" ) )
            {
                fileBrowser->setDir( args.value( "path" ) );
            }
        }
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


