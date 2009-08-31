/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef TIMECODEMETA_H
#define TIMECODEMETA_H

#include "meta/Meta.h"
#include "meta/capabilities/BoundedPlaybackCapability.h"
#include "meta/capabilities/EditCapability.h"


class QAction;

namespace Meta {

    class TimecodeTrack;
    class TimecodeAlbum;
    class TimecodeArtist;
    class TimecodeGenre;
    class TimecodeComposer;
    class TimecodeYear;

    typedef KSharedPtr<TimecodeTrack> TimecodeTrackPtr;
    typedef KSharedPtr<TimecodeArtist> TimecodeArtistPtr;
    typedef KSharedPtr<TimecodeAlbum> TimecodeAlbumPtr;
    typedef KSharedPtr<TimecodeGenre> TimecodeGenrePtr;
    typedef KSharedPtr<TimecodeComposer> TimecodeComposerPtr;
    typedef KSharedPtr<TimecodeYear> TimecodeYearPtr;

class BoundedPlaybackCapabilityImpl : public BoundedPlaybackCapability
{
Q_OBJECT
public:
    BoundedPlaybackCapabilityImpl( TimecodeTrack * track )
        : m_track( track )
    {}
    
    virtual qint64 startPosition();
    virtual qint64 endPosition();

private:
    TimecodeTrack * m_track;

};

class TimecodeEditCapabilityImpl : public EditCapability
{
Q_OBJECT
public:
    TimecodeEditCapabilityImpl( TimecodeTrack * track );
    ~TimecodeEditCapabilityImpl() {}

    static Type capabilityInterfaceType() { return Meta::Capability::Editable; }

    virtual bool isEditable() const { return true; }
    virtual void setAlbum( const QString &newAlbum );
    virtual void setArtist( const QString &newArtist );
    virtual void setComposer( const QString &newComposer );
    virtual void setGenre( const QString &newGenre );
    virtual void setYear( const QString &newYear );
    virtual void setTitle( const QString &newTitle );
    virtual void setComment( const QString &newComment );
    virtual void setTrackNumber( int newTrackNumber );
    virtual void setDiscNumber( int newDiscNumber );

    virtual void beginMetaDataUpdate();
    virtual void endMetaDataUpdate();
    virtual void abortMetaDataUpdate();

private:
    TimecodeTrack * m_track;
};

class TimecodeTrack : public Track
{
public:
    TimecodeTrack( const QString &name, const QString &url, qint64 start, qint64 end );
    virtual ~TimecodeTrack();

    virtual QString name() const;
    virtual QString prettyName() const;

    virtual KUrl playableUrl() const;
    virtual QString uidUrl() const;
    virtual QString prettyUrl() const;

    virtual bool isPlayable() const;
    virtual bool isEditable() const;

    virtual AlbumPtr album() const;
    virtual ArtistPtr artist() const;
    virtual GenrePtr genre() const;
    virtual ComposerPtr composer() const;
    virtual YearPtr year() const;

    virtual void setAlbum ( const QString &newAlbum );
    virtual void setArtist ( const QString &newArtist );
    virtual void setGenre ( const QString &newGenre );
    virtual void setComposer ( const QString &newComposer );
    virtual void setYear ( const QString &newYear );

    virtual void setTitle( const QString &newTitle );

    virtual QString comment() const;
    virtual void setComment ( const QString &newComment );

    virtual double score() const;
    virtual void setScore ( double newScore );

    virtual int rating() const;
    virtual void setRating ( int newRating );

    virtual int length() const;

    virtual int filesize() const;
    virtual int sampleRate() const;
    virtual int bitrate() const;

    virtual int trackNumber() const;
    virtual void setTrackNumber ( int newTrackNumber );

    virtual int discNumber() const;
    virtual void setDiscNumber ( int newDiscNumber );

    virtual uint lastPlayed() const;
    virtual int playCount() const;

    virtual QString type() const;

    virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
    virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

    void beginMetaDataUpdate();
    void endMetaDataUpdate();
    void abortMetaDataUpdate();

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
    int m_length;
    int m_trackNumber;
    int m_discNumber;
    QString m_comment;
    QString m_displayUrl;
    QString m_playableUrl;

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
        DISCNUMBER_UPDATED  = 1 << 8
    };
};

class TimecodeArtist : public Meta::Artist
{
public:
    TimecodeArtist( const QString &name );
    virtual ~TimecodeArtist();

    virtual QString name() const;
    virtual QString prettyName() const;

    virtual TrackList tracks();

    virtual AlbumList albums();

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
    virtual QString prettyName() const;

    virtual bool isCompilation() const;
    virtual bool hasAlbumArtist() const;
    virtual ArtistPtr albumArtist() const;
    virtual TrackList tracks();

    virtual QPixmap image( int size = 1 );
    virtual bool canUpdateImage() const;
    virtual void setImage( const QPixmap &pixmap );

    virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
    virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

    //TimecodeAlbum specific methods
    void addTrack( TimecodeTrackPtr track );
    void setAlbumArtist( TimecodeArtistPtr artist );
    void setIsCompilation( bool compilation );

    bool operator==( const Meta::Album &other ) const {
        return name() == other.name();
    }

private:
    QString m_name;
    TrackList m_tracks;
    bool m_isCompilation;
    TimecodeArtistPtr m_albumArtist;

    QPixmap m_cover;
    QMap<int, QPixmap> m_coverSizeMap;

    QAction *m_separator;
    QAction *m_displayCoverAction;
    QAction *m_fetchCoverAction;
    QAction *m_setCustomCoverAction;
    QAction *m_unsetCoverAction;
};

class TimecodeGenre : public Meta::Genre
{
public:
    TimecodeGenre( const QString &name );
    virtual ~TimecodeGenre();

    virtual QString name() const;
    virtual QString prettyName() const;

    virtual TrackList tracks();

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
    virtual QString prettyName() const;

    virtual TrackList tracks();

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
    virtual QString prettyName() const;

    virtual TrackList tracks();

    //TimecodeYear specific methods
    void addTrack( TimecodeTrackPtr track );

private:
    QString m_name;
    TrackList m_tracks;
};

}
#endif
