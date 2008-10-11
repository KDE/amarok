/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef METASQLPLAYLIST_H
#define METASQLPLAYLIST_H



#include "Playlist.h"
//Changed to fix a strange build error in AlbumModel.cpp
//#include "SqlPlaylistViewItem.h"
#include <browsers/playlistbrowser/SqlPlaylistViewItem.h>


namespace Meta {

class SqlPlaylist;

typedef KSharedPtr<SqlPlaylist> SqlPlaylistPtr;
typedef QList<SqlPlaylistPtr> SqlPlaylistList;

/**
    A playlist that saves and loads itself from the Amarok database

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class SqlPlaylist : public SqlPlaylistViewItem, public Playlist
{
public:
    //SqlPlaylist( int id );
    SqlPlaylist( const QString & name, const TrackList& tracks, SqlPlaylistGroupPtr parent );
    SqlPlaylist( const QStringList & resultRow, SqlPlaylistGroupPtr parent );

    ~SqlPlaylist();

    bool saveToDb( bool tracks = true );

    /* Playlist virtual functions */
    virtual QString name() const { return m_name; }
    QString prettyName() const { return m_name; }
    virtual QString description() const { return m_description; }
    virtual void rename( const QString &name );

    void reparent( SqlPlaylistGroupPtr parent );
    
    int id();
    

    /** returns all tracks in this playlist */
    virtual TrackList tracks();

    bool hasCapabilityInterface( Meta::Capability::Type type ) const { Q_UNUSED( type ); return false; }
    Capability* asCapabilityInterface( Capability::Type type ) { Q_UNUSED( type ); return 0; }

    virtual SqlPlaylistGroupPtr parent() const { return m_parent; }

    virtual void removeFromDb();

    //bool load();

private:

    void loadTracks();
    void saveTracks();

    int m_dbId;
    SqlPlaylistGroupPtr m_parent;
    Meta::TrackList m_tracks;
    QString m_name;
    QString m_description;

    bool m_tracksLoaded;

};

}

Q_DECLARE_METATYPE( Meta::SqlPlaylistPtr )
Q_DECLARE_METATYPE( Meta::SqlPlaylistList )

#endif
