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

#include <KDirModel>
#include <KGlobalSettings>

#include <QFontMetrics>
#include <QTimer>

class DirBrowserModel;
class FileView;
class KAction;
class KFilePlacesModel;
class MimeTypeFilterProxyModel;

class FileBrowser::Private
{
public:
    Private( FileBrowser *parent );
    ~Private();

    void readConfig();
    void writeConfig();
    QStringList siblingsForDir( const QString &path );

    void slotSaveHeaderState();

    QList< QAction * > columnActions; //!< Maintains the mapping action<->column

    DirBrowserModel *kdirModel;
    KFilePlacesModel *placesModel;

    MimeTypeFilterProxyModel *mimeFilterProxyModel;

    QTimer filterTimer;
    QString currentFilter;
    QString currentPath;
    FileView *fileView;

    KAction *upAction;
    KAction *homeAction;
    KAction *placesAction;

    bool showingPlaces;

private:
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

private slots:
    void updateRowHeight()
    {
        QFont font;
        rowHeight = QFontMetrics( font ).height() + 4;
    }

private:
    int rowHeight;
};

#endif /* AMAROK_FILEBROWSER_P_H */
