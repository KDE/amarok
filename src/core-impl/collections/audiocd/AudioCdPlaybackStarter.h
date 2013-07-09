/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef AMAROK_AUDIOCDPLAYBACKSTARTER_H
#define AMAROK_AUDIOCDPLAYBACKSTARTER_H

#include "core-impl/collections/support/CollectionManager.h"
#include <QTimer>


/**
 * Organizes adding the CD to playlist when amarok
 * was started with --cdplay flag
 */
class AudioCdPlaybackStarter: public QObject
{
    Q_OBJECT

    public:
        AudioCdPlaybackStarter();

    private slots:
        /** If changed collection is AudioCd then launches its playback */
        void slotCollectionChanged( Collections::Collection* );
    private:
        /** Stops playback;
         *  Clears current playlist;
         *  Inserts tracks from AudioCd to current playlist
         */
        void startPlayback( Collections::Collection* audiocd_collection );
        QTimer m_deleteTimer;
        int m_deleteTimeout;
};

#endif
