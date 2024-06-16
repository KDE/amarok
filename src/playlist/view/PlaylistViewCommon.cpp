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

#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "covermanager/CoverFetchingActions.h"
#include "dialogs/TagDialog.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/view/listview/PrettyListView.h"

#include <QMenu>

#include <QObject>
#include <QModelIndex>

Playlist::ViewCommon::ViewCommon()
    : m_stopAfterTrackAction( nullptr )
    , m_cueTrackAction( nullptr )
    , m_removeTracTrackAction( nullptr )
    , m_findInSourceAction( nullptr )
{}

Playlist::ViewCommon::~ViewCommon()
{}


void
Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos )
{
    DEBUG_BLOCK

    QMenu *menu = new QMenu( parent );

    menu->addActions( parentCheckActions( parent, trackActionsFor( parent, index ) ) );
    menu->addSeparator();

    QList<QAction *> albumActionsList = parentCheckActions( parent, albumActionsFor( index ) );
    if( !albumActionsList.isEmpty() )
    {
        // there are no cover actions if the song/album is not in the collection
        QMenu *menuCover = new QMenu( i18n( "Album" ), menu );
        menuCover->addActions( albumActionsList );
        menuCover->setIcon( QIcon::fromTheme( QStringLiteral("filename-album-amarok") ) );
        menu->addMenu( menuCover );
        menu->addSeparator();
    }

    menu->addActions( parentCheckActions( parent, multiSourceActionsFor( parent, index ) ) );
    menu->addSeparator();
    menu->addActions( parentCheckActions( parent, editActionsFor( parent, index ) ) );

    menu->exec( pos );
    delete menu;
}


QList<QAction *>
Playlist::ViewCommon::actionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction *> actions;

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );

    actions << parentCheckActions( parent, trackActionsFor( parent, index ) );
    actions << separator;

    QList<QAction *> albumActionsList = parentCheckActions( parent, albumActionsFor( index ) );
    if( !albumActionsList.isEmpty() )
    {
        actions << albumActionsList;
        actions << separator;
    }

    actions << parentCheckActions( parent, multiSourceActionsFor( parent, index ) );
    actions << separator;
    actions << parentCheckActions( parent, editActionsFor( parent, index ) );

    return actions;
}


QList<QAction *>
Playlist::ViewCommon::trackActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction *> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    QAction *separator = new QAction( parent );
    separator->setSeparator( true );
    const bool isQueued = index->data( Playlist::QueuePositionRole ).toInt() != 0;
    const QString queueText = !isQueued ? i18n( "Queue Track" ) : i18n( "Dequeue Track" );

    //display "Queue track" option only if the track is playable
    if( track->isPlayable() )
    {

        if( m_cueTrackAction == nullptr )
        {
            m_cueTrackAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-queue-amarok") ), queueText, parent );
        }
        else
        {
            m_cueTrackAction->disconnect();
            m_cueTrackAction->setText( queueText );
        }

        if( auto p = static_cast<Playlist::PrettyListView*>(parent))
        {
            if( isQueued )
                QObject::connect( m_cueTrackAction, &QAction::triggered,
                                  p, &Playlist::PrettyListView::dequeueSelection );
            else
                QObject::connect( m_cueTrackAction, &QAction::triggered,
                                  p, &Playlist::PrettyListView::queueSelection );
        }

        actions << m_cueTrackAction;

    }

    //actions << separator;

    const bool isCurrentTrack = index->data( Playlist::ActiveTrackRole ).toBool();

    //display "Stop after this track" option only if track is playable. not sure if this check is really needed
    if( track->isPlayable() )
    {
        if( m_stopAfterTrackAction == nullptr )
        {
            m_stopAfterTrackAction = new QAction( QIcon::fromTheme( QStringLiteral("media-playback-stop-amarok") ),
                                                  i18n( "Stop Playing After This Track" ), parent );

            if ( auto p = static_cast<Playlist::PrettyListView*>(parent) )
                QObject::connect( m_stopAfterTrackAction, &QAction::triggered,
                                  p, &Playlist::PrettyListView::stopAfterTrack );
        }
        actions << m_stopAfterTrackAction;
    }

    //actions << separator;

    if( m_removeTracTrackAction == nullptr )
    {
        m_removeTracTrackAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-remove-amarok") ),
                                               i18n( "Remove From Playlist" ), parent );

        if ( auto p = static_cast<Playlist::PrettyListView*>(parent) )
            QObject::connect( m_removeTracTrackAction, &QAction::triggered,
                              p, &Playlist::PrettyListView::removeSelection );
    }
    actions << m_removeTracTrackAction;

    //lets see if parent is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( isCurrentTrack )
    {
        //actions << separator;

        QList<QAction *> globalCurrentTrackActions = The::globalCurrentTrackActions()->actions();
        for( QAction *action : globalCurrentTrackActions )
            actions << action;

        if( track->has<Capabilities::ActionsCapability>() )
        {
            QScopedPointer<Capabilities::ActionsCapability>
                    ac( track->create<Capabilities::ActionsCapability>() );
            if ( ac )
                actions.append( ac->actions() );
        }
    }

    if( track->has<Capabilities::FindInSourceCapability>() )
    {
        if( m_findInSourceAction == nullptr )
        {
            m_findInSourceAction = new QAction( QIcon::fromTheme( QStringLiteral("edit-find") ),
                                                i18n( "Show in Media Sources" ), parent );

            if( auto p = static_cast<Playlist::PrettyListView*>(parent) )
                QObject::connect( m_findInSourceAction, &QAction::triggered,
                                  p, &Playlist::PrettyListView::findInSource );
        }
        actions << m_findInSourceAction;
    }

    return actions;
}

QList<QAction *>
Playlist::ViewCommon::albumActionsFor( const QModelIndex *index )
{
    QList<QAction *> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    Meta::AlbumPtr album = track->album();
    if( album )
    {
        QScopedPointer<Capabilities::ActionsCapability>
                ac( album->create<Capabilities::ActionsCapability>() );
        if( ac )
            actions.append( ac->actions() );
    }

    return actions;
}


QList<QAction *>
Playlist::ViewCommon::multiSourceActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction *> actions;
    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    const bool isMultiSource = index->data( Playlist::MultiSourceRole ).toBool();

    if( isMultiSource )
    {
        QAction *selectSourceAction = new QAction( QIcon::fromTheme( QStringLiteral("media-playlist-repeat") ),
                                                   i18n( "Select Source" ), parent );

        if( auto p = static_cast<Playlist::PrettyListView*>(parent) )
            QObject::connect( selectSourceAction, &QAction::triggered, p, &Playlist::PrettyListView::selectSource );

        actions << selectSourceAction;
    }

    return actions;
}


QList<QAction *>
Playlist::ViewCommon::editActionsFor( QWidget *parent, const QModelIndex *index )
{
    QList<QAction *> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    QAction *editAction = new QAction( QIcon::fromTheme( QStringLiteral("media-track-edit-amarok") ),
                                       i18n( "Edit Track Details" ), parent );
    editAction->setProperty( "popupdropper_svg_id", "edit" );

    if( auto p = static_cast<Playlist::PrettyListView*>(parent) )
        QObject::connect( editAction, &QAction::triggered, p, &Playlist::PrettyListView::editTrackInformation );

    actions << editAction;

    return actions;
}

QList<QAction *>
Playlist::ViewCommon::parentCheckActions( QObject *parent, QList<QAction *> actions )
{
    for( QAction *action : actions )
    {
        if( !action->parent() )
            action->setParent( parent );
    }

    return actions;
}
