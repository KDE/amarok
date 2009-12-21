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

#include "meta/Meta.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVariant>

class AlbumCapabilityDelegate;
class ArtistCapabilityDelegate;
class TrackCapabilityDelegate;
class SqlCollection;
class QAction;

namespace Meta
{

class SqlTrack : public Meta::Track
{
    public:
        /** returns a string of all database values that can be fetched for a track */
        static QString getTrackReturnValues();
        /** returns the number of return values in getTrackReturnValues() */
        static int getTrackReturnValueCount();
        static TrackPtr getTrack( int deviceid, const QString &rpath, SqlCollection *collection );
        static TrackPtr getTrackFromUid( const QString &uid, SqlCollection *collection );

        SqlTrack( SqlCollection *collection, const QStringList &queryResult );
        ~ SqlTrack();

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

        virtual float bpm() const { return m_bpm; }
        virtual void setBpm( const float newBpm );

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

        virtual void beginMetaDataUpdate();
        virtual void endMetaDataUpdate();
        virtual void abortMetaDataUpdate();
        virtual void setWriteAllStatisticsFields( const bool enable ) { m_writeAllStatisticsFields = enable; }

        virtual void finishedPlaying( double playedFraction );

        virtual bool inCollection() const;
        virtual Amarok::Collection* collection() const;

        virtual QString cachedLyrics() const;
        virtual void setCachedLyrics( const QString &lyrics );

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        //SqlTrack specific methods
        int deviceid() const { return m_deviceid; }
        QString rpath() const { return m_rpath; }
        int trackId() const { return m_trackId; }
        SqlCollection* sqlCollection() const { return m_collection; }
        void refreshFromDatabase( const QString &uid, SqlCollection* collection, bool updateObservers = true );
        void updateData( const QStringList &result, bool forceUpdates = false );
        void setCapabilityDelegate( TrackCapabilityDelegate *delegate );

    protected:
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

        SqlCollection* m_collection;
        TrackCapabilityDelegate *m_capabilityDelegate;

        QString m_title;
        KUrl m_url;

        int m_deviceid;
        QString m_rpath;
        int m_trackId;

        int m_length;
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
        float m_bpm;
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

        QString m_newUid;
};

class SqlArtist : public Meta::Artist
{
    public:
        SqlArtist( SqlCollection* collection, int id, const QString &name );
        ~SqlArtist();

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; } //change if necessary
        virtual QString sortableName() const;

        void updateData( SqlCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        virtual Meta::AlbumList albums();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        //SQL specific methods
        int id() const { return m_id; }
        void setCapabilityDelegate( ArtistCapabilityDelegate *delegate ) { m_delegate = delegate; }


    private:
        SqlCollection* m_collection;
        ArtistCapabilityDelegate *m_delegate;
        QString m_name;
        int m_id;
        mutable QString m_modifiedName;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        bool m_albumsLoaded;
        Meta::AlbumList m_albums;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

};

class SqlAlbum : public Meta::Album
{
    public:
        SqlAlbum( SqlCollection* collection, int id, const QString &name, int artist );
        ~SqlAlbum();

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( SqlCollection* collection, int id, const QString &name, int artist );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        virtual bool isCompilation() const;

        virtual bool hasAlbumArtist() const;
        virtual Meta::ArtistPtr albumArtist() const;

        //updating album images is possible for local tracks, but let's ignore it for now
        virtual bool hasImage( int size = 1 ) const;
        virtual bool canUpdateImage() const { return true; }
        virtual QPixmap image( int size = 1 );
        virtual KUrl imageLocation( int size = 1 );
        virtual void setImage( const QPixmap &pixmap );
        virtual void removeImage();
        virtual void setSuppressImageAutoFetch( const bool suppress ) { m_suppressAutoFetch = suppress; }
        virtual bool suppressImageAutoFetch() const { return m_suppressAutoFetch; }

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;

        virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );

        //SQL specific methods
        int id() const { return m_id; }

        void setCompilation( bool compilation );
        void setCapabilityDelegate( AlbumCapabilityDelegate *delegate ) { m_delegate = delegate; }
        SqlCollection *sqlCollection() const { return m_collection; }

    private:
        QByteArray md5sum( const QString& artist, const QString& album, const QString& file ) const;
        QString createScaledImage( QString path, int size ) const;
        QString findCachedImage( int size ) const;
        QString findLargeCachedImage() const;
        QString findImage( int size );
        QString imageKey() const;
        void updateImage( const QString path ) const; // Updates the database to ensure the album has the correct path
        // Finds or creates a magic value in the database which tells Amarok not to auto fetch an image since it has been explicitly unset.
        int unsetImageId() const;

    private:
        SqlCollection* m_collection;
        AlbumCapabilityDelegate *m_delegate;
        QString m_name;
        int m_id;
        int m_artistId;
        mutable bool m_hasImage;
        mutable bool m_hasImageChecked;
        mutable int m_unsetImageId;
        static const QString AMAROK_UNSET_MAGIC;
        mutable QHash<int, QString> m_images; // Cache mapping size -> path. hash used for O(1) insertion and O(1) lookup
        bool m_tracksLoaded;
        bool m_suppressAutoFetch;
        Meta::ArtistPtr m_artist;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;

        //TODO: add album artist
};

class SqlComposer : public Meta::Composer
{
    public:
        SqlComposer( SqlCollection* collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( SqlCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        SqlCollection* m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;
};

class SqlGenre : public Meta::Genre
{
    public:
        SqlGenre( SqlCollection* collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( SqlCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        SqlCollection* m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;
};

class SqlYear : public Meta::Year
{
    public:
        SqlYear( SqlCollection* collection, int id, const QString &name );

        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return m_name; }

        void updateData( SqlCollection* collection, int id, const QString &name );

        virtual void invalidateCache();

        virtual Meta::TrackList tracks();

        //SQL specific methods
        int id() const { return m_id; }

    private:
        SqlCollection* m_collection;
        QString m_name;
        int m_id;
        bool m_tracksLoaded;
        Meta::TrackList m_tracks;
        //QReadWriteLock does not support lock upgrades :(
        //see http://www.trolltech.com/developer/task-tracker/index_html?method=entry&id=131880
        //switch to QReadWriteLock as soon as it does!
        QMutex m_mutex;
};

}

#endif /* SQLMETA_H */
