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

    QString name() const override;

    QUrl playableUrl() const override;
    QString uidUrl() const override;
    QString prettyUrl() const override;
    QString notPlayableReason() const override;

    AlbumPtr album() const override;
    ArtistPtr artist() const override;
    GenrePtr genre() const override;
    ComposerPtr composer() const override;
    YearPtr year() const override;

    qreal bpm() const override;
    QString comment() const override;
    qint64 length() const override;
    int filesize() const override;
    int sampleRate() const override;
    int bitrate() const override;
    int trackNumber() const override;
    int discNumber() const override;
    QString type() const override;

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
    Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type ) override;

    TrackEditorPtr editor() override;

    // TrackEditor methods
    void setAlbum( const QString &newAlbum ) override;
    void setAlbumArtist( const QString &newAlbumArtist ) override;
    void setArtist( const QString &newArtist ) override;
    void setComposer( const QString &newComposer ) override;
    void setGenre( const QString &newGenre ) override;
    void setYear( int newYear ) override;
    void setTitle( const QString &newTitle ) override;
    void setComment( const QString &newComment ) override;
    void setTrackNumber( int newTrackNumber ) override;
    void setDiscNumber( int newDiscNumber ) override;
    void setBpm( const qreal newBpm ) override;

    void beginUpdate() override;
    void endUpdate() override;

    //TimecodeTrack specific methods
    void setAlbum( const TimecodeAlbumPtr &album );
    void setArtist( const TimecodeArtistPtr &artist );
    void setComposer( const TimecodeComposerPtr &composer );
    void setGenre( const TimecodeGenrePtr &genre );
    void setYear( const TimecodeYearPtr &year );

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
    explicit TimecodeArtist( const QString &name );
    virtual ~TimecodeArtist();

    QString name() const override;

    TrackList tracks() override;

    virtual AlbumList albums();

    bool operator==( const Meta::Artist &other ) const override
    {
        return name() == other.name();
    }

    //TimecodeArtist specific methods
    void addTrack( const TimecodeTrackPtr &track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeAlbum : public QObject, public Meta::Album
{
Q_OBJECT
public:
    explicit TimecodeAlbum( const QString &name );
    virtual ~TimecodeAlbum();

    QString name() const override;

    bool isCompilation() const override;
    bool hasAlbumArtist() const override;
    ArtistPtr albumArtist() const override;
    TrackList tracks() override;

    QImage image( int size = 0 ) const override;
    bool canUpdateImage() const override;
    void setImage( const QImage &image ) override;

    bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
    Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

    //TimecodeAlbum specific methods
    void addTrack( const TimecodeTrackPtr &track );
    void setAlbumArtist( const TimecodeArtistPtr &artist );

    bool operator==( const Meta::Album &other ) const override
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
    explicit TimecodeGenre( const QString &name );
    virtual ~TimecodeGenre();

    QString name() const override;

    TrackList tracks() override;

    bool operator==( const Meta::Genre &other ) const override
    {
        return name() == other.name();
    }

    //TimecodeGenre specific methods
    void addTrack( const TimecodeTrackPtr &track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeComposer : public Meta::Composer
{
public:
    explicit TimecodeComposer( const QString &name );
    virtual ~TimecodeComposer();

    QString name() const override;

    TrackList tracks() override;

    bool operator==( const Meta::Composer &other ) const override
    {
        return name() == other.name();
    }

    //TimecodeComposer specific methods
    void addTrack( const TimecodeTrackPtr &track );

private:
    QString m_name;
    TrackList m_tracks;
};

class TimecodeYear : public Meta::Year
{
public:
    explicit TimecodeYear( const QString &name );
    virtual ~TimecodeYear();

    QString name() const override;

    TrackList tracks() override;

    bool operator==( const Meta::Year &other ) const override
    {
        return name() == other.name();
    }

    //TimecodeYear specific methods
    void addTrack( const TimecodeTrackPtr &track );

private:
    QString m_name;
    TrackList m_tracks;
};

}
#endif
