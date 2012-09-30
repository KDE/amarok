/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_META_H
#define AMAROK_META_H

#include "amarok_export.h"
#include "MetaReplayGain.h"

#include "core/interfaces/MetaCapability.h"

#include <QList>
#include <QMetaType>
#include <QMutex>
#include <QImage>
#include <QDateTime>
#include <QSet>
#include <QSharedData>
#include <QString>

#include <KSharedPtr>
#include <KUrl>

namespace Collections
{
    class Collection;
    class QueryMaker;
}
class PersistentStatisticsStore;

namespace Meta
{
    class Base;
    class Track;
    class Artist;
    class Album;
    class Genre;
    class Composer;
    class Year;
    class Label;

    class Statistics;
    typedef KSharedPtr<Statistics> StatisticsPtr;
    typedef KSharedPtr<const Statistics> ConstStatisticsPtr;

    typedef KSharedPtr<Base> DataPtr;
    typedef QList<DataPtr> DataList;
    typedef KSharedPtr<Track> TrackPtr;
    typedef QList<TrackPtr> TrackList;
    typedef KSharedPtr<Artist> ArtistPtr;
    typedef QList<ArtistPtr> ArtistList;
    typedef KSharedPtr<Album> AlbumPtr;
    typedef QList<AlbumPtr> AlbumList;
    typedef KSharedPtr<Composer> ComposerPtr;
    typedef QList<ComposerPtr> ComposerList;
    typedef KSharedPtr<Genre> GenrePtr;
    typedef QList<GenrePtr> GenreList;
    typedef KSharedPtr<Year> YearPtr;
    typedef QList<YearPtr> YearList;
    typedef KSharedPtr<Label> LabelPtr;
    typedef QList<LabelPtr> LabelList;

    class AMAROK_CORE_EXPORT Observer
    {
        friend class Base; // so that is can call destroyedNotify()

        public:
            virtual ~Observer();

            /**
             * Subscribe to changes made by @param entity.
             *
             * Changed in 2.7: being subscribed to an entity no longer prevents its
             * destruction.
             */
            template <typename T>
            void subscribeTo( KSharedPtr<T> entity ) { subscribeTo( entity.data() ); }
            template <typename T>
            void unsubscribeFrom( KSharedPtr<T> entity ) { unsubscribeFrom( entity.data() ); }

            /**
             * This method is called when the metadata of a track has changed.
             * The called class may not cache the pointer.
             */
            virtual void metadataChanged( TrackPtr track );
            virtual void metadataChanged( ArtistPtr artist );
            virtual void metadataChanged( AlbumPtr album );
            virtual void metadataChanged( GenrePtr genre );
            virtual void metadataChanged( ComposerPtr composer );
            virtual void metadataChanged( YearPtr year );

            /**
             * One of the subscribed entities was destroyed. You don't get which one
             * because it is already invalid.
             */
            virtual void entityDestroyed();

        private:
            friend class ::PersistentStatisticsStore; // so that is can call KSharedPtr-free subscribe:
            void subscribeTo( Base *ptr );
            void unsubscribeFrom( Base *ptr );

            /**
             * Called in Meta::Base destructor so that Observer doesn't have a stale pointer.
             */
            void destroyedNotify( Base *ptr );

            QSet<Base *> m_subscriptions;
            QMutex m_subscriptionsMutex; /// mutex guarding access to m_subscriptions
    };

