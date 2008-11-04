/***************************************************************************
 * copyright            : (C) 2008 Bonne Eggletson <b.eggleston@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "PlaylistViewCommon.h"

#include "EngineController.h"
#include "Debug.h"
#include "TagDialog.h"
#include "playlist/PlaylistModel.h"
#include "covermanager/CoverFetchingActions.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "context/popupdropper/PopupDropperAction.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>
#include <KAction>

void
Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos, bool coverActions )
{
    DEBUG_BLOCK

    Meta::TrackPtr track = index->data( Playlist::TrackRole ).value< Meta::TrackPtr >();

    KMenu *menu = new KMenu( parent );

    const bool isCurrentTrack = index->data( Playlist::ActiveTrackRole ).toBool();
    
    if( isCurrentTrack )
    {
        const bool isPaused = The::engineController()->isPaused();
        const KIcon   icon = isPaused ? KIcon( "media-playback-start-amarok" ) : KIcon( "media-playback-pause-amarok" );
        const QString text = isPaused ? i18n( "&Play" ) : i18n( "&Pause");

        KAction *playPauseAction = new KAction( icon, text, parent );
        QObject::connect( playPauseAction, SIGNAL( triggered() ), The::engineController(), SLOT( playPause() ) );

        menu->addAction( playPauseAction );
    }
    else
    {
        KAction *playAction = new KAction( KIcon( "media-playback-start-amarok" ), i18n( "&Play" ), parent );
        playAction->setData( index->row() );
        QObject::connect( playAction, SIGNAL( triggered() ), parent, SLOT( playTrack() ) );

        menu->addAction( playAction );
    }
    
    //( menu->addAction( KIcon( "media-track-queue-amarok" ), i18n( "Queue Track" ), parent, SLOT( queueItem() ) ) )->setEnabled( false );
    //( menu->addAction( KIcon( "media-playback-stop-amarok" ), i18n( "Stop Playing After Track" ), parent, SLOT( stopAfterTrack() ) ) )->setEnabled( false );
    
    menu->addSeparator();
    ( menu->addAction( KIcon( "media-track-remove-amarok" ), i18n( "Remove From Playlist" ), parent, SLOT( removeSelection() ) ) )->setEnabled( true );
    menu->addSeparator();
    menu->addAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), parent, SLOT( editTrackInformation() ) );

    //lets see if this is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( isCurrentTrack )
    {
        if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
        {
            Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
            if ( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                foreach( PopupDropperAction *action, actions )
                menu->addAction( action );
            }
        }
    }
    menu->addSeparator();

    if ( coverActions )
    {
        Meta::AlbumPtr album = track->album();
        if ( album )
        {
            Meta::CustomActionsCapability *cac = album->as<Meta::CustomActionsCapability>();
            if ( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                menu->addSeparator();
                foreach( PopupDropperAction *action, actions )
                    menu->addAction( action );
            }
        }
    }

    menu->exec( pos );
}

