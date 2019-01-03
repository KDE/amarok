/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef MEMORYMETA_H
#define MEMORYMETA_H

#include "amarok_export.h"
#include "MemoryCollection.h"
#include "core/meta/Meta.h"

using namespace Collections;

/** These classes can be used with a MemoryCollection to populate the meta-type maps */
namespace MemoryMeta {

class Track;

/**
 * Base class for all MemoryMeta:: entities that store a list of associated tracks:
 * Artist, Album, Composer, Genre, Year.
 *
 * All methods of this class are thread-safe.
 */
class Base
{
    public:
        explicit Base( const QString &name ) : m_name( name ) {}
        virtual ~Base() {}

        // Meta::{Artist,Album,Composer,Genre,Year} methods:
        virtual QString name() const { return m_name; }
        virtual Meta::TrackList tracks();

        // MemoryMeta::Base methods:
        void addTrack( Track *track );
        void removeTrack( Track *track );

    private:
        QString m_name;
        /* We cannot easily store AmarokSharedPointer to tracks, because it creates reference
         * counting cycle: MemoryMeta::Track::m_album -> MemoryMeta::Album::tracks() ->
         * MemoryMeta::Track. We therefore store plain pointers and rely on
         * MemoryMeta::Track to notify when it is destroyed. */
        QList<Track *> m_tracks;
        QReadWriteLock m_tracksLock;
};

class Artist : public Meta::Artist, public Base
{
    public:
        explicit Artist( const QString &name ) : MemoryMeta::Base( name ) {}

        QString name() const override { return MemoryMeta::Base::name(); }
        Meta::TrackList tracks() override { return MemoryMeta::Base::tracks(); }
};

class Album : public Meta::Album, public Base
{
    public:
        Album( const QString &name, const Meta::ArtistPtr &albumArtist )
            : MemoryMeta::Base( name )
            , m_albumArtist( albumArtist )
            , m_isCompilation( false )
            , m_canUpdateCompilation( false )
            , m_canUpdateImage( false )
            {}
        /**
         * Copy-like constructor for MapChanger
         */
        explicit Album( const Meta::AlbumPtr &other );

        /* Meta::MetaCapability virtual methods */
        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        /* Meta::Base virtual methods */
        QString name() const override { return MemoryMeta::Base::name(); }

        /* Meta::Album virtual methods */
        bool isCompilation() const override { return m_isCompilation; }
        bool canUpdateCompilation() const override { return m_canUpdateCompilation; }
        void setCompilation( bool isCompilation ) override;

        bool hasAlbumArtist() const override { return !m_albumArtist.isNull(); }
        Meta::ArtistPtr albumArtist() const override { return m_albumArtist; }
        Meta::TrackList tracks() override { return MemoryMeta::Base::tracks(); }

        bool hasImage( int /* size */ = 0 ) const override { return !m_image.isNull(); }
        QImage image( int size = 0 ) const override;
        bool canUpdateImage() const override { return m_canUpdateImage; }
        void setImage( const QImage &image ) override;
        void removeImage() override;

        /* MemoryMeta::Album methods: */
        /**
         * Re-read isCompilation, canUpdateCompilation, image, canUpdateImage from all
         * underlying tracks.
         */
        void updateCachedValues();

    private:
        Meta::ArtistPtr m_albumArtist;
        bool m_isCompilation;
        bool m_canUpdateCompilation;
        QImage m_image;
        bool m_canUpdateImage;
};

class Composer : public Meta::Composer, public Base
{
    public:
        explicit Composer( const QString &name ) : MemoryMeta::Base( name ) {}

        QString name() const override { return MemoryMeta::Base::name(); }
        Meta::TrackList tracks() override { return  MemoryMeta::Base::tracks(); }
};

class Genre : public Meta::Genre, public Base
{
    public:
        explicit Genre( const QString &name ) : MemoryMeta::Base( name ) {}

        QString name() const override { return MemoryMeta::Base::name(); }
        Meta::TrackList tracks() override { return MemoryMeta::Base::tracks(); }
};

class Year : public Meta::Year, public Base
{
    public:
        explicit Year( const QString &name ) : MemoryMeta::Base( name ) {}

