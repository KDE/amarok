/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#ifndef MUSICBRAINZXMLPARSER_H
#define MUSICBRAINZXMLPARSER_H

#include <ThreadWeaver/Job>

#include <QDomDocument>
#include <QStringList>
#include <QVariantMap>

class MusicBrainzXmlParser : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        enum {
            TrackList,
            ReleaseGroup
        };

        explicit MusicBrainzXmlParser( const QString &doc );

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override;

        int type();

        QMap<QString, QVariantMap> tracks;
        QMap<QString, QString> artists;
        QMap<QString, QVariantMap> releases;
        QMap<QString, QVariantMap> releaseGroups;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        QStringList parseRecordingList( const QDomElement &e );
        QString parseRecording( const QDomElement &e );

        QStringList parseReleaseList( const QDomElement &e );
        QString parseRelease( const QDomElement &e );

        QMultiMap<QString, QVariant> parseMediumList( const QDomElement &e );
        QMultiMap<QString, QVariant> parseMedium( const QDomElement &e );
        QVariantMap parseTrackList( const QDomElement &e );
        QVariantMap parseTrack( const QDomElement &e );

        QString parseReleaseGroup( const QDomElement &e );

        QStringList parseArtist( const QDomElement &e );

        QDomDocument m_doc;

        int m_type;

        QVariantMap m_currentTrackInfo;

    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

};

#endif // MUSICBRAINZXMLPARSER_H
