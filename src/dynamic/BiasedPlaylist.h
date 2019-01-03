/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
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

#ifndef AMAROK_BIASEDPLAYLIST_H
#define AMAROK_BIASEDPLAYLIST_H

#include "Bias.h"
#include "DynamicPlaylist.h"
#include "core/meta/forward_declarations.h"

#include "amarok_export.h" // we are exporting it for the tests

#include <ThreadWeaver/Queue>
#include <ThreadWeaver/Job>

#include <QObject>
#include <QPointer>

class QXmlStreamWriter;
class QXmlStreamReader;

namespace Dynamic
{
    class BiasSolver;

    /** A concrete implementation of the DynamicPlaylist that uses a bias structure to determine new tracks.
    */
    class AMAROK_EXPORT BiasedPlaylist : public DynamicPlaylist
    {
        Q_OBJECT

        public:
            /** Creates a new random playlist */
            explicit BiasedPlaylist( QObject *parent = nullptr );

            /** Creates a new playlist from an xml stream */
            explicit BiasedPlaylist( QXmlStreamReader *reader, QObject *parent = nullptr );

            ~BiasedPlaylist();

            void toXml( QXmlStreamWriter *writer ) const override;

            void requestTracks(int) override;

            BiasPtr bias() const;

        public Q_SLOTS:
            void requestAbort() override;

        private Q_SLOTS:
            void solverFinished();
            void biasChanged();
            void biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias );

        private:
            /** Starts the BiasSolver (if not already running) and requests a couple of new tracks. */
            void startSolver( int numRequested );

            /** Returns all the tracks that will come before the newly generated ones. */
            Meta::TrackList getContext();

            /** The bias this playlist uses */
            BiasPtr m_bias;

            /** A currently running BiasSolver */
            BiasSolver* m_solver;

            static const int BUFFER_SIZE;
    };
}

// Q_DECLARE_METATYPE( Dynamic::BiasedPlaylistPtr )

#endif
