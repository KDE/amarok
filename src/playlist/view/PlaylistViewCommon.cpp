/****************************************************************************************
 * Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>                           *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009 Louis Bayle <louis.bayle@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistViewCommon.h"

#include "covermanager/CoverFetchingActions.h"
#include "Debug.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "TagDialog.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>

void
Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos, bool coverActions )
{
    DEBUG_BLOCK

    KMenu *menu = new KMenu( parent );

    menu->addActions(trackActionsFor(parent, index));
    menu->addSeparator();

    if( coverActions )
    {
        QList<QAction*> coverActionsList = coverActionsFor( index );
        if (!coverActionsList.isEmpty()) {
            // there are no cover actions if the song/album is not in the collection
            KMenu *menuCover = new KMenu( i18n( "Album" ), menu );
            menuCover->addActions( coverActionsList );
            menu->addMenu(menuCover);
            menu->addSeparator();
        }
    }

    menu->addActions(multiSourceActionsFor(parent, index));
    menu->addSeparator();
    menu->addActions(editActionsFor(parent, index));

    menu->exec( pos );
}


QList<QAction*>
Playlist::ViewCommon::actionsFor( QWidget *parent, const QModelIndex *index, bool coverActions )
{
    QList<QAction*> actions;

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );

    actions << trackActionsFor(parent, index);
    actions << separator;

    if( coverActions )
    {
        actions << coverActionsFor(index);
        actions << separator;
    }

    actions << multiSourceActionsFor(parent, index);
    actions << separator;
    actions << editActionsFor(parent, index);

    return actions;
}


QList<QAction*>
Playlist::ViewCommon::trackActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction*> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );

    const bool isCurrentTrack = index->data( Playlist::ActiveTrackRole ).toBool();

    QAction *stopAction = new QAction( KIcon( "media-playback-stop-amarok" ), i18n( "Stop Playing After This Track" ), parent );
    QObject::connect( stopAction, SIGNAL( triggered() ), parent, SLOT( stopAfterTrack() ) );
    actions << stopAction;

    //actions << separator;

    const bool isQueued = index->data( Playlist::StateRole ).toInt() & Item::Queued;
    const QString queueText = !isQueued ? i18n( "Queue Track" ) : i18n( "Dequeue Track" );
    QAction *queueAction = new QAction( KIcon( "media-track-queue-amarok" ), queueText, parent );
    if( isQueued )
        QObject::connect( queueAction, SIGNAL( triggered() ), parent, SLOT( dequeueSelection() ) );
    else
        QObject::connect( queueAction, SIGNAL( triggered() ), parent, SLOT( queueSelection() ) );

    actions << queueAction;

    //actions << separator;

    QAction *removeAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "Remove From Playlist" ), parent );
    QObject::connect( removeAction, SIGNAL( triggered() ), parent, SLOT( removeSelection() ) );
    actions << removeAction;

    //lets see if parent is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( isCurrentTrack )
    {
        //actions << separator;

        QList<QAction*> globalCurrentTrackActions = The::globalCurrentTrackActions()->actions();
        foreach( QAction *action, globalCurrentTrackActions )
            actions << action;

        if( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
        {
            Meta::CurrentTrackActionsCapability *cac = track->create<Meta::CurrentTrackActionsCapability>();
            if ( cac )
            {
                QList<QAction *> actions = cac->customActions();

                foreach( QAction *action, actions )
                    actions << action;
            }
            delete cac;
        }
    }

    return actions;
}

QList<QAction*>
Playlist::ViewCommon::coverActionsFor( const QModelIndex *index )
{
    QList<QAction*> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    Meta::AlbumPtr album = track->album();
    if( album )
    {
        Meta::CustomActionsCapability *cac = album->create<Meta::CustomActionsCapability>();
        if ( cac )
        {
            QList<QAction *> customActions = cac->customActions();
            foreach( QAction *customAction, customActions )
                actions << customAction;
        }
        delete cac;
    }

    return actions;
}


QList<QAction*>
Playlist::ViewCommon::multiSourceActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction*> actions;
    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    const bool isMultiSource = index->data( Playlist::MultiSourceRole ).toBool();

    if( isMultiSource )
    {
        QAction *selectSourceAction = new QAction( KIcon( "media-playlist-repeat" ), i18n( "Select Source" ), parent );
        QObject::connect( selectSourceAction, SIGNAL( triggered() ), parent, SLOT( selectSource() ) );

        actions << selectSourceAction;
    }

    return actions;
}


QList<QAction*>
Playlist::ViewCommon::editActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction*> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    QAction *editAction = new QAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), parent );
    QObject::connect( editAction, SIGNAL( triggered() ), parent, SLOT( editTrackInformation() ) );
    actions << editAction;

    return actions;
}
