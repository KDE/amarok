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

#include <core/collections/support/TrackForUrlWorker.h>
#include <core/collections/Collection.h>

namespace MetaProxy {

class Worker : public Amarok::TrackForUrlWorker
{
    Q_OBJECT
    public:
        explicit Worker( const KUrl &url );

        //TrackForUrlWorker virtual methods
        virtual void run();

    private slots:
        void slotNewTrackProvider( Collections::TrackProvider *newTrackProvider );
        void slotNewCollection( Collections::Collection *newCollection );

};

} // namespace MetaProxy

#endif // METAPROXY_METAPROXYWORKER_H
