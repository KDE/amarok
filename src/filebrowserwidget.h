/***************************************************************************
 *   Copyright (c) 2007  Dan Meltzer <hydrogen@notyetimplemented.com>      *
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

#ifndef FILEBROWSERWIDGET_H
#define FILEBROWSERWIDGET_H

#include <KVBox> //Base Class

#include <QModelIndex> //Stack Allocated

class QListView;

class KDirModel;
class KDirSortFilterProxyModel;
class KUrlComboBox;

class FileBrowserWidget : public KVBox
{
    Q_OBJECT
    public:
        FileBrowserWidget( const char *name );

        ~FileBrowserWidget();

    private slots:
        void setRootDirectory( const QString &text );
        void setRootDirectory( const QModelIndex &index );
    private:
        KDirModel *m_model;
        KDirSortFilterProxyModel *m_sortModel;
        QListView *m_view;
        KUrlComboBox *m_combo;
};

#endif
