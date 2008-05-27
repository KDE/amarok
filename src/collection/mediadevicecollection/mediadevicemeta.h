/* This file is part of the KDE project

   Note: Mostly taken from Daap code:
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef MEDIADEVICEMETA_H
#define MEDIADEVICEMETA_H

#include "Meta.h"

class MediaDeviceCollection;

namespace Meta
{

class MediaDeviceTrack;
class MediaDeviceAlbum;
class MediaDeviceArtist;
class MediaDeviceGenre;
class MediaDeviceComposer;
class MediaDeviceYear;

typedef KSharedPtr<MediaDeviceTrack> MediaDeviceTrackPtr;
typedef KSharedPtr<MediaDeviceArtist> MediaDeviceArtistPtr;
typedef KSharedPtr<MediaDeviceAlbum> MediaDeviceAlbumPtr;
typedef KSharedPtr<MediaDeviceGenre> MediaDeviceGenrePtr;
typedef KSharedPtr<MediaDeviceComposer> MediaDeviceComposerPtr;
typedef KSharedPtr<MediaDeviceYear> MediaDeviceYearPtr;

class MediaDeviceTrack : public Meta::Track
{
    public:
        MediaDeviceTrack( MediaDeviceCollection *collection, const QString &format);
        virtual ~MediaDeviceTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString url() const;
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

        virtual void beginMetaDataUpdate() {}    //read only
        virtual void endMetaDataUpdate() {}      //read only
        virtual void abortMetaDataUpdate() {}    //read only

        virtual void subscribe ( Observer *observer );
        virtual void unsubscribe ( Observer *observer );

        virtual bool inCollection() const;
        virtual Collection* collection() const;

        //MediaDeviceTrack specific methods
        void setAlbum( MediaDeviceAlbumPtr album );
        void setArtist( MediaDeviceArtistPtr artist );
        void setComposer( MediaDeviceComposerPtr composer );
        void setGenre( MediaDeviceGenrePtr genre );
        void setYear( MediaDeviceYearPtr year );

        void setLength( int length );

    private:
        MediaDeviceCollection *m_collection;

        MediaDeviceArtistPtr m_artist;
        MediaDeviceAlbumPtr m_album;
        MediaDeviceGenrePtr m_genre;
        MediaDeviceComposerPtr m_composer;
        MediaDeviceYearPtr m_year;

        QString m_name;
        QString m_type;
        int m_length;
        int m_trackNumber;
        QString m_displayUrl;
        QString m_playableUrl;
};

class MediaDeviceArtist : public Meta::Artist
{
    public:
        MediaDeviceArtist( const QString &name );
        virtual ~MediaDeviceArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

        //MediaDeviceArtist specific methods
        void addTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( const QString &name );
        virtual ~MediaDeviceAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QPixmap image( int size = 1, bool withShadow = false );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image);

        //MediaDeviceAlbum specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void setAlbumArtist( MediaDeviceArtistPtr artist );
        void setIsCompilation( bool compilation );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        MediaDeviceArtistPtr m_albumArtist;
};

class MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( const QString &name );
        virtual ~MediaDeviceGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceGenre specific methods
        void addTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MediaDeviceComposer : public Meta::Composer
{
    public:
        MediaDeviceComposer( const QString &name );
        virtual ~MediaDeviceComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceComposer specific methods
        void addTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MediaDeviceYear : public Meta::Year
{
    public:
        MediaDeviceYear( const QString &name );
        virtual ~MediaDeviceYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceYear specific methods
        void addTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

