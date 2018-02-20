/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNPMETA_H
#define UPNPMETA_H

#include "core/meta/Meta.h"

namespace Collections {
    class UpnpCollectionBase;
}

namespace Meta
{

class UpnpTrack;
class UpnpAlbum;
class UpnpArtist;
class UpnpGenre;
class UpnpComposer;
class UpnpYear;

typedef AmarokSharedPointer<UpnpTrack> UpnpTrackPtr;
typedef AmarokSharedPointer<UpnpArtist> UpnpArtistPtr;
typedef AmarokSharedPointer<UpnpAlbum> UpnpAlbumPtr;
typedef AmarokSharedPointer<UpnpGenre> UpnpGenrePtr;
typedef AmarokSharedPointer<UpnpComposer> UpnpComposerPtr;
typedef AmarokSharedPointer<UpnpYear> UpnpYearPtr;

class UpnpTrack : public Meta::Track
{
    public:
        UpnpTrack( Collections::UpnpCollectionBase *collection );
        virtual ~UpnpTrack();

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

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( int year );

        virtual void setTitle( const QString &newTitle );

        virtual void setUidUrl( const QString &url );

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

        //UpnpTrack specific methods
        void setAlbum( UpnpAlbumPtr album );
        void setArtist( UpnpArtistPtr artist );
        void setComposer( UpnpComposerPtr composer );
        void setGenre( UpnpGenrePtr genre );
        void setYear( UpnpYearPtr year );
        void setPlayableUrl( const QString &url );

        void setLength( qint64 length );
        void setBitrate( int rate );

    private:
        Collections::UpnpCollectionBase *m_collection;

        UpnpArtistPtr m_artist;
        UpnpAlbumPtr m_album;
        UpnpGenrePtr m_genre;
        UpnpComposerPtr m_composer;
        UpnpYearPtr m_year;

        QString m_name;
        QString m_type;
        qint64 m_length;
        int m_bitrate;
        int m_trackNumber;
        QString m_displayUrl;
        QString m_playableUrl;
        QString m_uidUrl;
};

class UpnpArtist : public Meta::Artist
{
    public:
        UpnpArtist( const QString &name );
        virtual ~UpnpArtist();

        virtual QString name() const;

        virtual TrackList tracks();

        //UpnpArtist specific methods
        void addTrack( UpnpTrackPtr track );
        void removeTrack( UpnpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpAlbum : public QObject, public Meta::Album
{
  Q_OBJECT
    public:
        UpnpAlbum( const QString &name );
        virtual ~UpnpAlbum();

        virtual QString name() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual bool hasImage( int size = 0 ) const;
        virtual QImage image( int size = 0 ) const;
        virtual QUrl imageLocation( int size = 0 );

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //UpnpAlbum specific methods
        void addTrack( UpnpTrackPtr track );
        void removeTrack( UpnpTrackPtr track );
        void setAlbumArtist( UpnpArtistPtr artist );
        void setAlbumArtUrl( const QUrl &url );

    private:
        QString m_name;
        mutable QImage m_image;
        TrackList m_tracks;
        bool m_isCompilation;
        UpnpArtistPtr m_albumArtist;
        QUrl m_albumArtUrl;
};

class UpnpGenre : public Meta::Genre
{
    public:
        UpnpGenre( const QString &name );
        virtual ~UpnpGenre();

        virtual QString name() const;

        virtual TrackList tracks();

        //UpnpGenre specific methods
        void addTrack( UpnpTrackPtr track );
        void removeTrack( UpnpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpComposer : public Meta::Composer
{
    public:
        UpnpComposer( const QString &name );
        virtual ~UpnpComposer();

        virtual QString name() const;

        virtual TrackList tracks();

        //UpnpComposer specific methods
        void addTrack( UpnpTrackPtr track );
        void removeTrack( UpnpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpYear : public Meta::Year
{
    public:
        UpnpYear( int year );
        virtual ~UpnpYear();

        virtual QString name() const;

        virtual TrackList tracks();

        //UpnpYear specific methods
        void addTrack( UpnpTrackPtr track );
        void removeTrack( UpnpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

