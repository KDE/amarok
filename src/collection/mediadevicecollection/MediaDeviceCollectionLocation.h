/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef AMAROK_MEDIADEVICECOLLECTIONLOCATION_H
#define AMAROK_MEDIADEVICECOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"
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
        virtual void showDestinationDialog( const Meta::TrackList &tracks, bool removeSources );
        virtual void getKIOCopyableUrls( const Meta::TrackList &tracks );

        /// Copies these tracks to the Collection using the Handler
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );


    private slots:
        void slotDialogAccepted();
        void slotDialogRejected();
        void copyOperationFinished( bool success );
        void removeOperationFinished();

    private:
        QMap<Meta::TrackPtr, QString> m_destinations;
        MediaDeviceCollection *m_collection;
        Meta::MediaDeviceHandler *m_handler;
};

#endif
