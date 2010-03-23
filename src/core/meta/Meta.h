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

#include "core/capabilities/Capability.h"

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QPixmap>
#include <QSet>
#include <QSharedData>
#include <QString>

#include <ksharedptr.h>
#include <kurl.h>

class QueryMaker;

namespace Amarok
{
    class Collection;
}

namespace Meta
{
    class MetaBase;
    class Track;
    class Artist;
    class Album;
    class Genre;
    class Composer;
    class Year;

    typedef KSharedPtr<MetaBase> DataPtr;
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

    class AMAROK_EXPORT Observer
    {
        public:
            void subscribeTo( TrackPtr );
            void unsubscribeFrom( TrackPtr );
            void subscribeTo( ArtistPtr );
            void unsubscribeFrom( ArtistPtr );
            void subscribeTo( AlbumPtr );
            void unsubscribeFrom( AlbumPtr );
            void subscribeTo( ComposerPtr );
            void unsubscribeFrom( ComposerPtr );
            void subscribeTo( GenrePtr );
            void unsubscribeFrom( GenrePtr );
            void subscribeTo( YearPtr );
            void unsubscribeFrom( YearPtr );

            /** This method is called when the metadata of a track has changed.
                The called class may not cache the pointer */
            virtual void metadataChanged( TrackPtr track );
            virtual void metadataChanged( ArtistPtr artist );
            virtual void metadataChanged( AlbumPtr album );
            virtual void metadataChanged( GenrePtr genre );
            virtual void metadataChanged( ComposerPtr composer );
            virtual void metadataChanged( YearPtr year );
            virtual ~Observer();

        private:
            QSet<TrackPtr> m_trackSubscriptions;
            QSet<ArtistPtr> m_artistSubscriptions;
            QSet<AlbumPtr> m_albumSubscriptions;
            QSet<ComposerPtr> m_composerSubscriptions;
            QSet<GenrePtr> m_genreSubscriptions;
            QSet<YearPtr> m_yearSubscriptions;
    };

    class AMAROK_EXPORT MetaCapability
    {
    public:
        virtual ~MetaCapability() {}

        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;

        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        /**
             * Retrieves a specialized interface which represents a capability of this
             * MetaBase object.
             *
             * @returns a pointer to the capability interface if it exists, 0 otherwise
             */
        template <class CapIface> CapIface *create()
        {
            Capabilities::Capability::Type type = CapIface::capabilityInterfaceType();
            Capabilities::Capability *iface = createCapabilityInterface( type );
            return qobject_cast<CapIface *>( iface );
        }

        /**
             * Tests if a MetaBase object provides a given capability interface.
             *
             * @returns true if the interface is available, false otherwise
             */
        template <class CapIface> bool is() const
        {
            return hasCapabilityInterface( CapIface::capabilityInterfaceType() );
        }
    };

    class AMAROK_EXPORT MetaBase : public QSharedData, public MetaCapability
    {
        friend class Observer;

        Q_PROPERTY( QString name READ name )
        Q_PROPERTY( QString prettyName READ prettyName )
        Q_PROPERTY( QString fullPrettyName READ fullPrettyName )
        Q_PROPERTY( QString sortableName READ sortableName )

        public:
            MetaBase() {}
            virtual ~MetaBase() {}
            virtual QString name() const = 0;

            virtual QString prettyName() const = 0;
            virtual QString fullPrettyName() const { return prettyName(); }
            virtual QString sortableName() const { return prettyName(); }

            /**used for showing a fixed name in the tree view. Needed for items that change
             * their name occasionally such as some streams. These should overwrite this
             */
            virtual QString fixedName() const { return prettyName(); }

            virtual void addMatchTo( QueryMaker *qm ) = 0;



        protected:
            virtual void subscribe( Observer *observer );
            virtual void unsubscribe( Observer *observer );

            virtual void notifyObservers() const = 0;

            QSet<Meta::Observer*> m_observers;

        private: // no copy allowed, since it's not safe with observer list
            Q_DISABLE_COPY(MetaBase)
    };

    class AMAROK_EXPORT Track : public MetaBase
    {
        public:

            /**
             * The Replay Gain mode.
             */
            enum ReplayGainMode
            {
                /** All tracks should be equally loud.  Also known as Radio mode. */
                TrackReplayGain,
                /** All albums should be equally loud.  Also known as Audiophile mode. */
                AlbumReplayGain
            };

            virtual ~Track() {}
            /** used to display the trackname, should never be empty, returns prettyUrl() by default if name() is empty */
            virtual QString prettyName() const = 0;
            /** an url which can be played by the engine backends */
            virtual KUrl playableUrl() const = 0;
            /** an url for display purposes */
            virtual QString prettyUrl() const = 0;
            /** an url which is unique for this track. Use this if you need a key for the track */
            virtual QString uidUrl() const = 0;

