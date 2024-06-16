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

#include "MainWindow.h"
#include "amarokconfig.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "browsers/BrowserDock.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "core/support/Debug.h"
#include "playlistmanager/PlaylistManager.h"
#include "services/ServiceBase.h"

NavigationUrlRunner::NavigationUrlRunner()
    : AmarokUrlRunnerBase()
{}


NavigationUrlRunner::~NavigationUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner( this );
}

bool
NavigationUrlRunner::run(const AmarokUrl &url )
{
    DEBUG_BLOCK;

    //get to the correct category
    debug() << "Navigate to path: " << url.path();
    The::mainWindow()->browserDock()->list()->navigate( url.path() );

    BrowserCategory * active =  The::mainWindow()->browserDock()->list()->activeCategoryRecursive();

    QMap<QString, QString> args = url.args();

    if ( args.keys().contains( QStringLiteral("levels") ) )
    {
        QString levelsString = args.value( QStringLiteral("levels") );
        QList<CategoryId::CatMenuId> levels;

        QStringList levelsStringList = levelsString.split( QLatin1Char('-') );

        for( const QString &levelString : levelsStringList ) {
            if( levelString == QLatin1String("genre") )
                levels.append( CategoryId::Genre );
            else if( levelString == QLatin1String("artist") )
                levels.append( CategoryId::Artist );
            else if( levelString == QLatin1String("album") )
                levels.append( CategoryId::Album );
            else if( levelString == QLatin1String("albumartist") )
                levels.append( CategoryId::AlbumArtist );
            else if( levelString == QLatin1String("composer") )
                levels.append( CategoryId::Composer );
            else if( levelString == QLatin1String("year") )
                levels.append( CategoryId::Year );
        }

        active->setLevels( levels );

    }


    //if we are activating the local collection, check if we need to restore "show cover" and "show year"
    //if in the local collection view, also store "show covers" and "show years"
    if( url.path().endsWith( QLatin1String("collections"), Qt::CaseInsensitive ) )
    {
        if ( args.keys().contains( QStringLiteral("show_cover") ) )
        {
            if( args.value( QStringLiteral("show_cover") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowAlbumArt( true );
            else if( args.value( QStringLiteral("show_cover") ).compare( QLatin1String("false"), Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowAlbumArt( false );
        }

        if ( args.keys().contains( QStringLiteral("show_years") ) )
        {
            if( args.value( QStringLiteral("show_years") ).compare( QLatin1String("true"), Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowYears( true );
            else if( args.value( QStringLiteral("show_years") ).compare( QLatin1String("false"), Qt::CaseInsensitive ) == 0 )
                AmarokConfig::setShowYears( false );
        }
    }

    //also set the correct path if we are navigating to the file browser
    if( url.path().endsWith( QLatin1String("files"), Qt::CaseInsensitive ) )
    {
        FileBrowser * fileBrowser = dynamic_cast<FileBrowser *>( The::mainWindow()->browserDock()->list()->activeCategory() );
        if( fileBrowser )
        {
            if( args.keys().contains( QStringLiteral("path") ) )
            {
                fileBrowser->setDir( QUrl::fromUserInput(args.value( QStringLiteral("path") )) );
            }
        }
    }

    if ( args.keys().contains( QStringLiteral("filter") ) )
        active->setFilter( QUrl::fromPercentEncoding(args.value( QStringLiteral("filter") ).toUtf8()) );

    The::mainWindow()->showDock( MainWindow::AmarokDockNavigation );

    return true;
}

QString NavigationUrlRunner::command() const
{
    return QStringLiteral("navigate");
}

QString NavigationUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that affects the view in the browser category", "Navigate" );
}

QIcon NavigationUrlRunner::icon() const
{
    return QIcon::fromTheme( QStringLiteral("flag-amarok") );
}


