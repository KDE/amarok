/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef MAGNATUNEXMLPARSER_H
#define MAGNATUNEXMLPARSER_H

#include "MagnatuneDatabaseHandler.h"
#include "MagnatuneMeta.h"

#include <qdom.h>
#include <QScopedPointer>
#include <QString>
#include <QDomElement>
#include <QMap>

#include <ThreadWeaver/Job>

/**
* Parser for the XML file from http://magnatune.com/info/album_info.xml
*
* @author Nikolaj Hald Nielsen
*/
class MagnatuneXmlParser : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param fileName The file to parse
     * @return Pointer to new object
     */
    explicit MagnatuneXmlParser( const QString &fileName );

    /**
     * The function that starts the actual work. Inherited from ThreadWeaver::Job
     * Note the work is performed in a separate thread
     */
    void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    /**
     * Destructor
     * @return none
     */
    ~MagnatuneXmlParser();

    /**
     * Reads, and starts parsing, file. Should not be used directly.
     * @param filename The file to read
     */
    void readConfigFile( const QString &filename );


    void setDbHandler( MagnatuneDatabaseHandler * dbHandler );

Q_SIGNALS:
    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

    /**
     * Signal emmited when parsing is complete.
     */
    void doneParsing();

    private Q_SLOTS:
        
    /**
     * Called when the job has completed. Is executed in the GUI thread
     */
    void completeJob();

private:

    QMap<QString, int> artistNameIdMap;

    QString m_currentArtist;
    QString m_currentArtistGenre;

    /**
     * Parses a DOM element
     * @param e The element to parse
     */
    void parseElement( const QDomElement &e );

    /**
     * Parses all children of a DOM element
     * @param e The element whose children is to be parsed
     */
    void parseChildren( const QDomElement &e );

    /**
     * Parse a DOM element representing an album
     * @param e The album element to parse
     */
    void parseAlbum( const QDomElement &e );

    /**
     * Parse a DOM element representing a track
     * @param e The track element to parse
     */
    void parseTrack( const QDomElement &e );

    /**
     * Parse the moods of a track
     * @param e The moods element to parse
     */
    void parseMoods( const QDomElement &e );

    QScopedPointer<Meta::MagnatuneAlbum> m_pCurrentAlbum;
    QScopedPointer<Meta::MagnatuneArtist> m_pCurrentArtist;
    QList<Meta::MagnatuneTrack*> m_currentAlbumTracksList;
    QStringList m_currentTrackMoodList;

    QString m_sFileName;

    int m_nNumberOfTracks;
    int m_nNumberOfAlbums;
    int m_nNumberOfArtists;

    MagnatuneDatabaseHandler * m_dbHandler;

protected:
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

#endif
