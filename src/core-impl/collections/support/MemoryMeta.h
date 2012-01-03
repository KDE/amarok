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

#include "MemoryCollection.h"

#include "core/meta/Meta.h"

using namespace Collections;

/** These classes can be used with a MemoryCollection to populate the meta-type maps */
namespace MemoryMeta {

class Base
{
    public:
        Base( const QString &name ) : m_name( name ) {}
        virtual ~Base() {}

        // Meta::{Artist,Album,Composer,Genre,Year} methods:
        virtual QString name() const { return m_name; }
        virtual Meta::TrackList tracks() { return m_tracks; }

        // MemoryMeta::Base methods:
        void addTrack( Meta::TrackPtr track ) { m_tracks << track; }

    protected:
        QString m_name;
        Meta::TrackList m_tracks;
};

class Artist : public Meta::Artist, public Base
{
    public:
        Artist( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}
};

class Album : public Meta::Album, public Base
{
    public:
        Album( const QString &name ) : Base( name ), m_isCompilation( false ) {}

        virtual QString name() const { return Base::name(); }

        /** Meta::Album virtual methods */
        virtual bool isCompilation() const { return m_isCompilation; }
        virtual bool hasAlbumArtist() const { return !m_albumArtist.isNull(); }
        virtual Meta::ArtistPtr albumArtist() const { return m_albumArtist; }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

        virtual bool hasImage( int /* size */ = 0 ) const { return !m_image.isNull(); }
        virtual QImage image( int size = 0 ) const
        {
            if( size > 1 && size <= 1000 && !m_image.isNull() )
                return m_image.scaled( size, size, Qt::KeepAspectRatio, Qt::FastTransformation );
            return m_image;
        }
        /* We intentionally don't advertise canUpdateImage() - setting image here would not
         * currently do what the user expects */
        virtual void setImage( const QImage &image ) { m_image = image; }

        /* MemoryMeta::Album methods: */
        void setAlbumArtist( Meta::ArtistPtr artist ) { m_albumArtist = artist; }

    protected:
        virtual void notifyObservers() const {}

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

    protected:
        virtual void notifyObservers() const {}
};

class Genre : public Meta::Genre, public Base
{
    public:
        Genre( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}
};

class Year : public Meta::Year, public Base
{
    public:
        Year( const QString &name ) : Base( name ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}
};

class Track : public Meta::Track
{
    public:
        Track( const Meta::TrackPtr &originalTrack )
            : m_track( originalTrack )
            , m_album( 0 )
            , m_artist( 0 )
            , m_composer( 0 )
            , m_genre( 0 )
            , m_year( 0 )
        {}

        /* Meta::Track virtual methods */
        virtual QString name() const { return m_track->name(); }

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

        /* MemoryMeta::Track methods */
        void setAlbum( Meta::AlbumPtr album ) { m_album = album; }
        void setArtist( Meta::ArtistPtr artist ) { m_artist = artist; }
        void setComposer( Meta::ComposerPtr composer ) { m_composer = composer; }
        void setGenre( Meta::GenrePtr genre ) { m_genre = genre; }
        void setYear( Meta::YearPtr year ) { m_year = year; }

    private:
        Meta::TrackPtr m_track;
        Meta::AlbumPtr m_album;
        Meta::ArtistPtr m_artist;
        Meta::ComposerPtr m_composer;
        Meta::GenrePtr m_genre;
        Meta::YearPtr m_year;
};

class MapAdder
{
    public:
        MapAdder( MemoryCollection *memoryCollection )
            : m_mc( memoryCollection )
        {
            m_mc->acquireWriteLock();
        }

        ~MapAdder() { m_mc->releaseLock(); }

        void addTrack( Meta::TrackPtr track )
        {
            Track *memoryTrack = new Track( track );
            Meta::TrackPtr metaTrackPtr =
                    Meta::TrackPtr( static_cast<Meta::Track *>( memoryTrack ) );

            ArtistMap artistMap = m_mc->artistMap();

            QString artistName = track->artist().isNull() ? QString() : track->artist()->name();
            Meta::ArtistPtr artist = artistMap.value( artistName );
            if( artist.isNull() )
            {
                artist = Meta::ArtistPtr( new Artist( artistName ) );
                artistMap.insert( artistName, artist );
            }

            static_cast<Artist *>( artist.data() )->addTrack( metaTrackPtr );
            memoryTrack->setArtist( artist );

            m_mc->setArtistMap( artistMap );

            AlbumMap albumMap = m_mc->albumMap();
            QString albumName = track->album().isNull() ? QString() : track->album()->name();
            Meta::AlbumPtr album = albumMap.value( albumName );
            if( album.isNull() )
            {
                album = Meta::AlbumPtr( new Album( albumName ) );
                albumMap.insert( albumName, album );
            }

            Album *memoryAlbum = static_cast<Album *>( album.data() );
            memoryAlbum->addTrack( metaTrackPtr );
            memoryAlbum->setAlbumArtist( metaTrackPtr->artist() );
            QImage albumImage = track->album().isNull() ? QImage() : track->album()->image();
            if( !albumImage.isNull() )
            {
                /* We overwrite album image only if it is bigger than the old one */
                int memoryImageArea = album->image().width() * album->image().height();
                int albumImageArea = albumImage.width() * albumImage.height();
                if( albumImageArea > memoryImageArea )
                    album->setImage( albumImage );
            }
            memoryTrack->setAlbum( album );

            m_mc->setAlbumMap( albumMap );

            GenreMap genreMap = m_mc->genreMap();

            QString genreName = track->genre().isNull() ? QString() : track->genre()->name();
            Meta::GenrePtr genre = genreMap.value( genreName );
            if( genre.isNull() )
            {
                genre = Meta::GenrePtr( new Genre( genreName ) );
                genreMap.insert( genreName, genre );
            }
            static_cast<Genre *>( genre.data() )->addTrack( metaTrackPtr );
            memoryTrack->setGenre( genre );

            m_mc->setGenreMap( genreMap );

            ComposerMap composerMap = m_mc->composerMap();

            QString composerName = track->composer().isNull() ? QString() : track->composer()->name();
            Meta::ComposerPtr composer = composerMap.value( composerName );
            if( composer.isNull() )
            {
                composer = Meta::ComposerPtr( new Composer( composerName ) );
                composerMap.insert( composerName, composer );
            }

            static_cast<Composer *>( composer.data() )->addTrack( metaTrackPtr );
            memoryTrack->setComposer( composer );

            m_mc->setComposerMap( composerMap );

            YearMap yearMap = m_mc->yearMap();

            int year = track->year().isNull() ? 0 : track->year()->year();
            Meta::YearPtr yearPtr = yearMap.value( year );
            if( yearPtr.isNull() )
            {
                yearPtr = Meta::YearPtr( new Year( year ? QString::number( year ) : QString() ) );
                yearMap.insert( year, yearPtr );
            }
            static_cast<Year *>( yearPtr.data() )->addTrack( metaTrackPtr );
            memoryTrack->setYear( yearPtr );

            m_mc->setYearMap( yearMap );

            m_mc->addTrack( metaTrackPtr );
            //TODO:labels
        }

    private:
        MemoryCollection *m_mc;
};

}
#endif
