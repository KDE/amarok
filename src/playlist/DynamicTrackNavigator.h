/***************************************************************************
 * copyright            : (C) 2008 Daniel Jones <danielcjones@gmail.com> 
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


#ifndef DYNAMICTRACKNAVIGATOR_H
#define DYNAMICTRACKNAVIGATOR_H

#include "DynamicPlaylist.h"
#include "PlaylistModel.h"
#include "SimpleTrackNavigator.h"

#include <QMutex>


namespace Playlist {

class Model;

    /**
     * A navigator that implements 'dynamic mode', which is a sort of never
     * ending queue of tracks.
     */
    // TODO: this should inherit TrackNavigator and do it's work asynchronously.
    class DynamicTrackNavigator : public SimpleTrackNavigator
    {
        Q_OBJECT

        public:
            DynamicTrackNavigator( Model* m, Dynamic::DynamicPlaylistPtr p ) ;
            ~DynamicTrackNavigator();

            void appendUpcoming();

        private slots:
            void activePlaylistChanged();
            void activeRowChanged( int from, int to );
            void repopulate();

        private:
            int nextRow();
            int lastRow();

            void setAsUpcoming( int row );
            void setAsPlayed( int row );
            void markPlayed();

            void removePlayed();
            
            bool m_abortRequested;

            Dynamic::DynamicPlaylistPtr m_playlist;

            QMutex m_mutex;
    };
}


#endif

