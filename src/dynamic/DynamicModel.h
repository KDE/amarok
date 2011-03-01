/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009-2010 Leo Franchi <lfranchi@kde.org>                               *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "DynamicPlaylist.h"

#include "amarok_export.h" // we are exporting it for the tests

#include <QAbstractItemModel>
#include <QList>
#include <QString>

namespace Dynamic {

class BiasedPlaylist;
class AbstractBias;

class AMAROK_EXPORT DynamicModel : public QAbstractItemModel
{
    Q_OBJECT
    public:
        // role used for the model
        enum Roles
        {
            WidgetRole = 0xf00d,
            PlaylistRole = 0xf00e,
            BiasRole = 0xf00f
        };

        static DynamicModel* instance();

        ~DynamicModel();

        // void changePlaylist( int i );

        /** Returns the currently active playlist.
            Don't free this pointer
        */
        Dynamic::DynamicPlaylist* activePlaylist() const;
        int activePlaylistIndex() const;

        /** Find the playlist with name, make it active and return it */
        Dynamic::DynamicPlaylist* setActivePlaylist( int );

        int defaultPlaylistIndex() const;

        int playlistIndex( Dynamic::DynamicPlaylist* playlist ) const;

        bool isActiveUnsaved() const;
        bool isActiveDefault() const;

        // --- QAbstractItemModel functions ---
        QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        QModelIndex parent(const QModelIndex& index) const;
        int rowCount( const QModelIndex & parent = QModelIndex() ) const;
        int columnCount( const QModelIndex & parent = QModelIndex() ) const;
        // ---

        void saveCurrentPlaylists();
        void loadCurrentPlaylists();

    signals:
        void activeChanged( int index ); // active row changed

    public slots:
        void playlistChanged( Dynamic::DynamicPlaylist* );

        /** Stores the active playlist under the given new title. */
        void saveActive( const QString& newTitle );

        /** Removes the active playlist. */
        void removeActive();

    private:
        // two functions to search for parents
        QModelIndex parent( Dynamic::BiasedPlaylist* list, Dynamic::AbstractBias* bias ) const;
        QModelIndex parent( Dynamic::AbstractBias* parent, Dynamic::AbstractBias* bias ) const;

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

