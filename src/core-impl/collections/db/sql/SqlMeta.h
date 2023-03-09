/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef SQLMETA_H
#define SQLMETA_H

#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core/meta/support/MetaConstants.h"
#include "amarok_sqlcollection_export.h"
#include "FileType.h"

#include <QAtomicInt>
#include <QByteArray>
#include <QRecursiveMutex>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Capabilities {
}

class SqlRegistry;
class TrackUrlsTableCommitter;
class TrackTracksTableCommitter;
class TrackStatisticsTableCommitter;
namespace Collections {
    class SqlCollection;
}

class SqlScanResultProcessor;

namespace Meta
{

/** The SqlTrack is a Meta::Track used by the SqlCollection.
    The SqlTrack has a couple of functions for writing values and also
    some functions for getting e.g. the track id used in the underlying database.
    However it is not recommended to interface with the database directly.

    The whole class should be thread save.
*/
class AMAROK_SQLCOLLECTION_EXPORT SqlTrack : public Track, public Statistics, public TrackEditor
{
    public:
        /** Creates a new SqlTrack without.
         *  Note that the trackId and urlId are empty meaning that this track
         *  has no database representation until it's written first by setting
         *  some of the meta information.
         *  It is advisable to set at least the path.
         */
        SqlTrack( Collections::SqlCollection *collection, int deviceId,
                  const QString &rpath, int directoryId, const QString &uidUrl );
        SqlTrack( Collections::SqlCollection *collection, const QStringList &queryResult );
        ~ SqlTrack() override;

        QString name() const override;
        QString prettyName() const override;
        QUrl playableUrl() const override;
        QString prettyUrl() const override;
        QString uidUrl() const override;
        QString notPlayableReason() const override;

        Meta::AlbumPtr album() const override;
        Meta::ArtistPtr artist() const override;
        Meta::ComposerPtr composer() const override;
        Meta::YearPtr year() const override;
        Meta::GenrePtr genre() const override;

        QString type() const override;
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

        bool inCollection() const override;
        Collections::Collection* collection() const override;

        QString cachedLyrics() const override;
        void setCachedLyrics( const QString &lyrics ) override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        void addLabel( const QString &label ) override;
        void addLabel( const Meta::LabelPtr &label ) override;
        void removeLabel( const Meta::LabelPtr &label ) override;
        Meta::LabelList labels() const override;

        TrackEditorPtr editor() override;
        StatisticsPtr statistics() override;

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
        double score() const override;
        void setScore( double newScore ) override;

        int rating() const override;
        void setRating( int newRating ) override;

        QDateTime firstPlayed() const override;
        void setFirstPlayed( const QDateTime &newTime ) override;

        QDateTime lastPlayed() const override;
        void setLastPlayed( const QDateTime &newTime ) override;

        int playCount() const override;
        void setPlayCount( const int newCount ) override;

        // combined Meta::Statistics and Meta::TrackEditor methods:
        void beginUpdate() override;
        void endUpdate() override;

        // SqlTrack specific methods
        /** true if there is a collection, the file exists on disk and is writable */
        bool isEditable() const;

        void setUidUrl( const QString &uid );
        void setAlbum( int albumId );
        void setType( Amarok::FileType newType );
        void setLength( qint64 newLength );
        void setSampleRate( int newSampleRate );
        void setUrl( int deviceId, const QString &rpath, int directoryId );
        void setBitrate( int newBitrate );
        void setModifyDate( const QDateTime &newTime );
        void setReplayGain( Meta::ReplayGainTag mode, qreal value );

        /** Enables or disables writing changes to the file.
         *  This function can be useful when changes are imported from the file.
         *  In such a case writing the changes back again is stupid.
         */
        virtual void setWriteFile( const bool enable )
        { m_writeFile = enable; }

        int id() const;
        int urlId() const;
        Collections::SqlCollection* sqlCollection() const { return m_collection; }

