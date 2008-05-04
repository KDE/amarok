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

#ifndef PLAYLISTCLASSICVIEW_H
#define PLAYLISTCLASSICVIEW_H

#include <KLineEdit>

#include <QTreeView>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

/**
A 'simple' classical playlist view for the purists out there

	Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
namespace Playlist {

    class Model;
    
    class ClassicView : public QWidget {
    Q_OBJECT
    public:
        ClassicView( QWidget * parent );
    
        ~ClassicView();
    
private:

        QTreeView *m_treeView;
        QVBoxLayout *m_layout;
        KLineEdit *m_lineEdit;

    Playlist::Model * m_model;
    QSortFilterProxyModel *m_proxyModel;
    QPersistentModelIndex *m_contextIndex;


private slots:

    void playTrack( );
    void playTrack( const QModelIndex &);
    void removeSelection();
    void editTrackInformation();


protected:
virtual void contextMenuEvent( QContextMenuEvent *event );
    
    };


}

#endif
