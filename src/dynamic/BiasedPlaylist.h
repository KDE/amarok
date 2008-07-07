/***************************************************************************
 * copyright         : (C) 2008 Daniel Caleb Jones <danielcjones@gmail.com>
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

#ifndef AMAROK_BIASEDPLAYLIST_H
#define AMAROK_BIASEDPLAYLIST_H

#include "Bias.h"
#include "BiasSolver.h"
#include "DynamicPlaylist.h"
#include "Meta.h"
#include "RandomPlaylist.h"

#include <QObject>
#include <QEventLoop>

namespace Dynamic
{
    class BiasedPlaylist : public DynamicPlaylist
    {
        Q_OBJECT

        public:
            BiasedPlaylist( QString title, QList<Bias*> );
            BiasedPlaylist( QString title, QList<Bias*>, Collection* m_collection );

            ~BiasedPlaylist();

            void setContext( Meta::TrackList );

            Meta::TrackPtr getTrack();
            void recalculate();

            QList<Bias*>& biases();
            const QList<Bias*>& biases() const;

        private slots:
            void solverFinished( ThreadWeaver::Job* );

        private:
            void startSolver();
            void getContext();

            Meta::TrackList m_context;
            Meta::TrackList m_buffer;
            Meta::TrackList m_backbuffer;

            QList<Bias*> m_biases;

            BiasSolver* m_solver;
            QEventLoop  m_solverLoop;

            RandomPlaylist m_randomSource;

            static const int BUFFER_SIZE;
    };

    typedef KSharedPtr<BiasedPlaylist> BiasedPlaylistPtr;
}

Q_DECLARE_METATYPE( Dynamic::BiasedPlaylistPtr )

#endif

