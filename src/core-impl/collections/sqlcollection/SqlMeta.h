/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
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
#include "amarok_sqlcollection_export.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWeakPointer>

namespace Capabilities {
    class AlbumCapabilityDelegate;
    class ArtistCapabilityDelegate;
    class TrackCapabilityDelegate;
}
class QAction;

namespace Collections {
    class SqlCollection;
}

namespace Meta
{

class SqlTrack : public Meta::Track
{
    public:
        /** returns a string of all database values that can be fetched for a track */
        static QString getTrackReturnValues();
        /** returns the number of return values in getTrackReturnValues() */
        static int getTrackReturnValueCount();
        static TrackPtr getTrack( int deviceid, const QString &rpath, Collections::SqlCollection *collection );
        static TrackPtr getTrackFromUid( const QString &uid, Collections::SqlCollection *collection );

        SqlTrack( QWeakPointer<Collections::SqlCollection> collection, const QStringList &queryResult );
        ~SqlTrack();

        /** returns the title of this track as stored in the database **/
        virtual QString name() const { return m_title; }
        /** returns the title of the track if existing in the database,
            a value deduced from the file name otherwise */
        virtual QString prettyName() const;
        /** returns "[artist] - [title]" if both are stored in the database,
            a value deduced from the file name otherwise */
        virtual QString fullPrettyName() const;

        /** returns the KUrl object describing the position of the track */
        virtual KUrl playableUrl() const { return m_url; }
        /** returns a string describing the position of the track; same as url() */
        virtual QString prettyUrl() const { return m_url.path(); }
        /** returns a string describing the position of the track */
        virtual QString uidUrl() const { return m_uid; }

        /** true if there is a collection and the file exists on disk */
        virtual bool isPlayable() const;
        /** true if there is a collection, the file exists on disk and is writeable */
        virtual bool isEditable() const;

        virtual Meta::AlbumPtr album() const { return m_album; }
        virtual void setAlbum( const QString &newAlbum );
        virtual void setArtist( const QString &newArtist );
        virtual Meta::ArtistPtr artist() const { return m_artist; }
        virtual Meta::ComposerPtr composer() const { return m_composer; }
        virtual void setComposer( const QString &newComposer );
        virtual Meta::YearPtr year() const { return m_year; }
        virtual void setYear( const QString &newYear );
        virtual Meta::GenrePtr genre() const { return m_genre; }
        virtual void setGenre( const QString &newGenre );

        virtual QString type() const;

        //helper functions
        static QString prettyTitle( const QString &filename );

        virtual void setTitle( const QString &newTitle );

        virtual void setUrl( const QString &url );
        virtual void setUrl( const int deviceid, const QString &rpath );

        virtual qreal bpm() const { return m_bpm; }
        virtual void setBpm( const qreal newBpm );

        virtual QString comment() const { return m_comment; }
        virtual void setComment( const QString &newComment );

        virtual double score() const { return m_score; }
        virtual void setScore( double newScore );

        virtual int rating() const { return m_rating; }
        virtual void setRating( int newRating );

        virtual qint64 length() const { return m_length; }
        virtual int filesize() const { return m_filesize; }
        virtual int sampleRate() const { return m_sampleRate; }
        virtual int bitrate() const { return m_bitrate; }
        virtual QDateTime createDate() const { return m_createDate; }

        virtual int trackNumber() const { return m_trackNumber; }
        virtual void setTrackNumber( int newTrackNumber );

        virtual int discNumber() const { return m_discNumber; }
        virtual void setDiscNumber( int newDiscNumber );

        virtual uint firstPlayed() const { return m_firstPlayed; }
        virtual void setFirstPlayed( const uint newTime );

        virtual uint lastPlayed() const { return m_lastPlayed; }
        virtual void setLastPlayed( const uint newTime );

        virtual int playCount() const { return m_playCount; }
        virtual void setPlayCount( const int newCount );

