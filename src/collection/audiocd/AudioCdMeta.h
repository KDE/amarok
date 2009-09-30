/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AUDIOCDMETA_H
#define AUDIOCDMETA_H

#include "Meta.h"

class AudioCdCollection;

namespace Meta
{

class AudioCdTrack;
class AudioCdAlbum;
class AudioCdArtist;
class AudioCdGenre;
class AudioCdComposer;
class AudioCdYear;

typedef KSharedPtr<AudioCdTrack> AudioCdTrackPtr;
typedef KSharedPtr<AudioCdArtist> AudioCdArtistPtr;
typedef KSharedPtr<AudioCdAlbum> AudioCdAlbumPtr;
typedef KSharedPtr<AudioCdGenre> AudioCdGenrePtr;
typedef KSharedPtr<AudioCdComposer> AudioCdComposerPtr;
typedef KSharedPtr<AudioCdYear> AudioCdYearPtr;

class AudioCdTrack : public Meta::Track
{
    public:
        AudioCdTrack( AudioCdCollection *collection, const QString &name, const QString &url );
        virtual ~AudioCdTrack();

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

        virtual qint64 length() const;

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

        virtual void beginMetaDataUpdate() {}    //read only
        virtual void endMetaDataUpdate() {}      //read only
        virtual void abortMetaDataUpdate() {}    //read only

        virtual void subscribe ( Observer *observer );
        virtual void unsubscribe ( Observer *observer );

        virtual bool inCollection() const;
        virtual Amarok::Collection* collection() const;

        //AudioCdTrack specific methods
        void setAlbum( AudioCdAlbumPtr album );
        void setArtist( AudioCdArtistPtr artist );
        void setComposer( AudioCdComposerPtr composer );
        void setGenre( AudioCdGenrePtr genre );
        void setYear( AudioCdYearPtr year );

        void setLength( qint64 length );

        void setFileNameBase( const QString &fileNameBase );
        QString fileNameBase();

    private:
        AudioCdCollection *m_collection;

        AudioCdArtistPtr m_artist;
        AudioCdAlbumPtr m_album;
        AudioCdGenrePtr m_genre;
        AudioCdComposerPtr m_composer;
        AudioCdYearPtr m_year;

        QString m_name;
        qint64 m_length;
        int m_trackNumber;
        QString m_displayUrl;
        QString m_playableUrl;
        QString m_fileNameBase;
};

class AudioCdArtist : public Meta::Artist
{
    public:
        AudioCdArtist( const QString &name );
        virtual ~AudioCdArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

        //AudioCdArtist specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class AudioCdAlbum : public Meta::Album
{
    public:
        AudioCdAlbum( const QString &name );
        virtual ~AudioCdAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QPixmap image( int size = 1 );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QPixmap &pixmap );

        //AudioCdAlbum specific methods
        void addTrack( AudioCdTrackPtr track );
        void setAlbumArtist( AudioCdArtistPtr artist );
        void setIsCompilation( bool compilation );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        AudioCdArtistPtr m_albumArtist;
        QPixmap m_cover;
        QMap<int, QPixmap> m_coverSizeMap;
};

class AudioCdGenre : public Meta::Genre
{
    public:
        AudioCdGenre( const QString &name );
        virtual ~AudioCdGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //AudioCdGenre specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class AudioCdComposer : public Meta::Composer
{
    public:
        AudioCdComposer( const QString &name );
        virtual ~AudioCdComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //AudioCdComposer specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class AudioCdYear : public Meta::Year
{
    public:
        AudioCdYear( const QString &name );
        virtual ~AudioCdYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //AudioCdYear specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

