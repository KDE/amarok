/****************************************************************************************
 * Copyright (c) 2009  Nikolaj Hald Nielsen <nhn@kde.org>                               *
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
 
#include "CurrentTrackToolbar.h"

#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/meta/Meta.h"


CurrentTrackToolbar::CurrentTrackToolbar( QWidget * parent )
    : QToolBar( parent )
{
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    //setIconDimensions( 16 );
    setContentsMargins( 0, 0, 0, 0 );

    EngineController *engine = The::engineController();

    connect( engine, &EngineController::trackChanged,
             this, &CurrentTrackToolbar::handleAddActions );
}

CurrentTrackToolbar::~CurrentTrackToolbar()
{}

void CurrentTrackToolbar::handleAddActions()
{
    clear();

    Meta::TrackPtr track = The::engineController()->currentTrack();

    for( QAction* action : The::globalCurrentTrackActions()->actions() )
        addAction( action );

    if( track )
    {
        QScopedPointer< Capabilities::ActionsCapability > ac( track->create<Capabilities::ActionsCapability>() );
        if( ac )
        {
            QList<QAction *> currentTrackActions = ac->actions();
            foreach( QAction *action, currentTrackActions )
            {
                if( !action->parent() )
                    action->setParent( this );
                addAction( action );
            }
        }

        QScopedPointer< Capabilities::BookmarkThisCapability > btc( track->create<Capabilities::BookmarkThisCapability>() );
        if( btc && btc->bookmarkAction() )
            addAction( btc->bookmarkAction() );
    }
}