        virtual qreal replayGain( ReplayGainMode mode ) const
        { return ( mode == AlbumReplayGain ) ? m_albumGain : m_trackGain; }
        virtual qreal replayPeakGain( ReplayGainMode mode ) const
        { return ( mode == AlbumReplayGain ) ? m_albumPeakGain : m_trackPeakGain; }

        virtual void setUidUrl( const QString &uid );
        virtual void setUid( const QString &newUidOwner, const QString &newUid );

        virtual void beginMetaDataUpdate();
        virtual void endMetaDataUpdate();
        virtual void setWriteAllStatisticsFields( const bool enable ) { m_writeAllStatisticsFields = enable; }

        virtual void finishedPlaying( double playedFraction );

        virtual bool inCollection() const;
        virtual Collections::Collection* collection() const;

        virtual QString cachedLyrics() const;
        virtual void setCachedLyrics( const QString &lyrics );

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        virtual void addLabel( const QString &label );
        virtual void addLabel( const Meta::LabelPtr &label );
        virtual void removeLabel( const Meta::LabelPtr &label );
        virtual Meta::LabelList labels() const;

        //SqlTrack specific methods
        int deviceid() const { return m_deviceid; }
        QString rpath() const { return m_rpath; }
        int trackId() const { return m_trackId; }
        QWeakPointer<Collections::SqlCollection> sqlCollection() const { return m_collection; }
        AMAROK_SQLCOLLECTION_EXPORT_TESTS void refreshFromDatabase( const QString &uid, QWeakPointer<Collections::SqlCollection> collection, bool updateObservers = true );
        void updateData( const QStringList &result, bool forceUpdates = false );
        void setCapabilityDelegate( Capabilities::TrackCapabilityDelegate *delegate );

    protected:
        /** Will commit all changes in m_cache.
         *  commitMetaDataChanges will do four things:<br>
         *  1. It will update the member variables.
         *  2. It will call writeMetaDataToFile
         *  3. It will call writeMetaDataToDB
         *  4. It will call updateStatisticsInDB
         *  5. It will notify all observers and the collection about the changes.
         */
        void commitMetaDataChanges();
        void writeMetaDataToFile();
        void writeMetaDataToDb( const QStringList &fields );
        void writeMetaDataToDb( const QString &field ) { writeMetaDataToDb( QStringList( field ) ); }
        void updateStatisticsInDb( const QStringList &fields );
        void updateStatisticsInDb( const QString &field ) { updateStatisticsInDb( QStringList( field ) ); }

    private:
        /** returns a string of all database joins that are required to fetch all values for a track*/
        static QString getTrackJoinConditions();
        void updateFileSize();

        QWeakPointer<Collections::SqlCollection> m_collection;
        Capabilities::TrackCapabilityDelegate *m_capabilityDelegate;

        QString m_title;
        KUrl m_url;

        int m_deviceid;
        QString m_rpath;
        int m_trackId;

        qint64 m_length;
        qint64 m_filesize;
        int m_trackNumber;
        int m_discNumber;
        uint m_lastPlayed;
        uint m_firstPlayed;
        int m_playCount;
        int m_bitrate;
        int m_sampleRate;
        int m_rating;
        double m_score;
        QString m_comment;
        qreal m_bpm;
        QString m_uid;
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
        bool m_writeAllStatisticsFields;
        QVariantMap m_cache;

        mutable bool m_labelsInCache;
        mutable Meta::LabelList m_labelsCache;

        QString m_newUid;
};

class SqlArtist : public Meta::Artist
{
    public:
        SqlArtist( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );
        ~SqlArtist();

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; } //change if necessary

        void updateData( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        virtual Meta::AlbumList albums();

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //SQL specific methods
        int id() const { return m_id; }
        void setCapabilityDelegate( Capabilities::ArtistCapabilityDelegate *delegate ) { m_delegate = delegate; }


    private:
        QWeakPointer<Collections::SqlCollection> m_collection;
        Capabilities::ArtistCapabilityDelegate *m_delegate;
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

        friend class SqlTrack; // needs to call notifyObservers
};

class SqlAlbum : public Meta::Album
{
    public:
        SqlAlbum( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name, int artist );
        ~SqlAlbum();

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name, int artist );

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

