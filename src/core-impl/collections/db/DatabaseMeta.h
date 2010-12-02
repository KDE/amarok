/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2010 Jeff Mitchell <mitchell@kde.org>                                  *
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

#ifndef DATABASEMETA_H
#define DATABASEMETA_H

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "amarok_databasecollection_export.h"

#include <QByteArray>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Capabilities {
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}
class QAction;

namespace Collections {
    class DatabaseCollection;
}

class ScanResultProcessor;

namespace Meta
{

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseTrack : public Meta::Track
{
    public:
        /** Creates a new DatabaseTrack without.
         *  Note that the trackId and urlId are empty meaning that this track
         *  has no database representation until it's written first by setting
         *  some of the meta information.
         *  It is advisable to set at least the path.
         */
        DatabaseTrack( Collections::DatabaseCollection *collection, int deviceId,
                  const QString &rpath, int directoryId, const QString uidUrl );
        DatabaseTrack( Collections::DatabaseCollection *collection, const QStringList &queryResult );
        ~ DatabaseTrack();

        /** returns the title of this track as stored in the database **/
        virtual QString name() const { return m_title; }
        /** returns "[artist] - [title]" if both are stored in the database,
            a value deduced from the file name otherwise */
        virtual QString fullPrettyName() const;

        /** returns the KUrl object describing the position of the track */
        virtual KUrl playableUrl() const { return m_url; }

        /** returns a string describing the position of the track; same as url() */
        virtual QString prettyUrl() const { return m_url.path(); }

        /** returns a string describing the position of the track */
        virtual QString uidUrl() const { return m_uid; }
        virtual void setUidUrl( const QString &uid );

        /** true if there is a collection and the file exists on disk */
        virtual bool isPlayable() const;
        /** true if there is a collection, the file exists on disk and is writeable */
        virtual bool isEditable() const;

        virtual Meta::AlbumPtr album() const { return m_album; }
        virtual void setAlbum( const QString &newAlbum );
        virtual void setAlbum( int albumId );
        virtual void setArtist( const QString &newArtist );
        virtual Meta::ArtistPtr artist() const { return m_artist; }
        virtual Meta::ComposerPtr composer() const { return m_composer; }
        virtual void setComposer( const QString &newComposer );
        virtual Meta::YearPtr year() const { return m_year; }
        virtual void setYear( int newYear );
        virtual Meta::GenrePtr genre() const { return m_genre; }
        virtual void setGenre( const QString &newGenre );

        virtual QString type() const;

        //helper functions
        static QString prettyTitle( const QString &filename );

        virtual void setTitle( const QString &newTitle );

        virtual void setUrl( int deviceId, const QString &rpath, int directoryId );

        virtual qreal bpm() const { return m_bpm; }
        virtual void setBpm( const qreal newBpm );

        virtual QString comment() const { return m_comment; }
        virtual void setComment( const QString &newComment );

        virtual double score() const { return m_score; }
        virtual void setScore( double newScore );

        virtual int rating() const { return m_rating; }
        virtual void setRating( int newRating );

        virtual qint64 length() const { return m_length; }
        virtual void setLength( qint64 newLength );

        virtual int filesize() const { return m_filesize; }

        virtual int sampleRate() const { return m_sampleRate; }
        virtual void setSampleRate( int newSampleRate );

        virtual int bitrate() const { return m_bitrate; }
        virtual void setBitrate( int newBitrate );

        virtual QDateTime createDate() const { return m_createDate; }

        virtual int trackNumber() const { return m_trackNumber; }
        virtual void setTrackNumber( int newTrackNumber );

        virtual int discNumber() const { return m_discNumber; }
        virtual void setDiscNumber( int newDiscNumber );

        virtual QDateTime firstPlayed() const { return m_firstPlayed; }
        virtual void setFirstPlayed( const QDateTime &newTime );

        virtual QDateTime lastPlayed() const { return m_lastPlayed; }
        virtual void setLastPlayed( const QDateTime &newTime );

        virtual int playCount() const { return m_playCount; }
        virtual void setPlayCount( const int newCount );

        virtual qreal replayGain( Meta::ReplayGainTag mode ) const;
        virtual void setReplayGain( Meta::ReplayGainTag mode, qreal value );

        virtual void beginMetaDataUpdate();
        virtual void endMetaDataUpdate();

        /** Enables or disables writing changes to the file.
         *  This function can be useful when changes are imported from the file.
         *  In such a case writing the changes back again is stupid.
         */
        virtual void setWriteFile( const bool enable )
        { m_writeFile = enable; }

        virtual void finishedPlaying( double playedFraction );

        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

        virtual QString cachedLyrics() const = 0;
        virtual void setCachedLyrics( const QString &lyrics ) = 0;

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        virtual void addLabel( const QString &label );
        virtual void addLabel( const Meta::LabelPtr &label ) = 0;
        virtual void removeLabel( const Meta::LabelPtr &label ) = 0;
        virtual Meta::LabelList labels() const;

        // DatabaseTrack specific methods

        int deviceId() const { return m_deviceId; }
        QString rpath() const { return m_rpath; }
        int id() const { return m_trackId; }
        Collections::DatabaseCollection* sqlCollection() const { return m_collection; }

        /** Does it's best to remove the track from database.
         *  Considered that there is no signal that says "I am now removed"
         *  this function still tries it's best to notify everyone
         *  That the track is now removed, plus it will also delete it from
         *  the database.
         *  Do not call this directly as it does not clean up the registy.
         *  Call m_registry->deleteTrack() instead
         */
        virtual void remove() = 0;

        // DatabaseDatabase specific values

        /** Some numbers used in DatabaseRegistry.
         *  Update if getTrackReturnValues is updated.
         */
        enum TrackReturnIndex
        {
            returnIndex_urlId = 0,
            returnIndex_urlDeviceId = 1,
            returnIndex_urlRPath = 2,
            returnIndex_urlUid = 3,
            returnIndex_trackId = 4
        };

        /** returns a string of all database values that can be fetched for a track */
        static QString getTrackReturnValues();
        /** returns the number of return values in getTrackReturnValues() */
        static int getTrackReturnValueCount();
        /** returns a string of all database joins that are required to fetch all values for a track*/
        static QString getTrackJoinConditions();


    protected:
        /** Will commit all changes in m_cache.
         *  commitMetaDataChanges will do three things:<br>
         *  1. It will update the member variables.
         *  2. It will call all write methods
         *  3. It will notify all observers and the collection about the changes.
         */
        void commitMetaDataChanges();
        void writeMetaDataToFile();
        virtual void writeMetaDataToDb( const FieldHash &fields ) = 0;
        virtual void writeUrlToDb( const FieldHash &fields ) = 0;
        virtual void writePlaylistsToDb( const FieldHash &fields, const QString &oldUid ) = 0;
        virtual void writeStatisticsToDb( const FieldHash &fields ) = 0;
        void writeStatisticsToDb( qint64 field )
        {
            FieldHash fields;
            fields.insert( field, QVariant() );
            writeStatisticsToDb( fields );
        }

    private:
        Collections::DatabaseCollection* m_collection;

        QString m_title;

        // the url table
        int m_urlId;
        int m_deviceId;
        QString m_rpath;
        int m_directoryId; // only set when the urls table needs to be written
        KUrl m_url;
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

        Meta::AlbumPtr m_album;
        Meta::ArtistPtr m_artist;
        Meta::GenrePtr m_genre;
        Meta::ComposerPtr m_composer;
        Meta::YearPtr m_year;

        bool m_batchUpdate;
        bool m_writeFile;
        bool m_writeAllStatisticsFields;
        FieldHash m_cache;

        /** This mutex is protecting the track from changes from different threads.
            All set functions should be protected.
        */
        QMutex m_mutex;

        mutable bool m_labelsInCache;
        mutable Meta::LabelList m_labelsCache;
};

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseArtist : public Meta::Artist
{
    public:
        DatabaseArtist( Collections::DatabaseCollection* collection, int id, const QString &name );
        ~DatabaseArtist();

        virtual QString name() const { return m_name; }

        // void updateData( Collections::DatabaseCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        virtual Meta::AlbumList albums();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //DATABASE specific methods
        int id() const { return m_id; }

    private:
        Collections::DatabaseCollection* m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        bool m_albumsLoaded;
        Meta::AlbumList m_albums;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class DatabaseTrack; // needs to call notifyObservers
};

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseAlbum : public Meta::Album
{
    public:
        DatabaseAlbum( Collections::DatabaseCollection* collection, int id, const QString &name, int artist );
        ~DatabaseAlbum();

        virtual QString name() const { return m_name; }

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        /** Returns true if this album is a compilation, meaning that the included songs are from different artists.
        */
        virtual bool isCompilation() const;

        /** Returns true if this album has an artist.
         *  The following equation is always true: isCompilation() != hasAlbumArtist()
         */
        virtual bool hasAlbumArtist() const;

        /** Returns the album artist.
         *  Note that setting the album artist is not supported.
         */
        virtual Meta::ArtistPtr albumArtist() const;

        //updating album images is possible for local tracks, but let's ignore it for now

        virtual void setSuppressImageAutoFetch( const bool suppress ) { m_suppressAutoFetch = suppress; }
        virtual bool suppressImageAutoFetch() const { return m_suppressAutoFetch; }

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //DATABASE specific methods
        int id() const { return m_id; }

        /** Set the compilation flag.
         *  Actually it does not cange this album but instead moves
         *  the tracks to other albums (e.g. one with the same name which is a
         *  compilation)
         *  If the compilation flag is set to "false" then all songs
         *  with different artists will be moved to other albums, possibly even
         *  creating them.
         */
        void setCompilation( bool compilation );
        Collections::DatabaseCollection *sqlCollection() const { return m_collection; }

    private:

        /**  check if we have an image inside a track (e.g. mp3 APIC)
        */
        bool hasEmbeddedImage() const;

        /** Returns the embedded large scale image */
        virtual QImage getEmbeddedImage() const = 0;

        QByteArray md5sum( const QString& artist, const QString& album, const QString& file ) const;

        /** Returns a unique key for the album cover. */
        QByteArray imageKey() const;

        /** Returns the path that the large scale image should have on the disk
         *  Does not check if the file exists.
         *  Note: not all large images have a disk cache, e.g. if they are set from outside
         *    or embedded inside an audio file.
         *    The largeDiskCache is only used for images set via setImage(QPixmap)
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
        virtual QString largeImagePath() = 0;

        /** Updates the database
         *  Sets the current albums image to the given path.
         *  The path should point to a valid image.
         *  Note: setImage will not delete the already set image.
         */
       // virtual void setImage( const QString &path ) = 0;

       /** Finds or creates a magic value in the database which tells Amarok not to auto fetch an image since it has been explicitly unset.
       */
       virtual int unsetImageId() const = 0;

    private:
        Collections::DatabaseCollection* m_collection;
        QString m_name;
        int m_id; // the id of this album in the database
        int m_artistId;
        int m_imageId;
        mutable QString m_imagePath; // path read from the database
        mutable bool m_hasImage; // true if we have an original image
        mutable bool m_hasImageChecked; // true if hasImage was checked

        mutable int m_unsetImageId; // this is the id of the unset magic value in the image sql database
        static const QString AMAROK_UNSET_MAGIC;

        bool m_tracksLoaded;
        bool m_suppressAutoFetch;
        Meta::ArtistPtr m_artist;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        //TODO: add album artist

        friend class DatabaseTrack; // needs to call notifyObservers
        friend class ::ScanResultProcessor; // needs to set images directly
};

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseComposer : public Meta::Composer
{
    public:
        DatabaseComposer( Collections::DatabaseCollection* collection, int id, const QString &name );

        virtual QString name() const { return m_name; }

        // void updateData( Collections::DatabaseCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //DATABASE specific methods
        int id() const { return m_id; }

    private:
        Collections::DatabaseCollection* m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class DatabaseTrack; // needs to call notifyObservers
};

class DatabaseGenre : public Meta::Genre
{
    public:
        DatabaseGenre( Collections::DatabaseCollection* collection, int id, const QString &name );

