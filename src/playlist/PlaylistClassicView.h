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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#ifndef PLAYLISTCLASSICVIEW_H
#define PLAYLISTCLASSICVIEW_H

#include <QTreeView>

/**
A 'simple' classical playlist view for the purists out there

	Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
namespace Playlist {

    class Model;
    
    class ClassicView : public QTreeView {
    Q_OBJECT
    public:
        ClassicView( QWidget * parent );
    
        ~ClassicView();
    
        void  setModel( Playlist::Model *model );

private:

    Playlist::Model * m_model;

private slots:

    void play( const QModelIndex & index );

    
    };


}

#endif