    class AMAROK_CORE_EXPORT Base : public virtual QSharedData, public MetaCapability
    // virtual inherit. so that implementations can be both Meta::Track and Meta::Statistics
    {
        friend class Observer; // so that Observer can call (un)subscribe()

        Q_PROPERTY( QString name READ name )
        Q_PROPERTY( QString prettyName READ prettyName )
        Q_PROPERTY( QString fullPrettyName READ fullPrettyName )
        Q_PROPERTY( QString sortableName READ sortableName )

        public:
            Base() {}
            virtual ~Base();

            /** The textual label for this object.
                For a track this is the track title, for an album it is the
                album name.
                If the name is unknown or unset then this returns an empty string.
            */
            virtual QString name() const = 0;

            /** This is a nicer representation of the object.
                We will try to prevent this name from being empty.
                E.g. a track will fall back to the filename if possible.
            */
            virtual QString prettyName() const { return name(); };

            /** A exact representation of the object.
                This might include the artist name for a track or album.
            */
            virtual QString fullPrettyName() const { return prettyName(); }

            /** A name that can be used for sorting.
                This should usually mean that "The Beatles" is returned as "Beatles The"
            */
            virtual QString sortableName() const { return name(); }

            /**used for showing a fixed name in the tree view. Needed for items that change
             * their name occasionally such as some streams. These should overwrite this
             */
            virtual QString fixedName() const { return prettyName(); }

        protected:
            /**
             * Subscribe @param observer for change updates. Don't ever think of calling
             * this method yourself or overriding it, it's highly coupled with Observer.
             */
            void subscribe( Observer *observer );

            /**
             * Unsubscribe @param observer from change updates. Don't ever think of
             * calling this method yourself or overriging it, it's highly coupled with
             * Observer.
             */
            void unsubscribe( Observer *observer );

            virtual void notifyObservers() const = 0;

            template <typename T>
            void notifyObserversHelper( const T *self ) const;

        private:
            // no copy allowed, since it's not safe with observer list
            Q_DISABLE_COPY( Base )

            QSet<Meta::Observer*> m_observers;
    };

    class AMAROK_CORE_EXPORT Track : public Base
    {
        public:

            virtual ~Track() {}
            /** used to display the trackname, should never be empty, returns prettyUrl() by default if name() is empty */
            virtual QString prettyName() const;
            /** an url which can be played by the engine backends */
            virtual KUrl playableUrl() const = 0;
            /** an url for display purposes */
            virtual QString prettyUrl() const = 0;
            /** an url which is unique for this track. Use this if you need a key for the track.
                Actually this is not guaranteed to be an url at all and could be something like
                mb-f5a3456bb0 for a MusicBrainz id.
            */
            virtual QString uidUrl() const = 0;

            /** Returns whether playableUrl() will return a playable Url
                In principle this means that the url is valid.
             */
            virtual bool isPlayable() const = 0;
            /** Returns the album this track belongs to */
            virtual AlbumPtr album() const = 0;
            /** Returns the artist of this track */
            virtual ArtistPtr artist() const = 0;
            /** Returns the composer of this track */
            virtual ComposerPtr composer() const = 0;
            /** Returns the genre of this track */
            virtual GenrePtr genre() const = 0;
            /** Returns the year of this track */
            virtual YearPtr year() const = 0;
            /**
              * Returns the labels that are assigned to a track.
              */
            virtual Meta::LabelList labels() const;
            /** Returns the BPM of this track */
            virtual qreal bpm() const = 0;
            /** Returns the comment of this track */
            virtual QString comment() const = 0;
            /** Returns the length of this track in milliseconds, or 0 if unknown */
            virtual qint64 length() const = 0;
            /** Returns the filesize of this track in bytes */
            virtual int filesize() const = 0;
            /** Returns the sample rate of this track */
            virtual int sampleRate() const = 0;
            /** Returns the bitrate o this track */
            virtual int bitrate() const = 0;
            /** Returns the time when the track was added to the collection,
                or an invalid QDateTime if the time is not known. */
            virtual QDateTime createDate() const;
            /** Returns the time when the track was last modified (before being added to the collection)
                or an invalid QDateTime if the time is not known. */
            virtual QDateTime modifyDate() const;
            /** Returns the track number of this track */
            virtual int trackNumber() const = 0;
            /** Returns the discnumber of this track */
            virtual int discNumber() const = 0;
            /**
             * Returns the gain adjustment for a given replay gain mode.
             *
             * Should return @c 0 if no replay gain value is known.
             *
             * Should return the track replay gain if the album gain is requested but
             * is not available.
             */
            virtual qreal replayGain( ReplayGainTag mode ) const;

