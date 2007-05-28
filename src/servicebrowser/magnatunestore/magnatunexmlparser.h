/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#ifndef MAGNATUNEXMLPARSER_H
#define MAGNATUNEXMLPARSER_H

#include "magnatunedatabasehandler.h"
#include "magnatunetypes.h"
#include "threadmanager.h"

#include <qdom.h>
#include <QString>
#include <QDomElement>


/**
* Parser for the XML file from http://magnatune.com/info/album_info.xml
*
* @author Nikolaj Hald Nielsen
*/
class MagnatuneXmlParser : public ThreadManager::Job
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param fileName The file to parse 
     * @return Pointer to new object
     */
    MagnatuneXmlParser( const QString &fileName );

    /**
     * The function that starts the actual work. Inherited fromThreadManager::Job 
     * Note the work is performed in a separate thread
     * @return Returns true on success and false on failure
     */
    bool doJob();

    /**
     * Called when the job has completed. Is executed in the GUI thread
     */
    void completeJob();

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

signals:

    /**
     * Signal emmited when parsing is complete.
     */
    void doneParsing();

private:

    QString m_currentArtist;
    QString m_currentArtistGenre;

    /**
     * Parses a DOM element
     * @param e The element to parse
     */
    void parseElement( QDomElement e );

    /**
     * Parses all children of a DOM element
     * @param e The element whose children is to be parsed
     */
    void parseChildren( QDomElement e );

    /**
     * Parse a DOM element representing an album
     * @param e The album element to parse
     */
    void parseAlbum( QDomElement e );

    /**
     * Parse a DOM element representing a track
     * @param e The track element to parse
     */
    void parseTrack( QDomElement e );

    /**
     * Parse the moods of a track
     * @param e The moods element to parse
     */
    void parseMoods( QDomElement e );

    MagnatuneAlbum *m_pCurrentAlbum;
    MagnatuneArtist *m_pCurrentArtist;
    MagnatuneTrackList m_currentAlbumTracksList;
    QStringList m_currentTrackMoodList;

    QString m_sFileName;

    int m_nNumberOfTracks;
    int m_nNumberOfAlbums;
    int m_nNumberOfArtists;

    MagnatuneDatabaseHandler * m_dbHandler;


};

#endif
