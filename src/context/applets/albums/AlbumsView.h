/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_ALBUMSVIEW_H
#define AMAROK_ALBUMSVIEW_H

#include <QGraphicsProxyWidget>

#include "core/meta/Meta.h"

class QTreeView;
class QAbstractItemModel;
class QGraphicsSceneContextMenuEvent;
class QModelIndex;

class AlbumsView : public QGraphicsProxyWidget
{
    Q_OBJECT
    Q_PROPERTY( QAbstractItemModel* model READ model WRITE setModel )
    Q_PROPERTY( QTreeView* nativeWidget READ nativeWidget )

public:
    explicit AlbumsView( QGraphicsWidget *parent = 0 );
    ~AlbumsView();

    /**
     * Sets a model for this weather view
     *
     * @arg model the model to display
     */
    void setModel( QAbstractItemModel *model );

    /**
     * @return the model shown by this view
     */
    QAbstractItemModel *model();

    /**
     * @return the native widget wrapped by this AlbumsView
     */
    QTreeView* nativeWidget() const;

protected:
    void contextMenuEvent( QGraphicsSceneContextMenuEvent *event );
    void resizeEvent( QGraphicsSceneResizeEvent *event );
    
private slots:
    void itemClicked( const QModelIndex &index );
    void slotAppendSelected();
    void slotEditSelected();
    void slotPlaySelected();
    void slotQueueSelected();

private:
    Meta::TrackList getSelectedTracks() const;
    
};

#endif // multiple inclusion guard
