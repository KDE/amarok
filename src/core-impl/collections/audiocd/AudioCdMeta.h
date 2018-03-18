/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AUDIOCDMETA_H
#define AUDIOCDMETA_H

#include "core/meta/Meta.h"

namespace Collections {
    class AudioCdCollection;
}

namespace Meta
{

class AudioCdTrack;
class AudioCdAlbum;
class AudioCdArtist;
class AudioCdGenre;
class AudioCdComposer;
class AudioCdYear;

typedef AmarokSharedPointer<AudioCdTrack> AudioCdTrackPtr;
typedef AmarokSharedPointer<AudioCdArtist> AudioCdArtistPtr;
typedef AmarokSharedPointer<AudioCdAlbum> AudioCdAlbumPtr;
typedef AmarokSharedPointer<AudioCdGenre> AudioCdGenrePtr;
typedef AmarokSharedPointer<AudioCdComposer> AudioCdComposerPtr;
typedef AmarokSharedPointer<AudioCdYear> AudioCdYearPtr;

class AudioCdTrack : public Meta::Track
{
    public:
        AudioCdTrack( Collections::AudioCdCollection *collection, const QString &name, const QUrl &url );
        virtual ~AudioCdTrack();

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

        virtual void setTitle( const QString &newTitle );

        virtual qreal bpm() const;

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual qint64 length() const;

        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual QString type() const;

        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

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
        Collections::AudioCdCollection *m_collection;

        AudioCdArtistPtr m_artist;
        AudioCdAlbumPtr m_album;
        AudioCdGenrePtr m_genre;
        AudioCdComposerPtr m_composer;
        AudioCdYearPtr m_year;

        QString m_name;
        qint64 m_length;
        int m_trackNumber;
        QUrl m_playableUrl;
        QString m_fileNameBase;
};

class AudioCdArtist : public Meta::Artist
{
    public:
        explicit AudioCdArtist( const QString &name );
        virtual ~AudioCdArtist();

        virtual QString name() const;

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
        explicit AudioCdAlbum( const QString &name );
        virtual ~AudioCdAlbum();

        virtual QString name() const;

        virtual bool isCompilation() const;
        virtual bool canUpdateCompilation() const;
        virtual void setCompilation( bool compilation );

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QImage image( int size = 0 ) const;
        virtual bool hasImage( int size = 0 ) const;
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image );

        //AudioCdAlbum specific methods
        void addTrack( AudioCdTrackPtr track );
        void setAlbumArtist( AudioCdArtistPtr artist );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        AudioCdArtistPtr m_albumArtist;
        QImage m_cover;
};

class AudioCdGenre : public Meta::Genre
{
    public:
        explicit AudioCdGenre( const QString &name );
        virtual ~AudioCdGenre();

        virtual QString name() const;

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
        explicit AudioCdComposer( const QString &name );
        virtual ~AudioCdComposer();

        virtual QString name() const;

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
        explicit AudioCdYear( const QString &name );
        virtual ~AudioCdYear();

        virtual QString name() const;

        virtual TrackList tracks();

        //AudioCdYear specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

