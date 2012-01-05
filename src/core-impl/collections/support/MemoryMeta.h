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
        Artist( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }
};

class Album : public Meta::Album, public Base
{
    public:
        Album( const QString &name ) : Base( name ), m_isCompilation( false ) {}
        /**
         * Copy-like constructor for MapChanger
         */
        Album( const Meta::AlbumPtr &other );

        virtual QString name() const { return Base::name(); }

        /** Meta::Album virtual methods */
        virtual bool isCompilation() const { return m_isCompilation; }
        virtual bool hasAlbumArtist() const { return !m_albumArtist.isNull(); }
        virtual Meta::ArtistPtr albumArtist() const { return m_albumArtist; }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

        virtual bool hasImage( int /* size */ = 0 ) const { return !m_image.isNull(); }
        virtual QImage image( int size = 0 ) const;
        /* We intentionally don't advertise canUpdateImage() - setting image here would not
         * currently do what the user expects */
        virtual void setImage( const QImage &image ) { m_image = image; }

        /* MemoryMeta::Album methods: */
        void setAlbumArtist( Meta::ArtistPtr artist ) { m_albumArtist = artist; }
        void setIsCompilation( bool isCompilation ) { m_isCompilation = isCompilation; }

    private:
        bool m_isCompilation;
        Meta::ArtistPtr m_albumArtist;
        QImage m_image;
};

class Composer : public Meta::Composer, public Base
{
    public:
        Composer( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }
};

class Genre : public Meta::Genre, public Base
{
    public:
        Genre( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }
};

class Year : public Meta::Year, public Base
{
    public:
        Year( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }
};

class Track : public Meta::Track
{
    public:
        Track( const Meta::TrackPtr &originalTrack );
        virtual ~Track();

        /* Meta::MetaCapability methods */
        virtual bool hasCapabilityInterface( Capabilities::Capability::Type type ) const
            { return m_track->hasCapabilityInterface( type ); }
        virtual Capabilities::Capability *createCapabilityInterface( Capabilities::Capability::Type type )
            { return m_track->createCapabilityInterface( type ); }

        /* Meta::MetaBase virtual methods */
        virtual QString name() const { return m_track->name(); }

        /* Meta::Track virtual methods */
        virtual KUrl playableUrl() const { return m_track->playableUrl(); }
        virtual QString prettyUrl() const { return m_track->prettyUrl(); }
        virtual QString uidUrl() const { return m_track->uidUrl(); }
        virtual bool isPlayable() const { return m_track->isPlayable(); }

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
        virtual double score() const { return m_track->score(); }
        virtual void setScore( double newScore ) { m_track->setScore( newScore ); }
        virtual int rating() const { return m_track->rating(); }
        virtual void setRating( int newRating ) { m_track->setRating( newRating ); }
        virtual qint64 length() const { return m_track->length(); }
        virtual int filesize() const { return m_track->filesize(); }
        virtual int sampleRate() const { return m_track->sampleRate(); }
        virtual int bitrate() const { return m_track->bitrate(); }
        virtual QDateTime createDate() const { return m_track->createDate(); }
        virtual QDateTime modifyDate() const { return m_track->modifyDate(); }
        virtual int trackNumber() const { return m_track->trackNumber(); }
        virtual int discNumber() const { return m_track->discNumber(); }
        virtual QDateTime lastPlayed() const { return m_track->lastPlayed(); }
        virtual QDateTime firstPlayed() const { return m_track->firstPlayed(); }
        virtual int playCount() const { return m_track->playCount(); }

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
         * successfull)
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
         * @param track is not found in collection ot if in wasn't added using MapChanger.
         */
        Meta::TrackPtr removeTrack( Meta::TrackPtr track );

    private:
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

        MemoryCollection *m_mc;
};

}
#endif
