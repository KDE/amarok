//
// C++ Interface: servicedynamiccollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SERVICEDYNAMICCOLLECTION_H
#define SERVICEDYNAMICCOLLECTION_H

#include "amarok_export.h"
#include <servicecollection.h>

typedef QMap<int, Meta::TrackPtr> TrackIdMap;
typedef QMap<int, Meta::ArtistPtr> ArtistIdMap;
typedef QMap<int, Meta::AlbumPtr> AlbumIdMap;
typedef QMap<int, Meta::GenrePtr> GenreIdMap;


/**
A specialised collection used for services that dynamically fetch their data from somewhere ( a web service, an external program, etc....)

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
        

        void addTrack( QString key, Meta::TrackPtr trackPtr );
        void addArtist( QString key, Meta::ArtistPtr artistPtr);
        void addAlbum ( QString key, Meta::AlbumPtr albumPtr );
        void addGenre( QString key, Meta::GenrePtr genrePtr);

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
