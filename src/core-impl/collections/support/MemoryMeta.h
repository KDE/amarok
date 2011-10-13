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
        Base( QString name, Meta::TrackPtr track )
        {
            m_name = name;
            m_tracks << track;
        }

        virtual ~Base() {}

        virtual QString name() const { return m_name; }
        virtual Meta::TrackList tracks() { return m_tracks; }
        virtual void addTrack( Meta::TrackPtr track ) { m_tracks << track; }

    protected:
        QString m_name;
        Meta::TrackList m_tracks;
};

class Artist : public Meta::Artist, public Base
{
    public:
        Artist( Meta::TrackPtr track ) : Base( track->artist()->name(), track ) {}

        virtual QString name() const { return Base::name(); }
        virtual Meta::TrackList tracks() { return Base::tracks(); }
    protected:
        virtual void notifyObservers() const {}
};

class Album : public Meta::Album, public Base
{
    public:
        Album( Meta::TrackPtr track ) : Base( track->album()->name(), track ) {}

        virtual QString name() const { return Base::name(); }

        /** Meta::Album virtual methods */
        virtual bool isCompilation() const { return m_isCompilation; }
        virtual bool hasAlbumArtist() const { return !m_albumArtist.isNull(); }
        virtual Meta::ArtistPtr albumArtist() const { return m_albumArtist; }
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}

    private:
        bool m_isCompilation;
        Meta::ArtistPtr m_albumArtist;
};

class Composer : public Meta::Composer, public Base
{
    public:
        Composer( Meta::TrackPtr track ) : Base( track->album()->name(), track ) {}

        virtual QString name() const { return Base::name(); }

        /** Meta::Composer virtual methods */
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}

    private:
        QString m_name;
        Meta::TrackList m_tracks;
};

class Genre : public Meta::Genre, public Base
{
    public:
        Genre( Meta::TrackPtr track ) : Base( track->genre()->name(), track ) {}

        virtual QString name() const { return Base::name(); }

        /** Meta::Genre virtual methods */
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}

    private:
        QString m_name;
        Meta::TrackList m_tracks;
};

class Year : public Meta::Year, public Base
{
    public:
        Year( Meta::TrackPtr track ) : Base( track->year()->name(), track ) {}

        virtual QString name() const { return Base::name(); }

        /** Meta::Year virtual methods */
        virtual Meta::TrackList tracks() { return Base::tracks(); }

    protected:
        virtual void notifyObservers() const {}

    private:
        QString m_name;
        Meta::TrackList m_tracks;
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
            m_mc->addTrack( track );

            ArtistMap artistMap = m_mc->artistMap();
            if( !track->artist().isNull() && !track->artist()->name().isEmpty() )
            {
                QString artistName = track->artist()->name();
                if( !artistMap.keys().contains( artistName ) )
                    artistMap.insert( artistName, Meta::ArtistPtr( new Artist( track ) ) );
                else
                    static_cast<Artist *>( artistMap[artistName].data() )->addTrack( track );
            }
            m_mc->setArtistMap( artistMap );

            AlbumMap albumMap = m_mc->albumMap();
            if( !track->album().isNull() && !track->album()->name().isEmpty() )
            {
                QString albumName = track->album()->name();
                if( !albumMap.keys().contains( albumName ) )
                    albumMap.insert( albumName, Meta::AlbumPtr( new Album( track ) ) );
                else
                    static_cast<Album *>( albumMap[albumName].data() )->addTrack( track );
            }
            m_mc->setAlbumMap( albumMap );

            GenreMap genreMap = m_mc->genreMap();
            if( !track->genre().isNull() && !track->genre()->name().isEmpty() )
            {
                QString genreName = track->genre()->name();
                if( !genreMap.keys().contains( genreName ) )
                    genreMap.insert( genreName, Meta::GenrePtr( new Genre( track ) ) );
                else
                    static_cast<Genre *>( genreMap[genreName].data() )->addTrack( track );
            }
            m_mc->setGenreMap( genreMap );

            ComposerMap composerMap = m_mc->composerMap();
            if( !track->composer().isNull() && !track->composer()->name().isEmpty() )
            {
                QString composerName = track->composer()->name();
                if( !composerMap.keys().contains( composerName ) )
                    composerMap.insert( composerName, Meta::ComposerPtr( new Composer( track ) ) );
                else
                    static_cast<Composer *>( composerMap[composerName].data() )->addTrack( track );
            }
            m_mc->setComposerMap( composerMap );

            YearMap yearMap = m_mc->yearMap();
            if( !track->year().isNull() && !track->year()->name().isEmpty() )
            {
                int year = track->year()->name().toInt();
                if( !yearMap.keys().contains( year ) )
                    yearMap.insert( year, Meta::YearPtr( new Year( track ) ) );
                else
                    static_cast<Year *>( yearMap[year].data() )->addTrack( track );
            }
            m_mc->setYearMap( yearMap );

            //TODO:labels
        }

    private:
        MemoryCollection *m_mc;
};

}
#endif
