/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_DYNAMICBIASMODEL_H
#define AMAROK_DYNAMICBIASMODEL_H

#include "BiasedPlaylist.h"

#include <QAbstractItemModel>
#include <QListView>

namespace PlaylistBrowserNS
{
    class BiasBoxWidget;

    /** This model is used in the playlist browser do display a biased Playlist.
    */
    class DynamicBiasModel : public QAbstractItemModel
    {
        Q_OBJECT

        public:
            enum Roles
            {
                WidgetRole = 0xf00d
            };

            DynamicBiasModel( QListView* listView );
            void setPlaylist( Dynamic::BiasedPlaylist *playlist );

            ~DynamicBiasModel();

            QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
            QModelIndex index ( int row, int column,
                                const QModelIndex & parent = QModelIndex() ) const;

            QModelIndex indexOf( BiasBoxWidget* );

            QModelIndex parent ( const QModelIndex & index ) const;

            int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
            int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

        signals:
            void playlistModified( Dynamic::BiasedPlaylist *playlist );

        public slots:
            void widgetChanged( QWidget* widget );
            // void removeBias( Dynamic::AbstractBias* );
            void biasChanged( Dynamic::AbstractBias *bias );
            /*
            void appendGlobalBias();
            void appendCustomBias();
            void appendNormalBias();
            */

        private:
            // void appendBias( Dynamic::AbstractBias* );
            void clearWidgets();

            QListView* m_listView;

            Dynamic::BiasedPlaylist* m_playlist;
            QList<BiasBoxWidget*> m_widgets;
    };
}

#endif

