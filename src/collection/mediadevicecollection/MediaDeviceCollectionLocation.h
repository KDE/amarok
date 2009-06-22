/*
 *  Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AMAROK_MEDIADEVICECOLLECTIONLOCATION_H
#define AMAROK_MEDIADEVICECOLLECTIONLOCATION_H

#include "CollectionLocation.h"
#include "MediaDeviceHandler.h"

#include "mediadevicecollection_export.h"

#include <QSet>
#include <QMap>
#include <QString>

class MediaDeviceCollection;
class KJob;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollectionLocation : public CollectionLocation
{
    Q_OBJECT
    public:
        MediaDeviceCollectionLocation( MediaDeviceCollection const *collection );
        virtual ~MediaDeviceCollectionLocation();

        virtual QString prettyLocation() const;
        virtual bool isWritable() const;
        virtual bool remove( const Meta::TrackPtr &track );

    protected:

        virtual void getKIOCopyableUrls( const Meta::TrackList &tracks );
        /// Copies these tracks to the Collection using the Handler
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );

    private slots:
        void copyOperationFinished( bool success );

    private:
        MediaDeviceCollection *m_collection;
        MediaDeviceHandler *m_handler;
};

#endif
