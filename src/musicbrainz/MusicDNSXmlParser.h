/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICDNSXMLPARSER_H
#define MUSICDNSXMLPARSER_H

#include <QDomDocument>
#include <QObject>
#include <QStringList>

#include <ThreadWeaver/Job>

class MusicDNSXmlParser : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        explicit MusicDNSXmlParser(QString &doc );
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override;
        QStringList puid();

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        void parseTrack( const QDomElement &e );

        void parsePUIDList( const QDomElement &e );
        void parsePUID( const QDomElement &e );

        QDomDocument m_doc;
        QStringList m_puid;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

};

#endif // MUSICDNSXMLPARSER_H
