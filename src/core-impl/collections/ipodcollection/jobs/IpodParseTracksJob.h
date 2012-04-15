/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef IPODPARSETRACKSJOB_H
#define IPODPARSETRACKSJOB_H

#include "IpodCollection.h"

#include <ThreadWeaver/Job>

#include <QWeakPointer>


class IpodParseTracksJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        explicit IpodParseTracksJob( IpodCollection *collection );
        virtual void run();

    public slots:
        void abort();

    signals:
        // signals for progress operation:
        void incrementProgress();
        void endProgressOperation( QObject *obj );
        void totalSteps( int steps ); // not used, defined to keep QObject::conect warning quiet

    private:
        QWeakPointer<IpodCollection> m_coll;
        bool m_aborted;
};

#endif // IPODPARSETRACKSJOB_H