        QString name() const override { return MemoryMeta::Base::name(); }
        Meta::TrackList tracks() override { return MemoryMeta::Base::tracks(); }
};

class AMAROK_EXPORT Track : public Meta::Track
{
    public:
        explicit Track( const Meta::TrackPtr &originalTrack );
        virtual ~Track();

        /* Meta::MetaCapability methods */
        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override
            { return m_track->hasCapabilityInterface( type ); }
        Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type ) override
            { return m_track->createCapabilityInterface( type ); }

        /* Meta::Base virtual methods */
        QString name() const override { return m_track->name(); }

        /* Meta::Track virtual methods */
        QUrl playableUrl() const override { return m_track->playableUrl(); }
        QString prettyUrl() const override { return m_track->prettyUrl(); }
        QString uidUrl() const override { return m_track->uidUrl(); }
        QString notPlayableReason() const override { return m_track->notPlayableReason(); }

        //these functions return the proxy track values
        Meta::AlbumPtr album() const override { return m_album; }
        Meta::ArtistPtr artist() const override { return m_artist; }
        Meta::ComposerPtr composer() const override { return m_composer; }
        Meta::GenrePtr genre() const override { return m_genre; }
        Meta::YearPtr year() const override { return m_year; }

        //TODO:implement labels
        Meta::LabelList labels() const override { return Meta::LabelList(); }
        qreal bpm() const override { return m_track->bpm(); }
        QString comment() const override { return m_track->comment(); }
        qint64 length() const override { return m_track->length(); }
        int filesize() const override { return m_track->filesize(); }
        int sampleRate() const override { return m_track->sampleRate(); }
        int bitrate() const override { return m_track->bitrate(); }
        QDateTime createDate() const override { return m_track->createDate(); }
        QDateTime modifyDate() const override { return m_track->modifyDate(); }
        int trackNumber() const override { return m_track->trackNumber(); }
        int discNumber() const override { return m_track->discNumber(); }

        qreal replayGain( Meta::ReplayGainTag mode ) const override
                { return m_track->replayGain( mode ); }
        QString type() const override { return m_track->type(); }

        void prepareToPlay() override { m_track->prepareToPlay(); }
        void finishedPlaying( double fraction ) override { m_track->finishedPlaying( fraction ); }

        bool inCollection() const override { return m_track->inCollection(); }
        Collections::Collection *collection() const override { return m_track->collection(); }

        QString cachedLyrics() const override { return m_track->cachedLyrics(); }
        void setCachedLyrics( const QString &lyrics ) override { m_track->setCachedLyrics( lyrics ); }

        //TODO: implement labels
        void addLabel( const QString &label ) override { Q_UNUSED( label ) }
        void addLabel( const Meta::LabelPtr &label ) override { Q_UNUSED( label ) }
        void removeLabel( const Meta::LabelPtr &label ) override { Q_UNUSED( label ) }

        Meta::TrackEditorPtr editor() override;
        Meta::StatisticsPtr statistics() override;

        // MemoryMeta::Track methods:

        /* All of these set* methods pass the pointer to AmarokSharedPointer (thus memory-manage it),
         * remove this track from previous {Album,Artist,Composer,Genre,Year} entity (if any)
         * and add this track to newly set entity. (if non-null)
         * All these methods are reentrant, but not thread-safe: caller must ensure that
         * only one of the following methods is called at a time on a single instance.
         */
        void setAlbum( Album *album );
        void setArtist( Artist *artist );
        void setComposer( Composer *composer );
        void setGenre( Genre *genre );
        void setYear( Year *year );

        /**
         * Return the original track this track proxies.
         */
        Meta::TrackPtr originalTrack() const { return m_track; }

        /**
         * Make notifyObservers() public so that MapChanger can call this
         */
        using Meta::Track::notifyObservers;

    private:
        Meta::TrackPtr m_track;
        Meta::AlbumPtr m_album;
        Meta::ArtistPtr m_artist;
        Meta::ComposerPtr m_composer;
        Meta::GenrePtr m_genre;
        Meta::YearPtr m_year;
};

