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


#include "tagdialog.h"
#include "PlaylistViewCommon.h"
#include "PlaylistModel.h"
#include "meta/CurrentTrackActionsCapability.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>
#include <KAction>

namespace Playlist
{
    namespace ViewCommon
    {

        void trackMenu( QWidget *parent, QModelIndex *index, const QPoint &pos, bool coverActions)
        {
            Meta::TrackPtr track= index->data( Playlist::ItemRole ).value< Playlist::Item* >()->track();
            KMenu *menu = new KMenu( parent );
            KAction *playAction = new KAction( KIcon( "media-playback-start-amarok" ), i18n( "&Play" ), parent );
            //playAction->setData( QVariant( sceneClickPos ) );
            QObject::connect( playAction, SIGNAL( triggered() ), parent, SLOT( playContext() ) );


            menu->addAction( playAction );
            ( menu->addAction( i18n( "Queue Track" ), parent, SLOT( queueItem() ) ) )->setEnabled( false );
            ( menu->addAction( i18n( "Stop Playing After Track" ), parent, SLOT( stopAfterTrack() ) ) )->setEnabled( false );
            menu->addSeparator();
            ( menu->addAction( i18n( "Remove From Playlist" ), parent, SLOT( removeSelection() ) ) )->setEnabled( true );
            menu->addSeparator();
            menu->addAction( i18n( "Edit Track Information" ), parent, SLOT( editTrackInformation() ) );
            menu->addSeparator();



            //Meta::TrackPtr  item = m_model->data(index, TrackRole).value< Meta::TrackPtr >();
            //lets see if this is the currently playing tracks, and if it has CurrentTrackActionsCapability
            if( index->data( Playlist::ActiveTrackRole ).toBool() ) {

                if ( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
                    debug() << "2";
                    Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
                    if( cac )
                    {
                        QList<QAction *> actions = cac->customActions();

                        foreach( QAction *action, actions )
                            menu->addAction( action );
                        menu->addSeparator();
                    }
                }
            }

            if (coverActions)
            {
                bool hasCover = track->album() && track->album()->hasImage();

                QAction *showCoverAction  = menu->addAction( i18n( "Show Fullsize" ), parent , SLOT( showItemImage() ) );
                QAction *fetchCoverAction = menu->addAction( i18n( "Fetch Cover" ), parent , SLOT( fetchItemImage() ) );
                QAction *unsetCoverAction = menu->addAction( i18n( "Unset Cover" ), parent , SLOT( unsetItemImage() ) );

                showCoverAction->setEnabled( hasCover );
                fetchCoverAction->setEnabled( true );
                unsetCoverAction->setEnabled( hasCover );
            }

            menu->exec( pos  );
        }
    }
}

