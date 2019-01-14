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
        explicit UpnpTrack( Collections::UpnpCollectionBase *collection );
        virtual ~UpnpTrack();

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

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( int year );

        virtual void setTitle( const QString &newTitle );

        virtual void setUidUrl( const QString &url );

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

        //UpnpTrack specific methods
        void setAlbum( const UpnpAlbumPtr &album );
        void setArtist( const UpnpArtistPtr &artist );
        void setComposer( const UpnpComposerPtr &composer );
        void setGenre( const UpnpGenrePtr &genre );
        void setYear( const UpnpYearPtr &year );
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
        explicit UpnpArtist( const QString &name );
        ~UpnpArtist() override;

        QString name() const override;

        TrackList tracks() override;

        //UpnpArtist specific methods
        void addTrack( const UpnpTrackPtr &track );
        void removeTrack( const UpnpTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpAlbum : public QObject, public Meta::Album
{
  Q_OBJECT
    public:
        explicit UpnpAlbum( const QString &name );
        virtual ~UpnpAlbum();

        QString name() const override;

        bool isCompilation() const override;
        bool hasAlbumArtist() const override;
        ArtistPtr albumArtist() const override;
        TrackList tracks() override;

        bool hasImage( int size = 0 ) const override;
        QImage image( int size = 0 ) const override;
        QUrl imageLocation( int size = 0 ) override;

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        //UpnpAlbum specific methods
        void addTrack( const UpnpTrackPtr &track );
        void removeTrack( const UpnpTrackPtr &track );
        void setAlbumArtist( const UpnpArtistPtr &artist );
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
        explicit UpnpGenre( const QString &name );
        virtual ~UpnpGenre();

        QString name() const override;

        TrackList tracks() override;

        //UpnpGenre specific methods
        void addTrack( const UpnpTrackPtr &track );
        void removeTrack( const UpnpTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpComposer : public Meta::Composer
{
    public:
        explicit UpnpComposer( const QString &name );
        virtual ~UpnpComposer();

        QString name() const override;

        TrackList tracks() override;

        //UpnpComposer specific methods
        void addTrack( const UpnpTrackPtr &track );
        void removeTrack( const UpnpTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class UpnpYear : public Meta::Year
{
    public:
        explicit UpnpYear( int year );
        virtual ~UpnpYear();

        QString name() const override;

        TrackList tracks() override;

        //UpnpYear specific methods
        void addTrack( const UpnpTrackPtr &track );
        void removeTrack( const UpnpTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

