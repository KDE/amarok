/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_FILEBROWSER_P_H
#define AMAROK_FILEBROWSER_P_H

#include "FileBrowser.h"

#include "browsers/BrowserBreadcrumbItem.h"

#include <KDirModel>
#include <KFilePlacesModel>

#include <QApplication>
#include <QFontMetrics>
#include <QStack>

class QSortFilterProxyModel;
class DirBrowserModel;
class SearchWidget;
class FileView;
class QAction;
class KFilePlacesModel;
class DirPlaylistTrackFilterProxyModel;

template<typename T>
class UniqueStack : public QStack<T>
{
    public:
        inline void push( const T &t )
        {
            if( QStack<T>::isEmpty() || t != QStack<T>::top() )
                QStack<T>::push( t );
        }
};

class FileBrowser::Private
{
public:
    Private( FileBrowser *parent );
    ~Private();

    void readConfig();
    void writeConfig();
    void restoreHeaderState();
    void saveHeaderState();

    void updateNavigateActions();
    BreadcrumbSiblingList siblingsForDir( const QUrl &path );

    void updateHeaderState();

    QList<QAction *> columnActions; //!< Maintains the mapping action<->column

    KFilePlacesModel *bottomPlacesModel;
    QSortFilterProxyModel *placesModel;

    DirBrowserModel *kdirModel;
    DirPlaylistTrackFilterProxyModel *mimeFilterProxyModel;

    SearchWidget *searchWidget;

    QUrl currentPath;
    FileView *fileView;

    QAction *upAction;
    QAction *homeAction;
    QAction *refreshAction;

    QAction *backAction;
    QAction *forwardAction;

    UniqueStack<QUrl> backStack;
    UniqueStack<QUrl> forwardStack;

private:
    void restoreDefaultHeaderState();

    FileBrowser *const q;
};

class DirBrowserModel : public KDirModel
{
    Q_OBJECT

public:
    explicit DirBrowserModel( QObject *parent = nullptr ) : KDirModel( parent )
    {
    }

    virtual ~DirBrowserModel() {}

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override
    {
        if( role == Qt::SizeHintRole )
            return QSize( 1, QFontMetrics( QFont() ).height() + 4 );
        else
            return KDirModel::data( index, role );
    }
};

class FilePlacesModel : public KFilePlacesModel
{
    Q_OBJECT

public:
    explicit FilePlacesModel( QObject *parent = nullptr ) : KFilePlacesModel( parent )
    {
    }

    virtual ~FilePlacesModel() {}

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override
    {
        if( role == Qt::SizeHintRole )
            return QSize( 1, QFontMetrics( QFont() ).height() + 4 );
        else
            return KFilePlacesModel::data( index, role );
    }
};

#endif /* AMAROK_FILEBROWSER_P_H */
