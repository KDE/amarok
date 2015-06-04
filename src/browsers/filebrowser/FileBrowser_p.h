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
#include <KGlobalSettings>

#include <QFontMetrics>
#include <QStack>

class QSortFilterProxyModel;
class DirBrowserModel;
class SearchWidget;
class FileView;
class KAction;
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

    KAction *upAction;
    KAction *homeAction;
    KAction *refreshAction;

    KAction *backAction;
    KAction *forwardAction;

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
    DirBrowserModel( QObject *parent = 0 ) : KDirModel( parent )
    {
        updateRowHeight();
        connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), SLOT(updateRowHeight()) );
    }

    virtual ~DirBrowserModel() {}

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const
    {
        if( role == Qt::SizeHintRole )
            return QSize( 1, rowHeight );
        else
            return KDirModel::data( index, role );
    }

private Q_SLOTS:
    void updateRowHeight()
    {
        QFont font;
        rowHeight = QFontMetrics( font ).height() + 4;
    }

private:
    int rowHeight;
};

class FilePlacesModel : public KFilePlacesModel
{
    Q_OBJECT

public:
    FilePlacesModel( QObject *parent = 0 ) : KFilePlacesModel( parent )
    {
        updateRowHeight();
        connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), SLOT(updateRowHeight()) );
    }

    virtual ~FilePlacesModel() {}

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const
    {
        if( role == Qt::SizeHintRole )
            return QSize( 1, rowHeight );
        else
            return KFilePlacesModel::data( index, role );
    }

private Q_SLOTS:
    void updateRowHeight()
    {
        QFont font;
        rowHeight = QFontMetrics( font ).height() + 4;
    }

private:
    int rowHeight;
};

#endif /* AMAROK_FILEBROWSER_P_H */