/**
 * Helper class that facilitates adding, removing and changing tracks that are in
 * MemoryCollection. This class locks underlying MemoryCollection upon construction for
 * writing and releases the lock in destructor.
 *
 * Typical usage:
 * {
 *     MemoryMeta::MapChanger changer( memoryCollectionPtr );
 *     Meta::Track newTrack = changer.addTrack( trackPtr );
 *     ...
 *     changer.removeTrack( newTrack );
 * }
 *
 * All methods in this class are re-entrant and it operates on MemoryCollection in
 * a thread-safe way: you can run MapChangers from multiple threads on a single
 * MemoryCollection at once. (each thread constructing MapChanger when needed and deleting
 * it as soon as possible)
 *
 * All methods can be called multiple times on a single instance and can be combined.
 */
class AMAROK_EXPORT MapChanger
{
    public:
        explicit MapChanger( MemoryCollection *memoryCollection );
        ~MapChanger();

        /**
         * Adds a track to MemoryCollection by proxying it using @see MemoryMeta::Track
         * track artist, album, genre, composer and year are replaced in MemoryMeta::Track
         * by relevant MemoryMeta entities, based on their value. Refuses to add a track
         * whose proxy is already in MemoryCollection (returns null pointer in this case)
         *
         * @return pointer to a newly created MemoryMeta::Track (may be null if not
         * successful)
         */
        Meta::TrackPtr addTrack( Meta::TrackPtr track );

        /**
         * Removes a track from MemoryCollection. Pays attention to remove artists,
         * albums, genres, composers and years that may become dangling in
         * MemoryCollection.
         *
         * @param track MemoryMeta track to remove, it doesn't matter if this is the track
         * returned by MapChanger::addTrack or the underlying one passed to
         * MapChanger::addTrack - the real track is looked up using its uidUrl in
         * MemoryCollection.
         *
         * @return shared pointer to underlying track of the deleted track, i.e. the track
         * that you passed to MapChanger::addTrack() originally. May be null pointer if
         * @param track is not found in collection or if it wasn't added using MapChanger.
         */
        Meta::TrackPtr removeTrack( Meta::TrackPtr track );

        /**
         * Reflects changes made to underlying track to its proxy track in
         * MemoryCollection and to MemoryCollection maps.
         *
         * The one who called MapChanger::addTrack() is responsible to call this method
         * every time it detects that some metadata of underlying track have changed
         * (perhaps by becoming its observer), even in minor fields such as comment. This
         * method instructs proxy track to call notifyObservers().
         *
         * Please note that this method is currently unable to cope with changes
         * to track uidUrl(). If you really need it, change MemoryCollection's
         * trackMap manually _before_ calling this.
         *
         * @param track track whose metadata have changed, it doesn't matter if this is
         * the track returned by MapChanger::addTrack or the underlying one passed to
         * MapChanger::addTrack - the real track is looked up using its uidUrl in
         * MemoryCollection. Does nothing if @param track is not found in MemoryCollection.
         *
         * @return true if memory collection maps had to be changed, false for minor
         * changes where this is not required
         */
        bool trackChanged( Meta::TrackPtr track );

    private:
        /**
         * Worker for addTrack.
         *
         * @param track original underlying track - source of metadata
         * @param memoryTrack new track to add to MemoryCollection - target of metadata
         */
        Meta::TrackPtr addExistingTrack( Meta::TrackPtr track, Track *memoryTrack );

        /**
         * Return true if at least one of the tracks in @param needles is in
         * @param haystack, false otherwise. Comparison is done using track uidUrl.
         */
        static bool hasTrackInMap( const Meta::TrackList &needles, const TrackMap &haystack );

        /**
         * Return true if artist @param artist is referenced as albumArtist of one of the
         * albums from @param haystack. The match is done using Meta:ArtistPtr
         * operator==.
         */
        static bool referencedAsAlbumArtist( const Meta::ArtistPtr &artist, const AlbumMap &haystack );

        /**
         * Return true if @param first has different value than @param other. Specifically
         * returns true if one entity is null and the other non-null, but returns true if
         * both are null.
         */
        static bool entitiesDiffer( const Meta::Base *first, const Meta::Base *second );
        /**
         * Overload for albums, we compare album artist, isCollection and image too
         */
        static bool entitiesDiffer( const Meta::Album *first, const Meta::Album *second );

        MemoryCollection *m_mc;
};

}
#endif
