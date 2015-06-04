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

        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                           const Transcoding::Configuration &configuration );
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );

    protected Q_SLOTS:
        void slotRemoveOperationFinished(); // hides intentionally parent methods

    private Q_SLOTS:
        /**
         * Needed for removal of source tracks during move operation
         */
        void slotTrackTransferred( const QUrl &sourceTrackUrl );

    private:
        UmsCollection *m_umsCollection;
        QHash<QUrl, Meta::TrackPtr> m_sourceUrlToTrackMap;
};

class UmsTransferJob : public KCompositeJob
{
    Q_OBJECT
    public:
        UmsTransferJob( UmsCollectionLocation *location, const Transcoding::Configuration &configuration );

        void addCopy( const QUrl &from, const QUrl &to );
        void addTranscode( const QUrl &from, const QUrl &to );
        virtual void start();

    Q_SIGNALS:
        void sourceFileTransferDone( QUrl source );
        void fileTransferDone( QUrl destination );

    public Q_SLOTS:
        void slotCancel();

    private Q_SLOTS:
        void startNextJob();
        void slotChildJobPercent( KJob *job, unsigned long percentage );

        //reimplemented from KCompositeJob
        virtual void slotResult( KJob *job );

    private:
        UmsCollectionLocation *m_location;
        Transcoding::Configuration m_transcodingConfiguration;
        bool m_abort;

        typedef QPair<QUrl,QUrl> KUrlPair;
        QList<KUrlPair> m_copyList;
        QList<KUrlPair> m_transcodeList;
        int m_totalTracks; // total number of tracks in whole transfer
};

#endif // UMSCOLLECTIONLOCATION_H
