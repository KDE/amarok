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

#ifndef AMAROK_BIASSOLVER_H
#define AMAROK_BIASSOLVER_H

#include "Bias.h"
#include "Meta.h"
#include "TrackSet.h"

#include <threadweaver/Job.h>

#include <QWaitCondition>

namespace Dynamic
{

    /**
     * A class to implement the optimization algorithm used to generate
     * playlists from a set of biases. The method used here is called 
     * "Simulated Annealing". Read about it before trying to work on this.
     */
    class BiasSolver : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            BiasSolver( 
                    int n, QList<Bias*> biases,
                    Meta::TrackList context = Meta::TrackList() );

            Meta::TrackList solution();
            void requestAbort();
            bool success() const;

            void doWork();

            static void setUniverseCollection( Collection* );
            static const QList<QByteArray>& universe();
            static void outdateUniverse();

        protected:
            void run();

        signals:
            void statusUpdate( int progress );

        private slots:
            void biasUpdated();
            void universeResults( QString collectionId, QStringList );
            void universeUpdated();
            void iterate( Meta::TrackPtr mutation );

        private:
            void computeDomain();
            void updateUniverse();
            double energy();
            double recalculateEnergy( Meta::TrackPtr mutation, int mutationPos );
            Meta::TrackPtr trackForUid( const QByteArray& );
            Meta::TrackPtr getMutation();
            Meta::TrackPtr getRandomTrack( const QList<QByteArray>& subset );


            bool generateInitialPlaylist(); //! returns true if the initial is known to be optimal

            QList<Bias*>  m_biases;     //! current energy for the whole system
            QList<double> m_biasEnergy; //! current energy of each indivial bias
            QList<double> m_biasMutationEnergy; //! individual bias energy for the mutation

            int m_n;                    //! size of playlist to generate
            double m_T;                 //! temperature
            double m_E;                 //! energy
            Meta::TrackList m_playlist; //! playlist being generated
            Meta::TrackList m_context;  //! tracks that precede the playlist

            int m_pendingBiasUpdates;
            QMutex m_biasMutex;
            QWaitCondition m_biasUpdateWaitCond;

            QList<QByteArray> m_domain;

            // set by computeDomain, used by generateInitialPlaylist
            QList<GlobalBias*> m_feasibleGlobalBiases;
            QList<TrackSet> m_feasibleGlobalBiasSets;

            bool m_abortRequested;

            /** A list of every track in the collection. This ends up making Amarok
             * use quite a bit of memory. Hopefully a better idea will come along. */
            static QList<QByteArray>  s_universe;
            static QMutex             s_universeMutex;
            static QueryMaker*        s_universeQuery;
            static Collection*        s_universeCollection;
            static bool               s_universeOutdated;
            static unsigned int       s_uidUrlProtocolPrefixLength;

            static const int    ITERATION_LIMIT; //! give up after this many iterations
            static const double INITIAL_TEMPERATURE;
            static const double COOLING_RATE;
    };

}

#endif