        /** Does it's best to remove the track from database.
         *  Considered that there is no signal that says "I am now removed"
         *  this function still tries it's best to notify everyone
         *  That the track is now removed, plus it will also delete it from
         *  the database.
         */
        void remove();

        // SqlDatabase specific values

        /** Some numbers used in SqlRegistry.
         *  Update if getTrackReturnValues is updated.
         */
        enum TrackReturnIndex
        {
            returnIndex_urlId = 0,
            returnIndex_urlDeviceId = 1,
            returnIndex_urlRPath = 2,
            returnIndex_urlUid = 4,
            returnIndex_trackId = 5
        };

        // SqlDatabase specific values

        /** returns a string of all database values that can be fetched for a track */
        static QString getTrackReturnValues();
        /** returns the number of return values in getTrackReturnValues() */
        static int getTrackReturnValueCount();
        /** returns a string of all database joins that are required to fetch all values for a track*/
        static QString getTrackJoinConditions();


    protected:
        /**
         * Will commit all changes in m_cache if m_batch == 0. Must be called with m_lock
         * locked for writing.
         *
         *  commitIfInNonBatchUpdate() will do three things:
         *  1. It will update the member variables.
         *  2. It will call all write methods
         *  3. It will notify all observers and the collection about the changes.
         */
        void commitIfInNonBatchUpdate( qint64 field, const QVariant &value );
        void commitIfInNonBatchUpdate();

        void updatePlaylistsToDb( const FieldHash &fields, const QString &oldUid );
        void updateEmbeddedCoversToDb( const FieldHash &fields, const QString &oldUid );

    private:
        //helper functions
        static QString prettyTitle( const QString &filename );

        Collections::SqlCollection* const m_collection;

        QString m_title;

        // the url table
        int m_urlId;
        int m_deviceId;
        QString m_rpath;
        int m_directoryId; // only set when the urls table needs to be written
        QUrl m_url;
        QString m_uid;

        // the rest
        int m_trackId;
        int m_statisticsId;

        qint64 m_length;
        qint64 m_filesize;
        int m_trackNumber;
        int m_discNumber;
        QDateTime m_lastPlayed;
        QDateTime m_firstPlayed;
        int m_playCount;
        int m_bitrate;
        int m_sampleRate;
        int m_rating;
        double m_score;
        QString m_comment;
        qreal m_bpm;
        qreal m_albumGain;
        qreal m_albumPeakGain;
        qreal m_trackGain;
        qreal m_trackPeakGain;
        QDateTime m_createDate;
        QDateTime m_modifyDate;

        Meta::AlbumPtr m_album;
        Meta::ArtistPtr m_artist;
        Meta::GenrePtr m_genre;
        Meta::ComposerPtr m_composer;
        Meta::YearPtr m_year;

        Amarok::FileType m_filetype;

        /**
         * Number of current batch operations started by @see beginUpdate() and not
         * yet ended by @see endUpdate(). Must only be accessed with m_lock held.
         */
        int m_batchUpdate;
        bool m_writeFile;
        bool m_writeAllStatisticsFields;
        FieldHash m_cache;

        /** This ReadWriteLock is protecting all internal variables.
            It is ensuring that m_cache, m_batchUpdate and the other internal variable are
            in a consistent state all the time.
        */
        mutable QReadWriteLock m_lock;

        mutable bool m_labelsInCache;
        mutable Meta::LabelList m_labelsCache;

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class ::TrackUrlsTableCommitter;
        friend class ::TrackTracksTableCommitter;
        friend class ::TrackStatisticsTableCommitter;
};

class AMAROK_SQLCOLLECTION_EXPORT SqlArtist : public Meta::Artist
{
    public:
        SqlArtist( Collections::SqlCollection* collection, int id, const QString &name );
        ~SqlArtist() override;

        QString name() const override { return m_name; }

        virtual void invalidateCache();

