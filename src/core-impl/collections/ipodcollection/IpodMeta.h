/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz                                       *
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

#ifndef IPODMETA_H
#define IPODMETA_H

#include "MetaValues.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"

#include <QPointer>
#include <QReadWriteLock>

#include <glib.h>

struct _Itdb_Track;
typedef _Itdb_Track Itdb_Track;
class IpodCollection;

namespace IpodMeta
{
    /**
     * An iPod track. album, artist, composer etc. are invisible to outside world, they are
     * proxied in the MemoMeta track. All methods in this class are thread-safe with a few
     * exceptions that are noted in relevant method docstrings.
     */
    class Track : public Meta::Track, public Meta::Statistics, public Meta::TrackEditor
    {
        public:
            /**
             * Constructs an iPod track from an existing libgpod track structure. Caller
             * must guarantee that these are already added to the collection's itdb database.
             */
            explicit Track( Itdb_Track *ipodTrack );

            /**
             * Constructs an iPod track out of an existing track by copying its meta-data
             */
            explicit Track( const Meta::TrackPtr &origTrack );

            virtual ~Track();

            // Meta::Base methods:
            QString name() const override;

            // Meta::Track methods:
            QUrl playableUrl() const override;
            QString prettyUrl() const override;
            QString uidUrl() const override;
            QString notPlayableReason() const override;

            Meta::AlbumPtr album() const override;
            Meta::ArtistPtr artist() const override;
            Meta::ComposerPtr composer() const override;
            Meta::GenrePtr genre() const override;
            Meta::YearPtr year() const override;

            qreal bpm() const override;
            QString comment() const override;

            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;

            QDateTime createDate() const override;
            QDateTime modifyDate() const override;

            int trackNumber() const override;
            int discNumber() const override;

            qreal replayGain( Meta::ReplayGainTag mode ) const override;
            QString type() const override;

            bool inCollection() const override;
            Collections::Collection* collection() const override;

            Meta::TrackEditorPtr editor() override;
            Meta::StatisticsPtr statistics() override;

            // Meta::TrackEditor methods:
            void setAlbum( const QString &newAlbum ) override;
            void setAlbumArtist( const QString &newAlbumArtist ) override;
            void setArtist( const QString &newArtist ) override;
            void setComposer( const QString &newComposer ) override;
            void setGenre( const QString &newGenre ) override;
            void setYear( int newYear ) override;
            void setTitle( const QString &newTitle ) override;
            void setComment( const QString &newComment ) override;
            void setTrackNumber( int newTrackNumber ) override;
            void setDiscNumber( int newDiscNumber ) override;
            void setBpm( const qreal newBpm ) override;

            // Meta::Statistics methods:
            int rating() const override;
            void setRating( int newRating ) override;

            QDateTime lastPlayed() const override;
            void setLastPlayed( const QDateTime &time ) override;

            QDateTime firstPlayed() const override;
            void setFirstPlayed( const QDateTime &time ) override;

            int playCount() const override;
            int recentPlayCount() const override;
            void setPlayCount( const int playcount ) override;

            // Combined Meta::TrackEditor, Meta::Statistics methods:
            void beginUpdate() override;
            void endUpdate() override;

            // IpodMeta::Track methods:
            /**
             * Return a pointer to IpodMeta::Track given pointer to underlying libgpod
             * track. Does not attempt to create the track, so it may return null ptr if
             * there is no IpodMeta::Track associated with given libgpod track.
             */
            static Meta::TrackPtr fromIpodTrack( const Itdb_Track *ipodTrack );

            /**
             * Return a pointer to underlying libgpod track. You aren't allowed to cache
             * the pointer - IpodMeta::Track owns it. Guaranteed to be non-null and
             * constant throughout the lifetime of IpodMeta::Track.
             */
            Itdb_Track *itdbTrack() const;

            /**
             * CollectionLocation must call this method when it finishes copying the
             * track file onto iPod, before adding this track to IpodCollection.
             * Sets ipod_path, filetype_marker, transferred and size m_track fields.
             *
             * @param mountPoint a path where iPod is mounted, e.g. /media/MyiPod in local
             * encoding (use QFile::encodeName())
             * @param filePath full absolute path to copied file, must be in form
             * <@p mountPoint>/iPod_Control/Music/... - it is recommended to use
             * itdb_cp_get_dest_filename() to construct the filename
             *
             * @return true if the track was "accepted", false if not in which case you
             * shouldn't add it to collection.
             */
            bool finalizeCopying( const gchar *mountPoint, const gchar *filePath );

            /**
             * Set collection this track belongs to. If collection is not null, (re)set
             * the mount point stored in track. (affects playableUrl())
             */
            void setCollection( QPointer<IpodCollection> collection );

