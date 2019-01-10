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

#define DEBUG_PREFIX "UpnpMemoryQueryMaker"

#include "UpnpMemoryQueryMaker.h"

#include <QWeakPointer>

#include "core/support/Debug.h"
#include "UpnpBrowseCollection.h"

namespace Collections {

bool UpnpMemoryQueryMaker::m_firstRun = true;
UpnpMemoryQueryMaker::UpnpMemoryQueryMaker( const QWeakPointer<MemoryCollection> &mc, const QString &collectionId )
    : MemoryQueryMaker( mc, collectionId )
{
}

UpnpMemoryQueryMaker::~UpnpMemoryQueryMaker()
{
}

void
UpnpMemoryQueryMaker::run()
{
DEBUG_BLOCK
    if( m_firstRun ) {
        m_firstRun = false;
        Q_EMIT startFullScan();
    }
    MemoryQueryMaker::run();
}

} //namespace Collections