            /**
             * Returns the type of this track, e.g. "ogg", "mp3", "stream"
             *
             * TODO: change return type to Amarok::FileType enum. Clients needing
             * user-representation would call FileTypeSupport::toLocalizedString()
             */
            virtual QString type() const = 0;

            /** tell the track to perform any prerequisite
             * operations before playing */
            virtual void prepareToPlay();

            /** tell the track object that amarok finished playing it.
                The argument is the percentage of the track which was played, in the range 0 to 1*/
            virtual void finishedPlaying( double playedFraction );

            /** returns true if the track is part of a collection false otherwise */
            virtual bool inCollection() const;
            /**
                returns the collection that the track is part of, or 0 iff
                inCollection() returns false */
            virtual Collections::Collection* collection() const;

            /** get the cached lyrics for the track. returns an empty string if
                no cached lyrics are available */
            virtual QString cachedLyrics() const;
            /**
                set the cached lyrics for the track
            */
            virtual void setCachedLyrics( const QString &lyrics );

            /**
              Adds a label to the track.
              Does nothing if the track already has the given label.
             */
            virtual void addLabel( const QString &label );
            /**
              Adds a label to the track.
              Does nothing if the track already has the given label.
             */
            virtual void addLabel( const Meta::LabelPtr &label );

            /**
              Removes a lbel from a track.
              Does nothing if the track does not actually have the label assigned to it.
             */
            virtual void removeLabel( const Meta::LabelPtr &label );

            virtual bool operator==( const Track &track ) const;

            static bool lessThan( const TrackPtr& left, const TrackPtr& right );

            /**
             * Return a pointer to track's Statistics interface. May never be null.
             *
             * Subclasses: always return the default implementation instead of returning 0.
             */
            virtual StatisticsPtr statistics();
            ConstStatisticsPtr statistics() const; // allow const statistics methods on const tracks

        protected:
            friend class ::PersistentStatisticsStore; // so that it can call notifyObservers
            virtual void notifyObservers() const;

    };

    class AMAROK_CORE_EXPORT Artist : public Base
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Artist() {}

            virtual QString prettyName() const;

