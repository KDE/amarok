/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
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

#ifndef DYNAMICMODEL_H
#define DYNAMICMODEL_H

#include "BiasedPlaylist.h"
#include "DynamicPlaylist.h"


#include <QAbstractItemModel>
#include <QDomDocument>
#include <QDomElement>
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QString>

namespace Dynamic
{
    class Bias;
}


namespace PlaylistBrowserNS {

class DynamicModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        static DynamicModel* instance();

        ~DynamicModel();

        void loadPlaylists();

        void enable( bool enable );
        void changePlaylist( int i );

        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

        Dynamic::DynamicPlaylistPtr defaultPlaylist();
        Dynamic::DynamicPlaylistPtr activePlaylist();
        int activePlaylistIndex();

        int playlistIndex( const QString& ) const;

        QModelIndex index ( int row, int column,
                const QModelIndex & parent = QModelIndex() ) const;

        bool isActiveUnsaved() const;
        bool isActiveDefault() const;


        QModelIndex parent ( const QModelIndex & index ) const;

        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

        /// find the playlist with name, make it active and return it
        Dynamic::DynamicPlaylistPtr setActivePlaylist( const QString& name );
        Dynamic::DynamicPlaylistPtr setActivePlaylist( int );

        QDomDocument savedPlaylistDoc() { return m_savedPlaylists; }

        void saveCurrent();
        
    signals:
        void activeChanged(); // active row changed
        void changeActive( int );  // request that active change
        void enableDynamicMode( bool );

    public slots:
        void playlistModified( Dynamic::BiasedPlaylistPtr );
        void saveActive( const QString& newTitle );
        void removeActive();

    private slots:
        void universeNeedsUpdate();
        void savePlaylists( bool final = true );

    private:
        Dynamic::DynamicPlaylistPtr createDefaultPlaylist();
        void insertPlaylist( Dynamic::DynamicPlaylistPtr );
        void computeUniverseSet();
        void loadAutoSavedPlaylist();
        
        DynamicModel();
        static DynamicModel* s_instance;

        int m_activePlaylist;
        int m_defaultPlaylist;

        bool m_activeUnsaved;

        QDomDocument m_savedPlaylists;
        QDomElement m_savedPlaylistsRoot;

        QHash< QString, Dynamic::DynamicPlaylistPtr >    m_playlistHash;
        Dynamic::DynamicPlaylistList                     m_playlistList;
        QList<QDomElement>                               m_playlistElements;

};

}

#endif

