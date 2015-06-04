/****************************************************************************************
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef METAPROXY_METAPROXYWORKER_H
#define METAPROXY_METAPROXYWORKER_H

#include "core/collections/Collection.h"

#include <ThreadWeaver/Job>

namespace MetaProxy
{
    /**
     * Worker to get real track for MetaProxy::Track. Worker deletes itself somewhere
     * after emitting finishedLookup().
     */
    class Worker : public ThreadWeaver::Job
    {
        Q_OBJECT

        public:
            /**
             * If @param provider is null (the default), all providers registered to
             * CollectionManager are used and a watch for new providers is used.
             * Otherwise the lookup happes just in @param provider and is one-shot.
             */
            explicit Worker( const QUrl &url, Collections::TrackProvider *provider = 0 );

            //TrackForUrlWorker virtual methods
            virtual void run();

        Q_SIGNALS:
            void finishedLookup( Meta::TrackPtr track );

        private Q_SLOTS:
            void slotNewTrackProvider( Collections::TrackProvider *newTrackProvider );
            void slotNewCollection( Collections::Collection *newCollection );
            void slotStepDone();

        private:
            QUrl m_url;
            Collections::TrackProvider *m_provider;
            int m_stepsDoneReceived;
    };
} // namespace MetaProxy

#endif // METAPROXY_METAPROXYWORKER_H
