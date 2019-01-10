/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNP_MEMORYQUERYMAKER_H
#define UPNP_MEMORYQUERYMAKER_H

#include "amarok_export.h"

#include "MemoryCollection.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

#include <QWeakPointer>

namespace Collections {

class UpnpMemoryQueryMaker : public MemoryQueryMaker
{
    Q_OBJECT
    public:
        UpnpMemoryQueryMaker( const QWeakPointer<MemoryCollection> &mc, const QString &collectionId );
        virtual ~UpnpMemoryQueryMaker();

        void run() override;

    Q_SIGNALS:
        void startFullScan();

    private:
        /*
         * On the first run we need to tell the collection
         * to start scanning. So we need to know when its
         * the first run. Is this crude? I don't know now
         */
        static bool m_firstRun;

};

} //namespace Collections

#endif
