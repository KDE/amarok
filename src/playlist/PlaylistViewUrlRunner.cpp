/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "PlaylistViewUrlRunner.h"

#include "MainWindow.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "core/support/Debug.h"
#include "layouts/LayoutManager.h"
#include "playlist/PlaylistDock.h"
#include "playlist/ProgressiveSearchWidget.h"

#include <KLocalizedString>

#include <QStandardPaths>

namespace Playlist
{

ViewUrlRunner::ViewUrlRunner()
    : AmarokUrlRunnerBase()
{}

ViewUrlRunner::~ViewUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner( this );
}

bool
ViewUrlRunner::run( const AmarokUrl &url )
{
    DEBUG_BLOCK

    const QMap< QString, QString > args = url.args();
    auto playlistDock = The::mainWindow()->playlistDock();

    if( args.keys().contains( QStringLiteral("filter") ) )
    {
        const QString filterExpr = args.value( QStringLiteral("filter") );
        playlistDock->searchWidget()->setCurrentFilter( filterExpr );
        if( args.keys().contains( QStringLiteral("matches") ) )
        {
            const QString onlyMatches = args.value( QStringLiteral("matches") );
            playlistDock->searchWidget()->slotShowOnlyMatches( ( onlyMatches == QStringLiteral( "true" ) ) );
        }
    }
    if( args.keys().contains( QStringLiteral("sort") ) )
    {
        const QString sortPath = args.value( QStringLiteral("sort") );
        playlistDock->sortWidget()->readSortPath( sortPath );
    }

    if( args.keys().contains( QStringLiteral("layout") ) )
    {
        const QString layout = args.value( QStringLiteral("layout") );
        LayoutManager::instance()->setActiveLayout( layout );
    }

    The::mainWindow()->showDock( MainWindow::AmarokDockPlaylist );

    return true;
}

QString
ViewUrlRunner::command() const
{
    return QStringLiteral("playlist");
}

QString
ViewUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that affects the sorting, layout and filtering int he Playlist", "Playlist" );
}

QIcon
ViewUrlRunner::icon() const
{
    return QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/playlist-bookmark-16.png") ) ) );
}

} //namespace Playlist