            // Methods for copy constructor:
            void setIsCompilation( bool newIsCompilation );
            void setImage( const QImage &newImage );
            void setLength( qint64 newLength );
            void setSampleRate( int newSampleRate );
            void setBitrate( int newBitrate );
            void setCreateDate( const QDateTime &newDate );
            void setModifyDate( const QDateTime &newDate );
            void setReplayGain( Meta::ReplayGainTag mode, qreal newReplayGain );
            void setType( const QString &newType );

        private:
            bool isEditable() const;

            /**
             * Must be called at end of every set*() method, with m_trackLock locked for
             * writing. Takes care of writing back the fields and notifying observers.
             */
            void commitIfInNonBatchUpdate( qint64 field, const QVariant &value );
            void commitIfInNonBatchUpdate();

            friend class Album; // so that is can access m_track and friends

            /**
             * Meta::Track is memory-managed using KSharedPointer to QSharedData, but
             * IpodCollection's memory management is out of our control, therefore the
             * weak pointer.
             */
            QPointer<IpodCollection> m_coll;

            /**
             * While mount point is accessible through m_track->itdb-> ..., we want to
             * remember our location even when we are removed from collection
             */
            QString m_mountPoint;

            /**
             * Associated libgpod track structure that holds all the data, we own this
             * pointer
             */
            Itdb_Track *const m_track; // yes, the address is constant, not the track

            /**
             * You must hold this lock when acessing m_track data. Beware that
             * m_track->itdb may change even with this lock hold - IpodCollection is the
             * owner of this field
             */
            mutable QReadWriteLock m_trackLock;

            /**
             * We need the temporary image file to exist for the lifetime of Track because
             * calling itdb_track_set_thumbnails() only saves the filename - the file is
             * read only when needed. If this path is non-empty, it means that the file
             * should be deleted in destructor.
             */
            QString m_tempImageFilePath;

            /**
             * Set of field types (identified by constants from MetaValues.h) changed by
             * TrackEditor or set{Rating,Score,...} not yet committed to database and
             * underlying file
             */
            Meta::FieldHash m_changedFields;

            /**
             * Number of current batch operations started by @see beginUpdate() and not
             * yet ended by @see endUpdate(). Must only be accessed with m_trackLock held.
             */
            int m_batch;

            static const quint64 m_gpodTrackUserTypeAmarokTrackPtr = Q_UINT64_C(0x416d61726f6b5472); /* AmarokTr */
    };

    /**
     * Dummy Artist that just stores its name; not visible from outside - iPod tracks are
     * proxied by MemoryMeta that creates its own Artist entities.
     */
    class Artist : public Meta::Artist
    {
        public:
            explicit Artist( const QString &name ) : m_name( name ) {}
            virtual ~Artist() {}

            QString name() const override { return m_name; }
            Meta::TrackList tracks() override { return Meta::TrackList(); }

        private:
            QString m_name;
    };

    /**
     * For performance reasons, Album stores just pointer to the tracks and reads all its
     * fields on-demand.
     */
    class Album : public Meta::Album
    {
        public:
            explicit Album( Track *track );

            QString name() const override;
            // dummy, iPod tracks are supposed to be proxied by MemoryMeta which handles this
            Meta::TrackList tracks() override { return Meta::TrackList(); }

            bool isCompilation() const override;
            bool canUpdateCompilation() const override;
            void setCompilation( bool isCompilation ) override;

            bool hasAlbumArtist() const override;
            Meta::ArtistPtr albumArtist() const override;

            bool hasImage( int size = 0 ) const override;
            QImage image( int size = 0 ) const override;
            bool canUpdateImage() const override;
            void setImage( const QImage &image ) override;
            void removeImage() override;

        private:
            AmarokSharedPointer<Track> m_track;
    };

    /**
     * Dummy Composer that just stores its name; not visible from outside - iPod tracks are
     * proxied by MemoryMeta that creates its own Composer entities.
     */
    class Composer : public Meta::Composer
    {
        public:
            explicit Composer( const QString &name ) : m_name( name ) {}
            virtual ~Composer() {}

            QString name() const override { return m_name; }
            Meta::TrackList tracks() override { return Meta::TrackList(); }

        private:
            QString m_name;
    };

    /**
     * Dummy Genre that just stores its name; not visible from outside - iPod tracks are
     * proxied by MemoryMeta that creates its own Genre entities.
     */
    class Genre : public Meta::Genre
    {
        public:
            explicit Genre( const QString &name ) : m_name( name ) {}
            virtual ~Genre() {}

            QString name() const override { return m_name; }
            Meta::TrackList tracks() override { return Meta::TrackList(); }

        private:
            QString m_name;
    };

    /**
     * Dummy Year that just stores its name; not visible from outside - iPod tracks are
     * proxied by MemoryMeta that creates its own Year entities.
     */
    class Year : public Meta::Year
    {
        public:
            explicit Year( const QString &name ) : m_name( name ) {}
            virtual ~Year() {}

            QString name() const override { return m_name; }
            Meta::TrackList tracks() override { return Meta::TrackList(); }

        private:
            QString m_name;
    };

} // namespace IpodMeta

#endif // IPODMETA_H
