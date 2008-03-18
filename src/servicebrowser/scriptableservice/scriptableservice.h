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

#ifndef AMAROKSCRIPTABLESERVICE_H
#define AMAROKSCRIPTABLESERVICE_H


#include "amarok.h"
#include "../servicebase.h"
#include "servicemetabase.h"
#include "DynamicScriptableServiceCollection.h"


typedef QMap<int, Meta::TrackPtr> TrackIdMap;
typedef QMap<int, Meta::ArtistPtr> ArtistIdMap;
typedef QMap<int, Meta::AlbumPtr> AlbumIdMap;
typedef QMap<int, Meta::GenrePtr> GenreIdMap;
typedef QMap<int, Meta::ComposerPtr> ComposerIdMap;
typedef QMap<int, Meta::YearPtr> YearIdMap;


class ScriptableService : public ServiceBase
{
    Q_OBJECT

public:

     /**
     * Constructor
     */
    ScriptableService( const QString &name );
    /**
     * Destructor
     */
    ~ScriptableService() { }

    void polish() {}

    ServiceCollection * collection();
    void setCollection( DynamicScriptableServiceCollection * collection );


    int addTrack( Meta::ServiceTrack * track, int albumId );
    int addAlbum( Meta::ServiceAlbum * album );
    int addArtist( Meta::ServiceArtist * artist );

    void donePopulating( int parentId );

private slots:


    //void treeItemSelected( const QModelIndex & index );
    //void infoChanged ( QString infoHtml );


private:

    DynamicScriptableServiceCollection * m_collection;
    int m_trackIdCounter;
    int m_albumIdCounter;
    int m_artistIdCounter;

    TrackIdMap trackIdMap;
    AlbumIdMap albumIdMap;
    ArtistIdMap artistIdMap;
};


#endif