            /** Returns whether playableUrl() will return a playable Url */
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
            /** Returns the BPM of this track */
            virtual qreal bpm() const = 0;
            /** Returns the comment of this track */
            virtual QString comment() const = 0;
            /** Returns the score of this track */
            virtual double score() const = 0;
            virtual void setScore( double newScore ) = 0;
            /** Returns the rating of this track */
            virtual int rating() const = 0;
            virtual void setRating( int newRating ) = 0;
            /** Returns the length of this track in milliseconds, or 0 if unknown */
            virtual qint64 length() const = 0;
            /** Returns the filesize of this track in bytes */
            virtual int filesize() const = 0;
            /** Returns the sample rate of this track */
            virtual int sampleRate() const = 0;
            /** Returns the bitrate o this track */
            virtual int bitrate() const = 0;
            /** Returns the time when the track was added to the collection,
                or an invalid QDateTime instance if the time is not known */
            virtual QDateTime createDate() const;
            /** Returns the track number of this track */
            virtual int trackNumber() const = 0;
            /** Returns the discnumber of this track */
            virtual int discNumber() const = 0;
            /** Returns the time the song was last played, or 0 if it has not been played yet */
            virtual uint lastPlayed() const = 0;
            /** Returns the time the song was first played, or 0 if it has not been played yet */
            virtual uint firstPlayed() const;
            /** Returns the number of times the track was played (what about unknown?)*/
            virtual int playCount() const = 0;
            /**
             * Returns the gain adjustment for a given replay gain mode.
             *
             * Should return @c 0 if no replay gain value is known.
             *
             * Should return the track replay gain if the album gain is requested but
             * is not available.
             */
            virtual qreal replayGain( ReplayGainMode mode ) const;
            /** Returns the peak (after gain adjustment) for a given replay gain mode */
            virtual qreal replayPeakGain( ReplayGainMode mode ) const;

            /** Returns the type of this track, e.g. "ogg", "mp3", "stream" */
            virtual QString type() const = 0;

            /** tell the track to perform any prerequisite
             * operations before playing */
            virtual void prepareToPlay();

            /** tell the track object that amarok finished playing it.
                The argument is the percentage of the track which was played, in the range 0 to 1*/
            virtual void finishedPlaying( double playedFraction );

            virtual void addMatchTo( QueryMaker* qm );

            /** returns true if the track is part of a collection false otherwise */
            virtual bool inCollection() const;
            /**
                returns the collection that the track is part of, or 0 iff
                inCollection() returns false */
            virtual Amarok::Collection* collection() const;

            /** get the cached lyrics for the track. returns an empty string if
                no cached lyrics are available */
            virtual QString cachedLyrics() const;
            /**
                set the cached lyrics for the track
            */
            virtual void setCachedLyrics( const QString &lyrics );

            virtual bool operator==( const Track &track ) const;

            static bool lessThan( const TrackPtr& left, const TrackPtr& right );

        protected:
            virtual void notifyObservers() const;

    };

    class AMAROK_EXPORT Artist : public MetaBase
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        Q_PROPERTY( AlbumList albums READ albums )
        public:

            virtual ~Artist() {}
            /** returns all tracks by this artist */
            virtual TrackList tracks() = 0;

            /** returns all albums by this artist */
            virtual AlbumList albums() = 0;

            virtual void addMatchTo( QueryMaker* qm );

            virtual bool operator==( const Meta::Artist &artist ) const;

            virtual QString sortableName() const;

        protected:
            virtual void notifyObservers() const;

        private:
            mutable QString m_sortableName;
    };

    class AMAROK_EXPORT Album : public MetaBase
    {
        Q_PROPERTY( bool compilation READ isCompilation )
        Q_PROPERTY( bool hasAlbumArtist READ hasAlbumArtist )
        Q_PROPERTY( ArtistPtr albumArtist READ albumArtist )
        Q_PROPERTY( TrackList tracks READ tracks )
        Q_PROPERTY( bool hasImage READ hasImage )
        Q_PROPERTY( QPixmap image READ image WRITE setImage )
        Q_PROPERTY( bool supressImageAutoFetch READ suppressImageAutoFetch WRITE setSuppressImageAutoFetch )

        public:
            Album() {}
            virtual ~Album() {}
            virtual bool isCompilation() const = 0;

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
            virtual bool hasImage( int size = 1 ) const { Q_UNUSED( size ); return false; }
            /** returns the "nocover" image; proper cover image getter is
             * implemented in subclasses  */
            virtual QPixmap image( int size = 1 );
            /** returns the image location on disk */
            virtual KUrl imageLocation( int size = 1 ) { Q_UNUSED( size ); return KUrl(); }
            /** returns the cover of the album with a nice border around it*/
            virtual QPixmap imageWithBorder( int size = 1, int borderWidth = 5 );
            /** Returns true if it is possible to update the cover of the album */
            virtual bool canUpdateImage() const { return false; }
            /** updates the cover of the album */
            virtual void setImage( const QPixmap &pixmap ) { Q_UNUSED( pixmap ); } //TODO: choose parameter
            /** removes the album art */
            virtual void removeImage() { }
            /** don't automatically fetch artwork */
            virtual void setSuppressImageAutoFetch( const bool suppress ) { Q_UNUSED( suppress ); }
            /** should automatic artwork retrieval be suppressed? */
            virtual bool suppressImageAutoFetch() const { return false; }

            virtual void addMatchTo( QueryMaker* qm );

            virtual bool operator==( const Meta::Album &album ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_EXPORT Composer : public MetaBase
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Composer() {}
            /** returns all tracks by this composer */
            virtual TrackList tracks() = 0;

            virtual void addMatchTo( QueryMaker* qm );

            virtual bool operator==( const Meta::Composer &composer ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_EXPORT Genre : public MetaBase
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Genre() {}
            /** returns all tracks which belong to the genre */
            virtual TrackList tracks() = 0;

            virtual void addMatchTo( QueryMaker* qm );

            virtual bool operator==( const Meta::Genre &genre ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_EXPORT Year : public MetaBase
    {
        Q_PROPERTY( TrackList tracks READ tracks )
        public:

            virtual ~Year() {}
            /** returns all tracks which are tagged with this year */
            virtual TrackList tracks() = 0;

            virtual void addMatchTo( QueryMaker* qm );

            virtual bool operator==( const Meta::Year &year ) const;

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



#endif /* AMAROK_META_H */
