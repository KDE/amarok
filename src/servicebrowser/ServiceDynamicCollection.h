/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef SERVICEDYNAMICCOLLECTION_H
#define SERVICEDYNAMICCOLLECTION_H

#include "amarok_export.h"
#include "servicecollection.h"

typedef QMap<int, Meta::TrackPtr> TrackIdMap;
typedef QMap<int, Meta::ArtistPtr> ArtistIdMap;
typedef QMap<int, Meta::AlbumPtr> AlbumIdMap;
typedef QMap<int, Meta::GenrePtr> GenreIdMap;


/**
A specialized collection used for services that dynamically fetch their data from somewhere ( a web service, an external program, etc....)

	@author 
*/
class AMAROK_EXPORT ServiceDynamicCollection : public ServiceCollection
{
public:

    Q_OBJECT
    public:
        ServiceDynamicCollection( const QString &id, const QString &prettyName );
        virtual ~ServiceDynamicCollection();

        virtual void startFullScan() {} //TODO
        virtual QueryMaker* queryMaker() = 0;

        virtual QString collectionId()  const;
        virtual QString prettyName() const;

        virtual QStringList query( const QString &query ) { Q_UNUSED( query ); return QStringList(); }
        virtual int insert( const QString &statement, const QString &table ) { Q_UNUSED( statement ); Q_UNUSED( table ); return 0; }

        virtual QString escape( QString text ) const { Q_UNUSED( text ); return QString(); }


        Meta::TrackPtr trackById( int id );
        Meta::AlbumPtr albumById( int id );
        Meta::ArtistPtr artistById( int id );
        Meta::GenrePtr genreById( int id );


        //Override some stuff to be able to hande id mappings
        

        void addTrack( const QString &key, Meta::TrackPtr trackPtr );
        void addArtist( const QString &key, Meta::ArtistPtr artistPtr);
        void addAlbum ( const QString &key, Meta::AlbumPtr albumPtr );
        void addGenre( const QString &key, Meta::GenrePtr genrePtr);

        //TODO:
        //void setTrackMap( TrackMap map ) { m_trackMap = map; }
        //void setArtistMap( ArtistMap map ) { m_artistMap = map; }
        //void setAlbumMap( AlbumMap map ) { m_albumMap = map; }
        //void setGenreMap( GenreMap map ) { m_genreMap = map; }

    private:
        ServiceMetaFactory * m_metaFactory;

        QString m_collectionId;
        QString m_prettyName;

        TrackIdMap m_trackIdMap;
        ArtistIdMap m_artistIdMap;
        AlbumIdMap m_albumIdMap;
        GenreIdMap m_genreIdMap;

};

#endif
