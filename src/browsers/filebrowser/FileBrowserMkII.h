/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef FILEBROWSERMKII_H
#define FILEBROWSERMKII_H

#include "BrowserCategory.h"
#include "DirectoryLoader.h"
#include "FileTreeView.h"

#include "widgets/SearchWidget.h"

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QTimer>

class FileBrowserMkII : public BrowserCategory
{
    Q_OBJECT
public:
    FileBrowserMkII( const char * name, QWidget *parent );

protected slots:
    void itemActivated( const QModelIndex &index );
    
    void  slotSetFilterTimeout();
    void slotFilterNow();

private:
    SearchWidget             *m_searchWidget;
    QFileSystemModel         *m_fileSystemModel;
    QSortFilterProxyModel    *m_proxyModel;
    DirectoryLoader          *m_directoryLoader;

    QTimer                    m_filterTimer;
    QString                   m_currentFilter;

    FileTreeView             *m_fileView; 
        
};

#endif // FILEBROWSERMKII_H
