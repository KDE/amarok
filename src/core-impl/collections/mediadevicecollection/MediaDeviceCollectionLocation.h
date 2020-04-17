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
#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandler.h"
#include "core-impl/collections/mediadevicecollection/support/mediadevicecollection_export.h"

#include <QMap>
#include <QSet>
#include <QString>


namespace Collections {

class MediaDeviceCollection;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollectionLocation : public CollectionLocation
{
    Q_OBJECT
    public:
        explicit MediaDeviceCollectionLocation( MediaDeviceCollection *collection );
        ~MediaDeviceCollectionLocation() override;

        QString prettyLocation() const override;
        bool isWritable() const override;

    protected:
        void getKIOCopyableUrls( const Meta::TrackList &tracks ) override;

        /// Copies these tracks to the Collection using the Handler
        void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                           const Transcoding::Configuration &configuration ) override;

        void removeUrlsFromCollection( const Meta::TrackList &sources ) override;


    private Q_SLOTS:
        void copyOperationFinished( bool success );
        void removeOperationFinished();

    private:
        MediaDeviceCollection *m_collection;
        Meta::MediaDeviceHandler *m_handler;
};

} //namespace Collections

#endif
