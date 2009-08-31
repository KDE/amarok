/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SERVICECOLLECTION_H
#define SERVICECOLLECTION_H

#include "amarok_export.h"
#include "Collection.h"
#include "support/MemoryCollection.h"
#include "ServiceBase.h"

#include <KIcon>

#include <QtGlobal>

typedef QMap<int, Meta::TrackPtr> TrackIdMap;
typedef QMap<int, Meta::ArtistPtr> ArtistIdMap;
typedef QMap<int, Meta::AlbumPtr> AlbumIdMap;
typedef QMap<int, Meta::GenrePtr> GenreIdMap;

/**
 *  This is a specialized collection that can be used by services who dynamically
 *  fetch their data from somewhere ( a web service, an external program, etc....)
 */

class AMAROK_EXPORT ServiceCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        ServiceCollection( ServiceBase * service = 0 );
        ServiceCollection( ServiceBase * service, const QString &id, const QString &prettyName );
        virtual ~ServiceCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("action-view-services-scripted-amarok"); }

        virtual CollectionLocation* location() const;

        void emitUpdated();

        virtual QStringList query( const QString &query ) { Q_UNUSED( query ); return QStringList(); }
        virtual int insert( const QString &statement, const QString &table ) { Q_UNUSED( statement ); Q_UNUSED( table ); return 0; }

        virtual QString escape( QString text ) const { Q_UNUSED( text ); return QString(); }


        Meta::TrackPtr trackById( int id );
        Meta::AlbumPtr albumById( int id );
        Meta::ArtistPtr artistById( int id );
        Meta::GenrePtr genreById( int id );

        //Override some stuff to be able to hande id mappings

        void addTrack( Meta::TrackPtr trackPtr );
        void addArtist( Meta::ArtistPtr artistPtr);
        void addAlbum ( Meta::AlbumPtr albumPtr );
        void addGenre( Meta::GenrePtr genrePtr);

        //TODO:
        //void setTrackMap( TrackMap map ) { m_trackMap = map; }
        //void setArtistMap( ArtistMap map ) { m_artistMap = map; }
        //void setAlbumMap( AlbumMap map ) { m_albumMap = map; }
        //void setGenreMap( GenreMap map ) { m_genreMap = map; }

        ServiceBase * service();

    private:
        ServiceBase * m_service;

        ServiceMetaFactory * m_metaFactory;

        QString m_collectionId;
        QString m_prettyName;

        TrackIdMap m_trackIdMap;
        ArtistIdMap m_artistIdMap;
        AlbumIdMap m_albumIdMap;
        GenreIdMap m_genreIdMap;
};

#endif
