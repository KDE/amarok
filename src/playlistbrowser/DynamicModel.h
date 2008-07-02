/*
    Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DYNAMICMODEL_H
#define DYNAMICMODEL_H

#include "DynamicPlaylist.h"

#include <QAbstractItemModel>
#include <QDomElement>
#include <QHash>
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
    
     
        QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

        Dynamic::DynamicPlaylistPtr retrievePlaylist( QString );
        Dynamic::DynamicPlaylistPtr retrieveDefaultPlaylist();
        int retrievePlaylistIndex( QString );

        QModelIndex index ( int row, int column,
                const QModelIndex & parent = QModelIndex() ) const;


        QModelIndex parent ( const QModelIndex & index ) const;

        int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        int columnCount ( const QModelIndex & parent = QModelIndex() ) const;


    private:
        Dynamic::Bias* createBias( QDomElement );
        

        DynamicModel();
        static DynamicModel* s_instance;

        Dynamic::DynamicPlaylistPtr m_defaultPlaylist;

        QHash< QString, Dynamic::DynamicPlaylistPtr >    m_playlistHash;
        Dynamic::DynamicPlaylistList                     m_playlistList;
};

}

#endif

