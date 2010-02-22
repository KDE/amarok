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
#include "FileView.h"
#include "MimeTypeFilterProxyModel.h"

#include "widgets/SearchWidget.h"

#include <KDirModel>
#include <QTimer>

class FileBrowser : public BrowserCategory
{
    Q_OBJECT
public:
    FileBrowser( const char * name, QWidget *parent );
    ~FileBrowser();

    virtual void setupAddItems();
    virtual void polish();
    
    virtual QString prettyName() const;

    /**
    * Navigate to a specific directory
    */
    void setDir( const QString &dir );

protected slots:
    void itemActivated( const QModelIndex &index );
    
    void slotSetFilterTimeout();
    void slotFilterNow();

    void addItemActivated( const QString &callback );

    virtual void reActivate();

    /**
     * Shows/hides the columns as selected in the context menu of the header of the
     * file view.
     * @param toggled the visibility state of a column in the context menu.
     */
    void toggleColumn( bool toggled);


private:
    void readConfig();
    void writeConfig();

    QList< QAction * >       m_columnActions; //! Maintains the mapping action<->column

    QStringList siblingsForDir( const QString &path );
    
    SearchWidget             *m_searchWidget;
    KDirModel                *m_kdirModel;
    MimeTypeFilterProxyModel *m_mimeFilterProxyModel;
    DirectoryLoader          *m_directoryLoader;

    QTimer                    m_filterTimer;
    QString                   m_currentFilter;
    QString                   m_currentPath;
    FileView                 *m_fileView;
        
};

#endif // FILEBROWSERMKII_H
