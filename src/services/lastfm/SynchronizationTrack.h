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

#ifndef SYNCHRONIZATIONTRACK_H
#define SYNCHRONIZATIONTRACK_H

#include "statsyncing/Track.h"

#include <QSemaphore>

/**
 * A class that represents a track in Last.fm users library. Can be used to synchronize
 * users's data with Last.fm through StatSyncing.
 */
class SynchronizationTrack : public QObject, public StatSyncing::Track
{
    Q_OBJECT

    public:
        /**
         * Create track used for synchronization with Last.fm. Can only be called from
         * the main thread.
         */
        SynchronizationTrack( QString artist, QString album, QString name, int playCount, bool useFancyRatingTags );

        virtual QString name() const;
        virtual QString album() const;
        virtual QString artist() const;
        virtual int rating() const;
        virtual void setRating( int rating );
        virtual QDateTime firstPlayed() const;
        virtual QDateTime lastPlayed() const;
        virtual int playCount() const;
        virtual QSet<QString> labels() const;
        virtual void setLabels( const QSet<QString> &labels );
        virtual void commit();

        /**
         * Set tags from Last.fm, parse them into Amarok labels and rating
         */
        void parseAndSaveLastFmTags( const QSet<QString> &tags );

    signals:
        /// hacks to create and start Last.fm queries in main eventloop
        void startTagAddition( QStringList tags );
        void startTagRemoval();

    private slots:
        void slotStartTagAddition( QStringList tags );
        void slotStartTagRemoval();

        void slotTagsAdded();
        void slotTagRemoved();

    private:
        QString m_artist;
        QString m_album;
        QString m_name;
        int m_rating;
        int m_newRating;
        int m_playCount;
        bool m_useFancyRatingTags;
        QSet<QString> m_labels;
        QSet<QString> m_newLabels;
        QSet<QString> m_ratingLabels;

        QStringList m_tagsToRemove;
        QSemaphore m_semaphore;
};

#endif // SYNCHRONIZATIONTRACK_H
