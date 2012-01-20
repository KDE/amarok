/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef UMSCOLLECTIONLOCATION_H
#define UMSCOLLECTIONLOCATION_H

#include "core/collections/CollectionLocation.h"
#include "UmsCollection.h"

#include <KJob>
#include <KCompositeJob>

#include <QList>
#include <QPair>

class UmsTransferJob;

class UmsCollectionLocation : public Collections::CollectionLocation
{
        Q_OBJECT
    public:
        UmsCollectionLocation( UmsCollection *umsCollection );
        ~UmsCollectionLocation();

        /* CollectionLocation methods */
        virtual QString prettyLocation() const;
        virtual QStringList actualLocation() const;
        virtual bool isWritable() const;
        virtual bool isOrganizable() const;

        virtual bool remove( const Meta::TrackPtr &track );
        virtual bool insert( const Meta::TrackPtr &track, const QString &url );

        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, KUrl> &sources,
            const Transcoding::Configuration &configuration = Transcoding::Configuration() );
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );

    private:
        UmsCollection *m_umsCollection;
};

class UmsTransferJob : public KCompositeJob
{
    Q_OBJECT
    public:
        UmsTransferJob( UmsCollectionLocation *location );

        virtual void addCopy( const KUrl &from, const KUrl &to );
        virtual void start();

    signals:
        void fileTransferDone( KUrl destination );
        void percent( KJob *job, unsigned long percent );
        void infoMessage( KJob *job, QString plain, QString rich );

    public slots:
        void slotCancel();

    private slots:
        void startNextJob();
        void slotChildJobPercent( KJob *job, unsigned long percentage );

        //reimplemented from KCompositeJob
        virtual void slotResult( KJob *job );

    private:
        UmsCollectionLocation *m_location;
        bool m_cancled;

        typedef QPair<KUrl,KUrl> KUrlPair;
        QList<KUrlPair> m_transferList;
};

#endif // UMSCOLLECTIONLOCATION_H