        virtual QString name() const { return m_name; }

        /** Invalidates the tracks cache */
        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //DATABASE specific methods
        int id() const { return m_id; }

    private:
        Collections::DatabaseCollection* m_collection;
        int m_id;
        QString m_name;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class DatabaseTrack; // needs to call notifyObservers
};

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseYear : public Meta::Year
{
    public:
        DatabaseYear( Collections::DatabaseCollection* collection, int id, int year );

        virtual QString name() const { return QString::number(m_year); }
        virtual int year() const { return m_year; }

        /** Invalidates the tracks cache */
        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //DATABASE specific methods
        int id() const { return m_id; }

    private:
        Collections::DatabaseCollection* m_collection;
        int m_id;
        int m_year;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class DatabaseTrack; // needs to call notifyObservers
};

class AMAROK_DATABASECOLLECTION_EXPORT_TESTS DatabaseLabel : public Meta::Label
{
public:
    DatabaseLabel( Collections::DatabaseCollection *collection, int id, const QString &name );

    virtual QString name() const { return m_name; }

    /** Invalidates the tracks cache */
    virtual void invalidateCache();

    virtual Meta::TrackList tracks();

    //DATABASE specific methods
    int id() const { return m_id; }

private:
    Collections::DatabaseCollection *m_collection;
    int m_id;
    QString m_name;
    bool m_tracksLoaded;
    Meta::TrackList m_tracks;
    //QReadWriteLock does not support lock upgrades :(
    //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
    //switch to QReadWriteLock as soon as it does!
    QMutex m_mutex;

    friend class DatabaseTrack; // needs to call notifyObservers
};

}

#endif /* DATABASEMETA_H */
