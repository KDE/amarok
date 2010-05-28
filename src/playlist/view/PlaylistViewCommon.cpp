/****************************************************************************************
 * Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>                           *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009 Louis Bayle <louis.bayle@gmail.com>                               *
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "core/support/Debug.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "TagDialog.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>


Playlist::ViewCommon::ViewCommon()
    : m_stopAfterTrackAction( 0 )
    , m_cueTrackAction( 0 )
    , m_removeTracTrackAction( 0 )
    , m_findInSourceAction( 0 )
{}

Playlist::ViewCommon::~ViewCommon()
{}


void
Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos, bool coverActions )
{
    DEBUG_BLOCK

    KMenu *menu = new KMenu( parent );

    menu->addActions( trackActionsFor( parent, index ) );
    menu->addSeparator();

    if( coverActions )
    {
        QList<QAction*> coverActionsList = coverActionsFor( index );
        if ( !coverActionsList.isEmpty() )
        {
            // there are no cover actions if the song/album is not in the collection
            KMenu *menuCover = new KMenu( i18n( "Album" ), menu );
            menuCover->addActions( coverActionsList );
            menu->addMenu( menuCover );
            menu->addSeparator();
        }
    }

    menu->addActions( multiSourceActionsFor( parent, index ) );
    menu->addSeparator();
    menu->addActions( editActionsFor( parent, index ) );

    menu->exec( pos );
}


QList<QAction*>
Playlist::ViewCommon::actionsFor( QWidget *parent, const QModelIndex *index, bool coverActions )
{
    QList<QAction*> actions;

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );

    actions << trackActionsFor( parent, index );
    actions << separator;

    if( coverActions )
    {
        actions << coverActionsFor( index );
        actions << separator;
    }

    actions << multiSourceActionsFor( parent, index );
    actions << separator;
    actions << editActionsFor( parent, index );

    return actions;
}


QList<QAction*>
Playlist::ViewCommon::trackActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction*> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );

    const bool isQueued = index->data( Playlist::StateRole ).toInt() & Item::Queued;
    const QString queueText = !isQueued ? i18n( "Queue Track" ) : i18n( "Dequeue Track" );

    if( m_cueTrackAction == 0 )
    {
        m_cueTrackAction = new QAction( KIcon( "media-track-queue-amarok" ), queueText, parent );
    }
    else
    {
        m_cueTrackAction->disconnect();
        m_cueTrackAction->setText( queueText );
    }

    if( isQueued )
        QObject::connect( m_cueTrackAction, SIGNAL( triggered() ), parent, SLOT( dequeueSelection() ) );
    else
        QObject::connect( m_cueTrackAction, SIGNAL( triggered() ), parent, SLOT( queueSelection() ) );

    actions << m_cueTrackAction;

    //actions << separator;

    const bool isCurrentTrack = index->data( Playlist::ActiveTrackRole ).toBool();

    if( m_stopAfterTrackAction == 0 )
    {
        m_stopAfterTrackAction = new QAction( KIcon( "media-playback-stop-amarok" ), i18n( "Stop Playing After This Track" ), parent );
        QObject::connect( m_stopAfterTrackAction, SIGNAL( triggered() ), parent, SLOT( stopAfterTrack() ) );
    }
    actions << m_stopAfterTrackAction;

    //actions << separator;

    if( m_removeTracTrackAction == 0 )
    {
        m_removeTracTrackAction = new QAction( KIcon( "media-track-remove-amarok" ), i18n( "Remove From Playlist" ), parent );
        QObject::connect( m_removeTracTrackAction, SIGNAL( triggered() ), parent, SLOT( removeSelection() ) );
    }
    actions << m_removeTracTrackAction;

    //lets see if parent is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( isCurrentTrack )
    {
        //actions << separator;

        QList<QAction*> globalCurrentTrackActions = The::globalCurrentTrackActions()->actions();
        foreach( QAction *action, globalCurrentTrackActions )
            actions << action;

        if( track->hasCapabilityInterface( Capabilities::Capability::CurrentTrackActions ) )
        {
            Capabilities::CurrentTrackActionsCapability *cac = track->create<Capabilities::CurrentTrackActionsCapability>();
            if ( cac )
            {
                QList<QAction *> cActions = cac->customActions();

                foreach( QAction *action, cActions )
                    actions << action;
            }
            delete cac;
        }
    }

    if( track->hasCapabilityInterface( Capabilities::Capability::FindInSource ) )
    {
        if( m_findInSourceAction == 0 )
        {
            m_findInSourceAction = new QAction( KIcon( "edit-find" ), i18n( "Show in Media Sources" ), parent );
            QObject::connect( m_findInSourceAction, SIGNAL( triggered() ), parent, SLOT( findInSource() ) );
        }
        actions << m_findInSourceAction;
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
        Capabilities::CustomActionsCapability *cac = album->create<Capabilities::CustomActionsCapability>();
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
    editAction->setProperty( "popupdropper_svg_id", "edit" );
    QObject::connect( editAction, SIGNAL( triggered() ), parent, SLOT( editTrackInformation() ) );
    actions << editAction;

    return actions;
}
