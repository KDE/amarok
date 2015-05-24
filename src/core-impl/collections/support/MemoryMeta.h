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
        Base( const QString &name ) : m_name( name ) {}
        virtual ~Base() {}

        // Meta::{Artist,Album,Composer,Genre,Year} methods:
        virtual QString name() const { return m_name; }
        virtual Meta::TrackList tracks();

        // MemoryMeta::Base methods:
        void addTrack( Track *track );
        void removeTrack( Track *track );

    private:
        QString m_name;
        /* We cannot easily store KSharedPtr to tracks, because it creates reference
         * counting cycle: MemoryMeta::Track::m_album -> MemoryMeta::Album::tracks() ->
         * MemoryMeta::Track. We therefore store plain pointers and rely on
         * MemoryMeta::Track to notify when it is destroyed. */
        QList<Track *> m_tracks;
        QReadWriteLock m_tracksLock;
};

class Artist : public Meta::Artist, public Base
{
    public:
        Artist( const QString &name ) : MemoryMeta::Base( name ) {}

        virtual QString name() const { return MemoryMeta::Base::name(); }
        virtual Meta::TrackList tracks() { return MemoryMeta::Base::tracks(); }
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
        Album( const Meta::AlbumPtr &other );

        /* Meta::MetaCapability virtual methods */
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const;
        virtual Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type );

        /* Meta::Base virtual methods */
        virtual QString name() const { return MemoryMeta::Base::name(); }

        /* Meta::Album virtual methods */
        virtual bool isCompilation() const { return m_isCompilation; }
        virtual bool canUpdateCompilation() const { return m_canUpdateCompilation; }
        virtual void setCompilation( bool isCompilation );

        virtual bool hasAlbumArtist() const { return !m_albumArtist.isNull(); }
        virtual Meta::ArtistPtr albumArtist() const { return m_albumArtist; }
        virtual Meta::TrackList tracks() { return MemoryMeta::Base::tracks(); }

        virtual bool hasImage( int /* size */ = 0 ) const { return !m_image.isNull(); }
        virtual QImage image( int size = 0 ) const;
        virtual bool canUpdateImage() const { return m_canUpdateImage; }
        virtual void setImage( const QImage &image );
        virtual void removeImage();

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
        Composer( const QString &name ) : MemoryMeta::Base( name ) {}

        virtual QString name() const { return MemoryMeta::Base::name(); }
        virtual Meta::TrackList tracks() { return  MemoryMeta::Base::tracks(); }
};

class Genre : public Meta::Genre, public Base
{
    public:
        Genre( const QString &name ) : MemoryMeta::Base( name ) {}

        virtual QString name() const { return MemoryMeta::Base::name(); }
        virtual Meta::TrackList tracks() { return MemoryMeta::Base::tracks(); }
};

class Year : public Meta::Year, public Base
{
    public:
        Year( const QString &name ) : MemoryMeta::Base( name ) {}

        virtual QString name() const { return MemoryMeta::Base::name(); }
        virtual Meta::TrackList tracks() { return MemoryMeta::Base::tracks(); }
};

class AMAROK_EXPORT Track : public Meta::Track
{
    public:
        Track( const Meta::TrackPtr &originalTrack );
        virtual ~Track();

        /* Meta::MetaCapability methods */
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
            { return m_track->hasCapabilityInterface( type ); }
        virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type )
            { return m_track->createCapabilityInterface( type ); }

        /* Meta::Base virtual methods */
        virtual QString name() const { return m_track->name(); }

        /* Meta::Track virtual methods */
        virtual QUrl playableUrl() const { return m_track->playableUrl(); }
        virtual QString prettyUrl() const { return m_track->prettyUrl(); }
        virtual QString uidUrl() const { return m_track->uidUrl(); }
        virtual QString notPlayableReason() const { return m_track->notPlayableReason(); }

        //these functions return the proxy track values
        virtual Meta::AlbumPtr album() const { return m_album; }
        virtual Meta::ArtistPtr artist() const { return m_artist; }
        virtual Meta::ComposerPtr composer() const { return m_composer; }
        virtual Meta::GenrePtr genre() const { return m_genre; }
        virtual Meta::YearPtr year() const { return m_year; }

        //TODO:implement labels
        virtual Meta::LabelList labels() const { return Meta::LabelList(); }
        virtual qreal bpm() const { return m_track->bpm(); }
        virtual QString comment() const { return m_track->comment(); }
        virtual qint64 length() const { return m_track->length(); }
        virtual int filesize() const { return m_track->filesize(); }
        virtual int sampleRate() const { return m_track->sampleRate(); }
        virtual int bitrate() const { return m_track->bitrate(); }
        virtual QDateTime createDate() const { return m_track->createDate(); }
        virtual QDateTime modifyDate() const { return m_track->modifyDate(); }
        virtual int trackNumber() const { return m_track->trackNumber(); }
        virtual int discNumber() const { return m_track->discNumber(); }

        virtual qreal replayGain( Meta::ReplayGainTag mode ) const
                { return m_track->replayGain( mode ); }
        virtual QString type() const { return m_track->type(); }

        virtual void prepareToPlay() { m_track->prepareToPlay(); }
        virtual void finishedPlaying( double fraction ) { m_track->finishedPlaying( fraction ); }

        virtual bool inCollection() const { return m_track->inCollection(); }
        virtual Collections::Collection *collection() const { return m_track->collection(); }

        virtual QString cachedLyrics() const { return m_track->cachedLyrics(); }
        virtual void setCachedLyrics( const QString &lyrics ) { m_track->setCachedLyrics( lyrics ); }

        //TODO: implement labels
        virtual void addLabel( const QString &label ) { Q_UNUSED( label ) }
        virtual void addLabel( const Meta::LabelPtr &label ) { Q_UNUSED( label ) }
        virtual void removeLabel( const Meta::LabelPtr &label ) { Q_UNUSED( label ) }

        virtual Meta::TrackEditorPtr editor();
        virtual Meta::StatisticsPtr statistics();

        // MemoryMeta::Track methods:

        /* All of these set* methods pass the pointer to KSharedPtr (thus memory-manage it),
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
        MapChanger( MemoryCollection *memoryCollection );
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
