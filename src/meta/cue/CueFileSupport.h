/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/


#ifndef CUEFILESUPPORT_H
#define CUEFILESUPPORT_H

#include "meta/Meta.h"

#include <KUrl>

#include <QFile>
#include <QSet>
#include <QString>

namespace MetaCue
{

class CueFileItem
{
public:
    CueFileItem ( const QString& title, const QString& artist, const QString& album, const int trackNumber, const long index )
            : m_title ( title )
            , m_artist ( artist )
            , m_album ( album )
            , m_trackNumber ( trackNumber )
            , m_index ( index )
            , m_length ( -1 )
    {}

    CueFileItem()
            : m_title( )
            , m_artist( )
            , m_album( )
            , m_trackNumber ( -1 )
            , m_index ( -1 )
            , m_length ( -1 )
    {}
    void setLength ( const long length )
    {
        m_length = length;
    }
    const QString getTitle () const
    {
        return m_title;
    }
    const QString getArtist () const
    {
        return m_artist;
    }
    const QString getAlbum () const
    {
        return m_album;
    }
    int getTrackNumber () const
    {
        return m_trackNumber;
    }
    long getIndex () const
    {
        return m_index;
    }
    long getLength () const
    {
        return m_length;
    }

private:
    QString m_title;
    QString m_artist;
    QString m_album;
    int     m_trackNumber;
    long    m_index;
    long    m_length;

    //QSet<Meta::Observer*> observers;
    KUrl m_url;
};

typedef QMap<long, CueFileItem> CueFileItemMap;


class CueFileSupport
{

    public:

    enum Markers
    {
        BEGIN = 0,
        TRACK_FOUND, // track found, index not yet found
        INDEX_FOUND
    };
        
     static CueFileItemMap loadCueFile( Meta::TrackPtr track );
        
     /**
     * Used to locate a cue sheet for a local track.
     * @return A KUrl containing the url for the cue sheet
     *         if a valid one was located
     */
    static KUrl locateCueSheet ( const KUrl &trackurl );

    /**
     * Attempts to load and parse a cue sheet.
     * @return true if the cue sheet is valid
     *         false if the cue sheet is invalid
     */
    static bool validateCueSheet ( const QString& cuefile );

    static Meta::TrackList generateTimeCodeTracks( Meta::TrackPtr baseTrack, CueFileItemMap itemMap );
};

}

#endif // CUEFILESUPPORT_H
