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
        explicit MediaDeviceTrack( Collections::MediaDeviceCollection *collection );
        virtual ~MediaDeviceTrack();

        QString name() const override;

        QUrl playableUrl() const override;
        QString uidUrl() const override;
        QString prettyUrl() const override;
        QString notPlayableReason() const override;

        bool isEditable() const;

        AlbumPtr album() const override;
        ArtistPtr artist() const override;
        GenrePtr genre() const override;
        ComposerPtr composer() const override;
        YearPtr year() const override;

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setAlbumArtist( const QString &newAlbumArtist );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( int newYear );

        virtual QString title() const;
        virtual void setTitle( const QString &newTitle );

        QString comment() const override;
        virtual void setComment ( const QString &newComment );

        qint64 length() const override;

        void setFileSize( int newFileSize );
        int filesize() const override;

        int bitrate() const override;
        virtual void setBitrate( int newBitrate );

        int sampleRate() const override;
        virtual void setSamplerate( int newSamplerate );

        qreal bpm() const override;
        virtual void setBpm( const qreal newBpm );

        int trackNumber() const override;
        virtual void setTrackNumber ( int newTrackNumber );

        int discNumber() const override;
        virtual void setDiscNumber ( int newDiscNumber );

        qreal replayGain( ReplayGainTag mode ) const override;
        /* Set the track replay gain (other types unsupported) */
        void setReplayGain( qreal newReplayGain );

        QString type() const override;
        void prepareToPlay() override;

        bool inCollection() const override;
        Collections::Collection* collection() const override;

        TrackEditorPtr editor() override;
        StatisticsPtr statistics() override;

        // Meta::Statistics methods
        double score() const override;
        void setScore ( double newScore ) override;

        int rating() const override;
        void setRating ( int newRating ) override;

        QDateTime lastPlayed() const override;
        void setLastPlayed( const QDateTime &newTime ) override;

        // firstPlayed() not available in any media device

        int playCount() const override;
        void setPlayCount( const int newCount ) override;

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
        explicit MediaDeviceArtist( const QString &name );
        virtual ~MediaDeviceArtist();

        QString name() const override;

        TrackList tracks() override;

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

        QString name() const override;

        bool isCompilation() const override;
        void setIsCompilation( bool compilation );

        bool hasAlbumArtist() const override;
        ArtistPtr albumArtist() const override;
        TrackList tracks() override;

        bool hasImage( int size = 0 ) const override;
        QImage image( int size = 0 ) const override;
        bool canUpdateImage() const override;
        void setImage( const QImage &image ) override;
        virtual void setImagePath( const QString &path );

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

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
        explicit MediaDeviceComposer( const QString &name );
        virtual ~MediaDeviceComposer();

        QString name() const override;

        TrackList tracks() override;

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
        explicit MediaDeviceGenre( const QString &name );
        virtual ~MediaDeviceGenre();

        QString name() const override;

        TrackList tracks() override;

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
        explicit MediaDeviceYear( const QString &name );
        virtual ~MediaDeviceYear();

        QString name() const override;

        TrackList tracks() override;

        //MediaDeviceYear specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

