/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef OPMLDIRECTORYXMLPARSER_H
#define OPMLDIRECTORYXMLPARSER_H

#include "OpmlDirectoryDatabaseHandler.h"

#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>

#include <threadweaver/Job.h>

/**
* Parser for the XML file from http://img.jamendo.com/data/dbdump.en.xml.gz
*
* @author Nikolaj Hald Nielsen
*/
class OpmlDirectoryXmlParser : public ThreadWeaver::Job
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param fileName The file to parse 
     * @return Pointer to new object
     */
    OpmlDirectoryXmlParser( const QString &fileName );

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
    ~OpmlDirectoryXmlParser();

    /**
     * Reads, and starts parsing, file. Should not be used directly.
     * @param filename The file to read
     */
    void readConfigFile( const QString &filename );


signals:

    /**
     * Signal emmited when parsing is complete.
     */
    void doneParsing();

    private slots:
        /**
         * Called when the job has completed. Is executed in the GUI thread
         */
        void completeJob();

private:

    int m_currentCategoryId;
    
    OpmlDirectoryDatabaseHandler * m_dbHandler;

    QString m_sFileName;

    QMap<int, QStringList> albumTags; //used for applying genres to individual tracks

    int m_nNumberOfFeeds;
    int m_nNumberOfCategories;

    /**
     * Parses a DOM element
     * @param e The element to parse
     */
    void parseElement( const  QDomElement &e );

    /**
     * Parses all children of a DOM element
     * @param e The element whose children is to be parsed
     */
    void parseChildren( const  QDomElement &e );

    /**
     * Parse a DOM element representing an album
     * @param e The album element to parse
     */
    void parseCategory( const  QDomElement &e );

    /**
     * Parse a DOM element representing a track
     * @param e The track element to parse
     */
    void parseFeed( const  QDomElement &e );

    void countTransaction();

    int n_numberOfTransactions;
    int n_maxNumberOfTransactions;
    QMap<int, int> m_albumArtistMap;
};

#endif
