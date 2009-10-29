/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include <threadweaver/Job.h>

#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

/**
* Parser for the XML file from http://img.jamendo.com/data/dbdump.en.xml.gz
*
* @author Nikolaj Hald Nielsen
*/
class JamendoXmlParser : public ThreadWeaver::Job
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param fileName The file to parse 
     * @return Pointer to new object
     */
    JamendoXmlParser( const QString &fileName );

    /**
     * The function that starts the actual work. Inherited from ThreadWeaver::Job 
     * Note the work is performed in a separate thread
     * @return Returns true on success and false on failure
     */
    void run();

    /**
     * Destructor
     * @return none
     */
    ~JamendoXmlParser();

    /**
     * Reads, and starts parsing, file. Should not be used directly.
     * @param filename The file to read
     */
    void readConfigFile( const QString &filename );

    virtual void requestAbort ();


signals:

    /**
     * Signal emitted when parsing is complete.
     */
    void doneParsing();

private slots:
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
};

#endif
