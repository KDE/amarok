/***************************************************************************
 * copyright            : (C) 2008 Bonne Eggletson <b.eggleston@gmail.com>
 * copyright            : (C) 2009 Seb Ruiz <ruiz@kde.org> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with parent program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "PlaylistViewCommon.h"

#include "EngineController.h"
#include "Debug.h"
#include "GlobalCurrentTrackActions.h"
#include "TagDialog.h"
#include "playlist/PlaylistModel.h"
#include "covermanager/CoverFetchingActions.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>

void
Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos, bool coverActions )
{
    DEBUG_BLOCK

    KMenu *menu = new KMenu( parent );
    QList<PopupDropperAction*> actions = actionsFor( parent, index, coverActions );
    foreach( PopupDropperAction *action, actions )
        menu->addAction( action );

    menu->exec( pos );
}

QList<PopupDropperAction*>
Playlist::ViewCommon::actionsFor( QWidget *parent, const QModelIndex *index, bool coverActions )
{
    QList<PopupDropperAction*> actions;

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    PopupDropperAction *separator = new PopupDropperAction( parent );
    separator->setSeparator( true );
    
    const bool isCurrentTrack = index->data( Playlist::ActiveTrackRole ).toBool();
    
    if( isCurrentTrack )
    {
        const bool isPaused = The::engineController()->isPaused();
        const KIcon   icon = isPaused ? KIcon( "media-playback-start-amarok" ) : KIcon( "media-playback-pause-amarok" );
        const QString text = isPaused ? i18n( "&Play" ) : i18n( "&Pause");

        PopupDropperAction *playPauseAction = new PopupDropperAction( icon, text, parent );
        QObject::connect( playPauseAction, SIGNAL( triggered() ), The::engineController(), SLOT( playPause() ) );

        actions << playPauseAction;
    }
    else
    {
        PopupDropperAction *playAction = new PopupDropperAction( KIcon( "media-playback-start-amarok" ), i18n( "&Play" ), parent );
        playAction->setData( index->row() );
        QObject::connect( playAction, SIGNAL( triggered() ), parent, SLOT( playTrack() ) );

        actions << playAction;
    }

    PopupDropperAction *stopAction = new PopupDropperAction( KIcon( "media-playback-stop-amarok" ), i18n( "Stop Playing After This Track" ), parent );
    QObject::connect( stopAction, SIGNAL( triggered() ), parent, SLOT( stopAfterTrack() ) );
    actions << stopAction;
    
    actions << separator;
    
    const bool isQueued = index->data( Playlist::StateRole ).toInt() & Item::Queued;
    const QString queueText = !isQueued ? i18n( "Queue Track" ) : i18n( "Dequeue Track" );
    PopupDropperAction *queueAction = new PopupDropperAction( KIcon( "media-track-queue-amarok" ), queueText, parent );
    if( isQueued )
        QObject::connect( queueAction, SIGNAL( triggered() ), parent, SLOT( dequeueSelection() ) );
    else
        QObject::connect( queueAction, SIGNAL( triggered() ), parent, SLOT( queueSelection() ) );

    actions << queueAction;

    actions << separator;

    PopupDropperAction *removeAction = new PopupDropperAction( KIcon( "media-track-remove-amarok" ), i18n( "Remove From Playlist" ), parent );
    QObject::connect( removeAction, SIGNAL( triggered() ), parent, SLOT( removeSelection() ) );
    actions << removeAction;

    actions << separator;

    //lets see if parent is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( isCurrentTrack )
    {
        QList<QAction*> globalCurrentTrackActions = The::globalCurrentTrackActions()->actions();
        foreach( QAction *action, globalCurrentTrackActions )
            actions << PopupDropperAction::from( action );
        
        if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
        {
            Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
            if ( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                foreach( PopupDropperAction *action, actions )
                    actions << action;
            }
            delete cac;
        }
    }

    actions << separator;

    if ( coverActions )
    {
        Meta::AlbumPtr album = track->album();
        if ( album )
        {
            Meta::CustomActionsCapability *cac = album->as<Meta::CustomActionsCapability>();
            if ( cac )
            {
                QList<PopupDropperAction *> customActions = cac->customActions();

                foreach( PopupDropperAction *customAction, customActions )
                    actions << customAction;
            }
            delete cac;
        }
    }

    actions << separator;
    
    const bool isMultiSource = index->data( Playlist::MultiSourceRole ).toBool();
    if( isMultiSource )
    {
        PopupDropperAction *selectSourceAction = new PopupDropperAction( KIcon( "media-playlist-repeat" ), i18n( "Select Source" ), parent );
        QObject::connect( selectSourceAction, SIGNAL( triggered() ), parent, SLOT( selectSource() ) );

        actions << selectSourceAction;
    }
    
    PopupDropperAction *editAction = new PopupDropperAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), parent );
    QObject::connect( editAction, SIGNAL( triggered() ), parent, SLOT( editTrackInformation() ) );
    actions << editAction;

    return actions;
}

