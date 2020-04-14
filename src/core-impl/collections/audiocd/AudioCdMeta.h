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
        ~AudioCdTrack() override;

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

        virtual void setTitle( const QString &newTitle );

        qreal bpm() const override;

        QString comment() const override;
        virtual void setComment ( const QString &newComment );

        qint64 length() const override;

        int filesize() const override;
        int sampleRate() const override;
        int bitrate() const override;

        int trackNumber() const override;
        virtual void setTrackNumber ( int newTrackNumber );

        int discNumber() const override;
        virtual void setDiscNumber ( int newDiscNumber );

        QString type() const override;

        bool inCollection() const override;
        Collections::Collection* collection() const override;

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
        ~AudioCdArtist() override;

        QString name() const override;

        TrackList tracks() override;

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
        ~AudioCdAlbum() override;

        QString name() const override;

        bool isCompilation() const override;
        bool canUpdateCompilation() const override;
        void setCompilation( bool compilation ) override;

        bool hasAlbumArtist() const override;
        ArtistPtr albumArtist() const override;
        TrackList tracks() override;

        QImage image( int size = 0 ) const override;
        bool hasImage( int size = 0 ) const override;
        bool canUpdateImage() const override;
        void setImage( const QImage &image ) override;

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
        ~AudioCdGenre() override;

        QString name() const override;

        TrackList tracks() override;

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
        ~AudioCdComposer() override;

        QString name() const override;

        TrackList tracks() override;

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
        ~AudioCdYear() override;

        QString name() const override;

        TrackList tracks() override;

        //AudioCdYear specific methods
        void addTrack( AudioCdTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

