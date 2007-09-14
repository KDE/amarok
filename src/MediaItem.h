/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <ruiz@kde.org>  
    (c) 2006 T.R.Shashwath <trshash84@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#ifndef AMAROK_MEDIADEVICEMETA_H
#define AMAROK_MEDIADEVICEMETA_H

#include "meta/meta.h"
#include "amarok_export.h"

#include <QList>

class MediaDevice;

namespace Meta
{

class MediaDeviceTrack;
class MediaDeviceAlbum;
class MediaDeviceArtist;
class MediaDeviceGenre;

typedef KSharedPtr<MediaDeviceTrack> MediaDeviceTrackPtr;
typedef KSharedPtr<MediaDeviceArtist> MediaDeviceArtistPtr;
typedef KSharedPtr<MediaDeviceAlbum> MediaDeviceAlbumPtr;
typedef KSharedPtr<MediaDeviceGenre> MediaDeviceGenrePtr;


typedef QList<MediaDeviceTrackPtr > MediaDeviceTrackList;
typedef QList<MediaDeviceArtistPtr > MediaDeviceArtistList;
typedef QList<MediaDeviceAlbumPtr > MediaDeviceAlbumList;
typedef QList<MediaDeviceGenrePtr > MediaDeviceGenreList;

class MediaDeviceTrack : public Meta::Track
{
    public:
        //Give this a displayable name as some services has terrible names for their streams
        MediaDeviceTrack( const QString & name );

        //create track based on an sql query result
        MediaDeviceTrack( const QStringList & resultRow );
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

        virtual QString filename() const;
        virtual void setFilename ( const QString &filename );

        virtual QString type() const;

        virtual void beginMetaDataUpdate() {}    //read only
        virtual void endMetaDataUpdate() {}      //read only
        virtual void abortMetaDataUpdate() {}    //read only

        //MediaDeviceTrack specific methods

        void setAlbum( Meta::AlbumPtr album );
        void setArtist( Meta::ArtistPtr artist );
        void setComposer( Meta::ComposerPtr composer );
        void setGenre( Meta::GenrePtr genre );
        void setYear( Meta::YearPtr year );

        void setLength( int length );

    private:
        ArtistPtr m_artist;
        AlbumPtr m_album;
        GenrePtr m_genre;
        ComposerPtr m_composer;
        YearPtr m_year;

        QString m_name;
        int m_trackNumber;
        int m_length;
        QString m_displayUrl;
        QString m_playableUrl;
        QString m_albumName;
        QString m_artistName;
        QString m_filename;

        QString m_type;
};

class MediaDeviceArtist : public Meta::Artist
{
    public:

        MediaDeviceArtist( const QStringList & resultRow );
        MediaDeviceArtist( const QString & name );
        virtual ~MediaDeviceArtist();

        virtual QString name() const;
        virtual QString prettyName() const;
        virtual void setTitle( const QString &title );
        virtual TrackList tracks();

        //MediaDeviceArtist specific methods

        void addTrack( TrackPtr track );

    private:
        QString m_name;
        QString m_description;
        TrackList m_tracks;

};

class MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( const QStringList & resultRow );
        MediaDeviceAlbum( const QString & name  );
        virtual ~MediaDeviceAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        //MediaDeviceAlbum specific methods

        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );
        void setIsCompilation( bool compilation );

        void setDescription( const QString &description );
        QString description( ) const;
        void setId( int id );
        int id( ) const;
        void setArtistId( int artistId );
        int artistId( ) const;
        void setArtistName( const QString &name );
        QString artistName() const;
        void setTitle( const QString &title );

    private:
        int m_id;
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        ArtistPtr m_albumArtist;
        QString m_description;
        int m_artistId;
        QString m_artistName;
};

class MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( const QString &name );
        MediaDeviceGenre( const QStringList &row );
        virtual ~MediaDeviceGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceGenre specific methods
        void addTrack( TrackPtr track );
        void setName( const QString &name );
        int albumId();
        void setAlbumId( int albumId );

    private:
        int m_albumId;
        QString m_name;
        TrackList m_tracks;
};


}


#endif /*AMAROK_MEDIADEVICEMETA_H*/
