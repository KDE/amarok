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

#ifndef IPODMETA_H
#define IPODMETA_H

#include "Meta.h"

class IpodCollection;

namespace Meta
{

class IpodTrack;
class IpodAlbum;
class IpodArtist;
class IpodGenre;
class IpodComposer;
class IpodYear;

typedef KSharedPtr<IpodTrack> IpodTrackPtr;
typedef KSharedPtr<IpodArtist> IpodArtistPtr;
typedef KSharedPtr<IpodAlbum> IpodAlbumPtr;
typedef KSharedPtr<IpodGenre> IpodGenrePtr;
typedef KSharedPtr<IpodComposer> IpodComposerPtr;
typedef KSharedPtr<IpodYear> IpodYearPtr;

class IpodTrack : public Meta::Track
{
    public:
        IpodTrack( IpodCollection *collection, const QString &format);
        virtual ~IpodTrack();

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

        virtual QString title() const;
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
        virtual void setBitrate( int newBitrate );

        virtual int samplerate() const;
        virtual void setSamplerate( int newSamplerate );

        virtual float bpm() const;
        virtual void setBpm( float newBpm );

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

        //IpodTrack specific methods
        void setAlbum( IpodAlbumPtr album );
        void setArtist( IpodArtistPtr artist );
        void setComposer( IpodComposerPtr composer );
        void setGenre( IpodGenrePtr genre );
        void setYear( IpodYearPtr year );

        void setLength( int length );
	void setPlayableUrl( QString Url ) { m_playableUrl = Url; }

    private:
        IpodCollection *m_collection;

        IpodArtistPtr m_artist;
        IpodAlbumPtr m_album;
        IpodGenrePtr m_genre;
        IpodComposerPtr m_composer;
        IpodYearPtr m_year;

        QString m_comment;
        QString m_name;
        QString m_type;
        int m_bitrate;
        int m_length;
        int m_discNumber;
        int m_samplerate;
        int m_trackNumber;
        float m_bpm;
        QString m_displayUrl;
        QString m_playableUrl;
};

class IpodArtist : public Meta::Artist
{
    public:
        IpodArtist( const QString &name );
        virtual ~IpodArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

        //IpodArtist specific methods
        void addTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodAlbum : public Meta::Album
{
    public:
        IpodAlbum( const QString &name );
        virtual ~IpodAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QPixmap image( int size = 1, bool withShadow = false );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image);

        //IpodAlbum specific methods
        void addTrack( IpodTrackPtr track );
        void setAlbumArtist( IpodArtistPtr artist );
        void setIsCompilation( bool compilation );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        IpodArtistPtr m_albumArtist;
};

class IpodGenre : public Meta::Genre
{
    public:
        IpodGenre( const QString &name );
        virtual ~IpodGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodGenre specific methods
        void addTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodComposer : public Meta::Composer
{
    public:
        IpodComposer( const QString &name );
        virtual ~IpodComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodComposer specific methods
        void addTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodYear : public Meta::Year
{
    public:
        IpodYear( const QString &name );
        virtual ~IpodYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodYear specific methods
        void addTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

