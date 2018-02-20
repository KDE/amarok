/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef TIMECODEMETA_H
#define TIMECODEMETA_H

#include "core/meta/Meta.h"
#include "core/meta/TrackEditor.h"

class QAction;

namespace Meta {

    class TimecodeTrack;
    class TimecodeAlbum;
    class TimecodeArtist;
    class TimecodeGenre;
    class TimecodeComposer;
    class TimecodeYear;

    typedef AmarokSharedPointer<TimecodeTrack> TimecodeTrackPtr;
    typedef AmarokSharedPointer<TimecodeArtist> TimecodeArtistPtr;
    typedef AmarokSharedPointer<TimecodeAlbum> TimecodeAlbumPtr;
    typedef AmarokSharedPointer<TimecodeGenre> TimecodeGenrePtr;
    typedef AmarokSharedPointer<TimecodeComposer> TimecodeComposerPtr;
    typedef AmarokSharedPointer<TimecodeYear> TimecodeYearPtr;


class TimecodeTrack : public Track, public TrackEditor
{
public:
    TimecodeTrack( const QString &name, const QUrl &url, qint64 start, qint64 end );
    virtual ~TimecodeTrack();

    virtual QString name() const;

    virtual QUrl playableUrl() const;
    virtual QString uidUrl() const;
    virtual QString prettyUrl() const;
    virtual QString notPlayableReason() const;

    virtual AlbumPtr album() const;
    virtual ArtistPtr artist() const;
    virtual GenrePtr genre() const;
    virtual ComposerPtr composer() const;
    virtual YearPtr year() const;

    virtual qreal bpm() const;
    virtual QString comment() const;
    virtual qint64 length() const;
    virtual int filesize() const;
    virtual int sampleRate() const;
    virtual int bitrate() const;
    virtual int trackNumber() const;
    virtual int discNumber() const;
    virtual QString type() const;

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
    virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type );

    virtual TrackEditorPtr editor();

    // TrackEditor methods
    virtual void setAlbum( const QString &newAlbum );
    virtual void setAlbumArtist( const QString &newAlbumArtist );
    virtual void setArtist( const QString &newArtist );
    virtual void setComposer( const QString &newComposer );
    virtual void setGenre( const QString &newGenre );
    virtual void setYear( int newYear );
    virtual void setTitle( const QString &newTitle );
    virtual void setComment( const QString &newComment );
    virtual void setTrackNumber( int newTrackNumber );
    virtual void setDiscNumber( int newDiscNumber );
    virtual void setBpm( const qreal newBpm );

    virtual void beginUpdate();
    virtual void endUpdate();

    //TimecodeTrack specific methods
    void setAlbum( TimecodeAlbumPtr album );
    void setArtist( TimecodeArtistPtr artist );
    void setComposer( TimecodeComposerPtr composer );
    void setGenre( TimecodeGenrePtr genre );
    void setYear( TimecodeYearPtr year );

    qint64 start();
    qint64 end();


private:
    //TimecodeCollection *m_collection;

    TimecodeArtistPtr m_artist;
    TimecodeAlbumPtr m_album;
    TimecodeGenrePtr m_genre;
    TimecodeComposerPtr m_composer;
    TimecodeYearPtr m_year;

    QString m_name;
    QString m_type;
    qint64 m_start;
    qint64 m_end;
    qint64 m_length;
    qreal m_bpm;
    int m_trackNumber;
    int m_discNumber;
    QString m_comment;
    QString m_displayUrl;
    QUrl m_playableUrl;

    int m_updatedFields;
    QMap<int, QString> m_fields;

    enum
    {
        ALBUM_UPDATED       = 1 << 0,
        ARTIST_UPDATED      = 1 << 1,
        COMPOSER_UPDATED    = 1 << 2,
        GENRE_UPDATED       = 1 << 3,
        YEAR_UPDATED        = 1 << 4,
        TITLE_UPDATED       = 1 << 5,
        COMMENT_UPDATED     = 1 << 6,
        TRACKNUMBER_UPDATED = 1 << 7,
        DISCNUMBER_UPDATED  = 1 << 8,
        BPM_UPDATED         = 1 << 9
    };
};

class TimecodeArtist : public Meta::Artist
{
public:
    TimecodeArtist( const QString &name );
    virtual ~TimecodeArtist();

    virtual QString name() const;

    virtual TrackList tracks();

    virtual AlbumList albums();

    bool operator==( const Meta::Artist &other ) const
    {
        return name() == other.name();
    }

    //TimecodeArtist specific methods
    void addTrack( TimecodeTrackPtr track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeAlbum : public QObject, public Meta::Album
{
Q_OBJECT
public:
    TimecodeAlbum( const QString &name );
    virtual ~TimecodeAlbum();

    virtual QString name() const;

    virtual bool isCompilation() const;
    virtual bool hasAlbumArtist() const;
    virtual ArtistPtr albumArtist() const;
    virtual TrackList tracks();

    virtual QImage image( int size = 0 ) const;
    virtual bool canUpdateImage() const;
    virtual void setImage( const QImage &image );

    virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
    virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

    //TimecodeAlbum specific methods
    void addTrack( TimecodeTrackPtr track );
    void setAlbumArtist( TimecodeArtistPtr artist );

    bool operator==( const Meta::Album &other ) const
    {
        return name() == other.name();
    }

private:
    QString m_name;
    TrackList m_tracks;
    bool m_isCompilation;
    TimecodeArtistPtr m_albumArtist;

    QImage m_cover;
};

class TimecodeGenre : public Meta::Genre
{
public:
    TimecodeGenre( const QString &name );
    virtual ~TimecodeGenre();

    virtual QString name() const;

    virtual TrackList tracks();

    bool operator==( const Meta::Genre &other ) const
    {
        return name() == other.name();
    }

    //TimecodeGenre specific methods
    void addTrack( TimecodeTrackPtr track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeComposer : public Meta::Composer
{
public:
    TimecodeComposer( const QString &name );
    virtual ~TimecodeComposer();

    virtual QString name() const;

    virtual TrackList tracks();

    bool operator==( const Meta::Composer &other ) const
    {
        return name() == other.name();
    }

    //TimecodeComposer specific methods
    void addTrack( TimecodeTrackPtr track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeYear : public Meta::Year
{
public:
    TimecodeYear( const QString &name );
    virtual ~TimecodeYear();

    virtual QString name() const;

    virtual TrackList tracks();

    bool operator==( const Meta::Year &other ) const
    {
        return name() == other.name();
    }

    //TimecodeYear specific methods
    void addTrack( TimecodeTrackPtr track );

private:
    QString m_name;
    TrackList m_tracks;
};

}
#endif
