/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "PlaylistViewUrlRunner.h"

#include "AmarokUrlHandler.h"
#include "Debug.h"
#include "MainWindow.h"
#include "PlaylistWidget.h"
#include "ProgressiveSearchWidget.h"
#include "PlaylistGroupingAction.h"
#include "layouts/LayoutManager.h"

#include <QList>
#include <QStringList>
#include <QActionGroup>

namespace Playlist
{

ViewUrlRunner::ViewUrlRunner()
    : AmarokUrlRunnerBase()
{
}

ViewUrlRunner::~ViewUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner( this );
}

bool
ViewUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK

    QMap< QString, QString > args = url.args();
    Widget * playlistWidget = The::mainWindow()->playlistWidget();

    if( args.keys().contains( "filter" ) )
    {
        QString filterExpr = args.value( "filter" );
        playlistWidget->searchWidget()->setCurrentFilter( filterExpr );
        if( args.keys().contains( "matches" ) )
        {
            QString onlyMatches = args.value( "matches" );
            playlistWidget->searchWidget()->slotShowOnlyMatches( ( onlyMatches == QString( "true" ) ) );
        }
    }
    if( args.keys().contains( "sort" ) )
    {
        playlistWidget->sortWidget()->trimToLevel();

        QString sortPath = args.value( "sort" );

        QStringList levels = sortPath.split( "-" );
        foreach( QString level, levels )
        {
            if( level == QString( "Random" ) )
            {
                playlistWidget->sortWidget()->addLevel( level );
                break;
            }
            QStringList levelParts = level.split( "_" );
            if( levelParts.length() > 2 )
                warning() << "Playlist view URL parse error: Invalid sort level " << level;
            if( levelParts.at( 1 ) == QString( "asc" ) )
                playlistWidget->sortWidget()->addLevel( levelParts.at( 0 ), Qt::AscendingOrder );
            else if( levelParts.at( 1 ) == QString( "des" ) )
                playlistWidget->sortWidget()->addLevel( levelParts.at( 0 ), Qt::DescendingOrder );
            else
                warning() << "Playlist view URL parse error: Invalid sort order for level " << level;
        }
    }
    if( args.keys().contains( "group" ) )
    {
        QString groupingCategory = args.value( "group" );
        QList< QAction * > actions = playlistWidget->groupingAction()->groupingActionGroup()->actions();
        foreach( QAction *action, actions )
        {
            if( action->data().toString() == groupingCategory )
            {
                action->trigger();
                break;
            }
        }
    }
    if( args.keys().contains( "layout" ) )
    {
        QString layout = args.value( "layout" );
        LayoutManager::instance()->setActiveLayout( layout );
    }

    return true;
}

QString
ViewUrlRunner::command() const
{
    return "playlist";
}

KIcon
ViewUrlRunner::icon() const
{
    return KIcon( "flag-amarok" );
}

} //namespace Playlist
