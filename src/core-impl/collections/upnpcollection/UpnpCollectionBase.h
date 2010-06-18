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

#ifndef UPNPCOLLECTIONBASE_H
#define UPNPCOLLECTIONBASE_H

#include "core/collections/Collection.h"

#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QtGlobal>
#include <QSharedPointer>

#include <KIcon>
#include <KDirNotify>

namespace KIO {
  class Job;
  class ListJob;
}
class KJob;

class QTimer;

namespace Collections {

class UpnpMemoryQueryMaker;

/**
 * UPnP Collections are of two types.
 * If a MediaServer only supports the Browse() action
 * a directory walking, recursive listing UpnpBrowseCollection
 * is used.
 * If a server also supports Search(), a more efficient,
 * UpnpSearchCollection is used.
 * Certain things are common to both, removal,
 * track creation from the UDSEntry, collection identification,
 */
class UpnpCollectionBase : public Collections::Collection
{
  Q_OBJECT
  public:
    UpnpCollectionBase();
    void removeCollection() { emit remove(); }
};

} //namespace Collections

#endif
