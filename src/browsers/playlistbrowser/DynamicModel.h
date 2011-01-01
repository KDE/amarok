/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
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
#include <QList>
#include <QMutex>
#include <QString>

namespace PlaylistBrowserNS {

class DynamicModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        static DynamicModel* instance();

        ~DynamicModel();

        // void enable( bool enable );
        void changePlaylist( int i );

        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

        /** Returns the currently active playlist.
            Don't free this pointer
        */
        Dynamic::DynamicPlaylist* activePlaylist();
        int activePlaylistIndex() const;
        int defaultPlaylistIndex() const;

        int playlistIndex( const QString& ) const;

        QModelIndex index ( int row, int column,
                            const QModelIndex & parent = QModelIndex() ) const;

        bool isActiveUnsaved() const;
        bool isActiveDefault() const;

        QModelIndex parent ( const QModelIndex & index ) const;

        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

        /// find the playlist with name, make it active and return it
        // Dynamic::DynamicPlaylist* setActivePlaylist( const QString& name );
        Dynamic::DynamicPlaylist* setActivePlaylist( int );

        void saveCurrentPlaylists();
        void loadCurrentPlaylists();

    signals:
        void activeChanged( int index ); // active row changed
        /*
        void changeActive( int );  // request that active change
        void enableDynamicMode( bool );
        */

    public slots:
        void playlistChanged( Dynamic::DynamicPlaylist* );
        void saveActive( const QString& newTitle );
        void removeActive();

    private:
        void savePlaylists();
        void loadPlaylists();
        bool savePlaylists( const QString &filename );
        bool loadPlaylists( const QString &filename );
        void initPlaylists();

        DynamicModel();
        static DynamicModel* s_instance;

        int m_activePlaylistIndex;

        /** Contains all the dynamic playlists.
            The first playlist is always the default one so we always have
            at least one playlist.
        */
        QList<Dynamic::DynamicPlaylist*> m_playlists;

        bool m_activeUnsaved;
};

}

#endif

