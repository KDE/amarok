/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef JAMENDOXMLPARSER_H
#define JAMENDOXMLPARSER_H

#include "JamendoDatabaseHandler.h"

#include <ThreadWeaver/Job>

#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

/**
* Parser for the XML file from http://imgjam.com/data/dbdump_artistalbumtrack.xml.gz
*
* @author Nikolaj Hald Nielsen
*/
class JamendoXmlParser : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param fileName The file to parse 
     */
    explicit JamendoXmlParser( const QString &fileName );

    /**
     * The function that starts the actual work. Inherited from ThreadWeaver::Job 
     * Note the work is performed in a separate thread
     */
    void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    /**
     * Destructor
     */
    ~JamendoXmlParser();

    /**
     * Reads, and starts parsing, file. Should not be used directly.
     * @param filename The file to read
     */
    void readConfigFile( const QString &filename );

    virtual void requestAbort ();

Q_SIGNALS:

    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

    /**
     * Signal emitted when parsing is complete.
     */
    void doneParsing();

private Q_SLOTS:
    /**
     * Called when the job has completed. Is executed in the GUI thread
     */
    void completeJob();

private:

    JamendoDatabaseHandler * m_dbHandler;
    QXmlStreamReader m_reader;
    QString m_sFileName;

    QMap<int, QStringList> albumTags; //used for applying genres to individual tracks

    int m_nNumberOfTracks;
    int m_nNumberOfAlbums;
    int m_nNumberOfArtists;

    /**
     * Read a DOM element representing an artist
     */
    void readArtist();
    /**
     * Read a DOM element representing an album
     */
    void readAlbum();
    /**
     * Read a DOM element representing a track
     */
    void readTrack();

    void countTransaction();
    int m_currentArtistId;
    int m_currentAlbumId;

    int n_numberOfTransactions;
    int n_maxNumberOfTransactions;
    QHash< int, QString > m_id3GenreHash;
    QMap<int, int> m_albumArtistMap;

    bool m_aborted;

protected:
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

#endif
