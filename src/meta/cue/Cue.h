/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef AMAROK_META_CUE_H
#define AMAROK_META_CUE_H

#include "../file/File.h"
#include "EngineController.h"
#include "EngineObserver.h"

#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>

/*
Method for locating cue files:

if ( track.url.isLocalFile() )
{
    QString path    = track.url().path();
    QString cueFile = path.left( path.findRev('.') ) + ".cue";

    // other stuff
}*/

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

    QSet<Meta::Observer*> observers;
    KUrl m_url;
};

typedef QMap<long, CueFileItem> CueFileItemMap;

class AMAROK_EXPORT Track : public MetaFile::Track, public EngineObserver
{
public:
    class Private;

    Track ( const KUrl &url, const KUrl &cuefile );
    ~Track();

    virtual CueFileItemMap cueItems() const;

    virtual void engineTrackPositionChanged ( long /*position*/ , bool /*userSeek*/ );

    virtual void subscribe ( Meta::Observer *observer );
    virtual void unsubscribe ( Meta::Observer *observer );

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

    enum Markers
    {
        BEGIN = 0,
        TRACK_FOUND, // track found, index not yet found
        INDEX_FOUND
    };

    //methods inherited from Meta::MetaBase
    virtual QString name() const;
    virtual QString prettyName() const;
    virtual QString fullPrettyName() const;
    virtual QString sortableName() const;

    virtual int trackNumber() const;
    virtual qint64 length() const;

    virtual Meta::AlbumPtr album() const;
    virtual Meta::ArtistPtr artist() const;


    virtual void setAlbum ( const QString &newAlbum );
    virtual void setArtist ( const QString &newArtist );
    virtual void setTitle ( const QString &newTitle );
    virtual void setTrackNumber ( int newTrackNumber );

    virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
    virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

private:
    typedef KSharedPtr<Track> TrackPtr;

    bool load ( qint64 mediaLength );
    void notify() const;

    KUrl m_cuefile;
    int m_lastSeekPos; // in seconds
    CueFileItemMap m_cueitems;
    QSet<Meta::Observer*> m_observers;

    Private * const d;
};
}
#endif
