/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DAAPREADER_H
#define DAAPREADER_H

#include <core-impl/collections/support/MemoryCollection.h>

#include <QObject>

#include <QUrl>
#include <ThreadWeaver/Job>

class QString;

namespace Collections {
    class DaapCollection;
}


namespace Daap
{
    typedef QMap<QString, QVariant>    Map;

    enum ContentTypes { INVALID = 0, CHAR = 1, SHORT = 3, LONG = 5, LONGLONG = 7,
                        STRING = 9, DATE = 10, DVERSION = 11, CONTAINER = 12 };

    struct Code
    {
        Code() : type(INVALID) { }
        Code( const QString& nName, ContentTypes nType ) : name( nName ), type( nType ) { }
        ~Code() { }
        QString name;
        ContentTypes type;
    };



    /**
        The nucleus of the DAAP client; composes queries and parses responses.
        @author Ian Monroe <ian@monroe.nu>
    */
    class Reader : public QObject
    {
        Q_OBJECT

        public:
            Reader( Collections::DaapCollection *mc, const QString& host, quint16 port, const QString& password, QObject* parent, const char* name );
           ~Reader() override;

            //QPtrList<MetaBundle> getSongList();
            enum Options { SESSION_ID = 1, SERVER_VERSION = 2  };
            void loginRequest();
            void logoutRequest();

            int sessionId() const { return m_sessionId; }
            QString host() const { return m_host; }
            quint16 port() const { return m_port; }

            bool parseSongList( const QByteArray &data, bool set_collection = false);
        public Q_SLOTS:
            void logoutRequestFinished();
            void contentCodesReceived();
            void loginHeaderReceived();
            void loginFinished();
            void updateFinished();
            void databaseIdFinished();
            void songListFinished();
            void fetchingError( const QString& error );

        Q_SIGNALS:
            //void daapBundles( const QString& host, Daap::SongList bundles );
            void httpError( const QString& );
            void passwordRequired();

        private:
            /**
            * Make a map-vector tree out of the DAAP binary result
            * @param raw stream of DAAP reply
            */
            Map parse( QDataStream &raw);
            static void addElement( Map &parentMap, char* tag, const QVariant &element ); //!< supporter function for parse
            static quint32 getTagAndLength( QDataStream &raw, char tag[5] );
            QVariant readTagData(QDataStream &, char[5], quint32);
            void addTrack( const QString& itemId, const QString& title, const QString& artist, const QString& composer,
                           const QString& comment, const QString& album, const QString& genre, int year,
                           const QString& format, qint32 trackNumber, qint32 songTime );

            QMap<QString, Code> m_codes;

            Collections::DaapCollection *m_memColl;
            QString m_host;
            quint16 m_port;
            QString m_loginString;
            QString m_databaseId;
            int m_sessionId;
            QString m_password;
            TrackMap m_trackMap;
            ArtistMap m_artistMap;
            AlbumMap m_albumMap;
            GenreMap m_genreMap;
            ComposerMap m_composerMap;
            YearMap m_yearMap;
    };

    class WorkerThread : public QObject, public ThreadWeaver::Job
    {
        Q_OBJECT
        public:
            WorkerThread( const QByteArray &data, Reader* reader, Collections::DaapCollection *coll );
            ~WorkerThread() override;

            bool success() const override;

        protected:
            void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
            void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
            void run(ThreadWeaver::JobPointer self=QSharedPointer<WorkerThread>(), ThreadWeaver::Thread *thread = nullptr) override;

        Q_SIGNALS:
            /** This signal is emitted when this job is being processed by a thread. */
            void started(ThreadWeaver::JobPointer);
            /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
            void done(ThreadWeaver::JobPointer);
            /** This job has failed.
             * This signal is emitted when success() returns false after the job is executed. */
            void failed(ThreadWeaver::JobPointer);

        private:
            bool m_success;
            QByteArray m_data;
            Reader *m_reader;
    };

}

#endif
