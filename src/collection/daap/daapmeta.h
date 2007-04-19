/* This file is part of the KDE project
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

#ifndef DAAPMETA_H
#define DAAPMETA_H

#include "meta.h"

using namespace Meta;

class DaapTrack : public Meta::Track
{
    public:
        DaapTrack( const QString &host, quint16 port, const QString &dbId, const QString &itemId, const QString &format);
        virtual ~DaapTrack();

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

        virtual void subscribe ( TrackObserver *observer );
        virtual void unsubscribe ( TrackObserver *observer );

        //DaapTrack specific methods
        void setAlbum( AlbumPtr album );
        void setArtist( ArtistPtr artist ); 
        void setComposer( ComposerPtr composer );
        void setGenre( GenrePtr genre );
        void setYear( YearPtr year );

        void setTitle( const QString &newTitle );
        void setLength( int length );

    private:
        ArtistPtr m_artist;
        AlbumPtr m_album;
        GenrePtr m_genre;
        ComposerPtr m_composer;
        YearPtr m_year;

        QString m_name;
        QString m_type;
        int m_length;
        int m_trackNumber;
        QString m_displayUrl;
        QString m_playableUrl;
};

class DaapArtist : public Meta::Artist
{
    public:
        DaapArtist( const QString &name );
        virtual ~DaapArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //DaapArtist specific methods
        void addTrack( TrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapAlbum : public Meta::Album
{
    public:
        DaapAlbum( const QString &name );
        virtual ~DaapAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual void image() const;
        virtual bool canUpdateImage() const;
        virtual void updateImage();

        //DaapAlbum specific methods
        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );
        void setIsCompilation( bool compilation );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        ArtistPtr m_albumArtist;
};

class DaapGenre : public Meta::Genre
{
    public:
        DaapGenre( const QString &name );
        virtual ~DaapGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //DaapGenre specific methods
        void addTrack( TrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapComposer : public Meta::Composer
{
    public:
        DaapComposer( const QString &name );
        virtual ~DaapComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //DaapComposer specific methods
        void addTrack( TrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapYear : public Meta::Year
{
    public:
        DaapYear( const QString &name );
        virtual ~DaapYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //DaapYear specific methods
        void addTrack( TrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

#endif

