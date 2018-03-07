/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef MEDIADEVICEMETA_H
#define MEDIADEVICEMETA_H

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Debug.h"
#include "core-impl/collections/mediadevicecollection/support/mediadevicecollection_export.h"

#include <QList>
#include <QMultiMap>
#include <QPointer>

namespace Collections {
    class MediaDeviceCollection;
}

class QAction;

namespace Handler { class ArtworkCapability; }

namespace Meta
{

class MediaDeviceTrack;
class MediaDeviceAlbum;
class MediaDeviceArtist;
class MediaDeviceGenre;
class MediaDeviceComposer;
class MediaDeviceYear;

typedef AmarokSharedPointer<MediaDeviceTrack> MediaDeviceTrackPtr;
typedef AmarokSharedPointer<MediaDeviceArtist> MediaDeviceArtistPtr;
typedef AmarokSharedPointer<MediaDeviceAlbum> MediaDeviceAlbumPtr;
typedef AmarokSharedPointer<MediaDeviceGenre> MediaDeviceGenrePtr;
typedef AmarokSharedPointer<MediaDeviceComposer> MediaDeviceComposerPtr;
typedef AmarokSharedPointer<MediaDeviceYear> MediaDeviceYearPtr;

typedef QList<MediaDeviceTrackPtr> MediaDeviceTrackList;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceTrack : public Meta::Track, public Statistics
{
    public:
        MediaDeviceTrack( Collections::MediaDeviceCollection *collection );
        virtual ~MediaDeviceTrack();

        virtual QString name() const;

        virtual QUrl playableUrl() const;
        virtual QString uidUrl() const;
        virtual QString prettyUrl() const;
        virtual QString notPlayableReason() const;

        bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setAlbumArtist( const QString &newAlbumArtist );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( int newYear );

        virtual QString title() const;
        virtual void setTitle( const QString &newTitle );

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual qint64 length() const;

        void setFileSize( int newFileSize );
        virtual int filesize() const;

        virtual int bitrate() const;
        virtual void setBitrate( int newBitrate );

        virtual int sampleRate() const;
        virtual void setSamplerate( int newSamplerate );

        virtual qreal bpm() const;
        virtual void setBpm( const qreal newBpm );

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual qreal replayGain( ReplayGainTag mode ) const;
        /* Set the track replay gain (other types unsupported) */
        void setReplayGain( qreal newReplayGain );

        virtual QString type() const;
        virtual void prepareToPlay();

        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

        virtual TrackEditorPtr editor();
        virtual StatisticsPtr statistics();

        // Meta::Statistics methods
        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual QDateTime lastPlayed() const;
        void setLastPlayed( const QDateTime &newTime );

        // firstPlayed() not available in any media device

        virtual int playCount() const;
        void setPlayCount( const int newCount );

        //MediaDeviceTrack specific methods

        // These methods are for Handler
        void setAlbum( MediaDeviceAlbumPtr album );
        void setArtist( MediaDeviceArtistPtr artist );
        void setComposer( MediaDeviceComposerPtr composer );
        void setGenre( MediaDeviceGenrePtr genre );
        void setYear( MediaDeviceYearPtr year );

        void setType( const QString & type );

        void setLength( qint64 length );
        void setPlayableUrl( const QUrl &url) { m_playableUrl = url; }

        /**
         * Notifies observers about changes to metadata, one of the observers is media
         * device handler which writes the changes back to the device.
         */
        void commitChanges();

    private:
        QPointer<Collections::MediaDeviceCollection> m_collection;

        MediaDeviceArtistPtr m_artist;
        MediaDeviceAlbumPtr m_album;
        MediaDeviceGenrePtr m_genre;
        MediaDeviceComposerPtr m_composer;
        MediaDeviceYearPtr m_year;

        // For MediaDeviceTrack-specific use

        QImage m_image;

        QString m_comment;
        QString m_name;
        QString m_type;
        int m_bitrate;
        int m_filesize;
        qint64 m_length;
        int m_discNumber;
        int m_samplerate;
        int m_trackNumber;
        int m_playCount;
        QDateTime m_lastPlayed;
        int m_rating;
        qreal m_bpm;
        qreal m_replayGain;
        QString m_displayUrl;
        QUrl m_playableUrl;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceArtist : public Meta::Artist
{
    public:
        MediaDeviceArtist( const QString &name );
        virtual ~MediaDeviceArtist();

        virtual QString name() const;

        virtual TrackList tracks();

        //MediaDeviceArtist specific methods
        virtual void addTrack( MediaDeviceTrackPtr track );
        virtual void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( Collections::MediaDeviceCollection *collection, const QString &name );
        virtual ~MediaDeviceAlbum();

        virtual QString name() const;

        virtual bool isCompilation() const;
        void setIsCompilation( bool compilation );

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual bool hasImage( int size = 0 ) const;
        virtual QImage image( int size = 0 ) const;
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image );
        virtual void setImagePath( const QString &path );

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //MediaDeviceAlbum specific methods

        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );
        void setAlbumArtist( MediaDeviceArtistPtr artist );

    private:
        QPointer<Collections::MediaDeviceCollection> m_collection;
        QPointer<Handler::ArtworkCapability> m_artworkCapability;

        QString         m_name;
        TrackList       m_tracks;
        bool            m_isCompilation;
        mutable bool    m_hasImagePossibility;
        mutable bool    m_hasImageChecked;
        mutable QImage  m_image;
        MediaDeviceArtistPtr   m_albumArtist;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceComposer : public Meta::Composer
{
    public:
        MediaDeviceComposer( const QString &name );
        virtual ~MediaDeviceComposer();

        virtual QString name() const;

        virtual TrackList tracks();

        //MediaDeviceComposer specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( const QString &name );
        virtual ~MediaDeviceGenre();

        virtual QString name() const;

        virtual TrackList tracks();

        //MediaDeviceGenre specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );
    private:
        QString m_name;
        TrackList m_tracks;
};



class MEDIADEVICECOLLECTION_EXPORT MediaDeviceYear : public Meta::Year
{
    public:
        MediaDeviceYear( const QString &name );
        virtual ~MediaDeviceYear();

        virtual QString name() const;

        virtual TrackList tracks();

        //MediaDeviceYear specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