            /** returns all tracks by this artist */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Artist &artist ) const;

            virtual QString sortableName() const;

        protected:
            virtual void notifyObservers() const;

        private:
            mutable QString m_sortableName;
    };

    /** Represents an album.
        Most collections do not store a specific album object. Instead an album
        is just a property of a track, a container containing one or more tracks.

        Collections should proved an album for every track as the collection browser
        will, depending on the setting, only display tracks inside albums.

        For all albums in a compilation the pair album-title/album-artist should
        be unique as this pair is used as a key in several places.

        Albums without an artist are called compilations.
        Albums without a title but with an artist should contain all singles of
         the specific artist.
        There should be one album without title and artist for all the rest.
    */
    class AMAROK_CORE_EXPORT Album : public Base
    {
        Q_PROPERTY( bool compilation READ isCompilation )
        Q_PROPERTY( bool hasAlbumArtist READ hasAlbumArtist )
        Q_PROPERTY( ArtistPtr albumArtist READ albumArtist )
        Q_PROPERTY( TrackList tracks READ tracks )
        Q_PROPERTY( bool hasImage READ hasImage )
        // Q_PROPERTY( QPixmap image READ image WRITE setImage )
        Q_PROPERTY( bool supressImageAutoFetch READ suppressImageAutoFetch WRITE setSuppressImageAutoFetch )

        public:
            Album() {}
            virtual ~Album() {}

            virtual QString prettyName() const;

            /**
             * Whether this album is considered to be a compilation of tracks from various
             * artists.
             */
            virtual bool isCompilation() const = 0;
            /**
             * Whether toggling the compilation status is currenlty supported. Default
             * implementation returns false.
             */
            virtual bool canUpdateCompilation() const { return false; }
            /**
             * Set compilation status. You should check canUpdateCompilation() first.
             */
            virtual void setCompilation( bool isCompilation ) { Q_UNUSED( isCompilation ) }

            /** Returns true if this album has an album artist */
            virtual bool hasAlbumArtist() const = 0;
            /** Returns a pointer to the album's artist */
            virtual ArtistPtr albumArtist() const = 0;
            /** returns all tracks on this album */
            virtual TrackList tracks() = 0;

            /**
             * A note about image sizes:
             *  when size is <= 1, return the full size image
             */
            /** returns true if the album has a cover set */
            virtual bool hasImage( int size = 0 ) const { Q_UNUSED( size ); return false; }

            /** Returns the image for the album, usually the cover image.
                The default implementation returns an null image.
                If you need a pixmap call The::coverCache()->getCover( album, size )
                instead. That function also returns a "nocover" pixmap
            */
            virtual QImage image( int size = 0 ) const;

            /** Returns the image location on disk.
                The mpris interface is using this information for notifications so
                it better is a local file url.
            */
            virtual KUrl imageLocation( int size = 0 ) { Q_UNUSED( size ); return KUrl(); }

            /** Returns true if it is possible to update the cover of the album */
            virtual bool canUpdateImage() const { return false; }

            /** updates the cover of the album
                @param image The large scale image that should be used as cover for the album.
                Note: the parameter should not be a QPixmap as a pixmap can only be created reliable in a UI thread.
            */
            virtual void setImage( const QImage &image ) { Q_UNUSED( image ); }

            /** removes the album art */
            virtual void removeImage() { }

            /** don't automatically fetch artwork */
            virtual void setSuppressImageAutoFetch( const bool suppress ) { Q_UNUSED( suppress ); }
            /** should automatic artwork retrieval be suppressed? */
            virtual bool suppressImageAutoFetch() const { return false; }

            virtual bool operator==( const Meta::Album &album ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Composer : public Base
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Composer() {}
            virtual QString prettyName() const;
            /** returns all tracks by this composer */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Composer &composer ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Genre : public Base
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Genre() {}
            virtual QString prettyName() const;
            /** returns all tracks which belong to the genre */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Genre &genre ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Year : public Base
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Year() {}

            /** Returns the year this object represents.
                A number of 0 is considered unset.
            */
            virtual int year() const { return name().toInt(); }

            /** returns all tracks which are tagged with this year */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Year &year ) const;

        protected:
            virtual void notifyObservers() const;
    };

    /**
      A Label represents an arbitrary classification of a Track.
      */
    class AMAROK_CORE_EXPORT Label : public Base
    {
    public:
        /**
          Constructs a new Label.
          */
        Label() : Base() {}
        /**
          Destructs an existing Label.
          */
        virtual ~ Label() {}

    protected:
        virtual void notifyObservers() const;
    };
}

Q_DECLARE_METATYPE( Meta::DataPtr )
Q_DECLARE_METATYPE( Meta::DataList )
Q_DECLARE_METATYPE( Meta::TrackPtr )
Q_DECLARE_METATYPE( Meta::TrackList )
Q_DECLARE_METATYPE( Meta::ArtistPtr )
Q_DECLARE_METATYPE( Meta::ArtistList )
Q_DECLARE_METATYPE( Meta::AlbumPtr )
Q_DECLARE_METATYPE( Meta::AlbumList )
Q_DECLARE_METATYPE( Meta::ComposerPtr )
Q_DECLARE_METATYPE( Meta::ComposerList )
Q_DECLARE_METATYPE( Meta::GenrePtr )
Q_DECLARE_METATYPE( Meta::GenreList )
Q_DECLARE_METATYPE( Meta::YearPtr )
Q_DECLARE_METATYPE( Meta::YearList )
Q_DECLARE_METATYPE( Meta::LabelPtr )
Q_DECLARE_METATYPE( Meta::LabelList )



#endif /* AMAROK_META_H */
