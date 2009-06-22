/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include <QSet>
#include <QMap>
#include <QString>

class MediaDeviceCollection;
class KJob;

class MediaDeviceCollectionLocation : public CollectionLocation
{
    Q_OBJECT
    public:
        MediaDeviceCollectionLocation( MediaDeviceCollection const *collection );
        virtual ~MediaDeviceCollectionLocation();

        virtual QString prettyLocation() const;
        virtual bool isWritable() const;
        virtual bool remove( const Meta::TrackPtr &track );

    protected:
        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources );

    signals:
        void addDevice( const QString &udi );

    private slots:
        void slotJobFinished( KJob *job );
        void copyOperationFinished( bool success );

    private:

        void insertTracks( const QMap<Meta::TrackPtr, QString> &trackMap );
        //QMap<QString, uint> updatedMtime( const QStringList &urls );
        void insertStatistics( const QMap<Meta::TrackPtr, QString> &trackMap );
        //called by the destination location if it detects that we are organizing the collection
        //because the source does not need to remove the files, that was done by the destination
     //   void movedByDestination( const Meta::TrackPtr &track, bool removeFromDatabase );

        MediaDeviceCollection *m_collection;
        QMap<Meta::TrackPtr, QString> m_destinations;
        bool m_removeSources;    //used by the destination to remember the value, needed in copyurlsToCollection
        bool m_overwriteFiles;
        QSet<KJob*> m_jobs;
        QStringList m_ignoredDestinations;  //these tracks were not copied/moved because source and destination url were the same
        QMap<Meta::TrackPtr, bool> m_tracksRemovedByDestination;    //used in the source when organizing the collection
};

#endif