        /** Returns true if the album has a cover image.
         *  @param size The maximum width or height of the result.
         *  when size is <= 1, return the full size image
         */
        virtual bool hasImage(int size = 1) const;
        virtual bool canUpdateImage() const { return true; }

        /** Returns the album cover image.
         *  Returns a default image if no specific album image could be found.
         *  In such a case it will starte the cover fetcher.
         *
         *  @param size is the maximum width or height of the resulting image.
         *  when size is <= 1, return the full size image
         */
        virtual QPixmap image( int size = 1 );

        virtual KUrl imageLocation( int size = 1 );
        virtual void setImage( const QPixmap &pixmap );
        virtual void removeImage();
        virtual void setSuppressImageAutoFetch( const bool suppress ) { m_suppressAutoFetch = suppress; }
        virtual bool suppressImageAutoFetch() const { return m_suppressAutoFetch; }

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        //SQL specific methods
        int id() const { return m_id; }

        /** Set the compilation flag.
         *  If the compilation flag is set to "false" then all songs
         *  with different artists will be moved to other albums, possibly even
         *  creating them.
         */
        void setCompilation( bool compilation );
        void setCapabilityDelegate( Capabilities::AlbumCapabilityDelegate *delegate ) { m_delegate = delegate; }
        QWeakPointer<Collections::SqlCollection> sqlCollection() const { return m_collection; }

    private:

        /**  check if we have an image inside a track (e.g. mp3 APIC)
        */
        bool hasEmbeddedImage() const;

        /** Returns the embedded large scale image */
        QImage getEmbeddedImage() const;

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

    private:
        QWeakPointer<Collections::SqlCollection> m_collection;
        Capabilities::AlbumCapabilityDelegate *m_delegate;
        QString m_name;
        int m_id; // the id of this album in the database
        int m_artistId;
        int m_imageId;
        mutable QString m_imagePath; // path read from the database
        mutable bool m_hasImage; // true if we have an original image
        mutable bool m_hasImageChecked; // true if hasImage was checked
        QSet<QString> m_pixmapCacheIds; // all our image keys
        bool m_pixmapCacheDirty; // true if we should remove our images from the cache

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

        friend class SqlTrack; // needs to call notifyObservers
};

class SqlComposer : public Meta::Composer
{
    public:
        SqlComposer( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        QWeakPointer<Collections::SqlCollection> m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class SqlTrack; // needs to call notifyObservers
};

class SqlGenre : public Meta::Genre
{
    public:
        SqlGenre( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        QWeakPointer<Collections::SqlCollection> m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class SqlTrack; // needs to call notifyObservers
};

class SqlYear : public Meta::Year
{
    public:
        SqlYear( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        QWeakPointer<Collections::SqlCollection> m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        friend class SqlTrack; // needs to call notifyObservers
};

class SqlLabel : public Meta::Label
{
public:
    SqlLabel( QWeakPointer<Collections::SqlCollection> collection, int id, const QString &name );

    virtual QString name() const { return m_name; }
    virtual QString prettyName() const { return m_name; }

    virtual void invalidateCache();

    virtual Meta::TrackList tracks();

    //SQL specific methods
    int id() const { return m_id; }

private:
    QWeakPointer<Collections::SqlCollection> m_collection;
    QString m_name;
    int m_id;
    bool m_tracksLoaded;
    Meta::TrackList m_tracks;
    //QReadWriteLock does not support lock upgrades :(
    //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
    //switch to QReadWriteLock as soon as it does!
    QMutex m_mutex;

    friend class SqlTrack; // needs to call notifyObservers
};

}

#endif /* SQLMETA_H */