        Meta::TrackList tracks() override;

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        //SQL specific methods
        int id() const { return m_id; }
    private:
        Collections::SqlCollection* const m_collection;
        const int m_id;
        const QString m_name;

        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        QMutex m_mutex;

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class Meta::SqlTrack; // needs to call notifyObservers
};

/** Represents an albums stored in the database.
    Note: The album without name is special. It will always be a compilation
    and never have a picture.
*/
class AMAROK_SQLCOLLECTION_EXPORT SqlAlbum : public Meta::Album
{
    public:
        SqlAlbum( Collections::SqlCollection* collection, int id, const QString &name, int artist );
        ~SqlAlbum() override;

        QString name() const override { return m_name; }

        virtual void invalidateCache();

        Meta::TrackList tracks() override;

        bool isCompilation() const override;
        bool canUpdateCompilation() const override { return true; }
        void setCompilation( bool compilation ) override;

        /** Returns true if this album has an artist.
         *  The following equation is always true: isCompilation() != hasAlbumArtist()
         */
        bool hasAlbumArtist() const override;

        /** Returns the album artist.
         *  Note that setting the album artist is not supported.
         *  A compilation does not have an artist and not only an empty artist.
         */
        Meta::ArtistPtr albumArtist() const override;

        //updating album images is possible for local tracks, but let's ignore it for now

        /** Returns true if the album has a cover image.
         *  @param size The maximum width or height of the result.
         *  when size is <= 1, return the full size image
         */
        bool hasImage(int size = 0) const override;
        bool canUpdateImage() const override { return true; }

        /** Returns the album cover image.
         *  Returns a default image if no specific album image could be found.
         *  In such a case it will start the cover fetcher.
         *
         *  @param size is the maximum width or height of the resulting image.
         *  when size is <= 1, return the full size image
         */
        QImage image( int size = 0 ) const override;

        QUrl imageLocation( int size = 0 ) override;
        void setImage( const QImage &image ) override;
        void removeImage() override;
        void setSuppressImageAutoFetch( const bool suppress ) override { m_suppressAutoFetch = suppress; }
        bool suppressImageAutoFetch() const override { return m_suppressAutoFetch; }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        //SQL specific methods
        int id() const { return m_id; }

        Collections::SqlCollection *sqlCollection() const { return m_collection; }

    private:
        QByteArray md5sum( const QString& artist, const QString& album, const QString& file ) const;

        /** Returns a unique key for the album cover. */
        QByteArray imageKey() const;

        /** Returns the path that the large scale image should have on the disk
         *  Does not check if the file exists.
         *  Note: not all large images have a disk cache, e.g. if they are set from outside
         *    or embedded inside an audio file.
         *    The largeDiskCache is only used for images set via setImage(QImage)
         */
        QString largeDiskCachePath() const;

        /** Returns the path that the image should have on the disk
         *  Does not check if the file exists.
         *  @param size is the maximum width or height of the resulting image.
         *         size==0 is the large image and the location of this file is completely different.
         *         there should never be a scaled cached version of the large image. it dose not make
         *         sense.
         */
        QString scaledDiskCachePath( int size ) const;

        /** Returns the path to the large image
         * Queries the database for the path of the large scale image.
         */
        QString largeImagePath();

        /** Updates the database
         *  Sets the current albums image to the given path.
         *  The path should point to a valid image.
         *  Note: setImage will not delete the already set image.
         */
        void setImage( const QString &path );

        /** Finds or creates a magic value in the database which tells Amarok not to auto fetch an image since it has been explicitly unset.
        */
        int unsetImageId() const;

        Collections::SqlCollection* const m_collection;

        enum TracksLoadingStatus
        {
            NotLoaded,
            Loading,
            Loaded
        };

        const QString m_name;
        const int m_id; // the id of this album in the database
        const int m_artistId;
        int m_imageId;
        mutable QString m_imagePath; // path read from the database
        mutable bool m_hasImage; // true if we have an original image
        mutable bool m_hasImageChecked; // true if hasImage was checked

        mutable int m_unsetImageId; // this is the id of the unset magic value in the image sql database
        static const QString AMAROK_UNSET_MAGIC;

        TracksLoadingStatus m_tracksLoaded;
        bool m_suppressAutoFetch;
        Meta::ArtistPtr m_artist;
        Meta::TrackList m_tracks;
        mutable QRecursiveMutex m_mutex;

        //TODO: add album artist

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class Meta::SqlTrack; // needs to set images directly
        friend class ::SqlScanResultProcessor; // needs to set images directly
};

class AMAROK_SQLCOLLECTION_EXPORT SqlComposer : public Meta::Composer
{
    public:
        SqlComposer( Collections::SqlCollection* collection, int id, const QString &name );

        QString name() const override { return m_name; }

        virtual void invalidateCache();

        Meta::TrackList tracks() override;

        //SQL specific methods
        int id() const { return m_id; }

    private:
        Collections::SqlCollection* const m_collection;

        const int m_id;
        const QString m_name;

        bool m_tracksLoaded;
        bool m_tracksLoading;
        Meta::TrackList m_tracks;
        QMutex m_mutex;

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class Meta::SqlTrack; // needs to call notifyObservers
};

class SqlGenre : public Meta::Genre
{
    public:
        SqlGenre( Collections::SqlCollection* collection, int id, const QString &name );

        QString name() const override { return m_name; }

        /** Invalidates the tracks cache */
        /** Invalidates the tracks cache */
        virtual void invalidateCache();

        Meta::TrackList tracks() override;

        //SQL specific methods
        int id() const { return m_id; }

    private:
        Collections::SqlCollection* const m_collection;

        const int m_id;
        const QString m_name;

        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        QMutex m_mutex;

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class Meta::SqlTrack; // needs to call notifyObservers
};

class AMAROK_SQLCOLLECTION_EXPORT SqlYear : public Meta::Year
{
    public:
        SqlYear( Collections::SqlCollection* collection, int id, int year );

        QString name() const override { return QString::number(m_year); }

        int year() const override { return m_year; }

        /** Invalidates the tracks cache */
        virtual void invalidateCache();

        Meta::TrackList tracks() override;

        //SQL specific methods
        int id() const { return m_id; }

    private:
        Collections::SqlCollection* const m_collection;
        const int m_id;
        const int m_year;


        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        QMutex m_mutex;

        friend class ::SqlRegistry; // needs to call notifyObservers
        friend class Meta::SqlTrack; // needs to call notifyObservers
};

class AMAROK_SQLCOLLECTION_EXPORT SqlLabel : public Meta::Label
{
public:
    SqlLabel( Collections::SqlCollection *collection, int id, const QString &name );

    QString name() const override { return m_name; }

    /** Invalidates the tracks cache */
    virtual void invalidateCache();

    virtual Meta::TrackList tracks();

    //SQL specific methods
    int id() const { return m_id; }

private:
    Collections::SqlCollection* const m_collection;
    const int m_id;
    const QString m_name;

    bool m_tracksLoaded;
    Meta::TrackList m_tracks;
    QMutex m_mutex;

    friend class ::SqlRegistry; // needs to call notifyObservers
    friend class Meta::SqlTrack; // needs to call notifyObservers
};

typedef AmarokSharedPointer<SqlTrack> SqlTrackPtr;
typedef AmarokSharedPointer<SqlArtist> SqlArtistPtr;
typedef AmarokSharedPointer<SqlAlbum> SqlAlbumPtr;
typedef AmarokSharedPointer<SqlComposer> SqlComposerPtr;
typedef AmarokSharedPointer<SqlGenre> SqlGenrePtr;
typedef AmarokSharedPointer<SqlYear> SqlYearPtr;

}

#endif /* SQLMETA_H */
